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

#include "ph/state/EcsContainer.hpp"

#include <algorithm>
#include <cstring>

#include <sfz/Assert.hpp>

namespace ph {

// EcsContainer: Constructors & destructors
// ------------------------------------------------------------------------------------------------

EcsContainer EcsContainer::createRaw(uint64_t numBytes, sfz::Allocator* allocator) noexcept
{
	sfz_assert_debug(allocator != nullptr);
	sfz_assert_debug(0 < numBytes);

	EcsContainer container;
	container.mAllocator = allocator;
	container.mNumBytes = numBytes;
	container.mEcsMemoryChunk = static_cast<uint8_t*>(allocator->allocate(numBytes, 16, "ECS"));
	memset(container.mEcsMemoryChunk, 0, numBytes);
	return container;
}

// EcsContainer: State methods
// ------------------------------------------------------------------------------------------------

void EcsContainer::cloneTo(EcsContainer& ecs) noexcept
{
	sfz_assert_debug(ecs.mEcsMemoryChunk != nullptr);
	sfz_assert_debug(this->mNumBytes == ecs.mNumBytes);

	std::memcpy(ecs.mEcsMemoryChunk, this->mEcsMemoryChunk, this->mNumBytes);
}

EcsContainer EcsContainer::clone(Allocator* allocator) noexcept
{
	sfz_assert_debug(this->mEcsMemoryChunk != nullptr);
	sfz_assert_debug(this->mNumBytes != 0);
	sfz_assert_debug(allocator != nullptr);

	EcsContainer container;
	container.mAllocator = allocator;
	container.mNumBytes = this->mNumBytes;
	container.mEcsMemoryChunk = static_cast<uint8_t*>(allocator->allocate(mNumBytes, 32, "ECS"));
	this->cloneTo(container);
	return container;
}

void EcsContainer::swap(EcsContainer& other) noexcept
{
	std::swap(this->mAllocator, other.mAllocator);
	std::swap(this->mEcsMemoryChunk, other.mEcsMemoryChunk);
	std::swap(this->mNumBytes, other.mNumBytes);
}

void EcsContainer::destroy() noexcept
{
	if (this->mEcsMemoryChunk != nullptr) {
		this->mAllocator->deallocate(this->mEcsMemoryChunk);
	}
	this->mAllocator = nullptr;
	this->mEcsMemoryChunk = nullptr;
	this->mNumBytes = 0;
}

// EcsContainer: Methods
// --------------------------------------------------------------------------------------------

NaiveEcsHeader* EcsContainer::getNaive() noexcept
{
	return reinterpret_cast<NaiveEcsHeader*>(mEcsMemoryChunk);
}

const NaiveEcsHeader* EcsContainer::getNaive() const noexcept
{
	return reinterpret_cast<const NaiveEcsHeader*>(mEcsMemoryChunk);
}

} // namespace ph
