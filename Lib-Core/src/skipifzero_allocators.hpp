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

#include "sfz.h"
#include "sfz_cpp.hpp"

#ifdef _WIN32
#include <malloc.h>
#endif

namespace sfz {

// Helper functions
// ------------------------------------------------------------------------------------------------

// Checks whether a pointer is aligned to a given byte aligment
sfz_constexpr_func bool isAligned(const void* pointer, u64 alignment) noexcept
{
	return ((u64)pointer & (alignment - 1)) == 0;
}

// StandardAllocator
// ------------------------------------------------------------------------------------------------

inline void* sfzStandardAlloc(void*, SfzDbgInfo, u64 size, u64 align)
{
	if (align < 32) align = 32;
#ifdef _WIN32
	return _aligned_malloc(size, align);
#else
	void* ptr = nullptr;
	posix_memalign(&ptr, align, size);
	return ptr;
#endif
}

inline void sfzStandardDealloc(void*, void* ptr)
{
	if (ptr == nullptr) return;
#ifdef _WIN32
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}

inline SfzAllocator createStandardAllocator()
{
	SfzAllocator alloc = {};
	alloc.alloc_func = sfzStandardAlloc;
	alloc.dealloc_func = sfzStandardDealloc;
	return alloc;
}

// Arena allocator
// ------------------------------------------------------------------------------------------------

// An arena allocator that allocates memory from a chunk of memory. It has an offset to the
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
// Prefer to use an ArenaHeap to avoid edge-cases and ensure you use the arena correctly.

struct AllocatorArenaState final {
	u8* memory = nullptr;
	u64 memory_size_bytes = 0;
	u64 current_offset_bytes = 0;
	u64 num_padding_bytes = 0;

	void init(void* memory_in, u64 memory_size_bytes_in)
	{
		sfz_assert(memory_in != nullptr);
		sfz_assert(isAligned(memory_in, 32));
		this->memory = reinterpret_cast<u8*>(memory_in);
		this->memory_size_bytes = memory_size_bytes_in;
		this->current_offset_bytes = 0;
		this->num_padding_bytes = 0;
	}

	void reset()
	{
		current_offset_bytes = 0;
		num_padding_bytes = 0;
	}
};

inline void* sfzArenaAlloc(void* rawArenaState, SfzDbgInfo, u64 size, u64 align)
{
	AllocatorArenaState& state = *reinterpret_cast<AllocatorArenaState*>(rawArenaState);
	//sfz_assert(sfzIsPow2_u64(align));

	// Get next suitable offset for allocation
	u64 padding = 0;
	if (!isAligned(state.memory + state.current_offset_bytes, align)) {

		// Calculate padding
		u64 alignmentOffset = u64(state.memory + state.current_offset_bytes) & (align - 1);
		padding = align - alignmentOffset;
		sfz_assert(isAligned(state.memory + state.current_offset_bytes + padding, align));
	}

	// Check if there is enough space left
	if ((state.current_offset_bytes + size + padding) > state.memory_size_bytes) return nullptr;

	// Allocate memory from arena and return pointer
	u8* ptr = state.memory + state.current_offset_bytes + padding;
	state.current_offset_bytes += padding + size;
	state.num_padding_bytes += padding;
	return ptr;
}

inline void sfzArenaDealloc(void*, void*) { /* no op */ }

// A convenience class creating and owning an arena allocator and a heap for it to use.
//
// This class makes it substantially simpler to use an arena allocator safely, as the allocator
// and its memory heap will always keep the same locations in memory. The handle (ArenaHeap) can
// easily be moved around like any drop type until it is destroyed.
class ArenaHeap final {
public:
	SFZ_DECLARE_DROP_TYPE(ArenaHeap);

	void init(SfzAllocator* allocator, u64 memory_size_bytes, SfzDbgInfo info)
	{
		this->destroy();
		m_allocator = allocator;
		u64 arena_size = sfzRoundUpAlignedU64(sizeof(SfzAllocator) + sizeof(AllocatorArenaState), 32);
		u64 block_plus_allocator_bytes = arena_size + memory_size_bytes;
		m_memory_block = reinterpret_cast<u8*>(allocator->alloc(info, block_plus_allocator_bytes, 32));

		SfzAllocator* allocMem = getArena();
		AllocatorArenaState* arenaState = getState();

		allocMem->alloc_func = sfzArenaAlloc;
		allocMem->dealloc_func = sfzArenaDealloc;
		allocMem->impl_data = arenaState;

		*arenaState = {};
		arenaState->memory = m_memory_block + arena_size;
		arenaState->memory_size_bytes = memory_size_bytes;
	}

	void destroy()
	{
		if (m_memory_block != nullptr) {
			m_allocator->dealloc(m_memory_block);
			m_allocator = nullptr;
			m_memory_block = nullptr;
		}
	}

	void resetArena()
	{
		getState()->reset();
	}

	SfzAllocator* getArena()
	{
		return reinterpret_cast<SfzAllocator*>(m_memory_block);
	}

	AllocatorArenaState* getState()
	{
		return reinterpret_cast<AllocatorArenaState*>(m_memory_block + sizeof(SfzAllocator));
	}

private:
	SfzAllocator* m_allocator = nullptr;
	u8* m_memory_block = nullptr;
};

} // namespace sfz

#endif
