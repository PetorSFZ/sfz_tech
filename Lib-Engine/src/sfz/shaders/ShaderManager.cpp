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

#include "sfz/shaders/ShaderManager.hpp"

#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/rendering/Mesh.hpp"
#include "sfz/shaders/ShaderManagerState.hpp"
#include "sfz/shaders/ShaderManagerUI.hpp"

namespace sfz {

// Shader
// ------------------------------------------------------------------------------------------------

bool Shader::build() noexcept
{
	bool buildSuccess = false;
	if (this->type == ShaderType::RENDER) {
		// Create pipeline builder
		zg::PipelineRenderBuilder pipelineBuilder;
		pipelineBuilder
			.addVertexShaderPath(render.vertexShaderEntry, shaderPath)
			.addPixelShaderPath(render.pixelShaderEntry, shaderPath);

		// Set vertex attributes
		if (render.inputLayout.standardVertexLayout) {
			pipelineBuilder
				.addVertexBufferInfo(0, sizeof(Vertex))
				.addVertexAttribute(0, 0, ZG_VERTEX_ATTRIBUTE_F32_3, offsetof(Vertex, pos))
				.addVertexAttribute(1, 0, ZG_VERTEX_ATTRIBUTE_F32_3, offsetof(Vertex, normal))
				.addVertexAttribute(2, 0, ZG_VERTEX_ATTRIBUTE_F32_2, offsetof(Vertex, texcoord));
		}
		else {
			pipelineBuilder.addVertexBufferInfo(0, render.inputLayout.vertexSizeBytes);
			for (const ZgVertexAttribute& attribute : render.inputLayout.attributes) {
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
		for (uint32_t i = 0; i < render.renderTargets.size(); i++) {
			pipelineBuilder.addRenderTarget(render.renderTargets[i]);
		}

		// Depth test
		if (render.depthTest) {
			pipelineBuilder
				.setDepthTestEnabled(true)
				.setDepthFunc(render.depthFunc);
		}

		// Culling
		if (render.cullingEnabled) {
			pipelineBuilder
				.setCullingEnabled(true)
				.setCullMode(render.cullFrontFacing, render.frontFacingIsCounterClockwise);
		}

		// Depth bias
		pipelineBuilder
			.setDepthBias(render.depthBias, render.depthBiasSlopeScaled, render.depthBiasClamp);

		// Wireframe rendering
		if (render.wireframeRenderingEnabled) {
			pipelineBuilder.setWireframeRendering(true);
		}

		// Blend mode
		if (render.blendMode == PipelineBlendMode::NO_BLENDING) {
			pipelineBuilder.setBlendingEnabled(false);
		}
		else if (render.blendMode == PipelineBlendMode::ALPHA_BLENDING) {
			pipelineBuilder
				.setBlendingEnabled(true)
				.setBlendFuncColor(ZG_BLEND_FUNC_ADD, ZG_BLEND_FACTOR_SRC_ALPHA, ZG_BLEND_FACTOR_SRC_INV_ALPHA)
				.setBlendFuncAlpha(ZG_BLEND_FUNC_ADD, ZG_BLEND_FACTOR_ONE, ZG_BLEND_FACTOR_ZERO);
		}
		else if (render.blendMode == PipelineBlendMode::ADDITIVE_BLENDING) {
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
		buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileHLSL(tmpPipeline);
		if (buildSuccess) {
			this->render.pipeline = std::move(tmpPipeline);
		}
	}
	else if (this->type == ShaderType::COMPUTE) {
		// Create pipeline builder
		zg::PipelineComputeBuilder pipelineBuilder;
		pipelineBuilder
			.addComputeShaderPath(compute.computeShaderEntry, shaderPath);

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
		buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileHLSL(tmpPipeline, ZG_SHADER_MODEL_6_1);
		if (buildSuccess) {
			this->compute.pipeline = std::move(tmpPipeline);
		}
		return buildSuccess;
	}
	else {
		sfz_assert_hard(false);
	}

	return buildSuccess;
}

// ShaderManager
// ------------------------------------------------------------------------------------------------

void ShaderManager::init(uint32_t maxNumShaders, Allocator* allocator) noexcept
{
	sfz_assert(mState == nullptr);
	mState = allocator->newObject<ShaderManagerState>(sfz_dbg(""));
	mState->allocator = allocator;

	mState->shaderHandles.init(maxNumShaders, allocator, sfz_dbg(""));
	mState->shaders.init(maxNumShaders, allocator, sfz_dbg(""));
}

void ShaderManager::destroy() noexcept
{
	if (mState == nullptr) return;

	// Flush ZeroG queues to ensure no shaders are still in-use
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	Allocator* allocator = mState->allocator;
	allocator->deleteObject(mState);
	mState = nullptr;
}

// ShaderManager: Methods
// ------------------------------------------------------------------------------------------------

void ShaderManager::renderDebugUI()
{
	shaderManagerUI(*mState);
}

PoolHandle ShaderManager::getShaderHandle(const char* name) const
{
	return this->getShaderHandle(strID(name));
}

PoolHandle ShaderManager::getShaderHandle(strID name) const
{
	const PoolHandle* handle = mState->shaderHandles.get(name);
	if (handle == nullptr) return NULL_HANDLE;
	return *handle;
}

Shader* ShaderManager::getShader(PoolHandle handle)
{
	return mState->shaders.get(handle);
}

PoolHandle ShaderManager::addShader(Shader&& shader)
{
	strID name = shader.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->shaderHandles.get(name) == nullptr);
	PoolHandle handle = mState->shaders.allocate(std::move(shader));
	mState->shaderHandles.put(name, handle);
	sfz_assert(mState->shaderHandles.size() == mState->shaders.numAllocated());
	return handle;
}

void ShaderManager::removeShader(strID name)
{
	// TODO: Currently blocking, can probably be made async.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	PoolHandle handle = this->getShaderHandle(name);
	if (handle == NULL_HANDLE) return;
	mState->shaderHandles.remove(name);
	mState->shaders.deallocate(handle);
}

} // namespace sfz
