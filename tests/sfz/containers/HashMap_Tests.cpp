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
}

// TODO: Copy constructors

TEST_CASE("HashMap: Swap & move constructors", "[sfz::DynArray]")
{
	HashMap<int,int> v1;
	HashMap<int,int> v2(1);
	v2.add(1, 2);
	v2.add(2, 3);
	v2.add(3, 4);

	REQUIRE(v1.size() == 0);
	REQUIRE(v1.capacity() == 0);
	REQUIRE(v2.size() == 3);
	REQUIRE(v2.capacity() != 0);

	v1.swap(v2);

	REQUIRE(v1.size() == 3);
	REQUIRE(v1.capacity() != 0);
	REQUIRE(v2.size() == 0);
	REQUIRE(v2.capacity() == 0);

	v1 = std::move(v2);

	REQUIRE(v1.size() == 0);
	REQUIRE(v1.capacity() == 0);
	REQUIRE(v2.size() == 3);
	REQUIRE(v2.capacity() != 0);
}

TEST_CASE("HashMap: Adding and retrieving elements", "[sfz::HashMap]")
{
	HashMap<int,int> m1(64);

	REQUIRE(m1.size() == 0);
	REQUIRE(m1.capacity() == 67);

	m1.add(2, 3);
	REQUIRE(*m1.get(2) == 3);
	REQUIRE(m1.size() == 1);

	m1.add(3, 1);
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
}

size_t zeroHash(const int& i) noexcept
{
	return 0;
}

TEST_CASE("HashMap: Hashing conflicts", "[sfz::HashMap]")
{
	HashMap<int,int,zeroHash> m(1);
	REQUIRE(m.size() == 0);
	REQUIRE(m.capacity() != 0);

	uint32_t sizeCount = 0;
	for (int i = -24; i <= 24; ++i) {
		REQUIRE(m.add(i, i - 1337));
		sizeCount += 1;
		REQUIRE(m.size() == sizeCount);
		REQUIRE(m.get(i) != nullptr);
		REQUIRE(*m.get(i) == (i - 1337));
	}

	for (int i = -24; i <= 24; ++i) {
		REQUIRE(m.get(i) != nullptr);
		REQUIRE(*m.get(i) == (i - 1337));
	}
}
