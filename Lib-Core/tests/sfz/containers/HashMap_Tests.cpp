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

#include <skipifzero_allocators.hpp>

#include "sfz/containers/HashMap.hpp"
#include "sfz/memory/DebugAllocator.hpp"
#include "sfz/strings/DynString.hpp"
#include "sfz/strings/StackString.hpp"
#include "sfz/strings/StringHashers.hpp"

using namespace sfz;

TEST_CASE("HashMap: Default constructor", "[sfz::HashMap]")
{
	HashMapDynamic<int,int> m1;
	REQUIRE(m1.size() == 0);
	REQUIRE(m1.capacity() == 0);
	REQUIRE(m1.placeholders() == 0);
}

TEST_CASE("HashMap: Copy constructors", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	HashMapDynamic<int,int> m1(1, &allocator, sfz_dbg(""));
	REQUIRE(m1.put(1, 2) == 2);
	REQUIRE(m1.put(2, 3) == 3);
	REQUIRE(m1.put(3, 4) == 4);
	REQUIRE(m1.size() == 3);
	REQUIRE(m1.capacity() != 0);
	REQUIRE(m1.placeholders() == 0);
	REQUIRE(m1[1] == 2);
	REQUIRE(m1[2] == 3);
	REQUIRE(m1[3] == 4);

	auto m2 = m1;
	REQUIRE(m2.size() == 3);
	REQUIRE(m2.capacity() != 0);
	REQUIRE(m2.placeholders() == 0);
	REQUIRE(m2[1] == 2);
	REQUIRE(m2[2] == 3);
	REQUIRE(m2[3] == 4);

	m2[1] = -1;
	m2[2] = -2;
	m2[3] = -3;
	REQUIRE(m2.size() == 3);
	REQUIRE(m2.capacity() != 0);
	REQUIRE(m2.placeholders() == 0);
	REQUIRE(m2[1] == -1);
	REQUIRE(m2[2] == -2);
	REQUIRE(m2[3] == -3);

	REQUIRE(m1.size() == 3);
	REQUIRE(m1.capacity() != 0);
	REQUIRE(m1.placeholders() == 0);
	REQUIRE(m1[1] == 2);
	REQUIRE(m1[2] == 3);
	REQUIRE(m1[3] == 4);

	m1.destroy();
	REQUIRE(m1.size() == 0);
	REQUIRE(m1.capacity() == 0);
	REQUIRE(m1.placeholders() == 0);

	REQUIRE(m2.size() == 3);
	REQUIRE(m2.capacity() != 0);
	REQUIRE(m2.placeholders() == 0);
	REQUIRE(m2[1] == -1);
	REQUIRE(m2[2] == -2);
	REQUIRE(m2[3] == -3);
}

TEST_CASE("HashMap: Copy constructor with allocator", "[sfz::ArrayDynamic]")
{
	sfz::setContext(sfz::getStandardContext());

	DebugAllocator first("first"), second("second");
	REQUIRE(first.numAllocations() == 0);
	REQUIRE(second.numAllocations() == 0);
	{
		HashMapDynamic<int,int> map1(10, &first, sfz_dbg(""));
		REQUIRE(map1.allocator() == &first);
		REQUIRE(first.numAllocations() == 1);

		map1[2] = 2;
		map1[3] = 3;
		map1[4] = 4;
		REQUIRE(map1.size() == 3);

		{
			HashMapDynamic<int, int> map2 = map1.clone(sfz_dbg(""), &second);
			REQUIRE(map2.allocator() == &second);
			REQUIRE(map2.capacity() == map1.capacity());
			REQUIRE(map2.size() == map1.size());
			REQUIRE(map2[2] == 2);
			REQUIRE(map2[3] == 3);
			REQUIRE(map2[4] == 4);
			REQUIRE(first.numAllocations() == 1);
			REQUIRE(second.numAllocations() == 1);
		}
		REQUIRE(first.numAllocations() == 1);
		REQUIRE(second.numAllocations() == 0);
	}
	REQUIRE(first.numAllocations() == 0);
	REQUIRE(second.numAllocations() == 0);
}


TEST_CASE("HashMap: Swap & move constructors", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	HashMapDynamic<int,int> v1;
	HashMapDynamic<int,int> v2(1, &allocator, sfz_dbg(""));
	v2.put(1, 2);
	v2.put(2, 3);
	v2.put(3, 4);

	REQUIRE(v1.size() == 0);
	REQUIRE(v1.capacity() == 0);
	REQUIRE(v1.placeholders() == 0);
	REQUIRE(v2.size() == 3);
	REQUIRE(v2.capacity() != 0);
	REQUIRE(v1.placeholders() == 0);

	v1.swap(v2);

	REQUIRE(v1.size() == 3);
	REQUIRE(v1.capacity() != 0);
	REQUIRE(v1.placeholders() == 0);
	REQUIRE(v2.size() == 0);
	REQUIRE(v2.capacity() == 0);
	REQUIRE(v2.placeholders() == 0);

	v1 = std::move(v2);

	REQUIRE(v1.size() == 0);
	REQUIRE(v1.capacity() == 0);
	REQUIRE(v1.placeholders() == 0);
	REQUIRE(v2.size() == 3);
	REQUIRE(v2.capacity() != 0);
	REQUIRE(v2.placeholders() == 0);
}

TEST_CASE("HashMap: rehash()", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	HashMapDynamic<int,int> m1(0, &allocator, sfz_dbg(""));
	REQUIRE(m1.capacity() == 0);
	REQUIRE(m1.size() == 0);
	REQUIRE(m1.placeholders() == 0);

	m1.rehash(1, sfz_dbg(""));
	REQUIRE(m1.capacity() != 0);
	REQUIRE(m1.size() == 0);
	REQUIRE(m1.placeholders() == 0);

	m1.put(1,2);
	m1.put(2,3);
	m1.put(3,4);
	REQUIRE(m1[1] == 2);
	REQUIRE(m1[2] == 3);
	REQUIRE(m1[3] == 4);
	REQUIRE(m1.size() == 3);

	m1.rehash(0, sfz_dbg(""));
	REQUIRE(m1[1] == 2);
	REQUIRE(m1[2] == 3);
	REQUIRE(m1[3] == 4);
	REQUIRE(m1.size() == 3);

	m1.rehash(m1.capacity() + 4, sfz_dbg(""));
	REQUIRE(m1[1] == 2);
	REQUIRE(m1[2] == 3);
	REQUIRE(m1[3] == 4);
	REQUIRE(m1.size() == 3);
}

TEST_CASE("HashMap: Rehashing in put()", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	HashMapDynamic<int, int> m1(0, &allocator, sfz_dbg(""));
	REQUIRE(m1.size() == 0);
	REQUIRE(m1.capacity() == 0);

	for (int i = 0; i < 256; ++i) {
		REQUIRE(m1.put(i, i + 1) == (i + 1));
		REQUIRE(m1.size() == uint32_t(i+1));
	}

	for (int i = 0; i < 256; ++i) {
		REQUIRE(m1.get(i) != nullptr);
		REQUIRE(*m1.get(i) == (i+1));
	}
}

TEST_CASE("HashMap: Adding and retrieving elements", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	HashMapDynamic<int, int> m1(0, &allocator, sfz_dbg(""));

	REQUIRE(m1.size() == 0);
	REQUIRE(m1.capacity() == 0);
	REQUIRE(m1.placeholders() == 0);

	m1.put(2, 3);
	REQUIRE(*m1.get(2) == 3);
	REQUIRE(m1.size() == 1);

	m1.put(3, 1);
	REQUIRE((*m1.get(3)) == 1);
	REQUIRE(m1.size() == 2);

	REQUIRE(m1.get(6) == nullptr);
	REQUIRE(m1.get(0) == nullptr);
	REQUIRE(m1.get(1) == nullptr);

	const HashMapDynamic<int, int>& mConst = m1;
	REQUIRE(mConst.size() == 2);
	REQUIRE(*mConst.get(2) == 3);
	REQUIRE(*mConst.get(3) == 1);
	REQUIRE(mConst.get(6) == nullptr);
	REQUIRE(mConst.get(0) == nullptr);
	REQUIRE(mConst.get(1) == nullptr);

	REQUIRE(m1.placeholders() == 0);
}

struct ZeroHashInteger {
	int value = 0;
	ZeroHashInteger(int value) : value(value) {}
	ZeroHashInteger() = default;
	ZeroHashInteger(const ZeroHashInteger&) = default;
	ZeroHashInteger& operator= (const ZeroHashInteger&) = default;
	bool operator== (ZeroHashInteger other) { return this->value == other.value; }
};

namespace sfz {
	constexpr uint64_t hash(ZeroHashInteger) noexcept { return 0; }
}

TEST_CASE("HashMap: Hashing conflicts", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	HashMapDynamic<ZeroHashInteger, int> m(0, &allocator, sfz_dbg(""));
	REQUIRE(m.size() == 0);
	REQUIRE(m.capacity() == 0);
	REQUIRE(m.placeholders() == 0);

	uint32_t sizeCount = 0;
	for (int i = -140; i <= 140; ++i) {
		m.put(i, i - 1337);
		sizeCount += 1;
		REQUIRE(m.size() == sizeCount);
		REQUIRE(m.get(i) != nullptr);
		REQUIRE(*m.get(i) == (i - 1337));
		REQUIRE(m.placeholders() == 0);

		if ((i % 3) == 0) {
			REQUIRE(m.remove(i));
			REQUIRE(!m.remove(i));
			sizeCount -= 1;
			REQUIRE(m.size() == sizeCount);
			REQUIRE(m.get(i) == nullptr);
			REQUIRE(m.placeholders() == 1); // Just removed an element (spot will be occupied again due to zero hash)
		}
	}

	for (int i = -140; i <= 140; ++i) {
		if ((i % 3) == 0) {
			REQUIRE(m.get(i) == nullptr);
			continue;
		}
		REQUIRE(m.get(i) != nullptr);
		REQUIRE(*m.get(i) == (i - 1337));
	}

	// Iterators
	uint32_t numPairs = 0;
	for (auto pair : m) {
		numPairs += 1;
		REQUIRE(m[pair.key] == pair.value);
		REQUIRE((pair.key.value - 1337) == pair.value);
	}
	REQUIRE(numPairs == sizeCount);

	// Const iterators
	const auto& constRef = m;
	numPairs = 0;
	for (auto pair : constRef) {
		numPairs += 1;
		REQUIRE(m[pair.key] == pair.value);
		REQUIRE((pair.key.value - 1337) == pair.value);
	}
	REQUIRE(numPairs == sizeCount);
}

TEST_CASE("HashMap operator[]", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	HashMapDynamic<int, int> m(1, &allocator, sfz_dbg(""));
	REQUIRE(m.size() == 0);
	REQUIRE(m.capacity() != 0);

	uint32_t sizeCount = 0;
	for (int i = -256; i <= 256; ++i) {
		m[i] = i - 1337;
		sizeCount += 1;
		REQUIRE(m.size() == sizeCount);
		REQUIRE(m[i] == (i - 1337));

		if ((i % 3) == 0) {
			REQUIRE(m.remove(i));
			REQUIRE(!m.remove(i));
			sizeCount -= 1;
			REQUIRE(m.size() == sizeCount);
			REQUIRE(m.placeholders() == 1);
			m[i];
			sizeCount += 1;
			REQUIRE(m.size() == sizeCount);
			REQUIRE(m.placeholders() == 0);
		}
	}
}

TEST_CASE("Empty HashMap", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	HashMapDynamic<int, int> m(0, &allocator, sfz_dbg(""));
	const HashMapDynamic<int,int> cm(0, &allocator, sfz_dbg(""));

	SECTION("Iterating") {
		int times = 0;
		for (HashMapDynamic<int,int>::KeyValuePair pair : m) {
			(void)pair;
			times += 1;
		}
		REQUIRE(times == 0);

		int ctimes = 0;
		for (HashMapDynamic<int, int>::ConstKeyValuePair pair : cm) {
			(void)pair;
			ctimes += 1;
		}
		REQUIRE(ctimes == 0);
	}
	SECTION("Retrieving") {
		int* ptr = m.get(0);
		REQUIRE(ptr == nullptr);

		const int* cPtr = cm.get(0);
		REQUIRE(cPtr == nullptr);
	}
	SECTION("put()") {
		int a = -1;
		m.put(2, a);
		m.put(3, 4);
		REQUIRE(m.capacity() != 0);
		REQUIRE(m.size() == 2);
		REQUIRE(m[2] == -1);
		REQUIRE(m.get(3) != nullptr);
		REQUIRE(*m.get(3) == 4);
	}
	SECTION("operator[]") {
		int a = -1;
		m[2] = a;
		m[3] = 4;
		REQUIRE(m.capacity() != 0);
		REQUIRE(m.size() == 2);
		REQUIRE(m[2] == -1);
		REQUIRE(m.get(3) != nullptr);
		REQUIRE(*m.get(3) == 4);
	}
}

TEST_CASE("HashMap with strings", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	SECTION("const char*") {
		HashMapDynamic<const char*, uint32_t> m(0, &allocator, sfz_dbg(""));
		const char* strFoo = "foo";
		const char* strBar = "bar";
		const char* strCar = "car";
		m.put(strFoo, 1);
		m.put(strBar, 2);
		m.put(strCar, 3);
		REQUIRE(m.get(strFoo) != nullptr);
		REQUIRE(*m.get(strFoo) == 1);
		REQUIRE(m.get(strBar) != nullptr);
		REQUIRE(*m.get(strBar) == 2);
		REQUIRE(m.get(strCar) != nullptr);
		REQUIRE(*m.get(strCar) == 3);
	}
	SECTION("DynString") {
		HashMapDynamic<DynString,uint32_t> m(0, &allocator, sfz_dbg(""));

		const uint32_t NUM_TESTS = 100;
		for (uint32_t i = 0; i < NUM_TESTS; i++) {
			DynString tmp("", 20);
			tmp.printf("str%u", i);
			m.put(tmp, i);
		}

		REQUIRE(m.size() == NUM_TESTS);
		REQUIRE(m.capacity() >= m.size());

		for (uint32_t i = 0; i < NUM_TESTS; i++) {
			DynString tmp("", 20);
			tmp.printf("str%u", i);
			uint32_t* ptr = m.get(tmp);
			REQUIRE(ptr != nullptr);
			REQUIRE(*ptr == i);

			uint32_t* ptr2 = m.get(tmp.str()); // alt key variant
			REQUIRE(ptr2 != nullptr);
			REQUIRE(*ptr2 == i);
			REQUIRE(*ptr2 == *ptr);
		}

		REQUIRE(m.get("str0") != nullptr);
		REQUIRE(*m.get("str0") == 0);
		REQUIRE(m.remove("str0"));
		REQUIRE(m.get("str0") == nullptr);

		m["str0"] = 3;
		REQUIRE(m["str0"] == 3);
	}
	SECTION("StackString") {
		HashMapDynamic<StackString,uint32_t> m(0, &allocator, sfz_dbg(""));

		const uint32_t NUM_TESTS = 100;
		for (uint32_t i = 0; i < NUM_TESTS; i++) {
			StackString tmp;
			tmp.printf("str%u", i);
			m.put(tmp, i);
		}

		REQUIRE(m.size() == NUM_TESTS);
		REQUIRE(m.capacity() >= m.size());

		for (uint32_t i = 0; i < NUM_TESTS; i++) {
			StackString tmp;
			tmp.printf("str%u", i);
			uint32_t* ptr = m.get(tmp);
			REQUIRE(ptr != nullptr);
			REQUIRE(*ptr == i);

			uint32_t* ptr2 = m.get(tmp.str); // alt key variant
			REQUIRE(ptr2 != nullptr);
			REQUIRE(*ptr2 == i);
			REQUIRE(*ptr2 == *ptr);
		}

		REQUIRE(m.get("str0") != nullptr);
		REQUIRE(*m.get("str0") == 0);
		REQUIRE(m.remove("str0"));
		REQUIRE(m.get("str0") == nullptr);

		m["str0"] = 3;
		REQUIRE(m["str0"] == 3);
	}
}

struct MoveTestStruct {
	int value = 0;
	bool moved = false;

	MoveTestStruct() noexcept = default;
	MoveTestStruct(const MoveTestStruct&) noexcept = default;
	MoveTestStruct& operator= (const MoveTestStruct&) noexcept = default;

	MoveTestStruct(int value) noexcept : value(value) { }

	MoveTestStruct(MoveTestStruct&& other) noexcept
	{
		*this = std::move(other);
	}

	MoveTestStruct& operator= (MoveTestStruct&& other) noexcept
	{
		this->value = other.value;
		this->moved = true;
		other.value = 0;
		other.moved = true;
		return *this;
	}

	bool operator== (const MoveTestStruct& other) const noexcept
	{
		return this->value == other.value;
	}
};

namespace sfz {
	constexpr uint64_t hash(const MoveTestStruct& val) noexcept { return uint64_t(val.value); }
}

TEST_CASE("Perfect forwarding in put()", "[sfz::HashMap]")
{
	sfz::StandardAllocator allocator;

	HashMapDynamic<MoveTestStruct, MoveTestStruct> m(0, &allocator, sfz_dbg(""));

	SECTION("const ref, const ref") {
		MoveTestStruct k = 2;
		MoveTestStruct v = 3;
		REQUIRE(!k.moved);
		REQUIRE(!v.moved);
		m.put(k, v);
		REQUIRE(!k.moved);
		REQUIRE(k.value == 2);
		REQUIRE(!v.moved);
		REQUIRE(v.value == 3);

		MoveTestStruct* ptr = m.get(k);
		REQUIRE(ptr != nullptr);
		REQUIRE(ptr->value == 3);

		MoveTestStruct* ptr2 = m.get(MoveTestStruct(2));
		REQUIRE(ptr2 != nullptr);
		REQUIRE(ptr2->value == 3);
	}
	SECTION("const ref, rvalue") {
		MoveTestStruct k = 2;
		MoveTestStruct v = 3;
		REQUIRE(!k.moved);
		REQUIRE(!v.moved);
		m.put(k, std::move(v));
		REQUIRE(!k.moved);
		REQUIRE(k.value == 2);
		REQUIRE(v.moved);
		REQUIRE(v.value == 0);

		MoveTestStruct* ptr = m.get(k);
		REQUIRE(ptr != nullptr);
		REQUIRE(ptr->value == 3);

		MoveTestStruct* ptr2 = m.get(MoveTestStruct(2));
		REQUIRE(ptr2 != nullptr);
		REQUIRE(ptr2->value == 3);
	}
	SECTION("altKey, const ref") {
		HashMapDynamic<StackString, MoveTestStruct> m2(0, &allocator, sfz_dbg(""));
		MoveTestStruct v(2);
		REQUIRE(!v.moved);
		m2.put("foo", v);
		REQUIRE(!v.moved);
		REQUIRE(v.value == 2);
		MoveTestStruct* ptr = m2.get("foo");
		REQUIRE(ptr != nullptr);
		REQUIRE(ptr->value == 2);
		REQUIRE(!ptr->moved);
	}
	SECTION("altKey, rvalue") {
		HashMapDynamic<StackString, MoveTestStruct> m2(0, &allocator, sfz_dbg(""));
		MoveTestStruct v(2);
		REQUIRE(!v.moved);
		m2.put("foo", std::move(v));
		REQUIRE(v.moved);
		REQUIRE(v.value == 0);
		MoveTestStruct* ptr = m2.get("foo");
		REQUIRE(ptr != nullptr);
		REQUIRE(ptr->value == 2);
		REQUIRE(ptr->moved);
	}
}
