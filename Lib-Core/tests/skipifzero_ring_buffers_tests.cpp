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

#include <chrono>
#include <thread>
#include <utility>

#include "skipifzero.hpp"
#include "skipifzero_allocators.hpp"
#include "skipifzero_new.hpp"

#define private public
#include "skipifzero_ring_buffers.hpp"
#undef private

using namespace sfz;

UTEST(RingBuffer, constructors)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Default constructor
	{
		RingBuffer<i32> buffer;
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.capacity() == 0);
		ASSERT_TRUE(buffer.allocator() == nullptr);
		ASSERT_TRUE(buffer.mDataPtr == nullptr);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
	// No capacity init
	{
		RingBuffer<i32> buffer(0, &allocator, sfz_dbg(""));
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.capacity() == 0);
		ASSERT_TRUE(buffer.allocator() == nullptr);
		ASSERT_TRUE(buffer.mDataPtr == nullptr);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
	// Init with capacity
	{
		RingBuffer<i32> buffer(32, &allocator, sfz_dbg(""));
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.capacity() == 32);
		ASSERT_TRUE(buffer.allocator() == &allocator);
		ASSERT_TRUE(buffer.mDataPtr != nullptr);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
}

UTEST(RingBuffer, adding_and_accessing_elements)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Capacity == 0
	{
		RingBuffer<i32> buffer;

		ASSERT_TRUE(buffer.capacity() == 0);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);

		ASSERT_TRUE(!buffer.pop());
		ASSERT_TRUE(buffer.capacity() == 0);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);

		i32 res1 = 32;
		ASSERT_TRUE(!buffer.pop(res1));
		ASSERT_TRUE(res1 == 32);
		ASSERT_TRUE(buffer.capacity() == 0);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);

		ASSERT_TRUE(!buffer.popLast());
		ASSERT_TRUE(buffer.capacity() == 0);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);

		i32 res2 = 27;
		ASSERT_TRUE(!buffer.popLast(res2));
		ASSERT_TRUE(res2 == 27);
		ASSERT_TRUE(buffer.capacity() == 0);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
	// Capacity == 1
	{
		RingBuffer<i32> buffer(1, &allocator, sfz_dbg(""));
		ASSERT_TRUE(buffer.capacity() == 1);

		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.add(24));
		ASSERT_TRUE(buffer.size() == 1);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		ASSERT_TRUE(buffer.mapIndex(buffer.mLastIndex) == 0);
		ASSERT_TRUE(buffer.first() == 24);
		ASSERT_TRUE(buffer.last() == 24);
		ASSERT_TRUE(buffer[0] == 24);

		ASSERT_TRUE(!buffer.add(36));
		ASSERT_TRUE(buffer.size() == 1);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		ASSERT_TRUE(buffer.mapIndex(buffer.mLastIndex) == 0);
		ASSERT_TRUE(buffer.first() == 24);
		ASSERT_TRUE(buffer.last() == 24);
		ASSERT_TRUE(buffer[0] == 24);

		i32 res = 0;
		ASSERT_TRUE(buffer.pop(res));
		ASSERT_TRUE(res == 24);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		ASSERT_TRUE(buffer.mapIndex(buffer.mLastIndex) == 0);

		ASSERT_TRUE(!buffer.pop());
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		ASSERT_TRUE(buffer.mapIndex(buffer.mLastIndex) == 0);

		ASSERT_TRUE(buffer.add(36));
		ASSERT_TRUE(buffer.size() == 1);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));
		ASSERT_TRUE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		ASSERT_TRUE(buffer.mapIndex(buffer.mLastIndex) == 0);
		ASSERT_TRUE(buffer.first() == 36);
		ASSERT_TRUE(buffer.last() == 36);
		ASSERT_TRUE(buffer[0] == 36);

		i32 res2 = 0;
		ASSERT_TRUE(buffer.popLast(res2));
		ASSERT_TRUE(res2 == 36);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		ASSERT_TRUE(buffer.mapIndex(buffer.mLastIndex) == 0);

		ASSERT_TRUE(!buffer.popLast());
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		ASSERT_TRUE(buffer.mapIndex(buffer.mLastIndex) == 0);

		ASSERT_TRUE(buffer.addFirst(12));
		ASSERT_TRUE(buffer.size() == 1);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + 0));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		ASSERT_TRUE(buffer.mapIndex(buffer.mLastIndex) == 0);
		ASSERT_TRUE(buffer.first() == 12);
		ASSERT_TRUE(buffer.last() == 12);
		ASSERT_TRUE(buffer[0] == 12);
	}
	// Capacity == 2, add()
	{
		RingBuffer<i32> buffer(2, &allocator, sfz_dbg(""));
		ASSERT_TRUE(buffer.capacity() == 2);

		ASSERT_TRUE(buffer.add(3));
		ASSERT_TRUE(buffer.size() == 1);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 1));
		ASSERT_TRUE(buffer.first() == 3);
		ASSERT_TRUE(buffer.last() == 3);
		ASSERT_TRUE(buffer[0] == 3);

		ASSERT_TRUE(buffer.add(4));
		ASSERT_TRUE(buffer.size() == 2);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));
		ASSERT_TRUE(buffer.first() == 3);
		ASSERT_TRUE(buffer.last() == 4);
		ASSERT_TRUE(buffer[0] == 3);
		ASSERT_TRUE(buffer[1] == 4);

		ASSERT_TRUE(!buffer.add(4));
		ASSERT_TRUE(buffer.size() == 2);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));
		ASSERT_TRUE(buffer.first() == 3);
		ASSERT_TRUE(buffer.last() == 4);
		ASSERT_TRUE(buffer[0] == 3);
		ASSERT_TRUE(buffer[1] == 4);

		int res1 = 0;
		ASSERT_TRUE(buffer.pop(res1));
		ASSERT_TRUE(res1 == 3);
		ASSERT_TRUE(buffer.size() == 1);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX + 1);
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));
		ASSERT_TRUE(buffer.first() == 4);
		ASSERT_TRUE(buffer.last() == 4);
		ASSERT_TRUE(buffer[0] == 4);

		ASSERT_TRUE(buffer.add(5));
		ASSERT_TRUE(buffer.size() == 2);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX + 1);
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 3));
		ASSERT_TRUE(buffer.first() == 4);
		ASSERT_TRUE(buffer.last() == 5);
		ASSERT_TRUE(buffer[0] == 4);
		ASSERT_TRUE(buffer[1] == 5);
	}
	// Capacity == 2, addFirst()
	{
		RingBuffer<i32> buffer(2, &allocator, sfz_dbg(""));
		ASSERT_TRUE(buffer.capacity() == 2);

		ASSERT_TRUE(buffer.addFirst(3));
		ASSERT_TRUE(buffer.size() == 1);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX - 1);
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX));
		ASSERT_TRUE(buffer.first() == 3);
		ASSERT_TRUE(buffer.last() == 3);
		ASSERT_TRUE(buffer[0] == 3);

		ASSERT_TRUE(buffer.addFirst(4));
		ASSERT_TRUE(buffer.size() == 2);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - 2));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 0));
		ASSERT_TRUE(buffer.first() == 4);
		ASSERT_TRUE(buffer.last() == 3);
		ASSERT_TRUE(buffer[0] == 4);
		ASSERT_TRUE(buffer[1] == 3);

		ASSERT_TRUE(!buffer.addFirst(5));
		ASSERT_TRUE(buffer.size() == 2);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - 2));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 0));
		ASSERT_TRUE(buffer.first() == 4);
		ASSERT_TRUE(buffer.last() == 3);
		ASSERT_TRUE(buffer[0] == 4);
		ASSERT_TRUE(buffer[1] == 3);

		int res1 = 0;
		ASSERT_TRUE(buffer.popLast(res1));
		ASSERT_TRUE(res1 == 3);
		ASSERT_TRUE(buffer.size() == 1);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - 2));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX - 1));
		ASSERT_TRUE(buffer.first() == 4);
		ASSERT_TRUE(buffer.last() == 4);
		ASSERT_TRUE(buffer[0] == 4);
	}
}

UTEST(RingBuffer, state_methods)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// swap() and move constructors
	{
		RingBuffer<UniquePtr<i32>> buffer(3, &allocator, sfz_dbg(""));
		ASSERT_TRUE(buffer.add(makeUnique<i32>(&allocator, sfz_dbg(""), 2)));
		ASSERT_TRUE(*buffer[0] == 2);
		{
			RingBuffer<UniquePtr<i32>> buffer2;
			buffer2 = std::move(buffer);
			ASSERT_TRUE(buffer.size() == 0);
			ASSERT_TRUE(buffer2.size() == 1);
			ASSERT_TRUE(*buffer2[0] == 2);
		}
	}
	// clear()
	{
		RingBuffer<UniquePtr<i32>> buffer(2, &allocator, sfz_dbg(""));
		ASSERT_TRUE(buffer.add(makeUnique<i32>(&allocator, sfz_dbg(""), 2)));
		ASSERT_TRUE(buffer.add(makeUnique<i32>(&allocator, sfz_dbg(""), 3)));
		ASSERT_TRUE(*buffer.first() == 2);
		ASSERT_TRUE(*buffer.last() == 3);
		ASSERT_TRUE(buffer.size() == 2);
		ASSERT_TRUE(*buffer[0] == 2);
		ASSERT_TRUE(*buffer[1] == 3);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + 2));

		buffer.clear();
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.capacity() == 2);
		ASSERT_TRUE(buffer.allocator() == &allocator);
		ASSERT_TRUE(buffer.mFirstIndex == RingBuffer<i32>::BASE_IDX);
		ASSERT_TRUE(buffer.mLastIndex == RingBuffer<i32>::BASE_IDX);
	}
}

#ifndef __EMSCRIPTEN__
#ifdef NDEBUG
UTEST(RingBuffer, multi_threading)
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

		for (bool& b : results) ASSERT_TRUE(b);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + NUM_RESULTS));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + NUM_RESULTS));
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

		for (bool& b : results) ASSERT_TRUE(b);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX + NUM_RESULTS));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + NUM_RESULTS));
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

		for (bool& b : results) ASSERT_TRUE(b);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - NUM_RESULTS));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX - NUM_RESULTS));
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

		for (bool& b : results) ASSERT_TRUE(b);
		ASSERT_TRUE(buffer.size() == 0);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - NUM_RESULTS));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX - NUM_RESULTS));
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
		ASSERT_TRUE(!producerFirstNoSuccess);
		ASSERT_TRUE(!producerLastNoSuccess);
		ASSERT_TRUE(buffer.size() == NUM_RESULTS);
		ASSERT_TRUE(buffer.mFirstIndex == (RingBuffer<i32>::BASE_IDX - HALF_NUM_RESULTS));
		ASSERT_TRUE(buffer.mLastIndex == (RingBuffer<i32>::BASE_IDX + HALF_NUM_RESULTS));
		for (u64 i = 0; i < HALF_NUM_RESULTS; i++) {
			ASSERT_TRUE(buffer[i] == i32(HALF_NUM_RESULTS - i - 1));
		}
		for (u64 i = 0; i < HALF_NUM_RESULTS; i++) {
			ASSERT_TRUE(buffer[HALF_NUM_RESULTS + i] == i32(i));
		}
	}
}
#endif
#endif
