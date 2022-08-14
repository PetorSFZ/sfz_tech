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

#include <doctest.h>

#include <skipifzero_allocators.hpp>

TEST_CASE("AllocatorArena: stack_based_memory")
{
	// Create default-constructed arena without memory
	sfz::AllocatorArenaState state = {};
	SfzAllocator arena = {};
	arena.implData = &state;
	arena.allocFunc = sfz::sfzArenaAlloc;
	arena.deallocFunc = sfz::sfzArenaDealloc;

	// Initialize arena with memory
	constexpr u64 MEMORY_HEAP_SIZE = sizeof(u32) * 4;
	alignas(32) u8 memoryHeap[MEMORY_HEAP_SIZE];
	state.memory = memoryHeap;
	state.memorySizeBytes = MEMORY_HEAP_SIZE;
	CHECK(state.currentOffsetBytes == 0);
	CHECK(state.numPaddingBytes == 0);

	// Do some allocations
	u32* first = (u32*)arena.alloc(sfz_dbg(""), sizeof(u32), sizeof(u32));
	CHECK(state.currentOffsetBytes == 4);
	CHECK(state.numPaddingBytes == 0);
	CHECK(first == (u32*)&memoryHeap[0]);

	u32* second = (u32*)arena.alloc(sfz_dbg(""), sizeof(u32), sizeof(u32));
	CHECK(state.currentOffsetBytes == 8);
	CHECK(state.numPaddingBytes == 0);
	CHECK(second == (u32*)&memoryHeap[4]);

	u32* third = (u32*)arena.alloc(sfz_dbg(""), sizeof(u32), sizeof(u32));
	CHECK(state.currentOffsetBytes == 12);
	CHECK(state.numPaddingBytes == 0);
	CHECK(third == (u32*)&memoryHeap[8]);

	u32* fourth = (u32*)arena.alloc(sfz_dbg(""), sizeof(u32), sizeof(u32));
	CHECK(state.currentOffsetBytes == 16);
	CHECK(state.numPaddingBytes == 0);
	CHECK(fourth == (u32*)&memoryHeap[12]);

	void* fifth = arena.alloc(sfz_dbg(""), 1, 1);
	CHECK(state.currentOffsetBytes == 16);
	CHECK(state.numPaddingBytes == 0);
	CHECK(fifth == nullptr);

	// Reset the arena
	state.reset();

	// Allocations with larger alignment requirements
	u32* first2 = (u32*)arena.alloc(sfz_dbg(""), sizeof(u32), sizeof(u32));
	CHECK(state.currentOffsetBytes == 4);
	CHECK(state.numPaddingBytes == 0);
	CHECK(first2 == (u32*)&memoryHeap[0]);

	u32* largeAligned = (u32*)arena.alloc(sfz_dbg(""), sizeof(u32), 8);
	CHECK(state.currentOffsetBytes == 12);
	CHECK(state.numPaddingBytes == 4);
	CHECK(largeAligned == (u32*)&memoryHeap[8]);
}
