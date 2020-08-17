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

#pragma once

#include <skipifzero.hpp>

namespace sfz {

// ArenaAllocator class
// ------------------------------------------------------------------------------------------------

// Arena allocator
//
// The Arena allocator is given a chunk of memory to distribute when initialized. It starts of by
// holding an offset to the beginning of this chunk. Each time memory is allocated, this offset is
// increased. This means extremely fast and efficient allocations.
//
// In essence, an arena allocator is not capable of deallocating indvidual allocations. It can only
// "deallocate" all the memory for everything that has been allocated from it, and this is done by
// just setting the offset back to 0 (the begininng of the memory chunk).
//
// The arena allocator is good for temporary allocations. An example would be to use it as a
// "frame allocator". The arena is used for temporary allocations during a frame and then reset at
// the end of it. Extremely fast temporary allocations, and no need to indvidually deallocate all of
// them.
//
// See more: https://en.wikipedia.org/wiki/Region-based_memory_management
class ArenaAllocator final : public Allocator {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ArenaAllocator() noexcept = default;
	ArenaAllocator(const ArenaAllocator&) = delete;
	ArenaAllocator& operator= (const ArenaAllocator&) = delete;
	ArenaAllocator(ArenaAllocator&&) = delete;
	ArenaAllocator& operator= (ArenaAllocator&&) = delete;
	~ArenaAllocator() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(void* memory, uint64_t memorySizeBytes) noexcept;
	void destroy() noexcept;

	// Resets this arena allocator, "deallocating" everything that has been allocated from it.
	// This simply means moving the internal offset back to the beginning of the memory chunk.
	void reset() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	uint64_t capacity() const noexcept { return mMemorySizeBytes; }
	uint64_t numBytesAllocated() const noexcept { return mCurrentOffsetBytes; }
	uint64_t numPaddingBytes() const noexcept { return mNumPaddingBytes; }

	// Implemented sfz::Allocator methods
	// --------------------------------------------------------------------------------------------

	void* allocate(DbgInfo dbg, uint64_t size, uint64_t alignment = 32) noexcept override final;
	void deallocate(void*) noexcept override final { /* no-op */ }

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	uint8_t* mMemory = nullptr;
	uint64_t mMemorySizeBytes = 0;
	uint64_t mCurrentOffsetBytes = 0;
	uint64_t mNumPaddingBytes = 0;
};

// ArenaEasyAllocator
// ------------------------------------------------------------------------------------------------

// A convenience class around ArenaAllocator that makes it simpler to use.
//
// * Owns the ArenaAllocator and its memory, reducing the amount of setup needed
// * The ArenaAllocator itself is allocated on the heap, ensuring that it never changes location
//   until it's destroyed.
// * Move semantics of the "EasyArenaAllocator" (since the allocator itself is on the heap).
// * Single allocation for both the arena allocator and the memory it handles.
class ArenaEasyAllocator final {
public:
	SFZ_DECLARE_DROP_TYPE(ArenaEasyAllocator);

	void init(Allocator* allocator, uint64_t memorySizeBytes, DbgInfo info);
	void destroy();

	ArenaAllocator* getArena();

private:
	Allocator* mAllocator = nullptr;
	void* mMemoryBlock = nullptr;
};

} // namespace sfz
