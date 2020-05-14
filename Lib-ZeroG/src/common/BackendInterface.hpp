// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#pragma once

#include "ZeroG.h"

// Backend interface
// ------------------------------------------------------------------------------------------------

struct ZgBackend {
	virtual ~ZgBackend() noexcept {}

	// Context methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult swapchainResize(
		uint32_t width,
		uint32_t height) noexcept = 0;

	virtual ZgResult setVsync(
		bool vsync) noexcept = 0;

	virtual ZgResult swapchainBeginFrame(
		ZgFramebuffer** framebufferOut,
		ZgProfiler* profiler,
		uint64_t* measurementIdOut) noexcept = 0;

	virtual ZgResult swapchainFinishFrame(
		ZgProfiler* profiler,
		uint64_t measurementId) noexcept = 0;

	virtual ZgResult fenceCreate(ZgFence** fenceOut) noexcept = 0;

	// Stats
	// --------------------------------------------------------------------------------------------

	virtual ZgResult getStats(ZgStats& statsOut) noexcept = 0;

	// Pipeline compute methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult pipelineComputeCreateFromFileHLSL(
		ZgPipelineCompute** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineComputeSignature* computeSignatureOut,
		const ZgPipelineComputeCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept = 0;

	virtual ZgResult pipelineComputeRelease(
		ZgPipelineCompute* pipeline) noexcept = 0;

	// Pipeline render methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult pipelineRenderCreateFromFileSPIRV(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo) noexcept = 0;

	virtual ZgResult pipelineRenderCreateFromFileHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept = 0;

	virtual ZgResult pipelineRenderCreateFromSourceHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept = 0;

	virtual ZgResult pipelineRenderRelease(
		ZgPipelineRender* pipeline) noexcept = 0;

	// Memory methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult memoryHeapCreate(
		ZgMemoryHeap** memoryHeapOut,
		const ZgMemoryHeapCreateInfo& createInfo) noexcept = 0;

	virtual ZgResult memoryHeapRelease(
		ZgMemoryHeap* memoryHeap) noexcept = 0;

	// Texture methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult texture2DGetAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept = 0;

	// Framebuffer methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult framebufferCreate(
		ZgFramebuffer** framebufferOut,
		const ZgFramebufferCreateInfo& createInfo) noexcept = 0;

	virtual void framebufferRelease(
		ZgFramebuffer* framebuffer) noexcept = 0;

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult getPresentQueue(ZgCommandQueue** presentQueueOut) noexcept = 0;
	virtual ZgResult getCopyQueue(ZgCommandQueue** copyQueueOut) noexcept = 0;

	// Profiler methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult profilerCreate(
		ZgProfiler** profilerOut,
		const ZgProfilerCreateInfo& createInfo) noexcept = 0;

	virtual void profilerRelease(
		ZgProfiler* profilerIn) noexcept = 0;
};

// Memory heap
// ------------------------------------------------------------------------------------------------

struct ZgMemoryHeap {
	virtual ~ZgMemoryHeap() noexcept {}

	virtual ZgResult bufferCreate(
		ZgBuffer** bufferOut,
		const ZgBufferCreateInfo& createInfo) noexcept = 0;

	virtual ZgResult texture2DCreate(
		ZgTexture2D** textureOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept = 0;
};

// Buffers
// ------------------------------------------------------------------------------------------------

struct ZgBuffer {
	virtual ~ZgBuffer() noexcept {}

	virtual ZgResult memcpyTo(
		uint64_t dstBufferOffsetBytes,
		const void* srcMemory,
		uint64_t numBytes) noexcept = 0;

	virtual ZgResult memcpyFrom(
		uint64_t srcBufferOffsetBytes,
		void* dstMemory,
		uint64_t numBytes) noexcept = 0;

	virtual ZgResult setDebugName(
		const char* name) noexcept = 0;
};

// Textures
// ------------------------------------------------------------------------------------------------

struct ZgTexture2D {
	virtual ~ZgTexture2D() noexcept {}

	virtual ZgResult setDebugName(
		const char* name) noexcept = 0;
};

// Framebuffer
// ------------------------------------------------------------------------------------------------

struct ZgFramebuffer {
	virtual ~ZgFramebuffer() noexcept {}

	virtual ZgResult getResolution(uint32_t& widthOut, uint32_t& heightOut) const noexcept = 0;
};

// Fence
// ------------------------------------------------------------------------------------------------

struct ZgFence {
	virtual ~ZgFence() noexcept {}

	virtual ZgResult reset() noexcept = 0;
	virtual ZgResult checkIfSignaled(bool& fenceSignaledOut) const noexcept = 0;
	virtual ZgResult waitOnCpuBlocking() const noexcept = 0;
};

// Profiler
// ------------------------------------------------------------------------------------------------

struct ZgProfiler {
	virtual ~ZgProfiler() noexcept {}

	virtual ZgResult getMeasurement(
		uint64_t measurementId,
		float& measurementMsOut) noexcept = 0;
};
