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

ZG_DLL_API uint32_t zgApiVersion(void)
{
	return ZG_COMPILED_API_VERSION;
}

// Compiled features
// ------------------------------------------------------------------------------------------------

ZG_DLL_API ZgFeatureBits zgCompiledFeatures(void)
{
	return
		uint64_t(ZG_FEATURE_BIT_BACKEND_D3D12);
}

// Context
// ------------------------------------------------------------------------------------------------

ZG_DLL_API ZgErrorCode zgContextCreate(
	ZgContext** contextOut, const ZgContextInitSettings* initSettings)
{
	ZgContextInitSettings settings = *initSettings;

	// Set default logger if none is specified
	bool usingDefaultLogger = settings.logger.log == nullptr;
	if (usingDefaultLogger) {
		settings.logger = zg::getDefaultLogger();
	}
	ZgLogger logger = settings.logger;
	if (usingDefaultLogger) {
		ZG_INFO(logger, "zgContextCreate(): Using default logger (printf)");
	}
	else {
		ZG_INFO(logger, "zgContextCreate(): Using user-provided logger");
	}

	// Set default allocator if none is specified
	if (settings.allocator.allocate == nullptr || settings.allocator.deallocate == nullptr) {
		settings.allocator = zg::getDefaultAllocator();
		ZG_INFO(logger, "zgContextCreate(): Using default allocator");
	}
	else {
		ZG_INFO(logger, "zgContextCreate(): Using user-provided allocator");
	}

	// Allocate context
	ZgContext* context = zg::zgNew<ZgContext>(settings.allocator, "ZeroG Context");
	if (context == nullptr) return ZG_ERROR_CPU_OUT_OF_MEMORY;

	// Set context's allocator
	context->allocator = settings.allocator;
	context->logger = settings.logger;

	// Create and allocate requested backend api
	switch (initSettings->backend) {

	case ZG_BACKEND_NONE:
		// TODO: Implement null backend
		zg::zgDelete(settings.allocator, context);
		ZG_ERROR(logger, "zgContextCreate(): Null backend not implemented, exiting.");
		return ZG_ERROR_UNIMPLEMENTED;

	case ZG_BACKEND_D3D12:
		{
			ZgErrorCode res = zg::createD3D12Backend(&context->context, settings);
			if (res != ZG_SUCCESS) {
				zg::zgDelete(settings.allocator, context);
				ZG_ERROR(logger, "zgContextCreate(): Could not create D3D12 backend, exiting.");
				return res;
			}
			ZG_INFO(logger, "zgContextCreate(): Created D3D12 backend");
		}
		break;

	default:
		zg::zgDelete(settings.allocator, context);
		return ZG_ERROR_GENERIC;
	}

	// Return context
	*contextOut = context;
	return ZG_SUCCESS;
}

ZG_DLL_API ZgErrorCode zgContextDestroy(ZgContext* context)
{
	if (context == nullptr) return ZG_SUCCESS;

	// Delete API
	zg::zgDelete<zg::IContext>(context->allocator, context->context);

	// Delete context
	ZgAllocator allocator = context->allocator;
	zg::zgDelete<ZgContext>(allocator, context);

	return ZG_SUCCESS;
}

ZG_DLL_API ZgErrorCode zgContextResize(ZgContext* context, uint32_t width, uint32_t height)
{
	return context->context->resize(width, height);
}

ZG_DLL_API ZgErrorCode zgContextGeCommandQueueGraphicsPresent(
	ZgContext* context,
	ZgCommandQueue** commandQueueOut)
{
	zg::ICommandQueue* commandQueue = nullptr;
	ZgErrorCode res = context->context->getCommandQueueGraphicsPresent(&commandQueue);
	if (res != ZG_SUCCESS) return res;
	*commandQueueOut = reinterpret_cast<ZgCommandQueue*>(commandQueue);
	return ZG_SUCCESS;
}

ZG_DLL_API ZgErrorCode zgContextBeginFrame(
	ZgContext* context,
	ZgFramebuffer** framebufferOut)
{
	zg::IFramebuffer* framebuffer = nullptr;
	ZgErrorCode res = context->context->beginFrame(&framebuffer);
	if (res != ZG_SUCCESS) return res;
	*framebufferOut = reinterpret_cast<ZgFramebuffer*>(framebuffer);
	return ZG_SUCCESS;
}

ZG_DLL_API ZgErrorCode zgContextFinishFrame(
	ZgContext* context)
{
	return context->context->finishFrame();
}

// Pipeline
// ------------------------------------------------------------------------------------------------

// Note: A ZgPipeline struct does not really exist. It's just an alias for the internal
// zg::IPipeline currently. This may (or may not) change in the future.

ZG_DLL_API ZgErrorCode zgPipelineRenderingCreate(
	ZgContext* context,
	ZgPipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfo* createInfo)
{
	// Check arguments
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (pipelineOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (signatureOut == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->vertexShaderPath == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->vertexShaderEntry == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->pixelShaderPath == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->pixelShaderEntry == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->shaderVersion == ZG_SHADER_MODEL_UNDEFINED) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numVertexAttributes == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numVertexBufferSlots == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS) return ZG_ERROR_INVALID_ARGUMENT;

	zg::IPipelineRendering* pipeline = nullptr;
	ZgErrorCode res = context->context->pipelineRenderingCreate(
		&pipeline, signatureOut, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*pipelineOut = reinterpret_cast<ZgPipelineRendering*>(pipeline);
	return ZG_SUCCESS;
}

ZG_DLL_API ZgErrorCode zgPipelineRenderingRelease(
	ZgContext* context,
	ZgPipelineRendering* pipeline)
{
	return context->context->pipelineRenderingRelease(
		reinterpret_cast<zg::IPipelineRendering*>(pipeline));
}

ZG_DLL_API ZgErrorCode zgPipelineRenderingGetSignature(
	ZgContext* context,
	const ZgPipelineRendering* pipeline,
	ZgPipelineRenderingSignature* signatureOut)
{
	return context->context->pipelineRenderingGetSignature(
		reinterpret_cast<const zg::IPipelineRendering*>(pipeline), signatureOut);
}

// Memory
// ------------------------------------------------------------------------------------------------

ZG_DLL_API ZgErrorCode zgMemoryHeapCreate(
	ZgContext* context,
	ZgMemoryHeap** memoryHeapOut,
	const ZgMemoryHeapCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->sizeInBytes == 0) return ZG_ERROR_INVALID_ARGUMENT;

	zg::IMemoryHeap* memoryHeap = nullptr;
	ZgErrorCode res = context->context->memoryHeapCreate(&memoryHeap, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*memoryHeapOut = reinterpret_cast<ZgMemoryHeap*>(memoryHeap);
	return ZG_SUCCESS;
}

ZG_DLL_API ZgErrorCode zgMemoryHeapRelease(
	ZgContext* context,
	ZgMemoryHeap* memoryHeap)
{
	return context->context->memoryHeapRelease(reinterpret_cast<zg::IMemoryHeap*>(memoryHeap));
}

ZG_DLL_API ZgErrorCode zgMemoryHeapBufferCreate(
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

ZG_DLL_API ZgErrorCode zgMemoryHeapBufferRelease(
	ZgMemoryHeap* memoryHeapIn,
	ZgBuffer* buffer)
{
	zg::IMemoryHeap* memoryHeap = reinterpret_cast<zg::IMemoryHeap*>(memoryHeapIn);
	return memoryHeap->bufferRelease(reinterpret_cast<zg::IBuffer*>(buffer));
}

ZG_DLL_API ZgErrorCode zgBufferMemcpyTo(
	ZgContext* context,
	ZgBuffer* dstBuffer,
	uint64_t bufferOffsetBytes,
	const void* srcMemory,
	uint64_t numBytes)
{
	return context->context->bufferMemcpyTo(
		reinterpret_cast<zg::IBuffer*>(dstBuffer),
		bufferOffsetBytes,
		reinterpret_cast<const uint8_t*>(srcMemory),
		numBytes);
}

// Textures
// ------------------------------------------------------------------------------------------------

ZG_DLL_API ZgErrorCode zgTextureHeapCreate(
	ZgContext* context,
	ZgTextureHeap** textureHeapOut,
	const ZgTextureHeapCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (createInfo->sizeInBytes == 0) return ZG_ERROR_INVALID_ARGUMENT;

	zg::ITextureHeap* textureHeap = nullptr;
	ZgErrorCode res = context->context->textureHeapCreate(&textureHeap, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*textureHeapOut = reinterpret_cast<ZgTextureHeap*>(textureHeap);
	return ZG_SUCCESS;
}

ZG_DLL_API ZgErrorCode zgTextureHeapRelease(
	ZgContext* context,
	ZgTextureHeap* textureHeap)
{
	return context->context->textureHeapRelease(reinterpret_cast<zg::ITextureHeap*>(textureHeap));
}

ZG_DLL_API ZgErrorCode zgTextureHeapTexture2DGetAllocationInfo(
	ZgTextureHeap* textureHeapIn,
	ZgTexture2DAllocationInfo* allocationInfoOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	zg::ITextureHeap* textureHeap = reinterpret_cast<zg::ITextureHeap*>(textureHeapIn);
	return textureHeap->texture2DGetAllocationInfo(*allocationInfoOut, *createInfo);
}

ZG_DLL_API ZgErrorCode zgTextureHeapTexture2DCreate(
	ZgTextureHeap* textureHeapIn,
	ZgTexture2D** textureOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	if (createInfo == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	//if ((createInfo->offsetInBytes % 65536) != 0) return ZG_ERROR_INVALID_ARGUMENT; // 64KiB alignment

	zg::ITextureHeap* textureHeap = reinterpret_cast<zg::ITextureHeap*>(textureHeapIn);
	zg::ITexture2D* texture = nullptr;
	ZgErrorCode res = textureHeap->texture2DCreate(&texture, *createInfo);
	if (res != ZG_SUCCESS) return res;
	*textureOut = reinterpret_cast<ZgTexture2D*>(texture);
	return ZG_SUCCESS;
}

ZG_DLL_API ZgErrorCode zgMemoryHeapTexture2DRelease(
	ZgTextureHeap* textureHeapIn,
	ZgTexture2D* texture)
{
	zg::ITextureHeap* textureHeap = reinterpret_cast<zg::ITextureHeap*>(textureHeapIn);
	return textureHeap->texture2DRelease(reinterpret_cast<zg::ITexture2D*>(texture));
}

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_DLL_API ZgErrorCode zgCommandQueueFlush(
	ZgCommandQueue* commandQueueIn)
{
	zg::ICommandQueue* commandQueue = reinterpret_cast<zg::ICommandQueue*>(commandQueueIn);
	return commandQueue->flush();
}

ZG_DLL_API ZgErrorCode zgCommandQueueBeginCommandListRecording(
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

ZG_DLL_API ZgErrorCode zgCommandQueueExecuteCommandList(
	ZgCommandQueue* commandQueueIn,
	ZgCommandList* commandListIn)
{
	zg::ICommandQueue* commandQueue = reinterpret_cast<zg::ICommandQueue*>(commandQueueIn);
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandQueue->executeCommandList(commandList);
}

// Command list
// ------------------------------------------------------------------------------------------------

ZG_DLL_API ZgErrorCode zgCommandListMemcpyBufferToBuffer(
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

ZG_DLL_API ZgErrorCode zgCommandListMemcpyToTexture(
	ZgCommandList* commandListIn,
	ZgTexture2D* dstTexture,
	const ZgImageViewConstCpu* srcImageCpu,
	ZgBuffer* tempUploadBuffer)
{
	if (srcImageCpu->data == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (srcImageCpu->width == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (srcImageCpu->height == 0) return ZG_ERROR_INVALID_ARGUMENT;
	if (srcImageCpu->pitchInBytes < srcImageCpu->width) return ZG_ERROR_INVALID_ARGUMENT;
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->memcpyToTexture(
		reinterpret_cast<zg::ITexture2D*>(dstTexture),
		*srcImageCpu,
		reinterpret_cast<zg::IBuffer*>(tempUploadBuffer));
}

ZG_DLL_API ZgErrorCode zgCommandListSetPushConstant(
	ZgCommandList* commandListIn,
	uint32_t shaderRegister,
	const void* data,
	uint32_t dataSizeInBytes)
{
	if (data == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setPushConstant(shaderRegister, data, dataSizeInBytes);
}

ZG_DLL_API ZgErrorCode zgCommandListSetPipelineBindings(
	ZgCommandList* commandListIn,
	const ZgPipelineBindings* bindings)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setPipelineBindings(*bindings);
}

ZG_DLL_API ZgErrorCode zgCommandListSetPipelineRendering(
	ZgCommandList* commandListIn,
	ZgPipelineRendering* pipeline)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setPipelineRendering(reinterpret_cast<zg::IPipelineRendering*>(pipeline));
}

ZG_DLL_API ZgErrorCode zgCommandListSetFramebuffer(
	ZgCommandList* commandListIn,
	const ZgCommandListSetFramebufferInfo* info)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setFramebuffer(*info);
}

ZG_DLL_API ZgErrorCode zgCommandListClearFramebuffer(
	ZgCommandList* commandListIn,
	float red,
	float green,
	float blue,
	float alpha)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->clearFramebuffer(red, green, blue, alpha);
}

ZG_DLL_API ZgErrorCode zgCommandListClearDepthBuffer(
	ZgCommandList* commandListIn,
	float depth)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->clearDepthBuffer(depth);
}

ZG_DLL_API ZgErrorCode zgCommandListSetIndexBuffer(
	ZgCommandList* commandListIn,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setIndexBuffer(reinterpret_cast<zg::IBuffer*>(indexBuffer), type);
}

ZG_DLL_API ZgErrorCode zgCommandListSetVertexBuffer(
	ZgCommandList* commandListIn,
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBuffer)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->setVertexBuffer(
		vertexBufferSlot, reinterpret_cast<zg::IBuffer*>(vertexBuffer));
}

ZG_DLL_API ZgErrorCode zgCommandListDrawTriangles(
	ZgCommandList* commandListIn,
	uint32_t startVertexIndex,
	uint32_t numVertices)
{
	if ((numVertices % 3) != 0) return ZG_ERROR_INVALID_ARGUMENT;
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->drawTriangles(startVertexIndex, numVertices);
}

ZG_DLL_API ZgErrorCode zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandListIn,
	uint32_t startIndex,
	uint32_t numTriangles)
{
	zg::ICommandList* commandList = reinterpret_cast<zg::ICommandList*>(commandListIn);
	return commandList->drawTrianglesIndexed(startIndex, numTriangles);
}
