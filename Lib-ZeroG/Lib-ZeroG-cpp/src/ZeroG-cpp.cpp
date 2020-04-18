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

#include "ZeroG-cpp.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <utility>

namespace zg {

// Context: State methods
// ------------------------------------------------------------------------------------------------

Result Context::init(const ZgContextInitSettings& settings) noexcept
{
	this->deinit();
	ZgResult res = zgContextInit(&settings);
	mInitialized = res == ZG_SUCCESS;
	return (Result)res;
}

void Context::deinit() noexcept
{
	if (mInitialized) zgContextDeinit();
	mInitialized = false;
}

void Context::swap(Context& other) noexcept
{
	std::swap(mInitialized, other.mInitialized);
}

// Context: Version methods
// ------------------------------------------------------------------------------------------------

uint32_t Context::linkedApiVersion() noexcept
{
	return zgApiLinkedVersion();
}

// Context: Context methods
// ------------------------------------------------------------------------------------------------

bool Context::alreadyInitialized() noexcept
{
	return zgContextAlreadyInitialized();
}

Result Context::swapchainResize(uint32_t width, uint32_t height) noexcept
{
	return (Result)zgContextSwapchainResize(width, height);
}

Result Context::swapchainSetVsync(bool vsync) noexcept
{
	return (Result)zgContextSwapchainSetVsync(vsync ? ZG_TRUE : ZG_FALSE);
}

Result Context::swapchainBeginFrame(Framebuffer& framebufferOut) noexcept
{
	if (framebufferOut.valid()) return Result::INVALID_ARGUMENT;
	Result res = (Result)zgContextSwapchainBeginFrame(&framebufferOut.framebuffer, nullptr, nullptr);
	if (!isSuccess(res)) return res;
	return (Result)zgFramebufferGetResolution(
		framebufferOut.framebuffer, &framebufferOut.width, &framebufferOut.height);
}

Result Context::swapchainBeginFrame(
	Framebuffer& framebufferOut, Profiler& profiler, uint64_t& measurementIdOut) noexcept
{
	if (framebufferOut.valid()) return Result::INVALID_ARGUMENT;
	Result res = (Result)zgContextSwapchainBeginFrame(
		&framebufferOut.framebuffer, profiler.profiler, &measurementIdOut);
	if (!isSuccess(res)) return res;
	return (Result)zgFramebufferGetResolution(
		framebufferOut.framebuffer, &framebufferOut.width, &framebufferOut.height);
}

Result Context::swapchainFinishFrame() noexcept
{
	return (Result)zgContextSwapchainFinishFrame(nullptr, 0);
}

Result Context::swapchainFinishFrame(Profiler& profiler, uint64_t measurementId) noexcept
{
	return (Result)zgContextSwapchainFinishFrame(profiler.profiler, measurementId);
}

Result Context::getStats(ZgStats& statsOut) noexcept
{
	return (Result)zgContextGetStats(&statsOut);
}


// PipelineBindings: Methods
// ------------------------------------------------------------------------------------------------

PipelineBindings& PipelineBindings::addConstantBuffer(ZgConstantBufferBinding binding) noexcept
{
	assert(bindings.numConstantBuffers < ZG_MAX_NUM_CONSTANT_BUFFERS);
	bindings.constantBuffers[bindings.numConstantBuffers] = binding;
	bindings.numConstantBuffers += 1;
	return *this;
}

PipelineBindings& PipelineBindings::addConstantBuffer(
	uint32_t bufferRegister, Buffer& buffer) noexcept
{
	ZgConstantBufferBinding binding = {};
	binding.bufferRegister = bufferRegister;
	binding.buffer = buffer.buffer;
	return this->addConstantBuffer(binding);
}

PipelineBindings& PipelineBindings::addUnorderedBuffer(ZgUnorderedBufferBinding binding) noexcept
{
	assert(bindings.numUnorderedBuffers < ZG_MAX_NUM_UNORDERED_BUFFERS);
	bindings.unorderedBuffers[bindings.numUnorderedBuffers] = binding;
	bindings.numUnorderedBuffers += 1;
	return *this;
}

PipelineBindings& PipelineBindings::addUnorderedBuffer(
	uint32_t unorderedRegister,
	uint32_t numElements,
	uint32_t elementStrideBytes,
	Buffer& buffer) noexcept
{
	return this->addUnorderedBuffer(unorderedRegister, 0, numElements, elementStrideBytes, buffer);
}

PipelineBindings& PipelineBindings::addUnorderedBuffer(
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

PipelineBindings& PipelineBindings::addTexture(ZgTextureBinding binding) noexcept
{
	assert(bindings.numTextures < ZG_MAX_NUM_TEXTURES);
	bindings.textures[bindings.numTextures] = binding;
	bindings.numTextures += 1;
	return *this;
}

PipelineBindings& PipelineBindings::addTexture(
	uint32_t textureRegister, Texture2D& texture) noexcept
{
	ZgTextureBinding binding;
	binding.textureRegister = textureRegister;
	binding.texture = texture.texture;
	return this->addTexture(binding);
}

PipelineBindings& PipelineBindings::addUnorderedTexture(ZgUnorderedTextureBinding binding) noexcept
{
	assert(bindings.numUnorderedTextures < ZG_MAX_NUM_UNORDERED_TEXTURES);
	bindings.unorderedTextures[bindings.numUnorderedTextures] = binding;
	bindings.numUnorderedTextures += 1;
	return *this;
}

PipelineBindings& PipelineBindings::addUnorderedTexture(
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


// PipelineComputeBuilder: Methods
// ------------------------------------------------------------------------------------------------

PipelineComputeBuilder& PipelineComputeBuilder::addComputeShaderPath(const char* entry, const char* path) noexcept
{
	createInfo.computeShaderEntry = entry;
	computeShaderPath = path;
	return *this;
}

PipelineComputeBuilder& PipelineComputeBuilder::addComputeShaderSource(const char* entry, const char* src) noexcept
{
	createInfo.computeShaderEntry = entry;
	computeShaderSrc = src;
	return *this;
}

PipelineComputeBuilder& PipelineComputeBuilder::addPushConstant(uint32_t constantBufferRegister) noexcept
{
	assert(createInfo.numPushConstants < ZG_MAX_NUM_CONSTANT_BUFFERS);
	createInfo.pushConstantRegisters[createInfo.numPushConstants] = constantBufferRegister;
	createInfo.numPushConstants += 1;
	return *this;
}

PipelineComputeBuilder& PipelineComputeBuilder::addSampler(uint32_t samplerRegister, ZgSampler sampler) noexcept
{
	assert(samplerRegister == createInfo.numSamplers);
	assert(createInfo.numSamplers < ZG_MAX_NUM_SAMPLERS);
	createInfo.samplers[samplerRegister] = sampler;
	createInfo.numSamplers += 1;
	return *this;
}

PipelineComputeBuilder& PipelineComputeBuilder::addSampler(
	uint32_t samplerRegister,
	ZgSamplingMode samplingMode,
	ZgWrappingMode wrappingModeU,
	ZgWrappingMode wrappingModeV,
	float mipLodBias) noexcept
{
	ZgSampler sampler = {};
	sampler.samplingMode = samplingMode;
	sampler.wrappingModeU = wrappingModeU;
	sampler.wrappingModeV = wrappingModeV;
	sampler.mipLodBias = mipLodBias;
	return addSampler(samplerRegister, sampler);
}

Result PipelineComputeBuilder::buildFromFileHLSL(
	PipelineCompute& pipelineOut, ZgShaderModel model) noexcept
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


// PipelineCompute: State methods
// ------------------------------------------------------------------------------------------------

void PipelineCompute::swap(PipelineCompute& other) noexcept
{
	std::swap(this->pipeline, other.pipeline);
	std::swap(this->bindingsSignature, other.bindingsSignature);
	std::swap(this->computeSignature, other.computeSignature);
}

Result PipelineCompute::createFromFileHLSL(
	const ZgPipelineComputeCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
{
	this->release();
	return (Result)zgPipelineComputeCreateFromFileHLSL(
		&this->pipeline, &this->bindingsSignature, &this->computeSignature, &createInfo, &compileSettings);
}

void PipelineCompute::release() noexcept
{
	if (this->pipeline != nullptr) zgPipelineComputeRelease(this->pipeline);
	this->pipeline = nullptr;
	this->bindingsSignature = {};
	this->computeSignature = {};
}


// PipelineRenderBuilder: Methods
// ------------------------------------------------------------------------------------------------

PipelineRenderBuilder& PipelineRenderBuilder::addVertexShaderPath(
	const char* entry, const char* path) noexcept
{
	createInfo.vertexShaderEntry = entry;
	vertexShaderPath = path;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::addPixelShaderPath(
	const char* entry, const char* path) noexcept
{
	createInfo.pixelShaderEntry = entry;
	pixelShaderPath = path;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::addVertexShaderSource(
	const char* entry, const char* src) noexcept
{
	createInfo.vertexShaderEntry = entry;
	vertexShaderSrc = src;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::addPixelShaderSource(
	const char* entry, const char* src) noexcept
{
	createInfo.pixelShaderEntry = entry;
	pixelShaderSrc = src;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::addVertexAttribute(
	ZgVertexAttribute attribute) noexcept
{
	assert(createInfo.numVertexAttributes < ZG_MAX_NUM_VERTEX_ATTRIBUTES);
	createInfo.vertexAttributes[createInfo.numVertexAttributes] = attribute;
	createInfo.numVertexAttributes += 1;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::addVertexAttribute(
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

PipelineRenderBuilder& PipelineRenderBuilder::addVertexBufferInfo(
	uint32_t slot, uint32_t vertexBufferStrideBytes) noexcept
{
	assert(slot == createInfo.numVertexBufferSlots);
	assert(createInfo.numVertexBufferSlots < ZG_MAX_NUM_VERTEX_ATTRIBUTES);
	createInfo.vertexBufferStridesBytes[slot] = vertexBufferStrideBytes;
	createInfo.numVertexBufferSlots += 1;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::addPushConstant(
	uint32_t constantBufferRegister) noexcept
{
	assert(createInfo.numPushConstants < ZG_MAX_NUM_CONSTANT_BUFFERS);
	createInfo.pushConstantRegisters[createInfo.numPushConstants] = constantBufferRegister;
	createInfo.numPushConstants += 1;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::addSampler(
	uint32_t samplerRegister, ZgSampler sampler) noexcept
{
	assert(samplerRegister == createInfo.numSamplers);
	assert(createInfo.numSamplers < ZG_MAX_NUM_SAMPLERS);
	createInfo.samplers[samplerRegister] = sampler;
	createInfo.numSamplers += 1;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::addSampler(
	uint32_t samplerRegister,
	ZgSamplingMode samplingMode,
	ZgWrappingMode wrappingModeU,
	ZgWrappingMode wrappingModeV,
	float mipLodBias) noexcept
{
	ZgSampler sampler = {};
	sampler.samplingMode = samplingMode;
	sampler.wrappingModeU = wrappingModeU;
	sampler.wrappingModeV = wrappingModeV;
	sampler.mipLodBias = mipLodBias;
	return addSampler(samplerRegister, sampler);
}

PipelineRenderBuilder& PipelineRenderBuilder::addRenderTarget(ZgTextureFormat format) noexcept
{
	assert(createInfo.numRenderTargets < ZG_MAX_NUM_RENDER_TARGETS);
	createInfo.renderTargets[createInfo.numRenderTargets] = format;
	createInfo.numRenderTargets += 1;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::setWireframeRendering(
	bool wireframeEnabled) noexcept
{
	createInfo.rasterizer.wireframeMode = wireframeEnabled ? ZG_TRUE : ZG_FALSE;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::setCullingEnabled(bool cullingEnabled) noexcept
{
	createInfo.rasterizer.cullingEnabled = cullingEnabled ? ZG_TRUE : ZG_FALSE;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::setCullMode(
	bool cullFrontFacing, bool fontFacingIsCounterClockwise) noexcept
{
	createInfo.rasterizer.cullFrontFacing = cullFrontFacing ? ZG_TRUE : ZG_FALSE;
	createInfo.rasterizer.frontFacingIsCounterClockwise =
		fontFacingIsCounterClockwise ? ZG_TRUE : ZG_FALSE;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::setDepthBias(
	int32_t bias, float biasSlopeScaled, float biasClamp) noexcept
{
	createInfo.rasterizer.depthBias = bias;
	createInfo.rasterizer.depthBiasSlopeScaled = biasSlopeScaled;
	createInfo.rasterizer.depthBiasClamp = biasClamp;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::setBlendingEnabled(bool blendingEnabled) noexcept
{
	createInfo.blending.blendingEnabled = blendingEnabled ? ZG_TRUE : ZG_FALSE;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::setBlendFuncColor(
	ZgBlendFunc func, ZgBlendFactor srcFactor, ZgBlendFactor dstFactor) noexcept
{
	createInfo.blending.blendFuncColor = func;
	createInfo.blending.srcValColor = srcFactor;
	createInfo.blending.dstValColor = dstFactor;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::setBlendFuncAlpha(
	ZgBlendFunc func, ZgBlendFactor srcFactor, ZgBlendFactor dstFactor) noexcept
{
	createInfo.blending.blendFuncAlpha = func;
	createInfo.blending.srcValAlpha = srcFactor;
	createInfo.blending.dstValAlpha = dstFactor;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::setDepthTestEnabled(
	bool depthTestEnabled) noexcept
{
	createInfo.depthTest.depthTestEnabled = depthTestEnabled ? ZG_TRUE : ZG_FALSE;
	return *this;
}

PipelineRenderBuilder& PipelineRenderBuilder::setDepthFunc(ZgDepthFunc depthFunc) noexcept
{
	createInfo.depthTest.depthFunc = depthFunc;
	return *this;
}

Result PipelineRenderBuilder::buildFromFileSPIRV(
	PipelineRender& pipelineOut) noexcept
{
	// Set path
	createInfo.vertexShader = this->vertexShaderPath;
	createInfo.pixelShader = this->pixelShaderPath;

	// Build pipeline
	return pipelineOut.createFromFileSPIRV(createInfo);
}

Result PipelineRenderBuilder::buildFromFileHLSL(
	PipelineRender& pipelineOut, ZgShaderModel model) noexcept
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

Result PipelineRenderBuilder::buildFromSourceHLSL(
	PipelineRender& pipelineOut, ZgShaderModel model) noexcept
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


// PipelineRender: State methods
// ------------------------------------------------------------------------------------------------

Result PipelineRender::createFromFileSPIRV(
	const ZgPipelineRenderCreateInfo& createInfo) noexcept
{
	this->release();
	return (Result)zgPipelineRenderCreateFromFileSPIRV(
		&this->pipeline, &this->bindingsSignature, &this->renderSignature, &createInfo);
}

Result PipelineRender::createFromFileHLSL(
	const ZgPipelineRenderCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
{
	this->release();
	return (Result)zgPipelineRenderCreateFromFileHLSL(
		&this->pipeline, &this->bindingsSignature, &this->renderSignature, &createInfo, &compileSettings);
}

Result PipelineRender::createFromSourceHLSL(
	const ZgPipelineRenderCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
{
	this->release();
	return (Result)zgPipelineRenderCreateFromSourceHLSL(
		&this->pipeline, &this->bindingsSignature, &this->renderSignature, &createInfo, &compileSettings);
}

void PipelineRender::swap(PipelineRender& other) noexcept
{
	std::swap(this->pipeline, other.pipeline);
	std::swap(this->bindingsSignature, other.bindingsSignature);
	std::swap(this->renderSignature, other.renderSignature);
}

void PipelineRender::release() noexcept
{
	if (this->pipeline != nullptr) zgPipelineRenderRelease(this->pipeline);
	this->pipeline = nullptr;
	this->bindingsSignature = {};
	this->renderSignature = {};
}


// MemoryHeap: State methods
// ------------------------------------------------------------------------------------------------

Result MemoryHeap::create(const ZgMemoryHeapCreateInfo& createInfo) noexcept
{
	this->release();
	return (Result)zgMemoryHeapCreate(&this->memoryHeap, &createInfo);
}

Result MemoryHeap::create(uint64_t sizeInBytes, ZgMemoryType memoryType) noexcept
{
	ZgMemoryHeapCreateInfo createInfo = {};
	createInfo.sizeInBytes = sizeInBytes;
	createInfo.memoryType = memoryType;
	return this->create(createInfo);
}

void MemoryHeap::swap(MemoryHeap& other) noexcept
{
	std::swap(this->memoryHeap, other.memoryHeap);
}

void MemoryHeap::release() noexcept
{
	if (this->memoryHeap != nullptr) zgMemoryHeapRelease(this->memoryHeap);
	this->memoryHeap = nullptr;
}

// MemoryHeap: MemoryHeap methods
// ------------------------------------------------------------------------------------------------

Result MemoryHeap::bufferCreate(zg::Buffer& bufferOut, const ZgBufferCreateInfo& createInfo) noexcept
{
	bufferOut.release();
	return (Result)zgMemoryHeapBufferCreate(this->memoryHeap, &bufferOut.buffer, &createInfo);
}

Result MemoryHeap::bufferCreate(Buffer& bufferOut, uint64_t offset, uint64_t size) noexcept
{
	ZgBufferCreateInfo createInfo = {};
	createInfo.offsetInBytes = offset;
	createInfo.sizeInBytes = size;
	return this->bufferCreate(bufferOut, createInfo);
}

Result MemoryHeap::texture2DCreate(
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


// Buffer: State methods
// --------------------------------------------------------------------------------------------

void Buffer::swap(Buffer& other) noexcept
{
	std::swap(this->buffer, other.buffer);
}

void Buffer::release() noexcept
{
	if (this->buffer != nullptr) {
		zgBufferRelease(this->buffer);
	}
	this->buffer = nullptr;
}

// Buffer: Buffer methods
// --------------------------------------------------------------------------------------------

Result Buffer::memcpyTo(uint64_t bufferOffsetBytes, const void* srcMemory, uint64_t numBytes)
{
	return (Result)zgBufferMemcpyTo(this->buffer, bufferOffsetBytes, srcMemory, numBytes);
}

Result Buffer::memcpyFrom(void* dstMemory, uint64_t srcBufferOffsetBytes, uint64_t numBytes)
{
	return (Result)zgBufferMemcpyFrom(dstMemory, this->buffer, srcBufferOffsetBytes, numBytes);
}

Result Buffer::setDebugName(const char* name) noexcept
{
	return (Result)zgBufferSetDebugName(this->buffer, name);
}


// Texture2D: State methods
// ------------------------------------------------------------------------------------------------

void Texture2D::swap(Texture2D& other) noexcept
{
	std::swap(this->texture, other.texture);
	std::swap(this->width, other.width);
	std::swap(this->height, other.height);
}

void Texture2D::release() noexcept
{
	if (this->texture != nullptr) zgTexture2DRelease(this->texture);
	this->texture = nullptr;
	this->width = 0;
	this->height = 0;
}

// Texture2D: Methods
// ------------------------------------------------------------------------------------------------

// See zgTexture2DGetAllocationInfo
Result Texture2D::getAllocationInfo(
	ZgTexture2DAllocationInfo& allocationInfoOut,
	const ZgTexture2DCreateInfo& createInfo) noexcept
{
	return (Result)zgTexture2DGetAllocationInfo(&allocationInfoOut, &createInfo);
}

Result Texture2D::setDebugName(const char* name) noexcept
{
	return (Result)zgTexture2DSetDebugName(this->texture, name);
}


// FramebufferBuilder: Methods
// ------------------------------------------------------------------------------------------------

FramebufferBuilder& FramebufferBuilder::addRenderTarget(Texture2D& renderTarget) noexcept
{
	assert(createInfo.numRenderTargets < ZG_MAX_NUM_RENDER_TARGETS);
	uint32_t idx = createInfo.numRenderTargets;
	createInfo.numRenderTargets += 1;
	createInfo.renderTargets[idx] = renderTarget.texture;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::setDepthBuffer(Texture2D& depthBuffer) noexcept
{
	createInfo.depthBuffer = depthBuffer.texture;
	return *this;
}

Result FramebufferBuilder::build(Framebuffer& framebufferOut) noexcept
{
	return framebufferOut.create(this->createInfo);
}


// Framebuffer: State methods
// ------------------------------------------------------------------------------------------------

Result Framebuffer::create(const ZgFramebufferCreateInfo& createInfo) noexcept
{
	this->release();
	Result res = (Result)zgFramebufferCreate(&this->framebuffer, &createInfo);
	if (!isSuccess(res)) return res;
	return (Result)zgFramebufferGetResolution(this->framebuffer, &this->width, &this->height);
}

void Framebuffer::swap(Framebuffer& other) noexcept
{
	std::swap(this->framebuffer, other.framebuffer);
	std::swap(this->width, other.width);
	std::swap(this->height, other.height);
}

void Framebuffer::release() noexcept
{
	if (this->framebuffer != nullptr) zgFramebufferRelease(this->framebuffer);
	this->framebuffer = nullptr;
	this->width = 0;
	this->height = 0;
}


// Fence: State methods
// ------------------------------------------------------------------------------------------------

Result Fence::create() noexcept
{
	this->release();
	return (Result)zgFenceCreate(&this->fence);
}

void Fence::swap(Fence& other) noexcept
{
	std::swap(this->fence, other.fence);
}

void Fence::release() noexcept
{
	if (this->fence != nullptr) zgFenceRelease(this->fence);
	this->fence = nullptr;
}

// Fence: Fence methods
// ------------------------------------------------------------------------------------------------

Result Fence::reset() noexcept
{
	return (Result)zgFenceReset(this->fence);
}

Result Fence::checkIfSignaled(bool& fenceSignaledOut) const noexcept
{
	ZgBool signaled = ZG_FALSE;
	Result res = (Result)zgFenceCheckIfSignaled(this->fence, &signaled);
	fenceSignaledOut = signaled == ZG_FALSE ? false : true;
	return res;
}

bool Fence::checkIfSignaled() const noexcept
{
	bool signaled = false;
	[[maybe_unused]] Result res = this->checkIfSignaled(signaled);
	return signaled;
}

Result Fence::waitOnCpuBlocking() const noexcept
{
	return (Result)zgFenceWaitOnCpuBlocking(this->fence);
}


// CommandQueue: Constructors & destructors
// ------------------------------------------------------------------------------------------------

Result CommandQueue::getPresentQueue(CommandQueue& presentQueueOut) noexcept
{
	if (presentQueueOut.commandQueue != nullptr) return Result::INVALID_ARGUMENT;
	return (Result)zgCommandQueueGetPresentQueue(&presentQueueOut.commandQueue);
}

Result CommandQueue::getCopyQueue(CommandQueue& copyQueueOut) noexcept
{
	if (copyQueueOut.commandQueue != nullptr) return Result::INVALID_ARGUMENT;
	return (Result)zgCommandQueueGetCopyQueue(&copyQueueOut.commandQueue);
}

// CommandQueue: State methods
// ------------------------------------------------------------------------------------------------

void CommandQueue::swap(CommandQueue& other) noexcept
{
	std::swap(this->commandQueue, other.commandQueue);
}
void CommandQueue::release() noexcept
{
	// TODO: Currently there is no destruction of command queues as there is only one
	this->commandQueue = nullptr;
}

// CommandQueue: CommandQueue methods
// ------------------------------------------------------------------------------------------------

Result CommandQueue::signalOnGpu(Fence& fenceToSignal) noexcept
{
	return (Result)zgCommandQueueSignalOnGpu(this->commandQueue, fenceToSignal.fence);
}

Result CommandQueue::waitOnGpu(const Fence& fence) noexcept
{
	return (Result)zgCommandQueueWaitOnGpu(this->commandQueue, fence.fence);
}

Result CommandQueue::flush() noexcept
{
	return (Result)zgCommandQueueFlush(this->commandQueue);
}

Result CommandQueue::beginCommandListRecording(CommandList& commandListOut) noexcept
{
	if (commandListOut.commandList != nullptr) return Result::INVALID_ARGUMENT;
	return (Result)zgCommandQueueBeginCommandListRecording(
		this->commandQueue, &commandListOut.commandList);
}

Result CommandQueue::executeCommandList(CommandList& commandList) noexcept
{
	ZgResult res = zgCommandQueueExecuteCommandList(this->commandQueue, commandList.commandList);
	commandList.commandList = nullptr;
	return (Result)res;
}


// CommandList: State methods
// ------------------------------------------------------------------------------------------------

void CommandList::swap(CommandList& other) noexcept
{
	std::swap(this->commandList, other.commandList);
}

void CommandList::release() noexcept
{
	// TODO: Currently there is no destruction of command lists as they are owned by the
	//       CommandQueue.
	this->commandList = nullptr;
}

// CommandList: CommandList methods
// ------------------------------------------------------------------------------------------------

Result CommandList::memcpyBufferToBuffer(
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

Result CommandList::memcpyToTexture(
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

Result CommandList::enableQueueTransition(Buffer& buffer) noexcept
{
	return (Result)zgCommandListEnableQueueTransitionBuffer(this->commandList, buffer.buffer);
}

Result CommandList::enableQueueTransition(Texture2D& texture) noexcept
{
	return (Result)zgCommandListEnableQueueTransitionTexture(this->commandList, texture.texture);
}

Result CommandList::setPushConstant(
	uint32_t shaderRegister, const void* data, uint32_t dataSizeInBytes) noexcept
{
	return (Result)zgCommandListSetPushConstant(
		this->commandList, shaderRegister, data, dataSizeInBytes);
}

Result CommandList::setPipelineBindings(const PipelineBindings& bindings) noexcept
{
	return (Result)zgCommandListSetPipelineBindings(this->commandList, &bindings.bindings);
}

Result CommandList::setPipeline(PipelineCompute& pipeline) noexcept
{
	return (Result)zgCommandListSetPipelineCompute(this->commandList, pipeline.pipeline);
}

Result CommandList::unorderedBarrier(Buffer& buffer) noexcept
{
	return (Result)zgCommandListUnorderedBarrierBuffer(this->commandList, buffer.buffer);
}

Result CommandList::unorderedBarrier(Texture2D& texture) noexcept
{
	return (Result)zgCommandListUnorderedBarrierTexture(this->commandList, texture.texture);
}

Result CommandList::unorderedBarrier() noexcept
{
	return (Result)zgCommandListUnorderedBarrierAll(this->commandList);
}

Result CommandList::dispatchCompute(
	uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) noexcept
{
	return (Result)zgCommandListDispatchCompute(
		this->commandList, groupCountX, groupCountY, groupCountZ);
}

Result CommandList::setPipeline(PipelineRender& pipeline) noexcept
{
	return (Result)zgCommandListSetPipelineRender(this->commandList, pipeline.pipeline);
}

Result CommandList::setFramebuffer(
	Framebuffer& framebuffer,
	const ZgFramebufferRect* optionalViewport,
	const ZgFramebufferRect* optionalScissor) noexcept
{
	return (Result)zgCommandListSetFramebuffer(
		this->commandList, framebuffer.framebuffer, optionalViewport, optionalScissor);
}

Result CommandList::setFramebufferViewport(
	const ZgFramebufferRect& viewport) noexcept
{
	return (Result)zgCommandListSetFramebufferViewport(this->commandList, &viewport);
}

Result CommandList::setFramebufferScissor(
	const ZgFramebufferRect& scissor) noexcept
{
	return (Result)zgCommandListSetFramebufferScissor(this->commandList, &scissor);
}

Result CommandList::clearFramebufferOptimal() noexcept
{
	return (Result)zgCommandListClearFramebufferOptimal(this->commandList);
}

Result CommandList::clearRenderTargets(float red, float green, float blue, float alpha) noexcept
{
	return (Result)zgCommandListClearRenderTargets(this->commandList, red, green, blue, alpha);
}

Result CommandList::clearDepthBuffer(float depth) noexcept
{
	return (Result)zgCommandListClearDepthBuffer(this->commandList, depth);
}

Result CommandList::setIndexBuffer(Buffer& indexBuffer, ZgIndexBufferType type) noexcept
{
	return (Result)zgCommandListSetIndexBuffer(this->commandList, indexBuffer.buffer, type);
}

Result CommandList::setVertexBuffer(uint32_t vertexBufferSlot, Buffer& vertexBuffer) noexcept
{
	return (Result)zgCommandListSetVertexBuffer(
		this->commandList, vertexBufferSlot, vertexBuffer.buffer);
}

Result CommandList::drawTriangles(uint32_t startVertexIndex, uint32_t numVertices) noexcept
{
	return (Result)zgCommandListDrawTriangles(this->commandList, startVertexIndex, numVertices);
}

Result CommandList::drawTrianglesIndexed(uint32_t startIndex, uint32_t numTriangles) noexcept
{
	return (Result)zgCommandListDrawTrianglesIndexed(
		this->commandList, startIndex, numTriangles);
}

Result CommandList::profileBegin(Profiler& profiler, uint64_t& measurementIdOut) noexcept
{
	return (Result)zgCommandListProfileBegin(this->commandList, profiler.profiler, &measurementIdOut);
}

Result CommandList::profileEnd(Profiler& profiler, uint64_t measurementId) noexcept
{
	return (Result)zgCommandListProfileEnd(this->commandList, profiler.profiler, measurementId);
}


// Profiler: State methods
// ------------------------------------------------------------------------------------------------

Result Profiler::create(const ZgProfilerCreateInfo& createInfo) noexcept
{
	this->release();
	return (Result)zgProfilerCreate(&this->profiler, &createInfo);
}

void Profiler::swap(Profiler& other) noexcept
{
	std::swap(this->profiler, other.profiler);
}

void Profiler::release() noexcept
{
	if (this->profiler != nullptr) zgProfilerRelease(this->profiler);
	this->profiler = nullptr;
}

// Profiler: Methods
// ------------------------------------------------------------------------------------------------

Result Profiler::getMeasurement(
	uint64_t measurementId,
	float& measurementMsOut) noexcept
{
	return (Result)zgProfilerGetMeasurement(this->profiler, measurementId, &measurementMsOut);
}


// Transformation and projection matrices
// ------------------------------------------------------------------------------------------------

void createViewMatrix(
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

void createPerspectiveProjection(
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

void createPerspectiveProjectionInfinite(
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

void createPerspectiveProjectionReverse(
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

void createPerspectiveProjectionReverseInfinite(
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

void createOrthographicProjection(
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

void createOrthographicProjectionReverse(
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
