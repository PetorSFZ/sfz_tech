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

namespace ph {

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
		vec2 scaled =vec2(windowRes) * resolutionScale;
		width = std::round(scaled.x);
		height = std::round(scaled.y);
	}

	// Allocate memory and initialize framebuffer
	framebuffer.numRenderTargets = 1;
	framebuffer.renderTargets[0] = gpuAllocatorFramebuffer.allocateTexture2D(
		ZG_TEXTURE_2D_FORMAT_RGBA_U8, ZG_TEXTURE_USAGE_RENDER_TARGET, width, height, 1);

	return CHECK_ZG zg::FramebufferBuilder()
		.addRenderTarget(framebuffer.renderTargets[0])
		.build(framebuffer.framebuffer);
}

//  Pipeline types
// ------------------------------------------------------------------------------------------------

bool PipelineRenderingItem::buildPipeline() noexcept
{
	// Create pipeline builder
	zg::PipelineRenderingBuilder pipelineBuilder;
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

	// Build pipeline
	bool buildSuccess = false;
	zg::PipelineRendering tmpPipeline;
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

uint32_t  RendererState::findPipelineRenderingIdx(StringID pipelineName) const noexcept
{
	sfz_assert_debug(pipelineName != StringID::invalid());
	uint32_t numPipelines = this->configurable.renderingPipelines.size();
	for (uint32_t i = 0; i < numPipelines; i++) {
		const PipelineRenderingItem& item = this->configurable.renderingPipelines[i];
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
