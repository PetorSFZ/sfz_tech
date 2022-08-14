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
#include "skipifzero_arrays.hpp"

// Array tests
// ------------------------------------------------------------------------------------------------

class Uncopiable {
public:
	int val = 0;

	Uncopiable(int val) : val(val) {}
	Uncopiable() = default;
	Uncopiable(const Uncopiable&) = delete;
	Uncopiable& operator= (const Uncopiable&) = delete;
	Uncopiable(Uncopiable&& other) { this->swap(other); }
	Uncopiable& operator= (Uncopiable&& other) { this->swap(other); return *this; }

	void swap(Uncopiable& other) { sfzSwap(this->val, other.val); }
};

TEST_CASE("Array: default_constructor")
{
	SfzArray<f32> floatArray;
	CHECK(floatArray.size() == 0);
	CHECK(floatArray.capacity() == 0);
	CHECK(floatArray.data() == nullptr);
	CHECK(floatArray.allocator() == nullptr);
}

TEST_CASE("Array: init_with_0_does_not_allocate")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<f32> v;
	v.init(0, &allocator, sfz_dbg(""));
	CHECK(v.size() == 0);
	CHECK(v.capacity() == 0);
	CHECK(v.data() == nullptr);
	CHECK(v.allocator() == &allocator);

	v.add(1.0f);
	CHECK(v.size() == 1);
	CHECK(v.capacity() == SFZ_ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	CHECK(v.data() != nullptr);
	CHECK(v.allocator() == &allocator);
}

TEST_CASE("Array: fill_constructor")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> twos(0, &allocator, sfz_dbg(""));
	twos.add(2, 8);

	for (u32 i = 0; i < 8; ++i) {
		CHECK(twos.data()[i] == 2);
	}
	CHECK(twos.size() == 8);
	CHECK(twos.capacity() == SFZ_ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);

	twos.destroy();
	CHECK(twos.size() == 0);
	CHECK(twos.capacity() == 0);
	CHECK(twos.data() == nullptr);
	CHECK(twos.allocator() == nullptr);
}

TEST_CASE("Array: swap_move_constructors")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> v1;
	SfzArray<int> v2(32, &allocator, sfz_dbg(""));
	v2.add(42, 2);

	CHECK(v1.size() == 0);
	CHECK(v1.capacity() == 0);
	CHECK(v1.data() == nullptr);
	CHECK(v1.allocator() == nullptr);

	CHECK(v2.size() == 2);
	CHECK(v2.capacity() == 32);
	CHECK(v2.data() != nullptr);
	CHECK(v2.allocator() == &allocator);
	CHECK(v2[0] == 42);
	CHECK(v2[1] == 42);

	v1.swap(v2);

	CHECK(v2.size() == 0);
	CHECK(v2.capacity() == 0);
	CHECK(v2.data() == nullptr);
	CHECK(v2.allocator() == nullptr);

	CHECK(v1.size() == 2);
	CHECK(v1.capacity() == 32);
	CHECK(v1.data() != nullptr);
	CHECK(v1.allocator() == &allocator);
	CHECK(v1[0] == 42);
	CHECK(v1[1] == 42);

	v1 = std::move(v2);

	CHECK(v1.size() == 0);
	CHECK(v1.capacity() == 0);
	CHECK(v1.data() == nullptr);
	CHECK(v1.allocator() == nullptr);

	CHECK(v2.size() == 2);
	CHECK(v2.capacity() == 32);
	CHECK(v2.data() != nullptr);
	CHECK(v2.allocator() == &allocator);
	CHECK(v2[0] == 42);
	CHECK(v2[1] == 42);
}

TEST_CASE("Array: access_operator")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> v(4, &allocator, sfz_dbg(""));
	v.hackSetSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	const auto& cv = v;
	CHECK(cv[0] == 0);
	CHECK(cv[1] == 1);
	CHECK(cv[2] == 2);
	CHECK(cv[3] == 3);
}

TEST_CASE("Array: iterators")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> v(4, &allocator, sfz_dbg(""));
	v.hackSetSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	int curr = 0;
	for (int val : v) {
		CHECK(val == curr);
		curr += 1;
	}
}

TEST_CASE("Array: add")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> v(2, &allocator, sfz_dbg(""));
	CHECK(v.size() == 0);
	CHECK(v.capacity() == 2);

	v.add(-1, 2);
	CHECK(v.size() == 2);
	CHECK(v.capacity() == 2);
	CHECK(v[0] == -1);
	CHECK(v[1] == -1);

	int a = 3;
	v.add(a);
	CHECK(v.size() == 3);
	CHECK(v.capacity() == 3);
	CHECK(v[0] == -1);
	CHECK(v[1] == -1);
	CHECK(v[2] == 3);

	v.add(a);
	CHECK(v.size() == 4);
	CHECK(v.capacity() == 5);
	CHECK(v[0] == -1);
	CHECK(v[1] == -1);
	CHECK(v[2] == 3);
	CHECK(v[3] == 3);

	SfzArray<Uncopiable> v2(0, &allocator, sfz_dbg(""));;

	CHECK(v2.size() == 0);
	CHECK(v2.capacity() == 0);
	CHECK(v2.data() == nullptr);
	CHECK(v2.allocator() == &allocator);

	v2.add(Uncopiable(3));

	CHECK(v2.size() == 1);
	CHECK(v2.capacity() == SFZ_ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	CHECK(v2[0].val == 3);

	Uncopiable b = Uncopiable(42);
	v2.add(std::move(b));

	CHECK(v2.size() == 2);
	CHECK(v2.capacity() == SFZ_ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	CHECK(v2[0].val == 3);
	CHECK(v2[1].val == 42);

	SfzArray<int> v3(0, &allocator, sfz_dbg(""));
	v3.add(v.data(), v.size());
	v3.add(v.data(), v.size());
	CHECK(v3.size() == 8);
	CHECK(v3[0] == -1);
	CHECK(v3[1] == -1);
	CHECK(v3[2] == 3);
	CHECK(v3[3] == 3);
	CHECK(v3[4] == -1);
	CHECK(v3[5] == -1);
	CHECK(v3[6] == 3);
	CHECK(v3[7] == 3);
}

TEST_CASE("Array: insert")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> v(2, &allocator, sfz_dbg(""));
	CHECK(v.size() == 0);
	CHECK(v.capacity() == 2);

	v.add(-1, 2);
	CHECK(v.size() == 2);
	CHECK(v.capacity() == 2);
	CHECK(v[0] == -1);
	CHECK(v[1] == -1);

	int a = 3;
	v.insert(0, a);
	CHECK(v.size() == 3);
	CHECK(v.capacity() == 3);
	CHECK(v[0] == 3);
	CHECK(v[1] == -1);
	CHECK(v[2] == -1);

	v.insert(2, a);
	CHECK(v.size() == 4);
	CHECK(v.capacity() == 5);
	CHECK(v[0] == 3);
	CHECK(v[1] == -1);
	CHECK(v[2] == 3);
	CHECK(v[3] == -1);

	SfzArray<int> v2(0, &allocator, sfz_dbg(""));
	v2.add(42, 3);
	v.insert(1, v2.data(), 2);
	CHECK(v.size() == 6);
	CHECK(v[0] == 3);
	CHECK(v[1] == 42);
	CHECK(v[2] == 42);
	CHECK(v[3] == -1);
	CHECK(v[4] == 3);
	CHECK(v[5] == -1);
}

TEST_CASE("Array: remove")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Basic test
	{
		SfzArray<int> v(0, &allocator, sfz_dbg(""));
		const int vals[] ={1, 2, 3, 4};
		v.add(vals, 4);

		CHECK(v.size() == 4);
		CHECK(v[0] == 1);
		CHECK(v[1] == 2);
		CHECK(v[2] == 3);
		CHECK(v[3] == 4);

		v.remove(3, 1000);
		CHECK(v.size() == 3);
		CHECK(v[0] == 1);
		CHECK(v[1] == 2);
		CHECK(v[2] == 3);

		v.remove(0, 2);
		CHECK(v.size() == 1);
		CHECK(v[0] == 3);
	}

	// Bug where memmove was passed numElements instead of numBytes
	{
		SfzArray<i32x2> v(0, &allocator, sfz_dbg(""));
		const i32x2 vals[] = { i32x2_splat(1), i32x2_splat(2), i32x2_splat(3), i32x2_splat(4)};
		v.add(vals, 4);

		CHECK(v.size() == 4);
		CHECK(v[0] == i32x2_splat(1));
		CHECK(v[1] == i32x2_splat(2));
		CHECK(v[2] == i32x2_splat(3));
		CHECK(v[3] == i32x2_splat(4));

		v.remove(1, 2);
		CHECK(v.size() == 2);
		CHECK(v[0] == i32x2_splat(1));
		CHECK(v[1] == i32x2_splat(4));
	}

	// Bug where not enough elements are moved
	{
		SfzArray<int> v(0, &allocator, sfz_dbg(""));
		const int vals[] = {1, 2, 3, 4, 5, 6};
		v.add(vals, 6);

		CHECK(v.size() == 6);
		CHECK(v[0] == 1);
		CHECK(v[1] == 2);
		CHECK(v[2] == 3);
		CHECK(v[3] == 4);
		CHECK(v[4] == 5);
		CHECK(v[5] == 6);

		v.remove(0, 1);
		CHECK(v.size() == 5);
		CHECK(v[0] == 2);
		CHECK(v[1] == 3);
		CHECK(v[2] == 4);
		CHECK(v[3] == 5);
		CHECK(v[4] == 6);
	}
}

TEST_CASE("Array: removeQuickSwap")
{
	SfzAllocator allocator = sfz::createStandardAllocator();
	SfzArray<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = {1, 2, 3, 4, 5, 6};
	v.add(vals, 6);

	CHECK(v.size() == 6);
	CHECK(v.first() == 1);
	CHECK(v.last() == 6);

	v.removeQuickSwap(0);
	CHECK(v.size() == 5);
	CHECK(v.last() == 5);
	CHECK(v.first() == 6);

	v.removeQuickSwap(1);
	CHECK(v.size() == 4);
	CHECK(v.last() == 4);
	CHECK(v[1] == 5);
}

TEST_CASE("Array: findElement")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = {1, 2, 2, 4};
	v.add(vals, 4);

	int* ptr = v.findElement(0);
	CHECK(ptr == nullptr);

	ptr = v.findElement(5);
	CHECK(ptr == nullptr);

	ptr = v.findElement(1);
	CHECK(ptr != nullptr);
	CHECK((ptr - v.data()) == 0);

	ptr = v.findElement(2);
	CHECK(ptr != nullptr);
	CHECK((ptr - v.data()) == 1);

	ptr = v.findElement(4);
	CHECK(ptr != nullptr);
	CHECK((ptr - v.data()) == 3);
}

TEST_CASE("Array: find")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = {1, 2, 3, 4};
	v.add(vals, 4);

	int* ptr = v.find([](int) { return false; });
	CHECK(ptr == nullptr);

	ptr = v.find([](int) { return true; });
	CHECK(ptr != nullptr);
	CHECK(*ptr == 1);

	ptr = v.find([](int param) { return param == 2; });
	CHECK(ptr != nullptr);
	CHECK(*ptr == 2);

	{
		const SfzArray<int>& vc = v;

		const int* ptr2 = vc.find([](int) { return false; });
		CHECK(ptr2 == nullptr);

		ptr2 = vc.find([](int) { return true; });
		CHECK(ptr2 != nullptr);
		CHECK(*ptr2 == 1);

		ptr2 = vc.find([](int param) { return param == 2; });
		CHECK(ptr2 != nullptr);
		CHECK(*ptr2 == 2);
	}
}

TEST_CASE("Array: findLast")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = { 1, 2, 3, 4 };
	v.add(vals, 4);

	int* ptr = v.findLast([](int) { return false; });
	CHECK(ptr == nullptr);

	ptr = v.findLast([](int) { return true; });
	CHECK(ptr != nullptr);
	CHECK(*ptr == 4);

	ptr = v.findLast([](int param) { return param == 2; });
	CHECK(ptr != nullptr);
	CHECK(*ptr == 2);

	{
		const SfzArray<int>& vc = v;

		const int* ptr2 = vc.findLast([](int) { return false; });
		CHECK(ptr2 == nullptr);

		ptr2 = vc.findLast([](int) { return true; });
		CHECK(ptr2 != nullptr);
		CHECK(*ptr2 == 4);

		ptr2 = vc.findLast([](int param) { return param == 2; });
		CHECK(ptr2 != nullptr);
		CHECK(*ptr2 == 2);
	}
}

TEST_CASE("Array: sort")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	SfzArray<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = { 7, 1, 0, 2, 9, -1, -2, -2, 10, 11, 9, 0 };
	v.add(vals, 12);

	v.sort();
	CHECK(v[0] == -2);
	CHECK(v[1] == -2);
	CHECK(v[2] == -1);
	CHECK(v[3] == 0);
	CHECK(v[4] == 0);
	CHECK(v[5] == 1);
	CHECK(v[6] == 2);
	CHECK(v[7] == 7);
	CHECK(v[8] == 9);
	CHECK(v[9] == 9);
	CHECK(v[10] == 10);
	CHECK(v[11] == 11);

	v.sort([](int lhs, int rhs) {
		return lhs > rhs;
	});
	CHECK(v[0] == 11);
	CHECK(v[1] == 10);
	CHECK(v[2] == 9);
	CHECK(v[3] == 9);
	CHECK(v[4] == 7);
	CHECK(v[5] == 2);
	CHECK(v[6] == 1);
	CHECK(v[7] == 0);
	CHECK(v[8] == 0);
	CHECK(v[9] == -1);
	CHECK(v[10] == -2);
	CHECK(v[11] == -2);
	
	// Sort twice
	v.sort([](int lhs, int rhs) {
		return lhs > rhs;
	});
	CHECK(v[0] == 11);
	CHECK(v[1] == 10);
	CHECK(v[2] == 9);
	CHECK(v[3] == 9);
	CHECK(v[4] == 7);
	CHECK(v[5] == 2);
	CHECK(v[6] == 1);
	CHECK(v[7] == 0);
	CHECK(v[8] == 0);
	CHECK(v[9] == -1);
	CHECK(v[10] == -2);
	CHECK(v[11] == -2);

	v.sort([](int lhs, int rhs) {
		return lhs < rhs;
	});
	CHECK(v[0] == -2);
	CHECK(v[1] == -2);
	CHECK(v[2] == -1);
	CHECK(v[3] == 0);
	CHECK(v[4] == 0);
	CHECK(v[5] == 1);
	CHECK(v[6] == 2);
	CHECK(v[7] == 7);
	CHECK(v[8] == 9);
	CHECK(v[9] == 9);
	CHECK(v[10] == 10);
	CHECK(v[11] == 11);
}

// ArrayLocal tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("ArrayLocal: default_constructor")
{
	alignas(32) SfzArrayLocal<f32, 5> fiveArray;
	CHECK(fiveArray.size() == 0);
	CHECK(fiveArray.capacity() == 5);
	CHECK((uintptr_t)fiveArray.data() == (uintptr_t)&fiveArray);
	
	alignas(64) SfzArrayLocal<f32, 8> eightArray;
	CHECK(eightArray.size() == 0);
	CHECK(eightArray.capacity() == 8);
	CHECK((uintptr_t)eightArray.data() == (uintptr_t)&eightArray);
	
	SfzArrayLocal<f32x4, 8> vecs;
	CHECK(vecs.size() == 0);
	CHECK(vecs.capacity() == 8);
	CHECK((uintptr_t)vecs.data() == (uintptr_t)&vecs);
}

TEST_CASE("ArrayLocal: fill_constructor")
{
	SfzArrayLocal<int, 16> twos;
	CHECK(twos.capacity() == 16);

	CHECK(twos.size() == 0);
	twos.add(2, 8);
	CHECK(twos.size() == 8);
	for (u32 i = 0; i < 8; i++) {
		CHECK(twos.data()[i] == 2);
	}

	twos.clear();
	CHECK(twos.size() == 0);
}

TEST_CASE("ArrayLocal: copy_constructors")
{
	SfzArrayLocal<int, 16> first;
	first.add(3, 3);
	SfzArrayLocal<int, 16> second;

	CHECK(first.size() == 3);
	CHECK(first.data()[0] == 3);
	CHECK(first.data()[1] == 3);
	CHECK(first.data()[2] == 3);

	CHECK(second.size() == 0);

	second = first;
	first.clear();

	CHECK(first.size() == 0);

	CHECK(second.size() == 3);
	CHECK(second.data()[0] == 3);
	CHECK(second.data()[1] == 3);
	CHECK(second.data()[2] == 3);
}

TEST_CASE("ArrayLocal: swap_move_constructors")
{
	SfzArrayLocal<Uncopiable, 16> v1;
	SfzArrayLocal<Uncopiable, 16> v2;
	v2.add(Uncopiable(42));
	v2.add(Uncopiable(42));

	CHECK(v1.size() == 0);
	CHECK(v2.size() == 2);
	CHECK(v2[0].val == 42);
	CHECK(v2[1].val == 42);

	v1.swap(v2);
	CHECK(v2.size() == 0);
	CHECK(v1.size() == 2);
	CHECK(v1[0].val == 42);
	CHECK(v1[1].val == 42);

	v1 = std::move(v2);
	CHECK(v1.size() == 0);
	CHECK(v2.size() == 2);
	CHECK(v2[0].val == 42);
	CHECK(v2[1].val == 42);
}

TEST_CASE("ArrayLocal: access_operator")
{
	SfzArrayLocal<int, 16> v;
	v.setSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	const auto& cv = v;
	CHECK(cv[0] == 0);
	CHECK(cv[1] == 1);
	CHECK(cv[2] == 2);
	CHECK(cv[3] == 3);
}

TEST_CASE("ArrayLocal: iterators")
{
	SfzArrayLocal<int, 16> v;
	v.setSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	int curr = 0;
	for (int val : v) {
		CHECK(val == curr);
		curr += 1;
	}
}

TEST_CASE("ArrayLoca: add")
{
	SfzArrayLocal<int, 16> v;
	CHECK(v.size() == 0);

	v.add(-1, 2);
	CHECK(v.size() == 2);
	CHECK(v[0] == -1);
	CHECK(v[1] == -1);

	int a = 3;
	v.add(a);
	CHECK(v.size() == 3);
	CHECK(v[0] == -1);
	CHECK(v[1] == -1);
	CHECK(v[2] == 3);

	v.add(a);
	CHECK(v.size() == 4);
	CHECK(v[0] == -1);
	CHECK(v[1] == -1);
	CHECK(v[2] == 3);
	CHECK(v[3] == 3);

	SfzArrayLocal<Uncopiable, 16> v2;
	CHECK(v2.size() == 0);

	v2.add(Uncopiable(3));

	CHECK(v2.size() == 1);
	CHECK(v2[0].val == 3);

	Uncopiable b = Uncopiable(42);
	v2.add(std::move(b));

	CHECK(v2.size() == 2);
	CHECK(v2[0].val == 3);
	CHECK(v2[1].val == 42);

	SfzArrayLocal<int, 16> v3;
	v3.add(v.data(), v.size());
	v3.add(v.data(), v.size());
	CHECK(v3.size() == 8);
	CHECK(v3[0] == -1);
	CHECK(v3[1] == -1);
	CHECK(v3[2] == 3);
	CHECK(v3[3] == 3);
	CHECK(v3[4] == -1);
	CHECK(v3[5] == -1);
	CHECK(v3[6] == 3);
	CHECK(v3[7] == 3);
}

TEST_CASE("ArrayLocal: insert")
{
	SfzArrayLocal<int, 21> v;
	CHECK(v.size() == 0);

	v.add(-1, 2);
	CHECK(v.size() == 2);
	CHECK(v[0] == -1);
	CHECK(v[1] == -1);

	int a = 3;
	v.insert(0, a);
	CHECK(v.size() == 3);
	CHECK(v[0] == 3);
	CHECK(v[1] == -1);
	CHECK(v[2] == -1);

	v.insert(2, a);
	CHECK(v.size() == 4);
	CHECK(v[0] == 3);
	CHECK(v[1] == -1);
	CHECK(v[2] == 3);
	CHECK(v[3] == -1);

	SfzArrayLocal<int, 23> v2;
	v2.add(42, 3);
	v.insert(1, v2.data(), 2);
	CHECK(v.size() == 6);
	CHECK(v[0] == 3);
	CHECK(v[1] == 42);
	CHECK(v[2] == 42);
	CHECK(v[3] == -1);
	CHECK(v[4] == 3);
	CHECK(v[5] == -1);
}

TEST_CASE("ArrayLocal: remove")
{
	// Basic test
	{
		SfzArrayLocal<int, 19> v;
		const int vals[] ={1, 2, 3, 4};
		v.add(vals, 4);

		CHECK(v.size() == 4);
		CHECK(v[0] == 1);
		CHECK(v[1] == 2);
		CHECK(v[2] == 3);
		CHECK(v[3] == 4);

		v.remove(3, 1000);
		CHECK(v.size() == 3);
		CHECK(v[0] == 1);
		CHECK(v[1] == 2);
		CHECK(v[2] == 3);

		v.remove(0, 2);
		CHECK(v.size() == 1);
		CHECK(v[0] == 3);
	}

	// Bug where memmove was passed numElements instead of numBytes
	{
		SfzArrayLocal<i32x2, 7> v;
		const i32x2 vals[] = { i32x2_splat(1), i32x2_splat(2), i32x2_splat(3), i32x2_splat(4)};
		v.add(vals, 4);

		CHECK(v.size() == 4);
		CHECK(v[0] == i32x2_splat(1));
		CHECK(v[1] == i32x2_splat(2));
		CHECK(v[2] == i32x2_splat(3));
		CHECK(v[3] == i32x2_splat(4));

		v.remove(1, 2);
		CHECK(v.size() == 2);
		CHECK(v[0] == i32x2_splat(1));
		CHECK(v[1] == i32x2_splat(4));
	}

	// Bug where not enough elements are moved
	{
		SfzArrayLocal<int, 11> v;
		const int vals[] = {1, 2, 3, 4, 5, 6};
		v.add(vals, 6);

		CHECK(v.size() == 6);
		CHECK(v[0] == 1);
		CHECK(v[1] == 2);
		CHECK(v[2] == 3);
		CHECK(v[3] == 4);
		CHECK(v[4] == 5);
		CHECK(v[5] == 6);

		v.remove(0, 1);
		CHECK(v.size() == 5);
		CHECK(v[0] == 2);
		CHECK(v[1] == 3);
		CHECK(v[2] == 4);
		CHECK(v[3] == 5);
		CHECK(v[4] == 6);
	}
}

TEST_CASE("ArrayLocal: removeQuickSwap")
{
	SfzArrayLocal<int, 13> v;
	const int vals[] = {1, 2, 3, 4, 5, 6};
	v.add(vals, 6);

	CHECK(v.size() == 6);
	CHECK(v.first() == 1);
	CHECK(v.last() == 6);

	v.removeQuickSwap(0);
	CHECK(v.size() == 5);
	CHECK(v.last() == 5);
	CHECK(v.first() == 6);

	v.removeQuickSwap(1);
	CHECK(v.size() == 4);
	CHECK(v.last() == 4);
	CHECK(v[1] == 5);
}

TEST_CASE("ArrayLocal: findElement")
{
	SfzArrayLocal<int, 16> v;
	const int vals[] = {1, 2, 2, 4};
	v.add(vals, 4);

	int* ptr = v.findElement(0);
	CHECK(ptr == nullptr);

	ptr = v.findElement(5);
	CHECK(ptr == nullptr);

	ptr = v.findElement(1);
	CHECK(ptr != nullptr);
	CHECK((ptr - v.data()) == 0);

	ptr = v.findElement(2);
	CHECK(ptr != nullptr);
	CHECK((ptr - v.data()) == 1);

	ptr = v.findElement(4);
	CHECK(ptr != nullptr);
	CHECK((ptr - v.data()) == 3);
}

TEST_CASE("ArrayLocal: find")
{
	SfzArrayLocal<int, 15> v;
	const int vals[] = {1, 2, 3, 4};
	v.add(vals, 4);

	int* ptr = v.find([](int) { return false; });
	CHECK(ptr == nullptr);

	ptr = v.find([](int) { return true; });
	CHECK(ptr != nullptr);
	CHECK(*ptr == 1);

	ptr = v.find([](int param) { return param == 2; });
	CHECK(ptr != nullptr);
	CHECK(*ptr == 2);

	{
		const SfzArrayLocal<int, 15>& vc = v;

		const int* ptr2 = vc.find([](int) { return false; });
		CHECK(ptr2 == nullptr);

		ptr2 = vc.find([](int) { return true; });
		CHECK(ptr2 != nullptr);
		CHECK(*ptr2 == 1);

		ptr2 = vc.find([](int param) { return param == 2; });
		CHECK(ptr2 != nullptr);
		CHECK(*ptr2 == 2);
	}
}

TEST_CASE("ArrayLocal: findLast")
{
	SfzArrayLocal<int, 15> v;
	const int vals[] = { 1, 2, 3, 4 };
	v.add(vals, 4);

	int* ptr = v.findLast([](int) { return false; });
	CHECK(ptr == nullptr);

	ptr = v.findLast([](int) { return true; });
	CHECK(ptr != nullptr);
	CHECK(*ptr == 4);

	ptr = v.findLast([](int param) { return param == 2; });
	CHECK(ptr != nullptr);
	CHECK(*ptr == 2);

	{
		const SfzArrayLocal<int, 15>& vc = v;

		const int* ptr2 = vc.findLast([](int) { return false; });
		CHECK(ptr2 == nullptr);

		ptr2 = vc.findLast([](int) { return true; });
		CHECK(ptr2 != nullptr);
		CHECK(*ptr2 == 4);

		ptr2 = vc.findLast([](int param) { return param == 2; });
		CHECK(ptr2 != nullptr);
		CHECK(*ptr2 == 2);
	}
}

TEST_CASE("ArrayLocal: sort")
{
	SfzArrayLocal<int, 16> v;
	const int vals[] = { 7, 1, 0, 2, 9, -1, -2, -2, 10, 11, 9, 0 };
	v.add(vals, 12);

	v.sort();
	CHECK(v[0] == -2);
	CHECK(v[1] == -2);
	CHECK(v[2] == -1);
	CHECK(v[3] == 0);
	CHECK(v[4] == 0);
	CHECK(v[5] == 1);
	CHECK(v[6] == 2);
	CHECK(v[7] == 7);
	CHECK(v[8] == 9);
	CHECK(v[9] == 9);
	CHECK(v[10] == 10);
	CHECK(v[11] == 11);

	v.sort([](int lhs, int rhs) {
		return lhs > rhs;
	});
	CHECK(v[0] == 11);
	CHECK(v[1] == 10);
	CHECK(v[2] == 9);
	CHECK(v[3] == 9);
	CHECK(v[4] == 7);
	CHECK(v[5] == 2);
	CHECK(v[6] == 1);
	CHECK(v[7] == 0);
	CHECK(v[8] == 0);
	CHECK(v[9] == -1);
	CHECK(v[10] == -2);
	CHECK(v[11] == -2);
	
	// Sort twice
	v.sort([](int lhs, int rhs) {
		return lhs > rhs;
	});
	CHECK(v[0] == 11);
	CHECK(v[1] == 10);
	CHECK(v[2] == 9);
	CHECK(v[3] == 9);
	CHECK(v[4] == 7);
	CHECK(v[5] == 2);
	CHECK(v[6] == 1);
	CHECK(v[7] == 0);
	CHECK(v[8] == 0);
	CHECK(v[9] == -1);
	CHECK(v[10] == -2);
	CHECK(v[11] == -2);

	v.sort([](int lhs, int rhs) {
		return lhs < rhs;
	});
	CHECK(v[0] == -2);
	CHECK(v[1] == -2);
	CHECK(v[2] == -1);
	CHECK(v[3] == 0);
	CHECK(v[4] == 0);
	CHECK(v[5] == 1);
	CHECK(v[6] == 2);
	CHECK(v[7] == 7);
	CHECK(v[8] == 9);
	CHECK(v[9] == 9);
	CHECK(v[10] == 10);
	CHECK(v[11] == 11);
}
