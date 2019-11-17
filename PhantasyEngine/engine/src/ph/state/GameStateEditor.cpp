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

#include "ph/state/GameStateEditor.hpp"

#include <algorithm>
#include <cinttypes>

#include <imgui.h>
#include <imgui_internal.h>

#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
#include <nfd.h>
#endif

#include <sfz/containers/HashMap.hpp>
#include <sfz/strings/StringHashers.hpp>
#include <sfz/Logging.hpp>
#include <sfz/strings/StackString.hpp>
#include <sfz/util/IO.hpp>

namespace ph {

using sfz::vec2;

// Static functions
// ------------------------------------------------------------------------------------------------

static const char* byteToBinaryStringLookupTable[256] = {
	"00000000",
	"00000001",
	"00000010",
	"00000011",
	"00000100",
	"00000101",
	"00000110",
	"00000111",
	"00001000",
	"00001001",
	"00001010",
	"00001011",
	"00001100",
	"00001101",
	"00001110",
	"00001111",
	"00010000",
	"00010001",
	"00010010",
	"00010011",
	"00010100",
	"00010101",
	"00010110",
	"00010111",
	"00011000",
	"00011001",
	"00011010",
	"00011011",
	"00011100",
	"00011101",
	"00011110",
	"00011111",
	"00100000",
	"00100001",
	"00100010",
	"00100011",
	"00100100",
	"00100101",
	"00100110",
	"00100111",
	"00101000",
	"00101001",
	"00101010",
	"00101011",
	"00101100",
	"00101101",
	"00101110",
	"00101111",
	"00110000",
	"00110001",
	"00110010",
	"00110011",
	"00110100",
	"00110101",
	"00110110",
	"00110111",
	"00111000",
	"00111001",
	"00111010",
	"00111011",
	"00111100",
	"00111101",
	"00111110",
	"00111111",
	"01000000",
	"01000001",
	"01000010",
	"01000011",
	"01000100",
	"01000101",
	"01000110",
	"01000111",
	"01001000",
	"01001001",
	"01001010",
	"01001011",
	"01001100",
	"01001101",
	"01001110",
	"01001111",
	"01010000",
	"01010001",
	"01010010",
	"01010011",
	"01010100",
	"01010101",
	"01010110",
	"01010111",
	"01011000",
	"01011001",
	"01011010",
	"01011011",
	"01011100",
	"01011101",
	"01011110",
	"01011111",
	"01100000",
	"01100001",
	"01100010",
	"01100011",
	"01100100",
	"01100101",
	"01100110",
	"01100111",
	"01101000",
	"01101001",
	"01101010",
	"01101011",
	"01101100",
	"01101101",
	"01101110",
	"01101111",
	"01110000",
	"01110001",
	"01110010",
	"01110011",
	"01110100",
	"01110101",
	"01110110",
	"01110111",
	"01111000",
	"01111001",
	"01111010",
	"01111011",
	"01111100",
	"01111101",
	"01111110",
	"01111111",
	"10000000",
	"10000001",
	"10000010",
	"10000011",
	"10000100",
	"10000101",
	"10000110",
	"10000111",
	"10001000",
	"10001001",
	"10001010",
	"10001011",
	"10001100",
	"10001101",
	"10001110",
	"10001111",
	"10010000",
	"10010001",
	"10010010",
	"10010011",
	"10010100",
	"10010101",
	"10010110",
	"10010111",
	"10011000",
	"10011001",
	"10011010",
	"10011011",
	"10011100",
	"10011101",
	"10011110",
	"10011111",
	"10100000",
	"10100001",
	"10100010",
	"10100011",
	"10100100",
	"10100101",
	"10100110",
	"10100111",
	"10101000",
	"10101001",
	"10101010",
	"10101011",
	"10101100",
	"10101101",
	"10101110",
	"10101111",
	"10110000",
	"10110001",
	"10110010",
	"10110011",
	"10110100",
	"10110101",
	"10110110",
	"10110111",
	"10111000",
	"10111001",
	"10111010",
	"10111011",
	"10111100",
	"10111101",
	"10111110",
	"10111111",
	"11000000",
	"11000001",
	"11000010",
	"11000011",
	"11000100",
	"11000101",
	"11000110",
	"11000111",
	"11001000",
	"11001001",
	"11001010",
	"11001011",
	"11001100",
	"11001101",
	"11001110",
	"11001111",
	"11010000",
	"11010001",
	"11010010",
	"11010011",
	"11010100",
	"11010101",
	"11010110",
	"11010111",
	"11011000",
	"11011001",
	"11011010",
	"11011011",
	"11011100",
	"11011101",
	"11011110",
	"11011111",
	"11100000",
	"11100001",
	"11100010",
	"11100011",
	"11100100",
	"11100101",
	"11100110",
	"11100111",
	"11101000",
	"11101001",
	"11101010",
	"11101011",
	"11101100",
	"11101101",
	"11101110",
	"11101111",
	"11110000",
	"11110001",
	"11110010",
	"11110011",
	"11110100",
	"11110101",
	"11110110",
	"11110111",
	"11111000",
	"11111001",
	"11111010",
	"11111011",
	"11111100",
	"11111101",
	"11111110",
	"11111111",
};

static sfz::HashMap<str32, uint8_t>* binaryStringToByteLookupMap = nullptr;

static const char* byteToBinaryString(uint8_t byte) noexcept
{
	return byteToBinaryStringLookupTable[byte];
}

static uint8_t binaryStringToByte(const char* binaryStr) noexcept
{
	const uint8_t* bytePtr = binaryStringToByteLookupMap->get(binaryStr);
	if (bytePtr == nullptr) return 0;
	return *bytePtr;
}

static void initializeComponentMaskEditor(str32 buffers[8], ComponentMask initialMask) noexcept
{
	for (uint64_t i = 0; i < 8; i++) {
		uint8_t byte = uint8_t((initialMask.rawMask >> (i * 8)) & uint64_t(0xFF));
		const char* byteBinaryStr = byteToBinaryString(byte);
		buffers[i].printf("%s", byteBinaryStr);
	}
}

static int imguiOnlyBinaryFilter(ImGuiInputTextCallbackData* data) noexcept
{
	bool isBinaryChar = data->EventChar == '0' || data->EventChar == '1';
	return isBinaryChar ? 0 : 1;
}

static void componentMaskVisualizer(ComponentMask mask) noexcept
{
	const int32_t NUM_BITS_PER_FIELD = 8;
	const int32_t NUM_FIELDS = 4;
	const int32_t NUM_ROWS = 2;
	static_assert(NUM_BITS_PER_FIELD * NUM_FIELDS * NUM_ROWS == 64, "Think again");

	for (int32_t rowIdx = 0; rowIdx < NUM_ROWS; rowIdx++) {
		for (int32_t fieldIdx = NUM_FIELDS - 1; fieldIdx >= 0; fieldIdx--) {

			int32_t byteIdx = rowIdx * NUM_FIELDS + fieldIdx;
			uint8_t byte = uint8_t((mask.rawMask >> (byteIdx * 8)) & uint64_t(0xFF));
			const char* byteBinaryStr = byteToBinaryString(byte);

			ImGui::Text("%s", byteBinaryStr);
			ImGui::SameLine();
		}

		if(rowIdx == 0) {
			uint32_t byte0 = uint32_t(mask.rawMask & 0xFF);
			uint32_t byte1 = uint32_t((mask.rawMask >> 8) & 0xFF);
			uint32_t byte2 = uint32_t((mask.rawMask >> 16) & 0xFF);
			uint32_t byte3 = uint32_t((mask.rawMask >> 24) & 0xFF);
			ImGui::Text("[%02X %02X %02X %02X]", byte3, byte2, byte1, byte0);
		}
		else {
			uint32_t byte4 = uint32_t((mask.rawMask >> 32) & 0xFF);
			uint32_t byte5 = uint32_t((mask.rawMask >> 40) & 0xFF);
			uint32_t byte6 = uint32_t((mask.rawMask >> 48) & 0xFF);
			uint32_t byte7 = uint32_t((mask.rawMask >> 56) & 0xFF);
			ImGui::Text("[%02X %02X %02X %02X]", byte7, byte6, byte5, byte4);
		}
	}
}

static bool componentMaskEditor(
	const char* identifier,
	str32 buffers[8],
	ComponentMask& mask) noexcept
{
	const int32_t NUM_BITS_PER_FIELD = 8;
	const int32_t NUM_FIELDS = 4;
	const int32_t NUM_ROWS = 2;
	static_assert(NUM_BITS_PER_FIELD * NUM_FIELDS * NUM_ROWS == 64, "Think again");

	bool bitsModified = false;

	for (int32_t rowIdx = 0; rowIdx < NUM_ROWS; rowIdx++) {
		for (int32_t fieldIdx = NUM_FIELDS - 1; fieldIdx >= 0; fieldIdx--) {

			int32_t byteIdx = rowIdx * NUM_FIELDS + fieldIdx;
			uint64_t shiftOffset = 8 * byteIdx;

			ImGuiInputTextFlags inputFlags = 0;
			inputFlags |= ImGuiInputTextFlags_EnterReturnsTrue;
			inputFlags |= ImGuiInputTextFlags_CallbackCharFilter;

			ImGui::PushItemWidth(85.0f);
			bool modified = ImGui::InputText(
				str32("##%s_%u", identifier, byteIdx),
				buffers[byteIdx],
				9, // We only allow 8 characters (bits) per byte
				inputFlags,
				imguiOnlyBinaryFilter);
			ImGui::PopItemWidth();

			if (modified) {
				const uint8_t modifiedByte = binaryStringToByte(buffers[byteIdx]);
				const uint64_t keepMask = ~(uint64_t(0xFF) << shiftOffset);
				const uint64_t insertByte = uint64_t(modifiedByte) << shiftOffset;
				const uint64_t modifiedMask = (mask.rawMask & keepMask) | insertByte;
				mask.rawMask = modifiedMask;
				bitsModified = true;
			}

			ImGui::SameLine();
		}

		if(rowIdx == 0) {
			uint32_t byte0 = uint32_t(mask.rawMask & 0xFF);
			uint32_t byte1 = uint32_t((mask.rawMask >> 8) & 0xFF);
			uint32_t byte2 = uint32_t((mask.rawMask >> 16) & 0xFF);
			uint32_t byte3 = uint32_t((mask.rawMask >> 24) & 0xFF);
			ImGui::Text("[%02X %02X %02X %02X]", byte3, byte2, byte1, byte0);
		}
		else {
			uint32_t byte4 = uint32_t((mask.rawMask >> 32) & 0xFF);
			uint32_t byte5 = uint32_t((mask.rawMask >> 40) & 0xFF);
			uint32_t byte6 = uint32_t((mask.rawMask >> 48) & 0xFF);
			uint32_t byte7 = uint32_t((mask.rawMask >> 56) & 0xFF);
			ImGui::Text("[%02X %02X %02X %02X]", byte7, byte6, byte5, byte4);
		}
	}

	return bitsModified;
}

#ifndef __EMSCRIPTEN__
static void saveDialog(const GameStateHeader* state) noexcept
{
	// Open file dialog
	nfdchar_t* path = nullptr;
	nfdresult_t result = NFD_SaveDialog("phstate", nullptr, &path);

	// Write game state to file if file dialog was succesful
	if (result == NFD_OKAY) {
		bool success =  sfz::writeBinaryFile(path, (const uint8_t*)state, state->stateSizeBytes);
		if (success) {
			SFZ_INFO("PhantasyEngine", "Wrote game state to \"%s\"", path);
		}
		else {
			SFZ_ERROR("PhantasyEngine", "Failed to write game state to \"%s\"", path);
		}
		free(path);
	}
	else if (result == NFD_ERROR) {
		SFZ_ERROR("PhantasyEngine", "nativefiledialog: NFD_SaveDialog() error: %s",
			NFD_GetError());
	}
}

static void loadDialog(GameStateHeader* state) noexcept
{
	// Open file dialog
	nfdchar_t* path = nullptr;
	nfdresult_t result = NFD_OpenDialog("phstate", nullptr, &path);

	// Load game state from file if file dialog was succesful
	if (result == NFD_OKAY) {
		sfz::DynArray<uint8_t> binary = sfz::readBinaryFile(path);
		if (binary.size() == 0) {
			SFZ_ERROR("PhantasyEngine", "Could not read game state from \"%s\"", path);
		}
		else if (binary.size() != state->stateSizeBytes) {
			SFZ_ERROR("PhantasyEngine", "Game state from \"%s\" is wrong size", path);
		}
		else {
			// TODO: Check if header matches
			std::memcpy(state, binary.data(), state->stateSizeBytes);
			SFZ_INFO("PhantasyEngine", "Loaded game state from \"%s\"", path);
		}
		free(path);
	}
	else if (result == NFD_ERROR) {
		SFZ_ERROR("PhantasyEngine", "nativefiledialog: NFD_OpenDialog() error: %s",
			NFD_GetError());
	}
}
#endif

// GameStateEditor: State methods
// ------------------------------------------------------------------------------------------------

void GameStateEditor::init(
	const char* windowName,
	SingletonInfo* singletonInfos,
	uint32_t numSingletonInfos,
	ComponentInfo* componentInfos,
	uint32_t numComponentInfos,
	sfz::Allocator* allocator)
{
	this->destroy();

	// Initialize binary string to byte lookup map
	if (binaryStringToByteLookupMap == nullptr)
	{
		binaryStringToByteLookupMap =
			allocator->newObject<sfz::HashMap<str32, uint8_t>>(sfz_dbg(""));
		binaryStringToByteLookupMap->create(512, allocator);
		for (uint32_t i = 0; i < 256; i++) {
			(*binaryStringToByteLookupMap)[byteToBinaryStringLookupTable[i]] = uint8_t(i);
		}
	}

	// Initialize some state
	mWindowName.printf("%s", windowName);
	initializeComponentMaskEditor(mFilterMaskEditBuffers, mFilterMask);

	// Temp variable to ensure all necesary singleton infos are set
	bool singletonInfoSet[64] = {};

	// Set rest of singleton infos
	for (uint32_t i = 0; i < numSingletonInfos; i++) {
		SingletonInfo& info = singletonInfos[i];
		sfz_assert(info.singletonIndex < 64);

		ReducedSingletonInfo& target = mSingletonInfos[info.singletonIndex];
		bool& set = singletonInfoSet[info.singletonIndex];
		sfz_assert(!set);

		set = true;
		target.singletonName.printf("%02u - %s", info.singletonIndex, info.singletonName.str);
		target.singletonEditor = info.singletonEditor;
		target.userPtr = std::move(info.userPtr); // Steal it!
	}

	mNumSingletonInfos = numSingletonInfos;

	// Ensure that they are all set
	for (uint32_t i = 0; i < mNumSingletonInfos; i++) {
		sfz_assert(singletonInfoSet[i]);
	}

	// Temp variable to ensure all necessary component infos are set
	bool componentInfoSet[64] = {};

	// Set active bit component info
	componentInfoSet[0] = true;
	mComponentInfos[0].componentName.printf("00 - Active bit");

	// Set rest of component infos
	for (uint32_t i = 0; i < numComponentInfos; i++) {
		ComponentInfo& info = componentInfos[i];
		sfz_assert(info.componentType != 0);
		sfz_assert(info.componentType < 64);

		ReducedComponentInfo& target = mComponentInfos[info.componentType];
		bool& set = componentInfoSet[info.componentType];
		sfz_assert(!set);

		set = true;
		target.componentName.printf("%02u - %s", info.componentType, info.componentName.str);
		target.componentEditor = info.componentEditor;
		target.userPtr = std::move(info.userPtr); // Steal it!
	}

	// Number of component types should be equal to numComponentInfos + 1
	mNumComponentInfos = numComponentInfos + 1;

	// Ensure that they are all set
	for (uint32_t i = 0; i < mNumComponentInfos; i++) {
		sfz_assert(componentInfoSet[i]);
	}
}

void GameStateEditor::swap(GameStateEditor& other) noexcept
{
	std::swap(this->mWindowName, other.mWindowName);
	for (uint32_t i = 0; i < 64; i++) {
		std::swap(this->mSingletonInfos[i], other.mSingletonInfos[i]);
		std::swap(this->mComponentInfos[i], other.mComponentInfos[i]);
	}
	std::swap(this->mNumSingletonInfos, other.mNumSingletonInfos);
	std::swap(this->mNumComponentInfos, other.mNumComponentInfos);
	std::swap(this->mFilterMask, other.mFilterMask);
	for (uint32_t i = 0; i < 8; i++) {
		std::swap(this->mFilterMaskEditBuffers[i], other.mFilterMaskEditBuffers[i]);
	}
	std::swap(this->mCompactEntityList, other.mCompactEntityList);
	std::swap(this->mCurrentSelectedEntityId, other.mCurrentSelectedEntityId);
}

void GameStateEditor::destroy() noexcept
{
	mWindowName.printf("");
	for (uint32_t i = 0; i < 64; i++) {
		this->mSingletonInfos[i] = ReducedSingletonInfo();
		this->mComponentInfos[i] = ReducedComponentInfo();
	}
	mNumSingletonInfos = 0;
	mNumComponentInfos = 0;
	mFilterMask = ComponentMask::activeMask();
	for (uint32_t i = 0; i < 8; i++) {
		mFilterMaskEditBuffers[i].printf("");
	}
	mCompactEntityList = false;
	mCurrentSelectedEntityId = 0;

	// TODO: Not perfect, probable race condition if multiple game state viewers.
	if (binaryStringToByteLookupMap != nullptr) {
		binaryStringToByteLookupMap->allocator()->deleteObject(binaryStringToByteLookupMap);
		binaryStringToByteLookupMap = nullptr;
	}
}

// GameStateEditor: Methods
// ------------------------------------------------------------------------------------------------

void GameStateEditor::render(GameStateHeader* state) noexcept
{
	// Begin window
	ImGui::SetNextWindowSize(sfz::vec2(720.0f, 750.0f), ImGuiCond_FirstUseEver);
	ImGuiWindowFlags windowFlags = 0;
	//windowFlags |= ImGuiWindowFlags_NoResize;
	//windowFlags |= ImGuiWindowFlags_NoScrollbar;
	windowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
	ImGui::Begin(mWindowName.str, nullptr, windowFlags);

	// End window and return if no game state
	if (state == nullptr) {
		ImGui::Text("<none>");
		ImGui::End();
		return;
	}

	// End window and return if not a game state
	if (state->magicNumber != GAME_STATE_MAGIC_NUMBER) {
		ImGui::Text("<none> (Magic number is wrong, corrupt data?)");
		ImGui::End();
		return;
	}

	// End window and return if wrong version
	if (state->gameStateVersion != GAME_STATE_VERSION) {
		ImGui::Text("<none> (Version is wrong, corrupt data?)");
		ImGui::End();
		return;
	}

	// End window and return if wrong number of singleton editors
	if (state->numSingletons != mNumSingletonInfos) {
		ImGui::Text("<none> (Wrong number of singleton editors)");
		ImGui::End();
		return;
	}

	// End window and return if wrong number of component editors
	if (state->numComponentTypes != mNumComponentInfos) {
		ImGui::Text("<none> (Wrong number of component editors)");
		ImGui::End();
		return;
	}

	// Tabs
	ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("GameStateEditorTabBar", tabBarFlags)) {
		if (ImGui::BeginTabItem("Singletons")) {
			ImGui::Spacing();
			this->renderSingletonEditor(state);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("ECS")) {
			ImGui::Spacing();
			this->renderEcsEditor(state);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Info")) {
			ImGui::Spacing();
			this->renderInfoViewer(state);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	// End window
	ImGui::End();
}

// GameStateEditor: Private methods
// ------------------------------------------------------------------------------------------------

void GameStateEditor::renderSingletonEditor(GameStateHeader* state) noexcept
{
	// Render singleton editors
	for (uint32_t i = 0; i < mNumSingletonInfos; i++) {
		const ReducedSingletonInfo& info = mSingletonInfos[i];

		if (ImGui::CollapsingHeader(info.singletonName.str, ImGuiTreeNodeFlags_DefaultOpen)) {

			// Run editor
			ImGui::Indent(28.0f);
			if (info.singletonEditor == nullptr) {
				ImGui::Text("<No editor specified>");
			}
			else {
				uint32_t singletonSizeOut = 0;
				info.singletonEditor(
					info.userPtr.get(),
					state->singletonUntyped(i, singletonSizeOut),
					state);
			}
			ImGui::Unindent(28.0f);
		}
	}
}

void GameStateEditor::renderEcsEditor(GameStateHeader* state) noexcept
{
	const sfz::vec4 INACTIVE_TEXT_COLOR = sfz::vec4(0.35f, 0.35f, 0.35f, 1.0f);

	// We need component info for each component type in ECS
	sfz_assert(state->numComponentTypes == mNumComponentInfos);

	// Get some stuff from the game state
	ComponentMask* masks = state->componentMasks();
	const uint8_t* generations = state->entityGenerations();

	// Currently selected entities component mask
	ImGui::BeginGroup();
	componentMaskEditor("FilterMaskBit", mFilterMaskEditBuffers, mFilterMask);
	ImGui::Checkbox("Compact entity list", &mCompactEntityList);
	ImGui::EndGroup();

	// Separator between the different type of views
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Entities column
	ImGui::BeginGroup();

	// Entities list
	if (ImGui::ListBoxHeader("##Entities", vec2(136.0f, ImGui::GetWindowHeight() - 320.0f))) {
		for (uint32_t entityId = 0; entityId < state->maxNumEntities; entityId++) {

			// Check if entity fulfills filter mask
			bool fulfillsFilter = masks[entityId].fulfills(mFilterMask);

			// If compact list and does not fulfill filter mask, don't show entity
			if (!fulfillsFilter && mCompactEntityList) continue;

			// Non-fulfilling or non-active entities are greyed out
			bool active = masks[entityId].active();
			if (!fulfillsFilter || !active) {
				ImGui::PushStyleColor(ImGuiCol_Text, INACTIVE_TEXT_COLOR);
			}

			str32 entityStr("%08u [%02X]", entityId, generations[entityId]);
			bool selected = mCurrentSelectedEntityId == entityId;
			bool activated = ImGui::Selectable(entityStr, selected);
			if (activated) mCurrentSelectedEntityId = entityId;

			// Non-fulfilling or non-active entities are greyed out
			if (!fulfillsFilter || !active) ImGui::PopStyleColor();
		}
		ImGui::ListBoxFooter();
	}

	// New entity button
	if (ImGui::Button("New", sfz::vec2(136.0f, 0))) {
		Entity entity = state->createEntity();
		if (entity != Entity::invalid()) mCurrentSelectedEntityId = entity.id();
	}

	// Clone entity button
	if (ImGui::Button("Clone", sfz::vec2(136.0f, 0))) {
		uint8_t gen = state->entityGenerations()[mCurrentSelectedEntityId];
		Entity entity = state->cloneEntity(Entity::create(mCurrentSelectedEntityId, gen));
		if (entity != Entity::invalid()) mCurrentSelectedEntityId = entity.id();
	}

	// Delete entity button
	if (ImGui::Button("Delete", sfz::vec2(136.0f, 0))) {
		state->deleteEntity(mCurrentSelectedEntityId);

		// Select previous active entity
		for (int64_t i = int64_t(mCurrentSelectedEntityId) - 1; i >= 0; i--) {
			if (masks[i].active()) {
				mCurrentSelectedEntityId = uint32_t(i);
				break;
			}
		}
	}

	// End entities column
	ImGui::EndGroup();

	// Calculate width of content to the right of entities column
	const float rhsContentWidth = ImGui::GetWindowWidth() - 171;

	ImGui::SameLine();
	ImGui::BeginGroup();

	// Only show entity edit menu if an active entity exists
	bool selectedEntityExists = mCurrentSelectedEntityId < state->maxNumEntities;
	if (selectedEntityExists) {

		// Currently selected entities component mask
		ComponentMask& mask = masks[mCurrentSelectedEntityId];
		componentMaskVisualizer(mask);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Create child window stretching the remaining content area
		ImGui::BeginChild(
			"ComponentsChild",
			vec2(rhsContentWidth, ImGui::GetWindowHeight() - 290.0f),
			false,
			ImGuiWindowFlags_AlwaysVerticalScrollbar);

		for (uint32_t i = 0; i < mNumComponentInfos; i++) {
			const ReducedComponentInfo& info = mComponentInfos[i];

			// Get component size and components array
			uint32_t componentSize = 0;
			uint8_t* components = state->componentsUntyped(i, componentSize);
			bool unsizedComponent = componentSize == 0;

			// Check if entity has this component
			bool entityHasComponent = mask.hasComponentType(i);

			// Specialize for unsized component (i.e.) flag
			if (unsizedComponent) {
				if (!entityHasComponent) ImGui::PushStyleColor(ImGuiCol_Text, INACTIVE_TEXT_COLOR);
				bool checkboxBool = entityHasComponent;
				if (ImGui::Checkbox(sfz::str96("##%s", info.componentName.str), &checkboxBool)) {
					if (i != 0) {
						uint8_t entityGen = state->getGeneration(mCurrentSelectedEntityId);
						Entity entity = Entity::create(mCurrentSelectedEntityId, entityGen);
						state->setComponentUnsized(entity, i, checkboxBool);
					}
				}
				ImGui::SameLine();
				ImGui::Indent(79.0f);
				ImGui::Text("%s", info.componentName.str);
				ImGui::Unindent(79.0f);
				if (!entityHasComponent) ImGui::PopStyleColor();
			}

			// Sized component
			else {

				bool checkboxBool = entityHasComponent;

				if (ImGui::Checkbox(
					sfz::str96("##%s_checkbox", info.componentName.str), &checkboxBool)) {
					if (checkboxBool) {
						mask.setComponentType(i, checkboxBool);
					}
					else {
						uint8_t entityGen = state->getGeneration(mCurrentSelectedEntityId);
						Entity entity = Entity::create(mCurrentSelectedEntityId, entityGen);
						state->deleteComponent(entity, i);
					}
				}

				ImGui::SameLine();

				if (!entityHasComponent) ImGui::PushStyleColor(ImGuiCol_Text, INACTIVE_TEXT_COLOR);
				if (ImGui::CollapsingHeader(info.componentName.str, ImGuiTreeNodeFlags_DefaultOpen)) {

					// Disable editor if entity does not have component
					if (!entityHasComponent) ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);

					// Run editor
					ImGui::Indent(39.0f);
					if(info.componentEditor == nullptr) {
						ImGui::Text("<No editor specified>");
					}
					else {
						info.componentEditor(
							info.userPtr.get(),
							components + mCurrentSelectedEntityId * componentSize,
							state,
							mCurrentSelectedEntityId);
					}
					ImGui::Unindent(39.0f);

					// Disable editor if entity does not have component
					if (!entityHasComponent) ImGui::PopItemFlag();
				}
				if (!entityHasComponent) ImGui::PopStyleColor();
			}
		}

		ImGui::EndChild();
	}

	ImGui::EndGroup();
}

void GameStateEditor::renderInfoViewer(GameStateHeader* state) noexcept
{
	// GameStateHeader viewer
	ImGui::Separator();
	ImGui::Text("GameStateHeader");
	ImGui::Spacing();

	const float valueXOffset = 200;

	uint64_t magicNumberString[2];
	magicNumberString[0] = state->magicNumber;
	magicNumberString[1] = 0;
	uint64_t realMagicNumberString[2];
	realMagicNumberString[0] = GAME_STATE_MAGIC_NUMBER;
	realMagicNumberString[1] = 0;
	ImGui::Text("magicNumber:"); ImGui::SameLine(valueXOffset);
	ImGui::Text("\"%s\" (expected: \"%s\")",
		(const char*)magicNumberString, (const char*)realMagicNumberString);
	ImGui::Text("gameStateVersion:"); ImGui::SameLine(valueXOffset);
	ImGui::Text("%" PRIu64 " (compiled version: %" PRIu64 ")", state->gameStateVersion, GAME_STATE_VERSION);
	ImGui::Text("stateSize:"); ImGui::SameLine(valueXOffset);
	if (state->stateSizeBytes < 1048576) {
		ImGui::Text("%.2f KiB", float(state->stateSizeBytes) / 1024.0f);
	}
	else {
		ImGui::Text("%.2f MiB", float(state->stateSizeBytes) / (1024.0f * 1024.0f));
	}
	ImGui::Text("numSingletons:"); ImGui::SameLine(valueXOffset); ImGui::Text("%u", state->numSingletons);
	ImGui::Text("numComponentTypes:"); ImGui::SameLine(valueXOffset); ImGui::Text("%u", state->numComponentTypes);
	ImGui::Text("maxNumEntities:"); ImGui::SameLine(valueXOffset); ImGui::Text("%u", state->maxNumEntities);
	ImGui::Text("currentNumEntities:"); ImGui::SameLine(valueXOffset); ImGui::Text("%u", state->currentNumEntities);
	ImGui::Spacing();


#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
	// Saving/loading to file options
	ImGui::Separator();
	ImGui::Text("File options");
	ImGui::Spacing();

	// Save to file button
	if (ImGui::Button("Save to file (.phstate)", sfz::vec2(280, 0))) {
		saveDialog(state);
	}

	ImGui::Spacing();

	// Load from file button
	if (ImGui::Button("Load from file (.phstate)", sfz::vec2(280, 0))) {
		loadDialog(state);
	}
#endif
}

} // namespace ph
