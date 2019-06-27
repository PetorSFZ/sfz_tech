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
	return
		uint64_t(ZG_FEATURE_BIT_BACKEND_D3D12);
}

// Error codes
// ------------------------------------------------------------------------------------------------

ZG_API const char* zgErrorCodeToString(ZgErrorCode errorCode)
{
	switch (errorCode) {
	case ZG_SUCCESS: return "ZG_SUCCESS";
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
	return zg::getApiContext() == nullptr ? ZG_FALSE : ZG_TRUE;
}

ZG_API ZgErrorCode zgContextInit(const ZgContextInitSettings* initSettings)
{
	if (initSettings == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (zgContextAlreadyInitialized() == ZG_TRUE) return ZG_ERROR_ALREADY_INITIALIZED;

	ZgContextInitSettings settings = *initSettings;

	// Set default logger if none is specified
	bool usingDefaultLogger = settings.logger.log == nullptr;
	if (usingDefaultLogger) {
		settings.logger = zg::getDefaultLogger();
	}
	ZgLogger logger = settings.logger;
	if (usingDefaultLogger) {
		ZG_INFO(logger, "zgContextInit(): Using default logger (printf)");
	}
	else {
		ZG_INFO(logger, "zgContextInit(): Using user-provided logger");
	}

	// Set default allocator if none is specified
	if (settings.allocator.allocate == nullptr || settings.allocator.deallocate == nullptr) {
		settings.allocator = zg::getDefaultAllocator();
		ZG_INFO(logger, "zgContextInit(): Using default allocator");
	}
	else {
		ZG_INFO(logger, "zgContextInit(): Using user-provided allocator");
	}

	// Set context's allocator
	ZgContext tmpContext = {};
	tmpContext.allocator = settings.allocator;
	tmpContext.logger = settings.logger;

	// Set tmp context as current active context, needed so operator new and delete will work when
	// initializing the API backend.
	zg::setContext(tmpContext);

	// Create and allocate requested backend api
	switch (initSettings->backend) {

	case ZG_BACKEND_NONE:
		// TODO: Implement null backend
		ZG_ERROR(logger, "zgContextInit(): Null backend not implemented, exiting.");
		return ZG_ERROR_UNIMPLEMENTED;

	case ZG_BACKEND_D3D12:
		{
			ZgErrorCode res = zg::createD3D12Backend(&tmpContext.context, settings);
			if (res != ZG_SUCCESS) {
				ZG_ERROR(logger, "zgContextInit(): Could not create D3D12 backend, exiting.");
				return res;
			}
			ZG_INFO(logger, "zgContextInit(): Created D3D12 backend");
		}
		break;

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
	zg::zgDelete<zg::IContext>(ctx.allocator, ctx.context);

	// Reset context
	ctx = {};

	return ZG_SUCCESS;
}

ZG_API ZgErrorCode zgContextSwapchainResize( uint32_t width, uint32_t height)
{
	return zg::getApiContext()->swapchainResize(width, height);
}

ZG_API ZgErrorCode zgContextSwapchainCommandQueue(
	ZgCommandQueue** commandQueueOut)
{
	zg::ICommandQueue* commandQueue = nullptr;
	ZgErrorCode res = zg::getApiContext()->swapchainCommandQueue(&commandQueue);
	if (res != ZG_SUCCESS) return res;
	*commandQueueOut = reinterpret_cast<ZgCommandQueue*>(commandQueue);
	return ZG_SUCCESS;
}

ZG_API ZgErrorCode zgContextSwapchainBeginFrame(
	ZgFramebuffer** framebufferOut)
{
	zg::IFramebuffer* framebuffer = nullptr;
	ZgErrorCode res = zg::getApiContext()->swapchainBeginFrame(&framebuffer);
	if (res != ZG_SUCCESS) return res;
	*framebufferOut = reinterpret_cast<ZgFramebuffer*>(framebuffer);
	return ZG_SUCCESS;
}

ZG_API ZgErrorCode zgContextSwapchainFinishFrame(void)
{
	return zg::getApiContext()->swapchainFinishFrame();
}

// Pipeline Rendering - Common
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgPipelineRenderingRelease(
	ZgPipelineRendering* pipeline)
{
	return zg::getApiContext()->pipelineRenderingRelease(
		reinterpret_cast<zg::IPipelineRendering*>(pipeline));
}

ZG_API ZgErrorCode zgPipelineRenderingGetSignature(
	const ZgPipelineRendering* pipeline,
	ZgPipelineRenderingSignature* signatureOut)
{
	return zg::getApiContext()->pipelineRenderingGetSignature(
		reinterpret_cast<const zg::IPipelineRendering*>(pipeline), signatureOut);
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
	
	zg::IPipelineRendering* pipeline = nullptr;
	ZgErrorCode res = zg::getApiContext()->pipelineRenderingCreateFromFileSPIRV(
		&pipeline, signatureOut, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*pipelineOut = reinterpret_cast<ZgPipelineRendering*>(pipeline);
	return ZG_SUCCESS;
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

	zg::IPipelineRendering* pipeline = nullptr;
	ZgErrorCode res = zg::getApiContext()->pipelineRenderingCreateFromFileHLSL(
		&pipeline, signatureOut, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*pipelineOut = reinterpret_cast<ZgPipelineRendering*>(pipeline);
	return ZG_SUCCESS;
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

	zg::IPipelineRendering* pipeline = nullptr;
	ZgErrorCode res = zg::getApiContext()->pipelineRenderingCreateFromSourceHLSL(
		&pipeline, signatureOut, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*pipelineOut = reinterpret_cast<ZgPipelineRendering*>(pipeline);
	return ZG_SUCCESS;
}

// Memory
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgMemoryHeapCreate(
	ZgMemoryHeap** memoryHeapOut,
	const ZgMemoryHeapCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->sizeInBytes == 0) return ZG_ERROR_INVALID_ARGUMENT;

	zg::IMemoryHeap* memoryHeap = nullptr;
	ZgErrorCode res = zg::getApiContext()->memoryHeapCreate(&memoryHeap, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*memoryHeapOut = reinterpret_cast<ZgMemoryHeap*>(memoryHeap);
	return ZG_SUCCESS;
}

ZG_API ZgErrorCode zgMemoryHeapRelease(
	ZgMemoryHeap* memoryHeap)
{
	return zg::getApiContext()->memoryHeapRelease(reinterpret_cast<zg::IMemoryHeap*>(memoryHeap));
}

ZG_API ZgErrorCode zgMemoryHeapBufferCreate(
	ZgMemoryHeap* memoryHeapIn,
	ZgBuffer** bufferOut,
	const ZgBufferCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if ((createInfo->offsetInBytes % 65536) != 0) return ZG_ERROR_INVALID_ARGUMENT; // 64KiB alignment

	zg::IMemoryHeap* memoryHeap = reinterpret_cast<zg::IMemoryHeap*>(memoryHeapIn);
	zg::IBuffer* buffer = nullptr;
	ZgErrorCode res = memoryHeap->bufferCreate(&buffer, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*bufferOut = reinterpret_cast<ZgBuffer*>(buffer);
	return ZG_SUCCESS;
}

ZG_API void zgBufferRelease(
	ZgBuffer* bufferIn)
{
	if (bufferIn == nullptr) return;
	zg::IBuffer* buffer = reinterpret_cast<zg::IBuffer*>(bufferIn);
	zg::zgDelete<zg::IBuffer>(zg::getContext().allocator, buffer);
}

ZG_API ZgErrorCode zgBufferMemcpyTo(
	ZgBuffer* dstBuffer,
	uint64_t bufferOffsetBytes,
	const void* srcMemory,
	uint64_t numBytes)
{
	return zg::getApiContext()->bufferMemcpyTo(
		reinterpret_cast<zg::IBuffer*>(dstBuffer),
		bufferOffsetBytes,
		reinterpret_cast<const uint8_t*>(srcMemory),
		numBytes);
}

// Textures
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgTextureHeapCreate(
	ZgTextureHeap** textureHeapOut,
	const ZgTextureHeapCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->sizeInBytes == 0) return ZG_ERROR_INVALID_ARGUMENT;

	zg::ITextureHeap* textureHeap = nullptr;
	ZgErrorCode res = zg::getApiContext()->textureHeapCreate(&textureHeap, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*textureHeapOut = reinterpret_cast<ZgTextureHeap*>(textureHeap);
	return ZG_SUCCESS;
}

ZG_API ZgErrorCode zgTextureHeapRelease(
	ZgTextureHeap* textureHeap)
{
	return zg::getApiContext()->textureHeapRelease(reinterpret_cast<zg::ITextureHeap*>(textureHeap));
}

ZG_API ZgErrorCode zgTexture2DGetAllocationInfo(
	ZgTexture2DAllocationInfo* allocationInfoOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	if (allocationInfoOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numMipmaps == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numMipmaps > ZG_TEXTURE_2D_MAX_NUM_MIPMAPS) return ZG_ERROR_INVALID_ARGUMENT;
	return zg::getApiContext()->texture2DGetAllocationInfo(*allocationInfoOut, *createInfo);
}

ZG_API ZgErrorCode zgTextureHeapTexture2DCreate(
	ZgTextureHeap* textureHeapIn,
	ZgTexture2D** textureOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numMipmaps == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numMipmaps > ZG_TEXTURE_2D_MAX_NUM_MIPMAPS) return ZG_ERROR_INVALID_ARGUMENT;
	//if ((createInfo->offsetInBytes % 65536) != 0) return ZG_ERROR_INVALID_ARGUMENT; // 64KiB alignment

	zg::ITextureHeap* textureHeap = reinterpret_cast<zg::ITextureHeap*>(textureHeapIn);
	zg::ITexture2D* texture = nullptr;
	ZgErrorCode res = textureHeap->texture2DCreate(&texture, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*textureOut = reinterpret_cast<ZgTexture2D*>(texture);
	return ZG_SUCCESS;
}

ZG_API void zgTexture2DRelease(
	ZgTexture2D* textureIn)
{
	if (textureIn == nullptr) return;
	zg::ITexture2D* texture = reinterpret_cast<zg::ITexture2D*>(textureIn);
	zg::zgDelete<zg::ITexture2D>(zg::getContext().allocator, texture);
}

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgCommandQueueFlush(
	ZgCommandQueue* commandQueueIn)
{
	zg::ICommandQueue* commandQueue = reinterpret_cast<zg::ICommandQueue*>(commandQueueIn);
	return commandQueue->flush();
}

ZG_API ZgErrorCode zgCommandQueueBeginCommandListRecording(
	ZgCommandQueue* commandQueueIn,
	ZgCommandList** commandListOut)
{
	zg::ICommandQueue* commandQueue = reinterpret_cast<zg::ICommandQueue*>(commandQueueIn);
	zg::ICommandList* commandList = nullptr;
	ZgErrorCode res = commandQueue->beginCommandListRecording(&commandList);
	if (res != ZG_SUCCESS) return res;
	*commandListOut = reinterpret_cast<ZgCommandList*>(commandList);
	return ZG_SUCCESS;
}

ZG_API ZgErrorCode zgCommandQueueExecuteCommandList(
	ZgCommandQueue* commandQueueIn,
	ZgCommandList* commandListIn)
{
	zg::ICommandQueue* commandQueue = reinterpret_cast<zg::ICommandQueue*>(commandQueueIn);
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandQueue->executeCommandList(commandList);
}

// Command list
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgCommandListMemcpyBufferToBuffer(
	ZgCommandList* commandListIn,
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes)
{
	if (numBytes == 0) return ZG_ERROR_INVALID_ARGUMENT;
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->memcpyBufferToBuffer(
		reinterpret_cast<zg::IBuffer*>(dstBuffer),
		dstBufferOffsetBytes,
		reinterpret_cast<zg::IBuffer*>(srcBuffer),
		srcBufferOffsetBytes,
		numBytes);
}

ZG_API ZgErrorCode zgCommandListMemcpyToTexture(
	ZgCommandList* commandListIn,
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
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->memcpyToTexture(
		reinterpret_cast<zg::ITexture2D*>(dstTexture),
		dstTextureMipLevel,
		*srcImageCpu,
		reinterpret_cast<zg::IBuffer*>(tempUploadBuffer));
}

ZG_API ZgErrorCode zgCommandListSetPushConstant(
	ZgCommandList* commandListIn,
	uint32_t shaderRegister,
	const void* data,
	uint32_t dataSizeInBytes)
{
	if (data == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setPushConstant(shaderRegister, data, dataSizeInBytes);
}

ZG_API ZgErrorCode zgCommandListSetPipelineBindings(
	ZgCommandList* commandListIn,
	const ZgPipelineBindings* bindings)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setPipelineBindings(*bindings);
}

ZG_API ZgErrorCode zgCommandListSetPipelineRendering(
	ZgCommandList* commandListIn,
	ZgPipelineRendering* pipeline)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setPipelineRendering(reinterpret_cast<zg::IPipelineRendering*>(pipeline));
}

ZG_API ZgErrorCode zgCommandListSetFramebuffer(
	ZgCommandList* commandListIn,
	ZgFramebuffer* framebufferIn,
	const ZgFramebufferRect* optionalViewport,
	const ZgFramebufferRect* optionalScissor)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	zg::IFramebuffer* framebuffer = reinterpret_cast<zg::IFramebuffer*>(framebufferIn);
	return commandList->setFramebuffer(framebuffer, optionalViewport, optionalScissor);
}

ZG_API ZgErrorCode zgCommandListClearFramebuffer(
	ZgCommandList* commandListIn,
	float red,
	float green,
	float blue,
	float alpha)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->clearFramebuffer(red, green, blue, alpha);
}

ZG_API ZgErrorCode zgCommandListClearDepthBuffer(
	ZgCommandList* commandListIn,
	float depth)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->clearDepthBuffer(depth);
}

ZG_API ZgErrorCode zgCommandListSetIndexBuffer(
	ZgCommandList* commandListIn,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setIndexBuffer(reinterpret_cast<zg::IBuffer*>(indexBuffer), type);
}

ZG_API ZgErrorCode zgCommandListSetVertexBuffer(
	ZgCommandList* commandListIn,
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBuffer)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setVertexBuffer(
		vertexBufferSlot, reinterpret_cast<zg::IBuffer*>(vertexBuffer));
}

ZG_API ZgErrorCode zgCommandListDrawTriangles(
	ZgCommandList* commandListIn,
	uint32_t startVertexIndex,
	uint32_t numVertices)
{
	if ((numVertices % 3) != 0) return ZG_ERROR_INVALID_ARGUMENT;
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->drawTriangles(startVertexIndex, numVertices);
}

ZG_API ZgErrorCode zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandListIn,
	uint32_t startIndex,
	uint32_t numTriangles)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->drawTrianglesIndexed(startIndex, numTriangles);
}
