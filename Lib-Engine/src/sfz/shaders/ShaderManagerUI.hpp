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

#pragma once

#include <imgui.h>

#include "sfz/Logging.hpp"
#include "sfz/renderer/RenderingEnumsToFromString.hpp"
#include "sfz/shaders/ShaderManagerState.hpp"
#include "sfz/util/ImGuiHelpers.hpp"

namespace sfz {

// ShaderManagerUI
// ------------------------------------------------------------------------------------------------

inline void shaderManagerUI(ShaderManagerState& state)
{
	if (!ImGui::Begin("Shaders", nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		ImGui::End();
		return;
	}

	constexpr float offset = 150.0f;
	constexpr vec4 normalTextColor = vec4(1.0f);
	constexpr vec4 filterTextColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	static str128 filter;

	ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
	ImGui::InputText("Filter##BuffersTab", filter.mRawStr, filter.capacity());
	ImGui::PopStyleColor();
	filter.toLower();

	const bool filterMode = filter != "";

	// Reload all button
	ImGui::SameLine(ImGui::GetWindowWidth() - 130.0f);
	if (ImGui::Button("Reload All##__shaders", vec2(120.0f, 0.0f))) {

		SFZ_INFO("Shaders", "Reloading all shaders...");

		// Flush ZeroG queues
		CHECK_ZG zg::CommandQueue::getPresentQueue().flush();

		// Rebuild shaders
		for (HashMapPair<strID, PoolHandle> itemItr : state.shaderHandles) {
			Shader& shader = state.shaders[itemItr.value];
			bool success = shader.build();
			if (!success) {
				SFZ_WARNING("Shaders", "Failed to rebuild shader: \"%s\"", shader.name.str());
			}
		}
	}

	for (HashMapPair<strID, PoolHandle> itemItr : state.shaderHandles) {
		const char* name = itemItr.key.str();
		const u32 idx = itemItr.value.idx();
		Shader& shader = state.shaders[itemItr.value];

		str320 lowerCaseName = name;
		lowerCaseName.toLower();
		if (!filter.isPartOf(lowerCaseName.str())) continue;

		// Reload button
		if (ImGui::Button(str64("Reload##__shader%u", idx), vec2(80.0f, 0.0f))) {

			CHECK_ZG zg::CommandQueue::getPresentQueue().flush();

			if (shader.build()) {
				SFZ_INFO("Shaders", "Reloaded shader: \"%s\"", name);
			}
			else {
				SFZ_WARNING("Shaders", "Failed to rebuild shader: \"%s\"", name);
			}
		}
		ImGui::SameLine();

		// Shader name
		if (filterMode) {
			imguiRenderFilteredText(name, filter.str(), normalTextColor, filterTextColor);
		}
		else {
			if (!ImGui::CollapsingHeader(name)) continue;
		}

		ImGui::Indent(20.0f);

		// Type
		ImGui::Spacing();
		alignedEdit("Type", offset, [&](const char*) {
			if (shader.type == ShaderType::RENDER) {
				ImGui::Text("RENDER");
			}
			else if (shader.type == ShaderType::COMPUTE) {
				ImGui::Text("COMPUTE");
			}
			else {
				sfz_assert(false);
			}
		});

		// Path
		alignedEdit("Path", offset, [&](const char*) {
			ImGui::Text("%s", shader.shaderPath.str());
		});

		// Group dimensions for compute shaders
		if (shader.type == ShaderType::COMPUTE) {
			vec3_u32 groupDims = vec3_u32(0u);
			shader.compute.pipeline.getGroupDims(groupDims.x, groupDims.y, groupDims.z);
			alignedEdit("Group dims", offset, [&](const char*) {
				ImGui::Text("%u x %u x %u", groupDims.x, groupDims.y, groupDims.z);
			});
		}

		// Print vertex attributes of render shaders
		if (shader.type == ShaderType::RENDER) {
			const ZgPipelineRenderSignature signature = shader.render.pipeline.getSignature();

			// Print vertex attributes
			ImGui::Spacing();
			ImGui::Text("Vertex attributes (%u):", signature.numVertexAttributes);
			ImGui::Indent(20.0f);
			for (u32 j = 0; j < signature.numVertexAttributes; j++) {
				const ZgVertexAttribute& attrib = signature.vertexAttributes[j];
				ImGui::Text("- Location: %u -- Type: %s",
					attrib.location, vertexAttributeTypeToString(attrib.type));
			}
			ImGui::Unindent(20.0f);
		}

		// Print bindings
		ZgPipelineBindingsSignature bindings = {};
		if (shader.type == ShaderType::RENDER) {
			bindings = shader.render.pipeline.getSignature().bindings;
		}
		else if (shader.type == ShaderType::COMPUTE) {
			bindings = shader.compute.pipeline.getBindingsSignature();
		}

		// Print constant buffers
		if (bindings.numConstBuffers > 0) {
			ImGui::Spacing();
			ImGui::Text("Constant buffers (%u):", bindings.numConstBuffers);
			ImGui::Indent(20.0f);
			for (u32 j = 0; j < bindings.numConstBuffers; j++) {
				const ZgConstantBufferBindingDesc& cbuffer = bindings.constBuffers[j];
				ImGui::Text("- Register: %u -- Size: %u bytes -- Push constant: %s",
					cbuffer.bufferRegister,
					cbuffer.sizeInBytes,
					cbuffer.pushConstant == ZG_TRUE ? "YES" : "NO");
			}
			ImGui::Unindent(20.0f);
		}

		// Print textures
		if (bindings.numTextures > 0) {
			ImGui::Spacing();
			ImGui::Text("Textures (%u):  ", bindings.numTextures);
			for (u32 j = 0; j < bindings.numTextures; j++) {
				const ZgTextureBindingDesc& texture = bindings.textures[j];
				ImGui::SameLine();
				ImGui::Text("%u, ", texture.textureRegister);
			}
		}

		// Print unordered buffers
		if (bindings.numUnorderedBuffers > 0) {
			ImGui::Spacing();
			ImGui::Text("Unordered buffers (%u):  ", bindings.numUnorderedBuffers);
			for (u32 j = 0; j < bindings.numUnorderedBuffers; j++) {
				const ZgUnorderedBufferBindingDesc& buffer = bindings.unorderedBuffers[j];
				ImGui::SameLine();
				ImGui::Text("%u, ", buffer.unorderedRegister);
			}
		}

		// Print unordered textures
		if (bindings.numUnorderedTextures > 0) {
			ImGui::Spacing();
			ImGui::Text("Unordered textures (%u):  ", bindings.numUnorderedTextures);
			for (u32 j = 0; j < bindings.numUnorderedTextures; j++) {
				const ZgUnorderedTextureBindingDesc& texture = bindings.unorderedTextures[j];
				ImGui::SameLine();
				ImGui::Text("%u, ", texture.unorderedRegister);
			}
		}

		// Print samplers
		if (shader.samplers.size() > 0) {
			ImGui::Spacing();
			ImGui::Text("Samplers (%u):", shader.samplers.size());
			ImGui::Indent(20.0f);
			for (u32 j = 0; j < shader.samplers.size(); j++) {
				SamplerItem& item = shader.samplers[j];
				ImGui::Text("- Register: %u", item.samplerRegister);
				ImGui::Indent(20.0f);
				constexpr float samplerXOffset = 260.0f;
				alignedEdit(" - Sampling Mode", "sampler", j, samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%u%s", idx, name).str(), samplingModeToString(item.sampler.samplingMode))) {
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
				alignedEdit(" - Wrapping Mode U", "sampler", j, samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%u%s", idx, name).str(), wrappingModeToString(item.sampler.wrappingModeU))) {
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_CLAMP), item.sampler.wrappingModeU == ZG_WRAPPING_MODE_CLAMP)) {
							item.sampler.wrappingModeU = ZG_WRAPPING_MODE_CLAMP;
						}
						if (ImGui::Selectable(wrappingModeToString(ZG_WRAPPING_MODE_REPEAT), item.sampler.wrappingModeU == ZG_WRAPPING_MODE_REPEAT)) {
							item.sampler.wrappingModeU = ZG_WRAPPING_MODE_REPEAT;
						}
						ImGui::EndCombo();
					}
				});
				alignedEdit(" - Wrapping Mode V", "sampler", j, samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%u%s", idx, name).str(), wrappingModeToString(item.sampler.wrappingModeV))) {
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

		// Render shader
		if (shader.type == ShaderType::RENDER) {
			ShaderRender& render = shader.render;

			// Print render targets
			ImGui::Spacing();
			ImGui::Text("Render Targets (%u):", render.renderTargets.size());
			ImGui::Indent(20.0f);
			for (u32 j = 0; j < render.renderTargets.size(); j++) {
				ImGui::Text("- Render Target: %u -- %s",
					j, textureFormatToString(render.renderTargets[j]));
			}
			ImGui::Unindent(20.0f);

			constexpr float xOffset = 300.0f;

			// Print depth test
			ImGui::Spacing();
			alignedEdit("Depth function", xOffset, [&](const char*) {
				ImGui::Text("%s", comparisonFuncToString(render.depthFunc));
			});

			// Print culling info
			ImGui::Spacing();
			alignedEdit("Culling", xOffset, [&](const char* name) {
				ImGui::Checkbox(str128("##%s", name).str(), &render.cullingEnabled);
				ImGui::SameLine();
				ImGui::Text(" - %s", render.cullingEnabled ? "ENABLED" : "DISABLED");
			});
			if (render.cullingEnabled) {
				ImGui::Indent(20.0f);
				ImGui::Text("Cull Front Face: %s", render.cullFrontFacing ? "YES" : "NO");
				ImGui::Text("Front Facing Is Counter Clockwise: %s", render.frontFacingIsCounterClockwise ? "YES" : "NO");
				ImGui::Unindent(20.0f);
			}

			// Print depth bias info
			ImGui::Spacing();
			ImGui::Text("Depth Bias");
			ImGui::Indent(20.0f);
			alignedEdit("Bias", xOffset, [&](const char* name) {
				ImGui::SetNextItemWidth(165.0f);
				ImGui::InputInt(str128("%s##render_%u", name, idx), &render.depthBias);
			});
			alignedEdit("Bias Slope Scaled", xOffset, [&](const char* name) {
				ImGui::SetNextItemWidth(100.0f);
				ImGui::InputFloat(str128("%s##render_%u", name, idx), &render.depthBiasSlopeScaled, 0.0f, 0.0f, "%.4f");
			});
			alignedEdit("Bias Clamp", xOffset, [&](const char* name) {
				ImGui::SetNextItemWidth(100.0f);
				ImGui::InputFloat(str128("%s##render_%u", name, idx), &render.depthBiasClamp, 0.0f, 0.0f, "%.4f");
			});
			ImGui::Unindent(20.0f);

			// Print wireframe rendering mode
			ImGui::Spacing();
			alignedEdit("Wireframe Rendering", xOffset, [&](const char* name) {
				ImGui::Checkbox(str128("##%s", name).str(), &render.wireframeRenderingEnabled);
				ImGui::SameLine();
				ImGui::Text(" - %s", render.wireframeRenderingEnabled ? "ENABLED" : "DISABLED");
			});

			// Print blend mode
			ImGui::Spacing();
			ImGui::Text("Blend Mode: %s", blendModeToString(render.blendMode));
		}

		ImGui::Spacing();
		ImGui::Unindent(20.0f);
	}

	ImGui::End();
}

} // namespace sfz
