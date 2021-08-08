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

#include <skipifzero.hpp>

#include <sfz/Context.hpp>

#include "sfz/state/GameState.hpp"

namespace sfz {

// GameStateContainer class
// ------------------------------------------------------------------------------------------------

// Smart-pointer-ish class owning the memory blob for a single snap-shot of the game state
class GameStateContainer final {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	GameStateContainer() noexcept = default;
	GameStateContainer(const GameStateContainer&) = delete;
	GameStateContainer& operator= (const GameStateContainer&) = delete;
	GameStateContainer(GameStateContainer&& other) noexcept { this->swap(other); }
	GameStateContainer& operator= (GameStateContainer&& other) noexcept { this->swap(other); return *this; }
	~GameStateContainer() noexcept { this->destroy(); }

	static GameStateContainer createRaw(uint64_t numBytes, SfzAllocator* allocator) noexcept;
	static GameStateContainer create(
		uint32_t numSingletonStructs,
		const uint32_t* singletonStructSizes,
		uint32_t maxNumEntities,
		uint32_t numComponentTypes,
		const uint32_t* componentSizes,
		SfzAllocator* allocator) noexcept;

	// State methods
	// --------------------------------------------------------------------------------------------

	void cloneTo(GameStateContainer& state) noexcept;
	GameStateContainer clone(SfzAllocator* allocator = sfz::getDefaultAllocator()) noexcept;
	void swap(GameStateContainer& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	GameStateHeader* getHeader() noexcept;
	const GameStateHeader* getHeader() const noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	SfzAllocator* mAllocator = nullptr;
	uint8_t* mGameStateMemoryChunk = nullptr;
	uint64_t mNumBytes = 0;
};

} // namespace sfz
