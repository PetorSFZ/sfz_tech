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

#include "ph/renderer/RendererConfigParser.hpp"

#include <sfz/Logging.hpp>

#include "ph/Context.hpp"
#include "ph/renderer/RendererState.hpp"
#include "ph/renderer/ZeroGUtils.hpp"
#include "ph/util/JsonParser.hpp"

namespace ph {

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
	T operator% (const ph::ParsedJsonNodeValue<T>& valuePair) noexcept {
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
	configurable.configPath.printf("%s", configPath);

	// Parse framebuffers if section exist
	if (root.accessMap("framebuffers").isValid()) {

		// Get number of framebuffers and and allocate memory for them
		ParsedJsonNode framebuffersNode = root.accessMap("framebuffers");
		uint32_t numFramebuffers = framebuffersNode.arrayLength();
		configurable.framebuffers.init(numFramebuffers, state.allocator, sfz_dbg(""));

		// Parse information abotu each framebuffer
		for (uint32_t i = 0; i < numFramebuffers; i++) {

			ParsedJsonNode fbNode = framebuffersNode.accessArray(i);
			configurable.framebuffers.add(FramebufferItem());
			FramebufferItem& fbItem = configurable.framebuffers.last();

			str256 name = CHECK_JSON fbNode.accessMap("name").valueStr256();
			sfz_assert(name != "default");
			fbItem.name = resStrings.getStringID(name);

			// Resolution type
			fbItem.resolutionIsFixed = !(fbNode.accessMap("resolution_scale").isValid() ||
				fbNode.accessMap("resolution_scale_setting").isValid());

			// Resolution
			if (fbItem.resolutionIsFixed) {
				fbItem.resolutionFixed.x = CHECK_JSON fbNode.accessMap("resolution_fixed_width").valueInt();
				fbItem.resolutionFixed.y = CHECK_JSON fbNode.accessMap("resolution_fixed_height").valueInt();
			}
			else {
				bool hasSetting = fbNode.accessMap("resolution_scale_setting").isValid();
				if (hasSetting) {
					str256 settingKey = CHECK_JSON fbNode.accessMap("resolution_scale_setting").valueStr256();

					// Default value
					float defaultScale = 1.0f;
					if (fbNode.accessMap("resolution_scale").isValid()) {
						defaultScale = CHECK_JSON fbNode.accessMap("resolution_scale").valueFloat();
					}
					fbItem.resolutionScaleSetting =
						cfg.sanitizeFloat("Renderer", settingKey, false, defaultScale, 0.1f, 4.0f);
					fbItem.resolutionScale = fbItem.resolutionScaleSetting->floatValue();
				}
				else {
					fbItem.resolutionScaleSetting = nullptr;
					fbItem.resolutionScale = CHECK_JSON fbNode.accessMap("resolution_scale").valueFloat();
				}
			}

			// Render targets
			ParsedJsonNode renderTargetsNode = fbNode.accessMap("render_targets");
			if (renderTargetsNode.isValid()) {
				fbItem.numRenderTargets = renderTargetsNode.arrayLength();
				for (uint32_t j = 0; j < fbItem.numRenderTargets; j++) {
					ParsedJsonNode renderTarget = renderTargetsNode.accessArray(j);
					fbItem.renderTargetItems[j].format = textureFormatFromString(
						CHECK_JSON renderTarget.accessMap("format").valueStr256());
					float clearValue = CHECK_JSON renderTarget.accessMap("clear_value").valueFloat();
					sfz_assert(clearValue == 0.0f || clearValue == 1.0f);
					fbItem.renderTargetItems[j].clearValue = clearValue;
				}
			}
			else {
				fbItem.numRenderTargets = 0;
			}

			// Depth buffer
			if (fbNode.accessMap("depth_buffer").isValid()) {
				fbItem.hasDepthBuffer = CHECK_JSON fbNode.accessMap("depth_buffer").valueBool();
				if (fbItem.hasDepthBuffer) {
					fbItem.depthBufferFormat = textureFormatFromString(
						CHECK_JSON fbNode.accessMap("depth_buffer_format").valueStr256());
					float clearValue = CHECK_JSON fbNode.accessMap("depth_buffer_clear_value").valueFloat();
					sfz_assert(clearValue == 0.0f || clearValue == 1.0f);
					fbItem.depthBufferClearValue = clearValue;
				}
				
			}
		}
	}

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

		item.vertexShaderEntry.printf("%s",
			(CHECK_JSON pipelineNode.accessMap("vertex_shader_entry").valueStr256()).str());
		item.pixelShaderEntry.printf("%s",
			(CHECK_JSON pipelineNode.accessMap("pixel_shader_entry").valueStr256()).str());

		item.standardVertexAttributes =
			CHECK_JSON pipelineNode.accessMap("standard_vertex_attributes").valueBool();

		// Push constants registers if specified
		item.numPushConstants = 0;
		ParsedJsonNode pushConstantsNode = pipelineNode.accessMap("push_constant_registers");
		if (pushConstantsNode.isValid()) {
			item.numPushConstants = pushConstantsNode.arrayLength();
			for (uint32_t j = 0; j < item.numPushConstants; j++) {
				item.pushConstantRegisters[j] =
					(uint32_t)(CHECK_JSON pushConstantsNode.accessArray(j).valueInt());
			}
		}
		
		// Constant buffers which are not user settable
		// I.e., constant buffers which should not have memory allocated for them
		item.numNonUserSettableConstantBuffers = 0;
		ParsedJsonNode nonUserSettableCBsNode =
			pipelineNode.accessMap("non_user_settable_constant_buffers");
		if (nonUserSettableCBsNode.isValid()) {
			item.numNonUserSettableConstantBuffers = nonUserSettableCBsNode.arrayLength();
			for (uint32_t j = 0; j < item.numNonUserSettableConstantBuffers; j++) {
				item.nonUserSettableConstantBuffers[j] =
					(uint32_t)(CHECK_JSON nonUserSettableCBsNode.accessArray(j).valueInt());
			}
		}

		// Samplers
		ParsedJsonNode samplersNode = pipelineNode.accessMap("samplers");
		if (samplersNode.isValid()) {
			item.numSamplers = samplersNode.arrayLength();
			for (uint32_t j = 0; j < item.numSamplers; j++) {
				ParsedJsonNode node = samplersNode.accessArray(j);
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
		item.numRenderTargets = renderTargetsNode.arrayLength();
		for (uint32_t j = 0; j < item.numRenderTargets; j++) {
			item.renderTargets[j] =
				textureFormatFromString(CHECK_JSON renderTargetsNode.accessArray(j).valueStr256());
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

	// Get number of present queue stages to load and allocate memory for them
	ParsedJsonNode presentQueueStagesNode = root.accessMap("present_queue_stages");
	uint32_t numPresentQueueStages = presentQueueStagesNode.arrayLength();
	configurable.presentQueueStages.init(numPresentQueueStages, state.allocator, sfz_dbg(""));

	// Parse information about present queue stage
	for (uint32_t i = 0; i < numPresentQueueStages; i++) {

		ParsedJsonNode stageNode = presentQueueStagesNode.accessArray(i);
		configurable.presentQueueStages.add(Stage());
		Stage& stage = configurable.presentQueueStages.last();

		str256 stageName = CHECK_JSON stageNode.accessMap("stage_name").valueStr256();
		stage.stageName = resStrings.getStringID(stageName);

		str256 stageType = CHECK_JSON stageNode.accessMap("stage_type").valueStr256();
		if (stageType == "USER_INPUT_RENDERING") stage.stageType = StageType::USER_INPUT_RENDERING;
		else if (stageType == "USER_STAGE_BARRIER") stage.stageType = StageType::USER_STAGE_BARRIER;
		else return false;

		if (stage.stageType == StageType::USER_INPUT_RENDERING) {
			str256 renderPipelineName =
				CHECK_JSON stageNode.accessMap("render_pipeline").valueStr256();
			stage.renderPipelineName = resStrings.getStringID(renderPipelineName);

			str256 framebufferName =
				CHECK_JSON stageNode.accessMap("framebuffer").valueStr256();
			stage.framebufferName = resStrings.getStringID(framebufferName);
		}

		// Bound render targets
		if (stageNode.accessMap("bound_render_targets").isValid()) {
			ParsedJsonNode boundTargetsNode = stageNode.accessMap("bound_render_targets");
			uint32_t numBoundTargets = boundTargetsNode.arrayLength();

			stage.boundRenderTargets.init(numBoundTargets, state.allocator, sfz_dbg(""));
			for (uint32_t j = 0; j < numBoundTargets; j++) {

				ParsedJsonNode targetNode = boundTargetsNode.accessArray(j);
				BoundRenderTarget boundTarget;
				boundTarget.textureRegister = CHECK_JSON targetNode.accessMap("register").valueInt();
				str256 framebufferName = CHECK_JSON targetNode.accessMap("framebuffer").valueStr256();
				sfz_assert(framebufferName != "debug"); // Can't bind default framebuffer
				boundTarget.framebuffer = resStrings.getStringID(framebufferName);

				// Check if depth buffer should be bound
				if (targetNode.accessMap("depth_buffer").isValid()) {
					sfz_assert(CHECK_JSON targetNode.accessMap("depth_buffer").valueBool());
					boundTarget.depthBuffer = true;
					boundTarget.renderTargetIdx = ~0u;
				}
				else {
					boundTarget.depthBuffer = false;
					boundTarget.renderTargetIdx =
						CHECK_JSON targetNode.accessMap("render_target_index").valueInt();
				}

				stage.boundRenderTargets.add(boundTarget);
			}
		}
	}

	// Create framebuffers
	bool success = true;
	for (FramebufferItem& item : configurable.framebuffers) {
		if (!item.buildFramebuffer(state.windowRes, state.gpuAllocatorFramebuffer)) {
			success = false;
		}
	}

	// Builds pipelines
	for (PipelineRenderItem& item : configurable.renderPipelines) {
		if (!item.buildPipeline()) {
			success = false;
		}
	}

	// Allocate stage memory
	if (!allocateStageMemory(state)) success = false;

	return success;
}

bool allocateStageMemory(RendererState& state) noexcept
{
	bool success = true;

	for (uint32_t i = 0; i < state.configurable.presentQueueStages.size(); i++) {
		ph::Stage& stage = state.configurable.presentQueueStages[i];
		if (stage.stageType != StageType::USER_INPUT_RENDERING) continue;
		
		// Find pipeline
		PipelineRenderItem* pipelineItem =
			state.configurable.renderPipelines.find([&](const PipelineRenderItem& item) {
			return item.name == stage.renderPipelineName;
		});
		sfz_assert(pipelineItem != nullptr);
		
		// Allocate CPU memory for constant buffer data
		uint32_t numConstantBuffers = pipelineItem->pipeline.signature.numConstantBuffers;
		stage.constantBuffers.init(numConstantBuffers, state.allocator, sfz_dbg(""));

		// Allocate GPU memory for all constant buffers
		for (uint32_t j = 0; j < numConstantBuffers; j++) {
			
			// Get constant buffer description, skip if push constant
			const ZgConstantBufferDesc& desc = pipelineItem->pipeline.signature.constantBuffers[j];
			if (desc.pushConstant == ZG_TRUE) continue;

			// Check if constant buffer is marked as non-user-settable, in that case skip it
			bool nonUserSettable = false;
			for (uint32_t k = 0; k < pipelineItem->numNonUserSettableConstantBuffers; k++) {
				if (pipelineItem->nonUserSettableConstantBuffers[k] == desc.shaderRegister) {
					nonUserSettable = true;
					break;
				}
			}
			if (nonUserSettable) continue;

			// Allocate container
			stage.constantBuffers.add({});
			Framed<ConstantBufferMemory>& framed = stage.constantBuffers.last();

			// Allocate ZeroG memory
			framed.initAllStates([&](ConstantBufferMemory& item) {

				// Seet 
				item.shaderRegister = desc.shaderRegister;
				
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

			// Initialize fences
			CHECK_ZG framed.initAllFences();
		}
	}

	return success;
}

bool deallocateStageMemory(RendererState& state) noexcept
{
	for (ph::Stage& stage : state.configurable.presentQueueStages) {
		for (Framed<ConstantBufferMemory>& framed : stage.constantBuffers) {
			framed.deinitAllStates([&](ConstantBufferMemory& item) {

				// Deallocate upload buffer
				sfz_assert(item.uploadBuffer.valid());
				state.gpuAllocatorUpload.deallocate(item.uploadBuffer);
				sfz_assert(!item.uploadBuffer.valid());

				// Deallocate device buffer
				sfz_assert(item.deviceBuffer.valid());
				state.gpuAllocatorDevice.deallocate(item.deviceBuffer);
				sfz_assert(!item.deviceBuffer.valid());
			});
			
			// Release fences
			framed.releaseAllFences();
		}

		stage.constantBuffers.destroy();
	}

	return true;
}

} // namespace ph
