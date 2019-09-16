// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include "ph/renderer/RendererState.hpp"

#include "ph/Context.hpp"

namespace ph {

// Statics
// ------------------------------------------------------------------------------------------------

static ZgOptimalClearValue floatToOptimalClearValue(float value) noexcept
{
	if (value == 0.0f) return ZG_OPTIMAL_CLEAR_VALUE_ZERO;
	if (value == 1.0f) return ZG_OPTIMAL_CLEAR_VALUE_ONE;
	sfz_assert_debug(false);
	return ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;
}

// Framebuffer types
// ------------------------------------------------------------------------------------------------

void FramebufferItem::deallocate(DynamicGpuAllocator& gpuAllocatorFramebuffer) noexcept
{
	// Deallocate framebuffer
	if (framebuffer.framebuffer.valid()) {
		framebuffer.framebuffer.release();
	}

	// Deallocate render targets
	for (uint32_t i = 0; i < framebuffer.numRenderTargets; i++) {
		if (framebuffer.renderTargets[i].valid()) {
			gpuAllocatorFramebuffer.deallocate(framebuffer.renderTargets[i]);
		}
	}
	framebuffer.numRenderTargets = 0;

	// Deallocate depth buffer
	if (framebuffer.depthBuffer.valid()) {
		gpuAllocatorFramebuffer.deallocate(framebuffer.depthBuffer);
	}
}

bool FramebufferItem::buildFramebuffer(vec2_s32 windowRes, DynamicGpuAllocator& gpuAllocatorFramebuffer) noexcept
{
	// Figure out resolution
	uint32_t width = 0;
	uint32_t height = 0;
	if (resolutionIsFixed) {
		width = uint32_t(resolutionFixed.x);
		height = uint32_t(resolutionFixed.y);
	}
	else {
		if (resolutionScaleSetting != nullptr) resolutionScale = resolutionScaleSetting->floatValue();
		vec2 scaled = vec2(windowRes) * resolutionScale;
		width = uint32_t(std::round(scaled.x));
		height = uint32_t(std::round(scaled.y));
	}

	zg::FramebufferBuilder builder;

	// Allocate render targets
	framebuffer.numRenderTargets = numRenderTargets;
	for (uint32_t i = 0; i < numRenderTargets; i++) {
		RenderTargetItem& rtItem = renderTargetItems[i];
		framebuffer.renderTargets[i] = gpuAllocatorFramebuffer.allocateTexture2D(
			rtItem.format,
			width,
			height,
			1,
			ZG_TEXTURE_USAGE_RENDER_TARGET,
			floatToOptimalClearValue(rtItem.clearValue));
		builder.addRenderTarget(framebuffer.renderTargets[i]);
	}

	// Allocate depth buffer
	if (hasDepthBuffer) {
		framebuffer.depthBuffer = gpuAllocatorFramebuffer.allocateTexture2D(
			depthBufferFormat,
			width,
			height,
			1,
			ZG_TEXTURE_USAGE_DEPTH_BUFFER,
			floatToOptimalClearValue(depthBufferClearValue));
		builder.setDepthBuffer(framebuffer.depthBuffer);
	}

	// Build framebuffer
	return CHECK_ZG builder.build(framebuffer.framebuffer);
}

//  Pipeline types
// ------------------------------------------------------------------------------------------------

bool PipelineRenderItem::buildPipeline() noexcept
{
	// Create pipeline builder
	zg::PipelineRenderBuilder pipelineBuilder;
	pipelineBuilder
		.addVertexShaderPath(vertexShaderEntry, vertexShaderPath)
		.addPixelShaderPath(pixelShaderEntry, pixelShaderPath);
	
	// Set vertex attributes
	if (standardVertexAttributes) {
		pipelineBuilder
			.addVertexBufferInfo(0, sizeof(Vertex))
			.addVertexAttribute(0, 0, ZG_VERTEX_ATTRIBUTE_F32_3, offsetof(Vertex, pos))
			.addVertexAttribute(1, 0, ZG_VERTEX_ATTRIBUTE_F32_3, offsetof(Vertex, normal))
			.addVertexAttribute(2, 0, ZG_VERTEX_ATTRIBUTE_F32_2, offsetof(Vertex, texcoord));
	}
	else {
		// TODO: Not yet implemented
		sfz_assert_release(false);
	}

	// Set push constants
	sfz_assert_debug(numPushConstants < ZG_MAX_NUM_CONSTANT_BUFFERS);
	for (uint32_t i = 0; i < numPushConstants; i++) {
		pipelineBuilder.addPushConstant(pushConstantRegisters[i]);
	}

	// Samplers
	sfz_assert_debug(numSamplers < ZG_MAX_NUM_SAMPLERS);
	for (uint32_t i = 0; i < numSamplers; i++) {
		SamplerItem& sampler = samplers[i];
		pipelineBuilder.addSampler(sampler.samplerRegister, sampler.sampler);
	}

	// Render targets
	sfz_assert_debug(numRenderTargets < ZG_MAX_NUM_RENDER_TARGETS);
	for (uint32_t i = 0; i < numRenderTargets; i++) {
		pipelineBuilder.addRenderTarget(renderTargets[i]);
	}

	// Depth test
	if (depthTest) {
		pipelineBuilder
			.setDepthTestEnabled(true)
			.setDepthFunc(depthFunc);
	}

	// Culling
	if (cullingEnabled) {
		pipelineBuilder
			.setCullingEnabled(true)
			.setCullMode(cullFrontFacing, frontFacingIsCounterClockwise);
	}

	// Wireframe rendering
	if (wireframeRenderingEnabled) {
		pipelineBuilder.setWireframeRendering(true);
	}

	// Blend mode
	if (blendMode == PipelineBlendMode::NO_BLENDING) {
		pipelineBuilder.setBlendingEnabled(false);
	}
	else if (blendMode == PipelineBlendMode::ALPHA_BLENDING) {
		pipelineBuilder
			.setBlendingEnabled(true)
			.setBlendFuncColor(ZG_BLEND_FUNC_ADD, ZG_BLEND_FACTOR_SRC_ALPHA, ZG_BLEND_FACTOR_SRC_INV_ALPHA)
			.setBlendFuncAlpha(ZG_BLEND_FUNC_ADD, ZG_BLEND_FACTOR_ONE, ZG_BLEND_FACTOR_ZERO);
	}
	else if (blendMode == PipelineBlendMode::ADDITIVE_BLENDING) {
		pipelineBuilder
			.setBlendingEnabled(true)
			.setBlendFuncColor(ZG_BLEND_FUNC_ADD, ZG_BLEND_FACTOR_ONE, ZG_BLEND_FACTOR_ONE)
			.setBlendFuncAlpha(ZG_BLEND_FUNC_ADD, ZG_BLEND_FACTOR_ONE, ZG_BLEND_FACTOR_ONE);
	}
	else {
		sfz_assert_debug(false);
	}

	// Build pipeline
	bool buildSuccess = false;
	zg::PipelineRender tmpPipeline;
	if (sourceType == PipelineSourceType::SPIRV) {
		buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileSPIRV(tmpPipeline);
	}
	else {
		buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileHLSL(tmpPipeline);
	}

	if (buildSuccess) {
		this->pipeline = std::move(tmpPipeline);
	}
	return buildSuccess;
}

// RendererConfigurableState: Helper methods
// ------------------------------------------------------------------------------------------------

zg::Framebuffer* RendererConfigurableState::getFramebuffer(
	zg::Framebuffer& defaultFramebuffer, StringID id) noexcept
{
	static StringID defaultId = []() {
		sfz::StringCollection& resStrings = getResourceStrings();
		return resStrings.getStringID("default");
	}();

	// If "default", return default framebuffer
	if (id == defaultId) return &defaultFramebuffer;

	// Otherwise linear search through framebuffers to find the correct one
	for (FramebufferItem& item : framebuffers) {
		if (item.name == id) return &item.framebuffer.framebuffer;
	}

	// Could not find framebuffer
	sfz_assert_debug(false);
	return nullptr;
}

FramebufferItem* RendererConfigurableState::getFramebufferItem(StringID id) noexcept
{
	// Linear search through framebuffers to find the correct one
	for (FramebufferItem& item : framebuffers) {
		if (item.name == id) return &item;
	}

	// Could not find framebuffer
	sfz_assert_debug(false);
	return nullptr;
}

// RendererState: Helper methods
// ------------------------------------------------------------------------------------------------

uint32_t RendererState::findNextBarrierIdx() const noexcept
{
	uint32_t numStages = this->configurable.presentQueueStages.size();
	for (uint32_t i = this->currentStageSetIdx; i < numStages; i++) {
		const Stage& stage = this->configurable.presentQueueStages[i];
		if (stage.stageType == StageType::USER_STAGE_BARRIER) return i;
	}
	return ~0u;
}

uint32_t  RendererState::findActiveStageIdx(StringID stageName) const noexcept
{
	sfz_assert_debug(stageName != StringID::invalid())
	uint32_t numStages = this->configurable.presentQueueStages.size();
	for (uint32_t i = this->currentStageSetIdx; i < numStages; i++) {
		const Stage& stage = this->configurable.presentQueueStages[i];
		if (stage.stageName == stageName) return i;
		if (stage.stageType == StageType::USER_STAGE_BARRIER) break;
	}
	return ~0u;
}

uint32_t  RendererState::findPipelineRenderIdx(StringID pipelineName) const noexcept
{
	sfz_assert_debug(pipelineName != StringID::invalid());
	uint32_t numPipelines = this->configurable.renderPipelines.size();
	for (uint32_t i = 0; i < numPipelines; i++) {
		const PipelineRenderItem& item = this->configurable.renderPipelines[i];
		if (item.name == pipelineName) return i;
	}
	return ~0u;
}

PerFrame<ConstantBufferMemory>* RendererState::findConstantBufferInCurrentInputStage(
	uint32_t shaderRegister) noexcept
{
	// Find constant buffer
	Framed<ConstantBufferMemory>* framed = currentInputEnabledStage->constantBuffers.find(
		[&](Framed<ConstantBufferMemory>& item) {
		return item.states[0].state.shaderRegister == shaderRegister;
	});
	if (framed == nullptr) return nullptr;

	// Get this frame's data
	return &framed->getState(currentFrameIdx);
}

} // namespace ph
