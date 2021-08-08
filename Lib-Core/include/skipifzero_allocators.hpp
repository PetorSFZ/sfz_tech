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

#ifndef SKIPIFZERO_ALLOCATORS_HPP
#define SKIPIFZERO_ALLOCATORS_HPP
#pragma once

#include "skipifzero.hpp"

#ifdef _WIN32
#include <malloc.h>
#endif

namespace sfz {

// StandardAllocator
// ------------------------------------------------------------------------------------------------

class StandardAllocator final : public Allocator {
public:
	void* allocate(SfzDbgInfo dbg, uint64_t size, uint64_t alignment) noexcept override final
	{
		(void)dbg;
		sfz_assert(isPowerOfTwo(alignment));

#ifdef _WIN32
		return _aligned_malloc(size, alignment);
#else
		void* ptr = nullptr;
		posix_memalign(&ptr, alignment, size);
		return ptr;
#endif
	}

	void deallocate(void* pointer) noexcept override final
	{
		if (pointer == nullptr) return;
#ifdef _WIN32
		_aligned_free(pointer);
#else
		free(pointer);
#endif
	}
};

// AllocatorArena
// ------------------------------------------------------------------------------------------------

// An Arena Allocator that allocates memory from a chunk of memory. It has an offset to the
// beginning of this chunk, each allocation increments the offset. Extremely fast and efficient
// allocations.
//
// In essence, an arena allocator is not capable of deallocating indvidual allocations. It can only
// "deallocate" all the memory for everything that has been allocated from it, and this is done by
// just setting the offset back to 0 (the begininng of the memory chunk).
//
// The arena allocator is good for temporary allocations. An example would be to use it as a
// "frame allocator". The arena is used for temporary allocations during a frame and then reset at
// the end of it. Extremely fast temporary allocations, and no need to indvidually deallocate all of
// them. See more: https://en.wikipedia.org/wiki/Region-based_memory_management
//
// Prefer to use an ArenaHeap (see below) rather than creating an AllocatorArena yourself.
class AllocatorArena final : public Allocator {
public:
	AllocatorArena() noexcept = default;
	AllocatorArena(const AllocatorArena&) = delete;
	AllocatorArena& operator= (const AllocatorArena&) = delete;
	AllocatorArena(AllocatorArena&&) = delete;
	AllocatorArena& operator= (AllocatorArena&&) = delete;
	~AllocatorArena() noexcept { this->destroy(); }

	void init(void* memory, uint64_t memorySizeBytes) noexcept
	{
		sfz_assert(memory != nullptr);
		sfz_assert(isAligned(memory, 32));

		this->destroy();
		mMemory = reinterpret_cast<uint8_t*>(memory);
		mMemorySizeBytes = memorySizeBytes;
	}

	void destroy() noexcept
	{
		mMemory = nullptr;
		mMemorySizeBytes = 0;
		this->reset();
	}

	// Resets this arena allocator, "deallocating" everything that has been allocated from it.
	// This simply means moving the internal offset back to the beginning of the memory chunk.
	void reset() noexcept
	{
		mCurrentOffsetBytes = 0;
		mNumPaddingBytes = 0;
	}

	uint64_t capacity() const noexcept { return mMemorySizeBytes; }
	uint64_t numBytesAllocated() const noexcept { return mCurrentOffsetBytes; }
	uint64_t numPaddingBytes() const noexcept { return mNumPaddingBytes; }

	void* allocate(SfzDbgInfo dbg, uint64_t size, uint64_t alignment = 32) noexcept override final
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
		if ((mCurrentOffsetBytes + size + padding) > mMemorySizeBytes) return nullptr;

		// Allocate memory from arena and return pointer
		uint8_t* ptr = mMemory + mCurrentOffsetBytes + padding;
		mCurrentOffsetBytes += padding + size;
		mNumPaddingBytes += padding;
		return ptr;
	}

	void deallocate(void*) noexcept override final { /* no-op */ }

private:
	uint8_t* mMemory = nullptr;
	uint64_t mMemorySizeBytes = 0;
	uint64_t mCurrentOffsetBytes = 0;
	uint64_t mNumPaddingBytes = 0;
};

// A convenience class creating and owning an AllocatorArena and a heap for it to use.
//
// This class makes it substantially simpler to use an AlloctorArena safely, as the allocator
// and its memory heap will always keep the same locations in memory. The handle (ArenaHeap) can
// easily be moved around like any drop type until it is destroyed.
class ArenaHeap final {
public:
	SFZ_DECLARE_DROP_TYPE(ArenaHeap);

	void init(Allocator* allocator, uint64_t memorySizeBytes, SfzDbgInfo info)
	{
		this->destroy();
		mAllocator = allocator;
		uint64_t arenaSize = roundUpAligned(sizeof(AllocatorArena), 32);
		uint64_t blockPlusAllocatorBytes = arenaSize + memorySizeBytes;
		mMemoryBlock = allocator->allocate(info, blockPlusAllocatorBytes, 32);

		void* startOfArenaMem = reinterpret_cast<uint8_t*>(mMemoryBlock) + arenaSize;
		new (mMemoryBlock) AllocatorArena();
		getArena()->init(startOfArenaMem, memorySizeBytes);
	}

	void destroy()
	{
		if (mMemoryBlock != nullptr) {
			getArena()->destroy();
			mAllocator->deallocate(mMemoryBlock);
			mAllocator = nullptr;
			mMemoryBlock = nullptr;
		}
	}

	AllocatorArena* getArena()
	{
		return reinterpret_cast<AllocatorArena*>(mMemoryBlock);
	}

private:
	Allocator* mAllocator = nullptr;
	void* mMemoryBlock = nullptr;
};

} // namespace sfz

#endif
