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

// A handle representing a compute pipeline
ZG_HANDLE(ZgPipelineCompute);

// A handle representing a render pipeline
ZG_HANDLE(ZgPipelineRender);

// A handle representing a memory heap (to allocate buffers and textures from)
ZG_HANDLE(ZgMemoryHeap);

// A handle representing a buffer
ZG_HANDLE(ZgBuffer);

// A handle representing a 2-dimensional texture
ZG_HANDLE(ZgTexture2D);

// A handle representing a framebuffer
ZG_HANDLE(ZgFramebuffer);

// A handle representing a fence for synchronizing command queues
ZG_HANDLE(ZgFence);

// A handle representing a command queue
ZG_HANDLE(ZgCommandQueue);

// A handle representing a command list
ZG_HANDLE(ZgCommandList);

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
static const uint32_t ZG_COMPILED_API_VERSION = 10;

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

	// The Vulkan backend, available on all platforms.
	ZG_BACKEND_VULKAN
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
	ZG_FEATURE_BIT_BACKEND_VULKAN = 1 << 2
};
typedef uint64_t ZgFeatureBits;

// Returns a bitmask containing the features compiled into this ZeroG dll.
ZG_API ZgFeatureBits zgCompiledFeatures(void);

// Results
// ------------------------------------------------------------------------------------------------

// The results
//
// 0 is success, negative values are errors and positive values are warnings.
enum ZgResultEnum {

	// Success (0)
	ZG_SUCCESS = 0,

	// Warnings (>0)
	ZG_WARNING_GENERIC = 1,
	ZG_WARNING_UNIMPLEMENTED = 2,
	ZG_WARNING_ALREADY_INITIALIZED = 3,

	// Errors (<0)
	ZG_ERROR_GENERIC = -1,
	ZG_ERROR_CPU_OUT_OF_MEMORY = -2,
	ZG_ERROR_GPU_OUT_OF_MEMORY = -3,
	ZG_ERROR_NO_SUITABLE_DEVICE = -4,
	ZG_ERROR_INVALID_ARGUMENT = -5,
	ZG_ERROR_SHADER_COMPILE_ERROR = -6,
	ZG_ERROR_OUT_OF_COMMAND_LISTS = -7,
	ZG_ERROR_INVALID_COMMAND_LIST_STATE = -8
};
typedef int32_t ZgResult;

// Returns a string representation of the given ZeroG result. The string is statically allocated
// and must NOT be freed by the user.
ZG_API const char* zgResultToString(ZgResult errorCode);

// Logging interface
// ------------------------------------------------------------------------------------------------

enum ZgLogLevelEnum {
	ZG_LOG_LEVEL_NOISE = 0,
	ZG_LOG_LEVEL_INFO,
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

	// Function pointer to allocate function. The allocation created must be 32-byte aligned. name,
	// file and line is (statically allocated) debug information related to the allocation.
	void* (*allocate)(void* userPtr, uint32_t size, const char* name, const char* file, uint32_t line);

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

	// [Mandatory] Platform specific native handle.
	//
	// On Windows, this is a HWND, i.e. native window handle.
	void* nativeHandle;
};
typedef struct ZgContextInitSettings ZgContextInitSettings;

// Checks if the implicit ZeroG context is already initialized or not
ZG_API ZgBool zgContextAlreadyInitialized(void);

// Initializes the implicit ZeroG context, will fail if a context is already initialized
ZG_API ZgResult zgContextInit(const ZgContextInitSettings* initSettings);

// Deinitializes the implicit ZeroG context
//
// Completely safe to call even if no context has been created
ZG_API ZgResult zgContextDeinit(void);

// Resize the back buffers in the swap chain to the new size.
//
// This should be called every time the size of the window or the resolution is changed. This
// function is guaranteed to not do anything if the specified width or height is the same as last
// time, so it is completely safe to call this at the beginning of each frame.
//
// Note: MUST be called before zgContextSwapchainBeginFrame() or after zgContextSwapchainFinishFrame(),
//       may NOT be called in-between.
ZG_API ZgResult zgContextSwapchainResize(
	uint32_t width,
	uint32_t height);

// The framebuffer returned is owned by the swapchain and can't be released by the user. It is
// still safe to call zgFramebufferRelease() on it, but it will be a no-op and the framebuffer
// will still be valid afterwards.
ZG_API ZgResult zgContextSwapchainBeginFrame(
	ZgFramebuffer** framebufferOut);

ZG_API ZgResult zgContextSwapchainFinishFrame(void);

// Statistics
// ------------------------------------------------------------------------------------------------

// A struct containing general statistics
struct ZgStats {

	// Text description (i.e. name) of the device in use
	char deviceDescription[128];

	// Total amount of dedicated GPU memory (in bytes) available on the GPU
	uint64_t dedicatedGpuMemoryBytes;

	// Total amount of dedicated CPU memory (in bytes) for this GPU. I.e. CPU memory dedicated for
	// the GPU and not directly usable by the CPU.
	uint64_t dedicatedCpuMemoryBytes;

	// Total amount of shared CPU memory (in bytes) for this GPU. I.e. CPU memory shared between
	// the CPU and GPU.
	uint64_t sharedCpuMemoryBytes;

	// The amount of "fast local" memory provided to the application by the OS. If more memory is
	// used then stuttering and swapping could occur among other things.
	uint64_t memoryBudgetBytes;

	// The amount of "fast local" memory used by the application. This will correspond to GPU
	// memory on a device with dedicated GPU memory.
	uint64_t memoryUsageBytes;

	// The amount of "non-local" memory provided to the application by the OS.
	uint64_t nonLocalBugetBytes;

	// The amount of "non-local" memory used by the application.
	uint64_t nonLocalUsageBytes;
};
typedef struct ZgStats ZgStats;

// Gets the current statistics of the ZeroG backend. Normally called once (or maybe up to a couple
// of times) per frame.
ZG_API ZgResult zgContextGetStats(ZgStats* statsOut);

// Texture formats
// ------------------------------------------------------------------------------------------------

enum ZgTextureFormatEnum {
	ZG_TEXTURE_FORMAT_UNDEFINED = 0,

	ZG_TEXTURE_FORMAT_R_U8_UNORM, // Normalized between [0, 1]
	ZG_TEXTURE_FORMAT_RG_U8_UNORM, // Normalized between [0, 1]
	ZG_TEXTURE_FORMAT_RGBA_U8_UNORM, // Normalized between [0, 1]

	ZG_TEXTURE_FORMAT_R_F16,
	ZG_TEXTURE_FORMAT_RG_F16,
	ZG_TEXTURE_FORMAT_RGBA_F16,

	ZG_TEXTURE_FORMAT_R_F32,
	ZG_TEXTURE_FORMAT_RG_F32,
	ZG_TEXTURE_FORMAT_RGBA_F32,

	ZG_TEXTURE_FORMAT_DEPTH_F32
};
typedef uint32_t ZgTextureFormat;

// Pipeline Bindings
// ------------------------------------------------------------------------------------------------

// The maximum number of constant buffers allowed on a single pipeline
static const uint32_t ZG_MAX_NUM_CONSTANT_BUFFERS = 16;

// The maximum number of textures allowed on a single pipeline
static const uint32_t ZG_MAX_NUM_TEXTURES = 16;

// The maximum number of samplers allowed on a single pipeline
static const uint32_t ZG_MAX_NUM_SAMPLERS = 8;

struct ZgConstantBufferBindingDesc {

	// Which register this buffer corresponds to in the shader. In D3D12 this is the "register"
	// keyword, i.e. a value of 0 would mean "register(b0)".
	uint32_t shaderRegister;

	// Size of the buffer in bytes
	uint32_t sizeInBytes;

	// Whether the buffer is a push constant or not
	//
	// The size of a push constant must be a multiple of 4 bytes, i.e. an even (32-bit) word.
	//
	// In D3D12, a push constant is stored directly in the root signature. Note that the maximum
	// size of a root signature in D3D12 is 64 32-bit words. Therefore ZeroG imposes an limit of a
	// maximum size of 32 32-bit words (128 bytes) per push constant. Microsoft recommends keeping
	// the root signature smaller than 16 words to maximize performance on some hardware.
	ZgBool pushConstant;
};
typedef struct ZgConstantBufferBindingDesc ZgConstantBufferBindingDesc;

struct ZgTextureBindingDesc {

	// Which register this texture corresponds to in the shader.
	uint32_t textureRegister;
};
typedef struct ZgTextureBindingDesc ZgTextureBindingDesc;

// A struct representing the signature of a pipeline, indicating what resources can be bound to it.
//
// The signature contains all information necessary to know how to bind input and output to a
// pipeline. This information is inferred by performing reflection on the shaders being compiled.
struct ZgPipelineBindingsSignature {

	// The constant buffers
	uint32_t numConstantBuffers;
	ZgConstantBufferBindingDesc constantBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// The textures
	uint32_t numTextures;
	ZgTextureBindingDesc textures[ZG_MAX_NUM_TEXTURES];
};
typedef struct ZgPipelineBindingsSignature ZgPipelineBindingsSignature;

struct ZgConstantBufferBinding {
	uint32_t shaderRegister;
	ZgBuffer* buffer;
};
typedef struct ZgConstantBufferBinding ZgConstantBufferBinding;

struct ZgTextureBinding {
	uint32_t textureRegister;
	ZgTexture2D* texture;
};
typedef struct ZgTextureBinding ZgTextureBinding;

struct ZgPipelineBindings {

	// The constant buffers to bind
	uint32_t numConstantBuffers;
	ZgConstantBufferBinding constantBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// The textures to bind
	uint32_t numTextures;
	ZgTextureBinding textures[ZG_MAX_NUM_TEXTURES];
};
typedef struct ZgPipelineBindings ZgPipelineBindings;

// Pipeline Compiler Settings
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

// Compile settings to the HLSL compiler
struct ZgPipelineCompileSettingsHLSL {

	// Which shader model to target when compiling the HLSL file
	ZgShaderModel shaderModel;

	// Flags to the DXC compiler
	const char* dxcCompilerFlags[ZG_MAX_NUM_DXC_COMPILER_FLAGS];
};
typedef struct ZgPipelineCompileSettingsHLSL ZgPipelineCompileSettingsHLSL;


// Pipeline Compute
// ------------------------------------------------------------------------------------------------

struct ZgPipelineComputeCreateInfo {

	// Path to the shader source or the source directly depending on what create function is called.
	const char* computeShader;

	// The name of the entry function
	const char* computeShaderEntry;

};

ZG_API ZgResult zgPipelineComputeCreateFromFileHLSL(
	ZgPipelineCompute** pipelineOut,
	const ZgPipelineComputeCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings);

ZG_API ZgResult zgPipelineComputeRelease(
	ZgPipelineCompute* pipeline);








// Pipeline Render Signature
// ------------------------------------------------------------------------------------------------

// The maximum number of vertex attributes allowed as input to a vertex shader
static const uint32_t ZG_MAX_NUM_VERTEX_ATTRIBUTES = 8;

// The maximum number of render targets allowed on a single pipeline
static const uint32_t ZG_MAX_NUM_RENDER_TARGETS = 8;

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

// A struct representing the rendering signature of a render pipeline.
//
// In contrast to ZgPipelineBindingsSignature, this specifies render pipeline specific bendings of
// a render pipeline. I.e. how vertex data is loaded and render targets being written to.
//
// Unlike ZgPipelineBindingsSignature, this data can not be inferred by reflection and is specified
// by the user when creating a pipeline anyway. It is mainly kept and provided in this form to make
// life a bit simpler for the user.
struct ZgPipelineRenderSignature {

	// The vertex attributes to the vertex shader
	uint32_t numVertexAttributes;
	ZgVertexAttribute vertexAttributes[ZG_MAX_NUM_VERTEX_ATTRIBUTES];

	// Render targets
	uint32_t numRenderTargets;
	ZgTextureFormat renderTargets[ZG_MAX_NUM_RENDER_TARGETS];
};
typedef struct ZgPipelineRenderSignature ZgPipelineRenderSignature;

// Pipeline Render
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

struct ZgRasterizerSettings {

	// Renders in wireframe mode instead of solid mode, i.e. only lines between vertices.
	ZgBool wireframeMode;

	// Whether to enable culling of front (or back) facing triangles
	ZgBool cullingEnabled;

	// Whether to cull front or back-facing triangles. Default should be ZG_FALSE, i.e. cull
	// back-facing triangles if culling is enabled.
	ZgBool cullFrontFacing;

	// Which winding order of a triangle is considered front-facing.
	//
	// Default is ZG_FALSE, in other words clockwise (left-hand rule). This is also the default
	// of D3D12.
	ZgBool frontFacingIsCounterClockwise;

	// Depth bias added to each pixel.
	//
	// For D3D12, see: https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
	int32_t depthBias;

	// Depth bias for each pixel's slope.
	//
	// For D3D12, see: https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
	float depthBiasSlopeScaled;

	// The max depth bias for a pixel.
	//
	// For D3D12, see: https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
	float depthBiasClamp;
};
typedef struct ZgRasterizerSettings ZgRasterizerSettings;

enum ZgBlendFuncEnum {
	ZG_BLEND_FUNC_ADD = 0,
	ZG_BLEND_FUNC_DST_SUB_SRC, // dst - src
	ZG_BLEND_FUNC_SRC_SUB_DST, // src - dst
	ZG_BLEND_FUNC_MIN,
	ZG_BLEND_FUNC_MAX
};
typedef uint32_t ZgBlendFunc;

// See: https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_blend
enum ZgBlendFactorEnum {
	ZG_BLEND_FACTOR_ZERO = 0,
	ZG_BLEND_FACTOR_ONE,
	ZG_BLEND_FACTOR_SRC_COLOR,
	ZG_BLEND_FACTOR_SRC_INV_COLOR,
	ZG_BLEND_FACTOR_SRC_ALPHA,
	ZG_BLEND_FACTOR_SRC_INV_ALPHA,
	ZG_BLEND_FACTOR_DST_COLOR,
	ZG_BLEND_FACTOR_DST_INV_COLOR,
	ZG_BLEND_FACTOR_DST_ALPHA,
	ZG_BLEND_FACTOR_DST_INV_ALPHA,
};
typedef uint32_t ZgBlendFactor;

struct ZgBlendSettings {

	// Whether to enable blending or not
	ZgBool blendingEnabled;

	// What blend function to use for color and alpha respectively
	ZgBlendFunc blendFuncColor;
	ZgBlendFunc blendFuncAlpha;

	// How the color values in src (output from pixel shader) and dst (value in framebuffer)
	// should be blended.
	ZgBlendFactor srcValColor;
	ZgBlendFactor dstValColor;

	// How the alpha values in src (output from pixel shader) and dst (value in framebuffer)
	// should be blended.
	ZgBlendFactor srcValAlpha;
	ZgBlendFactor dstValAlpha;
};
typedef struct ZgBlendSettings ZgBlendSettings;

enum ZgDepthFuncEnum {
	ZG_DEPTH_FUNC_LESS = 0,
	ZG_DEPTH_FUNC_LESS_EQUAL,
	ZG_DEPTH_FUNC_EQUAL,
	ZG_DEPTH_FUNC_NOT_EQUAL,
	ZG_DEPTH_FUNC_GREATER,
	ZG_DEPTH_FUNC_GREATER_EQUAL
};
typedef uint32_t ZgDepthFunc;

struct ZgDepthTestSettings {

	// Whether to enable to the depth test or not
	ZgBool depthTestEnabled;

	// What function to use for the depth test.
	//
	// Default is ZG_DEPTH_FUNC_LESS (0), i.e. pixels with a depth value less than what is stored
	// in the depth buffer is kept (in a "standard" 0 is near, 1 is furthest away depth buffer).
	ZgDepthFunc depthFunc;
};
typedef struct ZgDepthTestSettings ZgDepthTestSettings;

// The common information required to create a render pipeline
struct ZgPipelineRenderCreateInfo {

	// Path to the shader source or the source directly depending on what create function is called.
	const char* vertexShader;
	const char* pixelShader;

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

	// A list of render targets used by the pipeline
	uint32_t numRenderTargets;
	ZgTextureFormat renderTargets[ZG_MAX_NUM_RENDER_TARGETS];

	// Rasterizer settings
	ZgRasterizerSettings rasterizer;

	// Blend test settings
	ZgBlendSettings blending;

	// Depth test settings
	ZgDepthTestSettings depthTest;
};
typedef struct ZgPipelineRenderCreateInfo ZgPipelineRenderCreateInfo;

ZG_API ZgResult zgPipelineRenderCreateFromFileSPIRV(
	ZgPipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo* createInfo);

ZG_API ZgResult zgPipelineRenderCreateFromFileHLSL(
	ZgPipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings);

ZG_API ZgResult zgPipelineRenderCreateFromSourceHLSL(
	ZgPipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings);

ZG_API ZgResult zgPipelineRenderRelease(
	ZgPipelineRender* pipeline);

// Memory Heap
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
	ZG_MEMORY_TYPE_DEVICE,

	// Special version of ZG_MEMORY_TYPE_DEVICE that can be used to allocate textures.
	//
	// Some GPUs can allocate textures directly from ZG_MEMORY_TYPE_DEVICE, making it unnecessary
	// to create memory heaps of this type.
	// TODO: Implement support for this use case.
	ZG_MEMORY_TYPE_TEXTURE,

	// Special version of ZG_MEMORY_TYPE_DEVICE that can be used to allocate textures that can be
	// written to as framebuffer targets.
	//
	// Some GPUs can allocate framebuffer textures directly from ZG_MEMORY_TYPE_DEVICE, making it
	// unnecessary to create memory heaps of this type.
	// TODO: Implement support for this use case.
	ZG_MEMORY_TYPE_FRAMEBUFFER
};
typedef uint32_t ZgMemoryType;

struct ZgMemoryHeapCreateInfo {

	// The size in bytes of the heap
	uint64_t sizeInBytes;

	// The type of memory
	ZgMemoryType memoryType;
};
typedef struct ZgMemoryHeapCreateInfo ZgMemoryHeapCreateInfo;

ZG_API ZgResult zgMemoryHeapCreate(
	ZgMemoryHeap** memoryHeapOut,
	const ZgMemoryHeapCreateInfo* createInfo);

ZG_API ZgResult zgMemoryHeapRelease(
	ZgMemoryHeap* memoryHeap);

// Buffer
// ------------------------------------------------------------------------------------------------

struct ZgBufferCreateInfo {

	// The offset from the start of the memory heap to create the buffer at.
	// Note that the offset must be a multiple of 64KiB (= 2^16 bytes = 65 536 bytes), or 0.
	uint64_t offsetInBytes;

	// The size in bytes of the buffer
	uint64_t sizeInBytes;
};
typedef struct ZgBufferCreateInfo ZgBufferCreateInfo;

ZG_API ZgResult zgMemoryHeapBufferCreate(
	ZgMemoryHeap* memoryHeap,
	ZgBuffer** bufferOut,
	const ZgBufferCreateInfo* createInfo);

ZG_API void zgBufferRelease(
	ZgBuffer* buffer);

ZG_API ZgResult zgBufferMemcpyTo(
	ZgBuffer* dstBuffer,
	uint64_t bufferOffsetBytes,
	const void* srcMemory,
	uint64_t numBytes);

ZG_API ZgResult zgBufferSetDebugName(
	ZgBuffer* buffer,
	const char* name);

// Textures
// ------------------------------------------------------------------------------------------------

static const uint32_t ZG_MAX_NUM_MIPMAPS = 12;

enum ZgTextureUsageEnum {
	ZG_TEXTURE_USAGE_DEFAULT = 0,
	ZG_TEXTURE_USAGE_RENDER_TARGET,
	ZG_TEXTURE_USAGE_DEPTH_BUFFER
};
typedef uint32_t ZgTextureUsage;

enum ZgOptimalClearValueEnum {
	ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED = 0,
	ZG_OPTIMAL_CLEAR_VALUE_ZERO,
	ZG_OPTIMAL_CLEAR_VALUE_ONE
};
typedef uint32_t ZgOptimalClearValue;

struct ZgTexture2DCreateInfo {

	// The format of the texture
	ZgTextureFormat format;

	// How the texture will be used.
	//
	// If the texture is to be used as either a render target or a depth buffer it must be set
	// here.
	ZgTextureUsage usage;

	// The optimal clear value of this texture.
	//
	// This may only be set when creating a texture with usage RENDER_TARGET or DEPTH_BUFFER.
	// Otherwise it should be left to the default value (UNDEFINED).
	ZgOptimalClearValue optimalClearValue;

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
typedef struct ZgTexture2DCreateInfo ZgTexture2DCreateInfo;

struct ZgTexture2DAllocationInfo {

	// The size of the texture in bytes
	uint32_t sizeInBytes;

	// The alignment of the texture in bytes
	uint32_t alignmentInBytes;
};
typedef struct ZgTexture2DAllocationInfo ZgTexture2DAllocationInfo;

// Gets the allocation info of a Texture2D specified by a ZgTexture2DCreateInfo.
ZG_API ZgResult zgTexture2DGetAllocationInfo(
	ZgTexture2DAllocationInfo* allocationInfoOut,
	const ZgTexture2DCreateInfo* createInfo);

ZG_API ZgResult zgMemoryHeapTexture2DCreate(
	ZgMemoryHeap* memoryHeap,
	ZgTexture2D** textureOut,
	const ZgTexture2DCreateInfo* createInfo);

ZG_API void zgTexture2DRelease(
	ZgTexture2D* texture);

ZG_API ZgResult zgTexture2DSetDebugName(
	ZgTexture2D* texture,
	const char* name);

// Framebuffer
// ------------------------------------------------------------------------------------------------

struct ZgFramebufferCreateInfo {

	// Render targets
	uint32_t numRenderTargets;
	ZgTexture2D* renderTargets[ZG_MAX_NUM_RENDER_TARGETS];

	// Depth buffer
	ZgTexture2D* depthBuffer;
};
typedef struct ZgFramebufferCreateInfo ZgFramebufferCreateInfo;

ZG_API ZgResult zgFramebufferCreate(
	ZgFramebuffer** framebufferOut,
	const ZgFramebufferCreateInfo* createInfo);

// No-op if framebuffer is built-in swapchain framebuffer, which is managed and owned by the ZeroG
// context.
ZG_API void zgFramebufferRelease(
	ZgFramebuffer* framebuffer);

ZG_API ZgResult zgFramebufferGetResolution(
	const ZgFramebuffer* framebuffer,
	uint32_t* widthOut,
	uint32_t* heightOut);

// Fence
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgFenceCreate(
	ZgFence** fenceOut);

ZG_API void zgFenceRelease(
	ZgFence* fence);

// Resets a ZgFence to its initial state.
//
// Completely optional to call, zgCommandQueueSignalOnGpu() will performing an implicit reset by
// modifying the fence anyway. But can be useful for debugging.
ZG_API ZgResult zgFenceReset(
	ZgFence* fence);

ZG_API ZgResult zgFenceCheckIfSignaled(
	const ZgFence* fence,
	ZgBool* fenceSignaledOut);

ZG_API ZgResult zgFenceWaitOnCpuBlocking(
	const ZgFence* fence);

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgCommandQueueGetPresentQueue(
	ZgCommandQueue** presentQueueOut);

ZG_API ZgResult zgCommandQueueGetCopyQueue(
	ZgCommandQueue** copyQueueOut);

// Enqueues the command queue to signal the ZgFence (from the GPU).
//
// Note: This operation will reset the ZgFence, it is important that nothing is waiting (either
//       the CPU using zgFenceWaitOnCpuBlocking() or the GPU using zgCommandQueueWaitOnGpu()) on
//       this fence. Doing so is undefined behavior. Best case is that someone waits longer than
//       they have to, worst case is hard-lock. In other words, you will likely need more than one
//       fence (think double or triple-buffering) unless you are explicitly flushing each frame.
ZG_API ZgResult zgCommandQueueSignalOnGpu(
	ZgCommandQueue* commandQueue,
	ZgFence* fenceToSignal);

ZG_API ZgResult zgCommandQueueWaitOnGpu(
	ZgCommandQueue* commandQueue,
	const ZgFence* fence);

ZG_API ZgResult zgCommandQueueFlush(
	ZgCommandQueue* commandQueue);

ZG_API ZgResult zgCommandQueueBeginCommandListRecording(
	ZgCommandQueue* commandQueue,
	ZgCommandList** commandListOut);

ZG_API ZgResult zgCommandQueueExecuteCommandList(
	ZgCommandQueue* commandQueue,
	ZgCommandList* commandList);

// Command list
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgCommandListMemcpyBufferToBuffer(
	ZgCommandList* commandList,
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes);

struct ZgImageViewConstCpu {

	ZgTextureFormat format;
	const void* data;
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
ZG_API ZgResult zgCommandListMemcpyToTexture(
	ZgCommandList* commandList,
	ZgTexture2D* dstTexture,
	uint32_t dstTextureMipLevel,
	const ZgImageViewConstCpu* srcImageCpu,
	ZgBuffer* tempUploadBuffer);

// Transitions the specified buffer from copy queue -> other queues and vice versa.
//
// In order to switch a resource from usage on a e.g. copy queue to a graphics queue or vice versa
// it must first be transitioned into a common state (D3D12_RESOURCE_STATE_COMMON). This can,
// however, not always be done on the new queue. I.e. a copy queue can not transition most states
// into the common state.
//
// Therefore, this command need to be manually called in the old command queue after it has stopped
// using a resource. The new command queue then need to wait until it has actually been applied
// (i.e. insert a ZgFence and wait for it). Then the new command queue can start using the resource.
ZG_API ZgResult zgCommandListEnableQueueTransitionBuffer(
	ZgCommandList* commandList,
	ZgBuffer* buffer);

// See zgCommandListEnableQueueTransitionBuffer()
ZG_API ZgResult zgCommandListEnableQueueTransitionTexture(
	ZgCommandList* commandList,
	ZgTexture2D* texture);

ZG_API ZgResult zgCommandListSetPushConstant(
	ZgCommandList* commandList,
	uint32_t shaderRegister,
	const void* data,
	uint32_t dataSizeInBytes);

ZG_API ZgResult zgCommandListSetPipelineBindings(
	ZgCommandList* commandList,
	const ZgPipelineBindings* bindings);

ZG_API ZgResult zgCommandListSetPipelineCompute(
	ZgCommandList* commandList,
	ZgPipelineCompute* pipeline);

ZG_API ZgResult zgCommandListSetPipelineRender(
	ZgCommandList* commandList,
	ZgPipelineRender* pipeline);

// The viewport and scissor are optional, if nullptr they will cover the entire framebuffer
ZG_API ZgResult zgCommandListSetFramebuffer(
	ZgCommandList* commandList,
	ZgFramebuffer* framebuffer,
	const ZgFramebufferRect* optionalViewport,
	const ZgFramebufferRect* optionalScissor);

// Change the viewport for an already set framebuffer
ZG_API ZgResult zgCommandListSetFramebufferViewport(
	ZgCommandList* commandList,
	const ZgFramebufferRect* viewport);

// Change the scissor for an already set framebuffer
ZG_API ZgResult zgCommandListSetFramebufferScissor(
	ZgCommandList* commandList,
	const ZgFramebufferRect* scissor);

// Clears all render targets and depth buffers in a framebuffer to their optimal clear values, 0
// if none is specified.
ZG_API ZgResult zgCommandListClearFramebufferOptimal(
	ZgCommandList* commandList);

ZG_API ZgResult zgCommandListClearRenderTargets(
	ZgCommandList* commandList,
	float red,
	float green,
	float blue,
	float alpha);

ZG_API ZgResult zgCommandListClearDepthBuffer(
	ZgCommandList* commandList,
	float depth);

enum ZgIndexBufferTypeEnum {
	ZG_INDEX_BUFFER_TYPE_UINT32 = 0,
	ZG_INDEX_BUFFER_TYPE_UINT16
};
typedef uint32_t ZgIndexBufferType;

ZG_API ZgResult zgCommandListSetIndexBuffer(
	ZgCommandList* commandList,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type);

ZG_API ZgResult zgCommandListSetVertexBuffer(
	ZgCommandList* commandList,
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBuffer);

ZG_API ZgResult zgCommandListDispatchCompute(
	ZgCommandList* commandList,
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ);

ZG_API ZgResult zgCommandListDrawTriangles(
	ZgCommandList* commandList,
	uint32_t startVertexIndex,
	uint32_t numVertices);

ZG_API ZgResult zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandList,
	uint32_t startIndex,
	uint32_t numTriangles);

// This entire header is pure C
#ifdef __cplusplus
} // extern "C"
#endif
