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

#include "skipifzero.hpp"
#include "skipifzero_allocators.hpp"
#include "skipifzero_pool.hpp"

// Pool tests
// ------------------------------------------------------------------------------------------------

UTEST(Pool, init)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Default constructed
	{
		sfz::Pool<u64> pool;
		ASSERT_TRUE(pool.numAllocated() == 0);
		ASSERT_TRUE(pool.numHoles() == 0);
		ASSERT_TRUE(pool.arraySize() == 0);
		ASSERT_TRUE(pool.capacity() == 0);
		ASSERT_TRUE(pool.data() == nullptr);
		ASSERT_TRUE(pool.slots() == nullptr);
		ASSERT_TRUE(pool.allocator() == nullptr);
	}

	// Init method
	{
		sfz::Pool<u64> pool;
		pool.init(42, &allocator, sfz_dbg(""));
		ASSERT_TRUE(pool.numAllocated() == 0);
		ASSERT_TRUE(pool.numHoles() == 0);
		ASSERT_TRUE(pool.arraySize() == 0);
		ASSERT_TRUE(pool.capacity() == 42);
		ASSERT_TRUE(pool.data() != nullptr);
		ASSERT_TRUE(pool.slots() != nullptr);
		ASSERT_TRUE(pool.allocator() != nullptr);
	}

	// Init constructor
	{
		sfz::Pool<u64> pool = sfz::Pool<u64>(13, &allocator, sfz_dbg(""));
		ASSERT_TRUE(pool.numAllocated() == 0);
		ASSERT_TRUE(pool.numHoles() == 0);
		ASSERT_TRUE(pool.arraySize() == 0);
		ASSERT_TRUE(pool.capacity() == 13);
		ASSERT_TRUE(pool.data() != nullptr);
		ASSERT_TRUE(pool.slots() != nullptr);
		ASSERT_TRUE(pool.allocator() != nullptr);
	}
}

UTEST(Pool, allocating_and_deallocating)
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
			ASSERT_TRUE(handle.idx() == i);
			ASSERT_TRUE(handle.version() == u8(1));
			ASSERT_TRUE(pool.numAllocated() == (i + 1));
			ASSERT_TRUE(pool.numHoles() == 0);
			ASSERT_TRUE(pool.slotIsActive(handle.idx()));
			ASSERT_TRUE(pool.getVersion(handle.idx()) == handle.version());
		}
		ASSERT_TRUE(pool.numAllocated() == CAPACITY);
		ASSERT_TRUE(pool.numHoles() == 0);
	}

	// Allocating and deallocating a single slot until version wraps around
	{
		constexpr u32 CAPACITY = 4;
		sfz::Pool<u32> pool;
		pool.init(CAPACITY, &allocator, sfz_dbg(""));

		for (u32 i = 1; i <= 127; i++) {
			SfzHandle handle = pool.allocate();
			ASSERT_TRUE(pool.handleIsValid(handle));
			ASSERT_TRUE(handle.idx() == 0);
			ASSERT_TRUE(handle.version() == u8(i));
			ASSERT_TRUE(pool.numAllocated() == 1);
			ASSERT_TRUE(pool.numHoles() == 0);
			ASSERT_TRUE(pool.arraySize() == 1);
			ASSERT_TRUE(pool.slotIsActive(handle.idx()));
			ASSERT_TRUE(pool.getVersion(handle.idx()) == handle.version());

			pool.deallocate(handle, i);
			ASSERT_TRUE(!pool.handleIsValid(handle));
			ASSERT_TRUE(pool.numAllocated() == 0);
			ASSERT_TRUE(pool.numHoles() == 1);
			ASSERT_TRUE(pool.arraySize() == 1);
			ASSERT_TRUE(!pool.slotIsActive(handle.idx()));
			ASSERT_TRUE(pool.data()[handle.idx()] == i);
		}

		SfzHandle handle = pool.allocate();
		ASSERT_TRUE(pool.handleIsValid(handle));
		ASSERT_TRUE(handle.idx() == 0);
		ASSERT_TRUE(handle.version() == u8(1));
		ASSERT_TRUE(pool.numAllocated() == 1);
		ASSERT_TRUE(pool.numHoles() == 0);
		ASSERT_TRUE(pool.arraySize() == 1);
		ASSERT_TRUE(pool.slotIsActive(handle.idx()));
		ASSERT_TRUE(pool.getVersion(handle.idx()) == handle.version());

		pool.deallocate(handle);
		ASSERT_TRUE(!pool.handleIsValid(handle));
		ASSERT_TRUE(pool.numAllocated() == 0);
		ASSERT_TRUE(pool.numHoles() == 1);
		ASSERT_TRUE(pool.arraySize() == 1);
		ASSERT_TRUE(!pool.slotIsActive(handle.idx()));
		ASSERT_TRUE(pool.data()[handle.idx()] == u8(0));
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
		ASSERT_TRUE(pool.numAllocated() == CAPACITY);
		ASSERT_TRUE(pool.numHoles() == 0);
		ASSERT_TRUE(pool.arraySize() == CAPACITY);

		for (u32 i = 0; i < CAPACITY; i++) {
			SfzHandle handle = SfzHandle::create(i, 1);
			ASSERT_TRUE(pool.handleIsValid(handle));
			ASSERT_TRUE(*pool.get(handle) == i);
			pool.deallocate(i);
		}
		ASSERT_TRUE(pool.numAllocated() == 0);
		ASSERT_TRUE(pool.numHoles() == CAPACITY);
		ASSERT_TRUE(pool.arraySize() == CAPACITY);

		for (u32 i = 0; i < CAPACITY; i++) {
			SfzHandle handle = pool.allocate(42 + i);
			ASSERT_TRUE(pool[handle] == (42 + i));
			ASSERT_TRUE(handle.idx() == (CAPACITY - i - 1));
			ASSERT_TRUE(handle.version() == 2);
		}
		ASSERT_TRUE(pool.numAllocated() == CAPACITY);
		ASSERT_TRUE(pool.numHoles() == 0);
		ASSERT_TRUE(pool.arraySize() == CAPACITY);
	}
}
