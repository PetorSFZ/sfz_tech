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

	constexpr float offset = 250.0f;
	alignedEdit("Config path", offset, [&](const char*) {
		ImGui::Text("\"%s\"", state.configPath.str());
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


	ZgFeatureSupport features = {};
	CHECK_ZG zgContextGetFeatureSupport(&features);

	// Print ZeroG  feature support
	ImGui::Text("ZeroG Feature Support");
	ImGui::Spacing();
	ImGui::Indent(20.0f);

	constexpr float featuresOffset = 320.0f;
	alignedEdit("Device", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.deviceDescription);
	});

	alignedEdit("Shader model", featuresOffset, [&](const char*) {
		const char* model = [](ZgShaderModel shaderModel) {
			switch (shaderModel) {
			case ZG_SHADER_MODEL_6_0: return "6.0";
			case ZG_SHADER_MODEL_6_1: return "6.1";
			case ZG_SHADER_MODEL_6_2: return "6.2";
			case ZG_SHADER_MODEL_6_3: return "6.3";
			case ZG_SHADER_MODEL_6_4: return "6.4";
			case ZG_SHADER_MODEL_6_5: return "6.5";
			case ZG_SHADER_MODEL_6_6: return "6.6";
			}
			return "UNDEFINED";
		}(features.shaderModel);
		
		ImGui::TextUnformatted(model);
	});

	alignedEdit("Resource binding tier", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.resourceBindingTier);
	});

	alignedEdit("Resource heap tier", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.resourceHeapTier);
	});

	alignedEdit("Shader dynamic resources support", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.shaderDynamicResources != ZG_FALSE ? "True" : "False");
	});

	alignedEdit("Wave ops support", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.waveOps != ZG_FALSE ? "True" : "False");
	});

	alignedEdit("Wave lane count", featuresOffset, [&](const char*) {
		ImGui::Text("%u min, %u max", features.waveMinLaneCount, features.waveMaxLaneCount);
	});

	alignedEdit("GPU total lanes/threads count", featuresOffset, [&](const char*) {
		ImGui::Text("%u", features.gpuTotalLaneCount);
	});

	alignedEdit("Shader 16-bit ops support", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.shader16bitOps != ZG_FALSE ? "True" : "False");
	});

	alignedEdit("Raytracing support", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.raytracing != ZG_FALSE ? "True" : "False");
	});

	alignedEdit("Raytracing tier", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.raytracingTier);
	});

	alignedEdit("Variable shading rate support", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.variableShadingRate != ZG_FALSE ? "True" : "False");
	});

	alignedEdit("Variable shading rate tier", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.variableShadingRateTier);
	});

	alignedEdit("Variable shading rate tile size", featuresOffset, [&](const char*) {
		ImGui::Text("%ux%u", features.variableShadingRateTileSize, features.variableShadingRateTileSize);
	});

	alignedEdit("Mesh shaders support", featuresOffset, [&](const char*) {
		ImGui::TextUnformatted(features.meshShaders != ZG_FALSE ? "True" : "False");
	});

	ImGui::Unindent(20.0f);
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

	ImGui::End();
}

} // namespace sfz
