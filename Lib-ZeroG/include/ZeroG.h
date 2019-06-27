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
// and use the C++ wrapper static library where appropriate.

// Includes
// ------------------------------------------------------------------------------------------------

#pragma once

// Include standard integer types
#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

// This entire header is pure C
#ifdef __cplusplus
extern "C" {
#endif

// Macros
// ------------------------------------------------------------------------------------------------

#if defined(_WIN32)
#if defined(ZG_DLL_EXPORT)
#define ZG_API __declspec(dllexport)
#else
#define ZG_API __declspec(dllimport)
#endif
#else
#define ZG_API
#endif

// ZeroG handles
// ------------------------------------------------------------------------------------------------

// Macro to declare a ZeroG handle. As a user you can never dereference or inspect a ZeroG handle,
// you can only store pointers to them.
#define ZG_HANDLE(name) \
	struct name; \
	typedef struct name name;

// A handle representing a rendering pipeline
ZG_HANDLE(ZgPipelineRendering);

// A handle representing a memory heap (to allocate ZgBuffers from)
ZG_HANDLE(ZgMemoryHeap);

// A handle representing a buffer
ZG_HANDLE(ZgBuffer);

// A handle representing a memory heap for textures
ZG_HANDLE(ZgTextureHeap);

// A handle representing a 2-dimensional texture
ZG_HANDLE(ZgTexture2D);

// A handle representing a framebuffer
ZG_HANDLE(ZgFramebuffer);

// A handle representing a command list
ZG_HANDLE(ZgCommandList);

// A handle representing a command queue
ZG_HANDLE(ZgCommandQueue);

// Bool
// ------------------------------------------------------------------------------------------------

// The ZeroG bool type.
typedef uint32_t ZgBool;
static const ZgBool ZG_FALSE = 0;
static const ZgBool ZG_TRUE = 1;

// Framebuffer rectangle
// ------------------------------------------------------------------------------------------------

struct ZgFramebufferRect {
	uint32_t topLeftX, topLeftY, width, height;
};
typedef struct ZgFramebufferRect ZgFramebufferRect;

// Version information
// ------------------------------------------------------------------------------------------------

// The API version used to compile ZeroG.
static const uint32_t ZG_COMPILED_API_VERSION = 0;

// Returns the API version of the ZeroG DLL you have linked with
//
// As long as the DLL has the same API version as the version you compiled with it should be
// compatible.
ZG_API uint32_t zgApiLinkedVersion(void);

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
};
typedef uint64_t ZgFeatureBits;

// Returns a bitmask containing the features compiled into this ZeroG dll.
ZG_API ZgFeatureBits zgCompiledFeatures(void);

// Error codes
// ------------------------------------------------------------------------------------------------

// The error codes
enum ZgErrorCodeEnum {
	ZG_SUCCESS = 0,
	ZG_ERROR_GENERIC,
	ZG_ERROR_UNIMPLEMENTED,
	ZG_ERROR_ALREADY_INITIALIZED,
	ZG_ERROR_CPU_OUT_OF_MEMORY,
	ZG_ERROR_GPU_OUT_OF_MEMORY,
	ZG_ERROR_NO_SUITABLE_DEVICE,
	ZG_ERROR_INVALID_ARGUMENT,
	ZG_ERROR_SHADER_COMPILE_ERROR,
	ZG_ERROR_OUT_OF_COMMAND_LISTS,
	ZG_ERROR_INVALID_COMMAND_LIST_STATE
};
typedef uint32_t ZgErrorCode;

// Returns a string representation of the given ZeroG error code. The string is statically
// allocated and must NOT be freed by the user.
ZG_API const char* zgErrorCodeToString(ZgErrorCode errorCode);

// Logging interface
// ------------------------------------------------------------------------------------------------

enum ZgLogLevelEnum {
	ZG_LOG_LEVEL_INFO = 0,
	ZG_LOG_LEVEL_WARNING,
	ZG_LOG_LEVEL_ERROR
};
typedef uint32_t ZgLogLevel;

// Logger used for logging inside ZeroG.
//
// The logger must be thread-safe. I.e. it must be okay to call it simulatenously from multiple
// threads.
//
// If no custom logger is wanted leave all fields zero in this struct. Normal printf() will then be
// used for logging instead.
struct ZgLogger {

	// Function pointer to user-specified log function.
	void(*log)(void* userPtr, const char* file, int line, ZgLogLevel level, const char* message);

	// User specified pointer that is provied to each log() call.
	void* userPtr;
};
typedef struct ZgLogger ZgLogger;

// Memory allocator interface
// ------------------------------------------------------------------------------------------------

// Allocator interface for CPU allocations inside ZeroG.
//
// A few restrictions is placed on custom allocators:
// * They must be thread-safe. I.e. it must be okay to call it simulatenously from multiple threads.
// * All allocations must be at least 32-byte aligned.
//
// If no custom allocator is required, just leave all fields zero in this struct.
struct ZgAllocator {

	// Function pointer to allocate function. The allocation created must be 32-byte aligned. The
	// name is a short string (< ~32 chars) explaining what the allocation is used for, useful
	// for debug or visualization purposes.
	uint8_t* (*allocate)(void* userPtr, uint32_t size, const char* name);

	// Function pointer to deallocate function.
	void (*deallocate)(void* userPtr, void* allocation);

	// User specified pointer that is provided to each allocate/free call.
	void* userPtr;
};
typedef struct ZgAllocator ZgAllocator;

// Context
// ------------------------------------------------------------------------------------------------

// The settings used to create a context and initialize ZeroG
struct ZgContextInitSettings {

	// [Mandatory] The wanted ZeroG backend
	ZgBackendType backend;

	// [Mandatory] The dimensions (in pixels) of the window being rendered to
	uint32_t width;
	uint32_t height;

	// [Optional] Used to enable debug mode. This means enabling various debug layers and such
	//            in the underlaying APIs.
	ZgBool debugMode;

	// [Optional] The logger used for logging
	ZgLogger logger;

	// [Optional] The allocator used to allocate CPU memory
	ZgAllocator allocator;

	// [Mandatory] The native window handle, e.g. HWND on Windows
	// TODO: Might want to change this in the future, possibly want different things depending on
	//       backend and OS.
	void* nativeWindowHandle;

};
typedef struct ZgContextInitSettings ZgContextInitSettings;

// Checks if the implicit ZeroG context is already initialized or not
ZG_API ZgBool zgContextAlreadyInitialized(void);

// Initializes the implicit ZeroG context, will fail if a context is already initialized
ZG_API ZgErrorCode zgContextInit(const ZgContextInitSettings* initSettings);

// Deinitializes the implicit ZeroG context
//
// Completely safe to call even if no context has been created
ZG_API ZgErrorCode zgContextDeinit(void);

// Resize the back buffers in the swap chain to the new size.
//
// This should be called every time the size of the window or the resolution is changed. This
// function is guaranteed to not do anything if the specified width or height is the same as last
// time, so it is completely safe to call this at the beginning of each frame.
ZG_API ZgErrorCode zgContextSwapchainResize(
	uint32_t width,
	uint32_t height);

ZG_API ZgErrorCode zgContextSwapchainCommandQueue(
	ZgCommandQueue** commandQueueOut);

ZG_API ZgErrorCode zgContextSwapchainBeginFrame(
	ZgFramebuffer** framebufferOut);

ZG_API ZgErrorCode zgContextSwapchainFinishFrame(void);

// Pipeline Rendering - Signature
// ------------------------------------------------------------------------------------------------

// The maximum number of vertex attributes allowed as input to a vertex shader
static const uint32_t ZG_MAX_NUM_VERTEX_ATTRIBUTES = 8;

// The maximum number of constant buffers allowed on a single pipeline
static const uint32_t ZG_MAX_NUM_CONSTANT_BUFFERS = 16;

// The maximum number of textures allowed on a single pipeline
static const uint32_t ZG_MAX_NUM_TEXTURES = 16;

// The maximum number of samplers allowed on a single pipeline
static const uint32_t ZG_MAX_NUM_SAMPLERS = 8;

// The type of data contained in a vertex
enum ZgVertexAttributeTypeEnum {
	ZG_VERTEX_ATTRIBUTE_UNDEFINED = 0,

	ZG_VERTEX_ATTRIBUTE_F32,
	ZG_VERTEX_ATTRIBUTE_F32_2,
	ZG_VERTEX_ATTRIBUTE_F32_3,
	ZG_VERTEX_ATTRIBUTE_F32_4,

	ZG_VERTEX_ATTRIBUTE_S32,
	ZG_VERTEX_ATTRIBUTE_S32_2,
	ZG_VERTEX_ATTRIBUTE_S32_3,
	ZG_VERTEX_ATTRIBUTE_S32_4,

	ZG_VERTEX_ATTRIBUTE_U32,
	ZG_VERTEX_ATTRIBUTE_U32_2,
	ZG_VERTEX_ATTRIBUTE_U32_3,
	ZG_VERTEX_ATTRIBUTE_U32_4,
};
typedef uint32_t ZgVertexAttributeType;

// A struct defining a vertex attribute
struct ZgVertexAttribute {
	// The location of the attribute in the vertex input.
	//
	// For HLSL the semantic name need to be "TEXCOORD<attributeLocation>"
	// E.g.:
	// struct VSInput {
	//     float3 position : TEXCOORD0;
	//     float3 normal : TEXCOORD1;
	// }
	uint32_t location;

	// Which vertex buffer slot the attribute should be read from.
	//
	// If you are storing all vertex attributes in the same buffer (e.g. your buffer is an array
	// of a vertex struct of some kind), this parameter should typically be 0.
	//
	// This corresponds to the "vertexBufferSlot" parameter in zgCommandListSetVertexBuffer().
	uint32_t vertexBufferSlot;

	// The data type
	ZgVertexAttributeType type;

	// Offset in bytes from start of buffer to the first element of this type.
	uint32_t offsetToFirstElementInBytes;

};
typedef struct ZgVertexAttribute ZgVertexAttribute;

struct ZgConstantBufferDesc {

	// Which register this buffer corresponds to in the shader. In D3D12 this is the "register"
	// keyword, i.e. a value of 0 would mean "register(b0)". In GLSL this corresponds to the
	// "binding" keyword, i.e. "layout(binding = 0)".
	uint32_t shaderRegister;

	// Size of the buffer in bytes
	uint32_t sizeInBytes;

	// Whether the buffer is a push constant or not
	//
	// The size of a push constant must be a multiple of 4 bytes, i.e. an even (32-bit) word.
	//
	// In D3D12, a push constant is stored directly in the root signature. No indirection needed
	// when loading it in the shader. Do note that the maximum size of a root signature in D3D12
	// is 64 32-bit words. This is for ALL push constants and other types of parameters combined.
	// Therefore ZeroG imposes an limit of a maximum size of 32 32-bit words (128 bytes) per push
	// constant. In addition, Microsoft recommends keeping the root signature smaller than 16
	// words to maximize performance on some hardware.
	ZgBool pushConstant;

	// Whether the buffer is accessed by the vertex shader or not
	ZgBool vertexAccess;

	// Whether the buffer is accessed by the pixel shader or not
	ZgBool pixelAccess;
};
typedef struct ZgConstantBufferDesc ZgConstantBufferDesc;

struct ZgTextureDesc {

	// Which register this texture corresponds to in the shader.
	uint32_t textureRegister;

	// Whether the texture is accessed by the vertex shader or not
	ZgBool vertexAccess;

	// Whether the texture is accessed by the pixel shader or not
	ZgBool pixelAccess;
};
typedef struct ZgTextureDesc ZgTextureDesc;

// A struct representing the signature of a rendering pipeline
//
// The signature contains all information necessary to know how to bind input and output to a
// pipeline. Of course, in practice this is something the programmer should already be aware of as
// they have access to (or even wrote) the shaders in the first place.
//
// The signature is inferred by performing reflection on the shaders being compiled. Some
// information that can not be automatically inferred is created by additional data supplied
// when creating the pipeline.
struct ZgPipelineRenderingSignature {

	// The vertex attributes to the vertex shader
	uint32_t numVertexAttributes;
	ZgVertexAttribute vertexAttributes[ZG_MAX_NUM_VERTEX_ATTRIBUTES];

	// The constant buffers
	uint32_t numConstantBuffers;
	ZgConstantBufferDesc constantBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// The textures
	uint32_t numTextures;
	ZgTextureDesc textures[ZG_MAX_NUM_TEXTURES];
};
typedef struct ZgPipelineRenderingSignature ZgPipelineRenderingSignature;

// Pipeline Rendering - Common

// ------------------------------------------------------------------------------------------------
// Sample mode of a sampler
enum ZgSamplingModeEnum {
	ZG_SAMPLING_MODE_UNDEFINED = 0,

	ZG_SAMPLING_MODE_NEAREST, // D3D12_FILTER_MIN_MAG_MIP_POINT
	ZG_SAMPLING_MODE_TRILINEAR, // D3D12_FILTER_MIN_MAG_MIP_LINEAR
	ZG_SAMPLING_MODE_ANISOTROPIC, // D3D12_FILTER_ANISOTROPIC
};
typedef uint32_t ZgSamplingMode;

// Wrapping mode of a sampler
enum ZgWrappingModeEnum {
	ZG_WRAPPING_MODE_UNDEFINED = 0,

	ZG_WRAPPING_MODE_CLAMP, // D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	ZG_WRAPPING_MODE_REPEAT, // D3D12_TEXTURE_ADDRESS_MODE_WRAP
};
typedef uint32_t ZgWrappingMode;

// A struct defining a texture sampler
struct ZgSampler {

	// The sampling mode of the sampler
	ZgSamplingMode samplingMode;

	// The wrapping mode of the sampler (u == x, v == y)
	ZgWrappingMode wrappingModeU;
	ZgWrappingMode wrappingModeV;

	// Offset from the calculated mipmap level. E.g., if mipmap level 2 is calculated in the shader
	// and the lod bias is -1, then level 1 will be used instead. Level 0 is the highest resolution
	// texture.
	float mipLodBias;
};
typedef struct ZgSampler ZgSampler;

// The common information required to create a rendering pipeline
struct ZgPipelineRenderingCreateInfoCommon {

	// The names of the entry functions
	const char* vertexShaderEntry;
	const char* pixelShaderEntry;

	// The vertex attributes to the vertex shader
	uint32_t numVertexAttributes;
	ZgVertexAttribute vertexAttributes[ZG_MAX_NUM_VERTEX_ATTRIBUTES];

	// The number of vertex buffer slots used by the vertex attributes
	//
	// If only one buffer is used (i.e. array of vertex struct) then numVertexBufferSlots should be
	// 1 and vertexBufferStrides[0] should be sizeof(Vertex) stored in your buffer.
	uint32_t numVertexBufferSlots;
	uint32_t vertexBufferStridesBytes[ZG_MAX_NUM_VERTEX_ATTRIBUTES];

	// A list of constant buffer registers which should be declared as push constants. This is an
	// optimization, however it can lead to worse performance if used improperly. Can be left empty
	// if unsure.
	uint32_t numPushConstants;
	uint32_t pushConstantRegisters[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// A list of samplers used by the pipeline
	//
	// Note: For D3D12 the first sampler in the array (0th) corresponds with the 0th sampler
	//       register, etc. E.g. meaning if you have three samplers, they need to have the
	//       registers 0, 1, 2.
	uint32_t numSamplers;
	ZgSampler samplers[ZG_MAX_NUM_SAMPLERS];
};
typedef struct ZgPipelineRenderingCreateInfoCommon ZgPipelineRenderingCreateInfoCommon;

ZG_API ZgErrorCode zgPipelineRenderingRelease(
	ZgPipelineRendering* pipeline);

ZG_API ZgErrorCode zgPipelineRenderingGetSignature(
	const ZgPipelineRendering* pipeline,
	ZgPipelineRenderingSignature* signatureOut);

// Pipeline Rendering - SPIRV
// ------------------------------------------------------------------------------------------------

struct ZgPipelineRenderingCreateInfoFileSPIRV {

	// The common information always needed to create a rendering pipeline
	ZgPipelineRenderingCreateInfoCommon common;

	// Paths to the shader files
	const char* vertexShaderPath;
	const char* pixelShaderPath;
};
typedef struct ZgPipelineRenderingCreateInfoFileSPIRV ZgPipelineRenderingCreateInfoFileSPIRV;

ZG_API ZgErrorCode zgPipelineRenderingCreateFromFileSPIRV(
	ZgPipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfoFileSPIRV* createInfo);

// Pipeline Rendering - HLSL
// ------------------------------------------------------------------------------------------------

// Enum representing various shader model versions
enum ZgShaderModelEnum {
	ZG_SHADER_MODEL_UNDEFINED = 0,
	ZG_SHADER_MODEL_6_0,
	ZG_SHADER_MODEL_6_1,
	ZG_SHADER_MODEL_6_2,
	ZG_SHADER_MODEL_6_3
};
typedef uint32_t ZgShaderModel;

// The maximum number of compiler flags allowed to the DXC shader compiler
static const uint32_t ZG_MAX_NUM_DXC_COMPILER_FLAGS = 8;

struct ZgPipelineRenderingCreateInfoFileHLSL {

	// The common information always needed to create a rendering pipeline
	ZgPipelineRenderingCreateInfoCommon common;

	// Paths to the shader files
	const char* vertexShaderPath;
	const char* pixelShaderPath;

	// Information to the DXC compiler
	ZgShaderModel shaderModel;
	const char* dxcCompilerFlags[ZG_MAX_NUM_DXC_COMPILER_FLAGS];
};
typedef struct ZgPipelineRenderingCreateInfoFileHLSL ZgPipelineRenderingCreateInfoFileHLSL;

ZG_API ZgErrorCode zgPipelineRenderingCreateFromFileHLSL(
	ZgPipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfoFileHLSL* createInfo);

struct ZgPipelineRenderingCreateInfoSourceHLSL {

	// The common information always needed to create a rendering pipeline
	ZgPipelineRenderingCreateInfoCommon common;

	// Shader sources
	const char* vertexShaderSrc;
	const char* pixelShaderSrc;

	// Information to the DXC compiler
	ZgShaderModel shaderModel;
	const char* dxcCompilerFlags[ZG_MAX_NUM_DXC_COMPILER_FLAGS];
};
typedef struct ZgPipelineRenderingCreateInfoSourceHLSL ZgPipelineRenderingCreateInfoSourceHLSL;

ZG_API ZgErrorCode zgPipelineRenderingCreateFromSourceHLSL(
	ZgPipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfoSourceHLSL* createInfo);

// Memory
// ------------------------------------------------------------------------------------------------

enum ZgMemoryTypeEnum {
	ZG_MEMORY_TYPE_UNDEFINED = 0,

	// Memory suitable for uploading data to GPU.
	// Can not be used as a shader UAV, only as vertex shader input.
	ZG_MEMORY_TYPE_UPLOAD,

	// Memory suitable for downloading data from GPU.
	ZG_MEMORY_TYPE_DOWNLOAD,

	// Fastest memory available on GPU.
	// Can't upload or download directly to this memory from CPU, need to use UPLOAD and DOWNLOAD
	// as intermediary.
	ZG_MEMORY_TYPE_DEVICE
};
typedef uint32_t ZgMemoryType;

struct ZgMemoryHeapCreateInfo {

	// The size in bytes of the heap
	uint64_t sizeInBytes;

	// The type of memory
	ZgMemoryType memoryType;
};
typedef struct ZgMemoryHeapCreateInfo ZgMemoryHeapCreateInfo;

ZG_API ZgErrorCode zgMemoryHeapCreate(
	ZgMemoryHeap** memoryHeapOut,
	const ZgMemoryHeapCreateInfo* createInfo);

ZG_API ZgErrorCode zgMemoryHeapRelease(
	ZgMemoryHeap* memoryHeap);

struct ZgBufferCreateInfo {
	
	// The offset from the start of the memory heap to create the buffer at.
	// Note that the offset must be a multiple of 64KiB (= 2^16 bytes = 65 536 bytes), or 0.
	uint64_t offsetInBytes;

	// The size in bytes of the buffer
	uint64_t sizeInBytes;
};
typedef struct ZgBufferCreateInfo ZgBufferCreateInfo;

ZG_API ZgErrorCode zgMemoryHeapBufferCreate(
	ZgMemoryHeap* memoryHeap,
	ZgBuffer** bufferOut,
	const ZgBufferCreateInfo* createInfo);

ZG_API void zgBufferRelease(
	ZgBuffer* buffer);

ZG_API ZgErrorCode zgBufferMemcpyTo(
	ZgBuffer* dstBuffer,
	uint64_t bufferOffsetBytes,
	const void* srcMemory,
	uint64_t numBytes);

// Textures
// ------------------------------------------------------------------------------------------------

struct ZgTextureHeapCreateInfo {

	// The size in bytes of the heap
	uint64_t sizeInBytes;
};
typedef struct ZgTextureHeapCreateInfo ZgTextureHeapCreateInfo;

ZG_API ZgErrorCode zgTextureHeapCreate(
	ZgTextureHeap** textureHeapOut,
	const ZgTextureHeapCreateInfo* createInfo);

ZG_API ZgErrorCode zgTextureHeapRelease(
	ZgTextureHeap* textureHeap);

enum ZgTexture2DFormatEnum {
	ZG_TEXTURE_2D_FORMAT_UNDEFINED = 0,

	ZG_TEXTURE_2D_FORMAT_R_U8,
	ZG_TEXTURE_2D_FORMAT_RG_U8,
	ZG_TEXTURE_2D_FORMAT_RGBA_U8
};
typedef uint32_t ZgTexture2DFormat;

static const uint32_t ZG_TEXTURE_2D_MAX_NUM_MIPMAPS = 12;

struct ZgTexture2DCreateInfo {

	// The format of the texture
	ZgTexture2DFormat format;

	// Whether the texture is normalized or not.
	//
	// A normalized texture will normalize the contained value to a range of [0.0, 1.0] when read
	// in the shader.
	ZgBool normalized;

	// The dimensions of the texture
	uint32_t width;
	uint32_t height;

	// The number of mipmaps
	//
	// 1 equals no mipmaps, i.e. only a single layer. May not be 0, must be smaller than or equal
	// to ZG_TEXTURE_2D_MAX_NUM_MIPMAPS.
	uint32_t numMipmaps;
	
	// The offset from the start of the texture heap to create the buffer at.
	// Note that the offset must be a multiple of the alignment of the texture, which can be
	// acquired by zgTextureHeapTexture2DGetAllocationInfo(). Do not need to be set before calling
	// this function.
	uint64_t offsetInBytes;

	// The size in bytes of the texture
	// Note that this can only be learned by calling zgTexture2DGetAllocationInfo(). Do not need
	// to be set before calling this function.
	uint64_t sizeInBytes;
};

struct ZgTexture2DAllocationInfo {
	
	// The size of the texture in bytes
	uint32_t sizeInBytes;
	
	// The alignment of the texture in bytes
	uint32_t alignmentInBytes;
};

// Gets the allocation info of a Texture2D specified by a ZgTexture2DCreateInfo.
ZG_API ZgErrorCode zgTexture2DGetAllocationInfo(
	ZgTexture2DAllocationInfo* allocationInfoOut,
	const ZgTexture2DCreateInfo* createInfo);

ZG_API ZgErrorCode zgTextureHeapTexture2DCreate(
	ZgTextureHeap* textureHeap,
	ZgTexture2D** textureOut,
	const ZgTexture2DCreateInfo* createInfo);

ZG_API void zgTexture2DRelease(
	ZgTexture2D* texture);

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgCommandQueueFlush(
	ZgCommandQueue* commandQueue);

ZG_API ZgErrorCode zgCommandQueueBeginCommandListRecording(
	ZgCommandQueue* commandQueue,
	ZgCommandList** commandListOut);

ZG_API ZgErrorCode zgCommandQueueExecuteCommandList(
	ZgCommandQueue* commandQueue,
	ZgCommandList* commandList);

// Command list
// ------------------------------------------------------------------------------------------------

ZG_API ZgErrorCode zgCommandListMemcpyBufferToBuffer(
	ZgCommandList* commandList,
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes);

struct ZgImageViewConstCpu {

	ZgTexture2DFormat format;
	const uint8_t* data;
	uint32_t width;
	uint32_t height;
	uint32_t pitchInBytes;
};

// Copies an image from the CPU to a texture on the GPU.
//
// The CPU image (srcImageCpu) is first (synchronously) copied to a temporary upload buffer
// (tempUploadBuffer), and then asynchronously copied from this upload buffer to the specified
// texture (dstTexture). In other words, the CPU memory containing the image can freely be removed
// after this call. The temporary upload buffer must not be touched until this command list has
// finished executing.
ZG_API ZgErrorCode zgCommandListMemcpyToTexture(
	ZgCommandList* commandList,
	ZgTexture2D* dstTexture,
	uint32_t dstTextureMipLevel,
	const ZgImageViewConstCpu* srcImageCpu,
	ZgBuffer* tempUploadBuffer);

ZG_API ZgErrorCode zgCommandListSetPushConstant(
	ZgCommandList* commandList,
	uint32_t shaderRegister,
	const void* data,
	uint32_t dataSizeInBytes);

struct ZgConstantBufferBinding {
	uint32_t shaderRegister;
	ZgBuffer* buffer;
};
typedef struct ZgConstantBufferBinding ZgConstantBufferBinding;

struct ZgTextureBinding {
	uint32_t textureRegister;
	ZgTexture2D* texture;
};

struct ZgPipelineBindings {

	// The constant buffers to bind
	uint32_t numConstantBuffers;
	ZgConstantBufferBinding constantBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// The textures to bind
	uint32_t numTextures;
	ZgTextureBinding textures[ZG_MAX_NUM_TEXTURES];
};
typedef struct ZgPipelineBindings ZgPipelineBindings;

ZG_API ZgErrorCode zgCommandListSetPipelineBindings(
	ZgCommandList* commandList,
	const ZgPipelineBindings* bindings);

ZG_API ZgErrorCode zgCommandListSetPipelineRendering(
	ZgCommandList* commandList,
	ZgPipelineRendering* pipeline);

// The viewport and scissor are optional, if nullptr they will cover the entire framebuffer
ZG_API ZgErrorCode zgCommandListSetFramebuffer(
	ZgCommandList* commandList,
	ZgFramebuffer* framebuffer,
	const ZgFramebufferRect* optionalViewport,
	const ZgFramebufferRect* optionalScissor);

ZG_API ZgErrorCode zgCommandListClearFramebuffer(
	ZgCommandList* commandList,
	float red,
	float green,
	float blue,
	float alpha);

ZG_API ZgErrorCode zgCommandListClearDepthBuffer(
	ZgCommandList* commandList,
	float depth);

enum ZgIndexBufferTypeEnum {
	ZG_INDEX_BUFFER_TYPE_UINT32 = 0,
	ZG_INDEX_BUFFER_TYPE_UINT16
};
typedef uint32_t ZgIndexBufferType;

ZG_API ZgErrorCode zgCommandListSetIndexBuffer(
	ZgCommandList* commandList,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type);

ZG_API ZgErrorCode zgCommandListSetVertexBuffer(
	ZgCommandList* commandList,
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBuffer);

ZG_API ZgErrorCode zgCommandListDrawTriangles(
	ZgCommandList* commandList,
	uint32_t startVertexIndex,
	uint32_t numVertices);

ZG_API ZgErrorCode zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandList,
	uint32_t startIndex,
	uint32_t numTriangles);

// This entire header is pure C
#ifdef __cplusplus
} // extern "C"
#endif
