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

#include "ph/state/GameStateContainer.hpp"

#include <algorithm>
#include <cstring>

#include <sfz/Assert.hpp>

namespace ph {

// GameStateContainer: Constructors & destructors
// ------------------------------------------------------------------------------------------------

	GameStateContainer GameStateContainer::createRaw(
		uint64_t numBytes, sfz::Allocator* allocator) noexcept
{
	sfz_assert(allocator != nullptr);
	sfz_assert(0 < numBytes);

	GameStateContainer container;
	container.mAllocator = allocator;
	container.mNumBytes = numBytes;
	container.mGameStateMemoryChunk = static_cast<uint8_t*>(allocator->allocate(numBytes, 16, "ECS"));
	memset(container.mGameStateMemoryChunk, 0, numBytes);
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

GameStateContainer GameStateContainer::clone(Allocator* allocator) noexcept
{
	sfz_assert(this->mGameStateMemoryChunk != nullptr);
	sfz_assert(this->mNumBytes != 0);
	sfz_assert(allocator != nullptr);

	GameStateContainer container;
	container.mAllocator = allocator;
	container.mNumBytes = this->mNumBytes;
	container.mGameStateMemoryChunk = static_cast<uint8_t*>(allocator->allocate(mNumBytes, 32, "ECS"));
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
		this->mAllocator->deallocate(this->mGameStateMemoryChunk);
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

} // namespace ph
