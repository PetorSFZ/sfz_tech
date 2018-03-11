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
#include "catch.hpp"
#include "sfz/PopWarnings.hpp"

#define private public
#include "sfz/containers/RingBuffer.hpp"
#undef private
#include "sfz/memory/DebugAllocator.hpp"
#include "sfz/memory/New.hpp"
#include "sfz/memory/SmartPointers.hpp"

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
		REQUIRE(buffer.mFirstIndex == 0);
	}
	SECTION("No capacity default allocator") {
		RingBuffer<int32_t> buffer(0);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.capacity() == 0);
		REQUIRE(buffer.allocator() == getDefaultAllocator());
		REQUIRE(buffer.mDataPtr == nullptr);
		REQUIRE(buffer.mFirstIndex == 0);
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
		REQUIRE(buffer.mFirstIndex == 0);
	}
	SECTION("Default allocator with capacity") {
		RingBuffer<int32_t> buffer(32);
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.capacity() == 32);
		REQUIRE(buffer.allocator() == getDefaultAllocator());
		REQUIRE(buffer.mDataPtr != nullptr);
		REQUIRE(buffer.mFirstIndex == 0);
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
			REQUIRE(buffer.mFirstIndex == 0);
		}
		REQUIRE(alloc.numAllocations() == 0);
	}
}

TEST_CASE("RingBuffer: Adding and accessing elements", "[sfz:RingBuffer]")
{
	sfz::setContext(sfz::getStandardContext());

	SECTION("Capacity == 1") {
		RingBuffer<int32_t> buffer(1);
		REQUIRE(buffer.capacity() == 1);

		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.add(24));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 24);
		REQUIRE(buffer.last() == 24);
		REQUIRE(buffer[0] == 24);

		REQUIRE(!buffer.add(36));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 24);
		REQUIRE(buffer.last() == 24);
		REQUIRE(buffer[0] == 24);

		REQUIRE(buffer.add(19, true));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 19);
		REQUIRE(buffer.last() == 19);
		REQUIRE(buffer[0] == 19);

		REQUIRE(!buffer.addFirst(36));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 19);
		REQUIRE(buffer.last() == 19);
		REQUIRE(buffer[0] == 19);

		REQUIRE(buffer.addFirst(4, true));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 4);
		REQUIRE(buffer.last() == 4);
		REQUIRE(buffer[0] == 4);
	}
	SECTION("Capacity == 2, add()") {
		RingBuffer<int32_t> buffer(2);
		REQUIRE(buffer.capacity() == 2);

		REQUIRE(buffer.add(3, true));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 3);
		REQUIRE(buffer.last() == 3);
		REQUIRE(buffer[0] == 3);

		REQUIRE(buffer.add(4, true));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 3);
		REQUIRE(buffer.last() == 4);
		REQUIRE(buffer[0] == 3);
		REQUIRE(buffer[1] == 4);

		REQUIRE(buffer.add(5, true));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == 1);
		REQUIRE(buffer.first() == 4);
		REQUIRE(buffer.last() == 5);
		REQUIRE(buffer[0] == 4);
		REQUIRE(buffer[1] == 5);

		REQUIRE(!buffer.add(6));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == 1);
		REQUIRE(buffer.first() == 4);
		REQUIRE(buffer.last() == 5);
		REQUIRE(buffer[0] == 4);
		REQUIRE(buffer[1] == 5);

		REQUIRE(buffer.add(7, true));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 5);
		REQUIRE(buffer.last() == 7);
		REQUIRE(buffer[0] == 5);
		REQUIRE(buffer[1] == 7);
	}
	SECTION("Capacity == 2, addFirst()") {
		RingBuffer<int32_t> buffer(2);
		REQUIRE(buffer.capacity() == 2);

		REQUIRE(buffer.addFirst(3, true));
		REQUIRE(buffer.size() == 1);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 3);
		REQUIRE(buffer.last() == 3);
		REQUIRE(buffer[0] == 3);

		REQUIRE(buffer.addFirst(4, true));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == 1);
		REQUIRE(buffer.first() == 4);
		REQUIRE(buffer.last() == 3);
		REQUIRE(buffer[0] == 4);
		REQUIRE(buffer[1] == 3);

		REQUIRE(buffer.addFirst(5, true));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 5);
		REQUIRE(buffer.last() == 4);
		REQUIRE(buffer[0] == 5);
		REQUIRE(buffer[1] == 4);

		REQUIRE(!buffer.addFirst(6));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(buffer.first() == 5);
		REQUIRE(buffer.last() == 4);
		REQUIRE(buffer[0] == 5);
		REQUIRE(buffer[1] == 4);

		REQUIRE(buffer.addFirst(7, true));
		REQUIRE(buffer.size() == 2);
		REQUIRE(buffer.mFirstIndex == 1);
		REQUIRE(buffer.first() == 7);
		REQUIRE(buffer.last() == 5);
		REQUIRE(buffer[0] == 7);
		REQUIRE(buffer[1] == 5);
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
			REQUIRE(buffer.add(makeUnique<int32_t>(&alloc, 2)));
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
		REQUIRE(buffer.add(makeUnique<int32_t>(&alloc, 2)));
		REQUIRE(buffer.add(makeUnique<int32_t>(&alloc, 3)));
		REQUIRE(alloc.numAllocations() == 2);
		REQUIRE(*buffer.first() == 2);
		REQUIRE(*buffer.last() == 3);
		REQUIRE(buffer.size() == 2);
		REQUIRE(*buffer[0] == 2);
		REQUIRE(*buffer[1] == 3);
		REQUIRE(buffer.mFirstIndex == 0);

		REQUIRE(buffer.add(makeUnique<int32_t>(&alloc, 4), true));
		REQUIRE(buffer.size() == 2);
		REQUIRE(alloc.numAllocations() == 2);
		REQUIRE(*buffer.first() == 3);
		REQUIRE(*buffer.last() == 4);
		REQUIRE(*buffer[0] == 3);
		REQUIRE(*buffer[1] == 4);
		REQUIRE(buffer.mFirstIndex == 1);

		buffer.clear();
		REQUIRE(buffer.size() == 0);
		REQUIRE(buffer.capacity() == 2);
		REQUIRE(buffer.allocator() == getDefaultAllocator());
		REQUIRE(buffer.mFirstIndex == 0);
		REQUIRE(alloc.numAllocations() == 0);
	}
}

