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
	sfz::StandardAllocator allocator;

	// Default constructed
	{
		sfz::Pool<uint64_t> pool;
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
		sfz::Pool<uint64_t> pool;
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
		sfz::Pool<uint64_t> pool = sfz::Pool<uint64_t>(13, &allocator, sfz_dbg(""));
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
	sfz::StandardAllocator allocator;

	// Allocating to full capacity linearly
	{
		constexpr uint32_t CAPACITY = 64;
		sfz::Pool<uint32_t> pool;
		pool.init(CAPACITY, &allocator, sfz_dbg(""));

		for (uint32_t i = 0; i < CAPACITY; i++) {
			sfz::PoolHandle handle = pool.allocate();
			uint32_t& val = pool[handle];
			val = i;
			ASSERT_TRUE(handle.idx() == i);
			ASSERT_TRUE(handle.version() == uint8_t(1));
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
		constexpr uint32_t CAPACITY = 4;
		sfz::Pool<uint32_t> pool;
		pool.init(CAPACITY, &allocator, sfz_dbg(""));

		for (uint32_t i = 1; i <= 127; i++) {
			sfz::PoolHandle handle = pool.allocate();
			ASSERT_TRUE(pool.handleIsValid(handle));
			ASSERT_TRUE(handle.idx() == 0);
			ASSERT_TRUE(handle.version() == uint8_t(i));
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

		sfz::PoolHandle handle = pool.allocate();
		ASSERT_TRUE(pool.handleIsValid(handle));
		ASSERT_TRUE(handle.idx() == 0);
		ASSERT_TRUE(handle.version() == uint8_t(1));
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
		ASSERT_TRUE(pool.data()[handle.idx()] == uint8_t(0));
	}

	// Allocate full, deallocate full, and then allocate full again
	{
		constexpr uint32_t CAPACITY = 64;
		sfz::Pool<uint32_t> pool;
		pool.init(CAPACITY, &allocator, sfz_dbg(""));

		for (uint32_t i = 0; i < CAPACITY; i++) {
			sfz::PoolHandle handle = pool.allocate();
			pool[handle] = i;
		}
		ASSERT_TRUE(pool.numAllocated() == CAPACITY);
		ASSERT_TRUE(pool.numHoles() == 0);
		ASSERT_TRUE(pool.arraySize() == CAPACITY);

		for (uint32_t i = 0; i < CAPACITY; i++) {
			sfz::PoolHandle handle = sfz::PoolHandle(i, 1);
			ASSERT_TRUE(pool.handleIsValid(handle));
			ASSERT_TRUE(*pool.get(handle) == i);
			pool.deallocate(i);
		}
		ASSERT_TRUE(pool.numAllocated() == 0);
		ASSERT_TRUE(pool.numHoles() == CAPACITY);
		ASSERT_TRUE(pool.arraySize() == CAPACITY);

		for (uint32_t i = 0; i < CAPACITY; i++) {
			sfz::PoolHandle handle = pool.allocate(42 + i);
			ASSERT_TRUE(pool[handle] == (42 + i));
			ASSERT_TRUE(handle.idx() == (CAPACITY - i - 1));
			ASSERT_TRUE(handle.version() == 2);
		}
		ASSERT_TRUE(pool.numAllocated() == CAPACITY);
		ASSERT_TRUE(pool.numHoles() == 0);
		ASSERT_TRUE(pool.arraySize() == CAPACITY);
	}
}
