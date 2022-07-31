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

#include "sfz/SfzLogging.h"
#include "sfz/renderer/RenderingEnumsToFromString.hpp"
#include "sfz/shaders/ShaderManagerState.hpp"
#include "sfz/util/ImGuiHelpers.hpp"

namespace sfz {

// ShaderManagerUI
// ------------------------------------------------------------------------------------------------

inline void shaderManagerUI(SfzShaderManagerState& state, SfzStrIDs* ids)
{
	if (!ImGui::Begin("Shaders", nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		ImGui::End();
		return;
	}

	constexpr f32 offset = 150.0f;
	constexpr f32x4 normalTextColor = f32x4(1.0f);
	constexpr f32x4 filterTextColor = f32x4(1.0f, 0.0f, 0.0f, 1.0f);
	static str128 filter;

	ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
	ImGui::InputText("Filter##BuffersTab", filter.mRawStr, filter.capacity());
	ImGui::PopStyleColor();
	filter.toLower();

	const bool filterMode = filter != "";

	// Reload all button
	ImGui::SameLine(ImGui::GetWindowWidth() - 130.0f);
	if (ImGui::Button("Reload All##__shaders", f32x2(120.0f, 0.0f))) {

		SFZ_LOG_INFO("Reloading all shaders...");

		// Flush ZeroG queues
		CHECK_ZG zg::CommandQueue::getPresentQueue().flush();

		// Rebuild shaders
		for (HashMapPair<SfzStrID, SfzHandle> itemItr : state.shaderHandles) {
			SfzShader& shader = state.shaders[itemItr.value];
			bool success = shader.build();
			if (!success) {
				SFZ_LOG_WARNING("Failed to rebuild shader: \"%s\"", sfzStrIDGetStr(ids, shader.name));
			}
		}
	}

	for (HashMapPair<SfzStrID, SfzHandle> itemItr : state.shaderHandles) {
		const char* name = sfzStrIDGetStr(ids, itemItr.key);
		const u32 idx = itemItr.value.idx();
		SfzShader& shader = state.shaders[itemItr.value];

		str320 lowerCaseName = name;
		lowerCaseName.toLower();
		if (!filter.isPartOf(lowerCaseName.str())) continue;

		// Reload button
		if (ImGui::Button(str64("Reload##__shader%u", idx), f32x2(80.0f, 0.0f))) {

			CHECK_ZG zg::CommandQueue::getPresentQueue().flush();

			if (shader.build()) {
				SFZ_LOG_INFO("Reloaded shader: \"%s\"", name);
			}
			else {
				SFZ_LOG_WARNING("Failed to rebuild shader: \"%s\"", name);
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
			if (shader.type == SfzShaderType::RENDER) {
				ImGui::Text("RENDER");
			}
			else if (shader.type == SfzShaderType::COMPUTE) {
				ImGui::Text("COMPUTE");
			}
			else {
				sfz_assert(false);
			}
		});

		// Path
		alignedEdit("Path", offset, [&](const char*) {
			if (shader.type == SfzShaderType::RENDER) {
				ImGui::Text("%s", shader.renderDesc.path);
			}
			else if (shader.type == SfzShaderType::COMPUTE) {
				ImGui::Text("%s", shader.computeDesc.path);
			}
		});

		// Group dimensions for compute shaders
		if (shader.type == SfzShaderType::COMPUTE) {
			u32 x = 0, y = 0, z = 0;
			shader.computePipeline.getGroupDims(x, y, z);
			alignedEdit("Group dims", offset, [&](const char*) {
				ImGui::Text("%u x %u x %u", x, y, z);
			});
		}

		// Print samplers
		const u32 numSamplers = shader.type == SfzShaderType::RENDER ?
			shader.renderDesc.numSamplers : shader.computeDesc.numSamplers;
		if (numSamplers > 0) {
			ImGui::Spacing();
			ImGui::Text("Samplers (%u):", numSamplers);
			ImGui::Indent(20.0f);
			ZgSampler* samplers = shader.type == SfzShaderType::RENDER ?
				shader.renderDesc.samplers : shader.computeDesc.samplers;
			for (u32 j = 0; j < numSamplers; j++) {
				ZgSampler& sampler = samplers[j];
				ImGui::Text("- Register: %u", j);
				ImGui::Indent(20.0f);
				constexpr f32 samplerXOffset = 260.0f;
				alignedEdit(" - Sample Mode", "sampler", j, samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(160.0f);
					if (ImGui::BeginCombo(str128("##%u%s", idx, name).str(), sampleModeToString(sampler.sampleMode))) {
						if (ImGui::Selectable(sampleModeToString(ZG_SAMPLE_NEAREST), sampler.sampleMode == ZG_SAMPLE_NEAREST)) {
							sampler.sampleMode = ZG_SAMPLE_NEAREST;
						}
						if (ImGui::Selectable(sampleModeToString(ZG_SAMPLE_TRILINEAR), sampler.sampleMode == ZG_SAMPLE_TRILINEAR)) {
							sampler.sampleMode = ZG_SAMPLE_TRILINEAR;
						}
						if (ImGui::Selectable(sampleModeToString(ZG_SAMPLE_ANISOTROPIC_2X), sampler.sampleMode == ZG_SAMPLE_ANISOTROPIC_2X)) {
							sampler.sampleMode = ZG_SAMPLE_ANISOTROPIC_2X;
						}
						if (ImGui::Selectable(sampleModeToString(ZG_SAMPLE_ANISOTROPIC_4X), sampler.sampleMode == ZG_SAMPLE_ANISOTROPIC_4X)) {
							sampler.sampleMode = ZG_SAMPLE_ANISOTROPIC_4X;
						}
						if (ImGui::Selectable(sampleModeToString(ZG_SAMPLE_ANISOTROPIC_8X), sampler.sampleMode == ZG_SAMPLE_ANISOTROPIC_8X)) {
							sampler.sampleMode = ZG_SAMPLE_ANISOTROPIC_8X;
						}
						if (ImGui::Selectable(sampleModeToString(ZG_SAMPLE_ANISOTROPIC_16X), sampler.sampleMode == ZG_SAMPLE_ANISOTROPIC_16X)) {
							sampler.sampleMode = ZG_SAMPLE_ANISOTROPIC_16X;
						}
						ImGui::EndCombo();
					}
				});
				alignedEdit(" - Wrap U", "sampler", j, samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%u%s", idx, name).str(), wrapModeToString(sampler.wrapU))) {
						if (ImGui::Selectable(wrapModeToString(ZG_WRAP_CLAMP), sampler.wrapU == ZG_WRAP_CLAMP)) {
							sampler.wrapU = ZG_WRAP_CLAMP;
						}
						if (ImGui::Selectable(wrapModeToString(ZG_WRAP_REPEAT), sampler.wrapU == ZG_WRAP_REPEAT)) {
							sampler.wrapU = ZG_WRAP_REPEAT;
						}
						ImGui::EndCombo();
					}
				});
				alignedEdit(" - Wrap V", "sampler", j, samplerXOffset, [&](const char* name) {
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo(str128("##%u%s", idx, name).str(), wrapModeToString(sampler.wrapV))) {
						if (ImGui::Selectable(wrapModeToString(ZG_WRAP_CLAMP), sampler.wrapV == ZG_WRAP_CLAMP)) {
							sampler.wrapV = ZG_WRAP_CLAMP;
						}
						if (ImGui::Selectable(wrapModeToString(ZG_WRAP_REPEAT), sampler.wrapV == ZG_WRAP_REPEAT)) {
							sampler.wrapV = ZG_WRAP_REPEAT;
						}
						ImGui::EndCombo();
					}
				});
				ImGui::Unindent(20.0f);
			}
			ImGui::Unindent(20.0f);
		}

		// Render shader
		if (shader.type == SfzShaderType::RENDER) {
			ZgPipelineRenderDesc& render = shader.renderDesc;

			// Print render targets
			ImGui::Spacing();
			ImGui::Text("Render Targets (%u):", render.numRenderTargets);
			ImGui::Indent(20.0f);
			for (u32 j = 0; j < render.numRenderTargets; j++) {
				ImGui::Text("- Render Target: %u -- %s",
					j, textureFormatToString(render.renderTargets[j]));
			}
			ImGui::Unindent(20.0f);

			constexpr f32 xOffset = 300.0f;

			// Print depth test
			ImGui::Spacing();
			alignedEdit("Depth function", xOffset, [&](const char*) {
				ImGui::Text("%s", compFuncToString(render.depthFunc));
			});

			// Print culling info
			ImGui::Spacing();
			alignedEdit("Culling", xOffset, [&](const char* name) {
				bool enabled = render.rasterizer.cullingEnabled != ZG_FALSE;
				if (ImGui::Checkbox(str128("##%s", name).str(), &enabled)) {
					render.rasterizer.cullingEnabled = enabled ? ZG_TRUE : ZG_FALSE;
				}
				ImGui::SameLine();
				ImGui::Text(" - %s", enabled ? "ENABLED" : "DISABLED");
			});
			if (render.rasterizer.cullingEnabled != ZG_FALSE) {
				ImGui::Indent(20.0f);
				ImGui::Text("Cull Front Face: %s", render.rasterizer.cullFrontFacing != ZG_FALSE ? "YES" : "NO");
				ImGui::Text("Front Facing Is Clockwise: %s", render.rasterizer.frontFacingIsClockwise != ZG_FALSE ? "YES" : "NO");
				ImGui::Unindent(20.0f);
			}

			// Print depth bias info
			ImGui::Spacing();
			ImGui::Text("Depth Bias");
			ImGui::Indent(20.0f);
			alignedEdit("Bias", xOffset, [&](const char* name) {
				ImGui::SetNextItemWidth(165.0f);
				ImGui::InputInt(str128("%s##render_%u", name, idx), &render.rasterizer.depthBias);
			});
			alignedEdit("Bias Slope Scaled", xOffset, [&](const char* name) {
				ImGui::SetNextItemWidth(100.0f);
				ImGui::InputFloat(str128("%s##render_%u", name, idx), &render.rasterizer.depthBiasSlopeScaled, 0.0f, 0.0f, "%.4f");
			});
			alignedEdit("Bias Clamp", xOffset, [&](const char* name) {
				ImGui::SetNextItemWidth(100.0f);
				ImGui::InputFloat(str128("%s##render_%u", name, idx), &render.rasterizer.depthBiasClamp, 0.0f, 0.0f, "%.4f");
			});
			ImGui::Unindent(20.0f);

			// Print wireframe rendering mode
			ImGui::Spacing();
			alignedEdit("Wireframe Rendering", xOffset, [&](const char* name) {
				bool enabled = render.rasterizer.wireframeMode != ZG_FALSE;
				if (ImGui::Checkbox(str128("##%s", name).str(), &enabled)) {
					render.rasterizer.wireframeMode = enabled ? ZG_TRUE : ZG_FALSE;
				}
				ImGui::SameLine();
				ImGui::Text(" - %s", enabled ? "ENABLED" : "DISABLED");
			});

			// Print blend mode
			//ImGui::Spacing();
			//ImGui::Text("Blend Mode: %s", blendModeToString(render.blendMode));
		}

		ImGui::Spacing();
		ImGui::Unindent(20.0f);
	}

	ImGui::End();
}

} // namespace sfz
