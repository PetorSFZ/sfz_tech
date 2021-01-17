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
#include "sfz/renderer/RendererConfigParser.hpp"
#include "sfz/renderer/RendererState.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/resources/BufferResource.hpp"
#include "sfz/resources/FramebufferResource.hpp"
#include "sfz/resources/ResourceManager.hpp"
#include "sfz/resources/TextureResource.hpp"
#include "sfz/util/IO.hpp"

namespace sfz {

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

// High level command list methods
// --------------------------------------------------------------------------------------------

HighLevelCmdList Renderer::beginCommandList(const char* cmdListName)
{
	// Create profiling stats label if it doesn't exist
	ProfilingStats& stats = getProfilingStats();
	if (!stats.labelExists("gpu", cmdListName)) {
		stats.createLabel("gpu", cmdListName);
	}

	// Begin ZeroG command list on present queue
	zg::CommandList zgCmdList;
	CHECK_ZG mState->presentQueue.beginCommandListRecording(zgCmdList);

	// Add event
	if (mState->emitDebugEvents->boolValue()) {
		CHECK_ZG zgCmdList.beginEvent(cmdListName);
	}

	// Insert call to profile begin
	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);
	GroupProfilingID& groupId = frameIds.groupIds.add();
	groupId.groupName = strID(cmdListName);
	CHECK_ZG zgCmdList.profileBegin(mState->profiler, groupId.id);

	// Create high level command list
	HighLevelCmdList cmdList;
	cmdList.init(cmdListName, mState->currentFrameIdx, std::move(zgCmdList), &mState->windowFramebuffer);

	return cmdList;
}

void Renderer::executeCommandList(HighLevelCmdList cmdList)
{
	sfz_assert(cmdList.mCmdList.valid());

	// Insert profile end call
	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);
	strID cmdListName = cmdList.mName;
	GroupProfilingID* groupId = frameIds.groupIds.find([&](const GroupProfilingID& e) {
		return e.groupName == cmdListName;
	});
	sfz_assert(groupId != nullptr);
	sfz_assert(groupId->id != ~0ull);
	CHECK_ZG cmdList.mCmdList.profileEnd(mState->profiler, groupId->id);

	// Insert event end call
	if (mState->emitDebugEvents->boolValue()) {
		CHECK_ZG cmdList.mCmdList.endEvent();
	}

	// Execute command list
	CHECK_ZG mState->presentQueue.executeCommandList(cmdList.mCmdList);
}

// Renderer: Resource methods
// ------------------------------------------------------------------------------------------------

bool Renderer::uploadTextureBlocking(
	strID id, const ImageViewConst& image, bool generateMipmaps) noexcept
{
	// Error out and return false if texture already exists
	ResourceManager& resources = getResourceManager();
	if (resources.getTextureHandle(id) != NULL_HANDLE) return false;

	// Create resource and upload blocking
	TextureResource resource = TextureResource::createFixedSize(id.str(), image, generateMipmaps);
	sfz_assert(resource.texture.valid());
	resource.uploadBlocking(image, mState->allocator, mState->copyQueue);
	
	// Add to resource manager
	resources.addTexture(std::move(resource));

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
	ResourceManager& resources = getResourceManager();
	sfz_assert(id.isValid());
	if (resources.getMeshHandle(id) != NULL_HANDLE) return false;

	// Allocate memory for mesh
	MeshResource gpuMesh = meshResourceAllocate(id.str(), mesh, mState->allocator);

	// Upload memory to mesh
	meshResourceUploadBlocking(
		gpuMesh, mesh, mState->allocator, mState->copyQueue);

	// Store mesh
	resources.addMesh(std::move(gpuMesh));

	return true;
}

bool Renderer::meshLoaded(strID id) const noexcept
{
	ResourceManager& resources = getResourceManager();
	return resources.getMeshHandle(id) != NULL_HANDLE;
}

void Renderer::removeMeshGpuBlocking(strID id) noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	ResourceManager& resources = getResourceManager();
	resources.removeMesh(id);
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

	// If resolution has changed, resize swapchain
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
	}

	// Update resources with current resolution
	ResourceManager& resources = getResourceManager();
	resources.updateResolution(vec2_u32(newResX, newResY));

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
	ShaderManager& shaders = getShaderManager();
	Shader* shader = shaders.getShader(shaders.getShaderHandle(stage.pipelineName));
	sfz_assert(shader != nullptr);

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

	// Set pipeline
	if (stage.type == StageType::USER_INPUT_RENDERING) {
		sfz_assert(shader->type == ShaderType::RENDER);
		sfz_assert(shader->render.pipeline.valid());
		CHECK_ZG cmdList.setPipeline(shader->render.pipeline);
	}
	else if (stage.type == StageType::USER_INPUT_COMPUTE) {
		sfz_assert(shader->type == ShaderType::COMPUTE);
		sfz_assert(shader->compute.pipeline.valid());
		CHECK_ZG cmdList.setPipeline(shader->compute.pipeline);
	}
	else {
		sfz_assert(false);
	}
}

void Renderer::stageSetFramebuffer(const char* framebufferName) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);

	ResourceManager& resources = getResourceManager();
	FramebufferResource* fb = resources.getFramebuffer(resources.getFramebufferHandle(framebufferName));
	sfz_assert(fb != nullptr);
	CHECK_ZG mState->groupCmdList.setFramebuffer(fb->framebuffer);
}

void Renderer::stageSetFramebufferDefault() noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	CHECK_ZG mState->groupCmdList.setFramebuffer(mState->windowFramebuffer);
}

void Renderer::stageUploadToStreamingBufferUntyped(
	const char* bufferName, const void* data, uint32_t elementSize, uint32_t numElements) noexcept
{
	sfz_assert(inStageInputMode());
	
	// Get streaming buffer
	ResourceManager& resources = getResourceManager();
	BufferResource* resource = resources.getBuffer(resources.getBufferHandle(bufferName));
	sfz_assert(resource != nullptr);
	sfz_assert(resource->type == BufferResourceType::STREAMING);

	// Calculate number of bytes to copy to streaming buffer
	const uint32_t numBytes = elementSize * numElements;
	sfz_assert(numBytes != 0);
	sfz_assert(numBytes <= (resource->elementSizeBytes * resource->maxNumElements));
	sfz_assert(elementSize == resource->elementSizeBytes); // TODO: Might want to remove this assert

	// Grab this frame's memory
	StreamingBufferMemory& memory = resource->streamingMem.data(mState->currentFrameIdx);

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
	ShaderManager& shaders = getShaderManager();
	Shader* shader = shaders.getShader(shaders.getShaderHandle(mState->inputEnabled.stage->pipelineName));
	sfz_assert(shader != nullptr);

	ZgPipelineBindingsSignature signature = {};
	if (mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING) {
		sfz_assert(shader->type == ShaderType::RENDER);
		signature = shader->render.pipeline.getSignature().bindings;
	}
	else if (mState->inputEnabled.stage->type == StageType::USER_INPUT_COMPUTE) {
		sfz_assert(shader->type == ShaderType::COMPUTE);
		signature = shader->compute.pipeline.getBindingsSignature();
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

void Renderer::stageSetBindings(const PipelineBindings& bindings) noexcept
{
	ResourceManager& resources = getResourceManager();
	zg::PipelineBindings zgBindings;
	
	// Constant buffers
	for (const Binding& binding : bindings.constBuffers) {
		
		ZgBuffer* buffer = nullptr;
		if (binding.type == BindingResourceType::ID) {
			
			BufferResource* resource =
				resources.getBuffer(resources.getBufferHandle(binding.resource.resourceID));
			sfz_assert(resource != nullptr);

			if (resource->type == BufferResourceType::STATIC) {
				buffer = resource->staticMem.buffer.handle;
			}
			else if (resource->type == BufferResourceType::STREAMING) {
				buffer = resource->streamingMem.data(mState->currentFrameIdx).deviceBuffer.handle;
			}
			else {
				sfz_assert(false);
			}
		}
		else if (binding.type == BindingResourceType::RAW_BUFFER) {
			buffer = binding.resource.rawBuffer;
		}
		sfz_assert(buffer != nullptr);

		sfz_assert(binding.shaderRegister != ~0u);
		ZgConstantBufferBinding cbufferBinding = {};
		cbufferBinding.buffer = buffer;
		cbufferBinding.bufferRegister = binding.shaderRegister;
		zgBindings.addConstantBuffer(cbufferBinding);
	}

	// Textures
	for (const Binding& binding : bindings.textures) {
		sfz_assert(binding.type == BindingResourceType::ID);

		TextureResource* tex =
			resources.getTexture(resources.getTextureHandle(binding.resource.resourceID));
		sfz_assert(tex != nullptr);
		sfz_assert(binding.shaderRegister != ~0u);
		zgBindings.addTexture(binding.shaderRegister, tex->texture);
	}

	// Unordered buffers
	for (const Binding& binding : bindings.unorderedBuffers) {
		sfz_assert(binding.type == BindingResourceType::ID);

		BufferResource* resource =
			resources.getBuffer(resources.getBufferHandle(binding.resource.resourceID));
		sfz_assert(resource != nullptr);

		zg::Buffer* buffer = nullptr;
		if (resource->type == BufferResourceType::STATIC) {
			buffer = &resource->staticMem.buffer;
		}
		else if (resource->type == BufferResourceType::STREAMING) {
			buffer = &resource->streamingMem.data(mState->currentFrameIdx).deviceBuffer;
		}
		else {
			sfz_assert(false);
		}

		sfz_assert(binding.shaderRegister != ~0u);
		zgBindings.addUnorderedBuffer(
			binding.shaderRegister, resource->maxNumElements, resource->elementSizeBytes, *buffer);
	}

	// Unordered textures
	for (const Binding& binding : bindings.unorderedTextures) {
		sfz_assert(binding.type == BindingResourceType::ID);

		TextureResource* tex =
			resources.getTexture(resources.getTextureHandle(binding.resource.resourceID));
		sfz_assert(tex != nullptr);
		sfz_assert(binding.shaderRegister != ~0u);
		zgBindings.addUnorderedTexture(binding.shaderRegister, binding.mipLevel, tex->texture);
	}

	CHECK_ZG mState->groupCmdList.setPipelineBindings(zgBindings);
}

void Renderer::stageSetVertexBuffer(const char* bufferName) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	ResourceManager& resources = getResourceManager();

	// Grab buffer
	BufferResource* resource =
		resources.getBuffer(resources.getBufferHandle(bufferName));
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == BufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == BufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mState->currentFrameIdx).deviceBuffer;
	}
	else {
		sfz_assert(false);
	}

	// Set vertex buffer
	CHECK_ZG mState->groupCmdList.setVertexBuffer(0, *buffer);
}

void Renderer::stageSetIndexBuffer(const char* bufferName, ZgIndexBufferType indexBufferType) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	ResourceManager& resources = getResourceManager();

	// Grab buffer
	BufferResource* resource =
		resources.getBuffer(resources.getBufferHandle(bufferName));
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == BufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == BufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mState->currentFrameIdx).deviceBuffer;
	}
	else {
		sfz_assert(false);
	}

	// Set index buffer
	CHECK_ZG mState->groupCmdList.setIndexBuffer(*buffer, indexBufferType);
}

void Renderer::stageSetIndexBuffer(zg::Buffer& buffer, ZgIndexBufferType indexBufferType) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	CHECK_ZG mState->groupCmdList.setIndexBuffer(buffer, indexBufferType);
}

void Renderer::stageSetVertexBuffer(uint32_t slot, zg::Buffer& buffer) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	CHECK_ZG mState->groupCmdList.setVertexBuffer(slot, buffer);
}

void Renderer::stageSetIndexBuffer(PoolHandle handle, ZgIndexBufferType indexBufferType)
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	ResourceManager& resources = getResourceManager();

	// Grab buffer
	BufferResource* resource = resources.getBuffer(handle);
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == BufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == BufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mState->currentFrameIdx).deviceBuffer;
	}
	else {
		sfz_assert(false);
	}

	// Set index buffer
	CHECK_ZG mState->groupCmdList.setIndexBuffer(*buffer, indexBufferType);
}

void Renderer::stageSetVertexBuffer(uint32_t slot, PoolHandle handle)
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_RENDERING);
	ResourceManager& resources = getResourceManager();

	// Grab buffer
	BufferResource* resource = resources.getBuffer(handle);
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == BufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == BufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mState->currentFrameIdx).deviceBuffer;
	}
	else {
		sfz_assert(false);
	}

	// Set vertex buffer
	CHECK_ZG mState->groupCmdList.setVertexBuffer(slot, *buffer);
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

void Renderer::stageUnorderedBarrierBuffer(const char* bufferName) noexcept
{
	sfz_assert(inStageInputMode());
	ResourceManager& resources = getResourceManager();

	// Grab buffer
	BufferResource* resource =
		resources.getBuffer(resources.getBufferHandle(bufferName));
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == BufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == BufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mState->currentFrameIdx).deviceBuffer;
	}
	else {
		sfz_assert(false);
	}

	CHECK_ZG mState->groupCmdList.unorderedBarrier(*buffer);
}

void Renderer::stageUnorderedBarrierTexture(const char* textureName) noexcept
{
	sfz_assert(inStageInputMode());
	ResourceManager& resources = getResourceManager();
	TextureResource* tex = resources.getTexture(resources.getTextureHandle(textureName));
	sfz_assert(tex != nullptr);
	CHECK_ZG mState->groupCmdList.unorderedBarrier(tex->texture);
}

vec3_i32 Renderer::stageGetComputeGroupDims() noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(mState->inputEnabled.stage->type == StageType::USER_INPUT_COMPUTE);

	ShaderManager& shaders = getShaderManager();
	Shader* shader = shaders.getShader(shaders.getShaderHandle(mState->inputEnabled.stage->pipelineName));
	sfz_assert(shader != nullptr);
	sfz_assert(shader->type == ShaderType::COMPUTE);

	vec3_u32 groupDims;
	shader->compute.pipeline.getGroupDims(groupDims.x, groupDims.y, groupDims.z);
	return vec3_i32(groupDims);
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
