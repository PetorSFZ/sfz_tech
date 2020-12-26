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

#include "sfz/resources/ResourceManagerState.hpp"

#include <imgui.h>

#include "sfz/renderer/RenderingEnumsToFromString.hpp"
#include "sfz/util/ImGuiHelpers.hpp"

namespace sfz {

// Helper functions
// ------------------------------------------------------------------------------------------------

inline void renderTexturesTab(ResourceManagerState& state)
{
	constexpr float offset = 150.0f;

	for (auto itemItr : state.textureHandles) {
		const TextureItem& item = state.textures[itemItr.value];

		ImGui::Text("\"%s\"", itemItr.key.str());
		if (!item.texture.valid()) {
			ImGui::SameLine();
			ImGui::Text("-- NOT VALID");
		}

		ImGui::Indent(20.0f);
		alignedEdit("Format", offset, [&](const char*) {
			ImGui::Text("%s", textureFormatToString(item.format));
		});
		alignedEdit("Resolution", offset, [&](const char*) {
			ImGui::Text("%u x %u", item.width, item.height);
		});
		alignedEdit("Mipmaps", offset, [&](const char*) {
			ImGui::Text("%u", item.numMipmaps);
		});

		ImGui::Unindent(20.0f);
		ImGui::Spacing();
	}
}

// ResourceManagerState
// ------------------------------------------------------------------------------------------------

inline void resourceManagerUI(ResourceManagerState& state)
{
	if (!ImGui::Begin("Resources", nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginTabBar("ResourcesTabBar", ImGuiTabBarFlags_None)) {

		if (ImGui::BeginTabItem("Textures")) {
			ImGui::Spacing();
			renderTexturesTab(state);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

} // namespace sfz
