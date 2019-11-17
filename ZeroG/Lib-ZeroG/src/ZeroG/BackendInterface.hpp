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

	virtual ZgResult swapchainBeginFrame(
		ZgFramebuffer** framebufferOut) noexcept = 0;

	virtual ZgResult swapchainFinishFrame() noexcept = 0;

	virtual ZgResult fenceCreate(ZgFence** fenceOut) noexcept = 0;

	// Stats
	// --------------------------------------------------------------------------------------------

	virtual ZgResult getStats(ZgStats& statsOut) noexcept = 0;

	// Pipeline methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult pipelineRenderCreateFromFileSPIRV(
		ZgPipelineRender** pipelineOut,
		ZgPipelineRenderSignature* signatureOut,
		const ZgPipelineRenderCreateInfoFileSPIRV& createInfo) noexcept = 0;

	virtual ZgResult pipelineRenderCreateFromFileHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineRenderSignature* signatureOut,
		const ZgPipelineRenderCreateInfoFileHLSL& createInfo) noexcept = 0;

	virtual ZgResult pipelineRenderCreateFromSourceHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineRenderSignature* signatureOut,
		const ZgPipelineRenderCreateInfoSourceHLSL& createInfo) noexcept = 0;

	virtual ZgResult pipelineRenderRelease(
		ZgPipelineRender* pipeline) noexcept = 0;

	virtual ZgResult pipelineRenderGetSignature(
		const ZgPipelineRender* pipeline,
		ZgPipelineRenderSignature* signatureOut) const noexcept = 0;

	// Memory methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult memoryHeapCreate(
		ZgMemoryHeap** memoryHeapOut,
		const ZgMemoryHeapCreateInfo& createInfo) noexcept = 0;

	virtual ZgResult memoryHeapRelease(
		ZgMemoryHeap* memoryHeap) noexcept = 0;

	virtual ZgResult bufferMemcpyTo(
		ZgBuffer* dstBufferInterface,
		uint64_t bufferOffsetBytes,
		const uint8_t* srcMemory,
		uint64_t numBytes) noexcept = 0;

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
};

// PipelineRender
// ------------------------------------------------------------------------------------------------

struct ZgPipelineRender {
	virtual ~ZgPipelineRender() noexcept {}
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

// Command queue
// ------------------------------------------------------------------------------------------------

struct ZgCommandQueue {
	virtual ~ZgCommandQueue() noexcept {}

	virtual ZgResult signalOnGpu(ZgFence& fenceToSignal) noexcept = 0;
	virtual ZgResult waitOnGpu(const ZgFence& fence) noexcept = 0;
	virtual ZgResult flush() noexcept = 0;
	virtual ZgResult beginCommandListRecording(ZgCommandList** commandListOut) noexcept = 0;
	virtual ZgResult executeCommandList(ZgCommandList* commandList) noexcept = 0;
};

// Command lists
// ------------------------------------------------------------------------------------------------

struct ZgCommandList {
	virtual ~ZgCommandList() noexcept {};

	virtual ZgResult memcpyBufferToBuffer(
		ZgBuffer* dstBuffer,
		uint64_t dstBufferOffsetBytes,
		ZgBuffer* srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept = 0;

	virtual ZgResult memcpyToTexture(
		ZgTexture2D* dstTexture,
		uint32_t dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		ZgBuffer* tempUploadBuffer) noexcept = 0;

	virtual ZgResult enableQueueTransitionBuffer(ZgBuffer* buffer) noexcept = 0;

	virtual ZgResult enableQueueTransitionTexture(ZgTexture2D* texture) noexcept = 0;

	virtual ZgResult setPushConstant(
		uint32_t shaderRegister,
		const void* data,
		uint32_t dataSizeInBytes) noexcept = 0;

	virtual ZgResult setPipelineBindings(
		const ZgPipelineBindings& bindings) noexcept = 0;

	virtual ZgResult setPipelineRender(
		ZgPipelineRender* pipeline) noexcept = 0;

	virtual ZgResult setFramebuffer(
		ZgFramebuffer* framebuffer,
		const ZgFramebufferRect* optionalViewport,
		const ZgFramebufferRect* optionalScissor) noexcept = 0;

	virtual ZgResult setFramebufferViewport(
		const ZgFramebufferRect& viewport) noexcept = 0;

	virtual ZgResult setFramebufferScissor(
		const ZgFramebufferRect& scissor) noexcept = 0;

	virtual ZgResult clearFramebufferOptimal() noexcept = 0;

	virtual ZgResult clearRenderTargets(
		float red,
		float green,
		float blue,
		float alpha) noexcept = 0;

	virtual ZgResult clearDepthBuffer(
		float depth) noexcept = 0;

	virtual ZgResult setIndexBuffer(
		ZgBuffer* indexBuffer,
		ZgIndexBufferType type) noexcept = 0;

	virtual ZgResult setVertexBuffer(
		uint32_t vertexBufferSlot,
		ZgBuffer* vertexBuffer) noexcept = 0;

	virtual ZgResult drawTriangles(
		uint32_t startVertexIndex,
		uint32_t numVertices) noexcept = 0;

	virtual ZgResult drawTrianglesIndexed(
		uint32_t startIndex,
		uint32_t numTriangles) noexcept = 0;
};
