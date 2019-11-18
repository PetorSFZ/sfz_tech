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
#include "skipifzero_arrays.hpp"

// ArrayDynamic tests
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

	void swap(Uncopiable& other) { std::swap(this->val, other.val); }
};

UTEST(ArrayDynamic, default_constructor)
{
	sfz::ArrayDynamic<float> floatArray;
	ASSERT_EQ(floatArray.size(), 0);
	ASSERT_EQ(floatArray.capacity(), 0);
	ASSERT_EQ(floatArray.data(), nullptr);
	ASSERT_EQ(floatArray.allocator(), nullptr);
}

UTEST(ArrayDynamic, init_with_0_does_not_allocate)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<float> v;
	v.init(0, &allocator, sfz_dbg(""));
	ASSERT_EQ(v.size(), 0);
	ASSERT_EQ(v.capacity(), 0);
	ASSERT_EQ(v.data(), nullptr);
	ASSERT_EQ(v.allocator(), &allocator);

	v.add(1.0f);
	ASSERT_EQ(v.size(), 1);
	ASSERT_EQ(v.capacity(), sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	ASSERT_NE(v.data(), nullptr);
	ASSERT_EQ(v.allocator(), &allocator);
}

UTEST(ArrayDynamic, fill_constructor)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<int> twos(0, &allocator, sfz_dbg(""));
	twos.add(2, 8);

	for (uint32_t i = 0; i < 8; ++i) {
		ASSERT_EQ(twos.data()[i], 2);
	}
	ASSERT_EQ(twos.size(), 8);
	ASSERT_EQ(twos.capacity(), sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);

	twos.destroy();
	ASSERT_EQ(twos.size(), 0);
	ASSERT_EQ(twos.capacity(), 0);
	ASSERT_EQ(twos.data(), nullptr);
	ASSERT_EQ(twos.allocator(), nullptr);
}

UTEST(ArrayDynamic, copy_constructors)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<int> first(0, &allocator, sfz_dbg(""));
	first.add(3, 3);
	sfz::ArrayDynamic<int> second;

	ASSERT_EQ(first.size(), 3);
	ASSERT_EQ(first.capacity(), sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	ASSERT_EQ(first.allocator(), &allocator);
	ASSERT_EQ(first.data()[0], 3);
	ASSERT_EQ(first.data()[1], 3);
	ASSERT_EQ(first.data()[2], 3);

	ASSERT_EQ(second.size(), 0);
	ASSERT_EQ(second.capacity(), 0);
	ASSERT_EQ(second.data(), nullptr);
	ASSERT_EQ(second.allocator(), nullptr);

	second = first;
	first.destroy();

	ASSERT_EQ(first.size(), 0);
	ASSERT_EQ(first.capacity(), 0);
	ASSERT_EQ(first.data(), nullptr);
	ASSERT_EQ(first.allocator(), nullptr);

	ASSERT_EQ(second.size(), 3);
	ASSERT_EQ(second.capacity(), sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	ASSERT_EQ(second.allocator(), &allocator);
	ASSERT_EQ(second.data()[0], 3);
	ASSERT_EQ(second.data()[1], 3);
	ASSERT_EQ(second.data()[2], 3);
}

UTEST(ArrayDynamic, swap_move_constructors)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<int> v1;
	sfz::ArrayDynamic<int> v2(32, &allocator, sfz_dbg(""));
	v2.add(42, 2);

	ASSERT_EQ(v1.size(), 0);
	ASSERT_EQ(v1.capacity(), 0);
	ASSERT_EQ(v1.data(), nullptr);
	ASSERT_EQ(v1.allocator(), nullptr);

	ASSERT_EQ(v2.size(), 2);
	ASSERT_EQ(v2.capacity(), 32);
	ASSERT_NE(v2.data(), nullptr);
	ASSERT_EQ(v2.allocator(), &allocator);
	ASSERT_EQ(v2[0], 42);
	ASSERT_EQ(v2[1], 42);

	v1.swap(v2);

	ASSERT_EQ(v2.size(), 0);
	ASSERT_EQ(v2.capacity(), 0);
	ASSERT_EQ(v2.data(), nullptr);
	ASSERT_EQ(v2.allocator(), nullptr);

	ASSERT_EQ(v1.size(), 2);
	ASSERT_EQ(v1.capacity(), 32);
	ASSERT_NE(v1.data(), nullptr);
	ASSERT_EQ(v1.allocator(), &allocator);
	ASSERT_EQ(v1[0], 42);
	ASSERT_EQ(v1[1], 42);

	v1 = std::move(v2);

	ASSERT_EQ(v1.size(), 0);
	ASSERT_EQ(v1.capacity(), 0);
	ASSERT_EQ(v1.data(), nullptr);
	ASSERT_EQ(v1.allocator(), nullptr);

	ASSERT_EQ(v2.size(), 2);
	ASSERT_EQ(v2.capacity(), 32);
	ASSERT_NE(v2.data(), nullptr);
	ASSERT_EQ(v2.allocator(), &allocator);
	ASSERT_EQ(v2[0], 42);
	ASSERT_EQ(v2[1], 42);
}

UTEST(ArrayDynamic, access_operator)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<int> v(4, &allocator, sfz_dbg(""));
	v.hackSetSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	const auto& cv = v;
	ASSERT_EQ(cv[0], 0);
	ASSERT_EQ(cv[1], 1);
	ASSERT_EQ(cv[2], 2);
	ASSERT_EQ(cv[3], 3);
}

UTEST(ArrayDynamic, iterators)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<int> v(4, &allocator, sfz_dbg(""));
	v.hackSetSize(4);
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	int curr = 0;
	for (int val : v) {
		ASSERT_EQ(val, curr);
		curr += 1;
	}
}

UTEST(ArrayDynamic, add)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<int> v(2, &allocator, sfz_dbg(""));
	ASSERT_EQ(v.size(), 0);
	ASSERT_EQ(v.capacity(), 2);

	v.add(-1, 2);
	ASSERT_EQ(v.size(), 2);
	ASSERT_EQ(v.capacity(), 2);
	ASSERT_EQ(v[0], -1);
	ASSERT_EQ(v[1], -1);

	int a = 3;
	v.add(a);
	ASSERT_EQ(v.size(), 3);
	ASSERT_EQ(v.capacity(), 3);
	ASSERT_EQ(v[0], -1);
	ASSERT_EQ(v[1], -1);
	ASSERT_EQ(v[2], 3);

	v.add(a);
	ASSERT_EQ(v.size(), 4);
	ASSERT_EQ(v.capacity(), 5);
	ASSERT_EQ(v[0], -1);
	ASSERT_EQ(v[1], -1);
	ASSERT_EQ(v[2], 3);
	ASSERT_EQ(v[3], 3);

	sfz::ArrayDynamic<Uncopiable> v2(0, &allocator, sfz_dbg(""));;

	ASSERT_EQ(v2.size(), 0);
	ASSERT_EQ(v2.capacity(), 0);
	ASSERT_EQ(v2.data(), nullptr);
	ASSERT_EQ(v2.allocator(), &allocator);

	v2.add(Uncopiable(3));

	ASSERT_EQ(v2.size(), 1);
	ASSERT_EQ(v2.capacity(), sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	ASSERT_EQ(v2[0].val, 3);

	Uncopiable b = Uncopiable(42);
	v2.add(std::move(b));

	ASSERT_EQ(v2.size(), 2);
	ASSERT_EQ(v2.capacity(), sfz::ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY);
	ASSERT_EQ(v2[0].val, 3);
	ASSERT_EQ(v2[1].val, 42);

	sfz::ArrayDynamic<int> v3(0, &allocator, sfz_dbg(""));
	v3.add(v.data(), v.size());
	v3.add(v.data(), v.size());
	ASSERT_EQ(v3.size(), 8);
	ASSERT_EQ(v3[0], -1);
	ASSERT_EQ(v3[1], -1);
	ASSERT_EQ(v3[2], 3);
	ASSERT_EQ(v3[3], 3);
	ASSERT_EQ(v3[4], -1);
	ASSERT_EQ(v3[5], -1);
	ASSERT_EQ(v3[6], 3);
	ASSERT_EQ(v3[7], 3);
}

UTEST(ArrayDynamic, insert)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<int> v(2, &allocator, sfz_dbg(""));
	ASSERT_EQ(v.size(), 0);
	ASSERT_EQ(v.capacity(), 2);
	
	v.add(-1, 2);
	ASSERT_EQ(v.size(), 2);
	ASSERT_EQ(v.capacity(), 2);
	ASSERT_EQ(v[0], -1);
	ASSERT_EQ(v[1], -1);

	int a = 3;
	v.insert(0, a);
	ASSERT_EQ(v.size(), 3);
	ASSERT_EQ(v.capacity(), 3);
	ASSERT_EQ(v[0], 3);
	ASSERT_EQ(v[1], -1);
	ASSERT_EQ(v[2], -1);

	v.insert(2, a);
	ASSERT_EQ(v.size(), 4);
	ASSERT_EQ(v.capacity(), 5);
	ASSERT_EQ(v[0], 3);
	ASSERT_EQ(v[1], -1);
	ASSERT_EQ(v[2], 3);
	ASSERT_EQ(v[3], -1);

	sfz::ArrayDynamic<int> v2(0, &allocator, sfz_dbg(""));
	v2.add(42, 3);
	v.insert(1, v2.data(), 2);
	ASSERT_EQ(v.size(), 6);
	ASSERT_EQ(v[0], 3);
	ASSERT_EQ(v[1], 42);
	ASSERT_EQ(v[2], 42);
	ASSERT_EQ(v[3], -1);
	ASSERT_EQ(v[4], 3);
	ASSERT_EQ(v[5], -1);
}

UTEST(ArrayDynamic, remove)
{
	sfz::StandardAllocator allocator;

	// Basic test
	{
		sfz::ArrayDynamic<int> v(0, &allocator, sfz_dbg(""));
		const int vals[] ={1, 2, 3, 4};
		v.add(vals, 4);

		ASSERT_EQ(v.size(), 4);
		ASSERT_EQ(v[0], 1);
		ASSERT_EQ(v[1], 2);
		ASSERT_EQ(v[2], 3);
		ASSERT_EQ(v[3], 4);

		v.remove(3, 1000);
		ASSERT_EQ(v.size(), 3);
		ASSERT_EQ(v[0], 1);
		ASSERT_EQ(v[1], 2);
		ASSERT_EQ(v[2], 3);

		v.remove(0, 2);
		ASSERT_EQ(v.size(), 1);
		ASSERT_EQ(v[0], 3);
	}

	// Bug where memmove was passed numElements instead of numBytes
	{
		using sfz::vec2_i32;
		sfz::ArrayDynamic<vec2_i32> v(0, &allocator, sfz_dbg(""));
		const vec2_i32 vals[] = {vec2_i32(1), vec2_i32(2), vec2_i32(3), vec2_i32(4)};
		v.add(vals, 4);

		ASSERT_EQ(v.size(), 4);
		ASSERT_EQ(v[0], vec2_i32(1));
		ASSERT_EQ(v[1], vec2_i32(2));
		ASSERT_EQ(v[2], vec2_i32(3));
		ASSERT_EQ(v[3], vec2_i32(4));

		v.remove(1, 2);
		ASSERT_EQ(v.size(), 2);
		ASSERT_EQ(v[0], vec2_i32(1));
		ASSERT_EQ(v[1], vec2_i32(4));
	}

	// Bug where not enough elements are moved
	{
		sfz::ArrayDynamic<int> v(0, &allocator, sfz_dbg(""));
		const int vals[] = {1, 2, 3, 4, 5, 6};
		v.add(vals, 6);

		ASSERT_EQ(v.size(), 6);
		ASSERT_EQ(v[0], 1);
		ASSERT_EQ(v[1], 2);
		ASSERT_EQ(v[2], 3);
		ASSERT_EQ(v[3], 4);
		ASSERT_EQ(v[4], 5);
		ASSERT_EQ(v[5], 6);

		v.remove(0, 1);
		ASSERT_EQ(v.size(), 5);
		ASSERT_EQ(v[0], 2);
		ASSERT_EQ(v[1], 3);
		ASSERT_EQ(v[2], 4);
		ASSERT_EQ(v[3], 5);
		ASSERT_EQ(v[4], 6);
	}
}

UTEST(ArrayDynamic, removeQuickSwap)
{
	sfz::StandardAllocator allocator;
	sfz::ArrayDynamic<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = {1, 2, 3, 4, 5, 6};
	v.add(vals, 6);

	ASSERT_EQ(v.size(), 6);
	ASSERT_EQ(v.first(), 1);
	ASSERT_EQ(v.last(), 6);

	v.removeQuickSwap(0);
	ASSERT_EQ(v.size(), 5);
	ASSERT_EQ(v.last(), 5);
	ASSERT_EQ(v.first(), 6);

	v.removeQuickSwap(1);
	ASSERT_EQ(v.size(), 4);
	ASSERT_EQ(v.last(), 4);
	ASSERT_EQ(v[1], 5);
}

UTEST(ArrayDynamic, search)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = {1, 2, 2, 4};
	v.add(vals, 4);

	int* ptr = v.search(0);
	ASSERT_EQ(ptr, nullptr);

	ptr = v.search(5);
	ASSERT_EQ(ptr, nullptr);

	ptr = v.search(1);
	ASSERT_NE(ptr, nullptr);
	ASSERT_EQ((ptr - v.data()), 0);

	ptr = v.search(2);
	ASSERT_NE(ptr, nullptr);
	ASSERT_EQ((ptr - v.data()), 1);

	ptr = v.search(4);
	ASSERT_NE(ptr, nullptr);
	ASSERT_EQ((ptr - v.data()), 3);
}

UTEST(ArrayDynamic, find)
{
	sfz::StandardAllocator allocator;

	sfz::ArrayDynamic<int> v(0, &allocator, sfz_dbg(""));
	const int vals[] = {1, 2, 3, 4};
	v.add(vals, 4);

	int* ptr = v.find([](int) { return false; });
	ASSERT_EQ(ptr, nullptr);

	ptr = v.find([](int) { return true; });
	ASSERT_NE(ptr, nullptr);
	ASSERT_EQ(*ptr, 1);

	ptr = v.find([](int param) { return param == 2; });
	ASSERT_NE(ptr, nullptr);
	ASSERT_EQ(*ptr, 2);

	{
		const sfz::ArrayDynamic<int>& vc = v;

		const int* ptr2 = vc.find([](int) { return false; });
		ASSERT_EQ(ptr2, nullptr);

		ptr2 = vc.find([](int) { return true; });
		ASSERT_NE(ptr2, nullptr);
		ASSERT_EQ(*ptr2, 1);

		ptr2 = vc.find([](int param) { return param == 2; });
		ASSERT_NE(ptr2, nullptr);
		ASSERT_EQ(*ptr2, 2);
	}
}
