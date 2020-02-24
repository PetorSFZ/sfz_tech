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
#include <skipifzero_arrays.hpp>

#include <ZeroG-cpp.hpp>

#include <ZeroG-ImGui.hpp>

#include "sfz/Context.hpp"
#include "sfz/debug/ProfilingStats.hpp"
#include "sfz/Logging.hpp"
#include "sfz/config/GlobalConfig.hpp"
#include "sfz/math/Matrix.hpp"
#include "sfz/renderer/GpuTextures.hpp"
#include "sfz/renderer/RendererConfigParser.hpp"
#include "sfz/renderer/RendererState.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"

namespace sfz {

// Renderer: State methods
// ------------------------------------------------------------------------------------------------

bool Renderer::init(
	SDL_Window* window,
	const phConstImageView& fontTexture,
	sfz::Allocator* allocator) noexcept
{
	this->destroy();
	mState = allocator->newObject<RendererState>(sfz_dbg("RendererState"));
	mState->allocator = allocator;
	mState->window = window;

	// Settings
	GlobalConfig& cfg = getGlobalConfig();
	Setting* debugModeSetting =
		cfg.sanitizeBool("Renderer", "ZeroGDebugModeOnStartup", true, false);
	mState->flushPresentQueueEachFrame =
		cfg.sanitizeBool("Renderer", "flushPresentQueueEachFrame", false, false);
	mState->flushCopyQueueEachFrame =
		cfg.sanitizeBool("Renderer", "flushCopyQueueEachFrame", false, false);

	// Initializer ZeroG
	bool zgInitSuccess =
		initializeZeroG(mState->zgCtx, window, allocator, debugModeSetting->boolValue());
	if (!zgInitSuccess) {
		this->destroy();
		return false;
	}

	// Initialize fences
	mState->frameFences.init(mState->frameLatency, [](zg::Fence& fence) {
		CHECK_ZG fence.create();
	});

	// Set window resolution to default value (512x512)
	mState->windowRes = vec2_i32(512, 512);

	// Get command queues
	if (!(CHECK_ZG zg::CommandQueue::getPresentQueue(mState->presentQueue))) {
		this->destroy();
		return false;
	}
	if (!(CHECK_ZG zg::CommandQueue::getCopyQueue(mState->copyQueue))) {
		this->destroy();
		return false;
	}

	// Initialize profiler
	{
		ZgProfilerCreateInfo createInfo = {};
		createInfo.maxNumMeasurements = 1024;
		CHECK_ZG mState->profiler.create(createInfo);
		mState->frameMeasurementIds.init(mState->frameLatency);
	}

	// Initialize dynamic gpu allocator
	constexpr uint32_t PAGE_SIZE_UPLOAD = 32 * 1024 * 1024; // 32 MiB
	constexpr uint32_t PAGE_SIZE_DEVICE = 64 * 1024 * 1024; // 64 MiB
	constexpr uint32_t PAGE_SIZE_TEXTURE = 64 * 1024 * 1024; // 64 MiB
	constexpr uint32_t PAGE_SIZE_FRAMEBUFFER = 64 * 1024 * 1024; // 64 MiB
	mState->gpuAllocatorUpload.init(mState->allocator, ZG_MEMORY_TYPE_UPLOAD, PAGE_SIZE_UPLOAD);
	mState->gpuAllocatorDevice.init(mState->allocator, ZG_MEMORY_TYPE_DEVICE, PAGE_SIZE_DEVICE);
	mState->gpuAllocatorTexture.init(mState->allocator, ZG_MEMORY_TYPE_TEXTURE, PAGE_SIZE_TEXTURE);
	mState->gpuAllocatorFramebuffer.init(mState->allocator, ZG_MEMORY_TYPE_FRAMEBUFFER, PAGE_SIZE_FRAMEBUFFER);

	// Initialize hashmaps for resources
	mState->textures.init(512, mState->allocator, sfz_dbg(""));
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
	StringCollection& resStrings = getResourceStrings();
	ProfilingStats& stats = getProfilingStats();
	stats.createCategory("gpu", 300, 66.7f, "ms", "frame", 20.0f,
		StatsVisualizationType::FIRST_INDIVIDUALLY_REST_ADDED);
	stats.createLabel("gpu", "frametime", vec4(1.0f, 0.0f, 0.0f, 1.0f), 0.0f);
	for (const StageGroup& group : mState->configurable.presentQueue) {
		const char* label = resStrings.getString(group.groupName);
		stats.createLabel("gpu", label, vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f);
	}
	stats.createLabel("gpu", "imgui", vec4(1.0f, 1.0f, 0.0f, 1.0f), 0.0f);

	return true;
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
		this->removeAllTexturesGpuBlocking();
		this->removeAllMeshesGpuBlocking();

		// Destroy static textures
		for (StaticTextureItem& item : mState->configurable.staticTextures) {
			item.deallocate(mState->gpuAllocatorFramebuffer);
		}

		// Destroy framebuffers
		for (FramebufferItem& item : mState->configurable.framebuffers) {
			item.deallocate(mState->gpuAllocatorFramebuffer);
		}

		// Deallocate stage memory
		bool stageDeallocSuccess = deallocateStageMemory(*mState);
		sfz_assert(stageDeallocSuccess);

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
	StringID id, const phConstImageView& image, bool generateMipmaps) noexcept
{
	// Error out and return false if texture already exists
	if (mState->textures.get(id) != nullptr) return false;

	uint32_t numMipmaps = 0;
	zg::Texture2D texture = textureAllocateAndUploadBlocking(
		image,
		mState->gpuAllocatorTexture,
		mState->gpuAllocatorUpload,
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
	mState->textures[id] = std::move(item);

	return true;
}

bool Renderer::textureLoaded(StringID id) const noexcept
{
	const sfz::TextureItem* item = mState->textures.get(id);
	return item != nullptr;
}

void Renderer::removeTextureGpuBlocking(StringID id) noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	// Return if texture is not loaded in first place
	sfz::TextureItem* item = mState->textures.get(id);
	if (item == nullptr) return;

	// Ensure all GPU operations in progress are finished
	CHECK_ZG mState->presentQueue.flush();
	CHECK_ZG mState->copyQueue.flush();

	// Destroy texture
	mState->gpuAllocatorTexture.deallocate(item->texture);
	mState->textures.remove(id);
}

void Renderer::removeAllTexturesGpuBlocking() noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	// Ensure all GPU operations in progress are finished
	CHECK_ZG mState->presentQueue.flush();
	CHECK_ZG mState->copyQueue.flush();

	// Destroy all textures
	for (auto pair : mState->textures) {
		mState->gpuAllocatorTexture.deallocate(pair.value.texture);
	}
	mState->textures.clear();
}

bool Renderer::uploadMeshBlocking(StringID id, const Mesh& mesh) noexcept
{
	// Error out and return false if mesh already exists
	sfz_assert(id != StringID::invalid());
	if (mState->meshes.get(id) != nullptr) return false;

	// Allocate memory for mesh
	GpuMesh gpuMesh = gpuMeshAllocate(mesh, mState->gpuAllocatorDevice, mState->allocator);

	// Upload memory to mesh
	gpuMeshUploadBlocking(
		gpuMesh, mesh, mState->gpuAllocatorUpload, mState->allocator, mState->copyQueue);

	// Store mesh
	mState->meshes[id] = std::move(gpuMesh);

	return true;
}

bool Renderer::meshLoaded(StringID id) const noexcept
{
	const sfz::GpuMesh* mesh = mState->meshes.get(id);
	return mesh != nullptr;
}

void Renderer::removeMeshGpuBlocking(StringID id) noexcept
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
	gpuMeshDeallocate(*mesh, mState->gpuAllocatorDevice);
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
	for (auto pair : mState->meshes) {
		gpuMeshDeallocate(pair.value, mState->gpuAllocatorDevice);
	}
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
	StringCollection& resStrings = getResourceStrings();
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
		const char* label = resStrings.getString(groupId.groupName);
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
		for (StaticTextureItem& item : mState->configurable.staticTextures) {
			if (!item.resolutionIsFixed && item.resolutionScaleSetting != nullptr) {
				if (item.resolutionScale != item.resolutionScaleSetting->floatValue()) {
					resolutionChanged = true;
					break;
				}
			}
		}
	}

	// Check if any framebuffer scale settings has changed, necessating a resolution change
	if (!resolutionChanged) {
		for (FramebufferItem& item : mState->configurable.framebuffers) {
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
		CHECK_ZG mState->zgCtx.swapchainResize(
			uint32_t(mState->windowRes.x), uint32_t(mState->windowRes.y));

		// Resize static textures
		for (StaticTextureItem& item : mState->configurable.staticTextures) {
			
			// Only resize if not fixed resolution
			if (!item.resolutionIsFixed) {
				item.deallocate(mState->gpuAllocatorFramebuffer);
				item.buildTexture(mState->windowRes, mState->gpuAllocatorFramebuffer);
			}
		}

		// Resize our framebuffers
		for (FramebufferItem& item : mState->configurable.framebuffers) {
			
			// Only resize if not fixed resolution
			if (!item.resolutionIsFixed) {
				item.deallocate(mState->gpuAllocatorFramebuffer);
				bool res = item.buildFramebuffer(mState->windowRes, mState->gpuAllocatorFramebuffer);
				sfz_assert(res);
			}
		}
	}
	
	// Begin ZeroG frame
	sfz_assert(!mState->windowFramebuffer.valid());
	CHECK_ZG mState->zgCtx.swapchainBeginFrame(mState->windowFramebuffer, mState->profiler, frameIds.frameId);

	// Clear all framebuffers
	// TODO: Should probably only clear using a specific clear framebuffer stage
	zg::CommandList commandList;
	CHECK_ZG mState->presentQueue.beginCommandListRecording(commandList);
	CHECK_ZG commandList.setFramebuffer(mState->windowFramebuffer);
	CHECK_ZG commandList.clearFramebufferOptimal();
	CHECK_ZG mState->presentQueue.executeCommandList(commandList);

	for (FramebufferItem& fbItem : mState->configurable.framebuffers) {
		CHECK_ZG mState->presentQueue.beginCommandListRecording(commandList);
		CHECK_ZG commandList.setFramebuffer(fbItem.framebuffer.framebuffer);
		CHECK_ZG commandList.clearFramebufferOptimal();
		CHECK_ZG mState->presentQueue.executeCommandList(commandList);
	}

	// Set current stage group and stage to first
	mState->currentStageGroupIdx = 0;
}

bool Renderer::inStageInputMode() const noexcept
{
	return mState->inputEnabled.inInputMode;
}

void Renderer::stageBeginInput(StringID stageName) noexcept
{
	// Ensure no stage is currently set to accept input
	sfz_assert(!inStageInputMode());
	if (inStageInputMode()) return;

	// Find stage
	uint32_t stageIdx = mState->findActiveStageIdx(stageName);
	sfz_assert(stageIdx != ~0u);
	if (stageIdx == ~0u) return;
	Stage& stage = mState->configurable.presentQueue[mState->currentStageGroupIdx].stages[stageIdx];
	sfz_assert(stage.stageType == StageType::USER_INPUT_RENDERING);

	// Find render pipeline
	uint32_t pipelineIdx = mState->findPipelineRenderIdx(stage.renderPipelineName);
	sfz_assert(pipelineIdx != ~0u);
	if (pipelineIdx == ~0u) return;
	sfz_assert(pipelineIdx < mState->configurable.renderPipelines.size());
	PipelineRenderItem& pipelineItem = mState->configurable.renderPipelines[pipelineIdx];
	sfz_assert(pipelineItem.pipeline.valid());
	if (!pipelineItem.pipeline.valid()) return;

	// Set currently active stage
	mState->inputEnabled.inInputMode = true;
	mState->inputEnabled.stageIdx = stageIdx;
	mState->inputEnabled.stage = &stage;
	mState->inputEnabled.pipelineRender = &pipelineItem;

	// Get stage's framebuffer
	zg::Framebuffer* framebuffer =
		mState->configurable.getFramebuffer(mState->windowFramebuffer, stage.framebufferName);
	sfz_assert(framebuffer != nullptr);

	// In debug mode, validate that the pipeline's render targets matches the framebuffer
#ifndef NDEBUG
	if (framebuffer == &mState->windowFramebuffer) {
		sfz_assert(pipelineItem.numRenderTargets == 1);
		sfz_assert(pipelineItem.renderTargets[0] == ZG_TEXTURE_FORMAT_RGBA_U8_UNORM);
	}
	else {
		FramebufferItem* fbItem = mState->configurable.getFramebufferItem(stage.framebufferName);
		sfz_assert(fbItem != nullptr);
		sfz_assert(pipelineItem.numRenderTargets == fbItem->numRenderTargets);
		for (uint32_t i = 0; i < fbItem->numRenderTargets; i++) {
			sfz_assert(pipelineItem.renderTargets[i] == fbItem->renderTargetItems[i].format);
		}
	}
#endif

	// Get command list for this stage, if it has not yet been created, create it.
	StageCommandList* commandList = mState->getStageCommandList(stageName);
	if (commandList == nullptr) {

		mState->groupCommandLists.add({});
		StageCommandList& list = mState->groupCommandLists.last();
		list.stageName = stageName;

		// Begin recording command list and set pipeline and framebuffer
		CHECK_ZG mState->presentQueue.beginCommandListRecording(list.commandList);
		CHECK_ZG list.commandList.setFramebuffer(*framebuffer);
		CHECK_ZG list.commandList.setPipeline(pipelineItem.pipeline);

		// If first command list created, insert call to profile begin
		if (mState->groupCommandLists.size() == 1) {
			FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);
			sfz_assert(frameIds.groupIds.size() <= mState->currentStageGroupIdx);
			frameIds.groupIds.add({});
			GroupProfilingID& groupId = frameIds.groupIds.last();
			groupId.groupName =
				mState->configurable.presentQueue[mState->currentStageGroupIdx].groupName;
			CHECK_ZG list.commandList.profileBegin(mState->profiler, groupId.id);
		}

		commandList = &list;
	}
	mState->inputEnabled.commandList = commandList;
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
	const ZgPipelineBindingsSignature& signature =
		mState->inputEnabled.pipelineRender->pipeline.bindingsSignature;
	
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

	CHECK_ZG mState->inputEnabledCommandList().setPushConstant(shaderRegister, data, numBytes);
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
	const ZgPipelineBindingsSignature& signature =
		mState->inputEnabled.pipelineRender->pipeline.bindingsSignature;

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
	CHECK_ZG frame->uploadBuffer.memcpyTo(0, data, numBytes);

	// Issue upload to device buffer
	CHECK_ZG mState->inputEnabledCommandList().memcpyBufferToBuffer(
		frame->deviceBuffer, 0, frame->uploadBuffer, 0, numBytes);
}

void Renderer::stageDrawMesh(StringID meshId, const MeshRegisters& registers) noexcept
{
	sfz_assert(meshId != StringID::invalid());
	sfz_assert(inStageInputMode());

	// Find mesh
	GpuMesh* meshPtr = mState->meshes.get(meshId);
	sfz_assert(meshPtr != nullptr);
	if (meshPtr == nullptr) return;

	// Validate some stuff in debug mode
#ifndef NDEBUG
	// Validate pipeline vertex input for standard mesh rendering
	const ZgPipelineRenderSignature& renderSignature =
		mState->inputEnabled.pipelineRender->pipeline.renderSignature;
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

	const ZgPipelineBindingsSignature& bindingsSignature =
		mState->inputEnabled.pipelineRender->pipeline.bindingsSignature;

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
	CHECK_ZG mState->inputEnabledCommandList().setVertexBuffer(0, meshPtr->vertexBuffer);

	// Set index buffer
	sfz_assert(meshPtr->indexBuffer.valid());
	CHECK_ZG mState->inputEnabledCommandList().setIndexBuffer(
		meshPtr->indexBuffer, ZG_INDEX_BUFFER_TYPE_UINT32);

	// Set common pipeline bindings that are same for all components
	zg::PipelineBindings commonBindings;

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

	// Bound render targets
	for (const BoundRenderTarget& target : mState->inputEnabled.stage->boundRenderTargets) {
		
		FramebufferItem* item = mState->configurable.getFramebufferItem(target.framebuffer);
		sfz_assert(item != nullptr);
		if (target.depthBuffer) {
			sfz_assert(item->hasDepthBuffer);
			commonBindings.addTexture(
				target.textureRegister, item->framebuffer.depthBuffer);
		}
		else {
			sfz_assert(target.renderTargetIdx < item->framebuffer.numRenderTargets);
			commonBindings.addTexture(
				target.textureRegister, item->framebuffer.renderTargets[target.renderTargetIdx]);
		}
	}

	// Draw all mesh components
	for (MeshComponent& comp : meshPtr->components) {

		sfz_assert(comp.materialIdx < meshPtr->cpuMaterials.size());
		const Material& material = meshPtr->cpuMaterials[comp.materialIdx];

		// Set material index push constant
		if (registers.materialIdxPushConstant != ~0u) {
			sfz::vec4_u32 tmp = sfz::vec4_u32(0u);
			tmp.x = comp.materialIdx;
			CHECK_ZG mState->inputEnabledCommandList().setPushConstant(
				registers.materialIdxPushConstant, &tmp, sizeof(sfz::vec4_u32 ));
		}

		// Create texture bindings
		zg::PipelineBindings bindings = commonBindings;
		auto bindTexture = [&](uint32_t texRegister, StringID texID) {
			if (texRegister != ~0u && texID != StringID::invalid()) {

				// Find texture
				TextureItem* texItem = mState->textures.get(texID);
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
		CHECK_ZG mState->inputEnabledCommandList().setPipelineBindings(bindings);

		// Issue draw command
		sfz_assert(comp.numIndices != 0);
		sfz_assert((comp.numIndices % 3) == 0);
		CHECK_ZG mState->inputEnabledCommandList().drawTrianglesIndexed(
			comp.firstIndex, comp.numIndices / 3);
	}
}

void Renderer::stageEndInput() noexcept
{
	// Ensure a stage was set to accept input
	sfz_assert(inStageInputMode());
	if (!inStageInputMode()) return;

	// Clear currently active stage info
	mState->inputEnabled = {};
}

bool Renderer::frameProgressNextStageGroup() noexcept
{
	sfz_assert(!inStageInputMode());

	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);

	// Execute command lists
	// TODO: This should be a single "executeCommandLists()" call when that is implemented in ZeroG.
	for (uint32_t groupIdx = 0; groupIdx < mState->groupCommandLists.size(); groupIdx++) {
		StageCommandList& cmdList = mState->groupCommandLists[groupIdx];

		// If last command list to be executed, insert profile end call
		if ((groupIdx + 1) == mState->groupCommandLists.size()) {
			StringID groupName =
				mState->configurable.presentQueue[mState->currentStageGroupIdx].groupName;
			GroupProfilingID* groupId = frameIds.groupIds.find([&](const GroupProfilingID& e) {
				return e.groupName == groupName;
			});
			sfz_assert(groupId != nullptr);
			sfz_assert(groupId->id != ~0ull);
			CHECK_ZG cmdList.commandList.profileEnd(mState->profiler, groupId->id);
		}

		CHECK_ZG mState->presentQueue.executeCommandList(cmdList.commandList);
	}

	// Release command lists
	mState->groupCommandLists.clear();

	// Move to next stage group
	mState->currentStageGroupIdx += 1;
	sfz_assert(mState->currentStageGroupIdx < mState->configurable.presentQueue.size());
	
	return true;
}

void Renderer::frameFinish() noexcept
{
	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);

	// Execute command lists
	// TODO: This should be a single "executeCommandLists()" call when that is implemented in ZeroG.
	for (uint32_t groupIdx = 0; groupIdx < mState->groupCommandLists.size(); groupIdx++) {
		StageCommandList& cmdList = mState->groupCommandLists[groupIdx];

		// If last command list to be executed, insert profile end call
		if ((groupIdx + 1) == mState->groupCommandLists.size()) {
			StringID groupName =
				mState->configurable.presentQueue[mState->currentStageGroupIdx].groupName;
			GroupProfilingID* groupId = frameIds.groupIds.find([&](const GroupProfilingID& e) {
				return e.groupName == groupName;
				});
			sfz_assert(groupId != nullptr);
			sfz_assert(groupId->id != ~0ull);
			CHECK_ZG cmdList.commandList.profileEnd(mState->profiler, groupId->id);
		}

		CHECK_ZG mState->presentQueue.executeCommandList(cmdList.commandList);
	}

	// Release command lists
	mState->groupCommandLists.clear();

	// Render ImGui
	zg::imguiRender(
		mState->imguiRenderState,
		mState->currentFrameIdx,
		mState->presentQueue,
		mState->windowFramebuffer,
		mState->imguiScaleSetting->floatValue(),
		&mState->profiler,
		&frameIds.imguiId);

	// Finish ZeroG frame
	sfz_assert(mState->windowFramebuffer.valid());
	CHECK_ZG mState->zgCtx.swapchainFinishFrame(mState->profiler, frameIds.frameId);
	mState->windowFramebuffer.release();

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
