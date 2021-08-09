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

#include <utility>

#include "skipifzero.hpp"
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

	void swap(Uncopiable& other) { sfz::swap(this->val, other.val); }
};

UTEST(Array, default_constructor)
{
	sfz::Array<f32> floatArray;
	ASSERT_TRUE(floatArray.size() == 0);
	ASSERT_TRUE(floatArray.capacity() == 0);
	ASSERT_TRUE(floatArray.data() == nullptr);
	ASSERT_TRUE(floatArray.allocator() == nullptr);
}

UTEST(Array, init_with_0_does_not_allocate)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<f32> v;
	v.init(0, &allocator, sfz_dbg(""));
	ASSERT_TRUE(v.size() == 0);
	ASSERT_TRUE(v.capacity() == 0);
	ASSERT_TRUE(v.data() == nullptr);
	ASSERT_TRUE(v.allocator() == &allocator);

	v.add(1.0f);
	ASSERT_TRUE(v.size() == 1);
	ASSERT_TRUE(v.capacity() == sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	ASSERT_TRUE(v.data() != nullptr);
	ASSERT_TRUE(v.allocator() == &allocator);
}

UTEST(Array, fill_constructor)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> twos(0, &allocator, sfz_dbg(""));
	twos.add(2, 8);

	for (u32 i = 0; i < 8; ++i) {
		ASSERT_TRUE(twos.data()[i] == 2);
	}
	ASSERT_TRUE(twos.size() == 8);
	ASSERT_TRUE(twos.capacity() == sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);

	twos.destroy();
	ASSERT_TRUE(twos.size() == 0);
	ASSERT_TRUE(twos.capacity() == 0);
	ASSERT_TRUE(twos.data() == nullptr);
	ASSERT_TRUE(twos.allocator() == nullptr);
}

UTEST(Array, swap_move_constructors)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> v1;
	sfz::Array<int> v2(32, &allocator, sfz_dbg(""));
	v2.add(42, 2);

	ASSERT_TRUE(v1.size() == 0);
	ASSERT_TRUE(v1.capacity() == 0);
	ASSERT_TRUE(v1.data() == nullptr);
	ASSERT_TRUE(v1.allocator() == nullptr);

	ASSERT_TRUE(v2.size() == 2);
	ASSERT_TRUE(v2.capacity() == 32);
	ASSERT_TRUE(v2.data() != nullptr);
	ASSERT_TRUE(v2.allocator() == &allocator);
	ASSERT_TRUE(v2[0] == 42);
	ASSERT_TRUE(v2[1] == 42);

	v1.swap(v2);

	ASSERT_TRUE(v2.size() == 0);
	ASSERT_TRUE(v2.capacity() == 0);
	ASSERT_TRUE(v2.data() == nullptr);
	ASSERT_TRUE(v2.allocator() == nullptr);

	ASSERT_TRUE(v1.size() == 2);
	ASSERT_TRUE(v1.capacity() == 32);
	ASSERT_TRUE(v1.data() != nullptr);
	ASSERT_TRUE(v1.allocator() == &allocator);
	ASSERT_TRUE(v1[0] == 42);
	ASSERT_TRUE(v1[1] == 42);

	v1 = std::move(v2);

	ASSERT_TRUE(v1.size() == 0);
	ASSERT_TRUE(v1.capacity() == 0);
	ASSERT_TRUE(v1.data() == nullptr);
	ASSERT_TRUE(v1.allocator() == nullptr);

	ASSERT_TRUE(v2.size() == 2);
	ASSERT_TRUE(v2.capacity() == 32);
	ASSERT_TRUE(v2.data() != nullptr);
	ASSERT_TRUE(v2.allocator() == &allocator);
	ASSERT_TRUE(v2[0] == 42);
	ASSERT_TRUE(v2[1] == 42);
}

UTEST(Array, access_operator)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> v(4, &allocator, sfz_dbg(""));
	v.hackSetSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	const auto& cv = v;
	ASSERT_TRUE(cv[0] == 0);
	ASSERT_TRUE(cv[1] == 1);
	ASSERT_TRUE(cv[2] == 2);
	ASSERT_TRUE(cv[3] == 3);
}

UTEST(Array, iterators)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> v(4, &allocator, sfz_dbg(""));
	v.hackSetSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	int curr = 0;
	for (int val : v) {
		ASSERT_TRUE(val == curr);
		curr += 1;
	}
}

UTEST(Array, add)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> v(2, &allocator, sfz_dbg(""));
	ASSERT_TRUE(v.size() == 0);
	ASSERT_TRUE(v.capacity() == 2);

	v.add(-1, 2);
	ASSERT_TRUE(v.size() == 2);
	ASSERT_TRUE(v.capacity() == 2);
	ASSERT_TRUE(v[0] == -1);
	ASSERT_TRUE(v[1] == -1);

	int a = 3;
	v.add(a);
	ASSERT_TRUE(v.size() == 3);
	ASSERT_TRUE(v.capacity() == 3);
	ASSERT_TRUE(v[0] == -1);
	ASSERT_TRUE(v[1] == -1);
	ASSERT_TRUE(v[2] == 3);

	v.add(a);
	ASSERT_TRUE(v.size() == 4);
	ASSERT_TRUE(v.capacity() == 5);
	ASSERT_TRUE(v[0] == -1);
	ASSERT_TRUE(v[1] == -1);
	ASSERT_TRUE(v[2] == 3);
	ASSERT_TRUE(v[3] == 3);

	sfz::Array<Uncopiable> v2(0, &allocator, sfz_dbg(""));;

	ASSERT_TRUE(v2.size() == 0);
	ASSERT_TRUE(v2.capacity() == 0);
	ASSERT_TRUE(v2.data() == nullptr);
	ASSERT_TRUE(v2.allocator() == &allocator);

	v2.add(Uncopiable(3));

	ASSERT_TRUE(v2.size() == 1);
	ASSERT_TRUE(v2.capacity() == sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	ASSERT_TRUE(v2[0].val == 3);

	Uncopiable b = Uncopiable(42);
	v2.add(std::move(b));

	ASSERT_TRUE(v2.size() == 2);
	ASSERT_TRUE(v2.capacity() == sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	ASSERT_TRUE(v2[0].val == 3);
	ASSERT_TRUE(v2[1].val == 42);

	sfz::Array<int> v3(0, &allocator, sfz_dbg(""));
	v3.add(v.data(), v.size());
	v3.add(v.data(), v.size());
	ASSERT_TRUE(v3.size() == 8);
	ASSERT_TRUE(v3[0] == -1);
	ASSERT_TRUE(v3[1] == -1);
	ASSERT_TRUE(v3[2] == 3);
	ASSERT_TRUE(v3[3] == 3);
	ASSERT_TRUE(v3[4] == -1);
	ASSERT_TRUE(v3[5] == -1);
	ASSERT_TRUE(v3[6] == 3);
	ASSERT_TRUE(v3[7] == 3);
}

UTEST(Array, insert)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> v(2, &allocator, sfz_dbg(""));
	ASSERT_TRUE(v.size() == 0);
	ASSERT_TRUE(v.capacity() == 2);

	v.add(-1, 2);
	ASSERT_TRUE(v.size() == 2);
	ASSERT_TRUE(v.capacity() == 2);
	ASSERT_TRUE(v[0] == -1);
	ASSERT_TRUE(v[1] == -1);

	int a = 3;
	v.insert(0, a);
	ASSERT_TRUE(v.size() == 3);
	ASSERT_TRUE(v.capacity() == 3);
	ASSERT_TRUE(v[0] == 3);
	ASSERT_TRUE(v[1] == -1);
	ASSERT_TRUE(v[2] == -1);

	v.insert(2, a);
	ASSERT_TRUE(v.size() == 4);
	ASSERT_TRUE(v.capacity() == 5);
	ASSERT_TRUE(v[0] == 3);
	ASSERT_TRUE(v[1] == -1);
	ASSERT_TRUE(v[2] == 3);
	ASSERT_TRUE(v[3] == -1);

	sfz::Array<int> v2(0, &allocator, sfz_dbg(""));
	v2.add(42, 3);
	v.insert(1, v2.data(), 2);
	ASSERT_TRUE(v.size() == 6);
	ASSERT_TRUE(v[0] == 3);
	ASSERT_TRUE(v[1] == 42);
	ASSERT_TRUE(v[2] == 42);
	ASSERT_TRUE(v[3] == -1);
	ASSERT_TRUE(v[4] == 3);
	ASSERT_TRUE(v[5] == -1);
}

UTEST(Array, remove)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// Basic test
	{
		sfz::Array<int> v(0, &allocator, sfz_dbg(""));
		const int vals[] ={1, 2, 3, 4};
		v.add(vals, 4);

		ASSERT_TRUE(v.size() == 4);
		ASSERT_TRUE(v[0] == 1);
		ASSERT_TRUE(v[1] == 2);
		ASSERT_TRUE(v[2] == 3);
		ASSERT_TRUE(v[3] == 4);

		v.remove(3, 1000);
		ASSERT_TRUE(v.size() == 3);
		ASSERT_TRUE(v[0] == 1);
		ASSERT_TRUE(v[1] == 2);
		ASSERT_TRUE(v[2] == 3);

		v.remove(0, 2);
		ASSERT_TRUE(v.size() == 1);
		ASSERT_TRUE(v[0] == 3);
	}

	// Bug where memmove was passed numElements instead of numBytes
	{
		using sfz::i32x2;
		sfz::Array<i32x2> v(0, &allocator, sfz_dbg(""));
		const i32x2 vals[] = {i32x2(1), i32x2(2), i32x2(3), i32x2(4)};
		v.add(vals, 4);

		ASSERT_TRUE(v.size() == 4);
		ASSERT_TRUE(v[0] == i32x2(1));
		ASSERT_TRUE(v[1] == i32x2(2));
		ASSERT_TRUE(v[2] == i32x2(3));
		ASSERT_TRUE(v[3] == i32x2(4));

		v.remove(1, 2);
		ASSERT_TRUE(v.size() == 2);
		ASSERT_TRUE(v[0] == i32x2(1));
		ASSERT_TRUE(v[1] == i32x2(4));
	}

	// Bug where not enough elements are moved
	{
		sfz::Array<int> v(0, &allocator, sfz_dbg(""));
		const int vals[] = {1, 2, 3, 4, 5, 6};
		v.add(vals, 6);

		ASSERT_TRUE(v.size() == 6);
		ASSERT_TRUE(v[0] == 1);
		ASSERT_TRUE(v[1] == 2);
		ASSERT_TRUE(v[2] == 3);
		ASSERT_TRUE(v[3] == 4);
		ASSERT_TRUE(v[4] == 5);
		ASSERT_TRUE(v[5] == 6);

		v.remove(0, 1);
		ASSERT_TRUE(v.size() == 5);
		ASSERT_TRUE(v[0] == 2);
		ASSERT_TRUE(v[1] == 3);
		ASSERT_TRUE(v[2] == 4);
		ASSERT_TRUE(v[3] == 5);
		ASSERT_TRUE(v[4] == 6);
	}
}

UTEST(Array, removeQuickSwap)
{
	SfzAllocator allocator = sfz::createStandardAllocator();
	sfz::Array<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = {1, 2, 3, 4, 5, 6};
	v.add(vals, 6);

	ASSERT_TRUE(v.size() == 6);
	ASSERT_TRUE(v.first() == 1);
	ASSERT_TRUE(v.last() == 6);

	v.removeQuickSwap(0);
	ASSERT_TRUE(v.size() == 5);
	ASSERT_TRUE(v.last() == 5);
	ASSERT_TRUE(v.first() == 6);

	v.removeQuickSwap(1);
	ASSERT_TRUE(v.size() == 4);
	ASSERT_TRUE(v.last() == 4);
	ASSERT_TRUE(v[1] == 5);
}

UTEST(Array, findElement)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = {1, 2, 2, 4};
	v.add(vals, 4);

	int* ptr = v.findElement(0);
	ASSERT_TRUE(ptr == nullptr);

	ptr = v.findElement(5);
	ASSERT_TRUE(ptr == nullptr);

	ptr = v.findElement(1);
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE((ptr - v.data()) == 0);

	ptr = v.findElement(2);
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE((ptr - v.data()) == 1);

	ptr = v.findElement(4);
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE((ptr - v.data()) == 3);
}

UTEST(Array, find)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = {1, 2, 3, 4};
	v.add(vals, 4);

	int* ptr = v.find([](int) { return false; });
	ASSERT_TRUE(ptr == nullptr);

	ptr = v.find([](int) { return true; });
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(*ptr == 1);

	ptr = v.find([](int param) { return param == 2; });
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(*ptr == 2);

	{
		const sfz::Array<int>& vc = v;

		const int* ptr2 = vc.find([](int) { return false; });
		ASSERT_TRUE(ptr2 == nullptr);

		ptr2 = vc.find([](int) { return true; });
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(*ptr2 == 1);

		ptr2 = vc.find([](int param) { return param == 2; });
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(*ptr2 == 2);
	}
}

UTEST(Array, findLast)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = { 1, 2, 3, 4 };
	v.add(vals, 4);

	int* ptr = v.findLast([](int) { return false; });
	ASSERT_TRUE(ptr == nullptr);

	ptr = v.findLast([](int) { return true; });
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(*ptr == 4);

	ptr = v.findLast([](int param) { return param == 2; });
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(*ptr == 2);

	{
		const sfz::Array<int>& vc = v;

		const int* ptr2 = vc.findLast([](int) { return false; });
		ASSERT_TRUE(ptr2 == nullptr);

		ptr2 = vc.findLast([](int) { return true; });
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(*ptr2 == 4);

		ptr2 = vc.findLast([](int param) { return param == 2; });
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(*ptr2 == 2);
	}
}

UTEST(Array, sort)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::Array<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = { 7, 1, 0, 2, 9, -1, -2, -2, 10, 11, 9, 0 };
	v.add(vals, 12);

	v.sort();
	ASSERT_TRUE(v[0] == -2);
	ASSERT_TRUE(v[1] == -2);
	ASSERT_TRUE(v[2] == -1);
	ASSERT_TRUE(v[3] == 0);
	ASSERT_TRUE(v[4] == 0);
	ASSERT_TRUE(v[5] == 1);
	ASSERT_TRUE(v[6] == 2);
	ASSERT_TRUE(v[7] == 7);
	ASSERT_TRUE(v[8] == 9);
	ASSERT_TRUE(v[9] == 9);
	ASSERT_TRUE(v[10] == 10);
	ASSERT_TRUE(v[11] == 11);

	v.sort([](int lhs, int rhs) {
		return lhs > rhs;
	});
	ASSERT_TRUE(v[0] == 11);
	ASSERT_TRUE(v[1] == 10);
	ASSERT_TRUE(v[2] == 9);
	ASSERT_TRUE(v[3] == 9);
	ASSERT_TRUE(v[4] == 7);
	ASSERT_TRUE(v[5] == 2);
	ASSERT_TRUE(v[6] == 1);
	ASSERT_TRUE(v[7] == 0);
	ASSERT_TRUE(v[8] == 0);
	ASSERT_TRUE(v[9] == -1);
	ASSERT_TRUE(v[10] == -2);
	ASSERT_TRUE(v[11] == -2);
	
	// Sort twice
	v.sort([](int lhs, int rhs) {
		return lhs > rhs;
	});
	ASSERT_TRUE(v[0] == 11);
	ASSERT_TRUE(v[1] == 10);
	ASSERT_TRUE(v[2] == 9);
	ASSERT_TRUE(v[3] == 9);
	ASSERT_TRUE(v[4] == 7);
	ASSERT_TRUE(v[5] == 2);
	ASSERT_TRUE(v[6] == 1);
	ASSERT_TRUE(v[7] == 0);
	ASSERT_TRUE(v[8] == 0);
	ASSERT_TRUE(v[9] == -1);
	ASSERT_TRUE(v[10] == -2);
	ASSERT_TRUE(v[11] == -2);

	v.sort([](int lhs, int rhs) {
		return lhs < rhs;
	});
	ASSERT_TRUE(v[0] == -2);
	ASSERT_TRUE(v[1] == -2);
	ASSERT_TRUE(v[2] == -1);
	ASSERT_TRUE(v[3] == 0);
	ASSERT_TRUE(v[4] == 0);
	ASSERT_TRUE(v[5] == 1);
	ASSERT_TRUE(v[6] == 2);
	ASSERT_TRUE(v[7] == 7);
	ASSERT_TRUE(v[8] == 9);
	ASSERT_TRUE(v[9] == 9);
	ASSERT_TRUE(v[10] == 10);
	ASSERT_TRUE(v[11] == 11);
}

// ArrayLocal tests
// ------------------------------------------------------------------------------------------------

UTEST(ArrayLocal, default_constructor)
{
	alignas(32) sfz::ArrayLocal<f32, 5> fiveArray;
	ASSERT_TRUE(fiveArray.size() == 0);
	ASSERT_TRUE(fiveArray.capacity() == 5);
	ASSERT_TRUE((uintptr_t)fiveArray.data() == (uintptr_t)&fiveArray);
	ASSERT_TRUE(sfz::isAligned(fiveArray.data(), 32));

	alignas(64) sfz::ArrayLocal<f32, 8> eightArray;
	ASSERT_TRUE(eightArray.size() == 0);
	ASSERT_TRUE(eightArray.capacity() == 8);
	ASSERT_TRUE((uintptr_t)eightArray.data() == (uintptr_t)&eightArray);
	ASSERT_TRUE(sfz::isAligned(eightArray.data(), 64));

	sfz::ArrayLocal<sfz::f32x4, 8> vecs;
	ASSERT_TRUE(vecs.size() == 0);
	ASSERT_TRUE(vecs.capacity() == 8);
	ASSERT_TRUE((uintptr_t)vecs.data() == (uintptr_t)&vecs);
	ASSERT_TRUE(sfz::isAligned(vecs.data(), alignof(sfz::f32x4)));
}

UTEST(ArrayLocal, fill_constructor)
{
	sfz::ArrayLocal<int, 16> twos;
	ASSERT_TRUE(twos.capacity() == 16);

	ASSERT_TRUE(twos.size() == 0);
	twos.add(2, 8);
	ASSERT_TRUE(twos.size() == 8);
	for (u32 i = 0; i < 8; i++) {
		ASSERT_TRUE(twos.data()[i] == 2);
	}

	twos.clear();
	ASSERT_TRUE(twos.size() == 0);
}

UTEST(ArrayLocal, copy_constructors)
{
	sfz::ArrayLocal<int, 16> first;
	first.add(3, 3);
	sfz::ArrayLocal<int, 16> second;

	ASSERT_TRUE(first.size() == 3);
	ASSERT_TRUE(first.data()[0] == 3);
	ASSERT_TRUE(first.data()[1] == 3);
	ASSERT_TRUE(first.data()[2] == 3);

	ASSERT_TRUE(second.size() == 0);

	second = first;
	first.clear();

	ASSERT_TRUE(first.size() == 0);

	ASSERT_TRUE(second.size() == 3);
	ASSERT_TRUE(second.data()[0] == 3);
	ASSERT_TRUE(second.data()[1] == 3);
	ASSERT_TRUE(second.data()[2] == 3);
}

UTEST(ArrayLocal, swap_move_constructors)
{
	sfz::ArrayLocal<Uncopiable, 16> v1;
	sfz::ArrayLocal<Uncopiable, 16> v2;
	v2.add(Uncopiable(42));
	v2.add(Uncopiable(42));

	ASSERT_TRUE(v1.size() == 0);
	ASSERT_TRUE(v2.size() == 2);
	ASSERT_TRUE(v2[0].val == 42);
	ASSERT_TRUE(v2[1].val == 42);

	v1.swap(v2);
	ASSERT_TRUE(v2.size() == 0);
	ASSERT_TRUE(v1.size() == 2);
	ASSERT_TRUE(v1[0].val == 42);
	ASSERT_TRUE(v1[1].val == 42);

	v1 = std::move(v2);
	ASSERT_TRUE(v1.size() == 0);
	ASSERT_TRUE(v2.size() == 2);
	ASSERT_TRUE(v2[0].val == 42);
	ASSERT_TRUE(v2[1].val == 42);
}

UTEST(ArrayLocal, access_operator)
{
	sfz::ArrayLocal<int, 16> v;
	v.setSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	const auto& cv = v;
	ASSERT_TRUE(cv[0] == 0);
	ASSERT_TRUE(cv[1] == 1);
	ASSERT_TRUE(cv[2] == 2);
	ASSERT_TRUE(cv[3] == 3);
}

UTEST(ArrayLocal, iterators)
{
	sfz::ArrayLocal<int, 16> v;
	v.setSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	int curr = 0;
	for (int val : v) {
		ASSERT_TRUE(val == curr);
		curr += 1;
	}
}

UTEST(ArrayLocal, add)
{
	sfz::ArrayLocal<int, 16> v;
	ASSERT_TRUE(v.size() == 0);

	v.add(-1, 2);
	ASSERT_TRUE(v.size() == 2);
	ASSERT_TRUE(v[0] == -1);
	ASSERT_TRUE(v[1] == -1);

	int a = 3;
	v.add(a);
	ASSERT_TRUE(v.size() == 3);
	ASSERT_TRUE(v[0] == -1);
	ASSERT_TRUE(v[1] == -1);
	ASSERT_TRUE(v[2] == 3);

	v.add(a);
	ASSERT_TRUE(v.size() == 4);
	ASSERT_TRUE(v[0] == -1);
	ASSERT_TRUE(v[1] == -1);
	ASSERT_TRUE(v[2] == 3);
	ASSERT_TRUE(v[3] == 3);

	sfz::ArrayLocal<Uncopiable, 16> v2;
	ASSERT_TRUE(v2.size() == 0);

	v2.add(Uncopiable(3));

	ASSERT_TRUE(v2.size() == 1);
	ASSERT_TRUE(v2[0].val == 3);

	Uncopiable b = Uncopiable(42);
	v2.add(std::move(b));

	ASSERT_TRUE(v2.size() == 2);
	ASSERT_TRUE(v2[0].val == 3);
	ASSERT_TRUE(v2[1].val == 42);

	sfz::ArrayLocal<int, 16> v3;
	v3.add(v.data(), v.size());
	v3.add(v.data(), v.size());
	ASSERT_TRUE(v3.size() == 8);
	ASSERT_TRUE(v3[0] == -1);
	ASSERT_TRUE(v3[1] == -1);
	ASSERT_TRUE(v3[2] == 3);
	ASSERT_TRUE(v3[3] == 3);
	ASSERT_TRUE(v3[4] == -1);
	ASSERT_TRUE(v3[5] == -1);
	ASSERT_TRUE(v3[6] == 3);
	ASSERT_TRUE(v3[7] == 3);
}

UTEST(ArrayLocal, insert)
{
	sfz::ArrayLocal<int, 21> v;
	ASSERT_TRUE(v.size() == 0);

	v.add(-1, 2);
	ASSERT_TRUE(v.size() == 2);
	ASSERT_TRUE(v[0] == -1);
	ASSERT_TRUE(v[1] == -1);

	int a = 3;
	v.insert(0, a);
	ASSERT_TRUE(v.size() == 3);
	ASSERT_TRUE(v[0] == 3);
	ASSERT_TRUE(v[1] == -1);
	ASSERT_TRUE(v[2] == -1);

	v.insert(2, a);
	ASSERT_TRUE(v.size() == 4);
	ASSERT_TRUE(v[0] == 3);
	ASSERT_TRUE(v[1] == -1);
	ASSERT_TRUE(v[2] == 3);
	ASSERT_TRUE(v[3] == -1);

	sfz::ArrayLocal<int, 23> v2;
	v2.add(42, 3);
	v.insert(1, v2.data(), 2);
	ASSERT_TRUE(v.size() == 6);
	ASSERT_TRUE(v[0] == 3);
	ASSERT_TRUE(v[1] == 42);
	ASSERT_TRUE(v[2] == 42);
	ASSERT_TRUE(v[3] == -1);
	ASSERT_TRUE(v[4] == 3);
	ASSERT_TRUE(v[5] == -1);
}

UTEST(ArrayLocal, remove)
{
	// Basic test
	{
		sfz::ArrayLocal<int, 19> v;
		const int vals[] ={1, 2, 3, 4};
		v.add(vals, 4);

		ASSERT_TRUE(v.size() == 4);
		ASSERT_TRUE(v[0] == 1);
		ASSERT_TRUE(v[1] == 2);
		ASSERT_TRUE(v[2] == 3);
		ASSERT_TRUE(v[3] == 4);

		v.remove(3, 1000);
		ASSERT_TRUE(v.size() == 3);
		ASSERT_TRUE(v[0] == 1);
		ASSERT_TRUE(v[1] == 2);
		ASSERT_TRUE(v[2] == 3);

		v.remove(0, 2);
		ASSERT_TRUE(v.size() == 1);
		ASSERT_TRUE(v[0] == 3);
	}

	// Bug where memmove was passed numElements instead of numBytes
	{
		using sfz::i32x2;
		sfz::ArrayLocal<i32x2, 7> v;
		const i32x2 vals[] = {i32x2(1), i32x2(2), i32x2(3), i32x2(4)};
		v.add(vals, 4);

		ASSERT_TRUE(v.size() == 4);
		ASSERT_TRUE(v[0] == i32x2(1));
		ASSERT_TRUE(v[1] == i32x2(2));
		ASSERT_TRUE(v[2] == i32x2(3));
		ASSERT_TRUE(v[3] == i32x2(4));

		v.remove(1, 2);
		ASSERT_TRUE(v.size() == 2);
		ASSERT_TRUE(v[0] == i32x2(1));
		ASSERT_TRUE(v[1] == i32x2(4));
	}

	// Bug where not enough elements are moved
	{
		sfz::ArrayLocal<int, 11> v;
		const int vals[] = {1, 2, 3, 4, 5, 6};
		v.add(vals, 6);

		ASSERT_TRUE(v.size() == 6);
		ASSERT_TRUE(v[0] == 1);
		ASSERT_TRUE(v[1] == 2);
		ASSERT_TRUE(v[2] == 3);
		ASSERT_TRUE(v[3] == 4);
		ASSERT_TRUE(v[4] == 5);
		ASSERT_TRUE(v[5] == 6);

		v.remove(0, 1);
		ASSERT_TRUE(v.size() == 5);
		ASSERT_TRUE(v[0] == 2);
		ASSERT_TRUE(v[1] == 3);
		ASSERT_TRUE(v[2] == 4);
		ASSERT_TRUE(v[3] == 5);
		ASSERT_TRUE(v[4] == 6);
	}
}

UTEST(ArrayLocal, removeQuickSwap)
{
	sfz::ArrayLocal<int, 13> v;
	const int vals[] = {1, 2, 3, 4, 5, 6};
	v.add(vals, 6);

	ASSERT_TRUE(v.size() == 6);
	ASSERT_TRUE(v.first() == 1);
	ASSERT_TRUE(v.last() == 6);

	v.removeQuickSwap(0);
	ASSERT_TRUE(v.size() == 5);
	ASSERT_TRUE(v.last() == 5);
	ASSERT_TRUE(v.first() == 6);

	v.removeQuickSwap(1);
	ASSERT_TRUE(v.size() == 4);
	ASSERT_TRUE(v.last() == 4);
	ASSERT_TRUE(v[1] == 5);
}

UTEST(ArrayLocal, findElement)
{
	sfz::ArrayLocal<int, 16> v;
	const int vals[] = {1, 2, 2, 4};
	v.add(vals, 4);

	int* ptr = v.findElement(0);
	ASSERT_TRUE(ptr == nullptr);

	ptr = v.findElement(5);
	ASSERT_TRUE(ptr == nullptr);

	ptr = v.findElement(1);
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE((ptr - v.data()) == 0);

	ptr = v.findElement(2);
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE((ptr - v.data()) == 1);

	ptr = v.findElement(4);
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE((ptr - v.data()) == 3);
}

UTEST(ArrayLocal, find)
{
	sfz::ArrayLocal<int, 15> v;
	const int vals[] = {1, 2, 3, 4};
	v.add(vals, 4);

	int* ptr = v.find([](int) { return false; });
	ASSERT_TRUE(ptr == nullptr);

	ptr = v.find([](int) { return true; });
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(*ptr == 1);

	ptr = v.find([](int param) { return param == 2; });
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(*ptr == 2);

	{
		const sfz::ArrayLocal<int, 15>& vc = v;

		const int* ptr2 = vc.find([](int) { return false; });
		ASSERT_TRUE(ptr2 == nullptr);

		ptr2 = vc.find([](int) { return true; });
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(*ptr2 == 1);

		ptr2 = vc.find([](int param) { return param == 2; });
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(*ptr2 == 2);
	}
}

UTEST(ArrayLocal, findLast)
{
	sfz::ArrayLocal<int, 15> v;
	const int vals[] = { 1, 2, 3, 4 };
	v.add(vals, 4);

	int* ptr = v.findLast([](int) { return false; });
	ASSERT_TRUE(ptr == nullptr);

	ptr = v.findLast([](int) { return true; });
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(*ptr == 4);

	ptr = v.findLast([](int param) { return param == 2; });
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(*ptr == 2);

	{
		const sfz::ArrayLocal<int, 15>& vc = v;

		const int* ptr2 = vc.findLast([](int) { return false; });
		ASSERT_TRUE(ptr2 == nullptr);

		ptr2 = vc.findLast([](int) { return true; });
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(*ptr2 == 4);

		ptr2 = vc.findLast([](int param) { return param == 2; });
		ASSERT_TRUE(ptr2 != nullptr);
		ASSERT_TRUE(*ptr2 == 2);
	}
}

UTEST(ArrayLocal, sort)
{
	sfz::ArrayLocal<int, 16> v;
	const int vals[] = { 7, 1, 0, 2, 9, -1, -2, -2, 10, 11, 9, 0 };
	v.add(vals, 12);

	v.sort();
	ASSERT_TRUE(v[0] == -2);
	ASSERT_TRUE(v[1] == -2);
	ASSERT_TRUE(v[2] == -1);
	ASSERT_TRUE(v[3] == 0);
	ASSERT_TRUE(v[4] == 0);
	ASSERT_TRUE(v[5] == 1);
	ASSERT_TRUE(v[6] == 2);
	ASSERT_TRUE(v[7] == 7);
	ASSERT_TRUE(v[8] == 9);
	ASSERT_TRUE(v[9] == 9);
	ASSERT_TRUE(v[10] == 10);
	ASSERT_TRUE(v[11] == 11);

	v.sort([](int lhs, int rhs) {
		return lhs > rhs;
	});
	ASSERT_TRUE(v[0] == 11);
	ASSERT_TRUE(v[1] == 10);
	ASSERT_TRUE(v[2] == 9);
	ASSERT_TRUE(v[3] == 9);
	ASSERT_TRUE(v[4] == 7);
	ASSERT_TRUE(v[5] == 2);
	ASSERT_TRUE(v[6] == 1);
	ASSERT_TRUE(v[7] == 0);
	ASSERT_TRUE(v[8] == 0);
	ASSERT_TRUE(v[9] == -1);
	ASSERT_TRUE(v[10] == -2);
	ASSERT_TRUE(v[11] == -2);
	
	// Sort twice
	v.sort([](int lhs, int rhs) {
		return lhs > rhs;
	});
	ASSERT_TRUE(v[0] == 11);
	ASSERT_TRUE(v[1] == 10);
	ASSERT_TRUE(v[2] == 9);
	ASSERT_TRUE(v[3] == 9);
	ASSERT_TRUE(v[4] == 7);
	ASSERT_TRUE(v[5] == 2);
	ASSERT_TRUE(v[6] == 1);
	ASSERT_TRUE(v[7] == 0);
	ASSERT_TRUE(v[8] == 0);
	ASSERT_TRUE(v[9] == -1);
	ASSERT_TRUE(v[10] == -2);
	ASSERT_TRUE(v[11] == -2);

	v.sort([](int lhs, int rhs) {
		return lhs < rhs;
	});
	ASSERT_TRUE(v[0] == -2);
	ASSERT_TRUE(v[1] == -2);
	ASSERT_TRUE(v[2] == -1);
	ASSERT_TRUE(v[3] == 0);
	ASSERT_TRUE(v[4] == 0);
	ASSERT_TRUE(v[5] == 1);
	ASSERT_TRUE(v[6] == 2);
	ASSERT_TRUE(v[7] == 7);
	ASSERT_TRUE(v[8] == 9);
	ASSERT_TRUE(v[9] == 9);
	ASSERT_TRUE(v[10] == 10);
	ASSERT_TRUE(v[11] == 11);
}
