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

	virtual ZgErrorCode swapchainResize(
		uint32_t width,
		uint32_t height) noexcept = 0;

	virtual ZgErrorCode swapchainBeginFrame(
		ZgFramebuffer** framebufferOut) noexcept = 0;

	virtual ZgErrorCode swapchainFinishFrame() noexcept = 0;

	virtual ZgErrorCode fenceCreate(ZgFence** fenceOut) noexcept = 0;

	// Stats
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode getStats(ZgStats& statsOut) noexcept = 0;

	// Pipeline methods
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode pipelineRenderingCreateFromFileSPIRV(
		ZgPipelineRendering** pipelineOut,
		ZgPipelineRenderingSignature* signatureOut,
		const ZgPipelineRenderingCreateInfoFileSPIRV& createInfo) noexcept = 0;

	virtual ZgErrorCode pipelineRenderingCreateFromFileHLSL(
		ZgPipelineRendering** pipelineOut,
		ZgPipelineRenderingSignature* signatureOut,
		const ZgPipelineRenderingCreateInfoFileHLSL& createInfo) noexcept = 0;

	virtual ZgErrorCode pipelineRenderingCreateFromSourceHLSL(
		ZgPipelineRendering** pipelineOut,
		ZgPipelineRenderingSignature* signatureOut,
		const ZgPipelineRenderingCreateInfoSourceHLSL& createInfo) noexcept = 0;

	virtual ZgErrorCode pipelineRenderingRelease(
		ZgPipelineRendering* pipeline) noexcept = 0;

	virtual ZgErrorCode pipelineRenderingGetSignature(
		const ZgPipelineRendering* pipeline,
		ZgPipelineRenderingSignature* signatureOut) const noexcept = 0;

	// Memory methods
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode memoryHeapCreate(
		ZgMemoryHeap** memoryHeapOut,
		const ZgMemoryHeapCreateInfo& createInfo) noexcept = 0;

	virtual ZgErrorCode memoryHeapRelease(
		ZgMemoryHeap* memoryHeap) noexcept = 0;

	virtual ZgErrorCode bufferMemcpyTo(
		ZgBuffer* dstBufferInterface,
		uint64_t bufferOffsetBytes,
		const uint8_t* srcMemory,
		uint64_t numBytes) noexcept = 0;

	// Texture methods
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode texture2DGetAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept = 0;

	virtual ZgErrorCode textureHeapCreate(
		ZgTextureHeap** textureHeapOut,
		const ZgTextureHeapCreateInfo& createInfo) noexcept = 0;

	virtual ZgErrorCode textureHeapRelease(
		ZgTextureHeap* textureHeap) noexcept = 0;

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode getPresentQueue(ZgCommandQueue** presentQueueOut) noexcept = 0;
	virtual ZgErrorCode getCopyQueue(ZgCommandQueue** copyQueueOut) noexcept = 0;
};

// PipelineRendering
// ------------------------------------------------------------------------------------------------

struct ZgPipelineRendering {
	virtual ~ZgPipelineRendering() noexcept {}
};

// Memory
// ------------------------------------------------------------------------------------------------

struct ZgMemoryHeap {
	virtual ~ZgMemoryHeap() noexcept {}

	virtual ZgErrorCode bufferCreate(
		ZgBuffer** bufferOut,
		const ZgBufferCreateInfo& createInfo) noexcept = 0;
};

struct ZgBuffer {
	virtual ~ZgBuffer() noexcept {}

	virtual ZgErrorCode setDebugName(
		const char* name) noexcept = 0;
};

// Textures
// ------------------------------------------------------------------------------------------------

struct ZgTextureHeap {
	virtual ~ZgTextureHeap() noexcept {}

	virtual ZgErrorCode texture2DCreate(
		ZgTexture2D** textureOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept = 0;
};

struct ZgTexture2D {
	virtual ~ZgTexture2D() noexcept {}

	virtual ZgErrorCode setDebugName(
		const char* name) noexcept = 0;
};

// Framebuffer
// ------------------------------------------------------------------------------------------------

struct ZgFramebuffer {
	virtual ~ZgFramebuffer() noexcept {}
};

// Fence
// ------------------------------------------------------------------------------------------------

struct ZgFence {
	virtual ~ZgFence() noexcept {}

	virtual ZgErrorCode reset() noexcept = 0;
	virtual ZgErrorCode checkIfSignaled(bool& fenceSignaledOut) const noexcept = 0;
	virtual ZgErrorCode waitOnCpuBlocking() const noexcept = 0;
};

// Command queue
// ------------------------------------------------------------------------------------------------

struct ZgCommandQueue {
	virtual ~ZgCommandQueue() noexcept {}

	virtual ZgErrorCode signalOnGpu(ZgFence& fenceToSignal) noexcept = 0;
	virtual ZgErrorCode waitOnGpu(const ZgFence& fence) noexcept = 0;
	virtual ZgErrorCode flush() noexcept = 0;
	virtual ZgErrorCode beginCommandListRecording(ZgCommandList** commandListOut) noexcept = 0;
	virtual ZgErrorCode executeCommandList(ZgCommandList* commandList) noexcept = 0;
};

// Command lists
// ------------------------------------------------------------------------------------------------

struct ZgCommandList {
	virtual ~ZgCommandList() noexcept {};

	virtual ZgErrorCode memcpyBufferToBuffer(
		ZgBuffer* dstBuffer,
		uint64_t dstBufferOffsetBytes,
		ZgBuffer* srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept = 0;

	virtual ZgErrorCode memcpyToTexture(
		ZgTexture2D* dstTexture,
		uint32_t dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		ZgBuffer* tempUploadBuffer) noexcept = 0;

	virtual ZgErrorCode enableQueueTransitionBuffer(ZgBuffer* buffer) noexcept = 0;

	virtual ZgErrorCode enableQueueTransitionTexture(ZgTexture2D* texture) noexcept = 0;

	virtual ZgErrorCode setPushConstant(
		uint32_t shaderRegister,
		const void* data,
		uint32_t dataSizeInBytes) noexcept = 0;

	virtual ZgErrorCode setPipelineBindings(
		const ZgPipelineBindings& bindings) noexcept = 0;

	virtual ZgErrorCode setPipelineRendering(
		ZgPipelineRendering* pipeline) noexcept = 0;

	virtual ZgErrorCode setFramebuffer(
		ZgFramebuffer* framebuffer,
		const ZgFramebufferRect* optionalViewport,
		const ZgFramebufferRect* optionalScissor) noexcept = 0;

	virtual ZgErrorCode setFramebufferViewport(
		const ZgFramebufferRect& viewport) noexcept = 0;

	virtual ZgErrorCode setFramebufferScissor(
		const ZgFramebufferRect& scissor) noexcept = 0;

	virtual ZgErrorCode clearFramebuffer(
		float red,
		float green,
		float blue,
		float alpha) noexcept = 0;

	virtual ZgErrorCode clearDepthBuffer(
		float depth) noexcept = 0;

	virtual ZgErrorCode setIndexBuffer(
		ZgBuffer* indexBuffer,
		ZgIndexBufferType type) noexcept = 0;

	virtual ZgErrorCode setVertexBuffer(
		uint32_t vertexBufferSlot,
		ZgBuffer* vertexBuffer) noexcept = 0;

	virtual ZgErrorCode drawTriangles(
		uint32_t startVertexIndex,
		uint32_t numVertices) noexcept = 0;

	virtual ZgErrorCode drawTrianglesIndexed(
		uint32_t startIndex,
		uint32_t numTriangles) noexcept = 0;
};
