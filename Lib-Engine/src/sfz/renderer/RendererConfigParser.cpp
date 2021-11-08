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
#include "sfz/shaders/ShaderManager.hpp"
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
			SFZ_ERROR("Renderer", "Key did not exist in JSON file: %s:%i", file, line);
			sfz_assert(false);
		}
		return valuePair.value;
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

static ZgComparisonFunc comparisonFuncFromString(const str256& str) noexcept
{
	if (str == "NONE") return ZG_COMPARISON_FUNC_NONE;
	if (str == "LESS") return ZG_COMPARISON_FUNC_LESS;
	if (str == "LESS_EQUAL") return ZG_COMPARISON_FUNC_LESS_EQUAL;
	if (str == "EQUAL") return ZG_COMPARISON_FUNC_EQUAL;
	if (str == "NOT_EQUAL") return ZG_COMPARISON_FUNC_NOT_EQUAL;
	if (str == "GREATER") return ZG_COMPARISON_FUNC_GREATER;
	if (str == "GREATER_EQUAL") return ZG_COMPARISON_FUNC_GREATER_EQUAL;
	sfz_assert(false);
	return ZG_COMPARISON_FUNC_NONE;
}

static ZgFormat textureFormatFromString(const str256& str) noexcept
{
	if (str == "R_U8_UNORM") return ZG_FORMAT_R_U8_UNORM;
	if (str == "RG_U8_UNORM") return ZG_FORMAT_RG_U8_UNORM;
	if (str == "RGBA_U8_UNORM") return ZG_FORMAT_RGBA_U8_UNORM;

	if (str == "R_U8") return ZG_FORMAT_R_U8;
	if (str == "RG_U8") return ZG_FORMAT_RG_U8;
	if (str == "RGBA_U8") return ZG_FORMAT_RGBA_U8;

	if (str == "R_F16") return ZG_FORMAT_R_F16;
	if (str == "RG_F16") return ZG_FORMAT_RG_F16;
	if (str == "RGBA_F16") return ZG_FORMAT_RGBA_F16;

	if (str == "R_F32") return ZG_FORMAT_R_F32;
	if (str == "RG_F32") return ZG_FORMAT_RG_F32;
	if (str == "RGBA_F32") return ZG_FORMAT_RGBA_F32;

	if (str == "DEPTH_F32") return ZG_FORMAT_DEPTH_F32;

	sfz_assert(false);
	return ZG_FORMAT_UNDEFINED;
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
	state.configPath.clear();
	state.configPath.appendf("%s", configPath);

	ShaderManager& shaders = getShaderManager();

	// Render pipelines
	JsonNode renderPipelinesNode = root.accessMap("render_pipelines");
	u32 numRenderPipelines = renderPipelinesNode.arrayLength();
	for (u32 pipelineIdx = 0; pipelineIdx < numRenderPipelines; pipelineIdx++) {

		JsonNode pipelineNode = renderPipelinesNode.accessArray(pipelineIdx);
		Shader shader = {};
		shader.type = ShaderType::RENDER;

		str256 name = CHECK_JSON pipelineNode.accessMap("name").valueStr256();
		shader.name = sfzStrIDCreate(name);

		shader.shaderPath =
			CHECK_JSON pipelineNode.accessMap("path").valueStr256();

		shader.render.vertexShaderEntry.clear();
		shader.render.vertexShaderEntry.appendf("%s",
			(CHECK_JSON pipelineNode.accessMap("vertex_shader_entry").valueStr256()).str());
		shader.render.pixelShaderEntry.clear();
		shader.render.pixelShaderEntry.appendf("%s",
			(CHECK_JSON pipelineNode.accessMap("pixel_shader_entry").valueStr256()).str());

		// Input layout
		JsonNode inputLayoutNode = pipelineNode.accessMap("input_layout");
		{
			shader.render.inputLayout.standardVertexLayout =
				CHECK_JSON inputLayoutNode.accessMap("standard_vertex_layout").valueBool();

			// If non-standard layout, parse it
			if (!shader.render.inputLayout.standardVertexLayout) {

				shader.render.inputLayout.vertexSizeBytes =
					u32(CHECK_JSON inputLayoutNode.accessMap("vertex_size_bytes").valueInt());

				JsonNode attributesNode = inputLayoutNode.accessMap("attributes");
				const u32 numAttributes = attributesNode.arrayLength();
				for (u32 i = 0; i < numAttributes; i++) {
					JsonNode attribNode = attributesNode.accessArray(i);
					ZgVertexAttribute& attrib = shader.render.inputLayout.attributes.add();
					attrib.location = u32(CHECK_JSON attribNode.accessMap("location").valueInt());
					attrib.vertexBufferSlot = 0;
					attrib.type =
						attributeTypeFromString(CHECK_JSON attribNode.accessMap("type").valueStr256());
					attrib.offsetToFirstElementInBytes =
						u32(CHECK_JSON attribNode.accessMap("offset_in_struct_bytes").valueInt());
				}
			}
		}

		// Push constants registers if specified
		JsonNode pushConstantsNode = pipelineNode.accessMap("push_constant_registers");
		if (pushConstantsNode.isValid()) {
			u32 numPushConstants = pushConstantsNode.arrayLength();
			for (u32 j = 0; j < numPushConstants; j++) {
				shader.pushConstRegisters.add(
					(u32)(CHECK_JSON pushConstantsNode.accessArray(j).valueInt()));
			}
		}

		// Samplers
		JsonNode samplersNode = pipelineNode.accessMap("samplers");
		if (samplersNode.isValid()) {
			u32 numSamplers = samplersNode.arrayLength();
			for (u32 j = 0; j < numSamplers; j++) {
				JsonNode node = samplersNode.accessArray(j);
				SamplerItem& sampler = shader.samplers.add();
				sampler.samplerRegister = CHECK_JSON node.accessMap("register").valueInt();
				sampler.sampler.samplingMode = samplingModeFromString(
					CHECK_JSON node.accessMap("sampling_mode").valueStr256());
				sampler.sampler.wrappingModeU = wrappingModeFromString(
					CHECK_JSON node.accessMap("wrapping_mode").valueStr256());
				sampler.sampler.wrappingModeV = sampler.sampler.wrappingModeU;
				sampler.sampler.mipLodBias = 0.0f;
				if (node.accessMap("comparison_func").isValid()) {
					sampler.sampler.comparisonFunc = comparisonFuncFromString(
						CHECK_JSON node.accessMap("comparison_func").valueStr256());
				}
			}
		}

		// Render targets
		JsonNode renderTargetsNode = pipelineNode.accessMap("render_targets");
		sfz_assert(renderTargetsNode.isValid());
		u32 numRenderTargets = renderTargetsNode.arrayLength();
		for (u32 j = 0; j < numRenderTargets; j++) {
			shader.render.renderTargets.add(
				textureFormatFromString(CHECK_JSON renderTargetsNode.accessArray(j).valueStr256()));
		}

		// Depth test and function if specified
		JsonNode depthFuncNode = pipelineNode.accessMap("depth_func");
		if (depthFuncNode.isValid()) {
			shader.render.depthFunc = comparisonFuncFromString(CHECK_JSON depthFuncNode.valueStr256());
		}

		// Culling
		JsonNode cullingNode = pipelineNode.accessMap("culling");
		if (cullingNode.isValid()) {
			shader.render.cullingEnabled = true;
			shader.render.cullFrontFacing = CHECK_JSON cullingNode.accessMap("cull_front_face").valueBool();
			shader.render.frontFacingIsCounterClockwise =
				CHECK_JSON cullingNode.accessMap("front_facing_is_counter_clockwise").valueBool();
		}
		else {
			shader.render.frontFacingIsCounterClockwise = true;
		}

		// Depth bias
		JsonNode depthBiasNode = pipelineNode.accessMap("depth_bias");
		shader.render.depthBias = 0;
		shader.render.depthBiasSlopeScaled = 0.0f;
		shader.render.depthBiasClamp = 0.0f;
		if (depthBiasNode.isValid()) {
			shader.render.depthBias = CHECK_JSON depthBiasNode.accessMap("bias").valueInt();
			shader.render.depthBiasSlopeScaled = CHECK_JSON depthBiasNode.accessMap("bias_slope_scaled").valueFloat();
			shader.render.depthBiasClamp = CHECK_JSON depthBiasNode.accessMap("bias_clamp").valueFloat();
		}

		// Wireframe rendering
		JsonNode wireframeNode = pipelineNode.accessMap("wireframe_rendering");
		if (wireframeNode.isValid()) {
			shader.render.wireframeRenderingEnabled = CHECK_JSON wireframeNode.valueBool();
		}

		// Alpha blending
		JsonNode blendModeNode = pipelineNode.accessMap("blend_mode");
		shader.render.blendMode = PipelineBlendMode::NO_BLENDING;
		if (blendModeNode.isValid()) {
			shader.render.blendMode = blendModeFromString(CHECK_JSON blendModeNode.valueStr256());
		}

		bool buildSuccess = shader.build();
		sfz_assert(buildSuccess);
		shaders.addShader(sfz_move(shader));
	}


	// Compute pipelines
	JsonNode computePipelinesNode = root.accessMap("compute_pipelines");
	u32 numComputePipelines = computePipelinesNode.arrayLength();
	for (u32 i = 0; i < numComputePipelines; i++) {
		
		JsonNode pipelineNode = computePipelinesNode.accessArray(i);
		Shader shader = {};
		shader.type = ShaderType::COMPUTE;

		str256 name = CHECK_JSON pipelineNode.accessMap("name").valueStr256();
		shader.name = sfzStrIDCreate(name);

		shader.shaderPath =
			CHECK_JSON pipelineNode.accessMap("path").valueStr256();

		shader.compute.computeShaderEntry.clear();
		shader.compute.computeShaderEntry.appendf("%s",
			(CHECK_JSON pipelineNode.accessMap("compute_shader_entry").valueStr256()).str());

		// Push constants registers if specified
		JsonNode pushConstantsNode = pipelineNode.accessMap("push_constant_registers");
		if (pushConstantsNode.isValid()) {
			u32 numPushConstants = pushConstantsNode.arrayLength();
			for (u32 j = 0; j < numPushConstants; j++) {
				shader.pushConstRegisters.add(
					(u32)(CHECK_JSON pushConstantsNode.accessArray(j).valueInt()));
			}
		}

		// Samplers
		JsonNode samplersNode = pipelineNode.accessMap("samplers");
		if (samplersNode.isValid()) {
			u32 numSamplers = samplersNode.arrayLength();
			for (u32 j = 0; j < numSamplers; j++) {
				JsonNode node = samplersNode.accessArray(j);
				SamplerItem& sampler = shader.samplers.add();
				sampler.samplerRegister = CHECK_JSON node.accessMap("register").valueInt();
				sampler.sampler.samplingMode = samplingModeFromString(
					CHECK_JSON node.accessMap("sampling_mode").valueStr256());
				sampler.sampler.wrappingModeU = wrappingModeFromString(
					CHECK_JSON node.accessMap("wrapping_mode").valueStr256());
				sampler.sampler.wrappingModeV = sampler.sampler.wrappingModeU;
				sampler.sampler.mipLodBias = 0.0f;
				if (node.accessMap("comparison_func").isValid()) {
					sampler.sampler.comparisonFunc = comparisonFuncFromString(
						CHECK_JSON node.accessMap("comparison_func").valueStr256());
				}
			}
		}

		bool buildSuccess = shader.build();
		sfz_assert(buildSuccess);
		shaders.addShader(sfz_move(shader));
	}

	return true;
}

} // namespace sfz
