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
			sfz_assert_debug(false);
		}
		return std::move(valuePair.value);
	}
};

static ZgSamplingMode samplingModeFromString(const str256& str) noexcept
{
	if (str == "NEAREST") return ZG_SAMPLING_MODE_NEAREST;
	if (str == "TRILINEAR") return ZG_SAMPLING_MODE_TRILINEAR;
	if (str == "ANISOTROPIC") return ZG_SAMPLING_MODE_ANISOTROPIC;
	sfz_assert_debug(false);
	return ZG_SAMPLING_MODE_UNDEFINED;
}

static ZgWrappingMode wrappingModeFromString(const str256& str) noexcept
{
	if (str == "CLAMP") return ZG_WRAPPING_MODE_CLAMP;
	if (str == "REPEAT") return ZG_WRAPPING_MODE_REPEAT;
	sfz_assert_debug(false);
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
	sfz_assert_debug(false);
	return ZG_DEPTH_FUNC_LESS;
}

// Renderer config parser functions
// ------------------------------------------------------------------------------------------------

bool parseRendererConfig(RendererState& state, const char* configPath) noexcept
{
	RendererConfigurableState& configurable = state.configurable;

	// Attempt to parse JSON file containing game common
	ParsedJson json = ParsedJson::parseFile(configPath, state.allocator);
	if (!json.isValid()) {
		SFZ_ERROR("NextGenRenderer", "Failed to load config at: %s", configPath);
		return false;
	}
	ParsedJsonNode root = json.root();

	// Get global collection of resource strings in order to create StringIDs
	sfz::StringCollection& resStrings = getResourceStrings();

	// Ensure some necessary sections exist
	if (!root.accessMap("rendering_pipelines").isValid()) return false;

	// Get number of rendering pipelines to load and allocate memory for them
	ParsedJsonNode renderingPipelinesNode = root.accessMap("rendering_pipelines");
	uint32_t numRenderingPipelines = renderingPipelinesNode.arrayLength();
	configurable.renderingPipelines.create(numRenderingPipelines, state.allocator);

	// Parse information about each rendering pipeline
	for (uint32_t i = 0; i < numRenderingPipelines; i++) {

		ParsedJsonNode pipelineNode = renderingPipelinesNode.accessArray(i);
		configurable.renderingPipelines.add(PipelineRenderingItem());
		PipelineRenderingItem& item = configurable.renderingPipelines.last();

		str256 name = CHECK_JSON pipelineNode.accessMap("name").valueStr256();
		item.name = resStrings.getStringID(name);

		str256 sourceTypeStr = CHECK_JSON pipelineNode.accessMap("source_type").valueStr256();
		item.sourceType = [&]() {
			if (sourceTypeStr == "spirv") return PipelineSourceType::SPIRV;
			if (sourceTypeStr == "hlsl") return PipelineSourceType::HLSL;
			sfz_assert_release(false);
			return PipelineSourceType::SPIRV;
		}();

		item.vertexShaderPath = CHECK_JSON pipelineNode.accessMap("vertex_shader_path").valueStr256();
		item.pixelShaderPath = CHECK_JSON pipelineNode.accessMap("pixel_shader_path").valueStr256();

		item.vertexShaderEntry.printf("%s",
			(CHECK_JSON pipelineNode.accessMap("vertex_shader_entry").valueStr256()).str);
		item.pixelShaderEntry.printf("%s",
			(CHECK_JSON pipelineNode.accessMap("pixel_shader_entry").valueStr256()).str);

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
	}

	// Get number of present queue stages to load and allocate memory for them
	ParsedJsonNode presentQueueStagesNode = root.accessMap("present_queue_stages");
	uint32_t numPresentQueueStages = presentQueueStagesNode.arrayLength();
	configurable.presentQueueStages.create(numPresentQueueStages, state.allocator);

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
			str256 renderingPipelineName =
				CHECK_JSON stageNode.accessMap("rendering_pipeline").valueStr256();
			stage.renderingPipelineName = resStrings.getStringID(renderingPipelineName);
		}
	}

	// Builds pipelines
	bool success = true;
	for (PipelineRenderingItem& item : configurable.renderingPipelines) {
		if (!buildPipelineRendering(item)) {
			success = false;
		}
	}

	// Allocate stage memory
	if (!allocateStageMemory(state)) success = false;

	return success;
}

bool buildPipelineRendering(PipelineRenderingItem& item) noexcept
{
	// Create pipeline builder
	zg::PipelineRenderingBuilder pipelineBuilder;
	pipelineBuilder
		.addVertexShaderPath(item.vertexShaderEntry, item.vertexShaderPath)
		.addPixelShaderPath(item.pixelShaderEntry, item.pixelShaderPath);
	
	// Set vertex attributes
	if (item.standardVertexAttributes) {
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
	sfz_assert_debug(item.numPushConstants < ZG_MAX_NUM_CONSTANT_BUFFERS);
	for (uint32_t i = 0; i < item.numPushConstants; i++) {
		pipelineBuilder.addPushConstant(item.pushConstantRegisters[i]);
	}

	// Samplers
	sfz_assert_debug(item.numSamplers < ZG_MAX_NUM_SAMPLERS);
	for (uint32_t i = 0; i < item.numSamplers; i++) {
		SamplerItem& sampler = item.samplers[i];
		pipelineBuilder.addSampler(sampler.samplerRegister, sampler.sampler);
	}

	// Depth test
	if (item.depthTest) {
		pipelineBuilder
			.setDepthTestEnabled(true)
			.setDepthFunc(item.depthFunc);
	}

	// Culling
	if (item.cullingEnabled) {
		pipelineBuilder
			.setCullingEnabled(true)
			.setCullMode(item.cullFrontFacing, item.frontFacingIsCounterClockwise);
	}

	// Build pipeline
	bool buildSuccess = false;
	if (item.sourceType == PipelineSourceType::SPIRV) {
		buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileSPIRV(item.pipeline);
	}
	else {
		buildSuccess = CHECK_ZG pipelineBuilder.buildFromFileHLSL(item.pipeline);
	}

	return buildSuccess;
}

bool allocateStageMemory(RendererState& state) noexcept
{
	bool success = true;

	for (uint32_t i = 0; i < state.configurable.presentQueueStages.size(); i++) {
		ph::Stage& stage = state.configurable.presentQueueStages[i];
		if (stage.stageType != StageType::USER_INPUT_RENDERING) continue;
		
		// Find pipeline
		PipelineRenderingItem* pipelineItem =
			state.configurable.renderingPipelines.find([&](const PipelineRenderingItem& item) {
			return item.name == stage.renderingPipelineName;
		});
		sfz_assert_debug(pipelineItem != nullptr);
		
		// Allocate CPU memory for constant buffer data
		uint32_t numConstantBuffers = pipelineItem->pipeline.signature.numConstantBuffers;
		stage.constantBuffers.create(numConstantBuffers, state.allocator);

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
					state.dynamicAllocator.allocateBuffer(ZG_MEMORY_TYPE_UPLOAD, desc.sizeInBytes);
				sfz_assert_debug(item.uploadBuffer.valid());
				if (!item.uploadBuffer.valid()) success = false;

				// Allocate device buffer
				item.deviceBuffer =
					state.dynamicAllocator.allocateBuffer(ZG_MEMORY_TYPE_DEVICE, desc.sizeInBytes);
				sfz_assert_debug(item.deviceBuffer.valid());
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
				sfz_assert_debug(item.uploadBuffer.valid());
				state.dynamicAllocator.deallocate(item.uploadBuffer);
				sfz_assert_debug(!item.uploadBuffer.valid());

				// Deallocate device buffer
				sfz_assert_debug(item.deviceBuffer.valid());
				state.dynamicAllocator.deallocate(item.deviceBuffer);
				sfz_assert_debug(!item.deviceBuffer.valid());
			});
			
			// Release fences
			framed.releaseAllFences();
		}

		stage.constantBuffers.destroy();
	}

	return true;
}

} // namespace ph
