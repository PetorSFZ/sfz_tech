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

#include <utility>

#include "sfz.h"
#include "skipifzero_allocators.hpp"
#include "skipifzero_hash_maps.hpp"
#include "skipifzero_strings.hpp"

// Hashing tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("Hashing: vec_hashing")
{
	// vec2_s32
	{
		i32x2 v1 = i32x2_init(2, 100);
		i32x2 v2 = i32x2_init(-1, -10);
		i32x2 v3 = i32x2_init(0, 14);

		CHECK(sfzHash(v1) != sfzHash(v2));
		CHECK(sfzHash(v2) != sfzHash(v3));
	}

	// vec3_s32
	{
		i32x3 v1 = i32x3_init(2, 100, 32);
		i32x3 v2 = i32x3_init(-1, 0, -10);
		i32x3 v3 = i32x3_init(0, -9, 14);

		CHECK(sfzHash(v1) != sfzHash(v2));
		CHECK(sfzHash(v2) != sfzHash(v3));
	}

	// vec4_s32
	{
		i32x4 v1 = i32x4_init(2, 100, 32, 1);
		i32x4 v2 = i32x4_init(-1, 0, -10, 9);
		i32x4 v3 = i32x4_init(0, -9, 14, 1337);

		CHECK(sfzHash(v1) != sfzHash(v2));
		CHECK(sfzHash(v2) != sfzHash(v3));
	}
}

// HashMap tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("HashMap: default_constructor")
{
	SfzHashMap<int,int> m1;
	CHECK(m1.size() == 0);
	CHECK(m1.capacity() == 0);
	CHECK(m1.placeholders() == 0);
}

TEST_CASE("HashMap: swap_and_move_constructors")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzHashMap<int,int> v1;
	SfzHashMap<int,int> v2(1, &allocator, sfz_dbg(""));
	v2.put(1, 2);
	v2.put(2, 3);
	v2.put(3, 4);

	CHECK(v1.size() == 0);
	CHECK(v1.capacity() == 0);
	CHECK(v1.placeholders() == 0);
	CHECK(v2.size() == 3);
	CHECK(v2.capacity() != 0);
	CHECK(v1.placeholders() == 0);

	v1.swap(v2);

	CHECK(v1.size() == 3);
	CHECK(v1.capacity() != 0);
	CHECK(v1.placeholders() == 0);
	CHECK(v2.size() == 0);
	CHECK(v2.capacity() == 0);
	CHECK(v2.placeholders() == 0);

	v1 = std::move(v2);

	CHECK(v1.size() == 0);
	CHECK(v1.capacity() == 0);
	CHECK(v1.placeholders() == 0);
	CHECK(v2.size() == 3);
	CHECK(v2.capacity() != 0);
	CHECK(v2.placeholders() == 0);
}


TEST_CASE("HashMap: clone")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzHashMap<int,int> m1(1, &allocator, sfz_dbg(""));
	CHECK(m1.put(1, 2) == 2);
	CHECK(m1.put(2, 3) == 3);
	CHECK(m1.put(3, 4) == 4);
	CHECK(m1.size() == 3);
	CHECK(m1.capacity() != 0);
	CHECK(m1.placeholders() == 0);
	CHECK(m1[1] == 2);
	CHECK(m1[2] == 3);
	CHECK(m1[3] == 4);

	auto m2 = m1.clone(&allocator, sfz_dbg(""));
	CHECK(m2.size() == 3);
	CHECK(m2.capacity() != 0);
	CHECK(m2.placeholders() == 0);
	CHECK(m2[1] == 2);
	CHECK(m2[2] == 3);
	CHECK(m2[3] == 4);

	m2[1] = -1;
	m2[2] = -2;
	m2[3] = -3;
	CHECK(m2.size() == 3);
	CHECK(m2.capacity() != 0);
	CHECK(m2.placeholders() == 0);
	CHECK(m2[1] == -1);
	CHECK(m2[2] == -2);
	CHECK(m2[3] == -3);

	CHECK(m1.size() == 3);
	CHECK(m1.capacity() != 0);
	CHECK(m1.placeholders() == 0);
	CHECK(m1[1] == 2);
	CHECK(m1[2] == 3);
	CHECK(m1[3] == 4);

	m1.destroy();
	CHECK(m1.size() == 0);
	CHECK(m1.capacity() == 0);
	CHECK(m1.placeholders() == 0);

	CHECK(m2.size() == 3);
	CHECK(m2.capacity() != 0);
	CHECK(m2.placeholders() == 0);
	CHECK(m2[1] == -1);
	CHECK(m2[2] == -2);
	CHECK(m2[3] == -3);
}

TEST_CASE("HashMap: rehash")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzHashMap<int,int> m1(0, &allocator, sfz_dbg(""));
	CHECK(m1.capacity() == 0);
	CHECK(m1.size() == 0);
	CHECK(m1.placeholders() == 0);

	m1.rehash(1, sfz_dbg(""));
	CHECK(m1.capacity() != 0);
	CHECK(m1.size() == 0);
	CHECK(m1.placeholders() == 0);

	m1.put(1,2);
	m1.put(2,3);
	m1.put(3,4);
	CHECK(m1[1] == 2);
	CHECK(m1[2] == 3);
	CHECK(m1[3] == 4);
	CHECK(m1.size() == 3);

	m1.rehash(0, sfz_dbg(""));
	CHECK(m1[1] == 2);
	CHECK(m1[2] == 3);
	CHECK(m1[3] == 4);
	CHECK(m1.size() == 3);

	m1.rehash(m1.capacity() + 4, sfz_dbg(""));
	CHECK(m1[1] == 2);
	CHECK(m1[2] == 3);
	CHECK(m1[3] == 4);
	CHECK(m1.size() == 3);
}

TEST_CASE("HashMap: rehashing_in_put")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzHashMap<int, int> m1(0, &allocator, sfz_dbg(""));
	CHECK(m1.size() == 0);
	CHECK(m1.capacity() == 0);

	for (int i = 0; i < 256; ++i) {
		CHECK(m1.put(i, i + 1) == (i + 1));
		CHECK(m1.size() == u32(i+1));
	}

	for (int i = 0; i < 256; ++i) {
		CHECK(m1.get(i) != nullptr);
		CHECK(*m1.get(i) == (i+1));
	}
}

TEST_CASE("HashMap: adding_and_retrieving_elements")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzHashMap<int, int> m1(0, &allocator, sfz_dbg(""));

	CHECK(m1.size() == 0);
	CHECK(m1.capacity() == 0);
	CHECK(m1.placeholders() == 0);

	m1.put(2, 3);
	CHECK(*m1.get(2) == 3);
	CHECK(m1.size() == 1);

	m1.put(3, 1);
	CHECK((*m1.get(3)) == 1);
	CHECK(m1.size() == 2);

	CHECK(m1.get(6) == nullptr);
	CHECK(m1.get(0) == nullptr);
	CHECK(m1.get(1) == nullptr);

	const SfzHashMap<int, int>& mConst = m1;
	CHECK(mConst.size() == 2);
	CHECK(*mConst.get(2) == 3);
	CHECK(*mConst.get(3) == 1);
	CHECK(mConst.get(6) == nullptr);
	CHECK(mConst.get(0) == nullptr);
	CHECK(mConst.get(1) == nullptr);

	CHECK(m1.placeholders() == 0);
}

struct ZeroHashInteger {
	int value = 0;
	ZeroHashInteger(int value) : value(value) {}
	ZeroHashInteger() = default;
	ZeroHashInteger(const ZeroHashInteger&) = default;
	ZeroHashInteger& operator= (const ZeroHashInteger&) = default;
	bool operator== (ZeroHashInteger other) const { return this->value == other.value; }
};

constexpr u64 sfzHash(ZeroHashInteger) noexcept { return 0; }

TEST_CASE("HashMap: hashing_conflicts")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzHashMap<ZeroHashInteger, int> m(0, &allocator, sfz_dbg(""));
	CHECK(m.size() == 0);
	CHECK(m.capacity() == 0);
	CHECK(m.placeholders() == 0);

	u32 sizeCount = 0;
	for (int i = -140; i <= 140; ++i) {
		m.put(i, i - 1337);
		sizeCount += 1;
		CHECK(m.size() == sizeCount);
		CHECK(m.get(i) != nullptr);
		CHECK(*m.get(i) == (i - 1337));
		CHECK(m.placeholders() == 0);

		if ((i % 3) == 0) {
			CHECK(m.remove(i));
			CHECK(!m.remove(i));
			sizeCount -= 1;
			CHECK(m.size() == sizeCount);
			CHECK(m.get(i) == nullptr);
			CHECK(m.placeholders() == 1); // Just removed an element (spot will be occupied again due to zero hash)
		}
	}

	for (int i = -140; i <= 140; ++i) {
		if ((i % 3) == 0) {
			CHECK(m.get(i) == nullptr);
			continue;
		}
		CHECK(m.get(i) != nullptr);
		CHECK(*m.get(i) == (i - 1337));
	}

	// Iterators
	u32 numPairs = 0;
	for (auto pair : m) {
		numPairs += 1;
		CHECK(m[pair.key] == pair.value);
		CHECK((pair.key.value - 1337) == pair.value);
	}
	CHECK(numPairs == sizeCount);

	// Const iterators
	const auto& constRef = m;
	numPairs = 0;
	for (auto pair : constRef) {
		numPairs += 1;
		CHECK(m[pair.key] == pair.value);
		CHECK((pair.key.value - 1337) == pair.value);
	}
	CHECK(numPairs == sizeCount);
}

TEST_CASE("HashMap: access_operator")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzHashMap<int, int> m(1, &allocator, sfz_dbg(""));
	CHECK(m.size() == 0);
	CHECK(m.capacity() != 0);

	u32 sizeCount = 0;
	for (int i = -256; i <= 256; ++i) {
		m.put(i, i - 1337);
		sizeCount += 1;
		CHECK(m.size() == sizeCount);
		CHECK(m[i] == (i - 1337));

		if ((i % 3) == 0) {
			CHECK(m.remove(i));
			CHECK(!m.remove(i));
			sizeCount -= 1;
			CHECK(m.size() == sizeCount);
		}
	}
}

TEST_CASE("HashMap: empty_hashmap")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Iterating
	{
		SfzHashMap<int, int> m(0, &allocator, sfz_dbg(""));
		const SfzHashMap<int, int> cm(0, &allocator, sfz_dbg(""));

		int times = 0;
		for (SfzHashMapPair<int, int> pair : m) {
			(void)pair;
			times += 1;
		}
		CHECK(times == 0);

		int ctimes = 0;
		for (SfzHashMapPair<int, const int> pair : cm) {
			(void)pair;
			ctimes += 1;
		}
		CHECK(ctimes == 0);
	}
	// Retrieving
	{
		SfzHashMap<int, int> m(0, &allocator, sfz_dbg(""));
		const SfzHashMap<int, int> cm(0, &allocator, sfz_dbg(""));

		int* ptr = m.get(0);
		CHECK(ptr == nullptr);

		const int* cPtr = cm.get(0);
		CHECK(cPtr == nullptr);
	}
	// put()
	{
		SfzHashMap<int, int> m(0, &allocator, sfz_dbg(""));

		int a = -1;
		m.put(2, a);
		m.put(3, 4);
		CHECK(m.capacity() != 0);
		CHECK(m.size() == 2);
		CHECK(m[2] == -1);
		CHECK(m.get(3) != nullptr);
		CHECK(*m.get(3) == 4);
	}
	// operator[]
	{
		SfzHashMap<int, int> m(0, &allocator, sfz_dbg(""));

		int a = -1;
		m.put(2, a);
		m.put(3, 4);
		CHECK(m.capacity() != 0);
		CHECK(m.size() == 2);
		CHECK(m[2] == -1);
		CHECK(m.get(3) != nullptr);
		CHECK(*m.get(3) == 4);
		CHECK(m[2] == a);
		CHECK(m[3] == 4);
	}
}

TEST_CASE("HashMap: hashmap_with_strings")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// const char*
	{
		SfzHashMap<const char*, u32> m(0, &allocator, sfz_dbg(""));
		const char* strFoo = "foo";
		const char* strBar = "bar";
		const char* strCar = "car";
		m.put(strFoo, 1);
		m.put(strBar, 2);
		m.put(strCar, 3);
		CHECK(m.get(strFoo) != nullptr);
		CHECK(*m.get(strFoo) == 1);
		CHECK(m.get(strBar) != nullptr);
		CHECK(*m.get(strBar) == 2);
		CHECK(m.get(strCar) != nullptr);
		CHECK(*m.get(strCar) == 3);
	}
	// LocalString
	{
		SfzHashMap<SfzStr96,u32> m(0, &allocator, sfz_dbg(""));

		const u32 NUM_TESTS = 100;
		for (u32 i = 0; i < NUM_TESTS; i++) {
			SfzStr96 tmp = {};
			sfzStr96Appendf(&tmp, "str%u", i);
			m.put(tmp, i);
		}

		CHECK(m.size() == NUM_TESTS);
		CHECK(m.capacity() >= m.size());

		for (u32 i = 0; i < NUM_TESTS; i++) {
			SfzStr96 tmp = {};
			sfzStr96Appendf(&tmp, "str%u", i);
			u32* ptr = m.get(tmp);
			CHECK(ptr != nullptr);
			CHECK(*ptr == i);

			u32* ptr2 = m.get(tmp.str); // alt key variant
			CHECK(ptr2 != nullptr);
			CHECK(*ptr2 == i);
			CHECK(*ptr2 == *ptr);
		}

		CHECK(m.get("str0") != nullptr);
		CHECK(*m.get("str0") == 0);
		CHECK(m.remove("str0"));
		CHECK(m.get("str0") == nullptr);

		m.put("str0", 3);
		CHECK(m["str0"] == 3);
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

constexpr u64 sfzHash(const MoveTestStruct& val) noexcept { return u64(val.value); }

TEST_CASE("HashMap: perfect_forwarding_in_put")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzHashMap<MoveTestStruct, MoveTestStruct> m(0, &allocator, sfz_dbg(""));

	// (const ref, const ref)
	{
		MoveTestStruct k = 2;
		MoveTestStruct v = 3;
		CHECK(!k.moved);
		CHECK(!v.moved);
		m.put(k, v);
		CHECK(!k.moved);
		CHECK(k.value == 2);
		CHECK(!v.moved);
		CHECK(v.value == 3);

		MoveTestStruct* ptr = m.get(k);
		CHECK(ptr != nullptr);
		CHECK(ptr->value == 3);

		MoveTestStruct* ptr2 = m.get(MoveTestStruct(2));
		CHECK(ptr2 != nullptr);
		CHECK(ptr2->value == 3);
	}
	// (const ref, rvalue)
	{
		MoveTestStruct k = 2;
		MoveTestStruct v = 3;
		CHECK(!k.moved);
		CHECK(!v.moved);
		m.put(k, std::move(v));
		CHECK(!k.moved);
		CHECK(k.value == 2);
		CHECK(v.moved);
		CHECK(v.value == 0);

		MoveTestStruct* ptr = m.get(k);
		CHECK(ptr != nullptr);
		CHECK(ptr->value == 3);

		MoveTestStruct* ptr2 = m.get(MoveTestStruct(2));
		CHECK(ptr2 != nullptr);
		CHECK(ptr2->value == 3);
	}
	// (altKey, const ref)
	{
		SfzHashMap<SfzStr96, MoveTestStruct> m2(0, &allocator, sfz_dbg(""));
		MoveTestStruct v(2);
		CHECK(!v.moved);
		m2.put("foo", v);
		CHECK(!v.moved);
		CHECK(v.value == 2);
		MoveTestStruct* ptr = m2.get("foo");
		CHECK(ptr != nullptr);
		CHECK(ptr->value == 2);
		CHECK(!ptr->moved);
	}
	// (altKey, rvalue)
	{
		SfzHashMap<SfzStr96, MoveTestStruct> m2(0, &allocator, sfz_dbg(""));
		MoveTestStruct v(2);
		CHECK(!v.moved);
		m2.put("foo", std::move(v));
		CHECK(v.moved);
		CHECK(v.value == 0);
		MoveTestStruct* ptr = m2.get("foo");
		CHECK(ptr != nullptr);
		CHECK(ptr->value == 2);
		CHECK(ptr->moved);
	}
}

// HashMapLocal tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("HashMapLocal: default_constructor")
{
	SfzHashMapLocal<int, int, 16> m1;
	CHECK(m1.size() == 0);
	CHECK(m1.capacity() == 16);
	CHECK(m1.placeholders() == 0);

	SfzHashMapLocal<int, f32x4, 8> m2;
	CHECK(m2.size() == 0);
	CHECK(m2.capacity() == 8);
	CHECK(m2.placeholders() == 0);

	SfzHashMapLocal<i32x4, f32x4, 8> m3;
	CHECK(m3.size() == 0);
	CHECK(m3.capacity() == 8);
	CHECK(m3.placeholders() == 0);
}

TEST_CASE("HashMapLocal: copy_constructors")
{
	SfzHashMapLocal<int,int, 16> m1;
	CHECK(m1.put(1, 2) == 2);
	CHECK(m1.put(2, 3) == 3);
	CHECK(m1.put(3, 4) == 4);
	CHECK(m1.size() == 3);
	CHECK(m1.capacity() == 16);
	CHECK(m1.placeholders() == 0);
	CHECK(m1[1] == 2);
	CHECK(m1[2] == 3);
	CHECK(m1[3] == 4);

	auto m2 = m1;
	CHECK(m2.size() == 3);
	CHECK(m2.capacity() == 16);
	CHECK(m2.placeholders() == 0);
	CHECK(m2[1] == 2);
	CHECK(m2[2] == 3);
	CHECK(m2[3] == 4);

	m2[1] = -1;
	m2[2] = -2;
	m2[3] = -3;
	CHECK(m2.size() == 3);
	CHECK(m2.capacity() == 16);
	CHECK(m2.placeholders() == 0);
	CHECK(m2[1] == -1);
	CHECK(m2[2] == -2);
	CHECK(m2[3] == -3);

	CHECK(m1.size() == 3);
	CHECK(m1.capacity() == 16);
	CHECK(m1.placeholders() == 0);
	CHECK(m1[1] == 2);
	CHECK(m1[2] == 3);
	CHECK(m1[3] == 4);

	m1.clear();
	CHECK(m1.size() == 0);
	CHECK(m1.capacity() == 16);
	CHECK(m1.placeholders() == 0);

	CHECK(m2.size() == 3);
	CHECK(m2.capacity() == 16);
	CHECK(m2.placeholders() == 0);
	CHECK(m2[1] == -1);
	CHECK(m2[2] == -2);
	CHECK(m2[3] == -3);
}

TEST_CASE("HashMapLocal: swap_and_move_constructors")
{
	SfzHashMapLocal<int,int, 16> v1;
	SfzHashMapLocal<int,int, 16> v2;
	v2.put(1, 2);
	v2.put(2, 3);
	v2.put(3, 4);

	CHECK(v1.size() == 0);
	CHECK(v1.capacity() == 16);
	CHECK(v1.placeholders() == 0);
	CHECK(v2.size() == 3);
	CHECK(v2.capacity() == 16);
	CHECK(v1.placeholders() == 0);

	v1.swap(v2);

	CHECK(v1.size() == 3);
	CHECK(v1.capacity() == 16);
	CHECK(v1.placeholders() == 0);
	CHECK(v2.size() == 0);
	CHECK(v2.capacity() == 16);
	CHECK(v2.placeholders() == 0);

	v1 = std::move(v2);

	CHECK(v1.size() == 0);
	CHECK(v1.capacity() == 16);
	CHECK(v1.placeholders() == 0);
	CHECK(v2.size() == 3);
	CHECK(v2.capacity() == 16);
	CHECK(v2.placeholders() == 0);
}

TEST_CASE("HashMapLocal: adding_and_retrieving_elements")
{
	SfzHashMapLocal<int, int, 16> m1;

	CHECK(m1.size() == 0);
	CHECK(m1.capacity() == 16);
	CHECK(m1.placeholders() == 0);

	m1.put(2, 3);
	CHECK(*m1.get(2) == 3);
	CHECK(m1.size() == 1);

	m1.put(3, 1);
	CHECK((*m1.get(3)) == 1);
	CHECK(m1.size() == 2);

	CHECK(m1.get(6) == nullptr);
	CHECK(m1.get(0) == nullptr);
	CHECK(m1.get(1) == nullptr);

	const SfzHashMapLocal<int, int, 16>& mConst = m1;
	CHECK(mConst.size() == 2);
	CHECK(*mConst.get(2) == 3);
	CHECK(*mConst.get(3) == 1);
	CHECK(mConst.get(6) == nullptr);
	CHECK(mConst.get(0) == nullptr);
	CHECK(mConst.get(1) == nullptr);

	CHECK(m1.placeholders() == 0);
}

TEST_CASE("HashMapLocal: hashing_conflicts")
{
	SfzHashMapLocal<ZeroHashInteger, int, 320> m;
	CHECK(m.size() == 0);
	CHECK(m.capacity() == 320);
	CHECK(m.placeholders() == 0);

	u32 sizeCount = 0;
	for (int i = -140; i <= 140; ++i) {
		m.put(i, i - 1337);
		sizeCount += 1;
		CHECK(m.size() == sizeCount);
		CHECK(m.get(i) != nullptr);
		CHECK(*m.get(i) == (i - 1337));
		CHECK(m.placeholders() == 0);

		if ((i % 3) == 0) {
			CHECK(m.remove(i));
			CHECK(!m.remove(i));
			sizeCount -= 1;
			CHECK(m.size() == sizeCount);
			CHECK(m.get(i) == nullptr);
			CHECK(m.placeholders() == 1); // Just removed an element (spot will be occupied again due to zero hash)
		}
	}

	for (int i = -140; i <= 140; ++i) {
		if ((i % 3) == 0) {
			CHECK(m.get(i) == nullptr);
			continue;
		}
		CHECK(m.get(i) != nullptr);
		CHECK(*m.get(i) == (i - 1337));
	}

	// Iterators
	u32 numPairs = 0;
	for (auto pair : m) {
		numPairs += 1;
		CHECK(m[pair.key] == pair.value);
		CHECK((pair.key.value - 1337) == pair.value);
	}
	CHECK(numPairs == sizeCount);

	// Const iterators
	const auto& constRef = m;
	numPairs = 0;
	for (auto pair : constRef) {
		numPairs += 1;
		CHECK(m[pair.key] == pair.value);
		CHECK((pair.key.value - 1337) == pair.value);
	}
	CHECK(numPairs == sizeCount);
}

TEST_CASE("HashMapLocal: access_operator")
{
	SfzHashMapLocal<int, int, 512> m;
	CHECK(m.size() == 0);
	CHECK(m.capacity() != 0);

	u32 sizeCount = 0;
	for (int i = -256; i < 256; ++i) {
		m.put(i, i - 1337);
		sizeCount += 1;
		CHECK(m.size() == sizeCount);
		CHECK(m[i] == (i - 1337));

		if ((i % 3) == 0) {
			CHECK(m.remove(i));
			CHECK(!m.remove(i));
			sizeCount -= 1;
			CHECK(m.size() == sizeCount);
		}
	}
}

TEST_CASE("HashMapLocal: empty_hashmap")
{
	// Iterating
	{
		SfzHashMapLocal<int, int, 13> m;
		const SfzHashMapLocal<int, int, 21> cm;

		int times = 0;
		for (SfzHashMapPair<int, int> pair : m) {
			(void)pair;
			times += 1;
		}
		CHECK(times == 0);

		int ctimes = 0;
		for (SfzHashMapPair<int, const int> pair : cm) {
			(void)pair;
			ctimes += 1;
		}
		CHECK(ctimes == 0);
	}
	// Retrieving
	{
		SfzHashMapLocal<int, int, 11> m;
		const SfzHashMapLocal<int, int, 11> cm;

		int* ptr = m.get(0);
		CHECK(ptr == nullptr);

		const int* cPtr = cm.get(0);
		CHECK(cPtr == nullptr);
	}
	// put()
	{
		SfzHashMapLocal<int, int, 52> m;

		int a = -1;
		m.put(2, a);
		m.put(3, 4);
		CHECK(m.capacity() == 52);
		CHECK(m.size() == 2);
		CHECK(m[2] == -1);
		CHECK(m.get(3) != nullptr);
		CHECK(*m.get(3) == 4);
	}
	// operator[]
	{
		SfzHashMapLocal<int, int, 17> m;

		int a = -1;
		m.put(2, a);
		m.put(3, 4);
		CHECK(m.capacity() == 17);
		CHECK(m.size() == 2);
		CHECK(m[2] == -1);
		CHECK(m.get(3) != nullptr);
		CHECK(*m.get(3) == 4);
		CHECK(m[2] == a);
		CHECK(m[3] == 4);
	}
}

TEST_CASE("HashMapLocal: hashmap_with_strings")
{
	// const char*
	{
		SfzHashMapLocal<const char*, u32, 14> m;
		const char* strFoo = "foo";
		const char* strBar = "bar";
		const char* strCar = "car";
		m.put(strFoo, 1);
		m.put(strBar, 2);
		m.put(strCar, 3);
		CHECK(m.get(strFoo) != nullptr);
		CHECK(*m.get(strFoo) == 1);
		CHECK(m.get(strBar) != nullptr);
		CHECK(*m.get(strBar) == 2);
		CHECK(m.get(strCar) != nullptr);
		CHECK(*m.get(strCar) == 3);
	}
	// LocalString
	{
		SfzHashMapLocal<SfzStr96,u32, 101> m;

		const u32 NUM_TESTS = 100;
		for (u32 i = 0; i < NUM_TESTS; i++) {
			SfzStr96 tmp = {};
			sfzStr96Appendf(&tmp, "str%u", i);
			m.put(tmp, i);
		}

		CHECK(m.size() == NUM_TESTS);
		CHECK(m.capacity() >= m.size());

		for (u32 i = 0; i < NUM_TESTS; i++) {
			SfzStr96 tmp = {};
			sfzStr96Appendf(&tmp, "str%u", i);
			u32* ptr = m.get(tmp);
			CHECK(ptr != nullptr);
			CHECK(*ptr == i);

			u32* ptr2 = m.get(tmp.str); // alt key variant
			CHECK(ptr2 != nullptr);
			CHECK(*ptr2 == i);
			CHECK(*ptr2 == *ptr);
		}

		CHECK(m.get("str0") != nullptr);
		CHECK(*m.get("str0") == 0);
		CHECK(m.remove("str0"));
		CHECK(m.get("str0") == nullptr);

		m.put("str0", 3);
		CHECK(m["str0"] == 3);
	}
}

TEST_CASE("HashMapLocal: perfect_forwarding_in_put")
{
	SfzHashMapLocal<MoveTestStruct, MoveTestStruct, 32> m;

	// (const ref, const ref)
	{
		MoveTestStruct k = 2;
		MoveTestStruct v = 3;
		CHECK(!k.moved);
		CHECK(!v.moved);
		m.put(k, v);
		CHECK(!k.moved);
		CHECK(k.value == 2);
		CHECK(!v.moved);
		CHECK(v.value == 3);

		MoveTestStruct* ptr = m.get(k);
		CHECK(ptr != nullptr);
		CHECK(ptr->value == 3);

		MoveTestStruct* ptr2 = m.get(MoveTestStruct(2));
		CHECK(ptr2 != nullptr);
		CHECK(ptr2->value == 3);
	}
	// (const ref, rvalue)
	{
		MoveTestStruct k = 2;
		MoveTestStruct v = 3;
		CHECK(!k.moved);
		CHECK(!v.moved);
		m.put(k, std::move(v));
		CHECK(!k.moved);
		CHECK(k.value == 2);
		CHECK(v.moved);
		CHECK(v.value == 0);

		MoveTestStruct* ptr = m.get(k);
		CHECK(ptr != nullptr);
		CHECK(ptr->value == 3);

		MoveTestStruct* ptr2 = m.get(MoveTestStruct(2));
		CHECK(ptr2 != nullptr);
		CHECK(ptr2->value == 3);
	}
	// (altKey, const ref)
	{
		SfzHashMapLocal<SfzStr96, MoveTestStruct, 72> m2;
		MoveTestStruct v(2);
		CHECK(!v.moved);
		m2.put("foo", v);
		CHECK(!v.moved);
		CHECK(v.value == 2);
		MoveTestStruct* ptr = m2.get("foo");
		CHECK(ptr != nullptr);
		CHECK(ptr->value == 2);
		CHECK(!ptr->moved);
	}
	// (altKey, rvalue)
	{
		SfzHashMapLocal<SfzStr96, MoveTestStruct, 63> m2;;
		MoveTestStruct v(2);
		CHECK(!v.moved);
		m2.put("foo", std::move(v));
		CHECK(v.moved);
		CHECK(v.value == 0);
		MoveTestStruct* ptr = m2.get("foo");
		CHECK(ptr != nullptr);
		CHECK(ptr->value == 2);
		CHECK(ptr->moved);
	}
}
