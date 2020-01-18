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
#include "ZeroG/util/ErrorReporting.hpp"
#include "ZeroG/util/Logging.hpp"

#if defined(_WIN32)
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
#if defined(_WIN32)
		| uint64_t(ZG_FEATURE_BIT_BACKEND_D3D12)
#endif
#ifdef ZG_VULKAN
		| uint64_t(ZG_FEATURE_BIT_BACKEND_VULKAN)
#endif
	;
}

// Results
// ------------------------------------------------------------------------------------------------

ZG_API const char* zgResultToString(ZgResult result)
{
	switch (result) {
	case ZG_SUCCESS: return "ZG_SUCCESS";

	case ZG_WARNING_GENERIC: return "ZG_WARNING_GENERIC";
	case ZG_WARNING_UNIMPLEMENTED: return "ZG_WARNING_UNIMPLEMENTED";
	case ZG_WARNING_ALREADY_INITIALIZED: return "ZG_WARNING_ALREADY_INITIALIZED";

	case ZG_ERROR_GENERIC: return "ZG_ERROR_GENERIC";
	case ZG_ERROR_CPU_OUT_OF_MEMORY: return "ZG_ERROR_CPU_OUT_OF_MEMORY";
	case ZG_ERROR_GPU_OUT_OF_MEMORY: return "ZG_ERROR_GPU_OUT_OF_MEMORY";
	case ZG_ERROR_NO_SUITABLE_DEVICE: return "ZG_ERROR_NO_SUITABLE_DEVICE";
	case ZG_ERROR_INVALID_ARGUMENT: return "ZG_ERROR_INVALID_ARGUMENT";
	case ZG_ERROR_SHADER_COMPILE_ERROR: return "ZG_ERROR_SHADER_COMPILE_ERROR";
	case ZG_ERROR_OUT_OF_COMMAND_LISTS: return "ZG_ERROR_OUT_OF_COMMAND_LISTS";
	case ZG_ERROR_INVALID_COMMAND_LIST_STATE: return "ZG_ERROR_INVALID_COMMAND_LIST_STATE";
	}
	return "<UNKNOWN RESULT>";
}

// Context
// ------------------------------------------------------------------------------------------------

ZG_API ZgBool zgContextAlreadyInitialized(void)
{
	return zg::getBackend() == nullptr ? ZG_FALSE : ZG_TRUE;
}

ZG_API ZgResult zgContextInit(const ZgContextInitSettings* settings)
{
	// Can't use ZG_ARG_CHECK() here because logger is not yet initialized
	if (settings == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (zgContextAlreadyInitialized() == ZG_TRUE) return ZG_WARNING_ALREADY_INITIALIZED;

	ZgContext tmpContext;

	// Set default logger if none is specified
	bool usingDefaultLogger = settings->logger.log == nullptr;
	if (usingDefaultLogger) tmpContext.logger = zg::getDefaultLogger();
	else tmpContext.logger = settings->logger;

	// Set default allocator if none is specified
	bool usingDefaultAllocator =
		settings->allocator.allocate == nullptr || settings->allocator.deallocate == nullptr;
	if (usingDefaultAllocator) tmpContext.allocator = AllocatorWrapper::createDefaultAllocator();
	else tmpContext.allocator = AllocatorWrapper::createWrapper(settings->allocator);

	// Set temporary context (without API backend). Required so rest of initialization can allocate
	// memory and log.
	zg::setContext(tmpContext);

	// Log which logger is used
	if (usingDefaultLogger) ZG_INFO("zgContextInit(): Using default logger (printf)");
	else ZG_INFO("zgContextInit(): Using user-provided logger");

	// Log which allocator is used
	if (usingDefaultAllocator) ZG_INFO("zgContextInit(): Using default allocator");
	else ZG_INFO("zgContextInit(): Using user-provided allocator");

	// Create and allocate requested backend api
	switch (settings->backend) {

	case ZG_BACKEND_NONE:
		// TODO: Implement null backend
		ZG_ERROR("zgContextInit(): Null backend not implemented, exiting.");
		return ZG_WARNING_UNIMPLEMENTED;

#if defined(_WIN32)
	case ZG_BACKEND_D3D12:
		{
			ZG_INFO("zgContextInit(): Attempting to create D3D12 backend...");
			ZgResult res = zg::createD3D12Backend(&tmpContext.backend, *settings);
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
			ZG_INFO("zgContextInit(): Attempting to create Vulkan backend...");
			ZgResult res = zg::createVulkanBackend(&tmpContext.backend, *settings);
			if (res != ZG_SUCCESS) {
				ZG_ERROR("zgContextInit(): Could not create Vulkan backend, exiting.");
				return res;
			}
			ZG_INFO("zgContextInit(): Created Vulkan backend");
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

ZG_API ZgResult zgContextDeinit(void)
{
	if (zgContextAlreadyInitialized() == ZG_FALSE) return ZG_SUCCESS;

	ZgContext& ctx = zg::getContext();

	// Delete backend
	zg::getAllocator()->deleteObject(ctx.backend);

	// Reset context
	ctx = {};
	ctx.allocator = AllocatorWrapper();

	return ZG_SUCCESS;
}

ZG_API ZgResult zgContextSwapchainResize(
	uint32_t width,
	uint32_t height)
{
	return zg::getBackend()->swapchainResize(width, height);
}

ZG_API ZgResult zgContextSwapchainBeginFrame(
	ZgFramebuffer** framebufferOut)
{
	return zg::getBackend()->swapchainBeginFrame(framebufferOut);
}

ZG_API ZgResult zgContextSwapchainFinishFrame(void)
{
	return zg::getBackend()->swapchainFinishFrame();
}

// Statistics
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgContextGetStats(ZgStats* statsOut)
{
	ZG_ARG_CHECK(statsOut == nullptr, "");
	return zg::getBackend()->getStats(*statsOut);
}

// Pipeline Compute
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgPipelineComputeCreateFromFileHLSL(
	ZgPipelineCompute** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	const ZgPipelineComputeCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings)
{
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(bindingsSignatureOut == nullptr, "");
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(compileSettings == nullptr, "");
	
	return zg::getBackend()->pipelineComputeCreateFromFileHLSL(
		pipelineOut, bindingsSignatureOut, *createInfo, *compileSettings);
}

ZG_API ZgResult zgPipelineComputeRelease(
	ZgPipelineCompute* pipeline)
{
	return zg::getBackend()->pipelineComputeRelease(pipeline);
}

// Pipeline Render
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgPipelineRenderCreateFromFileSPIRV(
	ZgPipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo* createInfo)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(bindingsSignatureOut == nullptr, "");
	ZG_ARG_CHECK(renderSignatureOut == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShaderEntry == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShaderEntry == nullptr, "");
	ZG_ARG_CHECK(createInfo->numVertexAttributes == 0, "Must specify at least one vertex attribute");
	ZG_ARG_CHECK(createInfo->numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex attributes specified");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots == 0, "Must specify at least one vertex buffer");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex buffers specified");
	ZG_ARG_CHECK(createInfo->numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS, "Too many push constants specified");

	return zg::getBackend()->pipelineRenderCreateFromFileSPIRV(
		pipelineOut, bindingsSignatureOut, renderSignatureOut, *createInfo);
}

ZG_API ZgResult zgPipelineRenderCreateFromFileHLSL(
	ZgPipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(compileSettings == nullptr, "");
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(bindingsSignatureOut == nullptr, "");
	ZG_ARG_CHECK(renderSignatureOut == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShaderEntry == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShaderEntry == nullptr, "");
	ZG_ARG_CHECK(compileSettings->shaderModel == ZG_SHADER_MODEL_UNDEFINED, "Must specify shader model");
	ZG_ARG_CHECK(createInfo->numVertexAttributes == 0, "Must specify at least one vertex attribute");
	ZG_ARG_CHECK(createInfo->numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex attributes specified");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots == 0, "Must specify at least one vertex buffer");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex buffers specified");
	ZG_ARG_CHECK(createInfo->numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS, "Too many push constants specified");

	return zg::getBackend()->pipelineRenderCreateFromFileHLSL(
		pipelineOut, bindingsSignatureOut, renderSignatureOut, *createInfo, *compileSettings);
}

ZG_API ZgResult zgPipelineRenderCreateFromSourceHLSL(
	ZgPipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(compileSettings == nullptr, "");
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(bindingsSignatureOut == nullptr, "");
	ZG_ARG_CHECK(renderSignatureOut == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShaderEntry == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShaderEntry == nullptr, "");
	ZG_ARG_CHECK(compileSettings->shaderModel == ZG_SHADER_MODEL_UNDEFINED, "Must specify shader model");
	ZG_ARG_CHECK(createInfo->numVertexAttributes == 0, "Must specify at least one vertex attribute");
	ZG_ARG_CHECK(createInfo->numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex attributes specified");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots == 0, "Must specify at least one vertex buffer");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex buffers specified");
	ZG_ARG_CHECK(createInfo->numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS, "Too many push constants specified");

	return zg::getBackend()->pipelineRenderCreateFromSourceHLSL(
		pipelineOut, bindingsSignatureOut, renderSignatureOut, *createInfo, *compileSettings);
}

ZG_API ZgResult zgPipelineRenderRelease(
	ZgPipelineRender* pipeline)
{
	return zg::getBackend()->pipelineRenderRelease(pipeline);
}

// Memory Heap
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgMemoryHeapCreate(
	ZgMemoryHeap** memoryHeapOut,
	const ZgMemoryHeapCreateInfo* createInfo)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(createInfo->sizeInBytes == 0, "Can't create an empty memory heap");

	return zg::getBackend()->memoryHeapCreate(memoryHeapOut, *createInfo);
}

ZG_API ZgResult zgMemoryHeapRelease(
	ZgMemoryHeap* memoryHeap)
{
	return zg::getBackend()->memoryHeapRelease(memoryHeap);
}

// Buffer
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgMemoryHeapBufferCreate(
	ZgMemoryHeap* memoryHeap,
	ZgBuffer** bufferOut,
	const ZgBufferCreateInfo* createInfo)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK((createInfo->offsetInBytes % 65536) != 0, "Buffer must be 64KiB aligned");

	return memoryHeap->bufferCreate(bufferOut, *createInfo);
}

ZG_API void zgBufferRelease(
	ZgBuffer* buffer)
{
	if (buffer == nullptr) return;
	zg::getAllocator()->deleteObject(buffer);
}

ZG_API ZgResult zgBufferMemcpyTo(
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	const void* srcMemory,
	uint64_t numBytes)
{
	return zg::getBackend()->bufferMemcpyTo(
		dstBuffer,
		dstBufferOffsetBytes,
		reinterpret_cast<const uint8_t*>(srcMemory),
		numBytes);
}

ZG_API ZgResult zgBufferMemcpyFrom(
	void* dstMemory,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes)
{
	return zg::getBackend()->bufferMemcpyFrom(
		reinterpret_cast<uint8_t*>(dstMemory),
		srcBuffer,
		srcBufferOffsetBytes,
		numBytes);
}

ZG_API ZgResult zgBufferSetDebugName(
	ZgBuffer* buffer,
	const char* name)
{
	ZG_ARG_CHECK(buffer == nullptr, "");
	return buffer->setDebugName(name);
}

// Textures
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgTexture2DGetAllocationInfo(
	ZgTexture2DAllocationInfo* allocationInfoOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	ZG_ARG_CHECK(allocationInfoOut == nullptr, "");
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(createInfo->numMipmaps == 0, "Must specify at least 1 mipmap layer (i.e. the full image)");
	ZG_ARG_CHECK(createInfo->numMipmaps > ZG_MAX_NUM_MIPMAPS, "Too many mipmaps specified");
	return zg::getBackend()->texture2DGetAllocationInfo(*allocationInfoOut, *createInfo);
}

ZG_API ZgResult zgMemoryHeapTexture2DCreate(
	ZgMemoryHeap* memoryHeap,
	ZgTexture2D** textureOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(createInfo->numMipmaps == 0, "Must specify at least 1 mipmap layer (i.e. the full image)");
	ZG_ARG_CHECK(createInfo->numMipmaps > ZG_MAX_NUM_MIPMAPS, "Too many mipmaps specified");
	if (createInfo->usage == ZG_TEXTURE_USAGE_DEFAULT) {
		ZG_ARG_CHECK(createInfo->optimalClearValue != ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED,
			"May not define optimal clear value for default textures");
	}
	return memoryHeap->texture2DCreate(textureOut, *createInfo);
}

ZG_API void zgTexture2DRelease(
	ZgTexture2D* texture)
{
	if (texture == nullptr) return;
	zg::getAllocator()->deleteObject(texture);
}

ZG_API ZgResult zgTexture2DSetDebugName(
	ZgTexture2D* texture,
	const char* name)
{
	ZG_ARG_CHECK(texture == nullptr, "");
	ZG_ARG_CHECK(name == nullptr, "");
	return texture->setDebugName(name);
}

// Framebuffer
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgFramebufferCreate(
	ZgFramebuffer** framebufferOut,
	const ZgFramebufferCreateInfo* createInfo)
{
	ZG_ARG_CHECK(framebufferOut == nullptr, "");
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(createInfo->numRenderTargets > ZG_MAX_NUM_RENDER_TARGETS, "Too many render targets");
	return zg::getBackend()->framebufferCreate(framebufferOut, *createInfo);
}

ZG_API void zgFramebufferRelease(
	ZgFramebuffer* framebuffer)
{
	if (framebuffer == nullptr) return;
	// Done via backend so it can have a chance to check if framebuffer is built-in (i.e. swapchain
	// framebuffer) before it deallocates it.
	return zg::getBackend()->framebufferRelease(framebuffer);
}

ZG_API ZgResult zgFramebufferGetResolution(
	const ZgFramebuffer* framebuffer,
	uint32_t* widthOut,
	uint32_t* heightOut)
{
	return framebuffer->getResolution(*widthOut, *heightOut);
}

// Fence
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgFenceCreate(
	ZgFence** fenceOut)
{
	return zg::getBackend()->fenceCreate(fenceOut);
}

ZG_API void zgFenceRelease(
	ZgFence* fence)
{
	if (fence == nullptr) return;
	zg::getAllocator()->deleteObject(fence);
}

ZG_API ZgResult zgFenceReset(
	ZgFence* fence)
{
	return fence->reset();
}

ZG_API ZgResult zgFenceCheckIfSignaled(
	const ZgFence* fence,
	ZgBool* fenceSignaledOut)
{
	bool fenceSignaled = false;
	ZgResult res = fence->checkIfSignaled(fenceSignaled);
	*fenceSignaledOut = fenceSignaled ? ZG_TRUE : ZG_FALSE;
	return res;
}

ZG_API ZgResult zgFenceWaitOnCpuBlocking(
	const ZgFence* fence)
{
	return fence->waitOnCpuBlocking();
}

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgCommandQueueGetPresentQueue(
	ZgCommandQueue** presentQueueOut)
{
	return zg::getBackend()->getPresentQueue(presentQueueOut);
}

ZG_API ZgResult zgCommandQueueGetCopyQueue(
	ZgCommandQueue** copyQueueOut)
{
	return zg::getBackend()->getCopyQueue(copyQueueOut);
}

ZG_API ZgResult zgCommandQueueSignalOnGpu(
	ZgCommandQueue* commandQueue,
	ZgFence* fenceToSignal)
{
	return commandQueue->signalOnGpu(*fenceToSignal);
}

ZG_API ZgResult zgCommandQueueWaitOnGpu(
	ZgCommandQueue* commandQueue,
	const ZgFence* fence)
{
	return commandQueue->waitOnGpu(*fence);
}

ZG_API ZgResult zgCommandQueueFlush(
	ZgCommandQueue* commandQueue)
{
	return commandQueue->flush();
}

ZG_API ZgResult zgCommandQueueBeginCommandListRecording(
	ZgCommandQueue* commandQueue,
	ZgCommandList** commandListOut)
{
	return commandQueue->beginCommandListRecording(commandListOut);
}

ZG_API ZgResult zgCommandQueueExecuteCommandList(
	ZgCommandQueue* commandQueue,
	ZgCommandList* commandList)
{
	return commandQueue->executeCommandList(commandList);
}

// Command list
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgCommandListMemcpyBufferToBuffer(
	ZgCommandList* commandList,
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes)
{
	ZG_ARG_CHECK(numBytes == 0, "Can't copy zero bytes");
	return commandList->memcpyBufferToBuffer(
		dstBuffer,
		dstBufferOffsetBytes,
		srcBuffer,
		srcBufferOffsetBytes,
		numBytes);
}

ZG_API ZgResult zgCommandListMemcpyToTexture(
	ZgCommandList* commandList,
	ZgTexture2D* dstTexture,
	uint32_t dstTextureMipLevel,
	const ZgImageViewConstCpu* srcImageCpu,
	ZgBuffer* tempUploadBuffer)
{
	ZG_ARG_CHECK(srcImageCpu->data == nullptr, "");
	ZG_ARG_CHECK(srcImageCpu->width == 0, "");
	ZG_ARG_CHECK(srcImageCpu->height == 0, "");
	ZG_ARG_CHECK(srcImageCpu->pitchInBytes < srcImageCpu->width, "");
	ZG_ARG_CHECK(dstTextureMipLevel >= ZG_MAX_NUM_MIPMAPS, "Invalid target mip level");
	return commandList->memcpyToTexture(
		dstTexture,
		dstTextureMipLevel,
		*srcImageCpu,
		tempUploadBuffer);
}

ZG_API ZgResult zgCommandListEnableQueueTransitionBuffer(
	ZgCommandList* commandList,
	ZgBuffer* buffer)
{
	return commandList->enableQueueTransitionBuffer(buffer);
}

ZG_API ZgResult zgCommandListEnableQueueTransitionTexture(
	ZgCommandList* commandList,
	ZgTexture2D* texture)
{
	return commandList->enableQueueTransitionTexture(texture);
}

ZG_API ZgResult zgCommandListSetPushConstant(
	ZgCommandList* commandList,
	uint32_t shaderRegister,
	const void* data,
	uint32_t dataSizeInBytes)
{
	ZG_ARG_CHECK(data == nullptr, "");
	return commandList->setPushConstant(shaderRegister, data, dataSizeInBytes);
}

ZG_API ZgResult zgCommandListSetPipelineBindings(
	ZgCommandList* commandList,
	const ZgPipelineBindings* bindings)
{
	return commandList->setPipelineBindings(*bindings);
}

ZG_API ZgResult zgCommandListSetPipelineCompute(
	ZgCommandList* commandList,
	ZgPipelineCompute* pipeline)
{
	return commandList->setPipelineCompute(pipeline);
}

ZG_API ZgResult zgCommandListSetPipelineRender(
	ZgCommandList* commandList,
	ZgPipelineRender* pipeline)
{
	return commandList->setPipelineRender(pipeline);
}

ZG_API ZgResult zgCommandListSetFramebuffer(
	ZgCommandList* commandList,
	ZgFramebuffer* framebuffer,
	const ZgFramebufferRect* optionalViewport,
	const ZgFramebufferRect* optionalScissor)
{
	return commandList->setFramebuffer(framebuffer, optionalViewport, optionalScissor);
}

ZG_API ZgResult zgCommandListSetFramebufferViewport(
	ZgCommandList* commandList,
	const ZgFramebufferRect* viewport)
{
	return commandList->setFramebufferViewport(*viewport);
}

ZG_API ZgResult zgCommandListSetFramebufferScissor(
	ZgCommandList* commandList,
	const ZgFramebufferRect* scissor)
{
	return commandList->setFramebufferScissor(*scissor);
}

ZG_API ZgResult zgCommandListClearFramebufferOptimal(
	ZgCommandList* commandList)
{
	return commandList->clearFramebufferOptimal();
}

ZG_API ZgResult zgCommandListClearRenderTargets(
	ZgCommandList* commandList,
	float red,
	float green,
	float blue,
	float alpha)
{
	return commandList->clearRenderTargets(red, green, blue, alpha);
}

ZG_API ZgResult zgCommandListClearDepthBuffer(
	ZgCommandList* commandList,
	float depth)
{
	return commandList->clearDepthBuffer(depth);
}

ZG_API ZgResult zgCommandListSetIndexBuffer(
	ZgCommandList* commandList,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type)
{
	return commandList->setIndexBuffer(indexBuffer, type);
}

ZG_API ZgResult zgCommandListSetVertexBuffer(
	ZgCommandList* commandList,
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBuffer)
{
	return commandList->setVertexBuffer(
		vertexBufferSlot, vertexBuffer);
}

ZG_API ZgResult zgCommandListDispatchCompute(
	ZgCommandList* commandList,
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ)
{
	return commandList->dispatchCompute(groupCountX, groupCountY, groupCountZ);
}

ZG_API ZgResult zgCommandListDrawTriangles(
	ZgCommandList* commandList,
	uint32_t startVertexIndex,
	uint32_t numVertices)
{
	ZG_ARG_CHECK((numVertices % 3) != 0, "Odd number of vertices");
	return commandList->drawTriangles(startVertexIndex, numVertices);
}

ZG_API ZgResult zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandList,
	uint32_t startIndex,
	uint32_t numTriangles)
{
	return commandList->drawTrianglesIndexed(startIndex, numTriangles);
}
