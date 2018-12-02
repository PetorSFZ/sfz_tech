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

#include "ph/ecs/naive/NaiveEcsEditor.hpp"

#include <algorithm>

#include <imgui.h>

#include <sfz/strings/StackString.hpp>

#include "ph/ecs/EcsEnums.hpp"

namespace ph {

using sfz::str32;

// Static functions
// ------------------------------------------------------------------------------------------------

// Edits a component mask using a checkbox for each bit, returns whether mask was modified or not
static bool componentMaskEditor(const char* identifier, ComponentMask& mask) noexcept
{
	const int32_t NUM_COLS = 16;
	const int32_t NUM_ROWS = 4;
	static_assert(NUM_COLS * NUM_ROWS == 64, "Think again");

	bool bitsModified = false;

	for (int32_t offs = 0; offs < NUM_ROWS; offs++) {
		for (int32_t i = NUM_COLS - 1; i >= 0; i--) {

			uint32_t bitIndex = uint32_t(i + offs * NUM_COLS);

			ImGui::BeginGroup();
			ImGui::Text("%s", str32("%02u", bitIndex).str);

			bool b = mask.hasComponentType(bitIndex);
			if (ImGui::Checkbox(str32("##%s_%u", identifier, bitIndex), &b)) {
				bitsModified = true;
				mask.setComponentType(bitIndex, b);
			}

			ImGui::EndGroup();
			if (i != 0) ImGui::SameLine();
		}
	}

	return bitsModified;
}

// NaiveEcsEditor: State methods
// ------------------------------------------------------------------------------------------------

void NaiveEcsEditor::init(ComponentInfo* componentInfos, uint32_t numComponentInfos)
{
	this->destroy();

	// Set active bit component info
	mComponentInfos[0].componentName.printf("00 - Active bit");

	// Set rest of component infos
	for (uint32_t compType = 1; compType < 64; compType++) {

		// See if the current component type is specified in the input list of component infos
		uint32_t componentInfoIndex = ~0u;
		for (uint32_t i = 0; i < numComponentInfos; i++) {
			if (componentInfos[i].componentType == compType) {
				componentInfoIndex = i;
				break;
			}
		}

		// If available, steal it!
		if (componentInfoIndex != ~0u) {
			ComponentInfo& info = componentInfos[componentInfoIndex];
			ReducedComponentInfo& target = mComponentInfos[compType];

			target.componentName.printf("%02u -- %s", compType, info.componentName.str);
			target.componentEditor = info.componentEditor;
			target.editorState = std::move(info.editorState);
		}

		// Otherwise, create default
		else {
			mComponentInfos[compType].componentName.printf("%02u -- <unnamed>", compType);
		}
	}
}

void NaiveEcsEditor::swap(NaiveEcsEditor& other) noexcept
{
	for (uint32_t i = 0; i < 64; i++) {
		std::swap(this->mComponentInfos[i], other.mComponentInfos[i]);
	}
	std::swap(this->mFilterMask, other.mFilterMask);
	std::swap(this->mCurrentSelectedEntity, other.mCurrentSelectedEntity);
}

void NaiveEcsEditor::destroy() noexcept
{
	for (uint32_t i = 0; i < 64; i++) {
		this->mComponentInfos[i] = ReducedComponentInfo();
	}
	mFilterMask = ComponentMask::activeMask();
	mCurrentSelectedEntity = 0;
}

// NaiveEcsEditor: Methods
// ------------------------------------------------------------------------------------------------

void NaiveEcsEditor::render(NaiveEcsHeader* ecs) noexcept
{
	// Begin window
	ImGui::SetNextWindowContentSize(sfz::vec2(550.0f, 480.0f));
	ImGuiWindowFlags windowFlags = 0;
	//windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoScrollbar;
	ImGui::Begin("Naive ECS Editor", nullptr, windowFlags);

	// End window and return if no ECS system
	if (ecs == nullptr) {
		ImGui::Text("<none>");
		ImGui::End();
		return;
	}

	// End window and return if not a naive ECS system
	if (ecs->ECS_TYPE != ECS_TYPE_NAIVE) {
		ImGui::Text("<none> (Not a naive ECS system)");
		ImGui::End();
		return;
	}

	// Get some stuff from the ECS system
	ComponentMask* masks = ecs->componentMasks();

	// Print size of ECS system in bytes
	if (ecs->ecsSizeBytes < 1048576) {
		ImGui::Text("Size: %.2f KiB", float(ecs->ecsSizeBytes) / 1024.0f);
	}
	else {
		ImGui::Text("Size: %.2f MiB", float(ecs->ecsSizeBytes) / (1024.0f * 1024.0f));
	}

	// Print current number and max number of entities
	ImGui::SameLine();
	uint32_t numEntities = ecs->currentNumEntities;
	ImGui::Text(" --  %u / %u entities", numEntities, ecs->maxNumEntities);

	// Currently selected entities component mask
	if (ImGui::CollapsingHeader("Component mask filter")) {
		componentMaskEditor("FilterMaskBit", mFilterMask);
	}

	// Spacing and separator between the different type of views
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Entities column
	ImGui::PushItemWidth(100);
	ImGui::BeginGroup();

	// Entities list
	if (ImGui::ListBoxHeader("##Entities", ecs->maxNumEntities, 20)) {
		for (uint32_t entity = 0; entity < ecs->maxNumEntities; entity++) {

			// Only show entitites that fulfill filter mask
			if (!masks[entity].fulfills(mFilterMask)) continue;

			// Non-active entites are greyed out
			bool active = masks[entity].active();
			if (!active) ImGui::PushStyleColor(ImGuiCol_Text, sfz::vec4(0.35f, 0.35f, 0.35f, 1.0f));

			str32 entityStr("%08u", entity);
			bool selected = mCurrentSelectedEntity == entity;
			bool activated = ImGui::Selectable(entityStr, selected);
			if (activated) mCurrentSelectedEntity = entity;

			// Non-active entites are greyed out
			if (!active) ImGui::PopStyleColor();

		}
		ImGui::ListBoxFooter();
	}

	// New entity button
	if (ImGui::Button("New", sfz::vec2(100, 0))) {
		ecs->createEntity();
	}

	// Clone entity button
	if (ImGui::Button("Clone", sfz::vec2(100, 0))) {
		ecs->cloneEntity(mCurrentSelectedEntity);
	}

	// Delete entity button
	if (ImGui::Button("Delete", sfz::vec2(100, 0))) {
		ecs->deleteEntity(mCurrentSelectedEntity);
	}

	// End entities column
	ImGui::EndGroup();
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::BeginGroup();

	// Only show entity edit menu if an active entity is selected
	if (masks[mCurrentSelectedEntity].active()) {

		// Currently selected entities component mask
		if (ImGui::CollapsingHeader("Component Mask")) {
			ComponentMask mask = masks[mCurrentSelectedEntity];
			if (componentMaskEditor("EntityMask", mask)) {
				masks[mCurrentSelectedEntity] = mask;
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Component edit menu
		if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BeginChild("ComponentsChild");

			for (uint32_t i = 0; i < 64; i++) {

				// Skip component type if it does not have data
				uint32_t componentSize = 0;
				uint8_t* components = ecs->componentsUntyped(i, componentSize);
				if (components == nullptr) continue;

				// Skip component type if entity does not have it
				ComponentMask mask = masks[mCurrentSelectedEntity];
				const ReducedComponentInfo& info = mComponentInfos[i];
				if (!mask.hasComponentType(i)) continue;

				// Component editor
				if (ImGui::CollapsingHeader(info.componentName)) {
					if(info.componentEditor == nullptr) {
						ImGui::Text("<No editor specified>");
					}
					else {
						info.componentEditor(
							info.editorState.get(),
							components + mCurrentSelectedEntity * componentSize,
							ecs,
							mCurrentSelectedEntity);
					}
				}
			}

			ImGui::EndChild();
		}
	}

	ImGui::EndGroup();
	ImGui::End();
}

} // namespace ph
