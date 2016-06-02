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

#include "sfz/containers/HashMap.hpp"

using namespace sfz;

TEST_CASE("HashMap: Default constructor", "[sfz::HashMap]")
{
	HashMap<int,int> m1;
	REQUIRE(m1.size() == 0);
	REQUIRE(m1.capacity() == 0);
	REQUIRE(m1.placeholders() == 0);
}

TEST_CASE("HashMap: Copy constructors", "[sfz::HashMap]")
{
	HashMap<int,int> m1(1);
	m1.put(1, 2);
	m1.put(2, 3);
	m1.put(3, 4);
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

TEST_CASE("HashMap: Swap & move constructors", "[sfz::HashMap]")
{
	HashMap<int,int> v1;
	HashMap<int,int> v2(1);
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
	HashMap<int,int> m1;
	REQUIRE(m1.capacity() == 0);
	REQUIRE(m1.size() == 0);
	REQUIRE(m1.placeholders() == 0);

	m1.rehash(1);
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
	
	m1.rehash(0);
	REQUIRE(m1[1] == 2);
	REQUIRE(m1[2] == 3);
	REQUIRE(m1[3] == 4);
	REQUIRE(m1.size() == 3);

	m1.rehash(m1.capacity() + 4);
	REQUIRE(m1[1] == 2);
	REQUIRE(m1[2] == 3);
	REQUIRE(m1[3] == 4);
	REQUIRE(m1.size() == 3);
}

TEST_CASE("HashMap: Rehashing in put()", "[sfz::HashMap]")
{
	HashMap<int,int> m1;
	REQUIRE(m1.size() == 0);
	REQUIRE(m1.capacity() == 0);

	for (int i = 0; i < 128; ++i) {
		m1.put(i, i + 1);
		REQUIRE(m1.size() == uint32_t(i+1));
	}

	for (int i = 0; i < 128; ++i) {
		REQUIRE(m1.get(uint32_t(i)) != nullptr);
		REQUIRE(*m1.get(uint32_t(i)) == (i+1));
	}
}

TEST_CASE("HashMap: Adding and retrieving elements", "[sfz::HashMap]")
{
	HashMap<int,int> m1;

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

	const HashMap<int, int>& mConst = m1;
	REQUIRE(mConst.size() == 2);
	REQUIRE(*mConst.get(2) == 3);
	REQUIRE(*mConst.get(3) == 1);
	REQUIRE(mConst.get(6) == nullptr);
	REQUIRE(mConst.get(0) == nullptr);
	REQUIRE(mConst.get(1) == nullptr);

	REQUIRE(m1.placeholders() == 0);
}

struct ZeroHash {
	size_t operator() (const int&)
	{
		return 0;
	}
};

TEST_CASE("HashMap: Hashing conflicts", "[sfz::HashMap]")
{
	HashMap<int,int,ZeroHash> m;
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
		REQUIRE((pair.key - 1337) == pair.value);
	}
	REQUIRE(numPairs == sizeCount);

	// Const iterators
	const auto& constRef = m;
	numPairs = 0;
	for (auto pair : constRef) {
		numPairs += 1;
		REQUIRE(m[pair.key] == pair.value);
		REQUIRE((pair.key - 1337) == pair.value);
	}
	REQUIRE(numPairs == sizeCount);
}

TEST_CASE("HashMap operator[]", "[sfz::HashMap]")
{
	HashMap<int, int> m(1);
	REQUIRE(m.size() == 0);
	REQUIRE(m.capacity() != 0);

	uint32_t sizeCount = 0;
	for (int i = -32; i <= 32; ++i) {
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