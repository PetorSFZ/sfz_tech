// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include "sfz/renderer/RenderingEnumsToFromString.hpp"
#include "sfz/resources/ResourceManagerState.hpp"
#include "sfz/util/ImGuiHelpers.hpp"

namespace sfz {

// Helper functions
// ------------------------------------------------------------------------------------------------

inline void renderBuffersTab(SfzResourceManagerState& state, const SfzStrIDs* ids)
{
	constexpr f32 offset = 200.0f;
	constexpr f32x4 normalTextColor = f32x4(1.0f);
	constexpr f32x4 filterTextColor = f32x4(1.0f, 0.0f, 0.0f, 1.0f);
	static str128 filter;

	ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
	ImGui::InputText("Filter##BuffersTab", filter.mRawStr, filter.capacity());
	ImGui::PopStyleColor();
	filter.toLower();

	const bool filterMode = filter != "";

	for (HashMapPair<SfzStrID, SfzHandle> itemItr : state.bufferHandles) {
		const char* name = sfzStrIDGetStr(ids, itemItr.key);
		const SfzBufferResource& resource = state.buffers[itemItr.value];

		str320 lowerCaseName = name;
		lowerCaseName.toLower();
		if (!filter.isPartOf(lowerCaseName.str())) continue;

		if (filterMode) {
			imguiRenderFilteredText(name, filter.str(), normalTextColor, filterTextColor);
		}
		else {
			if (!ImGui::CollapsingHeader(name)) continue;
		}

		ImGui::Indent(20.0f);
		alignedEdit("Type", offset, [&](const char*) {
			ImGui::Text("%s", resource.type == SfzBufferResourceType::STATIC ? "STATIC" : "STREAMING");
		});
		alignedEdit("Size", offset, [&](const char*) {
			const u32 numElements = resource.maxNumElements;
			const u32 elementSize = resource.elementSizeBytes;
			const u32 numBytes = elementSize * numElements;
			f32 scaledSize = 0.0f;
			const char* ending = "";
			if (numBytes < 1024) {
				scaledSize = f32(numBytes);
				ending = "bytes";
			}
			else if (numBytes < (1024 * 1024)) {
				scaledSize = f32(numBytes) / 1024.0f;
				ending = "KiB";
			}
			else {
				scaledSize = f32(numBytes) / (1024.0f * 1024.0f);
				ending = "MiB";
			}
			ImGui::Text("%u elements x %u bytes = %.2f %s",
				numElements, elementSize, scaledSize, ending);
		});
		ImGui::Unindent(20.0f);
	}
}

inline void renderTexturesTab(SfzResourceManagerState& state, const SfzStrIDs* ids)
{
	constexpr f32 offset = 200.0f;
	constexpr f32 offset2 = 240.0f;
	constexpr f32x4 normalTextColor = f32x4(1.0f);
	constexpr f32x4 filterTextColor = f32x4(1.0f, 0.0f, 0.0f, 1.0f);
	static str128 filter;

	ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
	ImGui::InputText("Filter##TexturesTab", filter.mRawStr, filter.capacity());
	ImGui::PopStyleColor();
	filter.toLower();

	const bool filterMode = filter != "";

	for (HashMapPair<SfzStrID, SfzHandle> itemItr : state.textureHandles) {
		const char* name = sfzStrIDGetStr(ids, itemItr.key);
		const SfzTextureResource& resource = state.textures[itemItr.value];

		str320 lowerCaseName = name;
		lowerCaseName.toLower();
		if (!filter.isPartOf(lowerCaseName.str())) continue;

		if (filterMode) {
			imguiRenderFilteredText(name, filter.str(), normalTextColor, filterTextColor);
		}
		else {
			if (!ImGui::CollapsingHeader(name)) continue;
		}

		ImGui::Indent(20.0f);

		alignedEdit("Format", offset, [&](const char*) {
			ImGui::Text("%s", textureFormatToString(resource.format));
		});
		alignedEdit("Resolution", offset, [&](const char*) {
			ImGui::Text("%u x %u", resource.res.x, resource.res.y);
		});
		alignedEdit("Mipmaps", offset, [&](const char*) {
			ImGui::Text("%u", resource.numMipmaps);
		});
		alignedEdit("Committed alloc", offset, [&](const char*) {
			ImGui::Text("%s", resource.committedAllocation ? "TRUE" : "FALSE");
		});
		
		if (resource.usage != ZG_TEXTURE_USAGE_DEFAULT) {
			alignedEdit("Usage", offset, [&](const char*) {
				ImGui::Text("%s", usageToString(resource.usage));
			});
			alignedEdit("Clear value", offset, [&](const char*) {
				ImGui::Text("%s", clearValueToString(resource.optimalClearValue));
			});
		}

		if (resource.screenRelativeRes) {
			ImGui::Text("Screen relative resolution");
			ImGui::Indent(20.0f);
			alignedEdit("Fixed scale", offset2, [&](const char*) {
				ImGui::Text("%.2f", resource.resScale);
			});
			alignedEdit("Scale setting", offset2, [&](const char*) {
				ImGui::Text("%s.%s",
					resource.resScaleSetting->section().str(),
					resource.resScaleSetting->key().str());
			});
			if (resource.resScaleSettingScale != 1.0f) {
				alignedEdit("Scale setting scale", offset2, [&](const char*) {
					ImGui::Text("%.2f", resource.resScaleSettingScale);
				});
			}
			ImGui::Unindent(20.0f);
		}

		if (resource.settingControlledRes) {
			ImGui::Text("Setting controlled resolution");
			ImGui::Indent(20.0f);
			alignedEdit("Res setting", offset2, [&](const char*) {
				ImGui::Text("%s.%s",
					resource.controlledResSetting->section().str(),
					resource.controlledResSetting->key().str());
			});
			ImGui::Unindent(20.0f);
		}

		ImGui::Unindent(20.0f);
		ImGui::Spacing();
	}
}

inline void renderFramebuffersTab(SfzResourceManagerState& state, const SfzStrIDs* ids)
{
	constexpr f32 offset = 200.0f;
	constexpr f32 offset2 = 220.0f;
	constexpr f32x4 normalTextColor = f32x4(1.0f);
	constexpr f32x4 filterTextColor = f32x4(1.0f, 0.0f, 0.0f, 1.0f);
	static str128 filter;

	ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
	ImGui::InputText("Filter##FramebuffersTab", filter.mRawStr, filter.capacity());
	ImGui::PopStyleColor();
	filter.toLower();

	const bool filterMode = filter != "";

	for (HashMapPair<SfzStrID, SfzHandle> itemItr : state.framebufferHandles) {
		const char* name = sfzStrIDGetStr(ids, itemItr.key);
		const SfzFramebufferResource& resource = state.framebuffers[itemItr.value];

		str320 lowerCaseName = name;
		lowerCaseName.toLower();
		if (!filter.isPartOf(lowerCaseName.str())) continue;

		if (filterMode) {
			imguiRenderFilteredText(name, filter.str(), normalTextColor, filterTextColor);
		}
		else {
			if (!ImGui::CollapsingHeader(name)) continue;
		}

		ImGui::Indent(20.0f);

		alignedEdit("Resolution", offset, [&](const char*) {
			ImGui::Text("%u x %u", resource.res.x, resource.res.y);
		});

		if (resource.screenRelativeRes) {
			ImGui::Text("Screen relative resolution");
			ImGui::Indent(20.0f);
			alignedEdit("Fixed scale", offset2, [&](const char*) {
				ImGui::Text("%.2f", resource.resScale);
			});
			alignedEdit("Scale setting", offset2, [&](const char*) {
				ImGui::Text("%s.%s",
					resource.resScaleSetting->section().str(),
					resource.resScaleSetting->key().str());
			});
			ImGui::Unindent(20.0f);
		}

		if (resource.settingControlledRes) {
			ImGui::Text("Setting controlled resolution");
			ImGui::Indent(20.0f);
			alignedEdit("Res setting", offset2, [&](const char*) {
				ImGui::Text("%s.%s",
					resource.controlledResSetting->section().str(),
					resource.controlledResSetting->key().str());
			});
			ImGui::Unindent(20.0f);
		}

		if (!resource.renderTargetNames.isEmpty()) {
			ImGui::Spacing();
			for (u32 i = 0; i < resource.renderTargetNames.size(); i++) {
				SfzStrID renderTargetName = resource.renderTargetNames[i];
				const SfzTextureResource* renderTarget =
					state.textures.get(*state.textureHandles.get(renderTargetName));
				sfz_assert(renderTarget != nullptr);
				alignedEdit(str64("Render target %u", i).str(), offset, [&](const char*) {
					ImGui::Text("%s  --  %s", sfzStrIDGetStr(ids, renderTargetName), textureFormatToString(renderTarget->format));
				});
			}
		}
		
		if (resource.depthBufferName != SFZ_STR_ID_NULL) {
			ImGui::Spacing();
			alignedEdit("Depth buffer", offset, [&](const char*) {
				ImGui::Text("%s", sfzStrIDGetStr(ids, resource.depthBufferName));
			});
		}

		ImGui::Unindent(20.0f);
		ImGui::Spacing();
		ImGui::Spacing();
	}
}

// ResourceManagerUI
// ------------------------------------------------------------------------------------------------

inline void resourceManagerUI(SfzResourceManagerState& state, const SfzStrIDs* ids)
{
	if (!ImGui::Begin("Res", nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginTabBar("ResourcesTabBar", ImGuiTabBarFlags_None)) {

		if (ImGui::BeginTabItem("Buffers")) {
			ImGui::Spacing();
			renderBuffersTab(state, ids);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Textures")) {
			ImGui::Spacing();
			renderTexturesTab(state, ids);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Framebuffers")) {
			ImGui::Spacing();
			renderFramebuffersTab(state, ids);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

} // namespace sfz
