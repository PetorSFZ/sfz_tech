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

#include "sfz/PushWarnings.hpp"
#include "catch2/catch.hpp"
#include "sfz/PopWarnings.hpp"

#include <chrono>
#include <thread>

#include <skipifzero_smart_pointers.hpp>

#define private public
#include "sfz/containers/RingBuffer.hpp"
#undef private
#include "sfz/memory/DebugAllocator.hpp"

using namespace sfz;

TEST_CASE("RingBuffer: Constructors", "[sfz::RingBuffer]")
{
	sfz::setContext(sfz::getStandardContext());
	REQUIRE(getDefaultAllocator() != nullptr);

	SECTION("Default constructor") {
		RingBuffer<int32_t> buffer;
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.capacity() == 0);
		REQUIRE(buffer.allocator() == nullptr);
		REQUIRE(buffer.mDataPtr == nullptr);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);
	}
	SECTION("No capacity default allocator") {
		RingBuffer<int32_t> buffer(0);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.capacity() == 0);
		REQUIRE(buffer.allocator() == getDefaultAllocator());
		REQUIRE(buffer.mDataPtr == nullptr);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);
	}
	SECTION("No capacity non-default allocator") {
		DebugAllocator alloc("debug allocator");
		REQUIRE(alloc.numAllocations() == 0);
		RingBuffer<int32_t> buffer(0, &alloc);
		REQUIRE(alloc.numAllocations() == 0);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.capacity() == 0);
		REQUIRE(buffer.allocator() == &alloc);
		REQUIRE(buffer.mDataPtr == nullptr);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);
	}
	SECTION("Default allocator with capacity") {
		RingBuffer<int32_t> buffer(32);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.capacity() == 32);
		REQUIRE(buffer.allocator() == getDefaultAllocator());
		REQUIRE(buffer.mDataPtr != nullptr);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);
	}
	SECTION("Non-default allocator with capacity") {
		DebugAllocator alloc("debug allocator");
		REQUIRE(alloc.numAllocations() == 0);
		{
			RingBuffer<int32_t> buffer(32, &alloc);
			REQUIRE(alloc.numAllocations() == 1);
			REQUIRE(buffer.size() == 0);
			REQUIRE(buffer.capacity() == 32);
			REQUIRE(buffer.allocator() == &alloc);
			REQUIRE(buffer.mDataPtr != nullptr);
			REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
			REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);
		}
		REQUIRE(alloc.numAllocations() == 0);
	}
}

TEST_CASE("RingBuffer: Adding and accessing elements", "[sfz:RingBuffer]")
{
	sfz::setContext(sfz::getStandardContext());

	SECTION("Capacity == 0") {
		RingBuffer<int32_t> buffer;

		REQUIRE(buffer.capacity() == 0);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);

		REQUIRE(!buffer.pop());
		REQUIRE(buffer.capacity() == 0);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);

		int32_t res1 = 32;
		REQUIRE(!buffer.pop(res1));
		REQUIRE(res1 == 32);
		REQUIRE(buffer.capacity() == 0);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);

		REQUIRE(!buffer.popLast());
		REQUIRE(buffer.capacity() == 0);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);

		int32_t res2 = 27;
		REQUIRE(!buffer.popLast(res2));
		REQUIRE(res2 == 27);
		REQUIRE(buffer.capacity() == 0);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);
	}
	SECTION("Capacity == 1") {
		RingBuffer<int32_t> buffer(1);
		REQUIRE(buffer.capacity() == 1);

		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.add(24));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		REQUIRE(buffer.mapIndex(buffer.mLastIndex) == 0);
		REQUIRE(buffer.first() == 24);
		REQUIRE(buffer.last() == 24);
		REQUIRE(buffer[0] == 24);

		REQUIRE(!buffer.add(36));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		REQUIRE(buffer.mapIndex(buffer.mLastIndex) == 0);
		REQUIRE(buffer.first() == 24);
		REQUIRE(buffer.last() == 24);
		REQUIRE(buffer[0] == 24);

		int32_t res = 0;
		REQUIRE(buffer.pop(res));
		REQUIRE(res == 24);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		REQUIRE(buffer.mapIndex(buffer.mLastIndex) == 0);

		REQUIRE(!buffer.pop());
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		REQUIRE(buffer.mapIndex(buffer.mLastIndex) == 0);

		REQUIRE(buffer.add(36));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 2));
		REQUIRE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		REQUIRE(buffer.mapIndex(buffer.mLastIndex) == 0);
		REQUIRE(buffer.first() == 36);
		REQUIRE(buffer.last() == 36);
		REQUIRE(buffer[0] == 36);

		int32_t res2 = 0;
		REQUIRE(buffer.popLast(res2));
		REQUIRE(res2 == 36);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		REQUIRE(buffer.mapIndex(buffer.mLastIndex) == 0);

		REQUIRE(!buffer.popLast());
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		REQUIRE(buffer.mapIndex(buffer.mLastIndex) == 0);

		REQUIRE(buffer.addFirst(12));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX + 0));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.mapIndex(buffer.mFirstIndex) == 0);
		REQUIRE(buffer.mapIndex(buffer.mLastIndex) == 0);
		REQUIRE(buffer.first() == 12);
		REQUIRE(buffer.last() == 12);
		REQUIRE(buffer[0] == 12);
	}
	SECTION("Capacity == 2, add()") {
		RingBuffer<int32_t> buffer(2);
		REQUIRE(buffer.capacity() == 2);

		REQUIRE(buffer.add(3));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 1));
		REQUIRE(buffer.first() == 3);
		REQUIRE(buffer.last() == 3);
		REQUIRE(buffer[0] == 3);

		REQUIRE(buffer.add(4));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 2));
		REQUIRE(buffer.first() == 3);
		REQUIRE(buffer.last() == 4);
		REQUIRE(buffer[0] == 3);
		REQUIRE(buffer[1] == 4);

		REQUIRE(!buffer.add(4));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 2));
		REQUIRE(buffer.first() == 3);
		REQUIRE(buffer.last() == 4);
		REQUIRE(buffer[0] == 3);
		REQUIRE(buffer[1] == 4);

		int res1 = 0;
		REQUIRE(buffer.pop(res1));
		REQUIRE(res1 == 3);
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX + 1);
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 2));
		REQUIRE(buffer.first() == 4);
		REQUIRE(buffer.last() == 4);
		REQUIRE(buffer[0] == 4);

		REQUIRE(buffer.add(5));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX + 1);
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 3));
		REQUIRE(buffer.first() == 4);
		REQUIRE(buffer.last() == 5);
		REQUIRE(buffer[0] == 4);
		REQUIRE(buffer[1] == 5);
	}
	SECTION("Capacity == 2, addFirst()") {
		RingBuffer<int32_t> buffer(2);
		REQUIRE(buffer.capacity() == 2);

		REQUIRE(buffer.addFirst(3));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX - 1);
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX));
		REQUIRE(buffer.first() == 3);
		REQUIRE(buffer.last() == 3);
		REQUIRE(buffer[0] == 3);

		REQUIRE(buffer.addFirst(4));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX - 2));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 0));
		REQUIRE(buffer.first() == 4);
		REQUIRE(buffer.last() == 3);
		REQUIRE(buffer[0] == 4);
		REQUIRE(buffer[1] == 3);

		REQUIRE(!buffer.addFirst(5));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX - 2));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 0));
		REQUIRE(buffer.first() == 4);
		REQUIRE(buffer.last() == 3);
		REQUIRE(buffer[0] == 4);
		REQUIRE(buffer[1] == 3);

		int res1 = 0;
		REQUIRE(buffer.popLast(res1));
		REQUIRE(res1 == 3);
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX - 2));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX - 1));
		REQUIRE(buffer.first() == 4);
		REQUIRE(buffer.last() == 4);
		REQUIRE(buffer[0] == 4);
	}
}

TEST_CASE("RingBuffer: State methods", "[sfz::RingBuffer]")
{
	sfz::setContext(sfz::getStandardContext());

	SECTION("swap() and move constructors") {
		DebugAllocator alloc("debug");
		REQUIRE(alloc.numAllocations() == 0);
		{
			RingBuffer<UniquePtr<int32_t>> buffer(3);
			REQUIRE(buffer.add(makeUnique<int32_t>(&alloc, sfz_dbg(""), 2)));
			REQUIRE(alloc.numAllocations() == 1);
			REQUIRE(*buffer[0] == 2);
			{
				RingBuffer<UniquePtr<int32_t>> buffer2;
				buffer2 = std::move(buffer);
				REQUIRE(alloc.numAllocations() == 1);
				REQUIRE(buffer.size() == 0);
				REQUIRE(buffer2.size() == 1);
				REQUIRE(*buffer2[0] == 2);
			}
			REQUIRE(alloc.numAllocations() == 0);
		}
	}
	SECTION("clear()") {
		DebugAllocator alloc("debug");
		REQUIRE(alloc.numAllocations() == 0);

		RingBuffer<UniquePtr<int32_t>> buffer(2);
		REQUIRE(buffer.add(makeUnique<int32_t>(&alloc, sfz_dbg(""), 2)));
		REQUIRE(buffer.add(makeUnique<int32_t>(&alloc, sfz_dbg(""), 3)));
		REQUIRE(alloc.numAllocations() == 2);
		REQUIRE(*buffer.first() == 2);
		REQUIRE(*buffer.last() == 3);
		REQUIRE(buffer.size() == 2);
		REQUIRE(*buffer[0] == 2);
		REQUIRE(*buffer[1] == 3);
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + 2));

		buffer.clear();
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.capacity() == 2);
		REQUIRE(buffer.allocator() == getDefaultAllocator());
		REQUIRE(buffer.mFirstIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(buffer.mLastIndex == RINGBUFFER_BASE_IDX);
		REQUIRE(alloc.numAllocations() == 0);
	}
}

#ifndef __EMSCRIPTEN__
TEST_CASE("RingBuffer: Multi-threading", "[sfz::RingBuffer]")
{
	sfz::setContext(sfz::getStandardContext());

	SECTION("Slow Producer & fast consumer (add() & pop())") {
		constexpr uint64_t NUM_RESULTS = 1024;
		RingBuffer<int32_t> buffer(16);
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		std::thread producer([&]() {
			for (int32_t i = 0; i < int32_t(NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				bool success = buffer.add(i);
				if (!success) i--;
			}
		});

		std::thread consumer([&]() {
			for (int32_t i = 0; i < int32_t(NUM_RESULTS); i++) {
				int32_t out = -1;
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

		for (bool& b : results) REQUIRE(b);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX + NUM_RESULTS));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + NUM_RESULTS));
	}
	SECTION("Fast Producer & slow consumer (add() & pop())") {
		constexpr uint64_t NUM_RESULTS = 1024;
		RingBuffer<int32_t> buffer(16);
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		std::thread producer([&]() {
			for (int32_t i = 0; i < int32_t(NUM_RESULTS); i++) {
				bool success = buffer.add(i);
				if (!success) i--;
			}
		});

		std::thread consumer([&]() {
			for (int32_t i = 0; i < int32_t(NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				int32_t out = -1;
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

		for (bool& b : results) REQUIRE(b);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX + NUM_RESULTS));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + NUM_RESULTS));
	}
	SECTION("Slow Producer & fast consumer (addFirst() & popLast())") {
		constexpr uint64_t NUM_RESULTS = 1024;
		RingBuffer<int32_t> buffer(16);
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		std::thread producer([&]() {
			for (int32_t i = 0; i < int32_t(NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				bool success = buffer.addFirst(i);
				if (!success) i--;
			}
		});

		std::thread consumer([&]() {
			for (int32_t i = 0; i < int32_t(NUM_RESULTS); i++) {
				int32_t out = -1;
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

		for (bool& b : results) REQUIRE(b);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX - NUM_RESULTS));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX - NUM_RESULTS));
	}
	SECTION("Fast Producer & slow consumer (addFirst() & popLast())") {
		constexpr uint64_t NUM_RESULTS = 1024;
		RingBuffer<int32_t> buffer(16);
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		std::thread producer([&]() {
			for (int32_t i = 0; i < int32_t(NUM_RESULTS); i++) {
				bool success = buffer.addFirst(i);
				if (!success) i--;
			}
		});

		std::thread consumer([&]() {
			for (int32_t i = 0; i < int32_t(NUM_RESULTS); i++) {
				std::this_thread::sleep_for(std::chrono::microseconds(250));
				int32_t out = -1;
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

		for (bool& b : results) REQUIRE(b);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX - NUM_RESULTS));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX - NUM_RESULTS));
	}
	SECTION("Two producers (add() & addFirst())") {
		constexpr uint64_t NUM_RESULTS = 1024;
		constexpr uint64_t HALF_NUM_RESULTS = NUM_RESULTS / 2;
		RingBuffer<int32_t> buffer(NUM_RESULTS);
		bool results[NUM_RESULTS];
		for (bool& b : results) b = false;

		bool producerFirstNoSuccess = false;
		std::thread producerFirst([&]() {
			for (int32_t i = 0; i < int32_t(HALF_NUM_RESULTS); i++) {
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
			for (int32_t i = 0; i < int32_t(HALF_NUM_RESULTS); i++) {
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
		REQUIRE(!producerFirstNoSuccess);
		REQUIRE(!producerLastNoSuccess);
		REQUIRE(buffer.size() == NUM_RESULTS);
		REQUIRE(buffer.mFirstIndex == (RINGBUFFER_BASE_IDX - HALF_NUM_RESULTS));
		REQUIRE(buffer.mLastIndex == (RINGBUFFER_BASE_IDX + HALF_NUM_RESULTS));
		for (uint64_t i = 0; i < HALF_NUM_RESULTS; i++) {
			REQUIRE(buffer[i] == int32_t(HALF_NUM_RESULTS - i - 1));
		}
		for (uint64_t i = 0; i < HALF_NUM_RESULTS; i++) {
			REQUIRE(buffer[HALF_NUM_RESULTS + i] == int32_t(i));
		}
	}
}
#endif
