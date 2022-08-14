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

#include "sfz.h"
#include "skipifzero_allocators.hpp"
#include "skipifzero_pool.hpp"

// Pool tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("Pool: init")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Default constructed
	{
		sfz::Pool<u64> pool;
		CHECK(pool.numAllocated() == 0);
		CHECK(pool.numHoles() == 0);
		CHECK(pool.arraySize() == 0);
		CHECK(pool.capacity() == 0);
		CHECK(pool.data() == nullptr);
		CHECK(pool.slots() == nullptr);
		CHECK(pool.allocator() == nullptr);
	}

	// Init method
	{
		sfz::Pool<u64> pool;
		pool.init(42, &allocator, sfz_dbg(""));
		CHECK(pool.numAllocated() == 0);
		CHECK(pool.numHoles() == 0);
		CHECK(pool.arraySize() == 0);
		CHECK(pool.capacity() == 42);
		CHECK(pool.data() != nullptr);
		CHECK(pool.slots() != nullptr);
		CHECK(pool.allocator() != nullptr);
	}

	// Init constructor
	{
		sfz::Pool<u64> pool = sfz::Pool<u64>(13, &allocator, sfz_dbg(""));
		CHECK(pool.numAllocated() == 0);
		CHECK(pool.numHoles() == 0);
		CHECK(pool.arraySize() == 0);
		CHECK(pool.capacity() == 13);
		CHECK(pool.data() != nullptr);
		CHECK(pool.slots() != nullptr);
		CHECK(pool.allocator() != nullptr);
	}
}

TEST_CASE("Pool: allocating_and_deallocating")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Allocating to full capacity linearly
	{
		constexpr u32 CAPACITY = 64;
		sfz::Pool<u32> pool;
		pool.init(CAPACITY, &allocator, sfz_dbg(""));

		for (u32 i = 0; i < CAPACITY; i++) {
			SfzHandle handle = pool.allocate();
			u32& val = pool[handle];
			val = i;
			CHECK(handle.idx() == i);
			CHECK(handle.version() == u8(1));
			CHECK(pool.numAllocated() == (i + 1));
			CHECK(pool.numHoles() == 0);
			CHECK(pool.slotIsActive(handle.idx()));
			CHECK(pool.getVersion(handle.idx()) == handle.version());
		}
		CHECK(pool.numAllocated() == CAPACITY);
		CHECK(pool.numHoles() == 0);
	}

	// Allocating and deallocating a single slot until version wraps around
	{
		constexpr u32 CAPACITY = 4;
		sfz::Pool<u32> pool;
		pool.init(CAPACITY, &allocator, sfz_dbg(""));

		for (u32 i = 1; i <= 127; i++) {
			SfzHandle handle = pool.allocate();
			CHECK(pool.handleIsValid(handle));
			CHECK(handle.idx() == 0);
			CHECK(handle.version() == u8(i));
			CHECK(pool.numAllocated() == 1);
			CHECK(pool.numHoles() == 0);
			CHECK(pool.arraySize() == 1);
			CHECK(pool.slotIsActive(handle.idx()));
			CHECK(pool.getVersion(handle.idx()) == handle.version());

			pool.deallocate(handle, i);
			CHECK(!pool.handleIsValid(handle));
			CHECK(pool.numAllocated() == 0);
			CHECK(pool.numHoles() == 1);
			CHECK(pool.arraySize() == 1);
			CHECK(!pool.slotIsActive(handle.idx()));
			CHECK(pool.data()[handle.idx()] == i);
		}

		SfzHandle handle = pool.allocate();
		CHECK(pool.handleIsValid(handle));
		CHECK(handle.idx() == 0);
		CHECK(handle.version() == u8(1));
		CHECK(pool.numAllocated() == 1);
		CHECK(pool.numHoles() == 0);
		CHECK(pool.arraySize() == 1);
		CHECK(pool.slotIsActive(handle.idx()));
		CHECK(pool.getVersion(handle.idx()) == handle.version());

		pool.deallocate(handle);
		CHECK(!pool.handleIsValid(handle));
		CHECK(pool.numAllocated() == 0);
		CHECK(pool.numHoles() == 1);
		CHECK(pool.arraySize() == 1);
		CHECK(!pool.slotIsActive(handle.idx()));
		CHECK(pool.data()[handle.idx()] == u8(0));
	}

	// Allocate full, deallocate full, and then allocate full again
	{
		constexpr u32 CAPACITY = 64;
		sfz::Pool<u32> pool;
		pool.init(CAPACITY, &allocator, sfz_dbg(""));

		for (u32 i = 0; i < CAPACITY; i++) {
			SfzHandle handle = pool.allocate();
			pool[handle] = i;
		}
		CHECK(pool.numAllocated() == CAPACITY);
		CHECK(pool.numHoles() == 0);
		CHECK(pool.arraySize() == CAPACITY);

		for (u32 i = 0; i < CAPACITY; i++) {
			SfzHandle handle = sfzHandleInit(i, 1);
			CHECK(pool.handleIsValid(handle));
			CHECK(*pool.get(handle) == i);
			pool.deallocate(i);
		}
		CHECK(pool.numAllocated() == 0);
		CHECK(pool.numHoles() == CAPACITY);
		CHECK(pool.arraySize() == CAPACITY);

		for (u32 i = 0; i < CAPACITY; i++) {
			SfzHandle handle = pool.allocate(42 + i);
			CHECK(pool[handle] == (42 + i));
			CHECK(handle.idx() == (CAPACITY - i - 1));
			CHECK(handle.version() == 2);
		}
		CHECK(pool.numAllocated() == CAPACITY);
		CHECK(pool.numHoles() == 0);
		CHECK(pool.arraySize() == CAPACITY);
	}
}
