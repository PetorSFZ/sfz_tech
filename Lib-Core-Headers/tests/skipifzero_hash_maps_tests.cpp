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
#include "skipifzero_hash_maps.hpp"
#include "skipifzero_strings.hpp"

// HashMap tests
// ------------------------------------------------------------------------------------------------

UTEST(HashMap, default_constructor)
{
	sfz::HashMap<int,int> m1;
	ASSERT_TRUE(m1.size() == 0);
	ASSERT_TRUE(m1.capacity() == 0);
	ASSERT_TRUE(m1.placeholders() == 0);
}

UTEST(HashMap, copy_constructors)
{
	sfz::StandardAllocator allocator;

	sfz::HashMap<int,int> m1(1, &allocator, sfz_dbg(""));
	ASSERT_TRUE(m1.put(1, 2) == 2);
	ASSERT_TRUE(m1.put(2, 3) == 3);
	ASSERT_TRUE(m1.put(3, 4) == 4);
	ASSERT_TRUE(m1.size() == 3);
	ASSERT_TRUE(m1.capacity() != 0);
	ASSERT_TRUE(m1.placeholders() == 0);
	ASSERT_TRUE(m1[1] == 2);
	ASSERT_TRUE(m1[2] == 3);
	ASSERT_TRUE(m1[3] == 4);

	auto m2 = m1;
	ASSERT_TRUE(m2.size() == 3);
	ASSERT_TRUE(m2.capacity() != 0);
	ASSERT_TRUE(m2.placeholders() == 0);
	ASSERT_TRUE(m2[1] == 2);
	ASSERT_TRUE(m2[2] == 3);
	ASSERT_TRUE(m2[3] == 4);

	m2[1] = -1;
	m2[2] = -2;
	m2[3] = -3;
	ASSERT_TRUE(m2.size() == 3);
	ASSERT_TRUE(m2.capacity() != 0);
	ASSERT_TRUE(m2.placeholders() == 0);
	ASSERT_TRUE(m2[1] == -1);
	ASSERT_TRUE(m2[2] == -2);
	ASSERT_TRUE(m2[3] == -3);

	ASSERT_TRUE(m1.size() == 3);
	ASSERT_TRUE(m1.capacity() != 0);
	ASSERT_TRUE(m1.placeholders() == 0);
	ASSERT_TRUE(m1[1] == 2);
	ASSERT_TRUE(m1[2] == 3);
	ASSERT_TRUE(m1[3] == 4);

	m1.destroy();
	ASSERT_TRUE(m1.size() == 0);
	ASSERT_TRUE(m1.capacity() == 0);
	ASSERT_TRUE(m1.placeholders() == 0);

	ASSERT_TRUE(m2.size() == 3);
	ASSERT_TRUE(m2.capacity() != 0);
	ASSERT_TRUE(m2.placeholders() == 0);
	ASSERT_TRUE(m2[1] == -1);
	ASSERT_TRUE(m2[2] == -2);
	ASSERT_TRUE(m2[3] == -3);
}

UTEST(HashMap, swap_and_move_constructors)
{
	sfz::StandardAllocator allocator;

	sfz::HashMap<int,int> v1;
	sfz::HashMap<int,int> v2(1, &allocator, sfz_dbg(""));
	v2.put(1, 2);
	v2.put(2, 3);
	v2.put(3, 4);

	ASSERT_TRUE(v1.size() == 0);
	ASSERT_TRUE(v1.capacity() == 0);
	ASSERT_TRUE(v1.placeholders() == 0);
	ASSERT_TRUE(v2.size() == 3);
	ASSERT_TRUE(v2.capacity() != 0);
	ASSERT_TRUE(v1.placeholders() == 0);

	v1.swap(v2);

	ASSERT_TRUE(v1.size() == 3);
	ASSERT_TRUE(v1.capacity() != 0);
	ASSERT_TRUE(v1.placeholders() == 0);
	ASSERT_TRUE(v2.size() == 0);
	ASSERT_TRUE(v2.capacity() == 0);
	ASSERT_TRUE(v2.placeholders() == 0);

	v1 = std::move(v2);

	ASSERT_TRUE(v1.size() == 0);
	ASSERT_TRUE(v1.capacity() == 0);
	ASSERT_TRUE(v1.placeholders() == 0);
	ASSERT_TRUE(v2.size() == 3);
	ASSERT_TRUE(v2.capacity() != 0);
	ASSERT_TRUE(v2.placeholders() == 0);
}

UTEST(HashMap, rehash)
{
	sfz::StandardAllocator allocator;

	sfz::HashMap<int,int> m1(0, &allocator, sfz_dbg(""));
	ASSERT_TRUE(m1.capacity() == 0);
	ASSERT_TRUE(m1.size() == 0);
	ASSERT_TRUE(m1.placeholders() == 0);

	m1.rehash(1, sfz_dbg(""));
	ASSERT_TRUE(m1.capacity() != 0);
	ASSERT_TRUE(m1.size() == 0);
	ASSERT_TRUE(m1.placeholders() == 0);

	m1.put(1,2);
	m1.put(2,3);
	m1.put(3,4);
	ASSERT_TRUE(m1[1] == 2);
	ASSERT_TRUE(m1[2] == 3);
	ASSERT_TRUE(m1[3] == 4);
	ASSERT_TRUE(m1.size() == 3);

	m1.rehash(0, sfz_dbg(""));
	ASSERT_TRUE(m1[1] == 2);
	ASSERT_TRUE(m1[2] == 3);
	ASSERT_TRUE(m1[3] == 4);
	ASSERT_TRUE(m1.size() == 3);

	m1.rehash(m1.capacity() + 4, sfz_dbg(""));
	ASSERT_TRUE(m1[1] == 2);
	ASSERT_TRUE(m1[2] == 3);
	ASSERT_TRUE(m1[3] == 4);
	ASSERT_TRUE(m1.size() == 3);
}

UTEST(HashMap, rehashing_in_put)
{
	sfz::StandardAllocator allocator;

	sfz::HashMap<int, int> m1(0, &allocator, sfz_dbg(""));
	ASSERT_TRUE(m1.size() == 0);
	ASSERT_TRUE(m1.capacity() == 0);

	for (int i = 0; i < 256; ++i) {
		ASSERT_TRUE(m1.put(i, i + 1) == (i + 1));
		ASSERT_TRUE(m1.size() == uint32_t(i+1));
	}

	for (int i = 0; i < 256; ++i) {
		ASSERT_TRUE(m1.get(i) != nullptr);
		ASSERT_TRUE(*m1.get(i) == (i+1));
	}
}

UTEST(HashMap, adding_and_retrieving_elements)
{
	sfz::StandardAllocator allocator;

	sfz::HashMap<int, int> m1(0, &allocator, sfz_dbg(""));

	ASSERT_TRUE(m1.size() == 0);
	ASSERT_TRUE(m1.capacity() == 0);
	ASSERT_TRUE(m1.placeholders() == 0);

	m1.put(2, 3);
	ASSERT_TRUE(*m1.get(2) == 3);
	ASSERT_TRUE(m1.size() == 1);

	m1.put(3, 1);
	ASSERT_TRUE((*m1.get(3)) == 1);
	ASSERT_TRUE(m1.size() == 2);

	ASSERT_TRUE(m1.get(6) == nullptr);
	ASSERT_TRUE(m1.get(0) == nullptr);
	ASSERT_TRUE(m1.get(1) == nullptr);

	const sfz::HashMap<int, int>& mConst = m1;
	ASSERT_TRUE(mConst.size() == 2);
	ASSERT_TRUE(*mConst.get(2) == 3);
	ASSERT_TRUE(*mConst.get(3) == 1);
	ASSERT_TRUE(mConst.get(6) == nullptr);
	ASSERT_TRUE(mConst.get(0) == nullptr);
	ASSERT_TRUE(mConst.get(1) == nullptr);

	ASSERT_TRUE(m1.placeholders() == 0);
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

UTEST(HashMap, hashing_conflicts)
{
	sfz::StandardAllocator allocator;

	sfz::HashMap<ZeroHashInteger, int> m(0, &allocator, sfz_dbg(""));
	ASSERT_TRUE(m.size() == 0);
	ASSERT_TRUE(m.capacity() == 0);
	ASSERT_TRUE(m.placeholders() == 0);

	uint32_t sizeCount = 0;
	for (int i = -140; i <= 140; ++i) {
		m.put(i, i - 1337);
		sizeCount += 1;
		ASSERT_TRUE(m.size() == sizeCount);
		ASSERT_TRUE(m.get(i) != nullptr);
		ASSERT_TRUE(*m.get(i) == (i - 1337));
		ASSERT_TRUE(m.placeholders() == 0);

		if ((i % 3) == 0) {
			ASSERT_TRUE(m.remove(i));
			ASSERT_TRUE(!m.remove(i));
			sizeCount -= 1;
			ASSERT_TRUE(m.size() == sizeCount);
			ASSERT_TRUE(m.get(i) == nullptr);
			ASSERT_TRUE(m.placeholders() == 1); // Just removed an element (spot will be occupied again due to zero hash)
		}
	}

	for (int i = -140; i <= 140; ++i) {
		if ((i % 3) == 0) {
			ASSERT_TRUE(m.get(i) == nullptr);
			continue;
		}
		ASSERT_TRUE(m.get(i) != nullptr);
		ASSERT_TRUE(*m.get(i) == (i - 1337));
	}

	// Iterators
	uint32_t numPairs = 0;
	for (auto pair : m) {
		numPairs += 1;
		ASSERT_TRUE(m[pair.key] == pair.value);
		ASSERT_TRUE((pair.key.value - 1337) == pair.value);
	}
	ASSERT_TRUE(numPairs == sizeCount);

	// Const iterators
	const auto& constRef = m;
	numPairs = 0;
	for (auto pair : constRef) {
		numPairs += 1;
		ASSERT_TRUE(m[pair.key] == pair.value);
		ASSERT_TRUE((pair.key.value - 1337) == pair.value);
	}
	ASSERT_TRUE(numPairs == sizeCount);
}

UTEST(HashMap, access_operator)
{
	sfz::StandardAllocator allocator;

	sfz::HashMap<int, int> m(1, &allocator, sfz_dbg(""));
	ASSERT_TRUE(m.size() == 0);
	ASSERT_TRUE(m.capacity() != 0);

	uint32_t sizeCount = 0;
	for (int i = -256; i <= 256; ++i) {
		m[i] = i - 1337;
		sizeCount += 1;
		ASSERT_TRUE(m.size() == sizeCount);
		ASSERT_TRUE(m[i] == (i - 1337));

		if ((i % 3) == 0) {
			ASSERT_TRUE(m.remove(i));
			ASSERT_TRUE(!m.remove(i));
			sizeCount -= 1;
			ASSERT_TRUE(m.size() == sizeCount);
			ASSERT_TRUE(m.placeholders() == 1);
			m[i];
			sizeCount += 1;
			ASSERT_TRUE(m.size() == sizeCount);
			ASSERT_TRUE(m.placeholders() == 0);
		}
	}
}

UTEST(HashMap, empty_hashmap)
{
	sfz::StandardAllocator allocator;

	// Iterating
	{
		sfz::HashMap<int, int> m(0, &allocator, sfz_dbg(""));
		const sfz::HashMap<int, int> cm(0, &allocator, sfz_dbg(""));

		int times = 0;
		for (sfz::HashMap<int,int>::Pair<int> pair : m) {
			(void)pair;
			times += 1;
		}
		ASSERT_TRUE(times == 0);

		int ctimes = 0;
		for (sfz::HashMap<int, int>::Pair<const int> pair : cm) {
			(void)pair;
			ctimes += 1;
		}
		ASSERT_TRUE(ctimes == 0);
	}
	// Retrieving
	{
		sfz::HashMap<int, int> m(0, &allocator, sfz_dbg(""));
		const sfz::HashMap<int, int> cm(0, &allocator, sfz_dbg(""));

		int* ptr = m.get(0);
		ASSERT_TRUE(ptr == nullptr);

		const int* cPtr = cm.get(0);
		ASSERT_TRUE(cPtr == nullptr);
	}
	// put()
	{
		sfz::HashMap<int, int> m(0, &allocator, sfz_dbg(""));

		int a = -1;
		m.put(2, a);
		m.put(3, 4);
		ASSERT_TRUE(m.capacity() != 0);
		ASSERT_TRUE(m.size() == 2);
		ASSERT_TRUE(m[2] == -1);
		ASSERT_TRUE(m.get(3) != nullptr);
		ASSERT_TRUE(*m.get(3) == 4);
	}
	// operator[]
	{
		sfz::HashMap<int, int> m(0, &allocator, sfz_dbg(""));

		int a = -1;
		m[2] = a;
		m[3] = 4;
		ASSERT_TRUE(m.capacity() != 0);
		ASSERT_TRUE(m.size() == 2);
		ASSERT_TRUE(m[2] == -1);
		ASSERT_TRUE(m.get(3) != nullptr);
		ASSERT_TRUE(*m.get(3) == 4);
	}
}

UTEST(HashMap, hashmap_with_strings)
{
	sfz::StandardAllocator allocator;

	// const char*
	{
		sfz::HashMap<const char*, uint32_t> m(0, &allocator, sfz_dbg(""));
		const char* strFoo = "foo";
		const char* strBar = "bar";
		const char* strCar = "car";
		m.put(strFoo, 1);
		m.put(strBar, 2);
		m.put(strCar, 3);
		ASSERT_TRUE(m.get(strFoo) != nullptr);
		ASSERT_TRUE(*m.get(strFoo) == 1);
		ASSERT_TRUE(m.get(strBar) != nullptr);
		ASSERT_TRUE(*m.get(strBar) == 2);
		ASSERT_TRUE(m.get(strCar) != nullptr);
		ASSERT_TRUE(*m.get(strCar) == 3);
	}
	// LocalString
	{
		sfz::HashMap<sfz::str96,uint32_t> m(0, &allocator, sfz_dbg(""));

		const uint32_t NUM_TESTS = 100;
		for (uint32_t i = 0; i < NUM_TESTS; i++) {
			sfz::str96 tmp;
			tmp.printf("str%u", i);
			m.put(tmp, i);
		}

		ASSERT_TRUE(m.size() == NUM_TESTS);
		ASSERT_TRUE(m.capacity() >= m.size());

		for (uint32_t i = 0; i < NUM_TESTS; i++) {
			sfz::str96 tmp;
			tmp.printf("str%u", i);
			uint32_t* ptr = m.get(tmp);
			ASSERT_TRUE(ptr != nullptr);
			ASSERT_TRUE(*ptr == i);

			uint32_t* ptr2 = m.get(tmp.str); // alt key variant
			ASSERT_TRUE(ptr2 != nullptr);
			ASSERT_TRUE(*ptr2 == i);
			ASSERT_TRUE(*ptr2 == *ptr);
		}

		ASSERT_TRUE(m.get("str0") != nullptr);
		ASSERT_TRUE(*m.get("str0") == 0);
		ASSERT_TRUE(m.remove("str0"));
		ASSERT_TRUE(m.get("str0") == nullptr);

		m["str0"] = 3;
		ASSERT_TRUE(m["str0"] == 3);
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

UTEST(HashMap, perfect_forwarding_in_put)
{
	sfz::StandardAllocator allocator;

	sfz::HashMap<MoveTestStruct, MoveTestStruct> m(0, &allocator, sfz_dbg(""));

	// (const ref, const ref)
	{
		MoveTestStruct k = 2;
		MoveTestStruct v = 3;
		ASSERT_TRUE(!k.moved);
		ASSERT_TRUE(!v.moved);
		m.put(k, v);
		ASSERT_TRUE(!k.moved);
		ASSERT_TRUE(k.value == 2);
		ASSERT_TRUE(!v.moved);
		ASSERT_TRUE(v.value == 3);

		MoveTestStruct* ptr = m.get(k);
		ASSERT_TRUE(ptr != nullptr);
		ASSERT_TRUE(ptr->value == 3);

		MoveTestStruct* ptr2 = m.get(MoveTestStruct(2));
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(ptr2->value == 3);
	}
	// (const ref, rvalue)
	{
		MoveTestStruct k = 2;
		MoveTestStruct v = 3;
		ASSERT_TRUE(!k.moved);
		ASSERT_TRUE(!v.moved);
		m.put(k, std::move(v));
		ASSERT_TRUE(!k.moved);
		ASSERT_TRUE(k.value == 2);
		ASSERT_TRUE(v.moved);
		ASSERT_TRUE(v.value == 0);

		MoveTestStruct* ptr = m.get(k);
		ASSERT_TRUE(ptr != nullptr);
		ASSERT_TRUE(ptr->value == 3);

		MoveTestStruct* ptr2 = m.get(MoveTestStruct(2));
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(ptr2->value == 3);
	}
	// (altKey, const ref)
	{
		sfz::HashMap<sfz::str96, MoveTestStruct> m2(0, &allocator, sfz_dbg(""));
		MoveTestStruct v(2);
		ASSERT_TRUE(!v.moved);
		m2.put("foo", v);
		ASSERT_TRUE(!v.moved);
		ASSERT_TRUE(v.value == 2);
		MoveTestStruct* ptr = m2.get("foo");
		ASSERT_TRUE(ptr != nullptr);
		ASSERT_TRUE(ptr->value == 2);
		ASSERT_TRUE(!ptr->moved);
	}
	// (altKey, rvalue)
	{
		sfz::HashMap<sfz::str96, MoveTestStruct> m2(0, &allocator, sfz_dbg(""));
		MoveTestStruct v(2);
		ASSERT_TRUE(!v.moved);
		m2.put("foo", std::move(v));
		ASSERT_TRUE(v.moved);
		ASSERT_TRUE(v.value == 0);
		MoveTestStruct* ptr = m2.get("foo");
		ASSERT_TRUE(ptr != nullptr);
		ASSERT_TRUE(ptr->value == 2);
		ASSERT_TRUE(ptr->moved);
	}
}

