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
#include "sfz/resources/TextureResource.hpp"
#include "sfz/util/ImGuiHelpers.hpp"

namespace sfz {

// Helper functions
// ------------------------------------------------------------------------------------------------

inline void renderBuffersTab(ResourceManagerState& state)
{
	constexpr float offset = 200.0f;
	constexpr float offset2 = 220.0f;
	constexpr vec4 normalTextColor = vec4(1.0f);
	constexpr vec4 filterTextColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	static str128 filter;

	ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
	ImGui::InputText("Filter##BuffersTab", filter.mRawStr, filter.capacity());
	ImGui::PopStyleColor();
	filter.toLower();

	const bool filterMode = filter != "";

	for (HashMapPair<strID, PoolHandle> itemItr : state.bufferHandles) {
		const char* name = itemItr.key.str();
		const BufferResource& resource = state.buffers[itemItr.value];

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
			ImGui::Text("%s", resource.type == BufferResourceType::STATIC ? "STATIC" : "STREAMING");
		});
		alignedEdit("Element size", offset, [&](const char*) {
			ImGui::Text("%u bytes", resource.elementSizeBytes);
		});
		alignedEdit("Max num elements", offset, [&](const char*) {
			ImGui::Text("%u", resource.maxNumElements);
		});
		ImGui::Unindent(20.0f);
	}
}

inline void renderTexturesTab(ResourceManagerState& state)
{
	constexpr float offset = 200.0f;
	constexpr float offset2 = 220.0f;
	constexpr vec4 normalTextColor = vec4(1.0f);
	constexpr vec4 filterTextColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	static str128 filter;

	ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
	ImGui::InputText("Filter##TexturesTab", filter.mRawStr, filter.capacity());
	ImGui::PopStyleColor();
	filter.toLower();

	const bool filterMode = filter != "";

	for (HashMapPair<strID, PoolHandle> itemItr : state.textureHandles) {
		const char* name = itemItr.key.str();
		const TextureResource& resource = state.textures[itemItr.value];

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

		if (resource.screenRelativeResolution) {
			ImGui::Text("Screen relative resolution");
			ImGui::Indent(20.0f);
			alignedEdit("Fixed scale", offset2, [&](const char*) {
				ImGui::Text("%.2f", resource.resolutionScale);
			});
			alignedEdit("Scale setting", offset2, [&](const char*) {
				ImGui::Text("%s.%s",
					resource.resolutionScaleSetting->section().str(),
					resource.resolutionScaleSetting->key().str());
			});
			ImGui::Unindent(20.0f);
		}

		ImGui::Unindent(20.0f);
		ImGui::Spacing();
	}
}

inline void renderFramebuffersTab(ResourceManagerState& state)
{
	constexpr float offset = 200.0f;
	constexpr float offset2 = 220.0f;
	constexpr vec4 normalTextColor = vec4(1.0f);
	constexpr vec4 filterTextColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	static str128 filter;

	ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
	ImGui::InputText("Filter##FramebuffersTab", filter.mRawStr, filter.capacity());
	ImGui::PopStyleColor();
	filter.toLower();

	const bool filterMode = filter != "";

	for (HashMapPair<strID, PoolHandle> itemItr : state.framebufferHandles) {
		const char* name = itemItr.key.str();
		const FramebufferResource& resource = state.framebuffers[itemItr.value];

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

		if (resource.screenRelativeResolution) {
			ImGui::Text("Screen relative resolution");
			ImGui::Indent(20.0f);
			alignedEdit("Fixed scale", offset2, [&](const char*) {
				ImGui::Text("%.2f", resource.resolutionScale);
			});
			alignedEdit("Scale setting", offset2, [&](const char*) {
				ImGui::Text("%s.%s",
					resource.resolutionScaleSetting->section().str(),
					resource.resolutionScaleSetting->key().str());
			});
			ImGui::Unindent(20.0f);
		}

		if (!resource.renderTargetNames.isEmpty()) {
			ImGui::Spacing();
			for (uint32_t i = 0; i < resource.renderTargetNames.size(); i++) {
				strID renderTargetName = resource.renderTargetNames[i];
				const TextureResource* renderTarget =
					state.textures.get(*state.textureHandles.get(renderTargetName));
				sfz_assert(renderTarget != nullptr);
				alignedEdit(str64("Render target %u", i).str(), offset, [&](const char*) {
					ImGui::Text("%s  --  %s", renderTargetName.str(), textureFormatToString(renderTarget->format));
				});
			}
		}
		
		if (resource.depthBufferName.isValid()) {
			ImGui::Spacing();
			alignedEdit("Depth buffer", offset, [&](const char*) {
				ImGui::Text("%s", resource.depthBufferName.str());
			});
		}

		ImGui::Unindent(20.0f);
		ImGui::Spacing();
		ImGui::Spacing();
	}
}

inline void renderMeshesTab(ResourceManagerState& state)
{
	for (auto itemItr : state.meshHandles) {
		MeshResource& mesh = state.meshes[itemItr.value];

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
			if (ImGui::BeginCombo(comboName, selectedTexStr)) {

				// Special case for no texture (~0)
				{
					bool isSelected = !texId.isValid();
					if (ImGui::Selectable("NO TEXTURE", isSelected)) {
						texId = STR_ID_INVALID;
						updateMesh = true;
					}
				}

				// Existing textures
				for (auto itemItr : state.textureHandles) {
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
			}
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

					zg::CommandQueue presentQueue = zg::CommandQueue::getPresentQueue();
					zg::CommandQueue copyQueue = zg::CommandQueue::getCopyQueue();

					// Flush ZeroG queues
					CHECK_ZG presentQueue.flush();
					CHECK_ZG copyQueue.flush();

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
					CHECK_ZG presentQueue.beginCommandListRecording(commandList);
					uint64_t dstOffset = sizeof(ShaderMaterial) * i;
					CHECK_ZG commandList.memcpyBufferToBuffer(
						mesh.materialsBuffer, dstOffset, uploadBuffer, 0, sizeof(ShaderMaterial));
					CHECK_ZG presentQueue.executeCommandList(commandList);
					CHECK_ZG presentQueue.flush();
				}
			}
			ImGui::Unindent(20.0f);
		}
		ImGui::Unindent(20.0f);

		ImGui::Spacing();
	}
}

// ResourceManagerUI
// ------------------------------------------------------------------------------------------------

inline void resourceManagerUI(ResourceManagerState& state)
{
	if (!ImGui::Begin("Resources", nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginTabBar("ResourcesTabBar", ImGuiTabBarFlags_None)) {

		if (ImGui::BeginTabItem("Buffers")) {
			ImGui::Spacing();
			renderBuffersTab(state);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Textures")) {
			ImGui::Spacing();
			renderTexturesTab(state);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Framebuffers")) {
			ImGui::Spacing();
			renderFramebuffersTab(state);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Meshes")) {
			ImGui::Spacing();
			renderMeshesTab(state);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

} // namespace sfz
