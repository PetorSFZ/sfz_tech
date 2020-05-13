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

#include <cassert>
#include <cmath>
#include <cstring>
#include <utility>

#include "ZeroG.h"

namespace zg {

// Results
// ------------------------------------------------------------------------------------------------

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

constexpr bool isSuccess(Result code) noexcept { return code == Result::SUCCESS; }
constexpr bool isWarning(Result code) noexcept { return ZgResult(code) > 0; }
constexpr bool isError(Result code) noexcept { return ZgResult(code) < 0; }

// Buffer
// ------------------------------------------------------------------------------------------------

class Buffer final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgBuffer* buffer = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Buffer() noexcept = default;
	Buffer(const Buffer&) = delete;
	Buffer& operator= (const Buffer&) = delete;
	Buffer(Buffer&& o) noexcept { this->swap(o); }
	Buffer& operator= (Buffer&& o) noexcept { this->swap(o); return *this; }
	~Buffer() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return buffer != nullptr; }

	void swap(Buffer& other) noexcept
	{
		std::swap(this->buffer, other.buffer);
	}

	// See zgBufferRelease()
	void release() noexcept
	{
		if (this->buffer != nullptr) {
			zgBufferRelease(this->buffer);
		}
		this->buffer = nullptr;
	}

	// Buffer methods
	// --------------------------------------------------------------------------------------------

	// See zgBufferMemcpyTo()
	Result memcpyTo(uint64_t bufferOffsetBytes, const void* srcMemory, uint64_t numBytes)
	{
		return (Result)zgBufferMemcpyTo(this->buffer, bufferOffsetBytes, srcMemory, numBytes);
	}

	// See zgBufferMemcpyFrom()
	Result memcpyFrom(void* dstMemory, uint64_t srcBufferOffsetBytes, uint64_t numBytes)
	{
		return (Result)zgBufferMemcpyFrom(dstMemory, this->buffer, srcBufferOffsetBytes, numBytes);
	}

	// See zgBufferSetDebugName()
	Result setDebugName(const char* name) noexcept
	{
		return (Result)zgBufferSetDebugName(this->buffer, name);
	}
};


// Texture2D
// ------------------------------------------------------------------------------------------------

class Texture2D final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgTexture2D* texture = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Texture2D() noexcept = default;
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator= (const Texture2D&) = delete;
	Texture2D(Texture2D&& o) noexcept { this->swap(o); }
	Texture2D& operator= (Texture2D&& o) noexcept { this->swap(o); return *this; }
	~Texture2D() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return this->texture != nullptr; }

	void swap(Texture2D& other) noexcept
	{
		std::swap(this->texture, other.texture);
		std::swap(this->width, other.width);
		std::swap(this->height, other.height);
	}

	// See zgTexture2DRelease()
	void release() noexcept
	{
		if (this->texture != nullptr) zgTexture2DRelease(this->texture);
		this->texture = nullptr;
		this->width = 0;
		this->height = 0;
	}

	// Methods
	// --------------------------------------------------------------------------------------------

	// See zgTexture2DGetAllocationInfo
	static Result getAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept
	{
		return (Result)zgTexture2DGetAllocationInfo(&allocationInfoOut, &createInfo);
	}

	// See zgTexture2DSetDebugName()
	Result setDebugName(const char* name) noexcept
	{
		return (Result)zgTexture2DSetDebugName(this->texture, name);
	}
};


// MemoryHeap
// ------------------------------------------------------------------------------------------------

class MemoryHeap final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgMemoryHeap* memoryHeap = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	MemoryHeap() noexcept = default;
	MemoryHeap(const MemoryHeap&) = delete;
	MemoryHeap& operator= (const MemoryHeap&) = delete;
	MemoryHeap(MemoryHeap&& o) noexcept { this->swap(o); }
	MemoryHeap& operator= (MemoryHeap&& o) noexcept { this->swap(o); return *this; }
	~MemoryHeap() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return memoryHeap != nullptr; }

	// See zgMemoryHeapCreate()
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

	// See zgMemoryHeapRelease()
	void release() noexcept
	{
		if (this->memoryHeap != nullptr) zgMemoryHeapRelease(this->memoryHeap);
		this->memoryHeap = nullptr;
	}

	// MemoryHeap methods
	// --------------------------------------------------------------------------------------------

	// See zgMemoryHeapBufferCreate()
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

	// See zgMemoryHeapTexture2DCreate()
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


// PipelineBindings
// ------------------------------------------------------------------------------------------------

class PipelineBindings final {
public:

	// Members
	// --------------------------------------------------------------------------------------------

	ZgPipelineBindings bindings = {};

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	PipelineBindings() noexcept = default;
	PipelineBindings(const PipelineBindings&) noexcept = default;
	PipelineBindings& operator= (const PipelineBindings&) noexcept = default;
	~PipelineBindings() noexcept = default;

	// Methods
	// --------------------------------------------------------------------------------------------

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


// PipelineCompute
// ------------------------------------------------------------------------------------------------

class PipelineCompute final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgPipelineCompute* pipeline = nullptr;
	ZgPipelineBindingsSignature bindingsSignature = {};
	ZgPipelineComputeSignature computeSignature = {};

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	PipelineCompute() noexcept = default;
	PipelineCompute(const PipelineCompute&) = delete;
	PipelineCompute& operator= (const PipelineCompute&) = delete;
	PipelineCompute(PipelineCompute&& o) noexcept { this->swap(o); }
	PipelineCompute& operator= (PipelineCompute&& o) noexcept { this->swap(o); return *this; }
	~PipelineCompute() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	// Checks if this pipeline is valid
	bool valid() const noexcept { return this->pipeline != nullptr; }

	// See zgPipelineComputeCreateInfoFileHLSL()
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

	// See zgPipelineComputeRelease()
	void release() noexcept
	{
		if (this->pipeline != nullptr) zgPipelineComputeRelease(this->pipeline);
		this->pipeline = nullptr;
		this->bindingsSignature = {};
		this->computeSignature = {};
	}
};


// PipelineComputeBuilder
// ------------------------------------------------------------------------------------------------

class PipelineComputeBuilder final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgPipelineComputeCreateInfo createInfo = {};
	const char* computeShaderPath = nullptr;
	const char* computeShaderSrc = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	PipelineComputeBuilder() noexcept = default;
	PipelineComputeBuilder(const PipelineComputeBuilder&) noexcept = default;
	PipelineComputeBuilder& operator= (const PipelineComputeBuilder&) noexcept = default;
	~PipelineComputeBuilder() noexcept = default;

	// Methods
	// --------------------------------------------------------------------------------------------

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

// PipelineRender
// ------------------------------------------------------------------------------------------------

class PipelineRender final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgPipelineRender* pipeline = nullptr;
	ZgPipelineBindingsSignature bindingsSignature = {};
	ZgPipelineRenderSignature renderSignature = {};

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	PipelineRender() noexcept = default;
	PipelineRender(const PipelineRender&) = delete;
	PipelineRender& operator= (const PipelineRender&) = delete;
	PipelineRender(PipelineRender&& o) noexcept { this->swap(o); }
	PipelineRender& operator= (PipelineRender&& o) noexcept { this->swap(o); return *this; }
	~PipelineRender() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	// Checks if this pipeline is valid
	bool valid() const noexcept { return this->pipeline != nullptr; }

	// See zgPipelineRenderCreateFromFileSPIRV()
	Result createFromFileSPIRV(
		const ZgPipelineRenderCreateInfo& createInfo) noexcept
	{
		this->release();
		return (Result)zgPipelineRenderCreateFromFileSPIRV(
			&this->pipeline, &this->bindingsSignature, &this->renderSignature, &createInfo);
	}

	// See ZgPipelineRenderCreateInfoFileHLSL()
	Result createFromFileHLSL(
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
	{
		this->release();
		return (Result)zgPipelineRenderCreateFromFileHLSL(
			&this->pipeline, &this->bindingsSignature, &this->renderSignature, &createInfo, &compileSettings);
	}

	// See ZgPipelineRenderCreateInfoSourceHLSL()
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

	// See zgPipelineRenderRelease()
	void release() noexcept
	{
		if (this->pipeline != nullptr) zgPipelineRenderRelease(this->pipeline);
		this->pipeline = nullptr;
		this->bindingsSignature = {};
		this->renderSignature = {};
	}
};

// PipelineRenderBuilder
// ------------------------------------------------------------------------------------------------

class PipelineRenderBuilder final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgPipelineRenderCreateInfo createInfo = {};
	const char* vertexShaderPath = nullptr;
	const char* pixelShaderPath = nullptr;
	const char* vertexShaderSrc = nullptr;
	const char* pixelShaderSrc = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	PipelineRenderBuilder() noexcept = default;
	PipelineRenderBuilder(const PipelineRenderBuilder&) noexcept = default;
	PipelineRenderBuilder& operator= (const PipelineRenderBuilder&) noexcept = default;
	~PipelineRenderBuilder() noexcept = default;

	// Methods
	// --------------------------------------------------------------------------------------------

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


// Framebuffer
// ------------------------------------------------------------------------------------------------

class Framebuffer final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgFramebuffer* framebuffer = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Framebuffer() noexcept = default;
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator= (const Framebuffer&) = delete;
	Framebuffer(Framebuffer&& other) noexcept { this->swap(other); }
	Framebuffer& operator= (Framebuffer&& other) noexcept { this->swap(other); return *this; }
	~Framebuffer() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return this->framebuffer != nullptr; }

	// See zgFramebufferCreate()
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

	// See zgFramebufferRelease()
	void release() noexcept
	{
		if (this->framebuffer != nullptr) zgFramebufferRelease(this->framebuffer);
		this->framebuffer = nullptr;
		this->width = 0;
		this->height = 0;
	}
};


// FramebufferBuilder
// ------------------------------------------------------------------------------------------------

class FramebufferBuilder final {
public:

	// Members
	// --------------------------------------------------------------------------------------------

	ZgFramebufferCreateInfo createInfo = {};

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	FramebufferBuilder() noexcept = default;
	FramebufferBuilder(const FramebufferBuilder&) noexcept = default;
	FramebufferBuilder& operator= (const FramebufferBuilder&) noexcept = default;
	~FramebufferBuilder() noexcept = default;

	// Methods
	// --------------------------------------------------------------------------------------------

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





// Fence
// ------------------------------------------------------------------------------------------------

class Fence final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgFence* fence = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Fence() noexcept = default;
	Fence(const Fence&) = delete;
	Fence& operator= (const Fence&) = delete;
	Fence(Fence&& other) noexcept { this->swap(other); }
	Fence& operator= (Fence&& other) noexcept { this->swap(other); return *this; }
	~Fence() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return this->fence != nullptr; }

	// See zgFenceCreate()
	Result create() noexcept
	{
		this->release();
		return (Result)zgFenceCreate(&this->fence);
	}

	void swap(Fence& other) noexcept
	{
		std::swap(this->fence, other.fence);
	}

	// See zgFenceRelease()
	void release() noexcept
	{
		if (this->fence != nullptr) zgFenceRelease(this->fence);
		this->fence = nullptr;
	}

	// Fence methods
	// --------------------------------------------------------------------------------------------

	// See zgFenceReset()
	Result reset() noexcept
	{
		return (Result)zgFenceReset(this->fence);
	}

	// See zgFenceCheckIfSignaled()
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

	// See zgFenceWaitOnCpuBlocking()
	Result waitOnCpuBlocking() const noexcept
	{
		return (Result)zgFenceWaitOnCpuBlocking(this->fence);
	}
};


// Profiler
// ------------------------------------------------------------------------------------------------

class Profiler final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgProfiler* profiler = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Profiler() noexcept = default;
	Profiler(const Profiler&) = delete;
	Profiler& operator= (const Profiler&) = delete;
	Profiler(Profiler&& other) noexcept { this->swap(other); }
	Profiler& operator= (Profiler&& other) noexcept { this->swap(other); return *this; }
	~Profiler() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return this->profiler != nullptr; }

	// See zgProfilerCreate()
	Result create(const ZgProfilerCreateInfo& createInfo) noexcept
	{
		this->release();
		return (Result)zgProfilerCreate(&this->profiler, &createInfo);
	}

	void swap(Profiler& other) noexcept
	{
		std::swap(this->profiler, other.profiler);
	}

	// See zgProfilerRelease()
	void release() noexcept
	{
		if (this->profiler != nullptr) zgProfilerRelease(this->profiler);
		this->profiler = nullptr;
	}

	// Profiler methods
	// --------------------------------------------------------------------------------------------

	// See zgProfilerGetMeasurement()
	Result getMeasurement(
		uint64_t measurementId,
		float& measurementMsOut) noexcept
	{
		return (Result)zgProfilerGetMeasurement(this->profiler, measurementId, &measurementMsOut);
	}
};


// CommandList
// ------------------------------------------------------------------------------------------------

class CommandList final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgCommandList* commandList = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	CommandList() noexcept = default;
	CommandList(const CommandList&) = delete;
	CommandList& operator= (const CommandList&) = delete;
	CommandList(CommandList&& other) noexcept { this->swap(other); }
	CommandList& operator= (CommandList&& other) noexcept { this->swap(other); return *this; }
	~CommandList() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

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

	// CommandList methods
	// --------------------------------------------------------------------------------------------

	// See zgCommandListMemcpyBufferToBuffer()
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

	// See zgCommandListMemcpyToTexture()
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

	// See zgCommandListEnableQueueTransitionBuffer()
	Result enableQueueTransition(Buffer& buffer) noexcept
	{
		return (Result)zgCommandListEnableQueueTransitionBuffer(this->commandList, buffer.buffer);
	}

	// See zgCommandListEnableQueueTransitionTexture()
	Result enableQueueTransition(Texture2D& texture) noexcept
	{
		return (Result)zgCommandListEnableQueueTransitionTexture(this->commandList, texture.texture);
	}

	// See zgCommandListSetPushConstant()
	Result setPushConstant(
		uint32_t shaderRegister, const void* data, uint32_t dataSizeInBytes) noexcept
	{
		return (Result)zgCommandListSetPushConstant(
			this->commandList, shaderRegister, data, dataSizeInBytes);
	}

	// See zgCommandListSetPipelineBindings()
	Result setPipelineBindings(const PipelineBindings& bindings) noexcept
	{
		return (Result)zgCommandListSetPipelineBindings(this->commandList, &bindings.bindings);
	}

	// See zgCommandListSetPipelineCompute()
	Result setPipeline(PipelineCompute& pipeline) noexcept
	{
		return (Result)zgCommandListSetPipelineCompute(this->commandList, pipeline.pipeline);
	}

	// See zgCommandListUnorderedBarrierBuffer()
	Result unorderedBarrier(Buffer& buffer) noexcept
	{
		return (Result)zgCommandListUnorderedBarrierBuffer(this->commandList, buffer.buffer);
	}

	// See zgCommandListUnorderedBarrierTexture()
	Result unorderedBarrier(Texture2D& texture) noexcept
	{
		return (Result)zgCommandListUnorderedBarrierTexture(this->commandList, texture.texture);
	}

	// See zgCommandListUnorderedBarrierAll()
	Result unorderedBarrier() noexcept
	{
		return (Result)zgCommandListUnorderedBarrierAll(this->commandList);
	}

	// See zgCommandListDispatchCompute()
	Result dispatchCompute(
		uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) noexcept
	{
		return (Result)zgCommandListDispatchCompute(
			this->commandList, groupCountX, groupCountY, groupCountZ);
	}

	// See zgCommandListSetPipelineRender()
	Result setPipeline(PipelineRender& pipeline) noexcept
	{
		return (Result)zgCommandListSetPipelineRender(this->commandList, pipeline.pipeline);
	}

	// See zgCommandListSetFramebuffer()
	Result setFramebuffer(
		Framebuffer& framebuffer,
		const ZgFramebufferRect* optionalViewport = nullptr,
		const ZgFramebufferRect* optionalScissor = nullptr) noexcept
	{
		return (Result)zgCommandListSetFramebuffer(
			this->commandList, framebuffer.framebuffer, optionalViewport, optionalScissor);
	}

	// See zgCommandListSetFramebufferViewport()
	Result setFramebufferViewport(
		const ZgFramebufferRect& viewport) noexcept
	{
		return (Result)zgCommandListSetFramebufferViewport(this->commandList, &viewport);
	}

	// See zgCommandListSetFramebufferScissor()
	Result setFramebufferScissor(
		const ZgFramebufferRect& scissor) noexcept
	{
		return (Result)zgCommandListSetFramebufferScissor(this->commandList, &scissor);
	}

	// See zgCommandListClearFramebufferOptimal()
	Result clearFramebufferOptimal() noexcept
	{
		return (Result)zgCommandListClearFramebufferOptimal(this->commandList);
	}

	// See zgCommandListClearRenderTargets()
	Result clearRenderTargets(float red, float green, float blue, float alpha) noexcept
	{
		return (Result)zgCommandListClearRenderTargets(this->commandList, red, green, blue, alpha);
	}

	// See zgCommandListClearDepthBuffer()
	Result clearDepthBuffer(float depth) noexcept
	{
		return (Result)zgCommandListClearDepthBuffer(this->commandList, depth);
	}

	// See zgCommandListSetIndexBuffer()
	Result setIndexBuffer(Buffer& indexBuffer, ZgIndexBufferType type) noexcept
	{
		return (Result)zgCommandListSetIndexBuffer(this->commandList, indexBuffer.buffer, type);
	}

	// See zgCommandListSetVertexBuffer()
	Result setVertexBuffer(uint32_t vertexBufferSlot, Buffer& vertexBuffer) noexcept
	{
		return (Result)zgCommandListSetVertexBuffer(
			this->commandList, vertexBufferSlot, vertexBuffer.buffer);
	}

	// See zgCommandListDrawTriangles()
	Result drawTriangles(uint32_t startVertexIndex, uint32_t numVertices) noexcept
	{
		return (Result)zgCommandListDrawTriangles(this->commandList, startVertexIndex, numVertices);
	}

	// See zgCommandListDrawTrianglesIndexed()
	Result drawTrianglesIndexed(uint32_t startIndex, uint32_t numTriangles) noexcept
	{
		return (Result)zgCommandListDrawTrianglesIndexed(
			this->commandList, startIndex, numTriangles);
	}

	// See zgCommandListProfileBegin()
	Result profileBegin(Profiler& profiler, uint64_t& measurementIdOut) noexcept
	{
		return (Result)zgCommandListProfileBegin(this->commandList, profiler.profiler, &measurementIdOut);
	}

	// See zgCommandListProfileEnd()
	Result profileEnd(Profiler& profiler, uint64_t measurementId) noexcept
	{
		return (Result)zgCommandListProfileEnd(this->commandList, profiler.profiler, measurementId);
	}
};


// CommandQueue
// ------------------------------------------------------------------------------------------------

class CommandQueue final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgCommandQueue* commandQueue = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	CommandQueue() noexcept = default;
	CommandQueue(const CommandQueue&) = delete;
	CommandQueue& operator= (const CommandQueue&) = delete;
	CommandQueue(CommandQueue&& other) noexcept { this->swap(other); }
	CommandQueue& operator= (CommandQueue&& other) noexcept { this->swap(other); return *this; }
	~CommandQueue() noexcept { this->release(); }

	// See zgCommandQueueGetPresentQueue()
	static Result getPresentQueue(CommandQueue& presentQueueOut) noexcept
	{
		if (presentQueueOut.commandQueue != nullptr) return Result::INVALID_ARGUMENT;
		return (Result)zgCommandQueueGetPresentQueue(&presentQueueOut.commandQueue);
	}

	// See zgCommandQueueGetCopyQueue()
	static Result getCopyQueue(CommandQueue& copyQueueOut) noexcept
	{
		if (copyQueueOut.commandQueue != nullptr) return Result::INVALID_ARGUMENT;
		return (Result)zgCommandQueueGetCopyQueue(&copyQueueOut.commandQueue);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

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

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	// See zgCommandQueueSignalOnGpu()
	Result signalOnGpu(Fence& fenceToSignal) noexcept
	{
		return (Result)zgCommandQueueSignalOnGpu(this->commandQueue, fenceToSignal.fence);
	}

	// See zgCommandQueueWaitOnGpu()
	Result waitOnGpu(const Fence& fence) noexcept
	{
		return (Result)zgCommandQueueWaitOnGpu(this->commandQueue, fence.fence);
	}

	// See zgCommandQueueFlush()
	Result flush() noexcept
	{
		return (Result)zgCommandQueueFlush(this->commandQueue);
	}

	// See zgCommandQueueBeginCommandListRecording()
	Result beginCommandListRecording(CommandList& commandListOut) noexcept
	{
		if (commandListOut.commandList != nullptr) return Result::INVALID_ARGUMENT;
		return (Result)zgCommandQueueBeginCommandListRecording(
			this->commandQueue, &commandListOut.commandList);
	}

	// See zgCommandQueueExecuteCommandList()
	Result executeCommandList(CommandList& commandList) noexcept
	{
		ZgResult res = zgCommandQueueExecuteCommandList(this->commandQueue, commandList.commandList);
		commandList.commandList = nullptr;
		return (Result)res;
	}
};

// Context
// ------------------------------------------------------------------------------------------------

// Initializes and deinitializes the ZeroG context.
//
// ZeroG has an implicit context, but we force access to all functions directly associated with the
// implicit context through methods on this class.
class Context final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Context() noexcept = default;
	Context(const Context&) = delete;
	Context& operator= (const Context&) = delete;
	Context(Context&& other) noexcept { this->swap(other); }
	Context& operator= (Context&& other) noexcept { this->swap(other); return *this; }
	~Context() noexcept { this->deinit(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	// Creates and initializes a context, see zgContextInit()
	Result init(const ZgContextInitSettings& settings) noexcept
	{
		this->deinit();
		ZgResult res = zgContextInit(&settings);
		mInitialized = res == ZG_SUCCESS;
		return (Result)res;
	}

	// Deinitializes a context, see zgContextDeinit()
	//
	// Not necessary to call manually, will be called by the destructor.
	void deinit() noexcept
	{
		if (mInitialized) zgContextDeinit();
		mInitialized = false;
	}

	// Swaps two contexts. Since only one can be active, this is equal to a move in practice.
	void swap(Context& other) noexcept
	{
		std::swap(mInitialized, other.mInitialized);
	}

	// Version methods
	// --------------------------------------------------------------------------------------------

	// The API version used to compile ZeroG, see ZG_COMPILED_API_VERSION
	static uint32_t compiledApiVersion() noexcept { return ZG_COMPILED_API_VERSION; }

	// The API version of the ZeroG DLL you have linked with, see zgApiLinkedVersion()
	static uint32_t linkedApiVersion() noexcept
	{
		return zgApiLinkedVersion();
	}

	// Context methods
	// --------------------------------------------------------------------------------------------

	// Checks if a ZeroG context is already initialized, see zgContextAlreadyInitialized()
	static bool alreadyInitialized() noexcept
	{
		return zgContextAlreadyInitialized();
	}

	// Resizes the back buffers in the swap chain, safe to call every frame.
	//
	// See zgContextSwapchainResize()
	Result swapchainResize(uint32_t width, uint32_t height) noexcept
	{
		return (Result)zgContextSwapchainResize(width, height);
	}

	// See zgContextSwapchainSetVsync()
	Result swapchainSetVsync(bool vsync) noexcept
	{
		return (Result)zgContextSwapchainSetVsync(vsync ? ZG_TRUE : ZG_FALSE);
	}

	// See zgContextSwapchainBeginFrame()
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

	// See zgContextSwapchainFinishFrame()
	Result swapchainFinishFrame() noexcept
	{
		return (Result)zgContextSwapchainFinishFrame(nullptr, 0);
	}

	Result swapchainFinishFrame(Profiler& profiler, uint64_t measurementId) noexcept
	{
		return (Result)zgContextSwapchainFinishFrame(profiler.profiler, measurementId);
	}

	// See zgContextGetStats()
	Result getStats(ZgStats& statsOut) noexcept
	{
		return (Result)zgContextGetStats(&statsOut);
	}

	// Private members
	// --------------------------------------------------------------------------------------------
private:

	bool mInitialized = false;
};


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
	float near,
	float far) noexcept
{
	assert(0.0f < vertFovDegs);
	assert(vertFovDegs < 180.0f);
	assert(0.0f < aspect);
	assert(0.0f < near);
	assert(near < far);

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
		0.0f, 0.0f, far / (near - far), near * far / (near - far),
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createPerspectiveProjectionInfinite(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float near) noexcept
{
	assert(0.0f < vertFovDegs);
	assert(vertFovDegs < 180.0f);
	assert(0.0f < aspect);
	assert(0.0f < near);

	// Same as createPerspectiveProjection(), but let far approach infinity

	constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
	const float vertFovRads = vertFovDegs * DEG_TO_RAD;
	const float yScale = 1.0f / std::tan(vertFovRads * 0.5f);
	const float xScale = yScale / aspect;
	float matrix[16] = {
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f,-near,
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createPerspectiveProjectionReverse(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float near,
	float far) noexcept
{
	assert(0.0f < vertFovDegs);
	assert(vertFovDegs < 180.0f);
	assert(0.0f < aspect);
	assert(0.0f < near);
	assert(near < far);

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
		0.0f, 0.0f, -(far / (near - far)) - 1.0f, -(near * far / (near - far)),
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createPerspectiveProjectionReverseInfinite(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float near) noexcept
{
	assert(0.0f < vertFovDegs);
	assert(vertFovDegs < 180.0f);
	assert(0.0f < aspect);
	assert(0.0f < near);

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
		0.0f, 0.0f, 0.0f, near,
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createOrthographicProjection(
	float rowMajorMatrixOut[16],
	float width,
	float height,
	float near,
	float far) noexcept
{
	assert(0.0f < width);
	assert(0.0f < height);
	assert(0.0f < near);
	assert(near < far);

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
		0.0f, 0.0f, 1.0f / (near - far), near / (near - far),
		0.0f, 0.0f, 0.0f, 1.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

inline void createOrthographicProjectionReverse(
	float rowMajorMatrixOut[16],
	float width,
	float height,
	float near,
	float far) noexcept
{
	assert(0.0f < width);
	assert(0.0f < height);
	assert(0.0f < near);
	assert(near < far);

	// http://dev.theomader.com/depth-precision/
	// "This can be achieved by multiplying the projection matrix with a simple ‘z reversal’ matrix"
	// 1, 0, 0, 0
	// 0, 1, 0, 0
	// 0, 0, -1, 1
	// 0, 0, 0, 1

	float matrix[16] = {
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f / (near - far), 1.0f - (near / (near - far)),
		0.0f, 0.0f, 0.0f, 1.0f
	};
	memcpy(rowMajorMatrixOut, matrix, sizeof(float) * 16);
}

} // namespace zg
