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

#include <stdint.h>

#ifdef __cplusplus
#include <assert.h>
#include <utility> // std::swap()
#endif

#ifdef __cplusplus
#define ZG_EXTERN_C extern "C"
#else
#define ZG_EXTERN_C
#endif

#if defined(_WIN32)
#if defined(ZG_DLL_EXPORT)
#define ZG_API ZG_EXTERN_C __declspec(dllexport)
#else
#define ZG_API ZG_EXTERN_C __declspec(dllimport)
#endif
#else
#define ZG_API
#endif

// Macro to declare a ZeroG handle. As a user you can never dereference or inspect a ZeroG handle,
// you can only store pointers to them.
#define ZG_HANDLE(name) \
	struct name; \
	typedef struct name name

// Macro to simplify the creation of C structs. In C you have to typedef the struct to its name in
// order to not have to write out struct each time you use it. This macro is used to enable us to
// only specify the struct name once, at the top of the struct declaration, while still getting the
// typedef done.
#define ZG_STRUCT(name) \
	struct name; \
	typedef struct name name; \
	struct name

// Macro to simplify the creation of C enums. Similar to ZG_STRUCT(), except all ZeroG enums have
// the type int32_t.
#define ZG_ENUM(name) \
	typedef int32_t name; \
	enum name ## Enum

// ZeroG handles
// ------------------------------------------------------------------------------------------------

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

// A handle representing a profiler
ZG_HANDLE(ZgProfiler);

// Bool
// ------------------------------------------------------------------------------------------------

// The ZeroG bool type.
ZG_ENUM(ZgBool) {
	ZG_FALSE = 0,
	ZG_TRUE = 1
};

// Framebuffer rectangle
// ------------------------------------------------------------------------------------------------

ZG_STRUCT(ZgFramebufferRect) {
	uint32_t topLeftX, topLeftY, width, height;
};

// Version information
// ------------------------------------------------------------------------------------------------

// The API version used to compile ZeroG.
static const uint32_t ZG_COMPILED_API_VERSION = 20;

// Returns the API version of the ZeroG DLL you have linked with
//
// As long as the DLL has the same API version as the version you compiled with it should be
// compatible.
ZG_API uint32_t zgApiLinkedVersion(void);

// Backends
// ------------------------------------------------------------------------------------------------

// The various backends supported by ZeroG
ZG_ENUM(ZgBackendType) {
	ZG_BACKEND_NONE = 0,
	ZG_BACKEND_D3D12,
	ZG_BACKEND_VULKAN
};

// Returns the backend compiled into this API
ZG_API ZgBackendType zgBackendCompiledType(void);

// Results
// ------------------------------------------------------------------------------------------------

// The results, 0 is success, negative values are errors and positive values are warnings.
ZG_ENUM(ZgResult) {

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

// Returns a string representation of the given ZeroG result. The string is statically allocated
// and must NOT be freed by the user.
ZG_API const char* zgResultToString(ZgResult errorCode);

#ifdef __cplusplus
namespace zg {

enum class [[nodiscard]] Result : ZgResult {

	SUCCESS = ZG_SUCCESS,

	WARNING_GENERIC = ZG_WARNING_GENERIC,
	WARNING_UNIMPLEMENTED = ZG_WARNING_UNIMPLEMENTED,
	WARNING_ALREADY_INITIALIZED = ZG_WARNING_ALREADY_INITIALIZED,

	GENERIC = ZG_ERROR_GENERIC,
	CPU_OUT_OF_MEMORY = ZG_ERROR_CPU_OUT_OF_MEMORY,
	GPU_OUT_OF_MEMORY = ZG_ERROR_GPU_OUT_OF_MEMORY,
	NO_SUITABLE_DEVICE = ZG_ERROR_NO_SUITABLE_DEVICE,
	INVALID_ARGUMENT = ZG_ERROR_INVALID_ARGUMENT,
	SHADER_COMPILE_ERROR = ZG_ERROR_SHADER_COMPILE_ERROR,
	OUT_OF_COMMAND_LISTS = ZG_ERROR_OUT_OF_COMMAND_LISTS,
	INVALID_COMMAND_LIST_STATE = ZG_ERROR_INVALID_COMMAND_LIST_STATE
};

inline const char* toString(Result res) { return zgResultToString(ZgResult(res)); }
constexpr bool isSuccess(Result res) { return res == Result::SUCCESS; }
constexpr bool isWarning(Result res) { return ZgResult(res) > 0; }
constexpr bool isError(Result res) { return ZgResult(res) < 0; }

} // namespace zg
#endif

// Buffer
// ------------------------------------------------------------------------------------------------

ZG_STRUCT(ZgBufferCreateInfo) {

	// The offset from the start of the memory heap to create the buffer at.
	// Note that the offset must be a multiple of 64KiB (= 2^16 bytes = 65 536 bytes), or 0.
	uint64_t offsetInBytes;

	// The size in bytes of the buffer
	uint64_t sizeInBytes;
};

ZG_API ZgResult zgMemoryHeapBufferCreate(
	ZgMemoryHeap* memoryHeap,
	ZgBuffer** bufferOut,
	const ZgBufferCreateInfo* createInfo);

ZG_API void zgBufferRelease(
	ZgBuffer* buffer);

ZG_API ZgResult zgBufferMemcpyTo(
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	const void* srcMemory,
	uint64_t numBytes);

ZG_API ZgResult zgBufferMemcpyFrom(
	void* dstMemory,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes);

ZG_API ZgResult zgBufferSetDebugName(
	ZgBuffer* buffer,
	const char* name);

#ifdef __cplusplus
namespace zg {

class Buffer final {
public:
	ZgBuffer* buffer = nullptr;

	Buffer() = default;
	Buffer(const Buffer&) = delete;
	Buffer& operator= (const Buffer&) = delete;
	Buffer(Buffer&& o) { this->swap(o); }
	Buffer& operator= (Buffer&& o) { this->swap(o); return *this; }
	~Buffer() { this->release(); }

	bool valid() const { return buffer != nullptr; }

	void swap(Buffer& other)
	{
		std::swap(this->buffer, other.buffer);
	}

	void release()
	{
		if (this->buffer != nullptr) {
			zgBufferRelease(this->buffer);
		}
		this->buffer = nullptr;
	}

	Result memcpyTo(uint64_t bufferOffsetBytes, const void* srcMemory, uint64_t numBytes)
	{
		return (Result)zgBufferMemcpyTo(this->buffer, bufferOffsetBytes, srcMemory, numBytes);
	}

	Result memcpyFrom(void* dstMemory, uint64_t srcBufferOffsetBytes, uint64_t numBytes)
	{
		return (Result)zgBufferMemcpyFrom(dstMemory, this->buffer, srcBufferOffsetBytes, numBytes);
	}

	Result setDebugName(const char* name)
	{
		return (Result)zgBufferSetDebugName(this->buffer, name);
	}
};

} // namespace zg
#endif

// Textures
// ------------------------------------------------------------------------------------------------

static const uint32_t ZG_MAX_NUM_MIPMAPS = 12;

ZG_ENUM(ZgTextureFormat) {
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

ZG_ENUM(ZgTextureUsage) {
	ZG_TEXTURE_USAGE_DEFAULT = 0,
	ZG_TEXTURE_USAGE_RENDER_TARGET,
	ZG_TEXTURE_USAGE_DEPTH_BUFFER
};

ZG_ENUM(ZgOptimalClearValue) {
	ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED = 0,
	ZG_OPTIMAL_CLEAR_VALUE_ZERO,
	ZG_OPTIMAL_CLEAR_VALUE_ONE
};

ZG_STRUCT(ZgTexture2DCreateInfo) {

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

ZG_STRUCT(ZgTexture2DAllocationInfo) {

	// The size of the texture in bytes
	uint32_t sizeInBytes;

	// The alignment of the texture in bytes
	uint32_t alignmentInBytes;
};

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

#ifdef __cplusplus
namespace zg {

class Texture2D final {
public:
	ZgTexture2D* texture = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;

	Texture2D() noexcept = default;
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator= (const Texture2D&) = delete;
	Texture2D(Texture2D&& o) noexcept { this->swap(o); }
	Texture2D& operator= (Texture2D&& o) noexcept { this->swap(o); return *this; }
	~Texture2D() noexcept { this->release(); }

	bool valid() const noexcept { return this->texture != nullptr; }

	void swap(Texture2D& other) noexcept
	{
		std::swap(this->texture, other.texture);
		std::swap(this->width, other.width);
		std::swap(this->height, other.height);
	}

	void release() noexcept
	{
		if (this->texture != nullptr) zgTexture2DRelease(this->texture);
		this->texture = nullptr;
		this->width = 0;
		this->height = 0;
	}

	static Result getAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept
	{
		return (Result)zgTexture2DGetAllocationInfo(&allocationInfoOut, &createInfo);
	}

	Result setDebugName(const char* name) noexcept
	{
		return (Result)zgTexture2DSetDebugName(this->texture, name);
	}
};

} // namespace zg
#endif

// Memory Heap
// ------------------------------------------------------------------------------------------------

ZG_ENUM(ZgMemoryType) {
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

ZG_STRUCT(ZgMemoryHeapCreateInfo) {

	// The size in bytes of the heap
	uint64_t sizeInBytes;

	// The type of memory
	ZgMemoryType memoryType;
};

ZG_API ZgResult zgMemoryHeapCreate(
	ZgMemoryHeap** memoryHeapOut,
	const ZgMemoryHeapCreateInfo* createInfo);

ZG_API ZgResult zgMemoryHeapRelease(
	ZgMemoryHeap* memoryHeap);

#ifdef __cplusplus
namespace zg {

class MemoryHeap final {
public:
	ZgMemoryHeap* memoryHeap = nullptr;

	MemoryHeap() noexcept = default;
	MemoryHeap(const MemoryHeap&) = delete;
	MemoryHeap& operator= (const MemoryHeap&) = delete;
	MemoryHeap(MemoryHeap&& o) noexcept { this->swap(o); }
	MemoryHeap& operator= (MemoryHeap&& o) noexcept { this->swap(o); return *this; }
	~MemoryHeap() noexcept { this->release(); }

	bool valid() const noexcept { return memoryHeap != nullptr; }

	Result create(const ZgMemoryHeapCreateInfo& createInfo) noexcept
	{
		this->release();
		return (Result)zgMemoryHeapCreate(&this->memoryHeap, &createInfo);
	}

	Result create(uint64_t sizeInBytes, ZgMemoryType memoryType) noexcept
	{
		ZgMemoryHeapCreateInfo createInfo = {};
		createInfo.sizeInBytes = sizeInBytes;
		createInfo.memoryType = memoryType;
		return this->create(createInfo);
	}

	void swap(MemoryHeap& other) noexcept
	{
		std::swap(this->memoryHeap, other.memoryHeap);
	}

	void release() noexcept
	{
		if (this->memoryHeap != nullptr) zgMemoryHeapRelease(this->memoryHeap);
		this->memoryHeap = nullptr;
	}

	Result bufferCreate(Buffer& bufferOut, const ZgBufferCreateInfo& createInfo) noexcept
	{
		bufferOut.release();
		return (Result)zgMemoryHeapBufferCreate(this->memoryHeap, &bufferOut.buffer, &createInfo);
	}

	Result bufferCreate(Buffer& bufferOut, uint64_t offset, uint64_t size) noexcept
	{
		ZgBufferCreateInfo createInfo = {};
		createInfo.offsetInBytes = offset;
		createInfo.sizeInBytes = size;
		return this->bufferCreate(bufferOut, createInfo);
	}

	Result texture2DCreate(
		Texture2D& textureOut, const ZgTexture2DCreateInfo& createInfo) noexcept
	{
		textureOut.release();
		Result res = (Result)zgMemoryHeapTexture2DCreate(
			this->memoryHeap, &textureOut.texture, &createInfo);
		if (isSuccess(res)) {
			textureOut.width = createInfo.width;
			textureOut.height = createInfo.height;
		}
		return res;
	}
};

} // namespace zg
#endif

// Pipeline Bindings
// ------------------------------------------------------------------------------------------------

// The maximum number of constant buffers allowed on a single pipeline.
static const uint32_t ZG_MAX_NUM_CONSTANT_BUFFERS = 16;

// The maximum number of unordered buffers allowed on a single pipeline.
static const uint32_t ZG_MAX_NUM_UNORDERED_BUFFERS = 16;

// The maximum number of textures allowed on a single pipeline.
static const uint32_t ZG_MAX_NUM_TEXTURES = 16;

// The maximum number of unordered textures on a single pipeline.
static const uint32_t ZG_MAX_NUM_UNORDERED_TEXTURES = 16;

// The maximum number of samplers allowed on a single pipeline.
static const uint32_t ZG_MAX_NUM_SAMPLERS = 8;

ZG_STRUCT(ZgConstantBufferBindingDesc) {

	// Which register this buffer corresponds to in the shader. In D3D12 this is the "register"
	// keyword, i.e. a value of 0 would mean "register(b0)".
	uint32_t bufferRegister;

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

ZG_STRUCT(ZgUnorderedBufferBindingDesc) {

	// Which register this buffer corresponds to in the shader.In D3D12 this is the "register"
	// keyword, i.e. a value of 0 would mean "register(u0)".
	uint32_t unorderedRegister;
};

ZG_STRUCT(ZgTextureBindingDesc) {

	// Which register this texture corresponds to in the shader.
	uint32_t textureRegister;
};

ZG_STRUCT(ZgUnorderedTextureBindingDesc) {

	// Which register this texture corresponds to in the shader.
	uint32_t unorderedRegister;
};

// A struct representing the signature of a pipeline, indicating what resources can be bound to it.
//
// The signature contains all information necessary to know how to bind input and output to a
// pipeline. This information is inferred by performing reflection on the shaders being compiled.
ZG_STRUCT(ZgPipelineBindingsSignature) {

	// The constant buffers
	uint32_t numConstBuffers;
	ZgConstantBufferBindingDesc constBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// The unordered buffers
	uint32_t numUnorderedBuffers;
	ZgUnorderedBufferBindingDesc unorderedBuffers[ZG_MAX_NUM_UNORDERED_BUFFERS];

	// The textures
	uint32_t numTextures;
	ZgTextureBindingDesc textures[ZG_MAX_NUM_TEXTURES];

	// The unordered textures
	uint32_t numUnorderedTextures;
	ZgUnorderedTextureBindingDesc unorderedTextures[ZG_MAX_NUM_UNORDERED_TEXTURES];
};

ZG_STRUCT(ZgConstantBufferBinding) {
	uint32_t bufferRegister;
	ZgBuffer* buffer;
};

ZG_STRUCT(ZgUnorderedBufferBinding) {
	
	// Register the unordered buffer is bound to
	uint32_t unorderedRegister;

	// The first element in the buffer (0 to bind from start of buffer)
	uint32_t firstElementIdx;
	
	// The number of elements to bind in the buffer
	uint32_t numElements;

	// The stride (size most of the time) between elements in the buffer in bytes
	uint32_t elementStrideBytes;

	// The buffer to bind as an unordered buffer
	ZgBuffer* buffer;
};

ZG_STRUCT(ZgTextureBinding) {
	uint32_t textureRegister;
	ZgTexture2D* texture;
};

ZG_STRUCT(ZgUnorderedTextureBinding) {

	// Register the unordered texture is bound to
	uint32_t unorderedRegister;

	// The mip level to bind
	uint32_t mipLevel;

	// The texture to bind as an unordered texture
	ZgTexture2D* texture;
};

ZG_STRUCT(ZgPipelineBindings) {

	// The constant buffers to bind
	uint32_t numConstantBuffers;
	ZgConstantBufferBinding constantBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// The unordered buffers to bind
	uint32_t numUnorderedBuffers;
	ZgUnorderedBufferBinding unorderedBuffers[ZG_MAX_NUM_UNORDERED_BUFFERS];

	// The textures to bind
	uint32_t numTextures;
	ZgTextureBinding textures[ZG_MAX_NUM_TEXTURES];

	// The unordered textures to bind
	uint32_t numUnorderedTextures;
	ZgUnorderedTextureBinding unorderedTextures[ZG_MAX_NUM_UNORDERED_TEXTURES];
};

#ifdef __cplusplus
namespace zg {

class PipelineBindings final {
public:
	ZgPipelineBindings bindings = {};

	PipelineBindings() noexcept = default;
	PipelineBindings(const PipelineBindings&) noexcept = default;
	PipelineBindings& operator= (const PipelineBindings&) noexcept = default;
	~PipelineBindings() noexcept = default;

	PipelineBindings& addConstantBuffer(ZgConstantBufferBinding binding) noexcept
	{
		assert(bindings.numConstantBuffers < ZG_MAX_NUM_CONSTANT_BUFFERS);
		bindings.constantBuffers[bindings.numConstantBuffers] = binding;
		bindings.numConstantBuffers += 1;
		return *this;
	}

	PipelineBindings& addConstantBuffer(uint32_t bufferRegister, Buffer& buffer) noexcept
	{
		ZgConstantBufferBinding binding = {};
		binding.bufferRegister = bufferRegister;
		binding.buffer = buffer.buffer;
		return this->addConstantBuffer(binding);
	}

	PipelineBindings& addUnorderedBuffer(ZgUnorderedBufferBinding binding) noexcept
	{
		assert(bindings.numUnorderedBuffers < ZG_MAX_NUM_UNORDERED_BUFFERS);
		bindings.unorderedBuffers[bindings.numUnorderedBuffers] = binding;
		bindings.numUnorderedBuffers += 1;
		return *this;
	}

	PipelineBindings& addUnorderedBuffer(
		uint32_t unorderedRegister,
		uint32_t numElements,
		uint32_t elementStrideBytes,
		Buffer& buffer) noexcept
	{
		return this->addUnorderedBuffer(unorderedRegister, 0, numElements, elementStrideBytes, buffer);
	}

	PipelineBindings& addUnorderedBuffer(
		uint32_t unorderedRegister,
		uint32_t firstElementIdx,
		uint32_t numElements,
		uint32_t elementStrideBytes,
		Buffer& buffer) noexcept
	{
		ZgUnorderedBufferBinding binding = {};
		binding.unorderedRegister = unorderedRegister;
		binding.firstElementIdx = firstElementIdx;
		binding.numElements = numElements;
		binding.elementStrideBytes = elementStrideBytes;
		binding.buffer = buffer.buffer;
		return this->addUnorderedBuffer(binding);
	}

	PipelineBindings& addTexture(ZgTextureBinding binding) noexcept
	{
		assert(bindings.numTextures < ZG_MAX_NUM_TEXTURES);
		bindings.textures[bindings.numTextures] = binding;
		bindings.numTextures += 1;
		return *this;
	}

	PipelineBindings& addTexture(uint32_t textureRegister, Texture2D& texture) noexcept
	{
		ZgTextureBinding binding;
		binding.textureRegister = textureRegister;
		binding.texture = texture.texture;
		return this->addTexture(binding);
	}

	PipelineBindings& addUnorderedTexture(ZgUnorderedTextureBinding binding) noexcept
	{
		assert(bindings.numUnorderedTextures < ZG_MAX_NUM_UNORDERED_TEXTURES);
		bindings.unorderedTextures[bindings.numUnorderedTextures] = binding;
		bindings.numUnorderedTextures += 1;
		return *this;
	}

	PipelineBindings& addUnorderedTexture(
		uint32_t unorderedRegister,
		uint32_t mipLevel,
		Texture2D& texture) noexcept
	{
		ZgUnorderedTextureBinding binding = {};
		binding.unorderedRegister = unorderedRegister;
		binding.mipLevel = mipLevel;
		binding.texture = texture.texture;
		return this->addUnorderedTexture(binding);
	}
};

} // namespace zg
#endif

// Pipeline Compiler Settings
// ------------------------------------------------------------------------------------------------

// Enum representing various shader model versions
ZG_ENUM(ZgShaderModel) {
	ZG_SHADER_MODEL_UNDEFINED = 0,
	ZG_SHADER_MODEL_6_0,
	ZG_SHADER_MODEL_6_1,
	ZG_SHADER_MODEL_6_2,
	ZG_SHADER_MODEL_6_3
};

// The maximum number of compiler flags allowed to the DXC shader compiler
static const uint32_t ZG_MAX_NUM_DXC_COMPILER_FLAGS = 8;

// Compile settings to the HLSL compiler
ZG_STRUCT(ZgPipelineCompileSettingsHLSL) {

	// Which shader model to target when compiling the HLSL file
	ZgShaderModel shaderModel;

	// Flags to the DXC compiler
	const char* dxcCompilerFlags[ZG_MAX_NUM_DXC_COMPILER_FLAGS];
};

// Pipeline Compute
// ------------------------------------------------------------------------------------------------

// Sample mode of a sampler
ZG_ENUM(ZgSamplingMode) {
	ZG_SAMPLING_MODE_UNDEFINED = 0,

	ZG_SAMPLING_MODE_NEAREST, // D3D12_FILTER_MIN_MAG_MIP_POINT
	ZG_SAMPLING_MODE_TRILINEAR, // D3D12_FILTER_MIN_MAG_MIP_LINEAR
	ZG_SAMPLING_MODE_ANISOTROPIC, // D3D12_FILTER_ANISOTROPIC
};

// Wrapping mode of a sampler
ZG_ENUM(ZgWrappingMode) {
	ZG_WRAPPING_MODE_UNDEFINED = 0,

	ZG_WRAPPING_MODE_CLAMP, // D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	ZG_WRAPPING_MODE_REPEAT, // D3D12_TEXTURE_ADDRESS_MODE_WRAP
};

// A struct defining a texture sampler
ZG_STRUCT(ZgSampler) {

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

ZG_STRUCT(ZgPipelineComputeCreateInfo) {

	// Path to the shader source or the source directly depending on what create function is called.
	const char* computeShader;

	// The name of the entry function
	const char* computeShaderEntry;

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

// A struct representing the compute signature of a compute pipeline.
ZG_STRUCT(ZgPipelineComputeSignature) {

	// The dimensions of the thread group launched by this compute pipeline.
	uint32_t groupDimX;
	uint32_t groupDimY;
	uint32_t groupDimZ;
};

ZG_API ZgResult zgPipelineComputeCreateFromFileHLSL(
	ZgPipelineCompute** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineComputeSignature* computeSignatureOut,
	const ZgPipelineComputeCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings);

ZG_API ZgResult zgPipelineComputeRelease(
	ZgPipelineCompute* pipeline);

#ifdef __cplusplus
namespace zg {

class PipelineCompute final {
public:
	ZgPipelineCompute* pipeline = nullptr;
	ZgPipelineBindingsSignature bindingsSignature = {};
	ZgPipelineComputeSignature computeSignature = {};

	PipelineCompute() noexcept = default;
	PipelineCompute(const PipelineCompute&) = delete;
	PipelineCompute& operator= (const PipelineCompute&) = delete;
	PipelineCompute(PipelineCompute&& o) noexcept { this->swap(o); }
	PipelineCompute& operator= (PipelineCompute&& o) noexcept { this->swap(o); return *this; }
	~PipelineCompute() noexcept { this->release(); }

	bool valid() const noexcept { return this->pipeline != nullptr; }

	Result createFromFileHLSL(
		const ZgPipelineComputeCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
	{
		this->release();
		return (Result)zgPipelineComputeCreateFromFileHLSL(
			&this->pipeline, &this->bindingsSignature, &this->computeSignature, &createInfo, &compileSettings);

	}

	void swap(PipelineCompute& other) noexcept
	{
		std::swap(this->pipeline, other.pipeline);
		std::swap(this->bindingsSignature, other.bindingsSignature);
		std::swap(this->computeSignature, other.computeSignature);
	}

	void release() noexcept
	{
		if (this->pipeline != nullptr) zgPipelineComputeRelease(this->pipeline);
		this->pipeline = nullptr;
		this->bindingsSignature = {};
		this->computeSignature = {};
	}
};

class PipelineComputeBuilder final {
public:
	ZgPipelineComputeCreateInfo createInfo = {};
	const char* computeShaderPath = nullptr;
	const char* computeShaderSrc = nullptr;

	PipelineComputeBuilder() noexcept = default;
	PipelineComputeBuilder(const PipelineComputeBuilder&) noexcept = default;
	PipelineComputeBuilder& operator= (const PipelineComputeBuilder&) noexcept = default;
	~PipelineComputeBuilder() noexcept = default;

	PipelineComputeBuilder& addComputeShaderPath(const char* entry, const char* path) noexcept
	{
		createInfo.computeShaderEntry = entry;
		computeShaderPath = path;
		return *this;
	}

	PipelineComputeBuilder& addComputeShaderSource(const char* entry, const char* src) noexcept
	{
		createInfo.computeShaderEntry = entry;
		computeShaderSrc = src;
		return *this;
	}

	PipelineComputeBuilder& addPushConstant(uint32_t constantBufferRegister) noexcept
	{
		assert(createInfo.numPushConstants < ZG_MAX_NUM_CONSTANT_BUFFERS);
		createInfo.pushConstantRegisters[createInfo.numPushConstants] = constantBufferRegister;
		createInfo.numPushConstants += 1;
		return *this;
	}

	PipelineComputeBuilder& addSampler(uint32_t samplerRegister, ZgSampler sampler) noexcept
	{
		assert(samplerRegister == createInfo.numSamplers);
		assert(createInfo.numSamplers < ZG_MAX_NUM_SAMPLERS);
		createInfo.samplers[samplerRegister] = sampler;
		createInfo.numSamplers += 1;
		return *this;
	}

	PipelineComputeBuilder& addSampler(
		uint32_t samplerRegister,
		ZgSamplingMode samplingMode,
		ZgWrappingMode wrappingModeU = ZG_WRAPPING_MODE_CLAMP,
		ZgWrappingMode wrappingModeV = ZG_WRAPPING_MODE_CLAMP,
		float mipLodBias = 0.0f) noexcept
	{
		ZgSampler sampler = {};
		sampler.samplingMode = samplingMode;
		sampler.wrappingModeU = wrappingModeU;
		sampler.wrappingModeV = wrappingModeV;
		sampler.mipLodBias = mipLodBias;
		return addSampler(samplerRegister, sampler);
	}

	Result buildFromFileHLSL(
		PipelineCompute& pipelineOut, ZgShaderModel model = ZG_SHADER_MODEL_6_0) noexcept
	{
		// Set path
		createInfo.computeShader = this->computeShaderPath;

		// Create compile settings
		ZgPipelineCompileSettingsHLSL compileSettings = {};
		compileSettings.shaderModel = model;
		compileSettings.dxcCompilerFlags[0] = "-Zi";
		compileSettings.dxcCompilerFlags[1] = "-O3";

		// Build pipeline
		return pipelineOut.createFromFileHLSL(createInfo, compileSettings);
	}
};

} // namespace zg
#endif

// Pipeline Render Signature
// ------------------------------------------------------------------------------------------------

// The maximum number of vertex attributes allowed as input to a vertex shader
static const uint32_t ZG_MAX_NUM_VERTEX_ATTRIBUTES = 8;

// The maximum number of render targets allowed on a single pipeline
static const uint32_t ZG_MAX_NUM_RENDER_TARGETS = 8;

// The type of data contained in a vertex
ZG_ENUM(ZgVertexAttributeType) {
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

// A struct defining a vertex attribute
ZG_STRUCT(ZgVertexAttribute) {
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

// A struct representing the rendering signature of a render pipeline.
//
// In contrast to ZgPipelineBindingsSignature, this specifies render pipeline specific bendings of
// a render pipeline. I.e. how vertex data is loaded and render targets being written to.
//
// Unlike ZgPipelineBindingsSignature, this data can not be inferred by reflection and is specified
// by the user when creating a pipeline anyway. It is mainly kept and provided in this form to make
// life a bit simpler for the user.
ZG_STRUCT(ZgPipelineRenderSignature) {

	// The vertex attributes to the vertex shader
	uint32_t numVertexAttributes;
	ZgVertexAttribute vertexAttributes[ZG_MAX_NUM_VERTEX_ATTRIBUTES];

	// Render targets
	uint32_t numRenderTargets;
	ZgTextureFormat renderTargets[ZG_MAX_NUM_RENDER_TARGETS];
};

// Pipeline Render
// ------------------------------------------------------------------------------------------------

ZG_STRUCT(ZgRasterizerSettings) {

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

ZG_ENUM(ZgBlendFunc) {
	ZG_BLEND_FUNC_ADD = 0,
	ZG_BLEND_FUNC_DST_SUB_SRC, // dst - src
	ZG_BLEND_FUNC_SRC_SUB_DST, // src - dst
	ZG_BLEND_FUNC_MIN,
	ZG_BLEND_FUNC_MAX
};

// See: https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_blend
ZG_ENUM(ZgBlendFactor) {
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

ZG_STRUCT(ZgBlendSettings) {

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

ZG_ENUM(ZgDepthFunc) {
	ZG_DEPTH_FUNC_LESS = 0,
	ZG_DEPTH_FUNC_LESS_EQUAL,
	ZG_DEPTH_FUNC_EQUAL,
	ZG_DEPTH_FUNC_NOT_EQUAL,
	ZG_DEPTH_FUNC_GREATER,
	ZG_DEPTH_FUNC_GREATER_EQUAL
};

ZG_STRUCT(ZgDepthTestSettings) {

	// Whether to enable to the depth test or not
	ZgBool depthTestEnabled;

	// What function to use for the depth test.
	//
	// Default is ZG_DEPTH_FUNC_LESS (0), i.e. pixels with a depth value less than what is stored
	// in the depth buffer is kept (in a "standard" 0 is near, 1 is furthest away depth buffer).
	ZgDepthFunc depthFunc;
};

// The common information required to create a render pipeline
ZG_STRUCT(ZgPipelineRenderCreateInfo) {

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

#ifdef __cplusplus
namespace zg {

class PipelineRender final {
public:
	ZgPipelineRender* pipeline = nullptr;
	ZgPipelineBindingsSignature bindingsSignature = {};
	ZgPipelineRenderSignature renderSignature = {};

	PipelineRender() noexcept = default;
	PipelineRender(const PipelineRender&) = delete;
	PipelineRender& operator= (const PipelineRender&) = delete;
	PipelineRender(PipelineRender&& o) noexcept { this->swap(o); }
	PipelineRender& operator= (PipelineRender&& o) noexcept { this->swap(o); return *this; }
	~PipelineRender() noexcept { this->release(); }

	bool valid() const noexcept { return this->pipeline != nullptr; }

	Result createFromFileSPIRV(
		const ZgPipelineRenderCreateInfo& createInfo) noexcept
	{
		this->release();
		return (Result)zgPipelineRenderCreateFromFileSPIRV(
			&this->pipeline, &this->bindingsSignature, &this->renderSignature, &createInfo);
	}

	Result createFromFileHLSL(
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
	{
		this->release();
		return (Result)zgPipelineRenderCreateFromFileHLSL(
			&this->pipeline, &this->bindingsSignature, &this->renderSignature, &createInfo, &compileSettings);
	}

	Result createFromSourceHLSL(
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
	{
		this->release();
		return (Result)zgPipelineRenderCreateFromSourceHLSL(
			&this->pipeline, &this->bindingsSignature, &this->renderSignature, &createInfo, &compileSettings);
	}

	void swap(PipelineRender& other) noexcept
	{
		std::swap(this->pipeline, other.pipeline);
		std::swap(this->bindingsSignature, other.bindingsSignature);
		std::swap(this->renderSignature, other.renderSignature);
	}

	void release() noexcept
	{
		if (this->pipeline != nullptr) zgPipelineRenderRelease(this->pipeline);
		this->pipeline = nullptr;
		this->bindingsSignature = {};
		this->renderSignature = {};
	}
};

class PipelineRenderBuilder final {
public:
	ZgPipelineRenderCreateInfo createInfo = {};
	const char* vertexShaderPath = nullptr;
	const char* pixelShaderPath = nullptr;
	const char* vertexShaderSrc = nullptr;
	const char* pixelShaderSrc = nullptr;

	PipelineRenderBuilder() noexcept = default;
	PipelineRenderBuilder(const PipelineRenderBuilder&) noexcept = default;
	PipelineRenderBuilder& operator= (const PipelineRenderBuilder&) noexcept = default;
	~PipelineRenderBuilder() noexcept = default;

	PipelineRenderBuilder& addVertexShaderPath(const char* entry, const char* path) noexcept
	{
		createInfo.vertexShaderEntry = entry;
		vertexShaderPath = path;
		return *this;
	}

	PipelineRenderBuilder& addPixelShaderPath(const char* entry, const char* path) noexcept
	{
		createInfo.pixelShaderEntry = entry;
		pixelShaderPath = path;
		return *this;
	}

	PipelineRenderBuilder& addVertexShaderSource(const char* entry, const char* src) noexcept
	{
		createInfo.vertexShaderEntry = entry;
		vertexShaderSrc = src;
		return *this;
	}

	PipelineRenderBuilder& addPixelShaderSource(const char* entry, const char* src) noexcept
	{
		createInfo.pixelShaderEntry = entry;
		pixelShaderSrc = src;
		return *this;
	}

	PipelineRenderBuilder& addVertexAttribute(ZgVertexAttribute attribute) noexcept
	{
		assert(createInfo.numVertexAttributes < ZG_MAX_NUM_VERTEX_ATTRIBUTES);
		createInfo.vertexAttributes[createInfo.numVertexAttributes] = attribute;
		createInfo.numVertexAttributes += 1;
		return *this;
	}

	PipelineRenderBuilder& addVertexAttribute(
		uint32_t location,
		uint32_t vertexBufferSlot,
		ZgVertexAttributeType type,
		uint32_t offsetInBuffer) noexcept
	{
		ZgVertexAttribute attribute = {};
		attribute.location = location;
		attribute.vertexBufferSlot = vertexBufferSlot;
		attribute.type = type;
		attribute.offsetToFirstElementInBytes = offsetInBuffer;
		return addVertexAttribute(attribute);
	}

	PipelineRenderBuilder& addVertexBufferInfo(
		uint32_t slot, uint32_t vertexBufferStrideBytes) noexcept
	{
		assert(slot == createInfo.numVertexBufferSlots);
		assert(createInfo.numVertexBufferSlots < ZG_MAX_NUM_VERTEX_ATTRIBUTES);
		createInfo.vertexBufferStridesBytes[slot] = vertexBufferStrideBytes;
		createInfo.numVertexBufferSlots += 1;
		return *this;
	}

	PipelineRenderBuilder& addPushConstant(uint32_t constantBufferRegister) noexcept
	{
		assert(createInfo.numPushConstants < ZG_MAX_NUM_CONSTANT_BUFFERS);
		createInfo.pushConstantRegisters[createInfo.numPushConstants] = constantBufferRegister;
		createInfo.numPushConstants += 1;
		return *this;
	}

	PipelineRenderBuilder& addSampler(uint32_t samplerRegister, ZgSampler sampler) noexcept
	{
		assert(samplerRegister == createInfo.numSamplers);
		assert(createInfo.numSamplers < ZG_MAX_NUM_SAMPLERS);
		createInfo.samplers[samplerRegister] = sampler;
		createInfo.numSamplers += 1;
		return *this;
	}

	PipelineRenderBuilder& addSampler(
		uint32_t samplerRegister,
		ZgSamplingMode samplingMode,
		ZgWrappingMode wrappingModeU = ZG_WRAPPING_MODE_CLAMP,
		ZgWrappingMode wrappingModeV = ZG_WRAPPING_MODE_CLAMP,
		float mipLodBias = 0.0f) noexcept
	{
		ZgSampler sampler = {};
		sampler.samplingMode = samplingMode;
		sampler.wrappingModeU = wrappingModeU;
		sampler.wrappingModeV = wrappingModeV;
		sampler.mipLodBias = mipLodBias;
		return addSampler(samplerRegister, sampler);
	}

	PipelineRenderBuilder& addRenderTarget(ZgTextureFormat format) noexcept
	{
		assert(createInfo.numRenderTargets < ZG_MAX_NUM_RENDER_TARGETS);
		createInfo.renderTargets[createInfo.numRenderTargets] = format;
		createInfo.numRenderTargets += 1;
		return *this;
	}

	PipelineRenderBuilder& setWireframeRendering(bool wireframeEnabled) noexcept
	{
		createInfo.rasterizer.wireframeMode = wireframeEnabled ? ZG_TRUE : ZG_FALSE;
		return *this;
	}

	PipelineRenderBuilder& setCullingEnabled(bool cullingEnabled) noexcept
	{
		createInfo.rasterizer.cullingEnabled = cullingEnabled ? ZG_TRUE : ZG_FALSE;
		return *this;
	}

	PipelineRenderBuilder& setCullMode(
		bool cullFrontFacing, bool fontFacingIsCounterClockwise = false) noexcept
	{
		createInfo.rasterizer.cullFrontFacing = cullFrontFacing ? ZG_TRUE : ZG_FALSE;
		createInfo.rasterizer.frontFacingIsCounterClockwise =
			fontFacingIsCounterClockwise ? ZG_TRUE : ZG_FALSE;
		return *this;
	}

	PipelineRenderBuilder& setDepthBias(
		int32_t bias, float biasSlopeScaled, float biasClamp = 0.0f) noexcept
	{
		createInfo.rasterizer.depthBias = bias;
		createInfo.rasterizer.depthBiasSlopeScaled = biasSlopeScaled;
		createInfo.rasterizer.depthBiasClamp = biasClamp;
		return *this;
	}

	PipelineRenderBuilder& setBlendingEnabled(bool blendingEnabled) noexcept
	{
		createInfo.blending.blendingEnabled = blendingEnabled ? ZG_TRUE : ZG_FALSE;
		return *this;
	}

	PipelineRenderBuilder& setBlendFuncColor(
		ZgBlendFunc func, ZgBlendFactor srcFactor, ZgBlendFactor dstFactor) noexcept
	{
		createInfo.blending.blendFuncColor = func;
		createInfo.blending.srcValColor = srcFactor;
		createInfo.blending.dstValColor = dstFactor;
		return *this;
	}

	PipelineRenderBuilder& setBlendFuncAlpha(
		ZgBlendFunc func, ZgBlendFactor srcFactor, ZgBlendFactor dstFactor) noexcept
	{
		createInfo.blending.blendFuncAlpha = func;
		createInfo.blending.srcValAlpha = srcFactor;
		createInfo.blending.dstValAlpha = dstFactor;
		return *this;
	}

	PipelineRenderBuilder& setDepthTestEnabled(bool depthTestEnabled) noexcept
	{
		createInfo.depthTest.depthTestEnabled = depthTestEnabled ? ZG_TRUE : ZG_FALSE;
		return *this;
	}

	PipelineRenderBuilder& setDepthFunc(ZgDepthFunc depthFunc) noexcept
	{
		createInfo.depthTest.depthFunc = depthFunc;
		return *this;
	}

	Result buildFromFileSPIRV(PipelineRender& pipelineOut) noexcept
	{
		// Set path
		createInfo.vertexShader = this->vertexShaderPath;
		createInfo.pixelShader = this->pixelShaderPath;

		// Build pipeline
		return pipelineOut.createFromFileSPIRV(createInfo);
	}

	Result buildFromFileHLSL(
		PipelineRender& pipelineOut, ZgShaderModel model = ZG_SHADER_MODEL_6_0) noexcept
	{
		// Set path
		createInfo.vertexShader = this->vertexShaderPath;
		createInfo.pixelShader = this->pixelShaderPath;

		// Create compile settings
		ZgPipelineCompileSettingsHLSL compileSettings = {};
		compileSettings.shaderModel = model;
		compileSettings.dxcCompilerFlags[0] = "-Zi";
		compileSettings.dxcCompilerFlags[1] = "-O3";

		// Build pipeline
		return pipelineOut.createFromFileHLSL(createInfo, compileSettings);
	}

	Result buildFromSourceHLSL(
		PipelineRender& pipelineOut, ZgShaderModel model = ZG_SHADER_MODEL_6_0) noexcept
	{
		// Set source
		createInfo.vertexShader = this->vertexShaderSrc;
		createInfo.pixelShader = this->pixelShaderSrc;

		// Create compile settings
		ZgPipelineCompileSettingsHLSL compileSettings = {};
		compileSettings.shaderModel = model;
		compileSettings.dxcCompilerFlags[0] = "-Zi";
		compileSettings.dxcCompilerFlags[1] = "-O3";

		// Build pipeline
		return pipelineOut.createFromSourceHLSL(createInfo, compileSettings);
	}
};

} // namespace zg
#endif

// Framebuffer
// ------------------------------------------------------------------------------------------------

ZG_STRUCT(ZgFramebufferCreateInfo) {

	// Render targets
	uint32_t numRenderTargets;
	ZgTexture2D* renderTargets[ZG_MAX_NUM_RENDER_TARGETS];

	// Depth buffer
	ZgTexture2D* depthBuffer;
};

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

#ifdef __cplusplus
namespace zg {

class Framebuffer final {
public:
	ZgFramebuffer* framebuffer = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;

	Framebuffer() noexcept = default;
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator= (const Framebuffer&) = delete;
	Framebuffer(Framebuffer&& other) noexcept { this->swap(other); }
	Framebuffer& operator= (Framebuffer&& other) noexcept { this->swap(other); return *this; }
	~Framebuffer() noexcept { this->release(); }

	bool valid() const noexcept { return this->framebuffer != nullptr; }

	Result create(const ZgFramebufferCreateInfo& createInfo) noexcept
	{
		this->release();
		Result res = (Result)zgFramebufferCreate(&this->framebuffer, &createInfo);
		if (!isSuccess(res)) return res;
		return (Result)zgFramebufferGetResolution(this->framebuffer, &this->width, &this->height);
	}

	void swap(Framebuffer& other) noexcept
	{
		std::swap(this->framebuffer, other.framebuffer);
		std::swap(this->width, other.width);
		std::swap(this->height, other.height);
	}

	void release() noexcept
	{
		if (this->framebuffer != nullptr) zgFramebufferRelease(this->framebuffer);
		this->framebuffer = nullptr;
		this->width = 0;
		this->height = 0;
	}
};

class FramebufferBuilder final {
public:
	ZgFramebufferCreateInfo createInfo = {};

	FramebufferBuilder() noexcept = default;
	FramebufferBuilder(const FramebufferBuilder&) noexcept = default;
	FramebufferBuilder& operator= (const FramebufferBuilder&) noexcept = default;
	~FramebufferBuilder() noexcept = default;

	FramebufferBuilder& addRenderTarget(Texture2D& renderTarget) noexcept
	{
		assert(createInfo.numRenderTargets < ZG_MAX_NUM_RENDER_TARGETS);
		uint32_t idx = createInfo.numRenderTargets;
		createInfo.numRenderTargets += 1;
		createInfo.renderTargets[idx] = renderTarget.texture;
		return *this;
	}

	FramebufferBuilder& setDepthBuffer(Texture2D& depthBuffer) noexcept
	{
		createInfo.depthBuffer = depthBuffer.texture;
		return *this;
	}

	Result build(Framebuffer& framebufferOut) noexcept
	{
		return framebufferOut.create(this->createInfo);
	}
};

} // namespace zg
#endif

// Profiler
// ------------------------------------------------------------------------------------------------

ZG_STRUCT(ZgProfilerCreateInfo) {
	
	// The number of measurements that this profiler can hold. Once this limit has been reached
	// older measurements will automatically be thrown out to make room for newer ones. In other
	// words, this should be at least "number of measurements per frame" times "number of frames
	// before syncing".
	uint32_t maxNumMeasurements;
};

ZG_API ZgResult zgProfilerCreate(
	ZgProfiler** profilerOut,
	const ZgProfilerCreateInfo* createInfo);

ZG_API void zgProfilerRelease(
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
	uint64_t measurementId,
	float* measurementMsOut);

#ifdef __cplusplus
namespace zg {

class Profiler final {
public:
	ZgProfiler* profiler = nullptr;

	Profiler() noexcept = default;
	Profiler(const Profiler&) = delete;
	Profiler& operator= (const Profiler&) = delete;
	Profiler(Profiler&& other) noexcept { this->swap(other); }
	Profiler& operator= (Profiler&& other) noexcept { this->swap(other); return *this; }
	~Profiler() noexcept { this->release(); }

	bool valid() const noexcept { return this->profiler != nullptr; }

	Result create(const ZgProfilerCreateInfo& createInfo) noexcept
	{
		this->release();
		return (Result)zgProfilerCreate(&this->profiler, &createInfo);
	}

	void swap(Profiler& other) noexcept
	{
		std::swap(this->profiler, other.profiler);
	}

	void release() noexcept
	{
		if (this->profiler != nullptr) zgProfilerRelease(this->profiler);
		this->profiler = nullptr;
	}

	Result getMeasurement(
		uint64_t measurementId,
		float& measurementMsOut) noexcept
	{
		return (Result)zgProfilerGetMeasurement(this->profiler, measurementId, &measurementMsOut);
	}
};

} // namespace zg
#endif

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

#ifdef __cplusplus
namespace zg {

class Fence final {
public:
	ZgFence* fence = nullptr;

	Fence() noexcept = default;
	Fence(const Fence&) = delete;
	Fence& operator= (const Fence&) = delete;
	Fence(Fence&& other) noexcept { this->swap(other); }
	Fence& operator= (Fence&& other) noexcept { this->swap(other); return *this; }
	~Fence() noexcept { this->release(); }

	bool valid() const noexcept { return this->fence != nullptr; }

	Result create() noexcept
	{
		this->release();
		return (Result)zgFenceCreate(&this->fence);
	}

	void swap(Fence& other) noexcept
	{
		std::swap(this->fence, other.fence);
	}

	void release() noexcept
	{
		if (this->fence != nullptr) zgFenceRelease(this->fence);
		this->fence = nullptr;
	}

	Result reset() noexcept
	{
		return (Result)zgFenceReset(this->fence);
	}

	Result checkIfSignaled(bool& fenceSignaledOut) const noexcept
	{
		ZgBool signaled = ZG_FALSE;
		Result res = (Result)zgFenceCheckIfSignaled(this->fence, &signaled);
		fenceSignaledOut = signaled == ZG_FALSE ? false : true;
		return res;
	}

	bool checkIfSignaled() const noexcept
	{
		bool signaled = false;
		[[maybe_unused]] Result res = this->checkIfSignaled(signaled);
		return signaled;
	}

	Result waitOnCpuBlocking() const noexcept
	{
		return (Result)zgFenceWaitOnCpuBlocking(this->fence);
	}
};

} // namespace zg
#endif

// Command list
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgCommandListMemcpyBufferToBuffer(
	ZgCommandList* commandList,
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes);

ZG_STRUCT(ZgImageViewConstCpu) {
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
	ZgTexture2D* texture);

// Inserts an UAV barrier for all unordered resources, meaning that all read/writes must finish
// before any new read/writes start.
// "The resource can be NULL, which indicates that any UAV access could require the barrier.
ZG_API ZgResult zgCommandListUnorderedBarrierAll(
	ZgCommandList* commandList);

ZG_API ZgResult zgCommandListDispatchCompute(
	ZgCommandList* commandList,
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ);

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

ZG_ENUM(ZgIndexBufferType) {
	ZG_INDEX_BUFFER_TYPE_UINT32 = 0,
	ZG_INDEX_BUFFER_TYPE_UINT16
};

ZG_API ZgResult zgCommandListSetIndexBuffer(
	ZgCommandList* commandList,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type);

ZG_API ZgResult zgCommandListSetVertexBuffer(
	ZgCommandList* commandList,
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBuffer);

ZG_API ZgResult zgCommandListDrawTriangles(
	ZgCommandList* commandList,
	uint32_t startVertexIndex,
	uint32_t numVertices);

ZG_API ZgResult zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandList,
	uint32_t startIndex,
	uint32_t numTriangles);

// Begins profiling operations from this point until zgCommandListProfileEnd() is called.
//
// "measurementIdOut" returns a unique identifier for this measurement. This identifier is later
// used to retrieve the measurement using zgProfilerGetMeasurement().
ZG_API ZgResult zgCommandListProfileBegin(
	ZgCommandList* commandList,
	ZgProfiler* profiler,
	uint64_t* measurementIdOut);

ZG_API ZgResult zgCommandListProfileEnd(
	ZgCommandList* commandList,
	ZgProfiler* profiler,
	uint64_t measurementId);

#ifdef __cplusplus
namespace zg {

class CommandList final {
public:
	ZgCommandList* commandList = nullptr;

	CommandList() noexcept = default;
	CommandList(const CommandList&) = delete;
	CommandList& operator= (const CommandList&) = delete;
	CommandList(CommandList&& other) noexcept { this->swap(other); }
	CommandList& operator= (CommandList&& other) noexcept { this->swap(other); return *this; }
	~CommandList() noexcept { this->release(); }

	bool valid() const noexcept { return commandList != nullptr; }

	void swap(CommandList& other) noexcept
	{
		std::swap(this->commandList, other.commandList);
	}

	void release() noexcept
	{
		// TODO: Currently there is no destruction of command lists as they are owned by the
		//       CommandQueue.
		this->commandList = nullptr;
	}

	Result memcpyBufferToBuffer(
		Buffer& dstBuffer,
		uint64_t dstBufferOffsetBytes,
		Buffer& srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept
	{
		return (Result)zgCommandListMemcpyBufferToBuffer(
			this->commandList,
			dstBuffer.buffer,
			dstBufferOffsetBytes,
			srcBuffer.buffer,
			srcBufferOffsetBytes,
			numBytes);
	}

	Result memcpyToTexture(
		Texture2D& dstTexture,
		uint32_t dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		Buffer& tempUploadBuffer) noexcept
	{
		return (Result)zgCommandListMemcpyToTexture(
			this->commandList,
			dstTexture.texture,
			dstTextureMipLevel,
			&srcImageCpu,
			tempUploadBuffer.buffer);
	}

	Result enableQueueTransition(Buffer& buffer) noexcept
	{
		return (Result)zgCommandListEnableQueueTransitionBuffer(this->commandList, buffer.buffer);
	}

	Result enableQueueTransition(Texture2D& texture) noexcept
	{
		return (Result)zgCommandListEnableQueueTransitionTexture(this->commandList, texture.texture);
	}

	Result setPushConstant(
		uint32_t shaderRegister, const void* data, uint32_t dataSizeInBytes) noexcept
	{
		return (Result)zgCommandListSetPushConstant(
			this->commandList, shaderRegister, data, dataSizeInBytes);
	}

	Result setPipelineBindings(const PipelineBindings& bindings) noexcept
	{
		return (Result)zgCommandListSetPipelineBindings(this->commandList, &bindings.bindings);
	}

	Result setPipeline(PipelineCompute& pipeline) noexcept
	{
		return (Result)zgCommandListSetPipelineCompute(this->commandList, pipeline.pipeline);
	}

	Result unorderedBarrier(Buffer& buffer) noexcept
	{
		return (Result)zgCommandListUnorderedBarrierBuffer(this->commandList, buffer.buffer);
	}

	Result unorderedBarrier(Texture2D& texture) noexcept
	{
		return (Result)zgCommandListUnorderedBarrierTexture(this->commandList, texture.texture);
	}

	Result unorderedBarrier() noexcept
	{
		return (Result)zgCommandListUnorderedBarrierAll(this->commandList);
	}

	Result dispatchCompute(
		uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) noexcept
	{
		return (Result)zgCommandListDispatchCompute(
			this->commandList, groupCountX, groupCountY, groupCountZ);
	}

	Result setPipeline(PipelineRender& pipeline) noexcept
	{
		return (Result)zgCommandListSetPipelineRender(this->commandList, pipeline.pipeline);
	}

	Result setFramebuffer(
		Framebuffer& framebuffer,
		const ZgFramebufferRect* optionalViewport = nullptr,
		const ZgFramebufferRect* optionalScissor = nullptr) noexcept
	{
		return (Result)zgCommandListSetFramebuffer(
			this->commandList, framebuffer.framebuffer, optionalViewport, optionalScissor);
	}

	Result setFramebufferViewport(
		const ZgFramebufferRect& viewport) noexcept
	{
		return (Result)zgCommandListSetFramebufferViewport(this->commandList, &viewport);
	}

	Result setFramebufferScissor(
		const ZgFramebufferRect& scissor) noexcept
	{
		return (Result)zgCommandListSetFramebufferScissor(this->commandList, &scissor);
	}

	Result clearFramebufferOptimal() noexcept
	{
		return (Result)zgCommandListClearFramebufferOptimal(this->commandList);
	}

	Result clearRenderTargets(float red, float green, float blue, float alpha) noexcept
	{
		return (Result)zgCommandListClearRenderTargets(this->commandList, red, green, blue, alpha);
	}

	Result clearDepthBuffer(float depth) noexcept
	{
		return (Result)zgCommandListClearDepthBuffer(this->commandList, depth);
	}

	Result setIndexBuffer(Buffer& indexBuffer, ZgIndexBufferType type) noexcept
	{
		return (Result)zgCommandListSetIndexBuffer(this->commandList, indexBuffer.buffer, type);
	}

	Result setVertexBuffer(uint32_t vertexBufferSlot, Buffer& vertexBuffer) noexcept
	{
		return (Result)zgCommandListSetVertexBuffer(
			this->commandList, vertexBufferSlot, vertexBuffer.buffer);
	}

	Result drawTriangles(uint32_t startVertexIndex, uint32_t numVertices) noexcept
	{
		return (Result)zgCommandListDrawTriangles(this->commandList, startVertexIndex, numVertices);
	}

	Result drawTrianglesIndexed(uint32_t startIndex, uint32_t numTriangles) noexcept
	{
		return (Result)zgCommandListDrawTrianglesIndexed(
			this->commandList, startIndex, numTriangles);
	}

	Result profileBegin(Profiler& profiler, uint64_t& measurementIdOut) noexcept
	{
		return (Result)zgCommandListProfileBegin(this->commandList, profiler.profiler, &measurementIdOut);
	}

	Result profileEnd(Profiler& profiler, uint64_t measurementId) noexcept
	{
		return (Result)zgCommandListProfileEnd(this->commandList, profiler.profiler, measurementId);
	}
};

} // namespace zg
#endif

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

#ifdef __cplusplus
namespace zg {

class CommandQueue final {
public:
	ZgCommandQueue* commandQueue = nullptr;

	CommandQueue() noexcept = default;
	CommandQueue(const CommandQueue&) = delete;
	CommandQueue& operator= (const CommandQueue&) = delete;
	CommandQueue(CommandQueue&& other) noexcept { this->swap(other); }
	CommandQueue& operator= (CommandQueue&& other) noexcept { this->swap(other); return *this; }
	~CommandQueue() noexcept { this->release(); }

	static Result getPresentQueue(CommandQueue& presentQueueOut) noexcept
	{
		if (presentQueueOut.commandQueue != nullptr) return Result::INVALID_ARGUMENT;
		return (Result)zgCommandQueueGetPresentQueue(&presentQueueOut.commandQueue);
	}

	static Result getCopyQueue(CommandQueue& copyQueueOut) noexcept
	{
		if (copyQueueOut.commandQueue != nullptr) return Result::INVALID_ARGUMENT;
		return (Result)zgCommandQueueGetCopyQueue(&copyQueueOut.commandQueue);
	}

	bool valid() const noexcept { return commandQueue != nullptr; }

	void swap(CommandQueue& other) noexcept
	{
		std::swap(this->commandQueue, other.commandQueue);
	}

	// TODO: No-op because there currently is no releasing of Command Queues...
	void release() noexcept
	{
		// TODO: Currently there is no destruction of command queues as there is only one
		this->commandQueue = nullptr;
	}

	Result signalOnGpu(Fence& fenceToSignal) noexcept
	{
		return (Result)zgCommandQueueSignalOnGpu(this->commandQueue, fenceToSignal.fence);
	}

	Result waitOnGpu(const Fence& fence) noexcept
	{
		return (Result)zgCommandQueueWaitOnGpu(this->commandQueue, fence.fence);
	}

	Result flush() noexcept
	{
		return (Result)zgCommandQueueFlush(this->commandQueue);
	}

	Result beginCommandListRecording(CommandList& commandListOut) noexcept
	{
		if (commandListOut.commandList != nullptr) return Result::INVALID_ARGUMENT;
		return (Result)zgCommandQueueBeginCommandListRecording(
			this->commandQueue, &commandListOut.commandList);
	}

	Result executeCommandList(CommandList& commandList) noexcept
	{
		ZgResult res = zgCommandQueueExecuteCommandList(this->commandQueue, commandList.commandList);
		commandList.commandList = nullptr;
		return (Result)res;
	}
};

} // namespace zg
#endif

// Logging interface
// ------------------------------------------------------------------------------------------------

ZG_ENUM(ZgLogLevel) {
	ZG_LOG_LEVEL_NOISE = 0,
	ZG_LOG_LEVEL_INFO,
	ZG_LOG_LEVEL_WARNING,
	ZG_LOG_LEVEL_ERROR
};

// Logger used for logging inside ZeroG.
//
// The logger must be thread-safe. I.e. it must be okay to call it simulatenously from multiple
// threads.
//
// If no custom logger is wanted leave all fields zero in this struct. Normal printf() will then be
// used for logging instead.
ZG_STRUCT(ZgLogger) {

	// Function pointer to user-specified log function.
	void(*log)(void* userPtr, const char* file, int line, ZgLogLevel level, const char* message);

	// User specified pointer that is provied to each log() call.
	void* userPtr;
};

// Memory allocator interface
// ------------------------------------------------------------------------------------------------

// Allocator interface for CPU allocations inside ZeroG.
//
// A few restrictions is placed on custom allocators:
// * They must be thread-safe. I.e. it must be okay to call it simulatenously from multiple threads.
// * All allocations must be at least 32-byte aligned.
//
// If no custom allocator is required, just leave all fields zero in this struct.
ZG_STRUCT(ZgAllocator) {

	// Function pointer to allocate function. The allocation created must be 32-byte aligned. name,
	// file and line is (statically allocated) debug information related to the allocation.
	void* (*allocate)(void* userPtr, uint32_t size, const char* name, const char* file, uint32_t line);

	// Function pointer to deallocate function.
	void (*deallocate)(void* userPtr, void* allocation);

	// User specified pointer that is provided to each allocate/free call.
	void* userPtr;
};

// Context
// ------------------------------------------------------------------------------------------------

ZG_STRUCT(ZgContextInitSettingsD3D12) {
	
	// [Optional] Used to enable D3D12 validation.
	ZgBool debugMode;

	// [Optional]
	ZgBool useSoftwareRenderer;
};

ZG_STRUCT(ZgContextInitSettingsVulkan) {

	// [Optional] Used to enable Vulkan debug layers
	ZgBool debugMode;
};

// The settings used to create a context and initialize ZeroG
ZG_STRUCT(ZgContextInitSettings) {

	// [Mandatory] The wanted ZeroG backend
	ZgBackendType backend;

	// [Mandatory] The dimensions (in pixels) of the window being rendered to
	uint32_t width;
	uint32_t height;

	// [Optional] Whether VSync should be enabled or not
	ZgBool vsync;

	// [Optional] The logger used for logging
	ZgLogger logger;

	// [Optional] The allocator used to allocate CPU memory
	ZgAllocator allocator;

	// [Mandatory] Platform specific native handle.
	//
	// On Windows, this is a HWND, i.e. native window handle.
	void* nativeHandle;

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
	uint32_t width,
	uint32_t height);

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
	uint64_t* measurementIdOut);

// Only specify profiler if you specified one to the begin frame call. Otherwise send in nullptr
// and 0.
ZG_API ZgResult zgContextSwapchainFinishFrame(
	ZgProfiler* profiler,
	uint64_t measurementId);

// A struct containing general statistics
ZG_STRUCT(ZgStats) {

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

// Gets the current statistics of the ZeroG backend. Normally called once (or maybe up to a couple
// of times) per frame.
ZG_API ZgResult zgContextGetStats(ZgStats* statsOut);

#ifdef __cplusplus
namespace zg {

class Context final {
public:

	Context() noexcept = default;
	Context(const Context&) = delete;
	Context& operator= (const Context&) = delete;
	Context(Context&& other) noexcept { this->swap(other); }
	Context& operator= (Context&& other) noexcept { this->swap(other); return *this; }
	~Context() noexcept { this->deinit(); }

	Result init(const ZgContextInitSettings& settings) noexcept
	{
		this->deinit();
		ZgResult res = zgContextInit(&settings);
		mInitialized = res == ZG_SUCCESS;
		return (Result)res;
	}

	void deinit() noexcept
	{
		if (mInitialized) zgContextDeinit();
		mInitialized = false;
	}
	void swap(Context& other) noexcept
	{
		std::swap(mInitialized, other.mInitialized);
	}

	static uint32_t compiledApiVersion() noexcept { return ZG_COMPILED_API_VERSION; }

	static uint32_t linkedApiVersion() noexcept
	{
		return zgApiLinkedVersion();
	}

	static bool alreadyInitialized() noexcept
	{
		return zgContextAlreadyInitialized();
	}

	Result swapchainResize(uint32_t width, uint32_t height) noexcept
	{
		return (Result)zgContextSwapchainResize(width, height);
	}

	Result swapchainSetVsync(bool vsync) noexcept
	{
		return (Result)zgContextSwapchainSetVsync(vsync ? ZG_TRUE : ZG_FALSE);
	}

	Result swapchainBeginFrame(Framebuffer& framebufferOut) noexcept
	{
		if (framebufferOut.valid()) return Result::INVALID_ARGUMENT;
		Result res = (Result)zgContextSwapchainBeginFrame(&framebufferOut.framebuffer, nullptr, nullptr);
		if (!isSuccess(res)) return res;
		return (Result)zgFramebufferGetResolution(
			framebufferOut.framebuffer, &framebufferOut.width, &framebufferOut.height);
	}

	Result swapchainBeginFrame(
		Framebuffer& framebufferOut, Profiler& profiler, uint64_t& measurementIdOut) noexcept
	{
		if (framebufferOut.valid()) return Result::INVALID_ARGUMENT;
		Result res = (Result)zgContextSwapchainBeginFrame(
			&framebufferOut.framebuffer, profiler.profiler, &measurementIdOut);
		if (!isSuccess(res)) return res;
		return (Result)zgFramebufferGetResolution(
			framebufferOut.framebuffer, &framebufferOut.width, &framebufferOut.height);
	}

	Result swapchainFinishFrame() noexcept
	{
		return (Result)zgContextSwapchainFinishFrame(nullptr, 0);
	}

	Result swapchainFinishFrame(Profiler& profiler, uint64_t measurementId) noexcept
	{
		return (Result)zgContextSwapchainFinishFrame(profiler.profiler, measurementId);
	}

	Result getStats(ZgStats& statsOut) noexcept
	{
		return (Result)zgContextGetStats(&statsOut);
	}

private:

	bool mInitialized = false;
};

} // namespace zg
#endif

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
// HLSL the "float4x4" primitive must be marked "row_major", otherwise the matrix will get
// transposed during the transfer and you will not get the results you expect.
//
// The createViewMatrix() function creates a view matrix similar to the one typically used in OpenGL.
// In other words, right-handed coordinate system with x to the right, y up and z towards the camera
// (negative z into the scene). This is the kind of view matrix that is expected for all the
// projection matrices here.
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
// If unsure I would recommend starting out with the basic createPerspectiveProjection() and then
// switching to createPerspectiveProjectionReverseInfinite() when feeling more confident.

#ifdef __cplusplus

// TODO: Remove
#include <string.h>
#include <cmath>

namespace zg {

inline void createViewMatrix(
	float rowMajorMatrixOut[16],
	const float origin[3],
	const float dir[3],
	const float up[3]) noexcept
{
	auto dot = [](const float lhs[3], const float rhs[3]) -> float {
		return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
	};

	auto normalize = [&](float v[3]) {
		float length = std::sqrt(dot(v, v));
		v[0] /= length;
		v[1] /= length;
		v[2] /= length;
	};

	auto cross = [](float out[3], const float lhs[3], const float rhs[3]) {
		out[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
		out[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
		out[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
	};

	// Z-Axis, away from screen
	float zAxis[3];
	memcpy(zAxis, dir, sizeof(float) * 3);
	normalize(zAxis);
	zAxis[0] = -zAxis[0];
	zAxis[1] = -zAxis[1];
	zAxis[2] = -zAxis[2];

	// X-Axis, to the right
	float xAxis[3];
	cross(xAxis, up, zAxis);
	normalize(xAxis);

	// Y-Axis, up
	float yAxis[3];
	cross(yAxis, zAxis, xAxis);

	float matrix[16] = {
		xAxis[0], xAxis[1], xAxis[2], -dot(xAxis, origin),
		yAxis[0], yAxis[1], yAxis[2], -dot(yAxis, origin),
		zAxis[0], zAxis[1], zAxis[2], -dot(zAxis, origin),
		0.0f,     0.0f,     0.0f,     1.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createPerspectiveProjection(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float nearPlane,
	float farPlane) noexcept
{
	assert(0.0f < vertFovDegs);
	assert(vertFovDegs < 180.0f);
	assert(0.0f < aspect);
	assert(0.0f < nearPlane);
	assert(nearPlane < farPlane);

	// From: https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh
	// xScale     0          0              0
	// 0        yScale       0              0
	// 0        0        zf/(zn-zf)        -1
	// 0        0        zn*zf/(zn-zf)      0
	// where:
	// yScale = cot(fovY/2)
	// xScale = yScale / aspect ratio
	//
	// Note that D3D uses column major matrices, we use row-major, so above is transposed.

	constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
	const float vertFovRads = vertFovDegs * DEG_TO_RAD;
	const float yScale = 1.0f / std::tan(vertFovRads * 0.5f);
	const float xScale = yScale / aspect;
	float matrix[16] = {
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, farPlane / (nearPlane - farPlane), nearPlane* farPlane / (nearPlane - farPlane),
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createPerspectiveProjectionInfinite(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float nearPlane) noexcept
{
	assert(0.0f < vertFovDegs);
	assert(vertFovDegs < 180.0f);
	assert(0.0f < aspect);
	assert(0.0f < nearPlane);

	// Same as createPerspectiveProjection(), but let far approach infinity

	constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
	const float vertFovRads = vertFovDegs * DEG_TO_RAD;
	const float yScale = 1.0f / std::tan(vertFovRads * 0.5f);
	const float xScale = yScale / aspect;
	float matrix[16] = {
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f,-nearPlane,
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createPerspectiveProjectionReverse(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float nearPlane,
	float farPlane) noexcept
{
	assert(0.0f < vertFovDegs);
	assert(vertFovDegs < 180.0f);
	assert(0.0f < aspect);
	assert(0.0f < nearPlane);
	assert(nearPlane < farPlane);

	// http://dev.theomader.com/depth-precision/
	// "This can be achieved by multiplying the projection matrix with a simple ‘z reversal’ matrix"
	// 1, 0, 0, 0
	// 0, 1, 0, 0
	// 0, 0, -1, 1
	// 0, 0, 0, 1

	constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
	const float vertFovRads = vertFovDegs * DEG_TO_RAD;
	const float yScale = 1.0f / std::tan(vertFovRads * 0.5f);
	const float xScale = yScale / aspect;
	float matrix[16] = {
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, -(farPlane / (nearPlane - farPlane)) - 1.0f, -(nearPlane * farPlane / (nearPlane - farPlane)),
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createPerspectiveProjectionReverseInfinite(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float nearPlane) noexcept
{
	assert(0.0f < vertFovDegs);
	assert(vertFovDegs < 180.0f);
	assert(0.0f < aspect);
	assert(0.0f < nearPlane);

	// http://dev.theomader.com/depth-precision/
	// "This can be achieved by multiplying the projection matrix with a simple ‘z reversal’ matrix"
	// 1, 0, 0, 0
	// 0, 1, 0, 0
	// 0, 0, -1, 1
	// 0, 0, 0, 1

	constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
	const float vertFovRads = vertFovDegs * DEG_TO_RAD;
	const float yScale = 1.0f / std::tan(vertFovRads * 0.5f);
	const float xScale = yScale / aspect;
	float matrix[16] = {
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, nearPlane,
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createOrthographicProjection(
	float rowMajorMatrixOut[16],
	float width,
	float height,
	float nearPlane,
	float farPlane) noexcept
{
	assert(0.0f < width);
	assert(0.0f < height);
	assert(0.0f < nearPlane);
	assert(nearPlane < farPlane);

	// https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthorh
	// 2/w  0    0           0
	// 0    2/h  0           0
	// 0    0    1/(zn-zf)   0
	// 0    0    zn/(zn-zf)  1
	//
	// Note that D3D uses column major matrices, we use row-major, so above is transposed.

	float matrix[16] = {
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f / (nearPlane - farPlane), nearPlane / (nearPlane - farPlane),
		0.0f, 0.0f, 0.0f, 1.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createOrthographicProjectionReverse(
	float rowMajorMatrixOut[16],
	float width,
	float height,
	float nearPlane,
	float farPlane) noexcept
{
	assert(0.0f < width);
	assert(0.0f < height);
	assert(0.0f < nearPlane);
	assert(nearPlane < farPlane);

	// http://dev.theomader.com/depth-precision/
	// "This can be achieved by multiplying the projection matrix with a simple ‘z reversal’ matrix"
	// 1, 0, 0, 0
	// 0, 1, 0, 0
	// 0, 0, -1, 1
	// 0, 0, 0, 1

	float matrix[16] = {
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f / (nearPlane - farPlane), 1.0f - (nearPlane / (nearPlane - farPlane)),
		0.0f, 0.0f, 0.0f, 1.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

} // namespace zg
#endif

#endif
