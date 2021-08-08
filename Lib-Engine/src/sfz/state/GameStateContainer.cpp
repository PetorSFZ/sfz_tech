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

#include "sfz/state/GameStateContainer.hpp"

#include <algorithm>
#include <cstring>

#include <skipifzero.hpp>

namespace sfz {

// GameStateContainer: Constructors & destructors
// ------------------------------------------------------------------------------------------------

GameStateContainer GameStateContainer::createRaw(
	uint64_t numBytes, SfzAllocator* allocator) noexcept
{
	sfz_assert(allocator != nullptr);
	sfz_assert(0 < numBytes);

	GameStateContainer container;
	container.mAllocator = allocator;
	container.mNumBytes = numBytes;
	container.mGameStateMemoryChunk = static_cast<uint8_t*>(allocator->alloc(sfz_dbg(""), numBytes, 16));
	memset(container.mGameStateMemoryChunk, 0, numBytes);
	return container;
}

GameStateContainer GameStateContainer::create(
	uint32_t numSingletonStructs,
	const uint32_t* singletonStructSizes,
	uint32_t maxNumEntities,
	uint32_t numComponentTypes,
	const uint32_t* componentSizes,
	SfzAllocator* allocator) noexcept
{
	uint32_t neededSize = calcSizeOfGameStateBytes(
		numSingletonStructs, singletonStructSizes, maxNumEntities, numComponentTypes, componentSizes);

	// Allocate memory
	GameStateContainer container = GameStateContainer::createRaw(neededSize, allocator);
	GameStateHeader* state = container.getHeader();

	// Initialize memory
	bool success = createGameState(state, neededSize, numSingletonStructs, singletonStructSizes, maxNumEntities, numComponentTypes, componentSizes);
	sfz_assert(success);

	return container;
}

// GameStateContainer: State methods
// ------------------------------------------------------------------------------------------------

void GameStateContainer::cloneTo(GameStateContainer& state) noexcept
{
	sfz_assert(state.mGameStateMemoryChunk != nullptr);
	sfz_assert(this->mNumBytes == state.mNumBytes);

	std::memcpy(state.mGameStateMemoryChunk, this->mGameStateMemoryChunk, this->mNumBytes);
}

GameStateContainer GameStateContainer::clone(SfzAllocator* allocator) noexcept
{
	sfz_assert(this->mGameStateMemoryChunk != nullptr);
	sfz_assert(this->mNumBytes != 0);
	sfz_assert(allocator != nullptr);

	GameStateContainer container;
	container.mAllocator = allocator;
	container.mNumBytes = this->mNumBytes;
	container.mGameStateMemoryChunk = static_cast<uint8_t*>(allocator->alloc(sfz_dbg(""), mNumBytes, 32));
	this->cloneTo(container);
	return container;
}

void GameStateContainer::swap(GameStateContainer& other) noexcept
{
	std::swap(this->mAllocator, other.mAllocator);
	std::swap(this->mGameStateMemoryChunk, other.mGameStateMemoryChunk);
	std::swap(this->mNumBytes, other.mNumBytes);
}

void GameStateContainer::destroy() noexcept
{
	if (this->mGameStateMemoryChunk != nullptr) {
		this->mAllocator->dealloc(this->mGameStateMemoryChunk);
	}
	this->mAllocator = nullptr;
	this->mGameStateMemoryChunk = nullptr;
	this->mNumBytes = 0;
}

// GameStateContainer: Methods
// --------------------------------------------------------------------------------------------

GameStateHeader* GameStateContainer::getHeader() noexcept
{
	return reinterpret_cast<GameStateHeader*>(mGameStateMemoryChunk);
}

const GameStateHeader* GameStateContainer::getHeader() const noexcept
{
	return reinterpret_cast<const GameStateHeader*>(mGameStateMemoryChunk);
}

} // namespace sfz
