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

#ifndef ZEROG_H
#define ZEROG_H
#pragma once

// About
// ------------------------------------------------------------------------------------------------

// This header file contains the entire ZeroG API. If you are compiling with a C++ compiler you
// will also get the definitions for the lightweight C++ wrapper API (mainly for managing memory
// using RAII).

// Macros and includes
// ------------------------------------------------------------------------------------------------

#include <sfz.h>

#if defined(_WIN32)
#if defined(ZG_DLL_EXPORT)
#define ZG_API SFZ_EXTERN_C __declspec(dllexport)
#else
#define ZG_API SFZ_EXTERN_C __declspec(dllimport)
#endif
#else
#define ZG_API
#endif

// Macro to declare a ZeroG handle. As a user you can never dereference or inspect a ZeroG handle,
// you can only store pointers to them.
#define ZG_HANDLE(name) \
	struct name; \
	typedef struct name name

// ZeroG handles
// ------------------------------------------------------------------------------------------------

ZG_HANDLE(ZgBuffer);
ZG_HANDLE(ZgTexture);
ZG_HANDLE(ZgMemoryHeap);
ZG_HANDLE(ZgPipelineCompute);
ZG_HANDLE(ZgPipelineRender);
ZG_HANDLE(ZgFramebuffer);
ZG_HANDLE(ZgFence);
ZG_HANDLE(ZgProfiler);
ZG_HANDLE(ZgCommandList);
ZG_HANDLE(ZgCommandQueue);

// Bool
// ------------------------------------------------------------------------------------------------

typedef enum {
	ZG_FALSE = 0,
	ZG_TRUE = 1,
	ZG_BOOL_FORCE_I32 = I32_MAX
} ZgBool;

// Version information
// ------------------------------------------------------------------------------------------------

// The API version used to compile ZeroG.
static const u32 ZG_COMPILED_API_VERSION = 39;

// Returns the API version of the ZeroG DLL you have linked with
//
// As long as the DLL has the same API version as the version you compiled with it should be
// compatible.
ZG_API u32 zgApiLinkedVersion(void);

// Backends
// ------------------------------------------------------------------------------------------------

// The various backends supported by ZeroG
typedef enum {
	ZG_BACKEND_NONE = 0,
	ZG_BACKEND_D3D12,
	ZG_BACKEND_VULKAN,
	ZG_BACKEND_FORCE_I32 = I32_MAX
} ZgBackendType;

// Returns the backend compiled into this API
ZG_API ZgBackendType zgBackendCompiledType(void);

// Results
// ------------------------------------------------------------------------------------------------

// The results, 0 is success, negative values are errors and positive values are warnings.
typedef enum SFZ_NODISCARD {
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
	ZG_ERROR_INVALID_COMMAND_LIST_STATE = -8,

	ZG_RESULT_FORCE_I32 = I32_MAX
} ZgResult;

// Returns a string representation of the given ZeroG result. The string is statically allocated
// and must NOT be freed by the user.
ZG_API const char* zgResultToString(ZgResult errorCode);

inline ZgBool zgIsSuccess(ZgResult res) { return res == ZG_SUCCESS ? ZG_TRUE : ZG_FALSE; }
inline ZgBool zgIsWarning(ZgResult res) { return res > 0 ? ZG_TRUE : ZG_FALSE; }
inline ZgBool zgIsError(ZgResult res) { return res < 0 ? ZG_TRUE : ZG_FALSE; }

// Common C++ Wrapper Stuff
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus
namespace zg {

template<typename HandleT>
inline void NoOpDestroyFunc(HandleT*) {}

template<typename HandleT, void DestroyFunc(HandleT*) = NoOpDestroyFunc>
struct ManagedHandle {

	HandleT* handle = nullptr;

	ManagedHandle() noexcept = default;
	ManagedHandle(const ManagedHandle&) = delete;
	ManagedHandle& operator= (const ManagedHandle&) = delete;
	ManagedHandle(ManagedHandle&& other) noexcept { this->swap(other); }
	ManagedHandle& operator= (ManagedHandle&& other) noexcept { this->swap(other); return *this; }
	~ManagedHandle() noexcept { this->destroy(); }

	bool valid() const { return handle != nullptr; }
	void swap(ManagedHandle& o) noexcept { HandleT* tmp = this->handle; this->handle = o.handle; o.handle = tmp; }
	void destroy() noexcept { if (handle != nullptr) { DestroyFunc(handle); } handle = nullptr; }
};

} // namespace zg
#endif

// Buffer
// ------------------------------------------------------------------------------------------------

typedef enum {
	// Fastest memory available on GPU.
	// Can't upload or download directly to this memory from CPU, need to use UPLOAD and DOWNLOAD
	// as intermediary.
	ZG_MEMORY_TYPE_DEVICE = 0,

	// Memory suitable for uploading data to GPU.
	// Can not be used as a shader UAV, only as vertex shader input.
	ZG_MEMORY_TYPE_UPLOAD,

	// Memory suitable for downloading data from GPU.
	ZG_MEMORY_TYPE_DOWNLOAD,

	ZG_MEMORY_TYPE_FORCE_I32 = I32_MAX
} ZgMemoryType;

sfz_struct(ZgBufferDesc) {
	ZgMemoryType memoryType;
	u64 sizeInBytes;
	ZgBool committedAllocation;
	const char* debugName;
};

ZG_API ZgResult zgBufferCreate(
	ZgBuffer** bufferOut,
	const ZgBufferDesc* desc);

ZG_API void zgBufferDestroy(
	ZgBuffer* buffer);

ZG_API ZgResult zgBufferMemcpyUpload(
	ZgBuffer* dstBuffer,
	u64 dstBufferOffsetBytes,
	const void* srcMemory,
	u64 numBytes);

ZG_API ZgResult zgBufferMemcpyDownload(
	void* dstMemory,
	ZgBuffer* srcBuffer,
	u64 srcBufferOffsetBytes,
	u64 numBytes);

#ifdef __cplusplus
namespace zg {

class Buffer final : public ManagedHandle<ZgBuffer, zgBufferDestroy> {
public:
	ZgResult create(
		u64 sizeBytes,
		ZgMemoryType type = ZG_MEMORY_TYPE_DEVICE,
		bool committedAllocation = false,
		const char* debugName = nullptr)
	{
		this->destroy();
		ZgBufferDesc desc = {};
		desc.memoryType = type;
		desc.sizeInBytes = sizeBytes;
		desc.committedAllocation = committedAllocation ? ZG_TRUE : ZG_FALSE;
		desc.debugName = debugName;
		return zgBufferCreate(&handle, &desc);
	}

	ZgResult memcpyUpload(u64 bufferOffsetBytes, const void* srcMemory, u64 numBytes)
	{
		return zgBufferMemcpyUpload(this->handle, bufferOffsetBytes, srcMemory, numBytes);
	}

	ZgResult memcpyDownload(void* dstMemory, u64 srcBufferOffsetBytes, u64 numBytes)
	{
		return zgBufferMemcpyDownload(dstMemory, this->handle, srcBufferOffsetBytes, numBytes);
	}
};

} // namespace zg
#endif

// Textures
// ------------------------------------------------------------------------------------------------

static const u32 ZG_MAX_NUM_MIPMAPS = 12;

typedef enum {
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

	ZG_TEXTURE_FORMAT_DEPTH_F32,

	ZG_TEXTURE_FORMAT_FORCE_I32 = I32_MAX
} ZgTextureFormat;

typedef enum {
	ZG_TEXTURE_USAGE_DEFAULT = 0,
	ZG_TEXTURE_USAGE_RENDER_TARGET,
	ZG_TEXTURE_USAGE_DEPTH_BUFFER,
	ZG_TEXTURE_USAGE_FORCE_I32 = I32_MAX
} ZgTextureUsage;

typedef enum {
	ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED = 0,
	ZG_OPTIMAL_CLEAR_VALUE_ZERO,
	ZG_OPTIMAL_CLEAR_VALUE_ONE,
	ZG_OPTIMAL_CLEAR_VALUE_FORCE_I32 = I32_MAX
} ZgOptimalClearValue;

sfz_struct(ZgTextureDesc) {

	// The format of the texture
	ZgTextureFormat format;

	// Whether this should be a committed allocation (VK_KHR_dedicated_allocation in Vulkan) or not.
	// (Large, such as framebuffers and render targets) committed allocations have better
	// performance on some GPUs.
	ZgBool committedAllocation;

	// Whether it should be allowed to access this texture using an unordered view or not.
	ZgBool allowUnorderedAccess;

	// If the texture is to be used as either a render target or a depth buffer it must be set
	// here.
	ZgTextureUsage usage;

	// The optimal clear value of this texture.
	//
	// This may only be set when creating a texture with usage RENDER_TARGET or DEPTH_BUFFER.
	// Otherwise it should be left to the default value (UNDEFINED).
	ZgOptimalClearValue optimalClearValue;

	// The dimensions of the texture
	u32 width;
	u32 height;

	// The number of mipmaps
	//
	// 1 equals no mipmaps, i.e. only a single layer. May not be 0, must be smaller than or equal
	// to ZG_TEXTURE_2D_MAX_NUM_MIPMAPS.
	u32 numMipmaps;

	const char* debugName;
};

ZG_API ZgResult zgTextureCreate(
	ZgTexture** textureOut,
	const ZgTextureDesc* desc);

ZG_API void zgTextureDestroy(
	ZgTexture* texture);

ZG_API u32 zgTextureSizeInBytes(
	const ZgTexture* texture);

#ifdef __cplusplus
namespace zg {

class Texture final : public ManagedHandle<ZgTexture, zgTextureDestroy> {
public:
	ZgResult create(const ZgTextureDesc& desc)
	{
		this->destroy();
		return zgTextureCreate(&this->handle, &desc);
	}

	u32 sizeInBytes() const { return zgTextureSizeInBytes(this->handle); }
};

} // namespace zg
#endif

// Pipeline Bindings
// ------------------------------------------------------------------------------------------------

// The maximum number of constant buffers allowed on a single pipeline.
static const u32 ZG_MAX_NUM_CONSTANT_BUFFERS = 16;

// The maximum number of unordered buffers allowed on a single pipeline.
static const u32 ZG_MAX_NUM_UNORDERED_BUFFERS = 16;

// The maximum number of textures allowed on a single pipeline.
static const u32 ZG_MAX_NUM_TEXTURES = 16;

// The maximum number of unordered textures on a single pipeline.
static const u32 ZG_MAX_NUM_UNORDERED_TEXTURES = 16;

// The maximum number of samplers allowed on a single pipeline.
static const u32 ZG_MAX_NUM_SAMPLERS = 8;

sfz_struct(ZgConstantBufferBindingDesc) {

	// Which register this buffer corresponds to in the shader. In D3D12 this is the "register"
	// keyword, i.e. a value of 0 would mean "register(b0)".
	u32 bufferRegister;

	// Size of the buffer in bytes
	u32 sizeInBytes;

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

sfz_struct(ZgUnorderedBufferBindingDesc) {

	// Which register this buffer corresponds to in the shader.In D3D12 this is the "register"
	// keyword, i.e. a value of 0 would mean "register(u0)".
	u32 unorderedRegister;
};

sfz_struct(ZgTextureBindingDesc) {

	// Which register this texture corresponds to in the shader.
	u32 textureRegister;
};

sfz_struct(ZgUnorderedTextureBindingDesc) {

	// Which register this texture corresponds to in the shader.
	u32 unorderedRegister;
};

// A struct representing the signature of a pipeline, indicating what resources can be bound to it.
//
// The signature contains all information necessary to know how to bind input and output to a
// pipeline. This information is inferred by performing reflection on the shaders being compiled.
sfz_struct(ZgPipelineBindingsSignature) {

	// The constant buffers
	u32 numConstBuffers;
	ZgConstantBufferBindingDesc constBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// The unordered buffers
	u32 numUnorderedBuffers;
	ZgUnorderedBufferBindingDesc unorderedBuffers[ZG_MAX_NUM_UNORDERED_BUFFERS];

	// The textures
	u32 numTextures;
	ZgTextureBindingDesc textures[ZG_MAX_NUM_TEXTURES];

	// The unordered textures
	u32 numUnorderedTextures;
	ZgUnorderedTextureBindingDesc unorderedTextures[ZG_MAX_NUM_UNORDERED_TEXTURES];
};

sfz_struct(ZgConstantBufferBinding) {
	u32 bufferRegister;
	ZgBuffer* buffer;
};

sfz_struct(ZgUnorderedBufferBinding) {

	// Register the unordered buffer is bound to
	u32 unorderedRegister;

	// The first element in the buffer (0 to bind from start of buffer)
	u32 firstElementIdx;

	// The number of elements to bind in the buffer
	u32 numElements;

	// The stride (size most of the time) between elements in the buffer in bytes
	u32 elementStrideBytes;

	// The buffer to bind as an unordered buffer
	ZgBuffer* buffer;
};

sfz_struct(ZgTextureBinding) {
	u32 textureRegister;
	ZgTexture* texture;
};

sfz_struct(ZgUnorderedTextureBinding) {

	// Register the unordered texture is bound to
	u32 unorderedRegister;

	// The mip level to bind
	u32 mipLevel;

	// The texture to bind as an unordered texture
	ZgTexture* texture;
};

sfz_struct(ZgPipelineBindings) {

	// The constant buffers to bind
	u32 numConstantBuffers;
	ZgConstantBufferBinding constantBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// The unordered buffers to bind
	u32 numUnorderedBuffers;
	ZgUnorderedBufferBinding unorderedBuffers[ZG_MAX_NUM_UNORDERED_BUFFERS];

	// The textures to bind
	u32 numTextures;
	ZgTextureBinding textures[ZG_MAX_NUM_TEXTURES];

	// The unordered textures to bind
	u32 numUnorderedTextures;
	ZgUnorderedTextureBinding unorderedTextures[ZG_MAX_NUM_UNORDERED_TEXTURES];
};

#ifdef __cplusplus
namespace zg {

class PipelineBindings final {
public:
	ZgPipelineBindings bindings = {};

	PipelineBindings() = default;
	PipelineBindings(const PipelineBindings&) = default;
	PipelineBindings& operator= (const PipelineBindings&) = default;
	~PipelineBindings() = default;

	PipelineBindings& addConstantBuffer(ZgConstantBufferBinding binding)
	{
		sfz_assert(bindings.numConstantBuffers < ZG_MAX_NUM_CONSTANT_BUFFERS);
		bindings.constantBuffers[bindings.numConstantBuffers] = binding;
		bindings.numConstantBuffers += 1;
		return *this;
	}

	PipelineBindings& addConstantBuffer(u32 bufferRegister, Buffer& buffer)
	{
		ZgConstantBufferBinding binding = {};
		binding.bufferRegister = bufferRegister;
		binding.buffer = buffer.handle;
		return this->addConstantBuffer(binding);
	}

	PipelineBindings& addUnorderedBuffer(ZgUnorderedBufferBinding binding)
	{
		sfz_assert(bindings.numUnorderedBuffers < ZG_MAX_NUM_UNORDERED_BUFFERS);
		bindings.unorderedBuffers[bindings.numUnorderedBuffers] = binding;
		bindings.numUnorderedBuffers += 1;
		return *this;
	}

	PipelineBindings& addUnorderedBuffer(
		u32 unorderedRegister,
		u32 numElements,
		u32 elementStrideBytes,
		Buffer& buffer)
	{
		return this->addUnorderedBuffer(unorderedRegister, 0, numElements, elementStrideBytes, buffer);
	}

	PipelineBindings& addUnorderedBuffer(
		u32 unorderedRegister,
		u32 firstElementIdx,
		u32 numElements,
		u32 elementStrideBytes,
		Buffer& buffer)
	{
		ZgUnorderedBufferBinding binding = {};
		binding.unorderedRegister = unorderedRegister;
		binding.firstElementIdx = firstElementIdx;
		binding.numElements = numElements;
		binding.elementStrideBytes = elementStrideBytes;
		binding.buffer = buffer.handle;
		return this->addUnorderedBuffer(binding);
	}

	PipelineBindings& addTexture(ZgTextureBinding binding)
	{
		sfz_assert(bindings.numTextures < ZG_MAX_NUM_TEXTURES);
		bindings.textures[bindings.numTextures] = binding;
		bindings.numTextures += 1;
		return *this;
	}

	PipelineBindings& addTexture(u32 textureRegister, Texture& texture)
	{
		ZgTextureBinding binding;
		binding.textureRegister = textureRegister;
		binding.texture = texture.handle;
		return this->addTexture(binding);
	}

	PipelineBindings& addUnorderedTexture(ZgUnorderedTextureBinding binding)
	{
		sfz_assert(bindings.numUnorderedTextures < ZG_MAX_NUM_UNORDERED_TEXTURES);
		bindings.unorderedTextures[bindings.numUnorderedTextures] = binding;
		bindings.numUnorderedTextures += 1;
		return *this;
	}

	PipelineBindings& addUnorderedTexture(
		u32 unorderedRegister,
		u32 mipLevel,
		Texture& texture)
	{
		ZgUnorderedTextureBinding binding = {};
		binding.unorderedRegister = unorderedRegister;
		binding.mipLevel = mipLevel;
		binding.texture = texture.handle;
		return this->addUnorderedTexture(binding);
	}
};

} // namespace zg
#endif

// Pipeline Compiler Settings
// ------------------------------------------------------------------------------------------------

// Enum representing various shader model versions
typedef enum {
	ZG_SHADER_MODEL_UNDEFINED = 0,
	ZG_SHADER_MODEL_6_0,
	ZG_SHADER_MODEL_6_1,
	ZG_SHADER_MODEL_6_2,
	ZG_SHADER_MODEL_6_3,
	ZG_SHADER_MODEL_6_4,
	ZG_SHADER_MODEL_6_5,
	ZG_SHADER_MODEL_6_6,
	ZG_SHADER_MODEL_FORCE_I32 = I32_MAX
} ZgShaderModel;

// The maximum number of compiler flags allowed to the DXC shader compiler
static const u32 ZG_MAX_NUM_DXC_COMPILER_FLAGS = 8;

// Compile settings to the HLSL compiler
sfz_struct(ZgPipelineCompileSettingsHLSL) {

	// Which shader model to target when compiling the HLSL file
	ZgShaderModel shaderModel;

	// Flags to the DXC compiler
	const char* dxcCompilerFlags[ZG_MAX_NUM_DXC_COMPILER_FLAGS];
};

// Pipeline Compute
// ------------------------------------------------------------------------------------------------

// Sample mode of a sampler
typedef enum {
	ZG_SAMPLING_MODE_UNDEFINED = 0,

	ZG_SAMPLING_MODE_NEAREST, // D3D12_FILTER_MIN_MAG_MIP_POINT
	ZG_SAMPLING_MODE_TRILINEAR, // D3D12_FILTER_MIN_MAG_MIP_LINEAR
	ZG_SAMPLING_MODE_ANISOTROPIC, // D3D12_FILTER_ANISOTROPIC

	ZG_SAMPLING_MODE_FORCE_I32 = I32_MAX
} ZgSamplingMode;

// Wrapping mode of a sampler
typedef enum {
	ZG_WRAPPING_MODE_UNDEFINED = 0,

	ZG_WRAPPING_MODE_CLAMP, // D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	ZG_WRAPPING_MODE_REPEAT, // D3D12_TEXTURE_ADDRESS_MODE_WRAP

	ZG_WRAPPING_MODE_FORCE_I32 = I32_MAX
} ZgWrappingMode;

typedef enum {
	ZG_COMPARISON_FUNC_NONE = 0,
	ZG_COMPARISON_FUNC_LESS,
	ZG_COMPARISON_FUNC_LESS_EQUAL,
	ZG_COMPARISON_FUNC_EQUAL,
	ZG_COMPARISON_FUNC_NOT_EQUAL,
	ZG_COMPARISON_FUNC_GREATER,
	ZG_COMPARISON_FUNC_GREATER_EQUAL,
	ZG_COMPARISON_FUNC_FORCE_I32 = I32_MAX
} ZgComparisonFunc;

// A struct defining a texture sampler
sfz_struct(ZgSampler) {

	// The sampling mode of the sampler
	ZgSamplingMode samplingMode;

	// The wrapping mode of the sampler (u == x, v == y)
	ZgWrappingMode wrappingModeU;
	ZgWrappingMode wrappingModeV;

	// Offset from the calculated mipmap level. E.g., if mipmap level 2 is calculated in the shader
	// and the lod bias is -1, then level 1 will be used instead. Level 0 is the highest resolution
	// texture.
	f32 mipLodBias;

	// Comparison func, mainly used to access hardware 2x2 PCF for shadow maps. If this is specified
	// to be anything but NONE the sampler turns into a comparison sampler.
	//
	// For D3D12 this means that the sampler should be specified as a "SamplerComparisonState"
	// instead of "SamplerState" in HLSL. It also modifies the sampling mode to a "comparison"
	// sampling mode. E.g. ZG_SAMPLING_MODE_NEAREST gives D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT
	// instead of D3D12_FILTER_MIN_MAG_MIP_POINT.
	ZgComparisonFunc comparisonFunc;
};

sfz_struct(ZgPipelineComputeDesc) {

	// Path to the shader source or the source directly depending on what create function is called.
	const char* computeShader;

	// The name of the entry function
	const char* computeShaderEntry;

	// A list of constant buffer registers which should be declared as push constants. This is an
	// optimization, however it can lead to worse performance if used improperly. Can be left empty
	// if unsure.
	u32 numPushConstants;
	u32 pushConstantRegisters[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// A list of samplers used by the pipeline
	//
	// Note: For D3D12 the first sampler in the array (0th) corresponds with the 0th sampler
	//       register, etc. E.g. meaning if you have three samplers, they need to have the
	//       registers 0, 1, 2.
	u32 numSamplers;
	ZgSampler samplers[ZG_MAX_NUM_SAMPLERS];
};

ZG_API ZgResult zgPipelineComputeCreateFromFileHLSL(
	ZgPipelineCompute** pipelineOut,
	const ZgPipelineComputeDesc* desc,
	const ZgPipelineCompileSettingsHLSL* compileSettings);

ZG_API void zgPipelineComputeDestroy(
	ZgPipelineCompute* pipeline);

ZG_API void zgPipelineComputeGetBindingsSignature(
	const ZgPipelineCompute* pipeline,
	ZgPipelineBindingsSignature* bindingsSignatureOut);

ZG_API void zgPipelineComputeGetGroupDimensions(
	const ZgPipelineCompute* pipeline,
	u32* groupDimXOut,
	u32* groupDimYOut,
	u32* groupDimZOut);

#ifdef __cplusplus
namespace zg {

class PipelineCompute final : public ManagedHandle<ZgPipelineCompute, zgPipelineComputeDestroy> {
public:
	ZgResult createFromFileHLSL(
		const ZgPipelineComputeDesc& desc,
		const ZgPipelineCompileSettingsHLSL& compileSettings)
	{
		this->destroy();
		return zgPipelineComputeCreateFromFileHLSL(
			&this->handle, &desc, &compileSettings);
	}

	ZgPipelineBindingsSignature getBindingsSignature() const
	{
		ZgPipelineBindingsSignature tmp = {};
		zgPipelineComputeGetBindingsSignature(this->handle, &tmp);
		return tmp;
	}

	void getGroupDims(u32& groupDimXOut, u32& groupDimYOut, u32& groupDimZOut) const
	{
		zgPipelineComputeGetGroupDimensions(this->handle, &groupDimXOut, &groupDimYOut, &groupDimZOut);
	}
};

class PipelineComputeBuilder final {
public:
	ZgPipelineComputeDesc desc = {};
	const char* computeShaderPath = nullptr;
	const char* computeShaderSrc = nullptr;

	PipelineComputeBuilder() = default;
	PipelineComputeBuilder(const PipelineComputeBuilder&) = default;
	PipelineComputeBuilder& operator= (const PipelineComputeBuilder&) = default;
	~PipelineComputeBuilder() = default;

	PipelineComputeBuilder& addComputeShaderPath(const char* entry, const char* path)
	{
		desc.computeShaderEntry = entry;
		computeShaderPath = path;
		return *this;
	}

	PipelineComputeBuilder& addComputeShaderSource(const char* entry, const char* src)
	{
		desc.computeShaderEntry = entry;
		computeShaderSrc = src;
		return *this;
	}

	PipelineComputeBuilder& addPushConstant(u32 constantBufferRegister)
	{
		sfz_assert(desc.numPushConstants < ZG_MAX_NUM_CONSTANT_BUFFERS);
		desc.pushConstantRegisters[desc.numPushConstants] = constantBufferRegister;
		desc.numPushConstants += 1;
		return *this;
	}

	PipelineComputeBuilder& addSampler(u32 samplerRegister, ZgSampler sampler)
	{
		sfz_assert(samplerRegister == desc.numSamplers);
		sfz_assert(desc.numSamplers < ZG_MAX_NUM_SAMPLERS);
		desc.samplers[samplerRegister] = sampler;
		desc.numSamplers += 1;
		return *this;
	}

	PipelineComputeBuilder& addSampler(
		u32 samplerRegister,
		ZgSamplingMode samplingMode,
		ZgWrappingMode wrappingModeU = ZG_WRAPPING_MODE_CLAMP,
		ZgWrappingMode wrappingModeV = ZG_WRAPPING_MODE_CLAMP,
		f32 mipLodBias = 0.0f)
	{
		ZgSampler sampler = {};
		sampler.samplingMode = samplingMode;
		sampler.wrappingModeU = wrappingModeU;
		sampler.wrappingModeV = wrappingModeV;
		sampler.mipLodBias = mipLodBias;
		return addSampler(samplerRegister, sampler);
	}

	ZgResult buildFromFileHLSL(
		PipelineCompute& pipelineOut, ZgShaderModel model = ZG_SHADER_MODEL_6_0)
	{
		// Set path
		desc.computeShader = this->computeShaderPath;

		// Create compile settings
		ZgPipelineCompileSettingsHLSL compileSettings = {};
		compileSettings.shaderModel = model;
		compileSettings.dxcCompilerFlags[0] = "-Zi";
		compileSettings.dxcCompilerFlags[1] = "-Qembed_debug";
		compileSettings.dxcCompilerFlags[2] = "-O3";

		// Build pipeline
		return pipelineOut.createFromFileHLSL(desc, compileSettings);
	}
};

} // namespace zg
#endif

// Pipeline Render Signature
// ------------------------------------------------------------------------------------------------

// The maximum number of vertex attributes allowed as input to a vertex shader
static const u32 ZG_MAX_NUM_VERTEX_ATTRIBUTES = 8;

// The maximum number of render targets allowed on a single pipeline
static const u32 ZG_MAX_NUM_RENDER_TARGETS = 8;

// The type of data contained in a vertex
typedef enum {
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

	ZG_VERTEX_ATTRIBUTE_FORCE_I32 = I32_MAX
} ZgVertexAttributeType;

// A struct defining a vertex attribute
sfz_struct(ZgVertexAttribute) {
	// The location of the attribute in the vertex input.
	//
	// For HLSL the semantic name need to be "TEXCOORD<attributeLocation>"
	// E.g.:
	// struct VSInput {
	//     f323 position : TEXCOORD0;
	//     f323 normal : TEXCOORD1;
	// }
	u32 location;

	// Which vertex buffer slot the attribute should be read from.
	//
	// If you are storing all vertex attributes in the same buffer (e.g. your buffer is an array
	// of a vertex struct of some kind), this parameter should typically be 0.
	//
	// This corresponds to the "vertexBufferSlot" parameter in zgCommandListSetVertexBuffer().
	u32 vertexBufferSlot;

	// The data type
	ZgVertexAttributeType type;

	// Offset in bytes from start of buffer to the first element of this type.
	u32 offsetToFirstElementInBytes;
};

// A struct representing the rendering signature of a render pipeline.
//
// In addition to the contents of ZgPipelineBindingsSignature, this also contains render pipeline
// specific input (i.e., how vertex data is read and render target information). The additional
// data cannot be inferred by reflection and is actually specified by the user when creating the
// pipeline. It's is mainly provided here as a convenience.
sfz_struct(ZgPipelineRenderSignature) {

	// The bindings signnature
	ZgPipelineBindingsSignature bindings;

	// The vertex attributes to the vertex shader
	u32 numVertexAttributes;
	ZgVertexAttribute vertexAttributes[ZG_MAX_NUM_VERTEX_ATTRIBUTES];

	// Render targets
	u32 numRenderTargets;
	ZgTextureFormat renderTargets[ZG_MAX_NUM_RENDER_TARGETS];
};

// Pipeline Render
// ------------------------------------------------------------------------------------------------

sfz_struct(ZgRasterizerSettings) {

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
	i32 depthBias;

	// Depth bias for each pixel's slope.
	//
	// For D3D12, see: https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
	f32 depthBiasSlopeScaled;

	// The max depth bias for a pixel.
	//
	// For D3D12, see: https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
	f32 depthBiasClamp;
};

typedef enum {
	ZG_BLEND_FUNC_ADD = 0,
	ZG_BLEND_FUNC_DST_SUB_SRC, // dst - src
	ZG_BLEND_FUNC_SRC_SUB_DST, // src - dst
	ZG_BLEND_FUNC_MIN,
	ZG_BLEND_FUNC_MAX,
	ZG_BLEND_FUNC_FORCE_I32 = I32_MAX
} ZgBlendFunc;

// See: https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_blend
typedef enum {
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
	ZG_BLEND_FACTOR_FORCE_I32 = I32_MAX
} ZgBlendFactor;

sfz_struct(ZgBlendSettings) {

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

// The common information required to create a render pipeline
sfz_struct(ZgPipelineRenderDesc) {

	// Path to the shader source or the source directly depending on what create function is called.
	const char* vertexShader;
	const char* pixelShader;

	// The names of the entry functions
	const char* vertexShaderEntry;
	const char* pixelShaderEntry;

	// The vertex attributes to the vertex shader
	u32 numVertexAttributes;
	ZgVertexAttribute vertexAttributes[ZG_MAX_NUM_VERTEX_ATTRIBUTES];

	// The number of vertex buffer slots used by the vertex attributes
	//
	// If only one buffer is used (i.e. array of vertex struct) then numVertexBufferSlots should be
	// 1 and vertexBufferStrides[0] should be sizeof(Vertex) stored in your buffer.
	u32 numVertexBufferSlots;
	u32 vertexBufferStridesBytes[ZG_MAX_NUM_VERTEX_ATTRIBUTES];

	// A list of constant buffer registers which should be declared as push constants. This is an
	// optimization, however it can lead to worse performance if used improperly. Can be left empty
	// if unsure.
	u32 numPushConstants;
	u32 pushConstantRegisters[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// A list of samplers used by the pipeline
	//
	// Note: For D3D12 the first sampler in the array (0th) corresponds with the 0th sampler
	//       register, etc. E.g. meaning if you have three samplers, they need to have the
	//       registers 0, 1, 2.
	u32 numSamplers;
	ZgSampler samplers[ZG_MAX_NUM_SAMPLERS];

	// A list of render targets used by the pipeline
	u32 numRenderTargets;
	ZgTextureFormat renderTargets[ZG_MAX_NUM_RENDER_TARGETS];

	// Rasterizer settings
	ZgRasterizerSettings rasterizer;

	// Blend test settings
	ZgBlendSettings blending;

	// Depth test settings, default is no depth test (ZG_COMPARISON_FUNC_NONE).
	ZgComparisonFunc depthFunc;
};

ZG_API ZgResult zgPipelineRenderCreateFromFileHLSL(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderDesc* desc,
	const ZgPipelineCompileSettingsHLSL* compileSettings);

ZG_API ZgResult zgPipelineRenderCreateFromSourceHLSL(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderDesc* desc,
	const ZgPipelineCompileSettingsHLSL* compileSettings);

ZG_API void zgPipelineRenderDestroy(
	ZgPipelineRender* pipeline);

ZG_API void zgPipelineRenderGetSignature(
	const ZgPipelineRender* pipeline,
	ZgPipelineRenderSignature* signatureOut);

#ifdef __cplusplus
namespace zg {

class PipelineRender final : public ManagedHandle<ZgPipelineRender, zgPipelineRenderDestroy> {
public:
	ZgResult createFromFileHLSL(
		const ZgPipelineRenderDesc& desc,
		const ZgPipelineCompileSettingsHLSL& compileSettings)
	{
		this->destroy();
		return zgPipelineRenderCreateFromFileHLSL(
			&this->handle, &desc, &compileSettings);
	}

	ZgResult createFromSourceHLSL(
		const ZgPipelineRenderDesc& desc,
		const ZgPipelineCompileSettingsHLSL& compileSettings)
	{
		this->destroy();
		return zgPipelineRenderCreateFromSourceHLSL(
			&this->handle, &desc, &compileSettings);
	}

	ZgPipelineRenderSignature getSignature() const
	{
		ZgPipelineRenderSignature tmp = {};
		zgPipelineRenderGetSignature(this->handle, &tmp);
		return tmp;
	}
};

class PipelineRenderBuilder final {
public:
	ZgPipelineRenderDesc desc = {};
	const char* vertexShaderPath = nullptr;
	const char* pixelShaderPath = nullptr;
	const char* vertexShaderSrc = nullptr;
	const char* pixelShaderSrc = nullptr;

	PipelineRenderBuilder() = default;
	PipelineRenderBuilder(const PipelineRenderBuilder&) = default;
	PipelineRenderBuilder& operator= (const PipelineRenderBuilder&) = default;
	~PipelineRenderBuilder() = default;

	PipelineRenderBuilder& addVertexShaderPath(const char* entry, const char* path)
	{
		desc.vertexShaderEntry = entry;
		vertexShaderPath = path;
		return *this;
	}

	PipelineRenderBuilder& addPixelShaderPath(const char* entry, const char* path)
	{
		desc.pixelShaderEntry = entry;
		pixelShaderPath = path;
		return *this;
	}

	PipelineRenderBuilder& addVertexShaderSource(const char* entry, const char* src)
	{
		desc.vertexShaderEntry = entry;
		vertexShaderSrc = src;
		return *this;
	}

	PipelineRenderBuilder& addPixelShaderSource(const char* entry, const char* src)
	{
		desc.pixelShaderEntry = entry;
		pixelShaderSrc = src;
		return *this;
	}

	PipelineRenderBuilder& addVertexAttribute(ZgVertexAttribute attribute)
	{
		sfz_assert(desc.numVertexAttributes < ZG_MAX_NUM_VERTEX_ATTRIBUTES);
		desc.vertexAttributes[desc.numVertexAttributes] = attribute;
		desc.numVertexAttributes += 1;
		return *this;
	}

	PipelineRenderBuilder& addVertexAttribute(
		u32 location,
		u32 vertexBufferSlot,
		ZgVertexAttributeType type,
		u32 offsetInBuffer)
	{
		ZgVertexAttribute attribute = {};
		attribute.location = location;
		attribute.vertexBufferSlot = vertexBufferSlot;
		attribute.type = type;
		attribute.offsetToFirstElementInBytes = offsetInBuffer;
		return addVertexAttribute(attribute);
	}

	PipelineRenderBuilder& addVertexBufferInfo(
		u32 slot, u32 vertexBufferStrideBytes)
	{
		sfz_assert(slot == desc.numVertexBufferSlots);
		sfz_assert(desc.numVertexBufferSlots < ZG_MAX_NUM_VERTEX_ATTRIBUTES);
		desc.vertexBufferStridesBytes[slot] = vertexBufferStrideBytes;
		desc.numVertexBufferSlots += 1;
		return *this;
	}

	PipelineRenderBuilder& addPushConstant(u32 constantBufferRegister)
	{
		sfz_assert(desc.numPushConstants < ZG_MAX_NUM_CONSTANT_BUFFERS);
		desc.pushConstantRegisters[desc.numPushConstants] = constantBufferRegister;
		desc.numPushConstants += 1;
		return *this;
	}

	PipelineRenderBuilder& addSampler(u32 samplerRegister, ZgSampler sampler)
	{
		sfz_assert(samplerRegister == desc.numSamplers);
		sfz_assert(desc.numSamplers < ZG_MAX_NUM_SAMPLERS);
		desc.samplers[samplerRegister] = sampler;
		desc.numSamplers += 1;
		return *this;
	}

	PipelineRenderBuilder& addSampler(
		u32 samplerRegister,
		ZgSamplingMode samplingMode,
		ZgWrappingMode wrappingModeU = ZG_WRAPPING_MODE_CLAMP,
		ZgWrappingMode wrappingModeV = ZG_WRAPPING_MODE_CLAMP,
		f32 mipLodBias = 0.0f)
	{
		ZgSampler sampler = {};
		sampler.samplingMode = samplingMode;
		sampler.wrappingModeU = wrappingModeU;
		sampler.wrappingModeV = wrappingModeV;
		sampler.mipLodBias = mipLodBias;
		return addSampler(samplerRegister, sampler);
	}

	PipelineRenderBuilder& addRenderTarget(ZgTextureFormat format)
	{
		sfz_assert(desc.numRenderTargets < ZG_MAX_NUM_RENDER_TARGETS);
		desc.renderTargets[desc.numRenderTargets] = format;
		desc.numRenderTargets += 1;
		return *this;
	}

	PipelineRenderBuilder& setWireframeRendering(bool wireframeEnabled)
	{
		desc.rasterizer.wireframeMode = wireframeEnabled ? ZG_TRUE : ZG_FALSE;
		return *this;
	}

	PipelineRenderBuilder& setCullingEnabled(bool cullingEnabled)
	{
		desc.rasterizer.cullingEnabled = cullingEnabled ? ZG_TRUE : ZG_FALSE;
		return *this;
	}

	PipelineRenderBuilder& setCullMode(
		bool cullFrontFacing, bool fontFacingIsCounterClockwise = false)
	{
		desc.rasterizer.cullFrontFacing = cullFrontFacing ? ZG_TRUE : ZG_FALSE;
		desc.rasterizer.frontFacingIsCounterClockwise =
			fontFacingIsCounterClockwise ? ZG_TRUE : ZG_FALSE;
		return *this;
	}

	PipelineRenderBuilder& setDepthBias(
		i32 bias, f32 biasSlopeScaled, f32 biasClamp = 0.0f)
	{
		desc.rasterizer.depthBias = bias;
		desc.rasterizer.depthBiasSlopeScaled = biasSlopeScaled;
		desc.rasterizer.depthBiasClamp = biasClamp;
		return *this;
	}

	PipelineRenderBuilder& setBlendingEnabled(bool blendingEnabled)
	{
		desc.blending.blendingEnabled = blendingEnabled ? ZG_TRUE : ZG_FALSE;
		return *this;
	}

	PipelineRenderBuilder& setBlendFuncColor(
		ZgBlendFunc func, ZgBlendFactor srcFactor, ZgBlendFactor dstFactor)
	{
		desc.blending.blendFuncColor = func;
		desc.blending.srcValColor = srcFactor;
		desc.blending.dstValColor = dstFactor;
		return *this;
	}

	PipelineRenderBuilder& setBlendFuncAlpha(
		ZgBlendFunc func, ZgBlendFactor srcFactor, ZgBlendFactor dstFactor)
	{
		desc.blending.blendFuncAlpha = func;
		desc.blending.srcValAlpha = srcFactor;
		desc.blending.dstValAlpha = dstFactor;
		return *this;
	}

	PipelineRenderBuilder& setDepthFunc(ZgComparisonFunc depthFunc)
	{
		desc.depthFunc = depthFunc;
		return *this;
	}

	ZgResult buildFromFileHLSL(
		PipelineRender& pipelineOut, ZgShaderModel model = ZG_SHADER_MODEL_6_0)
	{
		// Set path
		desc.vertexShader = this->vertexShaderPath;
		desc.pixelShader = this->pixelShaderPath;

		// Create compile settings
		ZgPipelineCompileSettingsHLSL compileSettings = {};
		compileSettings.shaderModel = model;
		compileSettings.dxcCompilerFlags[0] = "-Zi";
		compileSettings.dxcCompilerFlags[1] = "-Qembed_debug";
		compileSettings.dxcCompilerFlags[2] = "-O3";

		// Build pipeline
		return pipelineOut.createFromFileHLSL(desc, compileSettings);
	}

	ZgResult buildFromSourceHLSL(
		PipelineRender& pipelineOut, ZgShaderModel model = ZG_SHADER_MODEL_6_0)
	{
		// Set source
		desc.vertexShader = this->vertexShaderSrc;
		desc.pixelShader = this->pixelShaderSrc;

		// Create compile settings
		ZgPipelineCompileSettingsHLSL compileSettings = {};
		compileSettings.shaderModel = model;
		compileSettings.dxcCompilerFlags[0] = "-Zi";
		compileSettings.dxcCompilerFlags[1] = "-Qembed_debug";
		compileSettings.dxcCompilerFlags[2] = "-O3";

		// Build pipeline
		return pipelineOut.createFromSourceHLSL(desc, compileSettings);
	}
};

} // namespace zg
#endif

// Framebuffer
// ------------------------------------------------------------------------------------------------

sfz_struct(ZgFramebufferDesc) {

	// Render targets
	u32 numRenderTargets;
	ZgTexture* renderTargets[ZG_MAX_NUM_RENDER_TARGETS];

	// Depth buffer
	ZgTexture* depthBuffer;
};

ZG_API ZgResult zgFramebufferCreate(
	ZgFramebuffer** framebufferOut,
	const ZgFramebufferDesc* desc);

// No-op if framebuffer is built-in swapchain framebuffer, which is managed and owned by the ZeroG
// context.
ZG_API void zgFramebufferDestroy(
	ZgFramebuffer* framebuffer);

ZG_API ZgResult zgFramebufferGetResolution(
	const ZgFramebuffer* framebuffer,
	u32* widthOut,
	u32* heightOut);

#ifdef __cplusplus
namespace zg {

class Framebuffer final : public ManagedHandle<ZgFramebuffer, zgFramebufferDestroy> {
public:
	ZgResult create(const ZgFramebufferDesc& desc)
	{
		this->destroy();
		return zgFramebufferCreate(&this->handle, &desc);
	}

	ZgResult getResolution(u32& widthOut, u32& heightOut) const
	{
		return zgFramebufferGetResolution(this->handle, &widthOut, &heightOut);
	}
};

class FramebufferBuilder final {
public:
	ZgFramebufferDesc desc = {};

	FramebufferBuilder() = default;
	FramebufferBuilder(const FramebufferBuilder&) = default;
	FramebufferBuilder& operator= (const FramebufferBuilder&) = default;
	~FramebufferBuilder() = default;

	FramebufferBuilder& addRenderTarget(Texture& renderTarget)
	{
		sfz_assert(desc.numRenderTargets < ZG_MAX_NUM_RENDER_TARGETS);
		u32 idx = desc.numRenderTargets;
		desc.numRenderTargets += 1;
		desc.renderTargets[idx] = renderTarget.handle;
		return *this;
	}

	FramebufferBuilder& setDepthBuffer(Texture& depthBuffer)
	{
		desc.depthBuffer = depthBuffer.handle;
		return *this;
	}

	ZgResult build(Framebuffer& framebufferOut)
	{
		return framebufferOut.create(this->desc);
	}
};

} // namespace zg
#endif

// Profiler
// ------------------------------------------------------------------------------------------------

sfz_struct(ZgProfilerDesc) {

	// The number of measurements that this profiler can hold. Once this limit has been reached
	// older measurements will automatically be thrown out to make room for newer ones. In other
	// words, this should be at least "number of measurements per frame" times "number of frames
	// before syncing".
	u32 maxNumMeasurements;
};

ZG_API ZgResult zgProfilerCreate(
	ZgProfiler** profilerOut,
	const ZgProfilerDesc* desc);

ZG_API void zgProfilerDestroy(
	ZgProfiler* profiler);

// Retrieves the measurement recorded fora given measurement id.
//
// Note: Profiling is an async operation, this function must NOT be called before the command list
//       in which the profiling occured in has finished executing. Doing so is undefined behavior.
//
// Note: Will return an error if the measurement associated with the id has already been thrown
//       out and replaced with a newer one.
ZG_API ZgResult zgProfilerGetMeasurement(
	ZgProfiler* profiler,
	u64 measurementId,
	f32* measurementMsOut);

#ifdef __cplusplus
namespace zg {

class Profiler final : public ManagedHandle<ZgProfiler, zgProfilerDestroy> {
public:
	ZgResult create(const ZgProfilerDesc& desc)
	{
		this->destroy();
		return zgProfilerCreate(&this->handle, &desc);
	}

	ZgResult getMeasurement(
		u64 measurementId,
		f32& measurementMsOut)
	{
		return zgProfilerGetMeasurement(this->handle, measurementId, &measurementMsOut);
	}
};

} // namespace zg
#endif

// Fence
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgFenceCreate(
	ZgFence** fenceOut);

ZG_API void zgFenceDestroy(
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

#ifdef __cplusplus
namespace zg {

class Fence final : public ManagedHandle<ZgFence, zgFenceDestroy> {
public:
	ZgResult create()
	{
		this->destroy();
		return zgFenceCreate(&this->handle);
	}

	ZgResult reset()
	{
		return zgFenceReset(this->handle);
	}

	ZgResult checkIfSignaled(bool& fenceSignaledOut) const
	{
		ZgBool signaled = ZG_FALSE;
		ZgResult res = zgFenceCheckIfSignaled(this->handle, &signaled);
		fenceSignaledOut = signaled == ZG_FALSE ? false : true;
		return res;
	}

	bool checkIfSignaled() const
	{
		bool signaled = false;
		[[maybe_unused]] ZgResult res = this->checkIfSignaled(signaled);
		return signaled;
	}

	ZgResult waitOnCpuBlocking() const
	{
		return zgFenceWaitOnCpuBlocking(this->handle);
	}
};

} // namespace zg
#endif

// Command list
// ------------------------------------------------------------------------------------------------

sfz_struct(ZgRect) {
	u32 topLeftX, topLeftY, width, height;
};

sfz_struct(ZgImageViewConstCpu) {
	ZgTextureFormat format;
	const void* data;
	u32 width;
	u32 height;
	u32 pitchInBytes;
};

ZG_API ZgResult zgCommandListBeginEvent(
	ZgCommandList* commandList,
	const char* name,
	const f32* optionalRgbaColor);

ZG_API ZgResult zgCommandListEndEvent(
	ZgCommandList* commandList);

ZG_API ZgResult zgCommandListMemcpyBufferToBuffer(
	ZgCommandList* commandList,
	ZgBuffer* dstBuffer,
	u64 dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	u64 srcBufferOffsetBytes,
	u64 numBytes);

// Copies an image from the CPU to a texture on the GPU.
//
// The CPU image (srcImageCpu) is first (synchronously) copied to a temporary upload buffer
// (tempUploadBuffer), and then asynchronously copied from this upload buffer to the specified
// texture (dstTexture). In other words, the CPU memory containing the image can freely be removed
// after this call. The temporary upload buffer must not be touched until this command list has
// finished executing.
ZG_API ZgResult zgCommandListMemcpyToTexture(
	ZgCommandList* commandList,
	ZgTexture* dstTexture,
	u32 dstTextureMipLevel,
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
	ZgTexture* texture);

ZG_API ZgResult zgCommandListSetPushConstant(
	ZgCommandList* commandList,
	u32 shaderRegister,
	const void* data,
	u32 dataSizeInBytes);

ZG_API ZgResult zgCommandListSetPipelineBindings(
	ZgCommandList* commandList,
	const ZgPipelineBindings* bindings);

ZG_API ZgResult zgCommandListSetPipelineCompute(
	ZgCommandList* commandList,
	ZgPipelineCompute* pipeline);

// Inserts an UAV barrier for the specified buffer, meaning all unordered reads/writes must finish
// before any new ones start.
//
// "You don't need to insert a UAV barrier between 2 draw or dispatch calls that only read a UAV.
//  Additionally, you don't need to insert a UAV barrier between 2 draw or dispatch calls that
//  write to the same UAV if you know that it's safe to execute the UAV accesses in any order."
ZG_API ZgResult zgCommandListUnorderedBarrierBuffer(
	ZgCommandList* commandList,
	ZgBuffer* buffer);

// Inserts an UAV barrier for the specified texture, meaning all unordered reads/writes must finish
// before any new ones start.
//
// "You don't need to insert a UAV barrier between 2 draw or dispatch calls that only read a UAV.
//  Additionally, you don't need to insert a UAV barrier between 2 draw or dispatch calls that
//  write to the same UAV if you know that it's safe to execute the UAV accesses in any order."
ZG_API ZgResult zgCommandListUnorderedBarrierTexture(
	ZgCommandList* commandList,
	ZgTexture* texture);

// Inserts an UAV barrier for all unordered resources, meaning that all read/writes must finish
// before any new read/writes start.
// "The resource can be NULL, which indicates that any UAV access could require the barrier.
ZG_API ZgResult zgCommandListUnorderedBarrierAll(
	ZgCommandList* commandList);

ZG_API ZgResult zgCommandListDispatchCompute(
	ZgCommandList* commandList,
	u32 groupCountX,
	u32 groupCountY,
	u32 groupCountZ);

ZG_API ZgResult zgCommandListSetPipelineRender(
	ZgCommandList* commandList,
	ZgPipelineRender* pipeline);

// The viewport and scissor are optional, if nullptr they will cover the entire framebuffer
ZG_API ZgResult zgCommandListSetFramebuffer(
	ZgCommandList* commandList,
	ZgFramebuffer* framebuffer,
	const ZgRect* optionalViewport,
	const ZgRect* optionalScissor);

// Change the viewport for an already set framebuffer
ZG_API ZgResult zgCommandListSetFramebufferViewport(
	ZgCommandList* commandList,
	const ZgRect* viewport);

// Change the scissor for an already set framebuffer
ZG_API ZgResult zgCommandListSetFramebufferScissor(
	ZgCommandList* commandList,
	const ZgRect* scissor);

ZG_API ZgResult zgCommandListClearRenderTargetOptimal(
	ZgCommandList* commandList,
	u32 renderTargetIdx);

ZG_API ZgResult zgCommandListClearRenderTargets(
	ZgCommandList* commandList,
	f32 red,
	f32 green,
	f32 blue,
	f32 alpha);

ZG_API ZgResult zgCommandListClearRenderTargetsOptimal(
	ZgCommandList* commandList);

ZG_API ZgResult zgCommandListClearDepthBuffer(
	ZgCommandList* commandList,
	f32 depth);

ZG_API ZgResult zgCommandListClearDepthBufferOptimal(
	ZgCommandList* commandList);

typedef enum {
	ZG_INDEX_BUFFER_TYPE_UINT32 = 0,
	ZG_INDEX_BUFFER_TYPE_UINT16,
	ZG_INDEX_BUFFER_TYPE_FORCE_I32 = I32_MAX
} ZgIndexBufferType;

ZG_API ZgResult zgCommandListSetIndexBuffer(
	ZgCommandList* commandList,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type);

ZG_API ZgResult zgCommandListSetVertexBuffer(
	ZgCommandList* commandList,
	u32 vertexBufferSlot,
	ZgBuffer* vertexBuffer);

ZG_API ZgResult zgCommandListDrawTriangles(
	ZgCommandList* commandList,
	u32 startVertexIndex,
	u32 numVertices);

ZG_API ZgResult zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandList,
	u32 startIndex,
	u32 numIndices);

// Begins profiling operations from this point until zgCommandListProfileEnd() is called.
//
// "measurementIdOut" returns a unique identifier for this measurement. This identifier is later
// used to retrieve the measurement using zgProfilerGetMeasurement().
ZG_API ZgResult zgCommandListProfileBegin(
	ZgCommandList* commandList,
	ZgProfiler* profiler,
	u64* measurementIdOut);

ZG_API ZgResult zgCommandListProfileEnd(
	ZgCommandList* commandList,
	ZgProfiler* profiler,
	u64 measurementId);

#ifdef __cplusplus
namespace zg {

class CommandList final : public ManagedHandle<ZgCommandList> {
public:
	ZgResult beginEvent(const char* name, const f32* rgbaColors = nullptr)
	{
		return zgCommandListBeginEvent(this->handle, name, rgbaColors);
	}

	ZgResult endEvent()
	{
		return zgCommandListEndEvent(this->handle);
	}

	ZgResult memcpyBufferToBuffer(
		Buffer& dstBuffer,
		u64 dstBufferOffsetBytes,
		Buffer& srcBuffer,
		u64 srcBufferOffsetBytes,
		u64 numBytes)
	{
		return zgCommandListMemcpyBufferToBuffer(
			this->handle,
			dstBuffer.handle,
			dstBufferOffsetBytes,
			srcBuffer.handle,
			srcBufferOffsetBytes,
			numBytes);
	}

	ZgResult memcpyToTexture(
		Texture& dstTexture,
		u32 dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		Buffer& tempUploadBuffer)
	{
		return zgCommandListMemcpyToTexture(
			this->handle,
			dstTexture.handle,
			dstTextureMipLevel,
			&srcImageCpu,
			tempUploadBuffer.handle);
	}

	ZgResult enableQueueTransition(Buffer& buffer)
	{
		return zgCommandListEnableQueueTransitionBuffer(this->handle, buffer.handle);
	}

	ZgResult enableQueueTransition(Texture& texture)
	{
		return zgCommandListEnableQueueTransitionTexture(this->handle, texture.handle);
	}

	ZgResult setPushConstant(
		u32 shaderRegister, const void* data, u32 dataSizeInBytes)
	{
		return zgCommandListSetPushConstant(
			this->handle, shaderRegister, data, dataSizeInBytes);
	}

	ZgResult setPipelineBindings(const PipelineBindings& bindings)
	{
		return zgCommandListSetPipelineBindings(this->handle, &bindings.bindings);
	}

	ZgResult setPipeline(PipelineCompute& pipeline)
	{
		return zgCommandListSetPipelineCompute(this->handle, pipeline.handle);
	}

	ZgResult unorderedBarrier(Buffer& buffer)
	{
		return zgCommandListUnorderedBarrierBuffer(this->handle, buffer.handle);
	}

	ZgResult unorderedBarrier(Texture& texture)
	{
		return zgCommandListUnorderedBarrierTexture(this->handle, texture.handle);
	}

	ZgResult unorderedBarrier()
	{
		return zgCommandListUnorderedBarrierAll(this->handle);
	}

	ZgResult dispatchCompute(
		u32 groupCountX, u32 groupCountY = 1, u32 groupCountZ = 1)
	{
		return zgCommandListDispatchCompute(
			this->handle, groupCountX, groupCountY, groupCountZ);
	}

	ZgResult setPipeline(PipelineRender& pipeline)
	{
		return zgCommandListSetPipelineRender(this->handle, pipeline.handle);
	}

	ZgResult setFramebuffer(
		Framebuffer& framebuffer,
		const ZgRect* optionalViewport = nullptr,
		const ZgRect* optionalScissor = nullptr)
	{
		return zgCommandListSetFramebuffer(
			this->handle, framebuffer.handle, optionalViewport, optionalScissor);
	}

	ZgResult setFramebufferViewport(const ZgRect& viewport)
	{
		return zgCommandListSetFramebufferViewport(this->handle, &viewport);
	}

	ZgResult setFramebufferScissor(const ZgRect& scissor)
	{
		return zgCommandListSetFramebufferScissor(this->handle, &scissor);
	}

	ZgResult clearRenderTargetOptimal(u32 renderTargetIdx)
	{
		return zgCommandListClearRenderTargetOptimal(this->handle, renderTargetIdx);
	}

	ZgResult clearRenderTargets(f32 red, f32 green, f32 blue, f32 alpha)
	{
		return zgCommandListClearRenderTargets(this->handle, red, green, blue, alpha);
	}

	ZgResult clearRenderTargetsOptimal()
	{
		return zgCommandListClearRenderTargetsOptimal(this->handle);
	}

	ZgResult clearDepthBuffer(f32 depth)
	{
		return zgCommandListClearDepthBuffer(this->handle, depth);
	}

	ZgResult clearDepthBufferOptimal()
	{
		return zgCommandListClearDepthBufferOptimal(this->handle);
	}

	ZgResult setIndexBuffer(Buffer& indexBuffer, ZgIndexBufferType type)
	{
		return zgCommandListSetIndexBuffer(this->handle, indexBuffer.handle, type);
	}

	ZgResult setVertexBuffer(u32 vertexBufferSlot, Buffer& vertexBuffer)
	{
		return zgCommandListSetVertexBuffer(
			this->handle, vertexBufferSlot, vertexBuffer.handle);
	}

	ZgResult drawTriangles(u32 startVertexIndex, u32 numVertices)
	{
		return zgCommandListDrawTriangles(this->handle, startVertexIndex, numVertices);
	}

	ZgResult drawTrianglesIndexed(u32 startIndex, u32 numTriangles)
	{
		return zgCommandListDrawTrianglesIndexed(
			this->handle, startIndex, numTriangles);
	}

	ZgResult profileBegin(Profiler& profiler, u64& measurementIdOut)
	{
		return zgCommandListProfileBegin(this->handle, profiler.handle, &measurementIdOut);
	}

	ZgResult profileEnd(Profiler& profiler, u64 measurementId)
	{
		return zgCommandListProfileEnd(this->handle, profiler.handle, measurementId);
	}
};

} // namespace zg
#endif

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_API ZgCommandQueue* zgCommandQueueGetPresentQueue(void);

ZG_API ZgCommandQueue* zgCommandQueueGetCopyQueue(void);

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

#ifdef __cplusplus
namespace zg {

class CommandQueue final : public ManagedHandle<ZgCommandQueue>{
public:
	static CommandQueue getPresentQueue()
	{
		CommandQueue queue;
		queue.handle = zgCommandQueueGetPresentQueue();
		return queue;
	}

	static CommandQueue getCopyQueue()
	{
		CommandQueue queue;
		queue.handle = zgCommandQueueGetCopyQueue();
		return queue;
	}

	ZgResult signalOnGpu(Fence& fenceToSignal)
	{
		return zgCommandQueueSignalOnGpu(this->handle, fenceToSignal.handle);
	}

	ZgResult waitOnGpu(const Fence& fence)
	{
		return zgCommandQueueWaitOnGpu(this->handle, fence.handle);
	}

	ZgResult flush()
	{
		return zgCommandQueueFlush(this->handle);
	}

	ZgResult beginCommandListRecording(CommandList& commandListOut)
	{
		if (commandListOut.valid()) return ZG_ERROR_INVALID_ARGUMENT;
		return zgCommandQueueBeginCommandListRecording(this->handle, &commandListOut.handle);
	}

	ZgResult executeCommandList(CommandList& commandList)
	{
		ZgResult res = zgCommandQueueExecuteCommandList(this->handle, commandList.handle);
		commandList.destroy();
		return res;
	}
};

} // namespace zg
#endif

// Logging interface
// ------------------------------------------------------------------------------------------------

typedef enum {
	ZG_LOG_LEVEL_NOISE = 0,
	ZG_LOG_LEVEL_INFO,
	ZG_LOG_LEVEL_WARNING,
	ZG_LOG_LEVEL_ERROR,
	ZG_LOG_LEVEL_FORCE_I32 = I32_MAX
} ZgLogLevel;

// Logger used for logging inside ZeroG.
//
// The logger must be thread-safe. I.e. it must be okay to call it simulatenously from multiple
// threads.
//
// If no custom logger is wanted leave all fields zero in this struct. Normal printf() will then be
// used for logging instead.
sfz_struct(ZgLogger) {

	// Function pointer to user-specified log function.
	void(*log)(void* userPtr, const char* file, int line, ZgLogLevel level, const char* message);

	// User specified pointer that is provied to each log() call.
	void* userPtr;
};

// Context
// ------------------------------------------------------------------------------------------------

sfz_struct(ZgContextInitSettingsD3D12) {

	// [Optional] Used to enable D3D12 validation.
	ZgBool debugMode;

	// [Optional] Used to enable GPU based debug mode (slow)
	ZgBool debugModeGpuBased;

	// [Optional]
	ZgBool useSoftwareRenderer;

	// [Optional] Enable DRED (Device Removed Extended Data) Auto-Breadcrumbs
	// See: https://docs.microsoft.com/en-us/windows/win32/direct3d12/use-dred
	ZgBool enableDredAutoBreadcrumbs;
};

sfz_struct(ZgContextInitSettingsVulkan) {

	// [Optional] Used to enable Vulkan debug layers
	ZgBool debugMode;
};

// The settings used to create a context and initialize ZeroG
sfz_struct(ZgContextInitSettings) {

	// [Mandatory] Platform specific native handle.
	//
	// On Windows, this is a HWND, i.e. native window handle.
	void* nativeHandle;

	// [Mandatory] The dimensions (in pixels) of the window being rendered to
	u32 width;
	u32 height;

	// [Optional] Whether VSync should be enabled or not
	ZgBool vsync;

	// [Optional] The logger used for logging
	ZgLogger logger;

	// [Optional] The allocator used to allocate CPU memory.
	//
	// Need to be kept alive for the remaining duration of the program if specified. MUST be
	// thread-safe.
	SfzAllocator* allocator;

	// [Optional] Whether to auto-cache pipelines to disk or not.
	ZgBool autoCachePipelines;
	const char* autoCachePipelinesDir;

	// [Optional] D3D12 specific settings
	ZgContextInitSettingsD3D12 d3d12;

	// [Optional] Vulkan specific settings
	ZgContextInitSettingsVulkan vulkan;
};

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
	u32 width,
	u32 height);

// Sets whether VSync should be enabled or not. Should be cheap and safe to call every frame if
// wanted.
ZG_API ZgResult zgContextSwapchainSetVsync(
	ZgBool vsync);

// The framebuffer returned is owned by the swapchain and can't be released by the user. It is
// still safe to call zgFramebufferRelease() on it, but it will be a no-op and the framebuffer
// will still be valid afterwards.
//
// Optionally a profiler can be specified in order to measure the time it takes to render the
// frame. Specify nullptr for the profiler and measurementIdOut if you don't intend to measure
// performance.
ZG_API ZgResult zgContextSwapchainBeginFrame(
	ZgFramebuffer** framebufferOut,
	ZgProfiler* profiler,
	u64* measurementIdOut);

// Only specify profiler if you specified one to the begin frame call. Otherwise send in nullptr
// and 0.
ZG_API ZgResult zgContextSwapchainFinishFrame(
	ZgProfiler* profiler,
	u64 measurementId);

// A struct containing general statistics
sfz_struct(ZgStats) {

	// Total amount of dedicated GPU memory (in bytes) available on the GPU
	u64 dedicatedGpuMemoryBytes;

	// Total amount of dedicated CPU memory (in bytes) for this GPU. I.e. CPU memory dedicated for
	// the GPU and not directly usable by the CPU.
	u64 dedicatedCpuMemoryBytes;

	// Total amount of shared CPU memory (in bytes) for this GPU. I.e. CPU memory shared between
	// the CPU and GPU.
	u64 sharedCpuMemoryBytes;

	// The amount of "fast local" memory provided to the application by the OS. If more memory is
	// used then stuttering and swapping could occur among other things.
	u64 memoryBudgetBytes;

	// The amount of "fast local" memory used by the application. This will correspond to GPU
	// memory on a device with dedicated GPU memory.
	u64 memoryUsageBytes;

	// The amount of "non-local" memory provided to the application by the OS.
	u64 nonLocalBugetBytes;

	// The amount of "non-local" memory used by the application.
	u64 nonLocalUsageBytes;
};

// Gets the current statistics of the ZeroG backend. Normally called once (or maybe up to a couple
// of times) per frame.
ZG_API ZgResult zgContextGetStats(ZgStats* statsOut);

sfz_struct(ZgFeatureSupport) {

	// Text description (i.e. name) of the device in use
	char deviceDescription[128];

	// D3D12: Highest supported shader model level
	ZgShaderModel shaderModel;

	// D3D12: Resource Binding Tier
	// See: https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
	char resourceBindingTier[8];

	// D3D12: Resource Heap Tier
	// See: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_heap_tier
	char resourceHeapTier[8];

	// D3D12: Whether shader dynamic resources are supported or not. These are required to be
	//        supported if the device supports both shader model 6.6 and resource binding tier 3.
	// See: https://github.com/microsoft/DirectX-Specs/blob/master/d3d/HLSL_SM_6_6_DynamicResources.md
	ZgBool shaderDynamicResources;

	// D3D12: Whether wave operations (i.e. "shuffle" in CUDA, reading other threads memory in
	//        same wavefront) is supported.
	ZgBool waveOps;

	// D3D12: Minimum and maximum amount of lanes/threads per wave (wavefront, warp).
	u32 waveMinLaneCount;
	u32 waveMaxLaneCount;

	// D3D12: Total number of lanes, or threads, of the GPU.
	u32 gpuTotalLaneCount;

	// D3D12: Whether 16-bit operations are natively supported in shaders
	ZgBool shader16bitOps;

	// D3D12: Whether raytracing is supported and at what tier
	ZgBool raytracing;
	char raytracingTier[16];

	// D3D12: Whether variable shading rate is supported and at what tier.
	ZgBool variableShadingRate;
	char variableShadingRateTier[8];

	// D3D12: Variable shading rate tile size for Tier 2.
	// See: https://docs.microsoft.com/en-us/windows/win32/direct3d12/vrs
	u32 variableShadingRateTileSize;

	// D3D12: Whether mesh shaders are supported
	ZgBool meshShaders;
};

ZG_API ZgResult zgContextGetFeatureSupport(ZgFeatureSupport* featureSupportOut);

// Transformation and projection matrices
// ------------------------------------------------------------------------------------------------

// These are some helper functions to generate the standard transform and projection matrices you
// typically want to use with ZeroG.
//
// The inclusion of these might seem a bit out of place compared to the other stuff here, however
// when looking around I see quite a bit of confusion regarding these matrices. I figure I will save
// myself and others quite a bit of time by providing reasonable defaults that should cover most use
// cases.
//
// All matrices returned are 4x4 row-major matrices (i.e. column vectors). If passed directly into
// HLSL the "f324x4" primitive must be marked "row_major", otherwise the matrix will get
// transposed during the transfer and you will not get the results you expect.
//
// The zgUtilCreateViewMatrix() function creates a view matrix similar to the one typically used
// in OpenGL. In other words, right-handed coordinate system with x to the right, y up and z
// towards the camera (negative z into the scene). This is the kind of view matrix that is expected
// for all the projection matrices here.
//
// The are a couple of variants of the projection matrices, normal, "reverse" and "infinite".
//
// Reverse simply means that it uses reversed z (i.e. 1.0 is closest to camera, 0.0 is furthest away).
// This can greatly improve the precision of the depth buffer, see:
// * https://developer.nvidia.com/content/depth-precision-visualized
// * http://dev.theomader.com/depth-precision/
// * https://mynameismjp.wordpress.com/2010/03/22/attack-of-the-depth-buffer/
// Of course, if you are using reverse projection you must also change your depth function from
// "ZG_DEPTH_FUNC_LESS" to "ZG_DEPTH_FUNC_GREATER".
//
// Infinite means that the far plane is at infinity instead of at a fixed distance away from the camera.
// Somewhat counter intuitively, this does not reduce the precision of the depth buffer all that much.
// Because the depth buffer is logarithmic, mainly the distance to the near plane affects precision.
// Setting the far plane to infinity gives you one less thing to think about and simplifies the
// actual projection matrix a bit.
//
// If unsure I would recommend starting out with the basic zgUtilCreatePerspectiveProjection() and
// then switching to zgUtilCreatePerspectiveProjectionReverseInfinite() when feeling more confident.

ZG_API void zgUtilCreateViewMatrix(
	f32 rowMajorMatrixOut[16],
	const f32 origin[3],
	const f32 dir[3],
	const f32 up[3]);

ZG_API void zgUtilCreatePerspectiveProjection(
	f32 rowMajorMatrixOut[16],
	f32 vertFovDegs,
	f32 aspect,
	f32 nearPlane,
	f32 farPlane);

ZG_API void zgUtilCreatePerspectiveProjectionInfinite(
	f32 rowMajorMatrixOut[16],
	f32 vertFovDegs,
	f32 aspect,
	f32 nearPlane);

ZG_API void zgUtilCreatePerspectiveProjectionReverse(
	f32 rowMajorMatrixOut[16],
	f32 vertFovDegs,
	f32 aspect,
	f32 nearPlane,
	f32 farPlane);

ZG_API void zgUtilCreatePerspectiveProjectionReverseInfinite(
	f32 rowMajorMatrixOut[16],
	f32 vertFovDegs,
	f32 aspect,
	f32 nearPlane);

ZG_API void zgUtilCreateOrthographicProjection(
	f32 rowMajorMatrixOut[16],
	f32 width,
	f32 height,
	f32 nearPlane,
	f32 farPlane);

ZG_API void zgUtilCreateOrthographicProjectionReverse(
	f32 rowMajorMatrixOut[16],
	f32 width,
	f32 height,
	f32 nearPlane,
	f32 farPlane);

#endif
