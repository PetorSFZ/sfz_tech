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

// Static textures
// ------------------------------------------------------------------------------------------------

void StaticTextureItem::buildTexture(vec2_i32 windowRes) noexcept
{
	StringCollection& resStrings = getResourceStrings();

	// Figure out resolution
	uint32_t tmpWidth = 0;
	uint32_t tmpHeight = 0;
	if (resolutionIsFixed) {
		tmpWidth = uint32_t(resolutionFixed.x);
		tmpHeight = uint32_t(resolutionFixed.y);
	}
	else {
		if (resolutionScaleSetting != nullptr) resolutionScale = resolutionScaleSetting->floatValue();
		vec2 scaled = vec2(windowRes) * resolutionScale;
		tmpWidth = uint32_t(std::round(scaled.x));
		tmpHeight = uint32_t(std::round(scaled.y));
	}
	this->width = tmpWidth;
	this->height = tmpHeight;

	// Allocate texture
	const bool isDepth = format == ZG_TEXTURE_FORMAT_DEPTH_F32;
	ZgTextureUsage usage = isDepth ?
		ZG_TEXTURE_USAGE_DEPTH_BUFFER : ZG_TEXTURE_USAGE_RENDER_TARGET;
	ZgOptimalClearValue optimalClear = floatToOptimalClearValue(clearValue);
	{
		ZgTextureCreateInfo createInfo = {};
		createInfo.committedAllocation = ZG_TRUE;
		createInfo.allowUnorderedAccess = isDepth ? ZG_FALSE : ZG_TRUE;
		createInfo.format = format;
		createInfo.usage = usage;
		createInfo.optimalClearValue = optimalClear;
		createInfo.width = width;
		createInfo.height = height;
		createInfo.numMipmaps = 1;
		createInfo.debugName = resStrings.getString(this->name);
		CHECK_ZG this->texture.create(createInfo);
	}
}

// Streaming buffers
// ------------------------------------------------------------------------------------------------

void StreamingBufferItem::buildBuffer(uint32_t frameLatency)
{
	const uint64_t sizeBytes = this->elementSizeBytes * this->maxNumElements;
	const char* nameStr = getResourceStrings().getString(this->name);
	uint32_t frameIdx = 0;
	this->data.init(frameLatency, [&](StreamingBufferMemory& memory) {
		str256 uploadDebugName("%s_upload_%u", nameStr, frameIdx);
		str256 deviceDebugName("%s_device_%u", nameStr, frameIdx);
		frameIdx += 1;
		CHECK_ZG memory.uploadBuffer.create(
			sizeBytes, ZG_MEMORY_TYPE_UPLOAD, committedAllocation, uploadDebugName.str());
		CHECK_ZG memory.deviceBuffer.create(
			sizeBytes, ZG_MEMORY_TYPE_DEVICE, committedAllocation, deviceDebugName.str());
	});
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
	if (inputLayout.standardVertexLayout) {
		pipelineBuilder
			.addVertexBufferInfo(0, sizeof(Vertex))
			.addVertexAttribute(0, 0, ZG_VERTEX_ATTRIBUTE_F32_3, offsetof(Vertex, pos))
			.addVertexAttribute(1, 0, ZG_VERTEX_ATTRIBUTE_F32_3, offsetof(Vertex, normal))
			.addVertexAttribute(2, 0, ZG_VERTEX_ATTRIBUTE_F32_2, offsetof(Vertex, texcoord));
	}
	else {
		pipelineBuilder.addVertexBufferInfo(0, inputLayout.vertexSizeBytes);
		for (const ZgVertexAttribute& attribute : inputLayout.attributes) {
			pipelineBuilder.addVertexAttribute(attribute);
		}
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
	zg::PipelineRender tmpPipeline;
	bool buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileHLSL(tmpPipeline);
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
	zg::PipelineCompute tmpPipeline;
	bool buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileHLSL(tmpPipeline, ZG_SHADER_MODEL_6_1);
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
	const Array<Stage>& stages = configurable.presentStageGroups[currentStageGroupIdx].stages;
	const Stage* stage = stages.find([&](const Stage& e) { return e.name == stageName; });
	if (stage == nullptr) return ~0u;
	return uint32_t(stage - stages.data());
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
