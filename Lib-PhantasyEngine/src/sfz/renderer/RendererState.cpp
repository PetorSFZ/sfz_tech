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

#include "sfz/renderer/RendererState.hpp"

#include "sfz/Context.hpp"

namespace sfz {

// Statics
// ------------------------------------------------------------------------------------------------

static ZgOptimalClearValue floatToOptimalClearValue(float value) noexcept
{
	if (value == 0.0f) return ZG_OPTIMAL_CLEAR_VALUE_ZERO;
	if (value == 1.0f) return ZG_OPTIMAL_CLEAR_VALUE_ONE;
	sfz_assert(false);
	return ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;
}

// Static resources
// ------------------------------------------------------------------------------------------------

void StaticTextureItem::buildTexture(
	vec2_i32 windowRes, DynamicGpuAllocator& gpuAllocatorFramebuffer) noexcept
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

	// Allocate texture
	ZgTextureUsage usage = format == ZG_TEXTURE_FORMAT_DEPTH_F32 ?
		ZG_TEXTURE_USAGE_DEPTH_BUFFER : ZG_TEXTURE_USAGE_RENDER_TARGET;
	ZgOptimalClearValue optimalClear = floatToOptimalClearValue(clearValue);
	this->texture =
		gpuAllocatorFramebuffer.allocateTexture2D(format, width, height, 1, usage, optimalClear);

	// Set debug name for texture
	StringCollection& resStrings = getResourceStrings();
	str128 debugName("static_tex__%s", resStrings.getString(this->name));
	CHECK_ZG this->texture.setDebugName(debugName);
}

void StaticTextureItem::deallocate(DynamicGpuAllocator& gpuAllocatorFramebuffer) noexcept
{
	if (texture.valid()) {
		gpuAllocatorFramebuffer.deallocate(texture);
	}
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
		sfz_assert_hard(false);
	}

	// Set push constants
	for (uint32_t i = 0; i < pushConstRegisters.size(); i++) {
		pipelineBuilder.addPushConstant(pushConstRegisters[i]);
	}

	// Samplers
	for (uint32_t i = 0; i < samplers.size(); i++) {
		SamplerItem& sampler = samplers[i];
		pipelineBuilder.addSampler(sampler.samplerRegister, sampler.sampler);
	}

	// Render targets
	for (uint32_t i = 0; i < renderTargets.size(); i++) {
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

	// Depth bias
	pipelineBuilder
		.setDepthBias(depthBias, depthBiasSlopeScaled, depthBiasClamp);

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
		sfz_assert(false);
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

bool PipelineComputeItem::buildPipeline() noexcept
{
	// Create pipeline builder
	zg::PipelineComputeBuilder pipelineBuilder;
	pipelineBuilder
		.addComputeShaderPath(computeShaderEntry, computeShaderPath);
	
	// Set push constants
	for (uint32_t i = 0; i < pushConstRegisters.size(); i++) {
		pipelineBuilder.addPushConstant(pushConstRegisters[i]);
	}

	// Samplers
	for (uint32_t i = 0; i < samplers.size(); i++) {
		SamplerItem& sampler = samplers[i];
		pipelineBuilder.addSampler(sampler.samplerRegister, sampler.sampler);
	}

	// Build pipeline
	bool buildSuccess = false;
	zg::PipelineCompute tmpPipeline;
	if (sourceType == PipelineSourceType::SPIRV) {
		sfz_assert_hard(false);
		//buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileSPIRV(tmpPipeline);
	}
	else {
		buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileHLSL(tmpPipeline, ZG_SHADER_MODEL_6_1);
	}

	if (buildSuccess) {
		this->pipeline = std::move(tmpPipeline);
	}
	return buildSuccess;
}

// Stage: Helper methods
// ------------------------------------------------------------------------------------------------

void Stage::rebuildFramebuffer(Array<StaticTextureItem>& staticTextures) noexcept
{
	if (type != StageType::USER_INPUT_RENDERING) return;

	StringCollection& resStrings = getResourceStrings();
	StringID defaultId = resStrings.getStringID("default");

	// Create framebuffer
	if (!render.defaultFramebuffer) {

		if (!render.renderTargetNames.isEmpty() || render.depthBufferName != StringID::invalid()) {
			zg::FramebufferBuilder fbBuilder;

			for (StringID renderTargetName : render.renderTargetNames) {
				sfz_assert(renderTargetName != defaultId);
				StaticTextureItem* renderTarget =
					staticTextures.find([&](const StaticTextureItem& e) {
						return e.name == renderTargetName;
					});
				sfz_assert(renderTarget != nullptr);
				fbBuilder.addRenderTarget(renderTarget->texture);
			}

			if (render.depthBufferName != StringID::invalid()) {
				sfz_assert(render.depthBufferName != defaultId);
				StaticTextureItem* depthBuffer =
					staticTextures.find([&](const StaticTextureItem& e) {
						return e.name == render.depthBufferName;
					});
				sfz_assert(depthBuffer != nullptr);
				fbBuilder.setDepthBuffer(depthBuffer->texture);
			}

			bool fbSuccess = CHECK_ZG fbBuilder.build(render.framebuffer);
			sfz_assert(fbSuccess);
		}
	}
}

// RendererState: Helper methods
// ------------------------------------------------------------------------------------------------

uint32_t RendererState::findActiveStageIdx(StringID stageName) const noexcept
{
	sfz_assert(stageName != StringID::invalid());
	const Array<Stage>& stages = configurable.presentQueue[currentStageGroupIdx].stages;
	const Stage* stage = stages.find([&](const Stage& e) { return e.name == stageName; });
	if (stage == nullptr) return ~0u;
	return uint32_t(stage - stages.data());
}

StageCommandList* RendererState::getStageCommandList(StringID stageName) noexcept
{
	sfz_assert(stageName != StringID::invalid());
	StageCommandList* list = groupCommandLists.find([&](const StageCommandList& e) {
		return e.stageName == stageName;
	});
	return list;
}

zg::CommandList& RendererState::inputEnabledCommandList() noexcept
{
	sfz_assert(inputEnabled.inInputMode);
	sfz_assert(inputEnabled.commandList);
	return inputEnabled.commandList->commandList;
}

uint32_t RendererState::findPipelineRenderIdx(StringID pipelineName) const noexcept
{
	sfz_assert(pipelineName != StringID::invalid());
	uint32_t numPipelines = this->configurable.renderPipelines.size();
	for (uint32_t i = 0; i < numPipelines; i++) {
		const PipelineRenderItem& item = this->configurable.renderPipelines[i];
		if (item.name == pipelineName) return i;
	}
	return ~0u;
}

uint32_t RendererState::findPipelineComputeIdx(StringID pipelineName) const noexcept
{
	sfz_assert(pipelineName != StringID::invalid());
	uint32_t numPipelines = this->configurable.computePipelines.size();
	for (uint32_t i = 0; i < numPipelines; i++) {
		const PipelineComputeItem& item = this->configurable.computePipelines[i];
		if (item.name == pipelineName) return i;
	}
	return ~0u;
}

ConstantBufferMemory* RendererState::findConstantBufferInCurrentInputStage(
	uint32_t shaderRegister) noexcept
{
	// Find constant buffer
	PerFrameData<ConstantBufferMemory>* data = inputEnabled.stage->constantBuffers.find(
		[&](PerFrameData<ConstantBufferMemory>& item) {
		return item.data(0).shaderRegister == shaderRegister;
	});
	if (data == nullptr) return nullptr;

	// Get this frame's data
	return &data->data(currentFrameIdx);
}

} // namespace sfz
