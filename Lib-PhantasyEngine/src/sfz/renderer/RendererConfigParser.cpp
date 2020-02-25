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

#include "sfz/renderer/RendererConfigParser.hpp"

#include "sfz/Context.hpp"
#include "sfz/Logging.hpp"
#include "sfz/renderer/RendererState.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/util/JsonParser.hpp"

namespace sfz {

// Statics
// ------------------------------------------------------------------------------------------------

// Converts a ParsedJsonNodeValue<T> to T, logging the "exists" value along the way.
#define CHECK_JSON (CheckJsonImpl(__FILE__, __LINE__)) %

struct CheckJsonImpl final {
	const char* file;
	int line;

	CheckJsonImpl() = delete;
	CheckJsonImpl(const char* file, int line) noexcept : file(file), line(line) {}

	template<typename T>
	T operator% (const sfz::ParsedJsonNodeValue<T>& valuePair) noexcept {
		if (!valuePair.exists) {
			SFZ_ERROR("NextGenRenderer", "Key did not exist in JSON file: %s:%i", file, line);
			sfz_assert(false);
		}
		return std::move(valuePair.value);
	}
};

static ZgSamplingMode samplingModeFromString(const str256& str) noexcept
{
	if (str == "NEAREST") return ZG_SAMPLING_MODE_NEAREST;
	if (str == "TRILINEAR") return ZG_SAMPLING_MODE_TRILINEAR;
	if (str == "ANISOTROPIC") return ZG_SAMPLING_MODE_ANISOTROPIC;
	sfz_assert(false);
	return ZG_SAMPLING_MODE_UNDEFINED;
}

static ZgWrappingMode wrappingModeFromString(const str256& str) noexcept
{
	if (str == "CLAMP") return ZG_WRAPPING_MODE_CLAMP;
	if (str == "REPEAT") return ZG_WRAPPING_MODE_REPEAT;
	sfz_assert(false);
	return ZG_WRAPPING_MODE_UNDEFINED;
}

static ZgDepthFunc depthFuncFromString(const str256& str) noexcept
{
	if (str == "LESS") return ZG_DEPTH_FUNC_LESS;
	if (str == "LESS_EQUAL") return ZG_DEPTH_FUNC_LESS_EQUAL;
	if (str == "EQUAL") return ZG_DEPTH_FUNC_EQUAL;
	if (str == "NOT_EQUAL") return ZG_DEPTH_FUNC_NOT_EQUAL;
	if (str == "GREATER") return ZG_DEPTH_FUNC_GREATER;
	if (str == "GREATER_EQUAL") return ZG_DEPTH_FUNC_GREATER_EQUAL;
	sfz_assert(false);
	return ZG_DEPTH_FUNC_LESS;
}

static ZgTextureFormat textureFormatFromString(const str256& str) noexcept
{
	if (str == "R_U8_UNORM") return ZG_TEXTURE_FORMAT_R_U8_UNORM;
	if (str == "RG_U8_UNORM") return ZG_TEXTURE_FORMAT_RG_U8_UNORM;
	if (str == "RGBA_U8_UNORM") return ZG_TEXTURE_FORMAT_RGBA_U8_UNORM;

	if (str == "R_F16") return ZG_TEXTURE_FORMAT_R_F16;
	if (str == "RG_F16") return ZG_TEXTURE_FORMAT_RG_F16;
	if (str == "RGBA_F16") return ZG_TEXTURE_FORMAT_RGBA_F16;

	if (str == "R_F32") return ZG_TEXTURE_FORMAT_R_F32;
	if (str == "RG_F32") return ZG_TEXTURE_FORMAT_RG_F32;
	if (str == "RGBA_F32") return ZG_TEXTURE_FORMAT_RGBA_F32;

	if (str == "DEPTH_F32") return ZG_TEXTURE_FORMAT_DEPTH_F32;

	sfz_assert(false);
	return ZG_TEXTURE_FORMAT_UNDEFINED;
}

static PipelineBlendMode blendModeFromString(const str256& str) noexcept
{
	if (str == "no_blending") return PipelineBlendMode::NO_BLENDING;
	if (str == "alpha_blending") return PipelineBlendMode::ALPHA_BLENDING;
	if (str == "additive_blending") return PipelineBlendMode::ADDITIVE_BLENDING;

	sfz_assert(false);
	return PipelineBlendMode::NO_BLENDING;
}

// Renderer config parser functions
// ------------------------------------------------------------------------------------------------

bool parseRendererConfig(RendererState& state, const char* configPath) noexcept
{
	// Get resource strings and global config
	sfz::StringCollection& resStrings = getResourceStrings();
	GlobalConfig& cfg = getGlobalConfig();

	RendererConfigurableState& configurable = state.configurable;

	// Attempt to parse JSON file containing game common
	ParsedJson json = ParsedJson::parseFile(configPath, state.allocator);
	if (!json.isValid()) {
		SFZ_ERROR("NextGenRenderer", "Failed to load config at: %s", configPath);
		return false;
	}
	ParsedJsonNode root = json.root();

	// Ensure some necessary sections exist
	if (!root.accessMap("render_pipelines").isValid()) return false;

	// Store path to configuration
	configurable.configPath.clear();
	configurable.configPath.appendf("%s", configPath);


	// Render pipelines

	// Get number of render pipelines to load and allocate memory for them
	ParsedJsonNode renderPipelinesNode = root.accessMap("render_pipelines");
	uint32_t numRenderPipelines = renderPipelinesNode.arrayLength();
	configurable.renderPipelines.init(numRenderPipelines, state.allocator, sfz_dbg(""));

	// Parse information about each render pipeline
	for (uint32_t i = 0; i < numRenderPipelines; i++) {

		ParsedJsonNode pipelineNode = renderPipelinesNode.accessArray(i);
		configurable.renderPipelines.add(PipelineRenderItem());
		PipelineRenderItem& item = configurable.renderPipelines.last();

		str256 name = CHECK_JSON pipelineNode.accessMap("name").valueStr256();
		item.name = resStrings.getStringID(name);

		str256 sourceTypeStr = CHECK_JSON pipelineNode.accessMap("source_type").valueStr256();
		item.sourceType = [&]() {
			if (sourceTypeStr == "spirv") return PipelineSourceType::SPIRV;
			if (sourceTypeStr == "hlsl") return PipelineSourceType::HLSL;
			sfz_assert_hard(false);
			return PipelineSourceType::SPIRV;
		}();

		item.vertexShaderPath = CHECK_JSON pipelineNode.accessMap("vertex_shader_path").valueStr256();
		item.pixelShaderPath = CHECK_JSON pipelineNode.accessMap("pixel_shader_path").valueStr256();

		item.vertexShaderEntry.clear();
		item.vertexShaderEntry.appendf("%s",
			(CHECK_JSON pipelineNode.accessMap("vertex_shader_entry").valueStr256()).str());
		item.pixelShaderEntry.clear();
		item.pixelShaderEntry.appendf("%s",
			(CHECK_JSON pipelineNode.accessMap("pixel_shader_entry").valueStr256()).str());

		item.standardVertexAttributes =
			CHECK_JSON pipelineNode.accessMap("standard_vertex_attributes").valueBool();

		// Push constants registers if specified
		ParsedJsonNode pushConstantsNode = pipelineNode.accessMap("push_constant_registers");
		if (pushConstantsNode.isValid()) {
			uint32_t numPushConstants = pushConstantsNode.arrayLength();
			for (uint32_t j = 0; j < numPushConstants; j++) {
				item.pushConstRegisters.add(
					(uint32_t)(CHECK_JSON pushConstantsNode.accessArray(j).valueInt()));
			}
		}
		
		// Constant buffers which are not user settable
		// I.e., constant buffers which should not have memory allocated for them
		ParsedJsonNode nonUserSettableCBsNode =
			pipelineNode.accessMap("non_user_settable_constant_buffers");
		if (nonUserSettableCBsNode.isValid()) {
			uint32_t numNonUserSettableConstantBuffers = nonUserSettableCBsNode.arrayLength();
			for (uint32_t j = 0; j < numNonUserSettableConstantBuffers; j++) {
				item.nonUserSettableConstBuffers.add(
					(uint32_t)(CHECK_JSON nonUserSettableCBsNode.accessArray(j).valueInt()));
			}
		}

		// Samplers
		ParsedJsonNode samplersNode = pipelineNode.accessMap("samplers");
		if (samplersNode.isValid()) {
			uint32_t numSamplers = samplersNode.arrayLength();
			for (uint32_t j = 0; j < numSamplers; j++) {
				ParsedJsonNode node = samplersNode.accessArray(j);
				item.samplers.add({});
				SamplerItem& sampler = item.samplers[j];
				sampler.samplerRegister = CHECK_JSON node.accessMap("register").valueInt();
				sampler.sampler.samplingMode = samplingModeFromString(
					CHECK_JSON node.accessMap("sampling_mode").valueStr256());
				sampler.sampler.wrappingModeU = wrappingModeFromString(
					CHECK_JSON node.accessMap("wrapping_mode").valueStr256());
				sampler.sampler.wrappingModeV = sampler.sampler.wrappingModeU;
				sampler.sampler.mipLodBias = 0.0f;
			}
		}

		// Render targets
		ParsedJsonNode renderTargetsNode = pipelineNode.accessMap("render_targets");
		sfz_assert(renderTargetsNode.isValid());
		uint32_t numRenderTargets = renderTargetsNode.arrayLength();
		for (uint32_t j = 0; j < numRenderTargets; j++) {
			item.renderTargets.add(
				textureFormatFromString(CHECK_JSON renderTargetsNode.accessArray(j).valueStr256()));
		}

		// Depth test and function if specified
		ParsedJsonNode depthFuncNode = pipelineNode.accessMap("depth_func");
		if (depthFuncNode.isValid()) {
			item.depthTest = true;
			item.depthFunc = depthFuncFromString(CHECK_JSON depthFuncNode.valueStr256());
		}

		// Culling
		ParsedJsonNode cullingNode = pipelineNode.accessMap("culling");
		if (cullingNode.isValid()) {
			item.cullingEnabled = true;
			item.cullFrontFacing = CHECK_JSON cullingNode.accessMap("cull_front_face").valueBool();
			item.frontFacingIsCounterClockwise =
				CHECK_JSON cullingNode.accessMap("front_facing_is_counter_clockwise").valueBool();
		}

		// Depth bias
		ParsedJsonNode depthBiasNode = pipelineNode.accessMap("depth_bias");
		item.depthBias = 0;
		item.depthBiasSlopeScaled = 0.0f;
		item.depthBiasClamp = 0.0f;
		if (depthBiasNode.isValid()) {
			item.depthBias = CHECK_JSON depthBiasNode.accessMap("bias").valueInt();
			item.depthBiasSlopeScaled = CHECK_JSON depthBiasNode.accessMap("bias_slope_scaled").valueFloat();
			item.depthBiasClamp = CHECK_JSON depthBiasNode.accessMap("bias_clamp").valueFloat();
		}

		// Wireframe rendering
		ParsedJsonNode wireframeNode = pipelineNode.accessMap("wireframe_rendering");
		if (wireframeNode.isValid()) {
			item.wireframeRenderingEnabled = CHECK_JSON wireframeNode.valueBool();
		}

		// Alpha blending
		ParsedJsonNode blendModeNode = pipelineNode.accessMap("blend_mode");
		item.blendMode = PipelineBlendMode::NO_BLENDING;
		if (blendModeNode.isValid()) {
			item.blendMode = blendModeFromString(CHECK_JSON blendModeNode.valueStr256());
		}
	}


	// Compute pipelines

	// Get number of compute pipelines to load and allocate memory for them
	ParsedJsonNode computePipelinesNode = root.accessMap("compute_pipelines");
	uint32_t numComputePipelines = computePipelinesNode.arrayLength();
	configurable.computePipelines.init(numComputePipelines, state.allocator, sfz_dbg(""));

	// Parse information about each compute pipeline
	for (uint32_t i = 0; i < numComputePipelines; i++) {
		
	
		ParsedJsonNode pipelineNode = computePipelinesNode.accessArray(i);
		configurable.computePipelines.add(PipelineComputeItem());
		PipelineComputeItem& item = configurable.computePipelines.last();

		str256 name = CHECK_JSON pipelineNode.accessMap("name").valueStr256();
		item.name = resStrings.getStringID(name);

		str256 sourceTypeStr = CHECK_JSON pipelineNode.accessMap("source_type").valueStr256();
		item.sourceType = [&]() {
			if (sourceTypeStr == "spirv") return PipelineSourceType::SPIRV;
			if (sourceTypeStr == "hlsl") return PipelineSourceType::HLSL;
			sfz_assert_hard(false);
			return PipelineSourceType::SPIRV;
		}();

		item.computeShaderPath = CHECK_JSON pipelineNode.accessMap("compute_shader_path").valueStr256();
		item.computeShaderEntry.clear();
		item.computeShaderEntry.appendf("%s",
			(CHECK_JSON pipelineNode.accessMap("compute_shader_entry").valueStr256()).str());

		// Push constants registers if specified
		ParsedJsonNode pushConstantsNode = pipelineNode.accessMap("push_constant_registers");
		if (pushConstantsNode.isValid()) {
			uint32_t numPushConstants = pushConstantsNode.arrayLength();
			for (uint32_t j = 0; j < numPushConstants; j++) {
				item.pushConstRegisters.add(
					(uint32_t)(CHECK_JSON pushConstantsNode.accessArray(j).valueInt()));
			}
		}
		
		// Constant buffers which are not user settable
		// I.e., constant buffers which should not have memory allocated for them
		ParsedJsonNode nonUserSettableCBsNode =
			pipelineNode.accessMap("non_user_settable_constant_buffers");
		if (nonUserSettableCBsNode.isValid()) {
			uint32_t numNonUserSettableConstantBuffers = nonUserSettableCBsNode.arrayLength();
			for (uint32_t j = 0; j < numNonUserSettableConstantBuffers; j++) {
				item.nonUserSettableConstBuffers.add(
					(uint32_t)(CHECK_JSON nonUserSettableCBsNode.accessArray(j).valueInt()));
			}
		}

		// Samplers
		ParsedJsonNode samplersNode = pipelineNode.accessMap("samplers");
		if (samplersNode.isValid()) {
			uint32_t numSamplers = samplersNode.arrayLength();
			for (uint32_t j = 0; j < numSamplers; j++) {
				ParsedJsonNode node = samplersNode.accessArray(j);
				item.samplers.add({});
				SamplerItem& sampler = item.samplers[j];
				sampler.samplerRegister = CHECK_JSON node.accessMap("register").valueInt();
				sampler.sampler.samplingMode = samplingModeFromString(
					CHECK_JSON node.accessMap("sampling_mode").valueStr256());
				sampler.sampler.wrappingModeU = wrappingModeFromString(
					CHECK_JSON node.accessMap("wrapping_mode").valueStr256());
				sampler.sampler.wrappingModeV = sampler.sampler.wrappingModeU;
				sampler.sampler.mipLodBias = 0.0f;
			}
		}
	}


	// Static textures
	{
		// Get number of static textures to create and allocate memory for their handles
		ParsedJsonNode staticTexturesNode = root.accessMap("static_textures");
		uint32_t numStaticTextures = staticTexturesNode.arrayLength();
		configurable.staticTextures.init(numStaticTextures, state.allocator, sfz_dbg(""));

		// Parse information about each static texture
		for (uint32_t i = 0; i < numStaticTextures; i++) {
			
			ParsedJsonNode texNode = staticTexturesNode.accessArray(i);
			configurable.staticTextures.add(StaticTextureItem());
			StaticTextureItem& texItem = configurable.staticTextures.last();

			// Name
			str256 name = CHECK_JSON texNode.accessMap("name").valueStr256();
			sfz_assert(name != "default");
			texItem.name = resStrings.getStringID(name);

			// Format and clear value
			texItem.format = textureFormatFromString(
				CHECK_JSON texNode.accessMap("format").valueStr256());
			float clearValue = 0.0f;
			if (texNode.accessMap("clear_value").isValid()) {
				clearValue = CHECK_JSON texNode.accessMap("clear_value").valueFloat();
			}
			sfz_assert(clearValue == 0.0f || clearValue == 1.0f);
			texItem.clearValue = clearValue;

			// Resolution type
			texItem.resolutionIsFixed = !(texNode.accessMap("resolution_scale").isValid() ||
				texNode.accessMap("resolution_scale_setting").isValid());

			// Resolution
			if (texItem.resolutionIsFixed) {
				texItem.resolutionFixed.x = CHECK_JSON texNode.accessMap("resolution_fixed_width").valueInt();
				texItem.resolutionFixed.y = CHECK_JSON texNode.accessMap("resolution_fixed_height").valueInt();
			}
			else {
				bool hasSetting = texNode.accessMap("resolution_scale_setting").isValid();
				if (hasSetting) {
					str256 settingKey = CHECK_JSON texNode.accessMap("resolution_scale_setting").valueStr256();

					// Default value
					float defaultScale = 1.0f;
					if (texNode.accessMap("resolution_scale").isValid()) {
						defaultScale = CHECK_JSON texNode.accessMap("resolution_scale").valueFloat();
					}
					texItem.resolutionScaleSetting =
						cfg.sanitizeFloat("Renderer", settingKey, false, defaultScale, 0.1f, 4.0f);
					texItem.resolutionScale = texItem.resolutionScaleSetting->floatValue();
				}
				else {
					texItem.resolutionScaleSetting = nullptr;
					texItem.resolutionScale = CHECK_JSON texNode.accessMap("resolution_scale").valueFloat();
				}
			}
		}
	}

	// Present queue
	{
		StringID defaultId = resStrings.getStringID("default");

		ParsedJsonNode presentQueueNode = root.accessMap("present_queue");
		const uint32_t numGroups = presentQueueNode.arrayLength();
		
		// Allocate memory for stage groups
		configurable.presentQueue.init(numGroups, state.allocator, sfz_dbg(""));

		// Parse stage groups
		for (uint32_t groupIdx = 0; groupIdx < numGroups; groupIdx++) {
			ParsedJsonNode groupNode = presentQueueNode.accessArray(groupIdx);

			// Create group and read its name
			configurable.presentQueue.add({});
			StageGroup& group = configurable.presentQueue.last();
			group.groupName =
				resStrings.getStringID(CHECK_JSON groupNode.accessMap("group_name").valueStr256());

			// Get number of stages and allocate memory for them
			ParsedJsonNode stages = groupNode.accessMap("stages");
			const uint32_t numStages = stages.arrayLength();
			group.stages.init(numStages, state.allocator, sfz_dbg(""));

			// Stages
			for (uint32_t stageIdx = 0; stageIdx < numStages; stageIdx++) {
				ParsedJsonNode stageNode = stages.accessArray(stageIdx);
				group.stages.add({});
				Stage& stage = group.stages.last();

				str256 stageName = CHECK_JSON stageNode.accessMap("stage_name").valueStr256();
				stage.name = resStrings.getStringID(stageName);

				str256 stageType = CHECK_JSON stageNode.accessMap("stage_type").valueStr256();
				if (stageType == "USER_INPUT_RENDERING") stage.type = StageType::USER_INPUT_RENDERING;
				else return false;

				if (stage.type == StageType::USER_INPUT_RENDERING) {
					str256 renderPipelineName =
						CHECK_JSON stageNode.accessMap("render_pipeline").valueStr256();
					stage.renderPipelineName = resStrings.getStringID(renderPipelineName);

					if (stageNode.accessMap("render_targets").isValid()) {
						ParsedJsonNode renderTargetsNode = stageNode.accessMap("render_targets");
						uint32_t numRenderTargets = renderTargetsNode.arrayLength();
						for (uint32_t i = 0; i < numRenderTargets; i++) {
							str256 renderTargetName =
								CHECK_JSON renderTargetsNode.accessArray(i).valueStr256();
							stage.renderTargetNames.add(resStrings.getStringID(renderTargetName));
						}
					}

					if (stageNode.accessMap("depth_buffer").isValid()) {
						str256 depthBufferName =
							CHECK_JSON stageNode.accessMap("depth_buffer").valueStr256();
						stage.depthBufferName = resStrings.getStringID(depthBufferName);
					}

					stage.defaultFramebuffer =
						stage.renderTargetNames.size() == 1 &&
						stage.renderTargetNames[0] == defaultId &&
						stage.depthBufferName == defaultId;
				}

				// Bound textures
				if (stageNode.accessMap("bound_textures").isValid()) {
					ParsedJsonNode boundTexsNode = stageNode.accessMap("bound_textures");
					uint32_t numBoundTextures = boundTexsNode.arrayLength();

					for (uint32_t i = 0; i < numBoundTextures; i++) {
						ParsedJsonNode texNode = boundTexsNode.accessArray(i);
						BoundTexture boundTex;
						boundTex.textureRegister = CHECK_JSON texNode.accessMap("register").valueInt();
						str256 texName = CHECK_JSON texNode.accessMap("texture").valueStr256();
						boundTex.textureName = resStrings.getStringID(texName);
						stage.boundTextures.add(boundTex);
					}
				}
			}
		}
	}

	// Builds pipelines
	bool success = true;
	for (PipelineRenderItem& item : configurable.renderPipelines) {
		if (!item.buildPipeline()) {
			success = false;
		}
	}
	for (PipelineComputeItem& item : configurable.computePipelines) {
		if (!item.buildPipeline()) {
			success = false;
		}
	}

	// Create static textures
	for (StaticTextureItem& item : configurable.staticTextures) {
		item.buildTexture(state.windowRes, state.gpuAllocatorFramebuffer);
	}

	// Allocate stage memory
	if (!allocateStageMemory(state)) success = false;

	return success;
}

bool allocateStageMemory(RendererState& state) noexcept
{
	bool success = true;

	for (uint32_t groupIdx = 0; groupIdx < state.configurable.presentQueue.size(); groupIdx++) {
		StageGroup& group = state.configurable.presentQueue[groupIdx];

		for (uint32_t stageIdx = 0; stageIdx < group.stages.size(); stageIdx++) {
			Stage& stage = group.stages[stageIdx];
			if (stage.type != StageType::USER_INPUT_RENDERING) continue;

			// Create framebuffer
			stage.rebuildFramebuffer(state.configurable.staticTextures);

			// Find pipeline
			PipelineRenderItem* pipelineItem =
				state.configurable.renderPipelines.find([&](const PipelineRenderItem& item) {
					return item.name == stage.renderPipelineName;
				});
			sfz_assert(pipelineItem != nullptr);

			// Allocate CPU memory for constant buffer data
			uint32_t numConstantBuffers = pipelineItem->pipeline.bindingsSignature.numConstBuffers;
			stage.constantBuffers.init(numConstantBuffers, state.allocator, sfz_dbg(""));

			// Allocate GPU memory for all constant buffers
			for (uint32_t j = 0; j < numConstantBuffers; j++) {

				// Get constant buffer description, skip if push constant
				const ZgConstantBufferBindingDesc& desc = pipelineItem->pipeline.bindingsSignature.constBuffers[j];
				if (desc.pushConstant == ZG_TRUE) continue;

				// Check if constant buffer is marked as non-user-settable, in that case skip it
				bool nonUserSettable = pipelineItem->nonUserSettableConstBuffers
					.findElement(desc.bufferRegister) != nullptr;
				if (nonUserSettable) continue;

				// Allocate container
				stage.constantBuffers.add({});
				PerFrameData<ConstantBufferMemory>& framed = stage.constantBuffers.last();

				// Allocate ZeroG memory
				framed.init(state.frameLatency, [&](ConstantBufferMemory& item) {

					// Set shader register
					item.shaderRegister = desc.bufferRegister;

					// Allocate upload buffer
					item.uploadBuffer =
						state.gpuAllocatorUpload.allocateBuffer(desc.sizeInBytes);
					sfz_assert(item.uploadBuffer.valid());
					if (!item.uploadBuffer.valid()) success = false;

					// Allocate device buffer
					item.deviceBuffer =
						state.gpuAllocatorDevice.allocateBuffer(desc.sizeInBytes);
					sfz_assert(item.deviceBuffer.valid());
					if (!item.deviceBuffer.valid()) success = false;
				});
			}
		}
	}

	return success;
}

bool deallocateStageMemory(RendererState& state) noexcept
{
	for (StageGroup& group : state.configurable.presentQueue) {
		for (Stage& stage : group.stages) {
			for (PerFrameData<ConstantBufferMemory>& framed : stage.constantBuffers) {
				framed.destroy([&](ConstantBufferMemory& item) {

					// Deallocate upload buffer
					sfz_assert(item.uploadBuffer.valid());
					state.gpuAllocatorUpload.deallocate(item.uploadBuffer);
					sfz_assert(!item.uploadBuffer.valid());

					// Deallocate device buffer
					sfz_assert(item.deviceBuffer.valid());
					state.gpuAllocatorDevice.deallocate(item.deviceBuffer);
					sfz_assert(!item.deviceBuffer.valid());
				});
			}

			stage.constantBuffers.destroy();
		}
	}

	return true;
}

} // namespace sfz
