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

#include <SDL.h>

#include <imgui.h>

#include <skipifzero.hpp>
#include <skipifzero_math.hpp>
#include <skipifzero_new.hpp>

#include <ZeroG.h>

#include <ZeroG-ImGui.hpp>

#include "sfz/debug/ProfilingStats.hpp"
#include "sfz/SfzLogging.h"
#include "sfz/config/SfzConfig.h"
#include "sfz/config/GlobalConfig.hpp"
#include "sfz/renderer/RendererState.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/resources/BufferResource.hpp"
#include "sfz/resources/FramebufferResource.hpp"
#include "sfz/resources/ResourceManager.hpp"
#include "sfz/resources/TextureResource.hpp"
#include "sfz/util/IO.hpp"

// SfzRenderer: State methods
// ------------------------------------------------------------------------------------------------

bool SfzRenderer::init(
	SDL_Window* window,
	const SfzImageViewConst& fontTexture,
	SfzAllocator* allocator,
	SfzConfig* cfg,
	SfzProfilingStats* profStats,
	zg::Uploader&& uploader)
{
	this->destroy();
	mState = sfz_new<SfzRendererState>(allocator, sfz_dbg("SfzRendererState"));
	mState->allocator = allocator;
	mState->window = window;
	mState->uploader = sfz_move(uploader);

	// Settings
	mState->vsync = sfzCfgGetSetting(cfg, "Renderer.vsync");
	mState->flushPresentQueueEachFrame = sfzCfgGetSetting(cfg, "Renderer.flushPresentQueueEachFrame");
	mState->flushCopyQueueEachFrame = sfzCfgGetSetting(cfg, "Renderer.flushCopyQueueEachFrame");
	mState->emitDebugEvents = sfzCfgGetSetting(cfg, "Renderer.emitDebugEvents");

	// Initialize fences
	mState->frameFences.init(mState->frameLatency, [&](FrameFenceData& data) {
		CHECK_ZG data.fence.create();
		CHECK_ZG mState->uploader.getCurrentOffset(data.safeUploaderOffset);
	});

	// Get window resolution
	mState->windowRes;
	SDL_GL_GetDrawableSize(window, &mState->windowRes.x, &mState->windowRes.y);

	// Get command queues
	mState->presentQueue = zg::CommandQueue::getPresentQueue();
	mState->copyQueue = zg::CommandQueue::getCopyQueue();

	// Initialize profiler
	{
		ZgProfilerDesc desc = {};
		desc.maxNumMeasurements = 1024;
		CHECK_ZG mState->profiler.create(desc);
		mState->frameMeasurementIds.init(mState->frameLatency);
	}

	// Initialize ImGui rendering state
	mState->imguiScaleSetting = sfzCfgGetSetting(cfg, "Imgui.scale");
	sfz_assert(fontTexture.type == SFZ_IMAGE_TYPE_R_U8);
	ZgImageViewConstCpu zgFontTextureView = {};
	zgFontTextureView.format = ZG_FORMAT_R_U8_UNORM;
	zgFontTextureView.data = fontTexture.rawData;
	zgFontTextureView.width = fontTexture.width;
	zgFontTextureView.height = fontTexture.height;
	zgFontTextureView.pitchInBytes = fontTexture.width * sizeof(u8);
	bool imguiInitSuccess = CHECK_ZG zg::imguiInitRenderState(
		mState->imguiRenderState,
		mState->frameLatency,
		mState->allocator,
		mState->uploader.handle,
		mState->copyQueue,
		zgFontTextureView);
	if (!imguiInitSuccess) {
		this->destroy();
		return false;
	}

	// Initialize profiling stats
	profStats->createCategory("gpu", 300, 66.7f, "ms", "frame", 20.0f,
		SfzStatsVisualizationType::FIRST_INDIVIDUALLY_REST_ADDED);
	profStats->createLabel("gpu", "frametime", f32x4(1.0f, 0.0f, 0.0f, 1.0f), 0.0f);
	profStats->createLabel("gpu", "imgui");

	return true;
}

void SfzRenderer::destroy()
{
	if (mState != nullptr) {

		// Flush queues
		CHECK_ZG mState->presentQueue.flush();
		CHECK_ZG mState->copyQueue.flush();

		// Destroy ImGui renderer
		zg::imguiDestroyRenderState(mState->imguiRenderState);
		mState->imguiRenderState = nullptr;

		// Deallocate rest of state
		SfzAllocator* allocator = mState->allocator;
		sfz_delete(allocator, mState);
	}
	mState = nullptr;
}

// SfzRenderer: Getters
// ------------------------------------------------------------------------------------------------

u64 SfzRenderer::currentFrameIdx() const
{
	return mState->currentFrameIdx;
}

i32x2 SfzRenderer::windowResolution() const
{
	return mState->windowRes;
}

void SfzRenderer::frameTimeMs(u64& frameIdxOut, f32& frameTimeMsOut) const
{
	frameIdxOut = mState->lastRetrievedFrameTimeFrameIdx;
	frameTimeMsOut = mState->lastRetrievedFrameTimeMs;
}

ZgUploader* SfzRenderer::getUploader()
{
	return mState->uploader.handle;
}

// SfzRenderer: ImGui UI methods
// ------------------------------------------------------------------------------------------------

void SfzRenderer::renderImguiUI()
{
	mState->ui.render(*mState);
}

// SfzRenderer: Resource methods
// ------------------------------------------------------------------------------------------------

bool SfzRenderer::uploadTextureBlocking(
	SfzStrID id, const SfzImageViewConst& image, bool generateMipmaps, SfzStrIDs* ids, SfzResourceManager* resMan)
{
	// Error out and return false if texture already exists
	if (resMan->getTextureHandle(id) != SFZ_NULL_HANDLE) return false;

	// Create resource and upload blocking
	SfzTextureResource resource = SfzTextureResource::createFixedSize(sfzStrIDGetStr(ids, id), ids, image, generateMipmaps);
	sfz_assert(resource.texture.valid());
	resource.uploadBlocking(image, mState->allocator, mState->uploader.handle, mState->copyQueue);
	
	// Add to resource manager
	resMan->addTexture(sfz_move(resource));

	return true;
}

bool SfzRenderer::textureLoaded(SfzStrID id, const SfzResourceManager* resMan) const
{
	return resMan->getTextureHandle(id) != SFZ_NULL_HANDLE;
}

void SfzRenderer::removeTextureGpuBlocking(SfzStrID id, SfzResourceManager* resMan)
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());
	resMan->removeTexture(id);
}

// SfzRenderer: Render methods
// ------------------------------------------------------------------------------------------------

void SfzRenderer::frameBegin(
	SfzStrIDs* ids, SfzShaderManager* shaderMan, SfzResourceManager* resMan, SfzProfilingStats* profStats)
{
	// Increment frame index
	mState->currentFrameIdx += 1;

	// Wait on fence to ensure we have finished rendering frame that previously used this data
	FrameFenceData& frameFenceData = mState->frameFences.data(mState->currentFrameIdx);
	CHECK_ZG frameFenceData.fence.waitOnCpuBlocking();
	
	// Once we have reached this fence, it is safe to repurpose memory in the uploader
	CHECK_ZG mState->uploader.setSafeOffset(frameFenceData.safeUploaderOffset);

	// Get frame profiling data for frame that was previously rendered using these resources
	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);
	if (frameIds.frameId != ~0ull) {
		CHECK_ZG mState->profiler.getMeasurement(frameIds.frameId, mState->lastRetrievedFrameTimeMs);
		mState->lastRetrievedFrameTimeFrameIdx = mState->currentFrameIdx - mState->frameLatency;
		profStats->addSample("gpu", "frametime",
			mState->lastRetrievedFrameTimeFrameIdx, mState->lastRetrievedFrameTimeMs);
	}
	for (const GroupProfilingID& groupId : frameIds.groupIds) {
		u64 frameIdx = mState->lastRetrievedFrameTimeFrameIdx;
		f32 groupTimeMs = 0.0f;
		CHECK_ZG mState->profiler.getMeasurement(groupId.id, groupTimeMs);
		const char* label = sfzStrIDGetStr(ids, groupId.groupName);
		profStats->addSample("gpu", label, frameIdx, groupTimeMs);
	}
	if (frameIds.imguiId != ~0ull) {
		u64 frameIdx = mState->lastRetrievedFrameTimeFrameIdx;
		f32 imguiTimeMs = 0.0f;
		CHECK_ZG mState->profiler.getMeasurement(frameIds.imguiId, imguiTimeMs);
		profStats->addSample("gpu", "imgui", frameIdx, imguiTimeMs);
		frameIds.imguiId = ~0ull;
	}
	frameIds.groupIds.clear();

	// Query drawable width and height from SDL
	i32 newResX = 0;
	i32 newResY = 0;
	SDL_GL_GetDrawableSize(mState->window, &newResX, &newResY);
	bool resolutionChanged = newResX != mState->windowRes.x || newResY != mState->windowRes.y;

	// If resolution has changed, resize swapchain
	if (resolutionChanged) {

		SFZ_LOG_INFO("Resolution changed, new resolution: %i x %i. Updating framebuffers...",
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
			u32(mState->windowRes.x), u32(mState->windowRes.y));
	}

	// Update shaders
	shaderMan->update();

	// Update resources with current resolution
	resMan->updateResolution(i32x2(newResX, newResY), ids);

	// Set vsync settings
	CHECK_ZG zgContextSwapchainSetVsync(mState->vsync->boolValue() ? ZG_TRUE : ZG_FALSE);
	
	// Begin ZeroG frame
	sfz_assert(!mState->windowFramebuffer.valid());
	CHECK_ZG zgContextSwapchainBeginFrame(
		&mState->windowFramebuffer.handle, mState->profiler.handle, &frameIds.frameId);
}

sfz::HighLevelCmdList SfzRenderer::beginCommandList(
	const char* cmdListName,
	SfzStrIDs* ids,
	SfzProfilingStats* profStats,
	SfzShaderManager* shaderMan,
	SfzResourceManager* resMan)
{
	// Create profiling stats label if it doesn't exist
	SfzProfilingStats& stats = *profStats;
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
	groupId.groupName = sfzStrIDCreateRegister(ids, cmdListName);
	CHECK_ZG zgCmdList.profileBegin(mState->profiler, groupId.id);

	// Create high level command list
	sfz::HighLevelCmdList cmdList;
	cmdList.init(
		cmdListName,
		mState->currentFrameIdx,
		sfz_move(zgCmdList),
		&mState->uploader,
		&mState->windowFramebuffer,
		ids,
		resMan,
		shaderMan);

	return cmdList;
}

void SfzRenderer::executeCommandList(sfz::HighLevelCmdList cmdList)
{
	sfz_assert(cmdList.mCmdList.valid());

	// Insert profile end call
	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);
	SfzStrID cmdListName = cmdList.mName;
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

void SfzRenderer::frameFinish()
{
	FrameProfilingIDs& frameIds = mState->frameMeasurementIds.data(mState->currentFrameIdx);

	// This is a workaround for a particularly nasty bug. For some reason the D3D12 validation fails
	// with "device removed" and some invalid access, but its really hard to get it to tell exactly
	// what goes wrong. After a lot of investigation the conclusion is that we sometimes fail when
	// we don't have any imgui content on screen and execute the "empty" command list below. Can be
	// reproduced by:
	//
	//  zg::CommandList cmdList;
	//  CHECK_ZG mState->presentQueue.beginCommandListRecording(cmdList);
	//  CHECK_ZG cmdList.setFramebuffer(mState->windowFramebuffer);
	//  CHECK_ZG mState->presentQueue.executeCommandList(cmdList);
	//
	// I suspect something is slightly wrong with ZeroG's resource transitions for the default
	// framebuffer (special case versus non-default framebuffers), but I don't know what. The fix
	// below is simple enough that I don't feel justified spending time on it, but above information
	// is a start if it turns up again.
	ImGui::Render();
	ImDrawData& imguiDrawData = *ImGui::GetDrawData();
	if (imguiDrawData.CmdListsCount > 0) {
		zg::CommandList cmdList;
		CHECK_ZG mState->presentQueue.beginCommandListRecording(cmdList);
		CHECK_ZG cmdList.setFramebuffer(mState->windowFramebuffer);

		// Render ImGui
		zg::imguiRender(
			mState->imguiRenderState,
			mState->currentFrameIdx,
			cmdList,
			mState->uploader.handle,
			u32(mState->windowRes.x),
			u32(mState->windowRes.y),
			mState->imguiScaleSetting->floatValue(),
			&mState->profiler,
			&frameIds.imguiId);

		// Execute command list
		CHECK_ZG mState->presentQueue.executeCommandList(cmdList);
	}

	// Finish ZeroG frame
	sfz_assert(mState->windowFramebuffer.valid());
	CHECK_ZG zgContextSwapchainFinishFrame(mState->profiler.handle, frameIds.frameId);
	mState->windowFramebuffer.destroy();

	// Signal that we are done rendering use these resources, record offset in uploader at this time
	FrameFenceData& frameFenceData = mState->frameFences.data(mState->currentFrameIdx);
	CHECK_ZG mState->uploader.getCurrentOffset(frameFenceData.safeUploaderOffset);
	CHECK_ZG mState->presentQueue.signalOnGpu(frameFenceData.fence);

	// Flush queues if requested
	if (mState->flushPresentQueueEachFrame->boolValue()) {
		CHECK_ZG mState->presentQueue.flush();
	}
	if (mState->flushCopyQueueEachFrame->boolValue()) {
		CHECK_ZG mState->copyQueue.flush();
	}
}
