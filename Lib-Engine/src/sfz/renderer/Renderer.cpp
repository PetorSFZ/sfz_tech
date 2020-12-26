// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "sfz/renderer/Renderer.hpp"

#include <utility> // std::swap()

#include <SDL.h>

#include <skipifzero.hpp>
#include <skipifzero_math.hpp>

#include <ZeroG.h>

#include <ZeroG-ImGui.hpp>

#include "sfz/debug/ProfilingStats.hpp"
#include "sfz/Logging.hpp"
#include "sfz/config/GlobalConfig.hpp"
#include "sfz/renderer/GpuTextures.hpp"
#include "sfz/renderer/RendererConfigParser.hpp"
#include "sfz/renderer/RendererState.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/resources/ResourceManager.hpp"

namespace sfz {

// Statics
// ------------------------------------------------------------------------------------------------

static const char* stripFilePath(const char* file) noexcept
{
	const char* strippedFile1 = std::strrchr(file, '\\');
	const char* strippedFile2 = std::strrchr(file, '/');
	if (strippedFile1 == nullptr && strippedFile2 == nullptr) {
		return file;
	}
	else if (strippedFile2 == nullptr) {
		return strippedFile1 + 1;
	}
	else {
		return strippedFile2 + 1;
	}
}

// Renderer: State methods
// ------------------------------------------------------------------------------------------------

bool Renderer::init(
	SDL_Window* window,
	const ImageViewConst& fontTexture,
	sfz::Allocator* allocator) noexcept
{
	this->destroy();
	mState = allocator->newObject<RendererState>(sfz_dbg("RendererState"));
	mState->allocator = allocator;
	mState->window = window;

	// Settings
	GlobalConfig& cfg = getGlobalConfig();
	mState->vsync =
		cfg.sanitizeBool("Renderer", "vsync", true, false);
	mState->flushPresentQueueEachFrame =
		cfg.sanitizeBool("Renderer", "flushPresentQueueEachFrame", false, false);
	mState->flushCopyQueueEachFrame =
		cfg.sanitizeBool("Renderer", "flushCopyQueueEachFrame", false, false);
	mState->emitDebugEvents =
		cfg.sanitizeBool("Renderer", "emitDebugEvents", false, true);

	// Initialize fences
	mState->frameFences.init(mState->frameLatency, [](zg::Fence& fence) {
		CHECK_ZG fence.create();
	});

	// Get window resolution
	mState->windowRes;
	SDL_GL_GetDrawableSize(window, &mState->windowRes.x, &mState->windowRes.y);

	// Get command queues
	mState->presentQueue = zg::CommandQueue::getPresentQueue();
	mState->copyQueue = zg::CommandQueue::getCopyQueue();

	// Initialize profiler
	{
		ZgProfilerCreateInfo createInfo = {};
		createInfo.maxNumMeasurements = 1024;
		CHECK_ZG mState->profiler.create(createInfo);
		mState->frameMeasurementIds.init(mState->frameLatency);
	}

	// Initialize hashmaps for resources
	mState->meshes.init(512, mState->allocator, sfz_dbg(""));

	// Initialize ImGui rendering state
	mState->imguiScaleSetting =
		cfg.sanitizeFloat("Imgui", "scale", true, FloatBounds(1.5f, 1.0f, 3.0f));
	sfz_assert(fontTexture.type == ImageType::R_U8);
	ZgImageViewConstCpu zgFontTextureView = {};
	zgFontTextureView.format = ZG_TEXTURE_FORMAT_R_U8_UNORM;
	zgFontTextureView.data = fontTexture.rawData;
	zgFontTextureView.width = fontTexture.width;
	zgFontTextureView.height = fontTexture.height;
	zgFontTextureView.pitchInBytes = fontTexture.width * sizeof(uint8_t);
	bool imguiInitSuccess = CHECK_ZG zg::imguiInitRenderState(
		mState->imguiRenderState,
		mState->frameLatency,
		mState->allocator,
		mState->copyQueue,
		zgFontTextureView);
	if (!imguiInitSuccess) {
		this->destroy();
		return false;
	}

	return true;
}

bool Renderer::loadConfiguration(const char* jsonConfigPath) noexcept
{
	if (!this->active()) {
		sfz_assert(false);
		return false;
	}

	// Parse config
	bool parseSuccess = parseRendererConfig(*mState, jsonConfigPath);
	if (!parseSuccess) {
		// TODO: Maybe this is a bit too extreme?
		this->destroy();
		return false;
	}

	// Initialize profiling stats
	ProfilingStats& stats = getProfilingStats();
	stats.createCategory("gpu", 300, 66.7f, "ms", "frame", 20.0f,
		StatsVisualizationType::FIRST_INDIVIDUALLY_REST_ADDED);
	stats.createLabel("gpu", "frametime", vec4(1.0f, 0.0f, 0.0f, 1.0f), 0.0f);
	for (const StageGroup& group : mState->configurable.presentStageGroups) {
		const char* label = group.groupName.str();
		stats.createLabel("gpu", label);
	}
	stats.createLabel("gpu", "imgui");

	return true;
}

void Renderer::loadDummyConfiguration() noexcept
{
	if (!this->active()) {
		sfz_assert_hard(false);
	}

	// Parse dummy config
	bool parseSuccess = parseRendererConfig(*mState, "res_ph/shaders/dummy_renderer_config.json");
	if (!parseSuccess) {
		this->destroy();
		sfz_assert_hard(false);
	}

	// Initialize profiling stats
	ProfilingStats& stats = getProfilingStats();
	stats.createCategory("gpu", 300, 66.7f, "ms", "frame", 20.0f,
		StatsVisualizationType::FIRST_INDIVIDUALLY_REST_ADDED);
	stats.createLabel("gpu", "frametime", vec4(1.0f, 0.0f, 0.0f, 1.0f), 0.0f);
	stats.createLabel("gpu", "imgui", vec4(1.0f, 1.0f, 0.0f, 1.0f), 0.0f);

	// Set renderer to dummy mode
	mState->dummyMode = true;
}

void Renderer::swap(Renderer& other) noexcept
{
	std::swap(this->mState, other.mState);
}

void Renderer::destroy() noexcept
{
	if (mState != nullptr) {

		// Flush queues
		CHECK_ZG mState->presentQueue.flush();
		CHECK_ZG mState->copyQueue.flush();

		// Destroy ImGui renderer
		zg::imguiDestroyRenderState(mState->imguiRenderState);
		mState->imguiRenderState = nullptr;

		// Destroy all textures and meshes
		this->removeAllMeshesGpuBlocking();

		// Deallocate rest of state
		sfz::Allocator* allocator = mState->allocator;
		allocator->deleteObject(mState);
	}
	mState = nullptr;
}

// Renderer: Getters
// ------------------------------------------------------------------------------------------------

uint64_t Renderer::currentFrameIdx() const noexcept
{
	return mState->currentFrameIdx;
}

vec2_i32 Renderer::windowResolution() const noexcept
{
	return mState->windowRes;
}

void Renderer::frameTimeMs(uint64_t& frameIdxOut, float& frameTimeMsOut) const noexcept
{
	frameIdxOut = mState->lastRetrievedFrameTimeFrameIdx;
	frameTimeMsOut = mState->lastRetrievedFrameTimeMs;
}

// Renderer: ImGui UI methods
// ------------------------------------------------------------------------------------------------

void Renderer::renderImguiUI() noexcept
{
	mState->ui.render(*mState);
}

// Renderer: Resource methods
// ------------------------------------------------------------------------------------------------

bool Renderer::uploadTextureBlocking(
	strID id, const ImageViewConst& image, bool generateMipmaps) noexcept
{
	// Error out and return false if texture already exists
	ResourceManager& resources = getResourceManager();
	if (resources.getTextureHandle(id) != NULL_HANDLE) return false;

	const char* fileName = stripFilePath(id.str());
	uint32_t numMipmaps = 0;
	zg::Texture texture = textureAllocateAndUploadBlocking(
		fileName,
		image,
		mState->allocator,
		mState->copyQueue,
		generateMipmaps,
		numMipmaps);
	sfz_assert(texture.valid());

	// Fill texture item with info and store it
	TextureItem item;
	item.texture = std::move(texture);
	item.format = toZeroGImageFormat(image.type);
	item.width = image.width;
	item.height = image.height;
	item.numMipmaps = numMipmaps;
	resources.addTexture(id, std::move(item));

	return true;
}

bool Renderer::textureLoaded(strID id) const noexcept
{
	ResourceManager& resources = getResourceManager();
	return resources.getTextureHandle(id) != NULL_HANDLE;
}

void Renderer::removeTextureGpuBlocking(strID id) noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	ResourceManager& resources = getResourceManager();
	resources.removeTexture(id);
}

bool Renderer::uploadMeshBlocking(strID id, const Mesh& mesh) noexcept
{
	// Error out and return false if mesh already exists
	sfz_assert(id.isValid());
	if (mState->meshes.get(id) != nullptr) return false;

	// Allocate memory for mesh
	const char* fileName = stripFilePath(id.str());
	GpuMesh gpuMesh = gpuMeshAllocate(fileName, mesh, mState->allocator);

	// Upload memory to mesh
	gpuMeshUploadBlocking(
		gpuMesh, mesh, mState->allocator, mState->copyQueue);

	// Store mesh
	mState->meshes.put(id, std::move(gpuMesh));

	return true;
}

bool Renderer::meshLoaded(strID id) const noexcept
{
	const sfz::GpuMesh* mesh = mState->meshes.get(id);
	return mesh != nullptr;
}

void Renderer::removeMeshGpuBlocking(strID id) noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	// Return if mesh is not loaded in first place
	sfz::GpuMesh* mesh = mState->meshes.get(id);
	if (mesh == nullptr) return;

	// Ensure all GPU operations in progress are finished
	CHECK_ZG mState->presentQueue.flush();
	CHECK_ZG mState->copyQueue.flush();

	// Destroy mesh
	mState->meshes.remove(id);
}

void Renderer::removeAllMeshesGpuBlocking() noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	// Ensure all GPU operations in progress are finished
	CHECK_ZG mState->presentQueue.flush();
	CHECK_ZG mState->copyQueue.flush();

	// Destroy all meshes
	mState->meshes.clear();
}

// Renderer: Methods
// ------------------------------------------------------------------------------------------------

void Renderer::frameBegin() noexcept
{
	// Increment frame index
	mState->currentFrameIdx += 1;

	// Wait on fence to ensure we have finished rendering frame that previously used this data
	CHECK_ZG mState->frameFences.data(mState->currentFrameIdx).waitOnCpuBlocking();

	// Get frame profiling data for frame that was previously rendered using these resources
	ProfilingStats& stats = getProfilingStats();
	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);
	if (frameIds.frameId != ~0ull) {
		CHECK_ZG mState->profiler.getMeasurement(frameIds.frameId, mState->lastRetrievedFrameTimeMs);
		mState->lastRetrievedFrameTimeFrameIdx = mState->currentFrameIdx - mState->frameLatency;
		stats.addSample("gpu", "frametime",
			mState->lastRetrievedFrameTimeFrameIdx, mState->lastRetrievedFrameTimeMs);
	}
	for (const GroupProfilingID& groupId : frameIds.groupIds) {
		uint64_t frameIdx = mState->lastRetrievedFrameTimeFrameIdx;
		float groupTimeMs = 0.0f;
		CHECK_ZG mState->profiler.getMeasurement(groupId.id, groupTimeMs);
		const char* label = groupId.groupName.str();
		stats.addSample("gpu", label, frameIdx, groupTimeMs);
	}
	if (frameIds.imguiId != ~0ull) {
		uint64_t frameIdx = mState->lastRetrievedFrameTimeFrameIdx;
		float imguiTimeMs = 0.0f;
		CHECK_ZG mState->profiler.getMeasurement(frameIds.imguiId, imguiTimeMs);
		stats.addSample("gpu", "imgui", frameIdx, imguiTimeMs);
	}
	frameIds.groupIds.clear();

	// Query drawable width and height from SDL
	int32_t newResX = 0;
	int32_t newResY = 0;
	SDL_GL_GetDrawableSize(mState->window, &newResX, &newResY);
	bool resolutionChanged = newResX != mState->windowRes.x || newResY != mState->windowRes.y;
	
	// Check if any texture scale setting has changed, necessating a resolution change
	if (!resolutionChanged) {
		for (auto pair : mState->configurable.staticTextures) {
			StaticTextureItem& item = pair.value;
			if (!item.resolutionIsFixed && item.resolutionScaleSetting != nullptr) {
				if (item.resolutionScale != item.resolutionScaleSetting->floatValue()) {
					resolutionChanged = true;
					break;
				}
			}
		}
	}

	// If resolution has changed, resize swapchain and framebuffers
	if (resolutionChanged) {

		SFZ_INFO("Renderer",
			"Resolution changed, new resolution: %i x %i. Updating framebuffers...",
			newResX, newResY);

		// Set new resolution
		mState->windowRes.x = newResX;
		mState->windowRes.y = newResY;

		// Stop present queue so its safe to reallocate framebuffers
		CHECK_ZG mState->presentQueue.flush();
		
		// Resize swapchain
		// Note: This is actually safe to call every frame and without first flushing present
		//       queue, but since we are also resizing other framebuffers created by us we might
		//       as well protect this call just the same.
		CHECK_ZG zgContextSwapchainResize(
			uint32_t(mState->windowRes.x), uint32_t(mState->windowRes.y));

		// Resize static textures
		for (auto pair : mState->configurable.staticTextures) {
			StaticTextureItem& item = pair.value;
			
			// Only resize if not fixed resolution
			if (!item.resolutionIsFixed) {
				item.buildTexture(mState->windowRes);
			}
		}

		// Rebuild all stages' framebuffer objects
		for (StageGroup& group : mState->configurable.presentStageGroups) {
			for (Stage& stage : group.stages) {
				stage.rebuildFramebuffer(mState->configurable.staticTextures);
			}
		}
	}

	// Set vsync settings
	CHECK_ZG zgContextSwapchainSetVsync(mState->vsync->boolValue() ? ZG_TRUE : ZG_FALSE);
	
	// Begin ZeroG frame
	sfz_assert(!mState->windowFramebuffer.valid());
	CHECK_ZG zgContextSwapchainBeginFrame(
		&mState->windowFramebuffer.handle, mState->profiler.handle, &frameIds.frameId);

	// Set current stage group and stage to first
	mState->currentStageGroupIdx = 0;
}

bool Renderer::inStageInputMode() const noexcept
{
	return mState->inputEnabled.inInputMode;
}

void Renderer::stageBeginInput(const char* stageName) noexcept
{
	// Ensure no stage is currently set to accept input
	sfz_assert(!inStageInputMode());
	if (inStageInputMode()) return;

	strID stageNameID = strID(stageName);

	// Find stage
	uint32_t stageIdx = mState->findActiveStageIdx(stageNameID);
	sfz_assert(stageIdx != ~0u);
	if (stageIdx == ~0u) return;

	Stage& stage = mState->configurable.presentStageGroups[mState->currentStageGroupIdx].stages[stageIdx];

	if (stage.type == StageType::USER_INPUT_RENDERING) {
	
		// Find render pipeline
		uint32_t pipelineIdx = mState->findPipelineRenderIdx(stage.render.pipelineName);
		sfz_assert(pipelineIdx != ~0u);
		if (pipelineIdx == ~0u) return;
		sfz_assert(pipelineIdx < mState->configurable.renderPipelines.size());
		PipelineRenderItem& pipelineItem = mState->configurable.renderPipelines[pipelineIdx];
		sfz_assert(pipelineItem.pipeline.valid());
		if (!pipelineItem.pipeline.valid()) return;

		// Store pipeline in input enabled
		mState->inputEnabled.pipelineRender = &pipelineItem;

		// In debug mode, validate that the pipeline's render targets matches the framebuffer
#ifndef NDEBUG
		if (stage.render.defaultFramebuffer) {
			sfz_assert(pipelineItem.renderTargets.size() == 1);
			sfz_assert(pipelineItem.renderTargets[0] == ZG_TEXTURE_FORMAT_RGBA_U8_UNORM);
		}
		else {
			sfz_assert(pipelineItem.renderTargets.size() == stage.render.renderTargetNames.size());
			/*for (uint32_t i = 0; i < stage.renderTargetNames.size(); i++) {
				sfz_assert(pipelineItem.renderTargets[i] == fbItem->renderTargetItems[i].format);
			}*/
		}
#endif
	}
	else if (stage.type == StageType::USER_INPUT_COMPUTE) {

		// Find compute pipeline
		uint32_t pipelineIdx = mState->findPipelineComputeIdx(stage.compute.pipelineName);
		sfz_assert(pipelineIdx != ~0u);
		if (pipelineIdx == ~0u) return;
		sfz_assert(pipelineIdx < mState->configurable.computePipelines.size());
		PipelineComputeItem& pipelineItem = mState->configurable.computePipelines[pipelineIdx];
		sfz_assert(pipelineItem.pipeline.valid());
		if (!pipelineItem.pipeline.valid()) return;

		// Store pipeline in input enabled
		mState->inputEnabled.pipelineCompute = &pipelineItem;
	}
	else {
		sfz_assert(false);
	}

	// Set currently active stage
	mState->inputEnabled.inInputMode = true;
	mState->inputEnabled.stageIdx = stageIdx;
	mState->inputEnabled.stage = &stage;

	// If group's command list has not yet been created, create it
	zg::CommandList& cmdList = mState->groupCmdList;
	if (!cmdList.valid()) {

		// Begin command list
		CHECK_ZG mState->presentQueue.beginCommandListRecording(cmdList);

		// Since first usage of this group, insert call to profile begin
		FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);
		sfz_assert(frameIds.groupIds.size() <= mState->currentStageGroupIdx);
		GroupProfilingID& groupId = frameIds.groupIds.add();
		groupId.groupName =
			mState->configurable.presentStageGroups[mState->currentStageGroupIdx].groupName;
		CHECK_ZG cmdList.profileBegin(mState->profiler, groupId.id);

		// Add event
		if (mState->emitDebugEvents->boolValue()) {
			CHECK_ZG cmdList.beginEvent(groupId.groupName.str());
		}
	}

	// Add event
	if (mState->emitDebugEvents->boolValue()) {
		CHECK_ZG cmdList.beginEvent(stageName);
	}

	// Set pipeline and framebuffer (if rendering)
	if (stage.type == StageType::USER_INPUT_RENDERING) {
		CHECK_ZG cmdList.setPipeline(mState->inputEnabled.pipelineRender->pipeline);
		if (stage.render.defaultFramebuffer) {
			CHECK_ZG cmdList.setFramebuffer(mState->windowFramebuffer);
		}
		else {
			CHECK_ZG cmdList.setFramebuffer(stage.render.framebuffer);
		}
	}
	else if (stage.type == StageType::USER_INPUT_COMPUTE) {
		CHECK_ZG cmdList.setPipeline(mState->inputEnabled.pipelineCompute->pipeline);
	}
	else {
		sfz_assert(false);
	}
}

vec2_u32 Renderer::stageGetFramebufferDims() const noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	if (mState->inputEnabled.stage->render.defaultFramebuffer) {
		return vec2_u32(mState->windowRes);
	}
	else {
		vec2_u32 dims = vec2_u32(0u);
		CHECK_ZG mState->inputEnabled.stage->render.framebuffer.getResolution(dims.x, dims.y);
		return dims;
	}
}

void Renderer::stageUploadToStreamingBufferUntyped(
	const char* bufferName, const void* data, uint32_t elementSize, uint32_t numElements) noexcept
{
	sfz_assert(inStageInputMode());
	
	// Get streaming buffer item
	strID bufferID = strID(bufferName);
	StreamingBufferItem* item = mState->configurable.streamingBuffers.get(bufferID);
	sfz_assert(item != nullptr);

	// Calculate number of bytes to copy to streaming buffer
	const uint32_t numBytes = elementSize * numElements;
	sfz_assert(numBytes != 0);
	sfz_assert(numBytes < (item->elementSizeBytes * item->maxNumElements));
	sfz_assert(elementSize == item->elementSizeBytes); // TODO: Might want to remove this assert

	// Grab this frame's memory
	StreamingBufferMemory& memory = item->data.data(mState->currentFrameIdx);

	// Only allowed to upload to streaming buffer once per frame
	sfz_assert(memory.lastFrameIdxTouched < mState->currentFrameIdx);
	memory.lastFrameIdxTouched = mState->currentFrameIdx;

	// Memcpy to upload buffer
	CHECK_ZG memory.uploadBuffer.memcpyUpload(0, data, numBytes);

	// Schedule memcpy from upload buffer to device buffer
	CHECK_ZG mState->groupCmdList.memcpyBufferToBuffer(
		memory.deviceBuffer, 0, memory.uploadBuffer, 0, numBytes);
}

void Renderer::stageClearRenderTargetsOptimal() noexcept
{
	sfz_assert(inStageInputMode());
	CHECK_ZG mState->groupCmdList.clearRenderTargetsOptimal();
}

void Renderer::stageClearDepthBufferOptimal() noexcept
{
	sfz_assert(inStageInputMode());
	CHECK_ZG mState->groupCmdList.clearDepthBufferOptimal();
}

void Renderer::stageSetPushConstantUntyped(
	uint32_t shaderRegister, const void* data, uint32_t numBytes) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(data != nullptr);
	sfz_assert(numBytes > 0);
	sfz_assert(numBytes <= 128);

	// In debug mode, validate that the specified shader registers corresponds to a a suitable
	// push constant in the pipeline
#ifndef NDEBUG
	ZgPipelineBindingsSignature signature = {};
	if (mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING) {
		signature = mState->inputEnabled.pipelineRender->pipeline.getSignature().bindings;
	}
	else if (mState->inputEnabled.stage->type == StageType::USER_INPUT_COMPUTE) {
		signature = mState->inputEnabled.pipelineCompute->pipeline.getBindingsSignature();
	}
	else {
		sfz_assert(false);
	}
	
	uint32_t bufferIdx = ~0u;
	for (uint32_t i = 0; i < signature.numConstBuffers; i++) {
		if (shaderRegister == signature.constBuffers[i].bufferRegister) {
			bufferIdx = i;
			break;
		}
	}
	
	sfz_assert(bufferIdx != ~0u);
	sfz_assert(signature.constBuffers[bufferIdx].pushConstant == ZG_TRUE);
	sfz_assert(signature.constBuffers[bufferIdx].sizeInBytes >= numBytes);
#endif

	CHECK_ZG mState->groupCmdList.setPushConstant(shaderRegister, data, numBytes);
}

void Renderer::stageSetConstantBufferUntyped(
	uint32_t shaderRegister, const void* data, uint32_t numBytes) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(data != nullptr);
	sfz_assert(numBytes > 0);

	// In debug mode, validate that the specified shader registers corresponds to a a suitable
	// constant buffer in the pipeline
#ifndef NDEBUG
	ZgPipelineBindingsSignature signature = {};
	if (mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING) {
		signature = mState->inputEnabled.pipelineRender->pipeline.getSignature().bindings;
	}
	else if (mState->inputEnabled.stage->type == StageType::USER_INPUT_COMPUTE) {
		signature = mState->inputEnabled.pipelineCompute->pipeline.getBindingsSignature();
	}
	else {
		sfz_assert(false);
	}

	uint32_t bufferIdx = ~0u;
	for (uint32_t i = 0; i < signature.numConstBuffers; i++) {
		if (shaderRegister == signature.constBuffers[i].bufferRegister) {
			bufferIdx = i;
			break;
		}
	}

	sfz_assert(bufferIdx != ~0u);
	sfz_assert(signature.constBuffers[bufferIdx].pushConstant == ZG_FALSE);
	sfz_assert(signature.constBuffers[bufferIdx].sizeInBytes >= numBytes);
#endif

	// Find constant buffer
	ConstantBufferMemory* frame =
		mState->findConstantBufferInCurrentInputStage(shaderRegister);
	sfz_assert(frame != nullptr);

	// Ensure that we can only set constant buffer once per frame
	sfz_assert(frame->lastFrameIdxTouched != mState->currentFrameIdx);
	frame->lastFrameIdxTouched = mState->currentFrameIdx;

	// Copy data to upload buffer
	CHECK_ZG frame->uploadBuffer.memcpyUpload(0, data, numBytes);

	// Issue upload to device buffer
	CHECK_ZG mState->groupCmdList.memcpyBufferToBuffer(
		frame->deviceBuffer, 0, frame->uploadBuffer, 0, numBytes);
}

void Renderer::stageDrawMesh(strID meshId, const MeshRegisters& registers, bool skipBindings) noexcept
{
	sfz_assert(meshId.isValid());
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);

	// Find mesh
	GpuMesh* meshPtr = mState->meshes.get(meshId);
	sfz_assert(meshPtr != nullptr);
	if (meshPtr == nullptr) return;

	// If mesh is not enabled, skip rendering it
	if (!meshPtr->enabled) return;

	// Validate some stuff in debug mode
#ifndef NDEBUG
	// Validate pipeline vertex input for standard mesh rendering
	const ZgPipelineRenderSignature renderSignature =
		mState->inputEnabled.pipelineRender->pipeline.getSignature();
	sfz_assert(renderSignature.numVertexAttributes == 3);

	sfz_assert(renderSignature.vertexAttributes[0].location == 0);
	sfz_assert(renderSignature.vertexAttributes[0].vertexBufferSlot == 0);
	sfz_assert(renderSignature.vertexAttributes[0].type == ZG_VERTEX_ATTRIBUTE_F32_3);
	sfz_assert(
		renderSignature.vertexAttributes[0].offsetToFirstElementInBytes == offsetof(Vertex, pos));

	sfz_assert(renderSignature.vertexAttributes[1].location == 1);
	sfz_assert(renderSignature.vertexAttributes[1].vertexBufferSlot == 0);
	sfz_assert(renderSignature.vertexAttributes[1].type == ZG_VERTEX_ATTRIBUTE_F32_3);
	sfz_assert(
		renderSignature.vertexAttributes[1].offsetToFirstElementInBytes == offsetof(Vertex, normal));

	sfz_assert(renderSignature.vertexAttributes[2].location == 2);
	sfz_assert(renderSignature.vertexAttributes[2].vertexBufferSlot == 0);
	sfz_assert(renderSignature.vertexAttributes[2].type == ZG_VERTEX_ATTRIBUTE_F32_2);
	sfz_assert(
		renderSignature.vertexAttributes[2].offsetToFirstElementInBytes == offsetof(Vertex, texcoord));

	const ZgPipelineBindingsSignature bindingsSignature =
		mState->inputEnabled.pipelineRender->pipeline.getSignature().bindings;

	// Validate material index push constant
	if (registers.materialIdxPushConstant != ~0u) {
		bool found = false;
		for (uint32_t i = 0; i < bindingsSignature.numConstBuffers; i++) {
			const ZgConstantBufferBindingDesc& desc = bindingsSignature.constBuffers[i];
			if (desc.bufferRegister == registers.materialIdxPushConstant) {
				found = true;
				sfz_assert(desc.pushConstant == ZG_TRUE);
				break;
			}
		}
		sfz_assert(found);
	}

	// Validate materials array
	if (registers.materialsArray != ~0u) {
		bool found = false;
		for (uint32_t i = 0; i < bindingsSignature.numConstBuffers; i++) {
			const ZgConstantBufferBindingDesc& desc = bindingsSignature.constBuffers[i];
			if (desc.bufferRegister == registers.materialsArray) {
				found = true;
				sfz_assert(desc.pushConstant == ZG_FALSE);
				sfz_assert(desc.sizeInBytes >= meshPtr->numMaterials * sizeof(ShaderMaterial));
				sfz_assert(desc.sizeInBytes == sizeof(ForwardShaderMaterialsBuffer));
				break;
			}
		}
		sfz_assert(found);
	}

	// Validate texture bindings
	auto assertTextureRegister = [&](uint32_t texRegister) {
		if (texRegister == ~0u) return;
		bool found = false;
		for (uint32_t i = 0; i < bindingsSignature.numTextures; i++) {
			const ZgTextureBindingDesc& desc = bindingsSignature.textures[i];
			if (desc.textureRegister == texRegister) {
				found = true;
				break;
			}
		}
		sfz_assert(found);
	};

	assertTextureRegister(registers.albedo);
	assertTextureRegister(registers.metallicRoughness);
	assertTextureRegister(registers.normal);
	assertTextureRegister(registers.occlusion);
	assertTextureRegister(registers.emissive);
#endif

	// Set vertex buffer
	sfz_assert(meshPtr->vertexBuffer.valid());
	CHECK_ZG mState->groupCmdList.setVertexBuffer(0, meshPtr->vertexBuffer);

	// Set index buffer
	sfz_assert(meshPtr->indexBuffer.valid());
	CHECK_ZG mState->groupCmdList.setIndexBuffer(
		meshPtr->indexBuffer, ZG_INDEX_BUFFER_TYPE_UINT32);

	// Set common pipeline bindings that are same for all components
	zg::PipelineBindings commonBindings;

	if (!skipBindings) {
		// Create materials array binding
		if (registers.materialsArray != ~0u) {
			sfz_assert(meshPtr->materialsBuffer.valid());
			commonBindings.addConstantBuffer(registers.materialsArray, meshPtr->materialsBuffer);
		}

		// User-specified constant buffers
		for (PerFrameData<ConstantBufferMemory>& framed : mState->inputEnabled.stage->constantBuffers) {
			ConstantBufferMemory& frame = framed.data(mState->currentFrameIdx);
			sfz_assert(frame.lastFrameIdxTouched == mState->currentFrameIdx);
			commonBindings.addConstantBuffer(frame.shaderRegister, frame.deviceBuffer);
		}

		// Bound textures
		for (const BoundTexture& boundTex : mState->inputEnabled.stage->boundTextures) {
			StaticTextureItem* texItem = mState->configurable.staticTextures.get(boundTex.textureName);
			sfz_assert(texItem != nullptr);
			commonBindings.addTexture(boundTex.textureRegister, texItem->texture);
		}

		// Bound unordered textures
		for (const BoundTexture& boundTex : mState->inputEnabled.stage->boundUnorderedTextures) {
			StaticTextureItem* texItem = mState->configurable.staticTextures.get(boundTex.textureName);
			sfz_assert(texItem != nullptr);
			commonBindings.addUnorderedTexture(boundTex.textureRegister, 0, texItem->texture);
		}

		// Bound unordered buffers
		for (const BoundBuffer& boundBuf : mState->inputEnabled.stage->boundUnorderedBuffers) {
			StaticBufferItem* bufItem = mState->configurable.staticBuffers.get(boundBuf.bufferName);
			sfz_assert(bufItem != nullptr);
			commonBindings.addUnorderedBuffer(
				boundBuf.bufferRegister, bufItem->maxNumElements, bufItem->elementSizeBytes, bufItem->buffer);
		}
	}

	// Draw all mesh components
	ResourceManager& resources = getResourceManager();
	for (MeshComponent& comp : meshPtr->components) {

		if (!skipBindings) {
			sfz_assert(comp.materialIdx < meshPtr->cpuMaterials.size());
			const Material& material = meshPtr->cpuMaterials[comp.materialIdx];

			// Set material index push constant
			if (registers.materialIdxPushConstant != ~0u) {
				sfz::vec4_u32 tmp = sfz::vec4_u32(0u);
				tmp.x = comp.materialIdx;
				CHECK_ZG mState->groupCmdList.setPushConstant(
					registers.materialIdxPushConstant, &tmp, sizeof(sfz::vec4_u32));
			}

			// Create texture bindings
			zg::PipelineBindings bindings = commonBindings;
			auto bindTexture = [&](uint32_t texRegister, strID texID) {
				if (texRegister != ~0u && texID.isValid()) {

					// Find texture
					TextureItem* texItem = resources.getTexture(resources.getTextureHandle(texID));
					sfz_assert(texItem != nullptr);

					// Bind texture
					bindings.addTexture(texRegister, texItem->texture);
				}
			};
			bindTexture(registers.albedo, material.albedoTex);
			bindTexture(registers.metallicRoughness, material.metallicRoughnessTex);
			bindTexture(registers.normal, material.normalTex);
			bindTexture(registers.occlusion, material.occlusionTex);
			bindTexture(registers.emissive, material.emissiveTex);

			// Set pipeline bindings
			CHECK_ZG mState->groupCmdList.setPipelineBindings(bindings);
		}
		
		// Issue draw command
		sfz_assert(comp.numIndices != 0);
		sfz_assert((comp.numIndices % 3) == 0);
		CHECK_ZG mState->groupCmdList.drawTrianglesIndexed(
			comp.firstIndex, comp.numIndices);
	}
}

void Renderer::stageSetBindings(const PipelineBindings& bindings) noexcept
{
	// TODO: Verify bindings in debug mode

	zg::PipelineBindings zgBindings;
	
	// Constant buffers
	for (const Binding& binding : bindings.constBuffers) {
		
		StreamingBufferItem* streamingItem =
			mState->configurable.streamingBuffers.get(binding.resourceID);
		StaticBufferItem* staticItem =
			mState->configurable.staticBuffers.get(binding.resourceID);
		sfz_assert(streamingItem != nullptr || staticItem != nullptr);
		sfz_assert(!(streamingItem != nullptr && staticItem != nullptr));

		zg::Buffer* buffer = nullptr;
		if (streamingItem != nullptr) {
			buffer = &streamingItem->data.data(mState->currentFrameIdx).deviceBuffer;
		}
		else if (staticItem != nullptr) {
			buffer = &staticItem->buffer;
		}

		sfz_assert(binding.shaderRegister != ~0u);
		zgBindings.addConstantBuffer(binding.shaderRegister, *buffer);
	}

	// Textures
	ResourceManager& resources = getResourceManager();
	for (const Binding& binding : bindings.textures) {

		TextureItem* dynItem = resources.getTexture(resources.getTextureHandle(binding.resourceID));
		StaticTextureItem* staticItem = mState->configurable.staticTextures.get(binding.resourceID);
		sfz_assert(dynItem != nullptr || staticItem != nullptr);
		sfz_assert(!(dynItem == nullptr && staticItem == nullptr));

		zg::Texture* tex = nullptr;
		if (dynItem != nullptr) tex = &dynItem->texture;
		else if (staticItem != nullptr) tex = &staticItem->texture;
		sfz_assert(tex != nullptr);

		sfz_assert(binding.shaderRegister != ~0u);
		zgBindings.addTexture(binding.shaderRegister, *tex);
	}

	// Unordered buffers
	for (const Binding& binding : bindings.unorderedBuffers) {

		StreamingBufferItem* streamingItem =
			mState->configurable.streamingBuffers.get(binding.resourceID);
		StaticBufferItem* staticItem =
			mState->configurable.staticBuffers.get(binding.resourceID);
		sfz_assert(streamingItem != nullptr || staticItem != nullptr);
		sfz_assert(!(streamingItem != nullptr && staticItem != nullptr));

		zg::Buffer* buffer = nullptr;
		uint32_t elementSizeBytes = 0;
		uint32_t maxNumElements = 0;
		if (streamingItem != nullptr) {
			buffer = &streamingItem->data.data(mState->currentFrameIdx).deviceBuffer;
			elementSizeBytes = streamingItem->maxNumElements * streamingItem->elementSizeBytes;
			maxNumElements = streamingItem->maxNumElements * streamingItem->maxNumElements;
		}
		else if (staticItem != nullptr) {
			buffer = &staticItem->buffer;
			elementSizeBytes = staticItem->elementSizeBytes;
			maxNumElements = staticItem->maxNumElements;
		}

		sfz_assert(binding.shaderRegister != ~0u);
		zgBindings.addUnorderedBuffer(binding.shaderRegister, maxNumElements, elementSizeBytes, *buffer);
	}

	// Unordered textures
	for (const Binding& binding : bindings.unorderedTextures) {
		
		TextureItem* dynItem = resources.getTexture(resources.getTextureHandle(binding.resourceID));
		StaticTextureItem* staticItem = mState->configurable.staticTextures.get(binding.resourceID);
		sfz_assert(dynItem != nullptr || staticItem != nullptr);
		sfz_assert(!(dynItem == nullptr && staticItem == nullptr));

		zg::Texture* tex = nullptr;
		if (dynItem != nullptr) tex = &dynItem->texture;
		else if (staticItem != nullptr) tex = &staticItem->texture;
		sfz_assert(tex != nullptr);

		sfz_assert(binding.shaderRegister != ~0u);
		zgBindings.addUnorderedTexture(binding.shaderRegister, binding.mipLevel, *tex);
	}

	CHECK_ZG mState->groupCmdList.setPipelineBindings(zgBindings);
}

void Renderer::stageSetVertexBuffer(const char* streamingBufferName) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);

	// Get streaming buffer item
	strID bufferID = strID(streamingBufferName);
	StreamingBufferItem* item = mState->configurable.streamingBuffers.get(bufferID);
	sfz_assert(item != nullptr);

	// Grab this frame's memory
	StreamingBufferMemory& memory = item->data.data(mState->currentFrameIdx);

	// Set vertex buffer
	CHECK_ZG mState->groupCmdList.setVertexBuffer(0, memory.deviceBuffer);
}

void Renderer::stageSetIndexBuffer(const char* streamingBufferName, bool u32Buffer) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);

	// Get streaming buffer item
	strID bufferID = strID(streamingBufferName);
	StreamingBufferItem* item = mState->configurable.streamingBuffers.get(bufferID);
	sfz_assert(item != nullptr);

	// Grab this frame's memory
	StreamingBufferMemory& memory = item->data.data(mState->currentFrameIdx);

	// Set index buffer
	CHECK_ZG mState->groupCmdList.setIndexBuffer(
		memory.deviceBuffer, u32Buffer ? ZG_INDEX_BUFFER_TYPE_UINT32 : ZG_INDEX_BUFFER_TYPE_UINT16);
}

void Renderer::stageDrawTriangles(uint32_t startVertex, uint32_t numVertices) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	CHECK_ZG mState->groupCmdList.drawTriangles(startVertex, numVertices);
}

void Renderer::stageDrawTrianglesIndexed(uint32_t firstIndex, uint32_t numIndices) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	CHECK_ZG mState->groupCmdList.drawTrianglesIndexed(firstIndex, numIndices);
}

void Renderer::stageUnorderedBarrierAll() noexcept
{
	sfz_assert(inStageInputMode());
	CHECK_ZG mState->groupCmdList.unorderedBarrier();
}

void Renderer::stageUnorderedBarrierStaticBuffer(const char* staticBufferName) noexcept
{
	sfz_assert(inStageInputMode());
	
	// Get static buffer item
	strID bufferID = strID(staticBufferName);
	StaticBufferItem* item = mState->configurable.staticBuffers.get(bufferID);
	sfz_assert(item != nullptr);

	CHECK_ZG mState->groupCmdList.unorderedBarrier(item->buffer);
}

void Renderer::stageUnorderedBarrierStaticTexture(const char* staticBufferName) noexcept
{
	sfz_assert(inStageInputMode());

	// Get static texture item
	strID textureID = strID(staticBufferName);
	StaticTextureItem* item = mState->configurable.staticTextures.get(textureID);
	sfz_assert(item != nullptr);

	CHECK_ZG mState->groupCmdList.unorderedBarrier(item->texture);
}

vec3_i32 Renderer::stageGetComputeGroupDims() noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_COMPUTE);

	vec3_u32 groupDims;
	mState->inputEnabled.pipelineCompute->pipeline.getGroupDims(groupDims.x, groupDims.y, groupDims.z);
	return vec3_i32(groupDims);
}

void Renderer::stageDispatchCompute(
	uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_COMPUTE);
	sfz_assert(groupCountX > 0);
	sfz_assert(groupCountY > 0);
	sfz_assert(groupCountZ > 0);

	// Set common pipeline bindings that are same for all components
	zg::PipelineBindings commonBindings;

	// User-specified constant buffers
	for (PerFrameData<ConstantBufferMemory>& framed : mState->inputEnabled.stage->constantBuffers) {
		ConstantBufferMemory& frame = framed.data(mState->currentFrameIdx);
		sfz_assert(frame.lastFrameIdxTouched == mState->currentFrameIdx);
		commonBindings.addConstantBuffer(frame.shaderRegister, frame.deviceBuffer);
	}

	// Bound textures
	for (const BoundTexture& boundTex : mState->inputEnabled.stage->boundTextures) {
		StaticTextureItem* texItem = mState->configurable.staticTextures.get(boundTex.textureName);
		sfz_assert(texItem != nullptr);
		commonBindings.addTexture(boundTex.textureRegister, texItem->texture);
	}

	// Bound unordered textures
	for (const BoundTexture& boundTex : mState->inputEnabled.stage->boundUnorderedTextures) {
		StaticTextureItem* texItem = mState->configurable.staticTextures.get(boundTex.textureName);
		sfz_assert(texItem != nullptr);
		commonBindings.addUnorderedTexture(boundTex.textureRegister, 0, texItem->texture);
	}

	// Bound unordered buffers
	for (const BoundBuffer& boundBuf : mState->inputEnabled.stage->boundUnorderedBuffers) {
		StaticBufferItem* bufItem = mState->configurable.staticBuffers.get(boundBuf.bufferName);
		sfz_assert(bufItem != nullptr);
		commonBindings.addUnorderedBuffer(
			boundBuf.bufferRegister, bufItem->maxNumElements, bufItem->elementSizeBytes, bufItem->buffer);
	}

	// Set pipeline bindings
	CHECK_ZG mState->groupCmdList.setPipelineBindings(commonBindings);

	// Issue dispatch
	CHECK_ZG mState->groupCmdList.dispatchCompute(groupCountX, groupCountY, groupCountZ);
}

void Renderer::stageDispatchComputeNoAutoBindings(
	uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_COMPUTE);
	sfz_assert(groupCountX > 0);
	sfz_assert(groupCountY > 0);
	sfz_assert(groupCountZ > 0);

	// Issue dispatch
	CHECK_ZG mState->groupCmdList.dispatchCompute(groupCountX, groupCountY, groupCountZ);
}

void Renderer::stageEndInput() noexcept
{
	// Ensure a stage was set to accept input
	sfz_assert(inStageInputMode());
	if (!inStageInputMode()) return;

	// Insert compute barrier
	if (mState->inputEnabled.stage->type == StageType::USER_INPUT_COMPUTE) {
		// TODO: This is a bit too harsh
		CHECK_ZG mState->groupCmdList.unorderedBarrier();
	}

	// Insert event end call
	if (mState->emitDebugEvents->boolValue()) {
		CHECK_ZG mState->groupCmdList.endEvent();
	}

	// Clear currently active stage info
	mState->inputEnabled = {};
}

bool Renderer::frameProgressNextStageGroup() noexcept
{
	sfz_assert(!inStageInputMode());

	zg::CommandList& cmdList = mState->groupCmdList;
	if (cmdList.valid()) {
		FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);

		// Insert profile end call
		strID groupName = mState->configurable.presentStageGroups[mState->currentStageGroupIdx].groupName;
		GroupProfilingID* groupId = frameIds.groupIds.find([&](const GroupProfilingID& e) {
			return e.groupName == groupName;
		});
		sfz_assert(groupId != nullptr);
		sfz_assert(groupId->id != ~0ull);
		CHECK_ZG cmdList.profileEnd(mState->profiler, groupId->id);

		// Insert event end call
		if (mState->emitDebugEvents->boolValue()) {
			CHECK_ZG cmdList.endEvent();
		}

		// Execute command list
		// TODO: We should probably execute all of the frame's command lists simulatenously instead of
		//       one by one
		CHECK_ZG mState->presentQueue.executeCommandList(cmdList);
	}

	// Move to next stage group
	mState->currentStageGroupIdx += 1;
	sfz_assert(mState->currentStageGroupIdx < mState->configurable.presentStageGroups.size());
	
	return true;
}

void Renderer::frameFinish() noexcept
{
	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);

	zg::CommandList& cmdList = mState->groupCmdList;
	
	// End last stage group
	if (cmdList.valid()) {

		// Insert profile end call
		strID groupName = mState->configurable.presentStageGroups[mState->currentStageGroupIdx].groupName;
		GroupProfilingID* groupId = frameIds.groupIds.find([&](const GroupProfilingID& e) {
			return e.groupName == groupName;
		});
		sfz_assert(groupId != nullptr);
		sfz_assert(groupId->id != ~0ull);
		CHECK_ZG cmdList.profileEnd(mState->profiler, groupId->id);

		// Insert event end call
		if (mState->emitDebugEvents->boolValue()) {
			CHECK_ZG cmdList.endEvent();
		}
	}

	// If no command list, start one just to finish the frame
	else {
		CHECK_ZG mState->presentQueue.beginCommandListRecording(cmdList);
		CHECK_ZG cmdList.setFramebuffer(mState->windowFramebuffer);
	}

	// Render ImGui
	zg::imguiRender(
		mState->imguiRenderState,
		mState->currentFrameIdx,
		cmdList,
		uint32_t(mState->windowRes.x),
		uint32_t(mState->windowRes.y),
		mState->imguiScaleSetting->floatValue(),
		&mState->profiler,
		&frameIds.imguiId);

	// Execute command list
	// TODO: We should probably execute all of the frame's command lists simulatenously instead of
	//       one by one
	CHECK_ZG mState->presentQueue.executeCommandList(cmdList);

	// Finish ZeroG frame
	sfz_assert(mState->windowFramebuffer.valid());
	CHECK_ZG zgContextSwapchainFinishFrame(mState->profiler.handle, frameIds.frameId);
	mState->windowFramebuffer.destroy();

	// Signal that we are done rendering use these resources
	zg::Fence& frameFence = mState->frameFences.data(mState->currentFrameIdx);
	CHECK_ZG mState->presentQueue.signalOnGpu(frameFence);

	// Flush queues if requested
	if (mState->flushPresentQueueEachFrame->boolValue()) {
		CHECK_ZG mState->presentQueue.flush();
	}
	if (mState->flushCopyQueueEachFrame->boolValue()) {
		CHECK_ZG mState->copyQueue.flush();
	}
}

} // namespace sfz
