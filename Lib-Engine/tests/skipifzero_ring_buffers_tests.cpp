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

#include <thread>
#include <utility>

#include "sfz.h"
#include "sfz_unique_ptr.hpp"
#include "skipifzero_allocators.hpp"

#define private public
#include "skipifzero_ring_buffers.hpp"
#undef private

using namespace sfz;

TEST_CASE("RingBuffer: constructors")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Default constructor
	{
		RingBuffer<i32> buffer;
		CHECK(buffer.size() == 0);
		CHECK(buffer.capacity() == 0);
		CHECK(buffer.allocator() == nullptr);
		CHECK(buffer.mDataPtr == nullptr);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
	// No capacity init
	{
		RingBuffer<i32> buffer(0, &allocator, sfz_dbg(""));
		CHECK(buffer.size() == 0);
		CHECK(buffer.capacity() == 0);
		CHECK(buffer.allocator() == nullptr);
		CHECK(buffer.mDataPtr == nullptr);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
	// Init with capacity
	{
		RingBuffer<i32> buffer(32, &allocator, sfz_dbg(""));
		CHECK(buffer.size() == 0);
		CHECK(buffer.capacity() == 32);
		CHECK(buffer.allocator() == &allocator);
		CHECK(buffer.mDataPtr != nullptr);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
}

TEST_CASE("RingBuffer: adding_and_accessing_elements")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Capacity == 0
	{
		RingBuffer<i32> buffer;

		CHECK(buffer.capacity() == 0);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);

		CHECK(!buffer.pop());
		CHECK(buffer.capacity() == 0);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);

		i32 res1 = 32;
		CHECK(!buffer.pop(res1));
		CHECK(res1 == 32);
		CHECK(buffer.capacity() == 0);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);

		CHECK(!buffer.popLast());
		CHECK(buffer.capacity() == 0);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);

		i32 res2 = 27;
		CHECK(!buffer.popLast(res2));
		CHECK(res2 == 27);
		CHECK(buffer.capacity() == 0);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
	// Capacity == 1
	{
		RingBuffer<i32> buffer(1, &allocator, sfz_dbg(""));
		CHECK(buffer.capacity() == 1);

		CHECK(buffer.size() == 0);
		CHECK(buffer.add(24));
		CHECK(buffer.size() == 1);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mapIndex(buffer.mFirstIndex) == 0);
		CHECK(buffer.mapIndex(buffer.mLastIndex) == 0);
		CHECK(buffer.first() == 24);
		CHECK(buffer.last() == 24);
		CHECK(buffer[0] == 24);

		CHECK(!buffer.add(36));
		CHECK(buffer.size() == 1);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mapIndex(buffer.mFirstIndex) == 0);
		CHECK(buffer.mapIndex(buffer.mLastIndex) == 0);
		CHECK(buffer.first() == 24);
		CHECK(buffer.last() == 24);
		CHECK(buffer[0] == 24);

		i32 res = 0;
		CHECK(buffer.pop(res));
		CHECK(res == 24);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mapIndex(buffer.mFirstIndex) == 0);
		CHECK(buffer.mapIndex(buffer.mLastIndex) == 0);

		CHECK(!buffer.pop());
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mapIndex(buffer.mFirstIndex) == 0);
		CHECK(buffer.mapIndex(buffer.mLastIndex) == 0);

		CHECK(buffer.add(36));
		CHECK(buffer.size() == 1);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));
		CHECK(buffer.mapIndex(buffer.mFirstIndex) == 0);
		CHECK(buffer.mapIndex(buffer.mLastIndex) == 0);
		CHECK(buffer.first() == 36);
		CHECK(buffer.last() == 36);
		CHECK(buffer[0] == 36);

		i32 res2 = 0;
		CHECK(buffer.popLast(res2));
		CHECK(res2 == 36);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mapIndex(buffer.mFirstIndex) == 0);
		CHECK(buffer.mapIndex(buffer.mLastIndex) == 0);

		CHECK(!buffer.popLast());
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mapIndex(buffer.mFirstIndex) == 0);
		CHECK(buffer.mapIndex(buffer.mLastIndex) == 0);

		CHECK(buffer.addFirst(12));
		CHECK(buffer.size() == 1);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 0));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.mapIndex(buffer.mFirstIndex) == 0);
		CHECK(buffer.mapIndex(buffer.mLastIndex) == 0);
		CHECK(buffer.first() == 12);
		CHECK(buffer.last() == 12);
		CHECK(buffer[0] == 12);
	}
	// Capacity == 2, add()
	{
		RingBuffer<i32> buffer(2, &allocator, sfz_dbg(""));
		CHECK(buffer.capacity() == 2);

		CHECK(buffer.add(3));
		CHECK(buffer.size() == 1);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		CHECK(buffer.first() == 3);
		CHECK(buffer.last() == 3);
		CHECK(buffer[0] == 3);

		CHECK(buffer.add(4));
		CHECK(buffer.size() == 2);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));
		CHECK(buffer.first() == 3);
		CHECK(buffer.last() == 4);
		CHECK(buffer[0] == 3);
		CHECK(buffer[1] == 4);

		CHECK(!buffer.add(4));
		CHECK(buffer.size() == 2);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));
		CHECK(buffer.first() == 3);
		CHECK(buffer.last() == 4);
		CHECK(buffer[0] == 3);
		CHECK(buffer[1] == 4);

		int res1 = 0;
		CHECK(buffer.pop(res1));
		CHECK(res1 == 3);
		CHECK(buffer.size() == 1);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX + 1);
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));
		CHECK(buffer.first() == 4);
		CHECK(buffer.last() == 4);
		CHECK(buffer[0] == 4);

		CHECK(buffer.add(5));
		CHECK(buffer.size() == 2);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX + 1);
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 3));
		CHECK(buffer.first() == 4);
		CHECK(buffer.last() == 5);
		CHECK(buffer[0] == 4);
		CHECK(buffer[1] == 5);
	}
	// Capacity == 2, addFirst()
	{
		RingBuffer<i32> buffer(2, &allocator, sfz_dbg(""));
		CHECK(buffer.capacity() == 2);

		CHECK(buffer.addFirst(3));
		CHECK(buffer.size() == 1);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX - 1);
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX));
		CHECK(buffer.first() == 3);
		CHECK(buffer.last() == 3);
		CHECK(buffer[0] == 3);

		CHECK(buffer.addFirst(4));
		CHECK(buffer.size() == 2);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - 2));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 0));
		CHECK(buffer.first() == 4);
		CHECK(buffer.last() == 3);
		CHECK(buffer[0] == 4);
		CHECK(buffer[1] == 3);

		CHECK(!buffer.addFirst(5));
		CHECK(buffer.size() == 2);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - 2));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 0));
		CHECK(buffer.first() == 4);
		CHECK(buffer.last() == 3);
		CHECK(buffer[0] == 4);
		CHECK(buffer[1] == 3);

		int res1 = 0;
		CHECK(buffer.popLast(res1));
		CHECK(res1 == 3);
		CHECK(buffer.size() == 1);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - 2));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX - 1));
		CHECK(buffer.first() == 4);
		CHECK(buffer.last() == 4);
		CHECK(buffer[0] == 4);
	}
}

TEST_CASE("RingBuffer: state_methods")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// swap() and move constructors
	{
		RingBuffer<SfzUniquePtr<i32>> buffer(3, &allocator, sfz_dbg(""));
		CHECK(buffer.add(sfzMakeUnique<i32>(&allocator, sfz_dbg(""), 2)));
		CHECK(*buffer[0] == 2);
		{
			RingBuffer<SfzUniquePtr<i32>> buffer2;
			buffer2 = std::move(buffer);
			CHECK(buffer.size() == 0);
			CHECK(buffer2.size() == 1);
			CHECK(*buffer2[0] == 2);
		}
	}
	// clear()
	{
		RingBuffer<SfzUniquePtr<i32>> buffer(2, &allocator, sfz_dbg(""));
		CHECK(buffer.add(sfzMakeUnique<i32>(&allocator, sfz_dbg(""), 2)));
		CHECK(buffer.add(sfzMakeUnique<i32>(&allocator, sfz_dbg(""), 3)));
		CHECK(*buffer.first() == 2);
		CHECK(*buffer.last() == 3);
		CHECK(buffer.size() == 2);
		CHECK(*buffer[0] == 2);
		CHECK(*buffer[1] == 3);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));

		buffer.clear();
		CHECK(buffer.size() == 0);
		CHECK(buffer.capacity() == 2);
		CHECK(buffer.allocator() == &allocator);
		CHECK(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		CHECK(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
}

#ifdef NDEBUG
TEST_CASE("RingBuffer: multi_threading")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Slow Producer & fast consumer (add() & pop())
	{
		constexpr u64 NUM_RESULTS = 1024;
		RingBuffer<i32> buffer(16, &allocator, sfz_dbg(""));
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		std::thread producer([&]() {
			for (i32 i = 0; i < i32(NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				bool success = buffer.add(i);
				if (!success) i--;
			}
		});

		std::thread consumer([&]() {
			for (i32 i = 0; i < i32(NUM_RESULTS); i++) {
				i32 out = -1;
				bool success = buffer.pop(out);
				if (success) {
					results[i] = out == i;
				}
				else {
					i--;
				}
			}
		});

		producer.join();
		consumer.join();

		for (bool& b : results) CHECK(b);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + NUM_RESULTS));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + NUM_RESULTS));
	}
	// Fast Producer & slow consumer (add() & pop())
	{
		constexpr u64 NUM_RESULTS = 1024;
		RingBuffer<i32> buffer(16, &allocator, sfz_dbg(""));
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		std::thread producer([&]() {
			for (i32 i = 0; i < i32(NUM_RESULTS); i++) {
				bool success = buffer.add(i);
				if (!success) i--;
			}
		});

		std::thread consumer([&]() {
			for (i32 i = 0; i < i32(NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				i32 out = -1;
				bool success = buffer.pop(out);
				if (success) {
					results[i] = out == i;
				}
				else {
					i--;
				}
			}
		});

		producer.join();
		consumer.join();

		for (bool& b : results) CHECK(b);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + NUM_RESULTS));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + NUM_RESULTS));
	}
	// Slow Producer & fast consumer (addFirst() & popLast())
	{
		constexpr u64 NUM_RESULTS = 1024;
		RingBuffer<i32> buffer(16, &allocator, sfz_dbg(""));
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		std::thread producer([&]() {
			for (i32 i = 0; i < i32(NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				bool success = buffer.addFirst(i);
				if (!success) i--;
			}
		});

		std::thread consumer([&]() {
			for (i32 i = 0; i < i32(NUM_RESULTS); i++) {
				i32 out = -1;
				bool success = buffer.popLast(out);
				if (success) {
					results[i] = out == i;
				}
				else {
					i--;
				}
			}
		});

		producer.join();
		consumer.join();

		for (bool& b : results) CHECK(b);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - NUM_RESULTS));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX - NUM_RESULTS));
	}
	// Fast Producer & slow consumer (addFirst() & popLast())
	{
		constexpr u64 NUM_RESULTS = 1024;
		RingBuffer<i32> buffer(16, &allocator, sfz_dbg(""));
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		std::thread producer([&]() {
			for (i32 i = 0; i < i32(NUM_RESULTS); i++) {
				bool success = buffer.addFirst(i);
				if (!success) i--;
			}
		});

		std::thread consumer([&]() {
			for (i32 i = 0; i < i32(NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				i32 out = -1;
				bool success = buffer.popLast(out);
				if (success) {
					results[i] = out == i;
				}
				else {
					i--;
				}
			}
		});

		producer.join();
		consumer.join();

		for (bool& b : results) CHECK(b);
		CHECK(buffer.size() == 0);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - NUM_RESULTS));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX - NUM_RESULTS));
	}
	// Two producers (add() & addFirst())
	{
		constexpr u64 NUM_RESULTS = 1024;
		constexpr u64 HALF_NUM_RESULTS = NUM_RESULTS / 2;
		RingBuffer<i32> buffer(NUM_RESULTS, &allocator, sfz_dbg(""));
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		bool producerFirstNoSuccess = false;
		std::thread producerFirst([&]() {
			for (i32 i = 0; i < i32(HALF_NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				bool success = buffer.addFirst(i);
				if (!success) {
					producerFirstNoSuccess = true;
					break;
				}
			}
		});

		bool producerLastNoSuccess = false;
		std::thread producerLast([&]() {
			for (i32 i = 0; i < i32(HALF_NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				bool success = buffer.add(i);
				if (!success) {
					producerLastNoSuccess = true;
					break;
				}
			}
		});

		producerFirst.join();
		producerLast.join();
		CHECK(!producerFirstNoSuccess);
		CHECK(!producerLastNoSuccess);
		CHECK(buffer.size() == NUM_RESULTS);
		CHECK(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - HALF_NUM_RESULTS));
		CHECK(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + HALF_NUM_RESULTS));
		for (u64 i = 0; i < HALF_NUM_RESULTS; i++) {
			CHECK(buffer[i] == i32(HALF_NUM_RESULTS - i - 1));
		}
		for (u64 i = 0; i < HALF_NUM_RESULTS; i++) {
			CHECK(buffer[HALF_NUM_RESULTS + i] == i32(i));
		}
	}
}
#endif
