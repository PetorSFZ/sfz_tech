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

// About
// ------------------------------------------------------------------------------------------------

// This header file contains the ZeroG C-API. If you are using C++ you might also want to link with
// and use the C++ wrapper static library where appropriate. The C++ wrapper header is
// "ZeroG/ZeroG.hpp".

// Includes
// ------------------------------------------------------------------------------------------------

#pragma once

// This entire header is pure C
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Macros
// ------------------------------------------------------------------------------------------------

#if defined(_WIN32)
#if defined(ZG_DLL_EXPORT)
#define ZG_DLL_API __declspec(dllexport)
#else
#define ZG_DLL_API __declspec(dllimport)
#endif
#else
#define ZG_DLL_API
#endif

// ZeroG handles
// ------------------------------------------------------------------------------------------------

#define ZG_HANDLE(name) \
	struct name; \
	typedef struct name name;

// The main ZeroG context handle
ZG_HANDLE(ZgContext);

// A handle representing a rendering pipeline
ZG_HANDLE(ZgPipelineRendering);

// A handle representing a buffer
ZG_HANDLE(ZgBuffer);

// A handle representing a framebuffer
ZG_HANDLE(ZgFramebuffer);

// A handle representing a command list
ZG_HANDLE(ZgCommandList);

// A handle representing a command queue
ZG_HANDLE(ZgCommandQueue);

// Bool
// ------------------------------------------------------------------------------------------------

typedef uint32_t ZgBool;
static const ZgBool ZG_FALSE = 0;
static const ZgBool ZG_TRUE = 1;

// Version information
// ------------------------------------------------------------------------------------------------

// The API version used to compile ZeroG.
static const uint32_t ZG_COMPILED_API_VERSION = 0;

// Returns the API version of ZeroG.
//
// As long as the DLL has the same API version as the version you compiled with it should be
// compatible.
ZG_DLL_API uint32_t zgApiVersion(void);

// Backends enums
// ------------------------------------------------------------------------------------------------

// The various backends supported by ZeroG
enum ZgBackendTypeEnum {
	// The null backend, simply turn every ZeroG call into a no-op.
	ZG_BACKEND_NONE = 0,

	// The D3D12 backend, only available on Windows 10.
	ZG_BACKEND_D3D12,

	//ZG_BACKEND_VULKAN,
	//ZG_BACKEND_METAL,
	//ZG_BACKEND_WEB_GPU,
	//ZG_BACKEND_D3D11
};
typedef uint32_t ZgBackendType;

// Compiled features
// ------------------------------------------------------------------------------------------------

// Feature bits representing different features that can be compiled into ZeroG.
//
// If you depend on a specific feature (such as the D3D12 backend) it is a good idea to query and
// check if it is available
enum ZgFeatureBitsEnum {
	ZG_FEATURE_BIT_NONE = 0,
	ZG_FEATURE_BIT_BACKEND_D3D12 = 1 << 1,
	//ZG_FEATURE_BIT_BACKEND_VULKAN = 1 << 2
	//ZG_FEATURE_BIT_BACKEND_METAL = 1 << 3
	//ZG_FEATURE_BIT_BACKEND_WEB_GPU = 1 << 4
	//ZG_FEATURE_BIT_BACKEND_D3D11 = 1 << 5
};
typedef uint64_t ZgFeatureBits;

// Returns a bitmask containing the features compiled into this ZeroG dll.
ZG_DLL_API ZgFeatureBits zgCompiledFeatures(void);

// Error codes
// ------------------------------------------------------------------------------------------------

// The error codes
enum ZgErrorCodeEnum {
	ZG_SUCCESS = 0,
	ZG_ERROR_GENERIC,
	ZG_ERROR_UNIMPLEMENTED,
	ZG_ERROR_CPU_OUT_OF_MEMORY,
	ZG_ERROR_GPU_OUT_OF_MEMORY,
	ZG_ERROR_NO_SUITABLE_DEVICE,
	ZG_ERROR_INVALID_ARGUMENT,
	ZG_ERROR_SHADER_COMPILE_ERROR,
	ZG_ERROR_OUT_OF_COMMAND_LISTS,
	ZG_ERROR_INVALID_COMMAND_LIST_STATE
};
typedef uint32_t ZgErrorCode;

// Memory allocator interface
// ------------------------------------------------------------------------------------------------

// Allocator interface for CPU allocations inside ZeroG.
//
// A few restrictions is placed on custom allocators:
// * They must be thread-safe. I.e. it must be okay to call it simulatenously from multiple threads.
// * All allocations must be at least 32-byte aligned.
//
// If no custom allocator is required, just leave all fields zero in this struct.
typedef struct {
	// Function pointer to allocate function. The allocation created must be 32-byte aligned. The
	// name is a short string (< ~32 chars) explaining what the allocation is used for, useful
	// for debug or visualization purposes.
	uint8_t* (*allocate)(void* userPtr, uint32_t size, const char* name);

	// Function pointer to deallocate function.
	void (*deallocate)(void* userPtr, uint8_t* allocation);

	// User specified pointer that is provided to each allocate/free call.
	void* userPtr;
} ZgAllocator;

// Context
// ------------------------------------------------------------------------------------------------

// The settings used to create a context and initialize ZeroG
typedef struct {

	// [Mandatory] The wanted ZeroG backend
	ZgBackendType backend;

	// [Mandatory] The dimensions (in pixels) of the window being rendered to
	uint32_t width;
	uint32_t height;

	// [Optional] Used to enable debug mode. This means enabling various debug layers and such
	//            in the underlaying APIs.
	ZgBool debugMode;

	// [Optional] The allocator used to allocate CPU memory
	ZgAllocator allocator;

	// [Mandatory] The native window handle, e.g. HWND on Windows
	// TODO: Might want to change this in the future, possibly want different things depending on
	//       backend and OS.
	void* nativeWindowHandle;

} ZgContextInitSettings;

ZG_DLL_API ZgErrorCode zgContextCreate(
	ZgContext** contextOut,
	const ZgContextInitSettings* initSettings);

ZG_DLL_API ZgErrorCode zgContextDestroy(
	ZgContext* context);

// Resize the back buffers in the swap chain to the new size.
//
// This should be called every time the size of the window or the resolution is changed. This
// function is guaranteed to not do anything if the specified width or height is the same as last
// time, so it is completely safe to call this at the beginning of each frame.
ZG_DLL_API ZgErrorCode zgContextResize(
	ZgContext* context,
	uint32_t width,
	uint32_t height);

ZG_DLL_API ZgErrorCode zgContextGeCommandQueueGraphicsPresent(
	ZgContext* context,
	ZgCommandQueue** commandQueueOut);

// TODO: zgContextGetCopyQueue(), zgContextGetComputeQueue

ZG_DLL_API ZgErrorCode zgContextBeginFrame(
	ZgContext* context,
	ZgFramebuffer** framebufferOut);

ZG_DLL_API ZgErrorCode zgContextFinishFrame(
	ZgContext* context);

// Pipeline
// ------------------------------------------------------------------------------------------------

// Enum representing various shader model versions
enum ZgShaderModelEnum {
	ZG_SHADER_MODEL_UNDEFINED = 0,
	ZG_SHADER_MODEL_5_1,
	ZG_SHADER_MODEL_6_0,
	ZG_SHADER_MODEL_6_1,
	ZG_SHADER_MODEL_6_2,
	ZG_SHADER_MODEL_6_3
};
typedef uint32_t ZgShaderModel;

// The maximum number of compiler flags allowed to the DXC shader compiler
static const uint32_t ZG_MAX_NUM_DXC_COMPILER_FLAGS = 8;

// The type of data contained in a vertex
enum ZgVertexAttributeTypeEnum {
	ZG_VERTEX_ATTRIBUTE_UNDEFINED = 0,

	ZG_VERTEX_ATTRIBUTE_FLOAT,
	ZG_VERTEX_ATTRIBUTE_FLOAT2,
	ZG_VERTEX_ATTRIBUTE_FLOAT3,
	ZG_VERTEX_ATTRIBUTE_FLOAT4,
};
typedef uint32_t ZgVertexAttributeType;

// A struct defining a vertex attribute
typedef struct {
	// The location of the attribute in the vertex input.
	//
	// For HLSL the semantic name need to be "ATTRIBUTE_LOCATION_<attributeLocation>"
	// E.g.:
	// struct VSInput {
	//     float3 position : ATTRIBUTE_LOCATION_0;
	// }
	uint32_t attributeLocation;

	// The data type
	ZgVertexAttributeType type;

	// Offset in bytes from start of buffer to the first element of this type.
	uint32_t offsetToFirstElementInBytes;

} ZgVertexAttribute;

// The maximum number of vertex attributes allowed as input to a vertex shader
static const uint32_t ZG_MAX_NUM_VERTEX_ATTRIBUTES = 8;

// The information required to create a rendering pipeline
typedef struct {

	// Vertex shader information
	const char* vertexShaderPath;
	const char* vertexShaderEntry;

	// Pixel shader information
	const char* pixelShaderPath;
	const char* pixelShaderEntry;

	// Information to the DXC compiler
	ZgShaderModel shaderVersion;
	const char* dxcCompilerFlags[ZG_MAX_NUM_DXC_COMPILER_FLAGS];

	// The vertex attributes to the vertex shader
	uint32_t numVertexAttributes;
	ZgVertexAttribute vertexAttributes[ZG_MAX_NUM_VERTEX_ATTRIBUTES];

} ZgPipelineRenderingCreateInfo;

ZG_DLL_API ZgErrorCode zgPipelineRenderingCreate(
	ZgContext* context,
	ZgPipelineRendering** pipelineOut,
	const ZgPipelineRenderingCreateInfo* createInfo);

ZG_DLL_API ZgErrorCode zgPipelineRenderingRelease(
	ZgContext* context,
	ZgPipelineRendering* pipeline);

// Memory
// ------------------------------------------------------------------------------------------------

enum ZgBufferMemoryTypeEnum {
	ZG_BUFFER_MEMORY_TYPE_UNDEFINED = 0,

	// Memory suitable for uploading data to GPU.
	// Can not be used as a shader UAV, only as vertex shader input.
	ZG_BUFFER_MEMORY_TYPE_UPLOAD,

	// Memory suitable for downloading data from GPU.
	ZG_BUFFER_MEMORY_TYPE_DOWNLOAD,

	// Fastest memory available on GPU.
	// Can't upload or download directly to this memory from CPU, need to use UPLOAD and DOWNLOAD
	// as intermediary.
	ZG_BUFFER_MEMORY_TYPE_DEVICE
};
typedef uint32_t ZgBufferMemoryType;

typedef struct {

	// The size in bytes of the buffer
	uint64_t sizeInBytes;

	// The type of memory
	ZgBufferMemoryType bufferMemoryType;

} ZgBufferCreateInfo;

ZG_DLL_API ZgErrorCode zgBufferCreate(
	ZgContext* context,
	ZgBuffer** bufferOut,
	const ZgBufferCreateInfo* createInfo);

ZG_DLL_API ZgErrorCode zgBufferRelease(
	ZgContext* context,
	ZgBuffer* buffer);

ZG_DLL_API ZgErrorCode zgBufferMemcpyTo(
	ZgContext* context,
	ZgBuffer* dstBuffer,
	uint64_t bufferOffsetBytes,
	const uint8_t* srcMemory,
	uint64_t numBytes);

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_DLL_API ZgErrorCode zgCommandQueueFlush(
	ZgCommandQueue* commandQueue);

ZG_DLL_API ZgErrorCode zgCommandQueueBeginCommandListRecording(
	ZgCommandQueue* commandQueue,
	ZgCommandList** commandListOut);

ZG_DLL_API ZgErrorCode zgCommandQueueExecuteCommandList(
	ZgCommandQueue* commandQueue,
	ZgCommandList* commandList);

// Command list
// ------------------------------------------------------------------------------------------------

ZG_DLL_API ZgErrorCode zgCommandListSetPipelineRendering(
	ZgCommandList* commandList,
	ZgPipelineRendering* pipeline);

ZG_DLL_API ZgErrorCode zgCommandListExperimentalCommands(
	ZgCommandList* commandList,
	ZgFramebuffer* framebuffer,
	ZgBuffer* vertexBuffer,
	ZgPipelineRendering* pipeline);


// This entire header is pure C
#ifdef __cplusplus
} // extern "C"
#endif
