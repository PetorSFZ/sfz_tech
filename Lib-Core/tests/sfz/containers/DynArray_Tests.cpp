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

#include <skipifzero.hpp>

#include "sfz/containers/DynArray.hpp"
#include "sfz/memory/DebugAllocator.hpp"
#include "sfz/memory/SmartPointers.hpp"

using namespace sfz;

TEST_CASE("Default constructor", "[sfz::DynArray]")
{
	sfz::setContext(sfz::getStandardContext());

	DynArray<float> floatArray;
	REQUIRE(floatArray.size() == 0);
	REQUIRE(floatArray.capacity() == 0);
	REQUIRE(floatArray.data() == nullptr);
	REQUIRE(floatArray.allocator() == nullptr);
}

TEST_CASE("init() with 0 does not allocate memory", "[sfz::DynArray]")
{
	sfz::setContext(sfz::getStandardContext());

	DynArray<float> v;
	v.init(0, getDefaultAllocator(), sfz_dbg(""));
	REQUIRE(v.size() == 0);
	REQUIRE(v.capacity() == 0);
	REQUIRE(v.data() == nullptr);
	REQUIRE(v.allocator() == getDefaultAllocator());

	v.add(1.0f);
	REQUIRE(v.size() == 1);
	REQUIRE(v.capacity() == DYNARRAY_DEFAULT_INITIAL_CAPACITY);
	REQUIRE(v.data() != nullptr);
	REQUIRE(v.allocator() == getDefaultAllocator());
}

TEST_CASE("Fill constructor", "[sfz::DynArray]")
{
	sfz::setContext(sfz::getStandardContext());

	DynArray<int> twos(0, getDefaultAllocator(), sfz_dbg(""));
	twos.add(2, 8);

	for (uint32_t i = 0; i < 8; ++i) {
		REQUIRE(twos.data()[i] == 2);
	}
	REQUIRE(twos.size() == 8);
	REQUIRE(twos.capacity() == DYNARRAY_DEFAULT_INITIAL_CAPACITY);

	twos.destroy();
	REQUIRE(twos.data() == nullptr);
	REQUIRE(twos.size() == 0);
	REQUIRE(twos.capacity() == 0);
}

TEST_CASE("Copy constructors", "[sfz::DynArray]")
{
	sfz::setContext(sfz::getStandardContext());

	DynArray<int> first(0, getDefaultAllocator(), sfz_dbg(""));
	first.add(3, 3);
	DynArray<int> second;

	REQUIRE(first.size() == 3);
	REQUIRE(first.capacity() == DYNARRAY_DEFAULT_INITIAL_CAPACITY);
	REQUIRE(first.data()[0] == 3);
	REQUIRE(first.data()[1] == 3);
	REQUIRE(first.data()[2] == 3);

	REQUIRE(second.size() == 0);
	REQUIRE(second.capacity() == 0);
	REQUIRE(second.data() == nullptr);
	REQUIRE(second.allocator() == nullptr);

	second = first;
	first.destroy();

	REQUIRE(first.size() == 0);
	REQUIRE(first.capacity() == 0);
	REQUIRE(first.data() == nullptr);
	REQUIRE(first.allocator() == nullptr);

	REQUIRE(second.size() == 3);
	REQUIRE(second.capacity() == DYNARRAY_DEFAULT_INITIAL_CAPACITY);
	REQUIRE(second.data()[0] == 3);
	REQUIRE(second.data()[1] == 3);
	REQUIRE(second.data()[2] == 3);
	REQUIRE(second.allocator() == getDefaultAllocator());
}

TEST_CASE("Swap & move constructors", "[sfz::DynArray]")
{
	sfz::setContext(sfz::getStandardContext());

	DynArray<int> v1;
	DynArray<int> v2(32, getDefaultAllocator(), sfz_dbg(""));
	v2.add(42, 2);

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
	sfz::setContext(sfz::getStandardContext());

	DynArray<int> v{4, getDefaultAllocator(), sfz_dbg("")};
	v.hackSetSize(4);
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
	sfz::setContext(sfz::getStandardContext());

	DynArray<int> v{4, getDefaultAllocator(), sfz_dbg("")};
	v.hackSetSize(4);
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
	sfz::setContext(sfz::getStandardContext());

	DynArray<int> v(2, getDefaultAllocator(), sfz_dbg(""));
	REQUIRE(v.size() == 0);
	REQUIRE(v.capacity() == 2);
	v.add(-1, 2);

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

	DynArray<UniquePtr<int>> v2(0, getDefaultAllocator(), sfz_dbg(""));;

	REQUIRE(v2.size() == 0);
	REQUIRE(v2.capacity() == 0);
	REQUIRE(v2.data() == nullptr);

	v2.add(makeUniqueDefault<int>(3));

	REQUIRE(v2.size() == 1);
	REQUIRE(v2.capacity() == DYNARRAY_DEFAULT_INITIAL_CAPACITY);
	REQUIRE(*v2[0] == 3);

	UniquePtr<int> b = makeUniqueDefault<int>(42);

	v2.add(std::move(b));

	REQUIRE(v2.size() == 2);
	REQUIRE(v2.capacity() == DYNARRAY_DEFAULT_INITIAL_CAPACITY);
	REQUIRE(*v2[0] == 3);
	REQUIRE(*v2[1] == 42);


	DynArray<int> v3(0, getDefaultAllocator(), sfz_dbg(""));
	v3.add(v.data(), v.size());
	v3.add(v.data(), v.size());
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
	sfz::setContext(sfz::getStandardContext());

	DynArray<int> v(2, getDefaultAllocator(), sfz_dbg(""));
	REQUIRE(v.size() == 0);
	REQUIRE(v.capacity() == 2);
	v.add(-1, 2);

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

	DynArray<int> v2(0, getDefaultAllocator(), sfz_dbg(""));
	v2.add(42, 3);
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
	sfz::setContext(sfz::getStandardContext());

	SECTION("Basic test") {
		DynArray<int> v(0, getDefaultAllocator(), sfz_dbg(""));
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
		DynArray<vec2_i32> v(0, getDefaultAllocator(), sfz_dbg(""));
		const vec2_i32 vals[] = {vec2_i32(1), vec2_i32(2), vec2_i32(3), vec2_i32(4)};
		v.add(vals, 4);

		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == vec2_i32(1));
		REQUIRE(v[1] == vec2_i32(2));
		REQUIRE(v[2] == vec2_i32(3));
		REQUIRE(v[3] == vec2_i32(4));

		v.remove(1, 2);
		REQUIRE(v.size() == 2);
		REQUIRE(v[0] == vec2_i32(1));
		REQUIRE(v[1] == vec2_i32(4));
	}
	SECTION("Bug where not enough elements are moved")
	{
		DynArray<int> v(0, getDefaultAllocator(), sfz_dbg(""));
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

TEST_CASE("removeQuickSwap()", "[sfz::DynArray]")
{
	sfz::setContext(sfz::getStandardContext());
	DynArray<int> v(0, getDefaultAllocator(), sfz_dbg(""));
	const int vals[] = {1, 2, 3, 4, 5, 6};
	v.add(vals, 6);

	REQUIRE(v.size() == 6);
	REQUIRE(v.first() == 1);
	REQUIRE(v.last() == 6);

	v.removeQuickSwap(0);
	REQUIRE(v.size() == 5);
	REQUIRE(v.last() == 5);
	REQUIRE(v.first() == 6);

	v.removeQuickSwap(1);
	REQUIRE(v.size() == 4);
	REQUIRE(v.last() == 4);
	REQUIRE(v[1] == 5);
}

TEST_CASE("search()", "[sfz::DynArray]")
{
	sfz::setContext(sfz::getStandardContext());

	DynArray<int> v(0, getDefaultAllocator(), sfz_dbg(""));
	const int vals[] = {1, 2, 2, 4};
	v.add(vals, 4);

	int* ptr = v.search(0);
	REQUIRE(ptr == nullptr);

	ptr = v.search(5);
	REQUIRE(ptr == nullptr);

	ptr = v.search(1);
	REQUIRE(ptr != nullptr);
	REQUIRE((ptr - v.data()) == 0);

	ptr = v.search(2);
	REQUIRE(ptr != nullptr);
	REQUIRE((ptr - v.data()) == 1);

	ptr = v.search(4);
	REQUIRE(ptr != nullptr);
	REQUIRE((ptr - v.data()) == 3);
}

TEST_CASE("find()", "[sfz::DynArray]")
{
	sfz::setContext(sfz::getStandardContext());

	DynArray<int> v(0, getDefaultAllocator(), sfz_dbg(""));
	const int vals[] = {1, 2, 3, 4};
	v.add(vals, 4);

	int* ptr = v.find([](int) { return false; });
	REQUIRE(ptr == nullptr);

	ptr = v.find([](int) { return true; });
	REQUIRE(ptr != nullptr);
	REQUIRE(*ptr == 1);

	ptr = v.find([](int param) { return param == 2; });
	REQUIRE(ptr != nullptr);
	REQUIRE(*ptr == 2);

	{
		const DynArray<int>& vc = v;

		const int* ptr2 = vc.find([](int) { return false; });
		REQUIRE(ptr2 == nullptr);

		ptr2 = vc.find([](int) { return true; });
		REQUIRE(ptr2 != nullptr);
		REQUIRE(*ptr2 == 1);

		ptr2 = vc.find([](int param) { return param == 2; });
		REQUIRE(ptr2 != nullptr);
		REQUIRE(*ptr2 == 2);
	}
}

TEST_CASE("Allocator bug", "[sfz::DynArray]")
{
	sfz::setContext(sfz::getStandardContext());

	DebugAllocator debugAlloc("DebugAlloc", 4u);
	{
		DynArray<DynArray<uint32_t>> arr(0, &debugAlloc, sfz_dbg(""));
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

		DynArray<DynArray<uint32_t>> arr2(0, &debugAlloc, sfz_dbg(""));
		for (uint32_t i = 0; i < 250; i++) {
			DynArray<uint32_t> tmp(i * 100, &debugAlloc, sfz_dbg(""));
			tmp.add(0u, i * 10);
			arr2.add(std::move(tmp));
		}
	}
	REQUIRE(debugAlloc.numAllocations() == 0);
}
