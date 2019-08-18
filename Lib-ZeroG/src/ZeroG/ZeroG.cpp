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

#define ZG_DLL_EXPORT
#include "ZeroG.h"

#include "ZeroG/BackendInterface.hpp"
#include "ZeroG/Context.hpp"
#include "ZeroG/util/CpuAllocation.hpp"
#include "ZeroG/util/Logging.hpp"

#ifdef _WIN32
#include "ZeroG/d3d12/D3D12Backend.hpp"
#endif

#ifdef ZG_VULKAN
#include "ZeroG/vulkan/VulkanBackend.hpp"
#endif

// Version information
// ------------------------------------------------------------------------------------------------

ZG_API uint32_t zgApiLinkedVersion(void)
{
	return ZG_COMPILED_API_VERSION;
}

// Compiled features
// ------------------------------------------------------------------------------------------------

ZG_API ZgFeatureBits zgCompiledFeatures(void)
{
	return 0
#ifdef _WIN32
		| uint64_t(ZG_FEATURE_BIT_BACKEND_D3D12)
#endif
#ifdef ZG_VULKAN
		| uint64_t(ZG_FEATURE_BIT_BACKEND_VULKAN)
#endif
	;
}

// Error codes
// ------------------------------------------------------------------------------------------------

ZG_API const char* zgErrorCodeToString(ZgErrorCode errorCode)
{
	switch (errorCode) {
	case ZG_SUCCESS: return "ZG_SUCCESS";
	case ZG_WARNING_GENERIC: return "ZG_WARNING_GENERIC";
	case ZG_ERROR_GENERIC: return "ZG_ERROR_GENERIC";
	case ZG_ERROR_UNIMPLEMENTED: return "ZG_ERROR_UNIMPLEMENTED";
	case ZG_ERROR_ALREADY_INITIALIZED: return "ZG_ERROR_ALREADY_INITIALIZED";
	case ZG_ERROR_CPU_OUT_OF_MEMORY: return "ZG_ERROR_CPU_OUT_OF_MEMORY";
	case ZG_ERROR_GPU_OUT_OF_MEMORY: return "ZG_ERROR_GPU_OUT_OF_MEMORY";
	case ZG_ERROR_NO_SUITABLE_DEVICE: return "ZG_ERROR_NO_SUITABLE_DEVICE";
	case ZG_ERROR_INVALID_ARGUMENT: return "ZG_ERROR_INVALID_ARGUMENT";
	case ZG_ERROR_SHADER_COMPILE_ERROR: return "ZG_ERROR_SHADER_COMPILE_ERROR";
	case ZG_ERROR_OUT_OF_COMMAND_LISTS: return "ZG_ERROR_OUT_OF_COMMAND_LISTS";
	case ZG_ERROR_INVALID_COMMAND_LIST_STATE: return "ZG_ERROR_INVALID_COMMAND_LIST_STATE";
	}
	return "<UNKNOWN ERROR CODE>";
}

// Context
// ------------------------------------------------------------------------------------------------

ZG_API ZgBool zgContextAlreadyInitialized(void)
{
	return zg::getBackend() == nullptr ? ZG_FALSE : ZG_TRUE;
}

ZG_API ZgErrorCode zgContextInit(const ZgContextInitSettings* initSettings)
{
	if (initSettings == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (zgContextAlreadyInitialized() == ZG_TRUE) return ZG_ERROR_ALREADY_INITIALIZED;

	ZgContextInitSettings settings = *initSettings;

	// Set default logger if none is specified
	bool usingDefaultLogger = settings.logger.log == nullptr;
	ZgLogger logger;
	if (usingDefaultLogger) logger = zg::getDefaultLogger();
	else logger = settings.logger;

	// Set default allocator if none is specified
	bool usingDefaultAllocator =
		settings.allocator.allocate == nullptr || settings.allocator.deallocate == nullptr;
	ZgAllocator allocator;
	if (usingDefaultAllocator) allocator = zg::getDefaultAllocator();
	else allocator = settings.allocator;

	// Set temporary context with logger and allocator. Required so rest of initialization can
	// allocate memory and log.
	ZgContext tmpContext = {};
	tmpContext.allocator = allocator;
	tmpContext.logger = logger;
	zg::setContext(tmpContext);

	// Log which logger is used
	if (usingDefaultLogger) ZG_INFO("zgContextInit(): Using default logger (printf)");
	else ZG_INFO("zgContextInit(): Using user-provided logger");

	// Log which allocator is used
	if (usingDefaultAllocator) ZG_INFO("zgContextInit(): Using default allocator");
	else ZG_INFO("zgContextInit(): Using user-provided allocator");

	// Create and allocate requested backend api
	switch (initSettings->backend) {

	case ZG_BACKEND_NONE:
		// TODO: Implement null backend
		ZG_ERROR("zgContextInit(): Null backend not implemented, exiting.");
		return ZG_ERROR_UNIMPLEMENTED;

#ifdef _WIN32
	case ZG_BACKEND_D3D12:
		{
			ZgErrorCode res = zg::createD3D12Backend(&tmpContext.backend, settings);
			if (res != ZG_SUCCESS) {
				ZG_ERROR("zgContextInit(): Could not create D3D12 backend, exiting.");
				return res;
			}
			ZG_INFO("zgContextInit(): Created D3D12 backend");
		}
		break;
#endif

#ifdef ZG_VULKAN
	case ZG_BACKEND_VULKAN:
		{
			ZgErrorCode res = zg::createVulkanBackend(&tmpContext.backend, settings);
			if (res != ZG_SUCCESS) {
				ZG_ERROR("zgContextInit(): Could not create Vulkan backend, exiting.");
				return res;
			}
			ZG_INFO"zgContextInit(): Created Vulkan backend");
		}
		break;
#endif

	default:
		return ZG_ERROR_GENERIC;
	}

	// Set context
	zg::setContext(tmpContext);
	return ZG_SUCCESS;
}

ZG_API ZgErrorCode zgContextDeinit(void)
{
	if (zgContextAlreadyInitialized() == ZG_FALSE) return ZG_SUCCESS;

	ZgContext& ctx = zg::getContext();

	// Delete backend
	zg::zgDelete(ctx.backend);

	// Reset context
	ctx = {};

	return ZG_SUCCESS;
}

ZG_API ZgErrorCode zgContextSwapchainResize(
	uint32_t width,
	uint32_t height)
{
	return zg::getBackend()->swapchainResize(width, height);
}

ZG_API ZgErrorCode zgContextSwapchainBeginFrame(
	ZgFramebuffer** framebufferOut)
{
	return zg::getBackend()->swapchainBeginFrame(framebufferOut);
}

ZG_API ZgErrorCode zgContextSwapchainFinishFrame(void)
{
	return zg::getBackend()->swapchainFinishFrame();
}

// Statistics
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgContextGetStats(ZgStats* statsOut)
{
	if (statsOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	return zg::getBackend()->getStats(*statsOut);
}

// Pipeline Rendering - Common
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgPipelineRenderingRelease(
	ZgPipelineRendering* pipeline)
{
	return zg::getBackend()->pipelineRenderingRelease(pipeline);
}

ZG_API ZgErrorCode zgPipelineRenderingGetSignature(
	const ZgPipelineRendering* pipeline,
	ZgPipelineRenderingSignature* signatureOut)
{
	return zg::getBackend()->pipelineRenderingGetSignature(pipeline, signatureOut);
}

// Pipeline Rendering - SPIRV
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgPipelineRenderingCreateFromFileSPIRV(
	ZgPipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfoFileSPIRV* createInfo)
{
	// Check arguments
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (pipelineOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (signatureOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->vertexShaderPath == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.vertexShaderEntry == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->pixelShaderPath == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.pixelShaderEntry == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexAttributes == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexBufferSlots == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS) return ZG_ERROR_INVALID_ARGUMENT;

	return zg::getBackend()->pipelineRenderingCreateFromFileSPIRV(
		pipelineOut, signatureOut, *createInfo);
}

// Pipeline Rendering - HLSL
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgPipelineRenderingCreateFromFileHLSL(
	ZgPipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfoFileHLSL* createInfo)
{
	// Check arguments
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (pipelineOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (signatureOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->vertexShaderPath == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.vertexShaderEntry == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->pixelShaderPath == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.pixelShaderEntry == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->shaderModel == ZG_SHADER_MODEL_UNDEFINED) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexAttributes == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexBufferSlots == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS) return ZG_ERROR_INVALID_ARGUMENT;

	return zg::getBackend()->pipelineRenderingCreateFromFileHLSL(
		pipelineOut, signatureOut, *createInfo);
}

ZG_API ZgErrorCode zgPipelineRenderingCreateFromSourceHLSL(
	ZgPipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfoSourceHLSL* createInfo)
{
	// Check arguments
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (pipelineOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (signatureOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->vertexShaderSrc == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.vertexShaderEntry == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->pixelShaderSrc == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.pixelShaderEntry == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->shaderModel == ZG_SHADER_MODEL_UNDEFINED) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexAttributes == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexBufferSlots == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->common.numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS) return ZG_ERROR_INVALID_ARGUMENT;

	return zg::getBackend()->pipelineRenderingCreateFromSourceHLSL(
		pipelineOut, signatureOut, *createInfo);
}

// Memory
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgMemoryHeapCreate(
	ZgMemoryHeap** memoryHeapOut,
	const ZgMemoryHeapCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->sizeInBytes == 0) return ZG_ERROR_INVALID_ARGUMENT;

	return zg::getBackend()->memoryHeapCreate(memoryHeapOut, *createInfo);
}

ZG_API ZgErrorCode zgMemoryHeapRelease(
	ZgMemoryHeap* memoryHeap)
{
	return zg::getBackend()->memoryHeapRelease(memoryHeap);
}

ZG_API ZgErrorCode zgMemoryHeapBufferCreate(
	ZgMemoryHeap* memoryHeap,
	ZgBuffer** bufferOut,
	const ZgBufferCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if ((createInfo->offsetInBytes % 65536) != 0) return ZG_ERROR_INVALID_ARGUMENT; // 64KiB alignment

	return memoryHeap->bufferCreate(bufferOut, *createInfo);
}

ZG_API void zgBufferRelease(
	ZgBuffer* buffer)
{
	if (buffer == nullptr) return;
	zg::zgDelete(buffer);
}

ZG_API ZgErrorCode zgBufferMemcpyTo(
	ZgBuffer* dstBuffer,
	uint64_t bufferOffsetBytes,
	const void* srcMemory,
	uint64_t numBytes)
{
	return zg::getBackend()->bufferMemcpyTo(
		dstBuffer,
		bufferOffsetBytes,
		reinterpret_cast<const uint8_t*>(srcMemory),
		numBytes);
}

ZG_API ZgErrorCode zgBufferSetDebugName(
	ZgBuffer* buffer,
	const char* name)
{
	if (buffer == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	return buffer->setDebugName(name);
}

// Textures
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgTextureHeapCreate(
	ZgTextureHeap** textureHeapOut,
	const ZgTextureHeapCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->sizeInBytes == 0) return ZG_ERROR_INVALID_ARGUMENT;

	return zg::getBackend()->textureHeapCreate(textureHeapOut, *createInfo);
}

ZG_API ZgErrorCode zgTextureHeapRelease(
	ZgTextureHeap* textureHeap)
{
	return zg::getBackend()->textureHeapRelease(textureHeap);
}

ZG_API ZgErrorCode zgTexture2DGetAllocationInfo(
	ZgTexture2DAllocationInfo* allocationInfoOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	if (allocationInfoOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numMipmaps == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numMipmaps > ZG_TEXTURE_2D_MAX_NUM_MIPMAPS) return ZG_ERROR_INVALID_ARGUMENT;
	return zg::getBackend()->texture2DGetAllocationInfo(*allocationInfoOut, *createInfo);
}

ZG_API ZgErrorCode zgTextureHeapTexture2DCreate(
	ZgTextureHeap* textureHeap,
	ZgTexture2D** textureOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numMipmaps == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numMipmaps > ZG_TEXTURE_2D_MAX_NUM_MIPMAPS) return ZG_ERROR_INVALID_ARGUMENT;
	//if ((createInfo->offsetInBytes % 65536) != 0) return ZG_ERROR_INVALID_ARGUMENT; // 64KiB alignment

	return textureHeap->texture2DCreate(textureOut, *createInfo);
}

ZG_API void zgTexture2DRelease(
	ZgTexture2D* texture)
{
	if (texture == nullptr) return;
	zg::zgDelete(texture);
}

ZG_API ZgErrorCode zgTexture2DSetDebugName(
	ZgTexture2D* texture,
	const char* name)
{
	if (texture == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	return texture->setDebugName(name);
}

// Fence
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgFenceCreate(
	ZgFence** fenceOut)
{
	return zg::getBackend()->fenceCreate(fenceOut);
}

ZG_API void zgFenceRelease(
	ZgFence* fence)
{
	if (fence == nullptr) return;
	zg::zgDelete(fence);
}

ZG_API ZgErrorCode zgFenceReset(
	ZgFence* fence)
{
	return fence->reset();
}

ZG_API ZgErrorCode zgFenceCheckIfSignaled(
	const ZgFence* fence,
	ZgBool* fenceSignaledOut)
{
	bool fenceSignaled = false;
	ZgErrorCode res = fence->checkIfSignaled(fenceSignaled);
	*fenceSignaledOut = fenceSignaled ? ZG_TRUE : ZG_FALSE;
	return res;
}

ZG_API ZgErrorCode zgFenceWaitOnCpuBlocking(
	const ZgFence* fence)
{
	return fence->waitOnCpuBlocking();
}

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgCommandQueueGetPresentQueue(
	ZgCommandQueue** presentQueueOut)
{
	return zg::getBackend()->getPresentQueue(presentQueueOut);
}

ZG_API ZgErrorCode zgCommandQueueGetCopyQueue(
	ZgCommandQueue** copyQueueOut)
{
	return zg::getBackend()->getCopyQueue(copyQueueOut);
}

ZG_API ZgErrorCode zgCommandQueueSignalOnGpu(
	ZgCommandQueue* commandQueue,
	ZgFence* fenceToSignal)
{
	return commandQueue->signalOnGpu(*fenceToSignal);
}

ZG_API ZgErrorCode zgCommandQueueWaitOnGpu(
	ZgCommandQueue* commandQueue,
	const ZgFence* fence)
{
	return commandQueue->waitOnGpu(*fence);
}

ZG_API ZgErrorCode zgCommandQueueFlush(
	ZgCommandQueue* commandQueue)
{
	return commandQueue->flush();
}

ZG_API ZgErrorCode zgCommandQueueBeginCommandListRecording(
	ZgCommandQueue* commandQueue,
	ZgCommandList** commandListOut)
{
	return commandQueue->beginCommandListRecording(commandListOut);
}

ZG_API ZgErrorCode zgCommandQueueExecuteCommandList(
	ZgCommandQueue* commandQueue,
	ZgCommandList* commandList)
{
	return commandQueue->executeCommandList(commandList);
}

// Command list
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgCommandListMemcpyBufferToBuffer(
	ZgCommandList* commandList,
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes)
{
	if (numBytes == 0) return ZG_ERROR_INVALID_ARGUMENT;
	return commandList->memcpyBufferToBuffer(
		dstBuffer,
		dstBufferOffsetBytes,
		srcBuffer,
		srcBufferOffsetBytes,
		numBytes);
}

ZG_API ZgErrorCode zgCommandListMemcpyToTexture(
	ZgCommandList* commandList,
	ZgTexture2D* dstTexture,
	uint32_t dstTextureMipLevel,
	const ZgImageViewConstCpu* srcImageCpu,
	ZgBuffer* tempUploadBuffer)
{
	if (srcImageCpu->data == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (srcImageCpu->width == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (srcImageCpu->height == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (srcImageCpu->pitchInBytes < srcImageCpu->width) return ZG_ERROR_INVALID_ARGUMENT;
	if (dstTextureMipLevel >= ZG_TEXTURE_2D_MAX_NUM_MIPMAPS) return ZG_ERROR_INVALID_ARGUMENT;
	return commandList->memcpyToTexture(
		dstTexture,
		dstTextureMipLevel,
		*srcImageCpu,
		tempUploadBuffer);
}

ZG_API ZgErrorCode zgCommandListEnableQueueTransitionBuffer(
	ZgCommandList* commandList,
	ZgBuffer* buffer)
{
	return commandList->enableQueueTransitionBuffer(buffer);
}

ZG_API ZgErrorCode zgCommandListEnableQueueTransitionTexture(
	ZgCommandList* commandList,
	ZgTexture2D* texture)
{
	return commandList->enableQueueTransitionTexture(texture);
}

ZG_API ZgErrorCode zgCommandListSetPushConstant(
	ZgCommandList* commandList,
	uint32_t shaderRegister,
	const void* data,
	uint32_t dataSizeInBytes)
{
	if (data == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	return commandList->setPushConstant(shaderRegister, data, dataSizeInBytes);
}

ZG_API ZgErrorCode zgCommandListSetPipelineBindings(
	ZgCommandList* commandList,
	const ZgPipelineBindings* bindings)
{
	return commandList->setPipelineBindings(*bindings);
}

ZG_API ZgErrorCode zgCommandListSetPipelineRendering(
	ZgCommandList* commandList,
	ZgPipelineRendering* pipeline)
{
	return commandList->setPipelineRendering(pipeline);
}

ZG_API ZgErrorCode zgCommandListSetFramebuffer(
	ZgCommandList* commandList,
	ZgFramebuffer* framebuffer,
	const ZgFramebufferRect* optionalViewport,
	const ZgFramebufferRect* optionalScissor)
{
	return commandList->setFramebuffer(framebuffer, optionalViewport, optionalScissor);
}

ZG_API ZgErrorCode zgCommandListSetFramebufferViewport(
	ZgCommandList* commandList,
	const ZgFramebufferRect* viewport)
{
	return commandList->setFramebufferViewport(*viewport);
}

ZG_API ZgErrorCode zgCommandListSetFramebufferScissor(
	ZgCommandList* commandList,
	const ZgFramebufferRect* scissor)
{
	return commandList->setFramebufferScissor(*scissor);
}

ZG_API ZgErrorCode zgCommandListClearFramebuffer(
	ZgCommandList* commandList,
	float red,
	float green,
	float blue,
	float alpha)
{
	return commandList->clearFramebuffer(red, green, blue, alpha);
}

ZG_API ZgErrorCode zgCommandListClearDepthBuffer(
	ZgCommandList* commandList,
	float depth)
{
	return commandList->clearDepthBuffer(depth);
}

ZG_API ZgErrorCode zgCommandListSetIndexBuffer(
	ZgCommandList* commandList,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type)
{
	return commandList->setIndexBuffer(indexBuffer, type);
}

ZG_API ZgErrorCode zgCommandListSetVertexBuffer(
	ZgCommandList* commandList,
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBuffer)
{
	return commandList->setVertexBuffer(
		vertexBufferSlot, vertexBuffer);
}

ZG_API ZgErrorCode zgCommandListDrawTriangles(
	ZgCommandList* commandList,
	uint32_t startVertexIndex,
	uint32_t numVertices)
{
	if ((numVertices % 3) != 0) return ZG_ERROR_INVALID_ARGUMENT;
	return commandList->drawTriangles(startVertexIndex, numVertices);
}

ZG_API ZgErrorCode zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandList,
	uint32_t startIndex,
	uint32_t numTriangles)
{
	return commandList->drawTrianglesIndexed(startIndex, numTriangles);
}
