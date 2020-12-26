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

#include "sfz/renderer/RendererUI.hpp"

#include <utility> // std::swap()

#include <imgui.h>

#include "sfz/Context.hpp"
#include "sfz/Logging.hpp"
#include "sfz/renderer/RendererState.hpp"
#include "sfz/renderer/RenderingEnumsToFromString.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/util/ImGuiHelpers.hpp"

namespace sfz {

using sfz::str64;

// Statics
// ------------------------------------------------------------------------------------------------

static float toGiB(uint64_t bytes) noexcept
{
	return float(bytes) / (1024.0f * 1024.0f * 1024.0f);
}

static const char* toString(StageType type) noexcept
{
	switch (type) {
	case StageType::USER_INPUT_RENDERING: return "USER_INPUT_RENDERING";
	case StageType::USER_INPUT_COMPUTE: return "USER_INPUT_COMPUTE";
	}
	sfz_assert(false);
	return "<ERROR>";
}

// RendererUI: State methods
// ------------------------------------------------------------------------------------------------

void RendererUI::swap(RendererUI& other) noexcept
{
	(void)other;
}

void RendererUI::destroy() noexcept
{

}

// RendererUI: Methods
// ------------------------------------------------------------------------------------------------

void RendererUI::render(RendererState& state) noexcept
{
	ImGuiWindowFlags windowFlags = 0;
	windowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
	if (!ImGui::Begin("Renderer", nullptr, windowFlags)) {
		ImGui::End();
		return;
	}

	// Tabs
	ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("RendererTabBar", tabBarFlags)) {
		
		if (ImGui::BeginTabItem("General")) {
			ImGui::Spacing();
			this->renderGeneralTab(state);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Present Stage Groups")) {
			ImGui::Spacing();
			this->renderPresentStageGroupsTab(state.configurable);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Pipelines")) {
			ImGui::Spacing();
			this->renderPipelinesTab(state);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Static Memory")) {
			ImGui::Spacing();
			this->renderStaticMemoryTab(state.configurable);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Streaming Buffers")) {
			ImGui::Spacing();
			this->renderStreamingBuffersTab(state.configurable);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Meshes")) {
			ImGui::Spacing();
			this->renderMeshesTab(state);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

// RendererUI: Private methods
// --------------------------------------------------------------------------------------------

void RendererUI::renderGeneralTab(RendererState& state) noexcept
{
	constexpr float offset = 250.0f;
	alignedEdit("Config path", offset, [&](const char*) {
		ImGui::Text("\"%s\"", state.configurable.configPath.str());
	});
	alignedEdit("Current frame index", offset, [&](const char*) {
		ImGui::Text("%llu", state.currentFrameIdx);
	});
	alignedEdit("Window resolution", offset, [&](const char*) {
		ImGui::Text("%i x %i", state.windowRes.x, state.windowRes.y);
	});

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();


	// Get ZeroG stats
	ZgStats stats = {};
	CHECK_ZG zgContextGetStats(&stats);

	// Print ZeroG statistics
	ImGui::Text("ZeroG Stats");
	ImGui::Spacing();
	ImGui::Indent(20.0f);

	constexpr float statsValueOffset = 240.0f;
	alignedEdit("Device", statsValueOffset, [&](const char*) {
		ImGui::TextUnformatted(stats.deviceDescription);
	});
	ImGui::Spacing();
	alignedEdit("Dedicated GPU Memory", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.dedicatedGpuMemoryBytes));
	});
	alignedEdit("Dedicated CPU Memory", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.dedicatedCpuMemoryBytes));
	});
	alignedEdit("Shared GPU Memory", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.sharedCpuMemoryBytes));
	});
	ImGui::Spacing();
	alignedEdit("Memory Budget", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.memoryBudgetBytes));
	});
	alignedEdit("Current Memory Usage", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.memoryUsageBytes));
	});
	ImGui::Spacing();
	alignedEdit("Non-Local Budget", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.nonLocalBugetBytes));
	});
	alignedEdit("Non-Local Usage", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.nonLocalUsageBytes));
	});

	ImGui::Unindent(20.0f);
}

void RendererUI::renderPresentStageGroupsTab(RendererConfigurableState& state) noexcept
{
	for (uint32_t groupIdx = 0; groupIdx < state.presentStageGroups.size(); groupIdx++) {
		const StageGroup& group = state.presentStageGroups[groupIdx];
		const char* groupName = group.groupName.str();

		// Collapsing header with group name
		const bool collapsingHeaderOpen =
			ImGui::CollapsingHeader(str256("Stage Group %u - \"%s\"", groupIdx, groupName));
		if (!collapsingHeaderOpen) continue;

		ImGui::Indent(20.0f);
		for (uint32_t stageIdx = 0; stageIdx < group.stages.size(); stageIdx++) {
			const Stage& stage = group.stages[stageIdx];

			// Stage name
			ImGui::Text("Stage %u - \"%s\"", stageIdx, stage.name.str());
			ImGui::Indent(20.0f);

			// Stage type
			ImGui::Text("Type: %s", toString(stage.type));

			if (stage.type == StageType::USER_INPUT_RENDERING) {

				// Pipeline name
				ImGui::Text("Render Pipeline: \"%s\"", stage.render.pipelineName.str());

				if (stage.render.defaultFramebuffer) {
					ImGui::Text("Framebuffer: \"default\"");
				}
				else {
					// Render targets
					if (!stage.render.renderTargetNames.isEmpty()) {
						ImGui::Text("Render targets:");
						ImGui::Indent(20.0f);
						for (uint32_t i = 0; i < stage.render.renderTargetNames.size(); i++) {
							strID renderTarget = stage.render.renderTargetNames[i];
							ImGui::Text("- Render target %u: \"%s\"",
								i, renderTarget.str());
						}
						ImGui::Unindent(20.0f);
					}

					// Depth buffer
					if (stage.render.depthBufferName.isValid()) {
						ImGui::Text("Depth buffer: \"%s\"",
							stage.render.depthBufferName.str());
					}
				}
			}
			else if (stage.type == StageType::USER_INPUT_COMPUTE) {
				
				// Pipeline name
				ImGui::Text("Compute Pipeline: \"%s\"", stage.compute.pipelineName.str());
			}
			else {
				sfz_assert(false);
			}

			// Bound textures
			if (!stage.boundTextures.isEmpty()) {
				ImGui::Text("Bound textures:");
				ImGui::Indent(20.0f);
				for (const BoundTexture& boundTex : stage.boundTextures) {
					ImGui::Text("- Register: %u  --  Texture: \"%s\"",
						boundTex.textureRegister,
						boundTex.textureName.str());
				}
				ImGui::Unindent(20.0f);
			}

			// Bound unordered textures
			if (!stage.boundUnorderedTextures.isEmpty()) {
				ImGui::Text("Bound unordered textures:");
				ImGui::Indent(20.0f);
				for (const BoundTexture& boundTex : stage.boundUnorderedTextures) {
					ImGui::Text("- Register: %u  --  Texture: \"%s\"",
						boundTex.textureRegister,
						boundTex.textureName.str());
				}
				ImGui::Unindent(20.0f);
			}

			ImGui::Unindent(20.0f);
			ImGui::Spacing();
		}
		ImGui::Unindent(20.0f);
	}
}

void RendererUI::renderPipelinesTab(RendererState& state) noexcept
{
	RendererConfigurableState& configurable = state.configurable;

	// Render pipelines
	ImGui::Text("Render Pipelines");

	// Reload all button
	ImGui::SameLine(ImGui::GetWindowWidth() - 130.0f);
	if (ImGui::Button("Reload All##__render_pipelines", vec2(120.0f, 0.0f))) {

		SFZ_INFO("Renderer", "Reloading all render pipelines...");

		// Flush ZeroG queues
		CHECK_ZG state.presentQueue.flush();

		// Rebuild pipelines
		for (uint32_t i = 0; i < configurable.renderPipelines.size(); i++) {
			PipelineRenderItem& pipeline = configurable.renderPipelines[i];
			bool success = pipeline.buildPipeline();
			if (!success) {
				SFZ_WARNING("Renderer", "Failed to rebuild pipeline: \"%s\"",
					pipeline.name.str());
			}
		}
	}

	ImGui::Spacing();
	for (uint32_t i = 0; i < configurable.renderPipelines.size(); i++) {
		PipelineRenderItem& pipeline = configurable.renderPipelines[i];
		const ZgPipelineRenderSignature signature = pipeline.pipeline.getSignature();
		const char* name = pipeline.name.str();

		// Reload button
		if (ImGui::Button(str64("Reload##__render_%u", i), vec2(80.0f, 0.0f))) {

			// Flush ZeroG queues
			CHECK_ZG state.presentQueue.flush();

			if (pipeline.buildPipeline()) {
				SFZ_INFO("Renderer", "Reloaded pipeline: \"%s\"", name);
			}
			else {
				SFZ_WARNING("Renderer", "Failed to rebuild pipeline: \"%s\"", name);
			}
		}
		ImGui::SameLine();

		// Collapsing header with name
		bool collapsingHeaderOpen =
			ImGui::CollapsingHeader(str256("Pipeline %u - \"%s\"", i, name));
		if (!collapsingHeaderOpen) continue;
		ImGui::Indent(20.0f);

		// Valid or not
		ImGui::Indent(20.0f);
		if (!pipeline.pipeline.valid()) {
			ImGui::SameLine();
			ImGui::TextUnformatted("-- INVALID PIPELINE");
		}

		// Pipeline info
		ImGui::Spacing();
		ImGui::Text("Vertex shader: \"%s\" -- \"%s\"",
			pipeline.vertexShaderPath.str(), pipeline.vertexShaderEntry.str());
		ImGui::Text("Pixel shader: \"%s\" -- \"%s\"",
			pipeline.pixelShaderPath.str(), pipeline.pixelShaderEntry.str());

		// Print vertex attributes
		ImGui::Spacing();
		ImGui::Text("Vertex attributes (%u):", signature.numVertexAttributes);
		ImGui::Indent(20.0f);
		for (uint32_t j = 0; j < signature.numVertexAttributes; j++) {
			const ZgVertexAttribute& attrib = signature.vertexAttributes[j];
			ImGui::Text("- Location: %u -- Type: %s",
				attrib.location, vertexAttributeTypeToString(attrib.type));
		}
		ImGui::Unindent(20.0f);

		// Print constant buffers
		if (signature.bindings.numConstBuffers > 0) {
			ImGui::Spacing();
			ImGui::Text("Constant buffers (%u):", signature.bindings.numConstBuffers);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < signature.bindings.numConstBuffers; j++) {
				const ZgConstantBufferBindingDesc& cbuffer = signature.bindings.constBuffers[j];
				ImGui::Text("- Register: %u -- Size: %u bytes -- Push constant: %s",
					cbuffer.bufferRegister,
					cbuffer.sizeInBytes,
					cbuffer.pushConstant == ZG_TRUE ? "YES" : "NO");
			}
			ImGui::Unindent(20.0f);
		}

		// Print unordered buffers
		if (signature.bindings.numUnorderedBuffers > 0) {
			ImGui::Spacing();
			ImGui::Text("Unordered buffers (%u):", signature.bindings.numUnorderedBuffers);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < signature.bindings.numUnorderedBuffers; j++) {
				const ZgUnorderedBufferBindingDesc& buffer = signature.bindings.unorderedBuffers[j];
				ImGui::Text("- Register: %u", buffer.unorderedRegister);
			}
			ImGui::Unindent(20.0f);
		}

		// Print textures
		if (signature.bindings.numTextures > 0) {
			ImGui::Spacing();
			ImGui::Text("Textures (%u):", signature.bindings.numTextures);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < signature.bindings.numTextures; j++) {
				const ZgTextureBindingDesc& texture = signature.bindings.textures[j];
				ImGui::Text("- Register: %u", texture.textureRegister);
			}
			ImGui::Unindent(20.0f);
		}

		// Print unordered textures
		if (signature.bindings.numUnorderedTextures > 0) {
			ImGui::Spacing();
			ImGui::Text("Unordered textures (%u):", signature.bindings.numUnorderedTextures);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < signature.bindings.numUnorderedTextures; j++) {
				const ZgUnorderedTextureBindingDesc& texture = signature.bindings.unorderedTextures[j];
				ImGui::Text("- Register: %u", texture.unorderedRegister);
			}
			ImGui::Unindent(20.0f);
		}

		// Print samplers
		if (pipeline.samplers.size() > 0) {
			ImGui::Spacing();
			ImGui::Text("Samplers (%u):", pipeline.samplers.size());
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < pipeline.samplers.size(); j++) {
				SamplerItem& item = pipeline.samplers[j];
				ImGui::Text("- Register: %u", item.samplerRegister);
				ImGui::Indent(20.0f);
				constexpr float samplerXOffset = 260.0f;
				alignedEdit(" - Sampling Mode", samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%s", name).str(), samplingModeToString(item.sampler.samplingMode))) {
						if (ImGui::Selectable(samplingModeToString(ZG_SAMPLING_MODE_NEAREST), item.sampler.samplingMode == ZG_SAMPLING_MODE_NEAREST)) {
							item.sampler.samplingMode = ZG_SAMPLING_MODE_NEAREST;
						}
						if (ImGui::Selectable(samplingModeToString(ZG_SAMPLING_MODE_TRILINEAR), item.sampler.samplingMode == ZG_SAMPLING_MODE_TRILINEAR)) {
							item.sampler.samplingMode = ZG_SAMPLING_MODE_TRILINEAR;
						}
						if (ImGui::Selectable(samplingModeToString(ZG_SAMPLING_MODE_ANISOTROPIC), item.sampler.samplingMode == ZG_SAMPLING_MODE_ANISOTROPIC)) {
							item.sampler.samplingMode = ZG_SAMPLING_MODE_ANISOTROPIC;
						}
						ImGui::EndCombo();
					}
					
					
				});
				alignedEdit(" - Wrapping Mode U", samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%s", name).str(), wrappingModeToString(item.sampler.wrappingModeU))) {
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_CLAMP), item.sampler.wrappingModeU == ZG_WRAPPING_MODE_CLAMP)) {
							item.sampler.wrappingModeU = ZG_WRAPPING_MODE_CLAMP;
						}
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_REPEAT), item.sampler.wrappingModeU == ZG_WRAPPING_MODE_REPEAT)) {
							item.sampler.wrappingModeU = ZG_WRAPPING_MODE_REPEAT;
						}
						ImGui::EndCombo();
					}
				});
				alignedEdit(" - Wrapping Mode V", samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%s", name).str(), wrappingModeToString(item.sampler.wrappingModeV))) {
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_CLAMP), item.sampler.wrappingModeV == ZG_WRAPPING_MODE_CLAMP)) {
							item.sampler.wrappingModeV = ZG_WRAPPING_MODE_CLAMP;
						}
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_REPEAT), item.sampler.wrappingModeV == ZG_WRAPPING_MODE_REPEAT)) {
							item.sampler.wrappingModeV = ZG_WRAPPING_MODE_REPEAT;
						}
						ImGui::EndCombo();
					}
				});
				ImGui::Unindent(20.0f);
			}
			ImGui::Unindent(20.0f);
		}

		// Print render targets
		ImGui::Spacing();
		ImGui::Text("Render Targets (%u):", pipeline.renderTargets.size());
		ImGui::Indent(20.0f);
		for (uint32_t j = 0; j < pipeline.renderTargets.size(); j++) {
			ImGui::Text("- Render Target: %u -- %s",
				j, textureFormatToString(pipeline.renderTargets[j]));
		}
		ImGui::Unindent(20.0f);

		constexpr float xOffset = 300.0f;

		// Print depth test
		ImGui::Spacing();
		alignedEdit("Depth Test", xOffset, [&](const char* name) {
			ImGui::Checkbox(str128("##%s", name).str(), &pipeline.depthTest);
			ImGui::SameLine();
			ImGui::Text(" - %s", pipeline.depthTest ? "ENABLED" : "DISABLED");
		});
		if (pipeline.depthTest) {
			ImGui::Indent(20.0f);
			ImGui::Text("Depth function: %s", depthFuncToString(pipeline.depthFunc));
			ImGui::Unindent(20.0f);
		}

		// Print culling info
		ImGui::Spacing();
		alignedEdit("Culling", xOffset, [&](const char* name) {
			ImGui::Checkbox(str128("##%s", name).str(), &pipeline.cullingEnabled);
			ImGui::SameLine();
			ImGui::Text(" - %s", pipeline.cullingEnabled ? "ENABLED" : "DISABLED");
		});
		if (pipeline.cullingEnabled) {
			ImGui::Indent(20.0f);
			ImGui::Text("Cull Front Face: %s", pipeline.cullFrontFacing ? "YES" : "NO");
			ImGui::Text("Front Facing Is Counter Clockwise: %s", pipeline.frontFacingIsCounterClockwise ? "YES" : "NO");
			ImGui::Unindent(20.0f);
		}

		// Print depth bias info
		ImGui::Spacing();
		ImGui::Text("Depth Bias");
		ImGui::Indent(20.0f);
		alignedEdit("Bias", xOffset, [&](const char* name) {
			ImGui::SetNextItemWidth(165.0f);
			ImGui::InputInt(str128("%s##render_%u", name, i), &pipeline.depthBias);
		});
		alignedEdit("Bias Slope Scaled", xOffset, [&](const char* name) {
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputFloat(str128("%s##render_%u", name, i), &pipeline.depthBiasSlopeScaled, 0.0f, 0.0f, "%.4f");
		});
		alignedEdit("Bias Clamp", xOffset, [&](const char* name) {
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputFloat(str128("%s##render_%u", name, i), &pipeline.depthBiasClamp, 0.0f, 0.0f, "%.4f");
		});
		ImGui::Unindent(20.0f);

		// Print wireframe rendering mode
		ImGui::Spacing();
		alignedEdit("Wireframe Rendering", xOffset, [&](const char* name) {
			ImGui::Checkbox(str128("##%s", name).str(), &pipeline.wireframeRenderingEnabled);
			ImGui::SameLine();
			ImGui::Text(" - %s", pipeline.wireframeRenderingEnabled ? "ENABLED" : "DISABLED");
		});
		

		// Print blend mode
		ImGui::Spacing();
		ImGui::Text("Blend Mode: %s", blendModeToString(pipeline.blendMode));

		ImGui::Unindent(20.0f);
		ImGui::Unindent(20.0f);
		ImGui::Spacing();
	}



	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Text("Compute Pipelines");

	// Reload all button
	ImGui::SameLine(ImGui::GetWindowWidth() - 130.0f);
	if (ImGui::Button("Reload All##__compute_pipelines", vec2(120.0f, 0.0f))) {

		SFZ_INFO("Renderer", "Reloading all compute pipelines...");

		// Flush ZeroG queues
		CHECK_ZG state.presentQueue.flush();

		// Rebuild pipelines
		for (uint32_t i = 0; i < configurable.computePipelines.size(); i++) {
			PipelineComputeItem& pipeline = configurable.computePipelines[i];
			bool success = pipeline.buildPipeline();
			if (!success) {
				SFZ_WARNING("Renderer", "Failed to rebuild pipeline: \"%s\"", pipeline.name.str());
			}
		}
	}

	ImGui::Spacing();
	for (uint32_t pipelineIdx = 0; pipelineIdx < configurable.computePipelines.size(); pipelineIdx++) {
		PipelineComputeItem& pipeline = configurable.computePipelines[pipelineIdx];
		const ZgPipelineBindingsSignature& bindingsSignature = pipeline.pipeline.getBindingsSignature();
		const char* name = pipeline.name.str();

		// Reload button
		if (ImGui::Button(str64("Reload##__compute_%u", pipelineIdx), vec2(80.0f, 0.0f))) {

			// Flush ZeroG queues
			CHECK_ZG state.presentQueue.flush();

			if (pipeline.buildPipeline()) {
				SFZ_INFO("Renderer", "Reloaded pipeline: \"%s\"", name);
			}
			else {
				SFZ_WARNING("Renderer", "Failed to rebuild pipeline: \"%s\"", name);
			}
		}
		ImGui::SameLine();

		// Collapsing header with name
		bool collapsingHeaderOpen =
			ImGui::CollapsingHeader(str256("Pipeline %u - \"%s\"", pipelineIdx, name));
		if (!collapsingHeaderOpen) continue;
		ImGui::Indent(20.0f);

		// Valid or not
		ImGui::Indent(20.0f);
		if (!pipeline.pipeline.valid()) {
			ImGui::SameLine();
			ImGui::TextUnformatted("-- INVALID PIPELINE");
		}

		// Pipeline info
		ImGui::Spacing();
		ImGui::Text("Compute shader: \"%s\" -- \"%s\"",
			pipeline.computeShaderPath.str(), pipeline.computeShaderEntry.str());

		// Group dimensions
		ImGui::Spacing();
		vec3_u32 groupDims = vec3_u32(0u);
		pipeline.pipeline.getGroupDims(groupDims.x, groupDims.y, groupDims.z);
		ImGui::Text("Group dims: %u x %u x %u", groupDims.x, groupDims.y, groupDims.z);

		// Print constant buffers
		if (bindingsSignature.numConstBuffers > 0) {
			ImGui::Spacing();
			ImGui::Text("Constant buffers (%u):", bindingsSignature.numConstBuffers);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < bindingsSignature.numConstBuffers; j++) {
				const ZgConstantBufferBindingDesc& cbuffer = bindingsSignature.constBuffers[j];
				ImGui::Text("- Register: %u -- Size: %u bytes -- Push constant: %s",
					cbuffer.bufferRegister,
					cbuffer.sizeInBytes,
					cbuffer.pushConstant == ZG_TRUE ? "YES" : "NO");
			}
			ImGui::Unindent(20.0f);
		}

		// Print unordered buffers
		if (bindingsSignature.numUnorderedBuffers > 0) {
			ImGui::Spacing();
			ImGui::Text("Unordered buffers (%u):", bindingsSignature.numUnorderedBuffers);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < bindingsSignature.numUnorderedBuffers; j++) {
				const ZgUnorderedBufferBindingDesc& buffer = bindingsSignature.unorderedBuffers[j];
				ImGui::Text("- Register: %u", buffer.unorderedRegister);
			}
			ImGui::Unindent(20.0f);
		}

		// Print textures
		if (bindingsSignature.numTextures > 0) {
			ImGui::Spacing();
			ImGui::Text("Textures (%u):", bindingsSignature.numTextures);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < bindingsSignature.numTextures; j++) {
				const ZgTextureBindingDesc& texture = bindingsSignature.textures[j];
				ImGui::Text("- Register: %u", texture.textureRegister);
			}
			ImGui::Unindent(20.0f);
		}

		// Print unordered textures
		if (bindingsSignature.numUnorderedTextures > 0) {
			ImGui::Spacing();
			ImGui::Text("Unordered textures (%u):", bindingsSignature.numUnorderedTextures);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < bindingsSignature.numUnorderedTextures; j++) {
				const ZgUnorderedTextureBindingDesc& texture = bindingsSignature.unorderedTextures[j];
				ImGui::Text("- Register: %u", texture.unorderedRegister);
			}
			ImGui::Unindent(20.0f);
		}

		// Print samplers
		if (pipeline.samplers.size() > 0) {
			ImGui::Spacing();
			ImGui::Text("Samplers (%u):", pipeline.samplers.size());
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < pipeline.samplers.size(); j++) {
				SamplerItem& item = pipeline.samplers[j];
				ImGui::Text("- Register: %u", item.samplerRegister);
				ImGui::Indent(20.0f);
				constexpr float samplerXOffset = 260.0f;
				alignedEdit(" - Sampling Mode", samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%s", name).str(), samplingModeToString(item.sampler.samplingMode))) {
						if (ImGui::Selectable(samplingModeToString(ZG_SAMPLING_MODE_NEAREST), item.sampler.samplingMode == ZG_SAMPLING_MODE_NEAREST)) {
							item.sampler.samplingMode = ZG_SAMPLING_MODE_NEAREST;
						}
						if (ImGui::Selectable(samplingModeToString(ZG_SAMPLING_MODE_TRILINEAR), item.sampler.samplingMode == ZG_SAMPLING_MODE_TRILINEAR)) {
							item.sampler.samplingMode = ZG_SAMPLING_MODE_TRILINEAR;
						}
						if (ImGui::Selectable(samplingModeToString(ZG_SAMPLING_MODE_ANISOTROPIC), item.sampler.samplingMode == ZG_SAMPLING_MODE_ANISOTROPIC)) {
							item.sampler.samplingMode = ZG_SAMPLING_MODE_ANISOTROPIC;
						}
						ImGui::EndCombo();
					}
					
					
				});
				alignedEdit(" - Wrapping Mode U", samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%s", name).str(), wrappingModeToString(item.sampler.wrappingModeU))) {
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_CLAMP), item.sampler.wrappingModeU == ZG_WRAPPING_MODE_CLAMP)) {
							item.sampler.wrappingModeU = ZG_WRAPPING_MODE_CLAMP;
						}
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_REPEAT), item.sampler.wrappingModeU == ZG_WRAPPING_MODE_REPEAT)) {
							item.sampler.wrappingModeU = ZG_WRAPPING_MODE_REPEAT;
						}
						ImGui::EndCombo();
					}
				});
				alignedEdit(" - Wrapping Mode V", samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%s", name).str(), wrappingModeToString(item.sampler.wrappingModeV))) {
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_CLAMP), item.sampler.wrappingModeV == ZG_WRAPPING_MODE_CLAMP)) {
							item.sampler.wrappingModeV = ZG_WRAPPING_MODE_CLAMP;
						}
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_REPEAT), item.sampler.wrappingModeV == ZG_WRAPPING_MODE_REPEAT)) {
							item.sampler.wrappingModeV = ZG_WRAPPING_MODE_REPEAT;
						}
						ImGui::EndCombo();
					}
				});
				ImGui::Unindent(20.0f);
			}
			ImGui::Unindent(20.0f);
		}

		ImGui::Unindent(20.0f);
		ImGui::Unindent(20.0f);
		ImGui::Spacing();
	}
}

void RendererUI::renderStaticMemoryTab(RendererConfigurableState& state) noexcept
{
	if (ImGui::CollapsingHeader("Static Textures")) {
		for (uint32_t i = 0; i < state.staticTextures.size(); i++) {
			const StaticTextureItem& texItem = state.staticTextures.values()[i];

			// Texture name
			ImGui::Text("Texture %u - \"%s\" - %s - %ux%u", i,
				texItem.name.str(),
				textureFormatToString(texItem.format),
				texItem.width,
				texItem.height);
			ImGui::Indent(20.0f);

			constexpr float offset = 220.0f;

			// Mipmaps
			sfz_assert(texItem.numMipmaps != 0);
			if (texItem.numMipmaps > 1) {
				alignedEdit(" - Num mipmaps", offset, [&](const char*) {
					ImGui::Text("%u", texItem.numMipmaps);
				});
			}

			// Clear value
			if (texItem.clearValue != 0.0f) {
				alignedEdit(" - Clear", offset, [&](const char*) {
					ImGui::Text("%.1f", texItem.clearValue);
				});
			}

			// Resolution type
			if (texItem.resolutionIsFixed) {
				alignedEdit(" - Fixed resolution", offset, [&](const char*) {
					ImGui::Text("%i x %i", texItem.resolutionFixed.x, texItem.resolutionFixed.y);
				});
			}
			else {
				if (texItem.resolutionScaleSetting != nullptr) {
					alignedEdit(" - Resolution scale", offset, [&](const char*) {
						ImGui::Text("%.2f  --  Setting: \"%s\"",
							texItem.resolutionScale, texItem.resolutionScaleSetting->key().str());
					});
				}
				else {
					alignedEdit(" - Resolution scale", offset, [&](const char*) {
						ImGui::Text("%.2f", texItem.resolutionScale);
					});
				}
			}

			ImGui::Unindent(20.0f);
			ImGui::Spacing();
			ImGui::Spacing();
		}
	}

	if (ImGui::CollapsingHeader("Static Buffers")) {
		for (uint32_t i = 0; i < state.staticBuffers.size(); i++) {
			const StaticBufferItem& bufItem = state.staticBuffers.values()[i];

			ImGui::Text("Buffer %u - \"%s\" - %u bytes x %u elements",
				i,
				bufItem.name.str(),
				bufItem.elementSizeBytes,
				bufItem.maxNumElements);
			ImGui::Spacing();
			ImGui::Spacing();
		}
	}
}

void RendererUI::renderStreamingBuffersTab(RendererConfigurableState& state) noexcept
{
	constexpr float offset = 220.0f;

	for (auto itemItr : state.streamingBuffers) {
		const StreamingBufferItem& item = itemItr.value;

		ImGui::Text("\"%s\"", itemItr.key.str());

		ImGui::Indent(20.0f);
		alignedEdit("Element size", offset, [&](const char*) {
			ImGui::Text("%u bytes", item.elementSizeBytes);
		});
		alignedEdit("Max num elements", offset, [&](const char*) {
			ImGui::Text("%u", item.maxNumElements);
		});
		alignedEdit("Committed allocation", offset, [&](const char*) {
			ImGui::Text("%s", item.committedAllocation ? "TRUE" : "FALSE");
		});

		ImGui::Unindent(20.0f);
		ImGui::Spacing();
	}
}

void RendererUI::renderMeshesTab(RendererState& state) noexcept
{
	for (auto itemItr : state.meshes) {
		GpuMesh& mesh = itemItr.value;

		// Check if mesh is valid
		bool meshValid = true;
		if (!mesh.vertexBuffer.valid()) meshValid = false;
		if (!mesh.indexBuffer.valid()) meshValid = false;
		if (!mesh.materialsBuffer.valid()) meshValid = false;

		// Mesh name
		ImGui::Text("\"%s\"", itemItr.key.str());
		if (!meshValid) {
			ImGui::SameLine();
			ImGui::Text("-- NOT VALID");
		}
		ImGui::SameLine();
		ImGui::Checkbox(str64("##mesh_%llu_enabled", itemItr.key), &mesh.enabled);

		// Components
		ImGui::Indent(20.0f);
		if (ImGui::CollapsingHeader(str64("Components (%u):##%llu", mesh.components.size(), itemItr.key))) {

			ImGui::Indent(20.0f);
			for (uint32_t i = 0; i < mesh.components.size(); i++) {

				const MeshComponent& comp = mesh.components[i];
				constexpr float offset = 250.0f;
				ImGui::Text("Component %u -- Material Index: %u -- NumIndices: %u",
					i, comp.materialIdx, comp.numIndices);
			}
			ImGui::Unindent(20.0f);
		}
		ImGui::Unindent(20.0f);

		// Lambdas for converting vec4_u8 to vec4_f32 and back
		auto u8ToF32 = [](vec4_u8 v) { return vec4(v) * (1.0f / 255.0f); };
		auto f32ToU8 = [](vec4 v) { return vec4_u8(v * 255.0f); };

		// Lambda for converting texture index to combo string label
		auto textureToComboStr = [&](strID strId) {
			str128 texStr;
			if (!strId.isValid()) texStr.appendf("NO TEXTURE");
			else texStr= str128("%s", strId.str());
			return texStr;
		};

		// Lambda for creating a combo box to select texture
		auto textureComboBox = [&](const char* comboName, strID& texId, bool& updateMesh) {
			(void)updateMesh;
			(void)comboName;
			str128 selectedTexStr = textureToComboStr(texId);
			ImGui::Text("%s", textureToComboStr(texId));
			/*if (ImGui::BeginCombo(comboName, selectedTexStr)) {

				// Special case for no texture (~0)
				{
					bool isSelected = !texId.isValid();
					if (ImGui::Selectable("NO TEXTURE", isSelected)) {
						texId = STR_ID_INVALID;
						updateMesh = true;
					}
				}

				// Existing textures
				for (auto itemItr : state.textures) {
					strID id = itemItr.key;

					// Convert index to string and check if it is selected
					str128 texStr = textureToComboStr(id);
					bool isSelected = id == texId;

					// Report index to ImGui combo button and update current if it has changed
					if (ImGui::Selectable(texStr, isSelected)) {
						texId = id;
						updateMesh = true;
					}
				}
				ImGui::EndCombo();
			}*/
		};

		// Materials
		ImGui::Indent(20.0f);
		if (ImGui::CollapsingHeader(str64("Materials (%u):##%llu", mesh.cpuMaterials.size(), itemItr.key))) {

			ImGui::Indent(20.0f);
			for (uint32_t i = 0; i < mesh.cpuMaterials.size(); i++) {
				Material& material = mesh.cpuMaterials[i];

				// Edit CPU material
				bool updateMesh = false;
				if (ImGui::CollapsingHeader(str64("Material %u##%llu", i, itemItr.key))) {

					ImGui::Indent(20.0f);
					constexpr float offset = 310.0f;

					// Albedo
					vec4 colorFloat = u8ToF32(material.albedo);
					alignedEdit("Albedo Factor", offset, [&](const char* name) {
						if (ImGui::ColorEdit4(str128("%s##%u_%llu", name, i, itemItr.key), colorFloat.data(),
							ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float)) {
							material.albedo = f32ToU8(colorFloat);
							updateMesh = true;
						}
					});
					alignedEdit("Albedo Texture", offset, [&](const char* name) {
						textureComboBox(str128("##%s_%u_%llu", name, i, itemItr.key), material.albedoTex, updateMesh);
					});

					// Emissive
					alignedEdit("Emissive Factor", offset, [&](const char* name) {
						if (ImGui::ColorEdit3(str128("%s##%u_%llu", name, i, itemItr.key), material.emissive.data(),
							ImGuiColorEditFlags_Float)) {
							updateMesh = true;
						}
					});
					alignedEdit("Emissive Texture", offset, [&](const char* name) {
						textureComboBox(str128("##%s_%u_%llu", name, i, itemItr.key), material.emissiveTex, updateMesh);
					});

					// Metallic & roughness
					vec4_u8 metallicRoughnessU8(material.metallic, material.roughness, 0, 0);
					vec4 metallicRoughness = u8ToF32(metallicRoughnessU8);
					alignedEdit("Metallic Roughness Factors", offset, [&](const char* name) {
						if (ImGui::SliderFloat2(str128("%s##%u_%llu", name, i, itemItr.key), metallicRoughness.data(), 0.0f, 1.0f)) {
							metallicRoughnessU8 = f32ToU8(metallicRoughness);
							material.metallic = metallicRoughnessU8.x;
							material.roughness = metallicRoughnessU8.y;
							updateMesh = true;
						}
					});
					alignedEdit("Metallic Roughness Texture", offset, [&](const char* name) {
						textureComboBox(str64("##%s_%u_%llu", name, i, itemItr.key), material.metallicRoughnessTex, updateMesh);
					});

					// Normal and Occlusion textures
					alignedEdit("Normal Texture", offset, [&](const char* name) {
						textureComboBox(str64("##%s_%u_%llu", name, i, itemItr.key), material.normalTex, updateMesh);
					});
					alignedEdit("Occlusion Texture", offset, [&](const char* name) {
						textureComboBox(str64("##%s_%u_%llu", name, i, itemItr.key), material.occlusionTex, updateMesh);
					});

					ImGui::Unindent(20.0f);
				}

				// If material was edited, update mesh
				if (updateMesh) {

					// Flush ZeroG queues
					CHECK_ZG state.copyQueue.flush();
					CHECK_ZG state.presentQueue.flush();

					// Allocate temporary upload buffer
					zg::Buffer uploadBuffer;
					CHECK_ZG uploadBuffer.create(sizeof(ShaderMaterial), ZG_MEMORY_TYPE_UPLOAD);
					sfz_assert(uploadBuffer.valid());

					// Convert new material to shader material
					ShaderMaterial shaderMaterial = cpuMaterialToShaderMaterial(material);

					// Memcpy to temporary upload buffer
					CHECK_ZG uploadBuffer.memcpyUpload(0, &shaderMaterial, sizeof(ShaderMaterial));

					// Replace material in mesh with new material
					zg::CommandList commandList;
					CHECK_ZG state.presentQueue.beginCommandListRecording(commandList);
					uint64_t dstOffset = sizeof(ShaderMaterial) * i;
					CHECK_ZG commandList.memcpyBufferToBuffer(
						mesh.materialsBuffer, dstOffset, uploadBuffer, 0, sizeof(ShaderMaterial));
					CHECK_ZG state.presentQueue.executeCommandList(commandList);
					CHECK_ZG state.presentQueue.flush();
				}
			}
			ImGui::Unindent(20.0f);
		}
		ImGui::Unindent(20.0f);

		ImGui::Spacing();
	}
}

} // namespace sfz
