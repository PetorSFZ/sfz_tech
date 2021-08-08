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

#include "sfz/state/GameState.hpp"

#include <skipifzero_new.hpp>
#include <skipifzero_strings.hpp>

namespace sfz {

// Helper structs
// ------------------------------------------------------------------------------------------------

struct SingletonInfo final {
	uint32_t singletonIndex = ~0u;
	str80 singletonName;
	void(*singletonEditor)(
		uint8_t* userPtr, uint8_t* singletonData, GameStateHeader* state) = nullptr;
	sfz::UniquePtr<uint8_t> userPtr;
	// ^^^ Above is a bit of a hack. User ptr  may NOT have a non-trival destructor (i.e. must
	// be POD) because the destructor will never be called.
};

struct ComponentInfo final {
	uint32_t componentType = ~0u;
	str80 componentName;
	void(*componentEditor)(
		uint8_t* userPtr, uint8_t* componentData, GameStateHeader* state, uint32_t entity) = nullptr;
	sfz::UniquePtr<uint8_t> userPtr;
	// ^^^ Above is a bit of a hack. User ptr  may NOT have a non-trival destructor (i.e. must
	// be POD) because the destructor will never be called.
};

// GameStateEditor class
// ------------------------------------------------------------------------------------------------

class GameStateEditor final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	GameStateEditor() noexcept {}
	GameStateEditor(const GameStateEditor&) = delete;
	GameStateEditor& operator= (const GameStateEditor&) = delete;
	GameStateEditor(GameStateEditor&& other) noexcept { swap(other); }
	GameStateEditor& operator= (GameStateEditor&& other) noexcept { swap(other); return *this; }
	~GameStateEditor() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(
		const char* windowName,
		SingletonInfo* singletonInfos,
		uint32_t numSingletonInfos,
		ComponentInfo* componentInfos,
		uint32_t numComponentInfos,
		sfz::Allocator* allocator);
	void swap(GameStateEditor& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void render(GameStateHeader* state) noexcept;

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	void renderSingletonEditor(GameStateHeader* state) noexcept;

	void renderEcsEditor(GameStateHeader* state) noexcept;

	void renderInfoViewer(GameStateHeader* state) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	struct ReducedSingletonInfo final {
		str80 singletonName;
		void(*singletonEditor)(
			uint8_t* userPtr, uint8_t* singletonData, GameStateHeader* state) = nullptr;
		sfz::UniquePtr<uint8_t> userPtr;
	};

	struct ReducedComponentInfo {
		str80 componentName;
		void(*componentEditor)(
			uint8_t* userPtr, uint8_t* componentData, GameStateHeader* state, uint32_t entity) = nullptr;
		sfz::UniquePtr<uint8_t> userPtr;
	};

	str80 mWindowName;
	ReducedSingletonInfo mSingletonInfos[64] = {};
	ReducedComponentInfo mComponentInfos[64] = {};
	uint32_t mNumSingletonInfos = 0;
	uint32_t mNumComponentInfos = 0;
	CompMask mFilterMask = CompMask::activeMask();
	str32 mFilterMaskEditBuffers[8];
	bool mCompactEntityList = false;
	uint32_t mCurrentSelectedEntityId = 0;
};

} // namespace sfz
