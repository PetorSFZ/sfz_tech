// Copyright (c) 2019 Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include "sfz/memory/ArenaAllocator.hpp"

#include <skipifzero.hpp>
#include <skipifzero_allocators.hpp>

#include <sfz/Logging.hpp>

namespace sfz {

// ArenaAllocator: State methods
// ------------------------------------------------------------------------------------------------

void ArenaAllocator::init(void* memory, uint64_t memorySizeBytes) noexcept
{
	sfz_assert(memory != nullptr);
	sfz_assert(isAligned(memory, 32));

	this->destroy();
	mMemory = reinterpret_cast<uint8_t*>(memory);
	mMemorySizeBytes = memorySizeBytes;
}

void ArenaAllocator::destroy() noexcept
{
	mMemory = nullptr;
	mMemorySizeBytes = 0;
	this->reset();
}

void ArenaAllocator::reset() noexcept
{
	mCurrentOffsetBytes = 0;
	mNumPaddingBytes = 0;
}

// ArenaAllocator: Implemented sfz::Allocator methods
// ------------------------------------------------------------------------------------------------

void* ArenaAllocator::allocate(DbgInfo dbg, uint64_t size, uint64_t alignment) noexcept
{
	(void)dbg;
	sfz_assert(isPowerOfTwo(alignment));

	// Get next suitable offset for allocation
	uint64_t padding = 0;
	if (!isAligned(mMemory + mCurrentOffsetBytes, alignment)) {

		// Calculate padding
		uint64_t alignmentOffset = uint64_t(mMemory + mCurrentOffsetBytes) & (alignment - 1);
		padding = alignment - alignmentOffset;
		sfz_assert(isAligned(mMemory + mCurrentOffsetBytes + padding, alignment));
	}

	// Check if there is enough space left
	if ((mCurrentOffsetBytes + size + padding) > mMemorySizeBytes) {
		SFZ_WARNING("ArenaAllocator",
			"Out of memory. Trying to allocate %llu bytes, currently %llu of %llu bytes allocated.",
			size + padding, mCurrentOffsetBytes, mMemorySizeBytes);
		return nullptr;
	}

	// Allocate memory from arena and return pointer
	uint8_t* ptr = mMemory + mCurrentOffsetBytes + padding;
	mCurrentOffsetBytes += padding + size;
	mNumPaddingBytes += padding;
	return ptr;
}

// ArenaEasyAllocator
// ------------------------------------------------------------------------------------------------

void ArenaEasyAllocator::init(Allocator* allocator, uint64_t memorySizeBytes, DbgInfo info)
{
	this->destroy();
	mAllocator = allocator;
	uint64_t arenaSize = roundUpAligned(sizeof(ArenaAllocator), 32);
	uint64_t blockPlusAllocatorBytes = arenaSize + memorySizeBytes;
	mMemoryBlock = allocator->allocate(info, blockPlusAllocatorBytes);
	
	void* startOfArenaMem = reinterpret_cast<uint8_t*>(mMemoryBlock) + arenaSize;
	new (mMemoryBlock) ArenaAllocator();
	getArena()->init(startOfArenaMem, memorySizeBytes);
}

void ArenaEasyAllocator::destroy()
{
	if(mMemoryBlock != nullptr) {
		getArena()->destroy();
		mAllocator->deallocate(mMemoryBlock);
		mAllocator = nullptr;
		mMemoryBlock = nullptr;
	}
}

ArenaAllocator* ArenaEasyAllocator::getArena()
{
	return reinterpret_cast<ArenaAllocator*>(mMemoryBlock);
}

} // namespace sfz
