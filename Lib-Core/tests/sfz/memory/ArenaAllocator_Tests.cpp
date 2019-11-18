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

#include "sfz/PushWarnings.hpp"
#include "catch2/catch.hpp"
#include "sfz/PopWarnings.hpp"

#include <skipifzero_allocators.hpp>

#include "sfz/Context.hpp"
#include "sfz/Logging.hpp"
#include "sfz/memory/ArenaAllocator.hpp"

using namespace sfz;

TEST_CASE("ArenaAllocator: Stack based memory", "[sfz::ArenaAllocator]")
{
	sfz::setContext(sfz::getStandardContext());

	// Create default-constructed arena without memory
	ArenaAllocator arena;
	REQUIRE(arena.capacity() == 0);
	REQUIRE(arena.numBytesAllocated() == 0);
	REQUIRE(arena.numPaddingBytes() == 0);

	// Initialize arena with memory
	constexpr uint64_t MEMORY_HEAP_SIZE = sizeof(uint32_t) * 4;
	alignas(32) uint8_t memoryHeap[MEMORY_HEAP_SIZE];
	arena.init(memoryHeap, MEMORY_HEAP_SIZE);
	REQUIRE(arena.capacity() == MEMORY_HEAP_SIZE);
	REQUIRE(arena.numBytesAllocated() == 0);
	REQUIRE(arena.numPaddingBytes() == 0);

	// Do some allocations
	uint32_t* first = (uint32_t*)arena.allocate(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	REQUIRE(arena.numBytesAllocated() == 4);
	REQUIRE(arena.numPaddingBytes() == 0);
	REQUIRE(first == (uint32_t*)&memoryHeap[0]);

	uint32_t* second = (uint32_t*)arena.allocate(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	REQUIRE(arena.numBytesAllocated() == 8);
	REQUIRE(arena.numPaddingBytes() == 0);
	REQUIRE(second == (uint32_t*)&memoryHeap[4]);

	uint32_t* third = (uint32_t*)arena.allocate(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	REQUIRE(arena.numBytesAllocated() == 12);
	REQUIRE(arena.numPaddingBytes() == 0);
	REQUIRE(third == (uint32_t*)&memoryHeap[8]);

	uint32_t* fourth = (uint32_t*)arena.allocate(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	REQUIRE(arena.numBytesAllocated() == 16);
	REQUIRE(arena.numPaddingBytes() == 0);
	REQUIRE(fourth == (uint32_t*)&memoryHeap[12]);

	SFZ_INFO("ArenaAllocator Tests", "The warning below is expected, ignore");
	void* fifth = arena.allocate(sfz_dbg(""), 1, 1);
	REQUIRE(arena.numBytesAllocated() == 16);
	REQUIRE(arena.numPaddingBytes() == 0);
	REQUIRE(fifth == nullptr);

	// Reset the arena
	arena.reset();

	// Allocations with larger alignment requirements
	uint32_t* first2 = (uint32_t*)arena.allocate(sfz_dbg(""), sizeof(uint32_t), sizeof(uint32_t));
	REQUIRE(arena.numBytesAllocated() == 4);
	REQUIRE(arena.numPaddingBytes() == 0);
	REQUIRE(first2 == (uint32_t*)&memoryHeap[0]);

	uint32_t* largeAligned = (uint32_t*)arena.allocate(sfz_dbg(""), sizeof(uint32_t), 8);
	REQUIRE(arena.numBytesAllocated() == 12);
	REQUIRE(arena.numPaddingBytes() == 4);
	REQUIRE(largeAligned == (uint32_t*)&memoryHeap[8]);
}
