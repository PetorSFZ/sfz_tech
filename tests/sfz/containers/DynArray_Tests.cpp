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

#include "sfz/containers/DynArray.hpp"
#include "sfz/math/Vector.hpp"
#include "sfz/memory/DebugAllocator.hpp"
#include "sfz/memory/New.hpp"
#include "sfz/memory/SmartPointers.hpp"

using namespace sfz;

TEST_CASE("Default constructor", "[sfz::DynArray]")
{
	DynArray<float> floatArray;
	REQUIRE(floatArray.size() == 0);
	REQUIRE(floatArray.capacity() == 0);
	REQUIRE(floatArray.data() == nullptr);
}

TEST_CASE("Fill constructor", "[sfz::DynArray]")
{
	DynArray<UniquePtr<int>> nullptrs;
	nullptrs.addMany(8);
	for (uint32_t i = 0; i < 8; ++i) {
		REQUIRE(nullptrs.data()[i] == nullptr);
	}
	REQUIRE(nullptrs.size() == 8);
	REQUIRE(nullptrs.capacity() == 8);

	DynArray<int> twos;
	twos.addMany(8, 2);

	for (uint32_t i = 0; i < 8; ++i) {
		REQUIRE(twos.data()[i] == 2);
	}
	REQUIRE(twos.size() == 8);
	REQUIRE(twos.capacity() == 8);

	nullptrs.destroy();
	REQUIRE(nullptrs.data() == nullptr);
	REQUIRE(nullptrs.size() == 0);
	REQUIRE(nullptrs.capacity() == 0);

	twos.destroy();
	REQUIRE(twos.data() == nullptr);
	REQUIRE(twos.size() == 0);
	REQUIRE(twos.capacity() == 0);
}

TEST_CASE("Copy constructors", "[sfz::DynArray]")
{
	DynArray<int> first;
	first.addMany(3, 3);
	DynArray<int> second;

	REQUIRE(first.size() == 3);
	REQUIRE(first.capacity() == 3);
	REQUIRE(first.data()[0] == 3);
	REQUIRE(first.data()[1] == 3);
	REQUIRE(first.data()[2] == 3);

	REQUIRE(second.size() == 0);
	REQUIRE(second.capacity() == 0);
	REQUIRE(second.data() == nullptr);

	second = first;
	first.destroy();

	REQUIRE(first.size() == 0);
	REQUIRE(first.capacity() == 0);
	REQUIRE(first.data() == nullptr);

	REQUIRE(second.size() == 3);
	REQUIRE(second.capacity() == 3);
	REQUIRE(second.data()[0] == 3);
	REQUIRE(second.data()[1] == 3);
	REQUIRE(second.data()[2] == 3);
}

TEST_CASE("Copy constructor with allocator", "[sfz::DynArray]")
{
	DebugAllocator first("first"), second("second");
	REQUIRE(first.numAllocations() == 0);
	REQUIRE(second.numAllocations() == 0);
	{
		DynArray<int> arr1(10, &first);
		REQUIRE(arr1.allocator() == &first);
		REQUIRE(first.numAllocations() == 1);

		arr1.add(2);
		arr1.add(3);
		arr1.add(4);
		REQUIRE(arr1.size() == 3);

		{
			DynArray<int> arr2(arr1, &second);
			REQUIRE(arr2.allocator() == &second);
			REQUIRE(arr2.capacity() == arr1.capacity());
			REQUIRE(arr2.size() == arr1.size());
			REQUIRE(arr2[0] == 2);
			REQUIRE(arr2[1] == 3);
			REQUIRE(arr2[2] == 4);
			REQUIRE(first.numAllocations() == 1);
			REQUIRE(second.numAllocations() == 1);
		}
		REQUIRE(second.numAllocations() == 0);
	}
	REQUIRE(first.numAllocations() == 0);
	REQUIRE(second.numAllocations() == 0);
}

TEST_CASE("Swap & move constructors", "[sfz::DynArray]")
{
	DynArray<int> v1;
	DynArray<int> v2(32);
	v2.addMany(2, 42);

	REQUIRE(v1.size() == 0);
	REQUIRE(v1.capacity() == 0);
	REQUIRE(v1.data() == nullptr);

	REQUIRE(v2.size() == 2);
	REQUIRE(v2.capacity() == 32);
	REQUIRE(v2.data() != nullptr);

	v1.swap(v2);

	REQUIRE(v1.size() == 2);
	REQUIRE(v1.capacity() == 32);
	REQUIRE(v1.data() != nullptr);

	REQUIRE(v2.size() == 0);
	REQUIRE(v2.capacity() == 0);
	REQUIRE(v2.data() == nullptr);

	v1 = std::move(v2);

	REQUIRE(v1.size() == 0);
	REQUIRE(v1.capacity() == 0);
	REQUIRE(v1.data() == nullptr);

	REQUIRE(v2.size() == 2);
	REQUIRE(v2.capacity() == 32);
	REQUIRE(v2.data() != nullptr);
}

TEST_CASE("operator[]", "[sfz::DynArray]")
{
	DynArray<int> v{4};
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	const auto& cv = v;
	REQUIRE(cv[0] == 0);
	REQUIRE(cv[1] == 1);
	REQUIRE(cv[2] == 2);
	REQUIRE(cv[3] == 3);
}

TEST_CASE("iterators", "[sfz::DynArray]")
{
	DynArray<int> v{4};
	v[0] = 0;
	v[1] = 1;
	v[2] = 2;
	v[3] = 3;

	int curr = 0;
	for (int val : v) {
		REQUIRE(val == curr);
		curr += 1;
	}
}

TEST_CASE("add()", "[sfz::DynArray]")
{
	DynArray<int> v(2);
	REQUIRE(v.size() == 0);
	REQUIRE(v.capacity() == 2);
	v.addMany(2, -1);

	REQUIRE(v.size() == 2);
	REQUIRE(v.capacity() == 2);
	REQUIRE(v[0] == -1);
	REQUIRE(v[1] == -1);

	int a = 3;
	v.add(a);
	REQUIRE(v.size() == 3);
	REQUIRE(v.capacity() == 3);
	REQUIRE(v[0] == -1);
	REQUIRE(v[1] == -1);
	REQUIRE(v[2] == 3);

	v.add(a);
	REQUIRE(v.size() == 4);
	REQUIRE(v.capacity() == 5);
	REQUIRE(v[0] == -1);
	REQUIRE(v[1] == -1);
	REQUIRE(v[2] == 3);
	REQUIRE(v[3] == 3);


	DynArray<UniquePtr<int>> v2;

	REQUIRE(v2.size() == 0);
	REQUIRE(v2.capacity() == 0);
	REQUIRE(v2.data() == nullptr);

	v2.add(makeUniqueDefault<int>(3));

	REQUIRE(v2.size() == 1);
	REQUIRE(v2.capacity() == DynArray<UniquePtr<int>>::DEFAULT_INITIAL_CAPACITY);
	REQUIRE(*v2[0] == 3);

	UniquePtr<int> b = makeUniqueDefault<int>(42);

	v2.add(std::move(b));

	REQUIRE(v2.size() == 2);
	REQUIRE(v2.capacity() == DynArray<UniquePtr<int>>::DEFAULT_INITIAL_CAPACITY);
	REQUIRE(*v2[0] == 3);
	REQUIRE(*v2[1] == 42);


	DynArray<int> v3;
	v3.add(v);
	v3.add(v);
	REQUIRE(v3.size() == 8);
	REQUIRE(v3[0] == -1);
	REQUIRE(v3[1] == -1);
	REQUIRE(v3[2] == 3);
	REQUIRE(v3[3] == 3);
	REQUIRE(v3[4] == -1);
	REQUIRE(v3[5] == -1);
	REQUIRE(v3[6] == 3);
	REQUIRE(v3[7] == 3);
}

TEST_CASE("insert()", "[sfz::DynArray]")
{
	DynArray<int> v(2);
	REQUIRE(v.size() == 0);
	REQUIRE(v.capacity() == 2);
	v.addMany(2, -1);

	REQUIRE(v.size() == 2);
	REQUIRE(v.capacity() == 2);
	REQUIRE(v[0] == -1);
	REQUIRE(v[1] == -1);

	int a = 3;
	v.insert(0, a);
	REQUIRE(v.size() == 3);
	REQUIRE(v.capacity() == 3);
	REQUIRE(v[0] == 3);
	REQUIRE(v[1] == -1);
	REQUIRE(v[2] == -1);

	v.insert(2, a);
	REQUIRE(v.size() == 4);
	REQUIRE(v.capacity() == 5);
	REQUIRE(v[0] == 3);
	REQUIRE(v[1] == -1);
	REQUIRE(v[2] == 3);
	REQUIRE(v[3] == -1);

	DynArray<int> v2;
	v2.addMany(3, 42);
	v.insert(1, v2.data(), 2);
	REQUIRE(v.size() == 6);
	REQUIRE(v[0] == 3);
	REQUIRE(v[1] == 42);
	REQUIRE(v[2] == 42);
	REQUIRE(v[3] == -1);
	REQUIRE(v[4] == 3);
	REQUIRE(v[5] == -1);
}

TEST_CASE("remove()", "[sfz::DynArray]")
{
	SECTION("Basic test") {
		DynArray<int> v;
		const int vals[] ={1, 2, 3, 4};
		v.add(vals, 4);

		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == 1);
		REQUIRE(v[1] == 2);
		REQUIRE(v[2] == 3);
		REQUIRE(v[3] == 4);

		v.remove(3, 1000);
		REQUIRE(v.size() == 3);
		REQUIRE(v[0] == 1);
		REQUIRE(v[1] == 2);
		REQUIRE(v[2] == 3);

		v.remove(0, 2);
		REQUIRE(v.size() == 1);
		REQUIRE(v[0] == 3);
	}
	SECTION("Bug where memmove was passed numElements instead of numBytes") {
		DynArray<vec2i> v;
		const vec2i vals[] = {vec2i(1), vec2i(2), vec2i(3), vec2i(4)};
		v.add(vals, 4);

		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == vec2i(1));
		REQUIRE(v[1] == vec2i(2));
		REQUIRE(v[2] == vec2i(3));
		REQUIRE(v[3] == vec2i(4));

		v.remove(1, 2);
		REQUIRE(v.size() == 2);
		REQUIRE(v[0] == vec2i(1));
		REQUIRE(v[1] == vec2i(4));
	}
	SECTION("Bug where not enough elements are moved")
	{
		DynArray<int> v;
		const int vals[] = {1, 2, 3, 4, 5, 6};
		v.add(vals, 6);

		REQUIRE(v.size() == 6);
		REQUIRE(v[0] == 1);
		REQUIRE(v[1] == 2);
		REQUIRE(v[2] == 3);
		REQUIRE(v[3] == 4);
		REQUIRE(v[4] == 5);
		REQUIRE(v[5] == 6);

		v.remove(0, 1);
		REQUIRE(v.size() == 5);
		REQUIRE(v[0] == 2);
		REQUIRE(v[1] == 3);
		REQUIRE(v[2] == 4);
		REQUIRE(v[3] == 5);
		REQUIRE(v[4] == 6);
	}
}

TEST_CASE("find()", "[sfz::DynArray]")
{
	DynArray<int> v;
	const int vals[] = {1, 2, 3, 4};
	v.add(vals, 4);

	int* ptr = v.find([](int) { return false; });
	REQUIRE(ptr == nullptr);
	int64_t index = v.findIndex([](int) { return false; });
	REQUIRE(index == -1);

	ptr = v.find([](int) { return true; });
	REQUIRE(ptr != nullptr);
	REQUIRE(*ptr == 1);
	index = v.findIndex([](int) { return true; });
	REQUIRE(index == 0);

	ptr = v.find([](int param) { return param == 2; });
	REQUIRE(ptr != nullptr);
	REQUIRE(*ptr == 2);
	index = v.findIndex([](int param) { return param == 2; });
	REQUIRE(index == 1);
}

TEST_CASE("Allocator bug", "[sfz::DynArray]")
{
	DebugAllocator debugAlloc("DebugAlloc", 4u);
	{
		DynArray<DynArray<uint32_t>> arr(0, &debugAlloc);
		REQUIRE(arr.size() == 0);
		REQUIRE(arr.capacity() == 0);
		REQUIRE(arr.allocator() == &debugAlloc);

		for (uint32_t i = 0; i < 250; i++) {
			arr.add(DynArray<uint32_t>());
			REQUIRE(arr.allocator() == &debugAlloc);
			REQUIRE(arr.size() == (i + 1));
			DynArray<uint32_t>& inner = arr[i];
			REQUIRE(inner.data() == nullptr);
			REQUIRE(inner.allocator() == nullptr);
			REQUIRE(inner.size() == 0);
			REQUIRE(inner.capacity() == 0);
		}

		DynArray<DynArray<uint32_t>> arr2(0, &debugAlloc);
		for (uint32_t i = 0; i < 250; i++) {
			DynArray<uint32_t> tmp(i * 100, &debugAlloc);
			tmp.addMany(i * 10, 0u);
			arr2.add(std::move(tmp));
		}
	}
	REQUIRE(debugAlloc.numAllocations() == 0);
}
