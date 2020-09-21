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
	T operator% (const sfz::JsonNodeValue<T>& valuePair) noexcept {
		if (!valuePair.exists) {
			SFZ_ERROR("NextGenRenderer", "Key did not exist in JSON file: %s:%i", file, line);
			sfz_assert(false);
		}
		return std::move(valuePair.value);
	}
};

static ZgVertexAttributeType attributeTypeFromString(const str256& str) noexcept
{
	if (str == "F32") return ZG_VERTEX_ATTRIBUTE_F32;
	if (str == "F32_2") return ZG_VERTEX_ATTRIBUTE_F32_2;
	if (str == "F32_3") return ZG_VERTEX_ATTRIBUTE_F32_3;
	if (str == "F32_4") return ZG_VERTEX_ATTRIBUTE_F32_4;

	if (str == "S32") return ZG_VERTEX_ATTRIBUTE_S32;
	if (str == "S32_2") return ZG_VERTEX_ATTRIBUTE_S32_2;
	if (str == "S32_3") return ZG_VERTEX_ATTRIBUTE_S32_3;
	if (str == "S32_4") return ZG_VERTEX_ATTRIBUTE_S32_4;

	if (str == "U32") return ZG_VERTEX_ATTRIBUTE_U32;
	if (str == "U32_2") return ZG_VERTEX_ATTRIBUTE_U32_2;
	if (str == "U32_3") return ZG_VERTEX_ATTRIBUTE_U32_3;
	if (str == "U32_4") return ZG_VERTEX_ATTRIBUTE_U32_4;

	sfz_assert(false);
	return ZG_VERTEX_ATTRIBUTE_UNDEFINED;
}

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
	GlobalConfig& cfg = getGlobalConfig();

	RendererConfigurableState& configurable = state.configurable;

	// Attempt to parse JSON file containing game common
	ParsedJson json = ParsedJson::parseFile(configPath, state.allocator);
	if (!json.isValid()) {
		SFZ_ERROR("NextGenRenderer", "Failed to load config at: %s", configPath);
		return false;
	}
	JsonNode root = json.root();

	// Ensure some necessary sections exist
	if (!root.accessMap("render_pipelines").isValid()) return false;

	// Store path to configuration
	configurable.configPath.clear();
	configurable.configPath.appendf("%s", configPath);


	// Render pipelines

	// Get number of render pipelines to load and allocate memory for them
	JsonNode renderPipelinesNode = root.accessMap("render_pipelines");
	uint32_t numRenderPipelines = renderPipelinesNode.arrayLength();
	configurable.renderPipelines.init(numRenderPipelines, state.allocator, sfz_dbg(""));

	// Parse information about each render pipeline
	for (uint32_t pipelineIdx = 0; pipelineIdx < numRenderPipelines; pipelineIdx++) {

		JsonNode pipelineNode = renderPipelinesNode.accessArray(pipelineIdx);
		configurable.renderPipelines.add(PipelineRenderItem());
		PipelineRenderItem& item = configurable.renderPipelines.last();

		str256 name = CHECK_JSON pipelineNode.accessMap("name").valueStr256();
		item.name = strID(name);

		item.vertexShaderPath = CHECK_JSON pipelineNode.accessMap("vertex_shader_path").valueStr256();
		item.pixelShaderPath = CHECK_JSON pipelineNode.accessMap("pixel_shader_path").valueStr256();

		item.vertexShaderEntry.clear();
		item.vertexShaderEntry.appendf("%s",
			(CHECK_JSON pipelineNode.accessMap("vertex_shader_entry").valueStr256()).str());
		item.pixelShaderEntry.clear();
		item.pixelShaderEntry.appendf("%s",
			(CHECK_JSON pipelineNode.accessMap("pixel_shader_entry").valueStr256()).str());

		// Input layout
		JsonNode inputLayoutNode = pipelineNode.accessMap("input_layout");
		{
			item.inputLayout.standardVertexLayout =
				CHECK_JSON inputLayoutNode.accessMap("standard_vertex_layout").valueBool();

			// If non-standard layout, parse it
			if (!item.inputLayout.standardVertexLayout) {

				item.inputLayout.vertexSizeBytes =
					uint32_t(CHECK_JSON inputLayoutNode.accessMap("vertex_size_bytes").valueInt());

				JsonNode attributesNode = inputLayoutNode.accessMap("attributes");
				const uint32_t numAttributes = attributesNode.arrayLength();
				for (uint32_t i = 0; i < numAttributes; i++) {
					JsonNode attribNode = attributesNode.accessArray(i);
					ZgVertexAttribute& attrib = item.inputLayout.attributes.add();
					attrib.location = uint32_t(CHECK_JSON attribNode.accessMap("location").valueInt());
					attrib.vertexBufferSlot = 0;
					attrib.type =
						attributeTypeFromString(CHECK_JSON attribNode.accessMap("type").valueStr256());
					attrib.offsetToFirstElementInBytes =
						uint32_t(CHECK_JSON attribNode.accessMap("offset_in_struct_bytes").valueInt());
				}
			}
		}

		// Push constants registers if specified
		JsonNode pushConstantsNode = pipelineNode.accessMap("push_constant_registers");
		if (pushConstantsNode.isValid()) {
			uint32_t numPushConstants = pushConstantsNode.arrayLength();
			for (uint32_t j = 0; j < numPushConstants; j++) {
				item.pushConstRegisters.add(
					(uint32_t)(CHECK_JSON pushConstantsNode.accessArray(j).valueInt()));
			}
		}
		
		// Constant buffers which are not user settable
		// I.e., constant buffers which should not have memory allocated for them
		JsonNode nonUserSettableCBsNode =
			pipelineNode.accessMap("non_user_settable_constant_buffers");
		if (nonUserSettableCBsNode.isValid()) {
			uint32_t numNonUserSettableConstantBuffers = nonUserSettableCBsNode.arrayLength();
			for (uint32_t j = 0; j < numNonUserSettableConstantBuffers; j++) {
				item.nonUserSettableConstBuffers.add(
					(uint32_t)(CHECK_JSON nonUserSettableCBsNode.accessArray(j).valueInt()));
			}
		}

		// Samplers
		JsonNode samplersNode = pipelineNode.accessMap("samplers");
		if (samplersNode.isValid()) {
			uint32_t numSamplers = samplersNode.arrayLength();
			for (uint32_t j = 0; j < numSamplers; j++) {
				JsonNode node = samplersNode.accessArray(j);
				SamplerItem& sampler = item.samplers.add();
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
		JsonNode renderTargetsNode = pipelineNode.accessMap("render_targets");
		sfz_assert(renderTargetsNode.isValid());
		uint32_t numRenderTargets = renderTargetsNode.arrayLength();
		for (uint32_t j = 0; j < numRenderTargets; j++) {
			item.renderTargets.add(
				textureFormatFromString(CHECK_JSON renderTargetsNode.accessArray(j).valueStr256()));
		}

		// Depth test and function if specified
		JsonNode depthFuncNode = pipelineNode.accessMap("depth_func");
		if (depthFuncNode.isValid()) {
			item.depthTest = true;
			item.depthFunc = depthFuncFromString(CHECK_JSON depthFuncNode.valueStr256());
		}

		// Culling
		JsonNode cullingNode = pipelineNode.accessMap("culling");
		if (cullingNode.isValid()) {
			item.cullingEnabled = true;
			item.cullFrontFacing = CHECK_JSON cullingNode.accessMap("cull_front_face").valueBool();
			item.frontFacingIsCounterClockwise =
				CHECK_JSON cullingNode.accessMap("front_facing_is_counter_clockwise").valueBool();
		}

		// Depth bias
		JsonNode depthBiasNode = pipelineNode.accessMap("depth_bias");
		item.depthBias = 0;
		item.depthBiasSlopeScaled = 0.0f;
		item.depthBiasClamp = 0.0f;
		if (depthBiasNode.isValid()) {
			item.depthBias = CHECK_JSON depthBiasNode.accessMap("bias").valueInt();
			item.depthBiasSlopeScaled = CHECK_JSON depthBiasNode.accessMap("bias_slope_scaled").valueFloat();
			item.depthBiasClamp = CHECK_JSON depthBiasNode.accessMap("bias_clamp").valueFloat();
		}

		// Wireframe rendering
		JsonNode wireframeNode = pipelineNode.accessMap("wireframe_rendering");
		if (wireframeNode.isValid()) {
			item.wireframeRenderingEnabled = CHECK_JSON wireframeNode.valueBool();
		}

		// Alpha blending
		JsonNode blendModeNode = pipelineNode.accessMap("blend_mode");
		item.blendMode = PipelineBlendMode::NO_BLENDING;
		if (blendModeNode.isValid()) {
			item.blendMode = blendModeFromString(CHECK_JSON blendModeNode.valueStr256());
		}
	}


	// Compute pipelines

	// Get number of compute pipelines to load and allocate memory for them
	JsonNode computePipelinesNode = root.accessMap("compute_pipelines");
	uint32_t numComputePipelines = computePipelinesNode.arrayLength();
	configurable.computePipelines.init(numComputePipelines, state.allocator, sfz_dbg(""));

	// Parse information about each compute pipeline
	for (uint32_t i = 0; i < numComputePipelines; i++) {
		
		JsonNode pipelineNode = computePipelinesNode.accessArray(i);
		configurable.computePipelines.add(PipelineComputeItem());
		PipelineComputeItem& item = configurable.computePipelines.last();

		str256 name = CHECK_JSON pipelineNode.accessMap("name").valueStr256();
		item.name = strID(name);

		item.computeShaderPath = CHECK_JSON pipelineNode.accessMap("compute_shader_path").valueStr256();
		item.computeShaderEntry.clear();
		item.computeShaderEntry.appendf("%s",
			(CHECK_JSON pipelineNode.accessMap("compute_shader_entry").valueStr256()).str());

		// Push constants registers if specified
		JsonNode pushConstantsNode = pipelineNode.accessMap("push_constant_registers");
		if (pushConstantsNode.isValid()) {
			uint32_t numPushConstants = pushConstantsNode.arrayLength();
			for (uint32_t j = 0; j < numPushConstants; j++) {
				item.pushConstRegisters.add(
					(uint32_t)(CHECK_JSON pushConstantsNode.accessArray(j).valueInt()));
			}
		}
		
		// Constant buffers which are not user settable
		// I.e., constant buffers which should not have memory allocated for them
		JsonNode nonUserSettableCBsNode =
			pipelineNode.accessMap("non_user_settable_constant_buffers");
		if (nonUserSettableCBsNode.isValid()) {
			uint32_t numNonUserSettableConstantBuffers = nonUserSettableCBsNode.arrayLength();
			for (uint32_t j = 0; j < numNonUserSettableConstantBuffers; j++) {
				item.nonUserSettableConstBuffers.add(
					(uint32_t)(CHECK_JSON nonUserSettableCBsNode.accessArray(j).valueInt()));
			}
		}

		// Samplers
		JsonNode samplersNode = pipelineNode.accessMap("samplers");
		if (samplersNode.isValid()) {
			uint32_t numSamplers = samplersNode.arrayLength();
			for (uint32_t j = 0; j < numSamplers; j++) {
				JsonNode node = samplersNode.accessArray(j);
				SamplerItem& sampler = item.samplers.add();
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
		JsonNode staticTexturesNode = root.accessMap("static_textures");
		uint32_t numStaticTextures = staticTexturesNode.arrayLength();
		configurable.staticTextures.init(numStaticTextures, state.allocator, sfz_dbg(""));

		// Parse information about each static texture
		for (uint32_t i = 0; i < numStaticTextures; i++) {
			
			JsonNode texNode = staticTexturesNode.accessArray(i);
			StaticTextureItem texItem = {};

			// Name
			str256 name = CHECK_JSON texNode.accessMap("name").valueStr256();
			sfz_assert(name != "default");
			texItem.name = strID(name);

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

			configurable.staticTextures.put(texItem.name, std::move(texItem));
		}
	}


	// Streaming buffers
	JsonNode streamingBuffersNode = root.accessMap("streaming_buffers");
	if (streamingBuffersNode.isValid()) {

		const uint32_t numStreamingBuffers = streamingBuffersNode.arrayLength();
		configurable.streamingBuffers.init(numStreamingBuffers * 2, state.allocator, sfz_dbg(""));
		for (uint32_t bufferIdx = 0; bufferIdx < numStreamingBuffers; bufferIdx++) {

			JsonNode bufferNode = streamingBuffersNode.accessArray(bufferIdx);
			StreamingBufferItem item;
			item.name = strID(CHECK_JSON bufferNode.accessMap("name").valueStr256());
			item.elementSizeBytes =
				uint32_t(CHECK_JSON bufferNode.accessMap("element_size_bytes").valueInt());
			item.maxNumElements =
				uint32_t(CHECK_JSON bufferNode.accessMap("max_num_elements").valueInt());
			item.committedAllocation = CHECK_JSON bufferNode.accessMap("committed_allocation").valueBool();
			configurable.streamingBuffers.put(item.name, std::move(item));
		}
	}


	// Present queue
	{
		strID defaultId = strID("default");

		JsonNode presentGroupsNode = root.accessMap("present_stage_groups");
		const uint32_t numGroups = presentGroupsNode.arrayLength();
		
		// Allocate memory for stage groups
		configurable.presentStageGroups.init(numGroups, state.allocator, sfz_dbg(""));

		// Parse stage groups
		for (uint32_t groupIdx = 0; groupIdx < numGroups; groupIdx++) {
			JsonNode groupNode = presentGroupsNode.accessArray(groupIdx);

			// Create group and read its name
			StageGroup& group = configurable.presentStageGroups.add();
			group.groupName = strID(CHECK_JSON groupNode.accessMap("group_name").valueStr256());

			// Get number of stages and allocate memory for them
			JsonNode stages = groupNode.accessMap("stages");
			const uint32_t numStages = stages.arrayLength();
			group.stages.init(numStages, state.allocator, sfz_dbg(""));

			// Stages
			for (uint32_t stageIdx = 0; stageIdx < numStages; stageIdx++) {
				JsonNode stageNode = stages.accessArray(stageIdx);
				Stage& stage = group.stages.add();

				str256 stageName = CHECK_JSON stageNode.accessMap("stage_name").valueStr256();
				stage.name = strID(stageName);

				str256 stageType = CHECK_JSON stageNode.accessMap("stage_type").valueStr256();
				if (stageType == "USER_INPUT_RENDERING") stage.type = StageType::USER_INPUT_RENDERING;
				else if (stageType == "USER_INPUT_COMPUTE") stage.type = StageType::USER_INPUT_COMPUTE;
				else return false;

				if (stage.type == StageType::USER_INPUT_RENDERING) {
					str256 renderPipelineName =
						CHECK_JSON stageNode.accessMap("render_pipeline").valueStr256();
					stage.render.pipelineName = strID(renderPipelineName);

					if (stageNode.accessMap("render_targets").isValid()) {
						JsonNode renderTargetsNode = stageNode.accessMap("render_targets");
						uint32_t numRenderTargets = renderTargetsNode.arrayLength();
						for (uint32_t i = 0; i < numRenderTargets; i++) {
							str256 renderTargetName =
								CHECK_JSON renderTargetsNode.accessArray(i).valueStr256();
							stage.render.renderTargetNames.add(strID(renderTargetName));
						}
					}

					if (stageNode.accessMap("depth_buffer").isValid()) {
						str256 depthBufferName =
							CHECK_JSON stageNode.accessMap("depth_buffer").valueStr256();
						stage.render.depthBufferName = strID(depthBufferName);
					}

					stage.render.defaultFramebuffer =
						stage.render.renderTargetNames.size() == 1 &&
						stage.render.renderTargetNames[0] == defaultId &&
						stage.render.depthBufferName == defaultId;
				}
				else if (stage.type == StageType::USER_INPUT_COMPUTE) {
					str256 computePipelineName =
						CHECK_JSON stageNode.accessMap("compute_pipeline").valueStr256();
					stage.compute.pipelineName = strID(computePipelineName);
				}
				else {
					sfz_assert(false);
				}

				// Bound textures
				if (stageNode.accessMap("bound_textures").isValid()) {
					JsonNode boundTexsNode = stageNode.accessMap("bound_textures");
					uint32_t numBoundTextures = boundTexsNode.arrayLength();

					for (uint32_t i = 0; i < numBoundTextures; i++) {
						JsonNode texNode = boundTexsNode.accessArray(i);
						BoundTexture boundTex;
						boundTex.textureRegister = CHECK_JSON texNode.accessMap("register").valueInt();
						str256 texName = CHECK_JSON texNode.accessMap("texture").valueStr256();
						boundTex.textureName = strID(texName);
						stage.boundTextures.add(boundTex);
					}
				}

				// Bound unordered textures
				if (stageNode.accessMap("bound_unordered_textures").isValid()) {
					JsonNode boundTexsNode = stageNode.accessMap("bound_unordered_textures");
					uint32_t numBoundTextures = boundTexsNode.arrayLength();

					for (uint32_t i = 0; i < numBoundTextures; i++) {
						JsonNode texNode = boundTexsNode.accessArray(i);
						BoundTexture boundTex;
						boundTex.textureRegister = CHECK_JSON texNode.accessMap("register").valueInt();
						str256 texName = CHECK_JSON texNode.accessMap("texture").valueStr256();
						boundTex.textureName = strID(texName);
						stage.boundUnorderedTextures.add(boundTex);
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
	for (auto pair : configurable.staticTextures) {
		StaticTextureItem& item = pair.value;
		item.buildTexture(state.windowRes);
	}

	// Create streaming buffers
	for (auto pair : configurable.streamingBuffers) {
		StreamingBufferItem& item = pair.value;
		item.buildBuffer(state.frameLatency);
	}

	// Allocate stage memory
	if (!allocateStageMemory(state)) success = false;

	return success;
}

bool allocateStageMemory(RendererState& state) noexcept
{
	bool success = true;

	for (uint32_t groupIdx = 0; groupIdx < state.configurable.presentStageGroups.size(); groupIdx++) {
		StageGroup& group = state.configurable.presentStageGroups[groupIdx];

		for (uint32_t stageIdx = 0; stageIdx < group.stages.size(); stageIdx++) {
			Stage& stage = group.stages[stageIdx];

			// Grab bindings and nonUserSettableConstBuffers from pipeline
			ZgPipelineBindingsSignature bindings = {};
			ArrayLocal<uint32_t, ZG_MAX_NUM_CONSTANT_BUFFERS>* nonUserSettableConstBuffers = nullptr;
			if (stage.type == StageType::USER_INPUT_RENDERING) {

				// Create framebuffer
				stage.rebuildFramebuffer(state.configurable.staticTextures);

				// Find pipeline
				PipelineRenderItem* pipelineItem =
					state.configurable.renderPipelines.find([&](const PipelineRenderItem& item) {
						return item.name == stage.render.pipelineName;
					});
				sfz_assert(pipelineItem != nullptr);

				// Grab stuff from pipeline item
				bindings = pipelineItem->pipeline.getSignature().bindings;
				nonUserSettableConstBuffers = &pipelineItem->nonUserSettableConstBuffers;
			}
			else if (stage.type == StageType::USER_INPUT_COMPUTE) {

				// Find pipeline
				PipelineComputeItem* pipelineItem =
					state.configurable.computePipelines.find([&](const PipelineComputeItem& item) {
						return item.name == stage.compute.pipelineName;
					});
				sfz_assert(pipelineItem != nullptr);

				// Grab stuff from pipeline item
				bindings = pipelineItem->pipeline.getBindingsSignature();
				nonUserSettableConstBuffers = &pipelineItem->nonUserSettableConstBuffers;
			}
			else {
				sfz_assert(false);
			}
			sfz_assert(nonUserSettableConstBuffers != nullptr);

			// Allocate CPU memory for constant buffer data
			uint32_t numConstantBuffers = bindings.numConstBuffers;
			stage.constantBuffers.init(numConstantBuffers, state.allocator, sfz_dbg(""));

			// Allocate GPU memory for all constant buffers
			for (uint32_t j = 0; j < numConstantBuffers; j++) {

				// Get constant buffer description, skip if push constant
				const ZgConstantBufferBindingDesc& desc = bindings.constBuffers[j];
				if (desc.pushConstant == ZG_TRUE) continue;

				// Check if constant buffer is marked as non-user-settable, in that case skip it
				bool nonUserSettable =
					nonUserSettableConstBuffers->findElement(desc.bufferRegister) != nullptr;
				if (nonUserSettable) continue;

				// Allocate container
				PerFrameData<ConstantBufferMemory>& framed = stage.constantBuffers.add();

				// Allocate ZeroG memory
				framed.init(state.frameLatency, [&](ConstantBufferMemory& item) {

					// Set shader register
					item.shaderRegister = desc.bufferRegister;

					// Allocate upload buffer
					CHECK_ZG item.uploadBuffer.create(desc.sizeInBytes, ZG_MEMORY_TYPE_UPLOAD);
					sfz_assert(item.uploadBuffer.valid());
					if (!item.uploadBuffer.valid()) success = false;

					// Allocate device buffer
					CHECK_ZG item.deviceBuffer.create(desc.sizeInBytes, ZG_MEMORY_TYPE_DEVICE);
					sfz_assert(item.deviceBuffer.valid());
					if (!item.deviceBuffer.valid()) success = false;
				});
			}
		}
	}

	return success;
}

} // namespace sfz
