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

// Renderer: Render methods
// ------------------------------------------------------------------------------------------------

void Renderer::frameBegin()
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
}

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

void Renderer::frameFinish()
{
	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);

	zg::CommandList cmdList;
	CHECK_ZG mState->presentQueue.beginCommandListRecording(cmdList);
	CHECK_ZG cmdList.setFramebuffer(mState->windowFramebuffer);

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
