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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "utest.h"
#undef near
#undef far

#include <skipifzero_allocators.hpp>

UTEST(AllocatorArena, stack_based_memory)
{
	// Create default-constructed arena without memory
	sfz::AllocatorArenaState state = {};
	SfzAllocator arena = {};
	arena.implData = &state;
	arena.allocFunc = sfz::sfzArenaAlloc;
	arena.deallocFunc = sfz::sfzArenaDealloc;

	// Initialize arena with memory
	constexpr uint64_t MEMORY_HEAP_SIZE = sizeof(uint32_t) * 4;
	alignas(32) uint8_t memoryHeap[MEMORY_HEAP_SIZE];
	state.memory = memoryHeap;
	state.memorySizeBytes = MEMORY_HEAP_SIZE;
	ASSERT_TRUE(state.currentOffsetBytes == 0);
	ASSERT_TRUE(state.numPaddingBytes == 0);

	// Do some allocations
	uint32_t* first = (uint32_t*)arena.alloc(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	ASSERT_TRUE(state.currentOffsetBytes == 4);
	ASSERT_TRUE(state.numPaddingBytes == 0);
	ASSERT_TRUE(first == (uint32_t*)&memoryHeap[0]);

	uint32_t* second = (uint32_t*)arena.alloc(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	ASSERT_TRUE(state.currentOffsetBytes == 8);
	ASSERT_TRUE(state.numPaddingBytes == 0);
	ASSERT_TRUE(second == (uint32_t*)&memoryHeap[4]);

	uint32_t* third = (uint32_t*)arena.alloc(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	ASSERT_TRUE(state.currentOffsetBytes == 12);
	ASSERT_TRUE(state.numPaddingBytes == 0);
	ASSERT_TRUE(third == (uint32_t*)&memoryHeap[8]);

	uint32_t* fourth = (uint32_t*)arena.alloc(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	ASSERT_TRUE(state.currentOffsetBytes == 16);
	ASSERT_TRUE(state.numPaddingBytes == 0);
	ASSERT_TRUE(fourth == (uint32_t*)&memoryHeap[12]);

	void* fifth = arena.alloc(sfz_dbg(""), 1, 1);
	ASSERT_TRUE(state.currentOffsetBytes == 16);
	ASSERT_TRUE(state.numPaddingBytes == 0);
	ASSERT_TRUE(fifth == nullptr);

	// Reset the arena
	state.reset();

	// Allocations with larger alignment requirements
	uint32_t* first2 = (uint32_t*)arena.alloc(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	ASSERT_TRUE(state.currentOffsetBytes == 4);
	ASSERT_TRUE(state.numPaddingBytes == 0);
	ASSERT_TRUE(first2 == (uint32_t*)&memoryHeap[0]);

	uint32_t* largeAligned = (uint32_t*)arena.alloc(sfz_dbg(""), sizeof(uint32_t), 8);
	ASSERT_TRUE(state.currentOffsetBytes == 12);
	ASSERT_TRUE(state.numPaddingBytes == 4);
	ASSERT_TRUE(largeAligned == (uint32_t*)&memoryHeap[8]);
}
