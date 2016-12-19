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

#include "sfz/math/MathPrimitiveHashers.hpp"

TEST_CASE("Hashing", "[sfz::Vector]")
{
	sfz::Vector<int, 3> v1{2, 100, 32};
	sfz::Vector<int, 3> v2{-1, 0, -10};
	sfz::Vector<int, 3> v3{0, -9, 14};

	REQUIRE(sfz::hash(v1) != sfz::hash(v2));
	REQUIRE(sfz::hash(v2) != sfz::hash(v3));

	std::hash<sfz::Vector<int, 3>> hasher;

	REQUIRE(hasher(v1) == sfz::hash(v1));
	REQUIRE(hasher(v2) == sfz::hash(v2));
	REQUIRE(hasher(v3) == sfz::hash(v3));
}

TEST_CASE("Matrix hashing", "[sfz::Matrix]")
{
	sfz::Matrix<int32_t,2,2> m1(2, 100,
	                            1, -99);
	sfz::Matrix<int32_t,2,2> m2(-1, 0,
	                            3, -10);
	sfz::Matrix<int32_t,2,2> m3(0, -9,
	                            32, 14);

	REQUIRE(hash(m1) != hash(m2));
	REQUIRE(hash(m2) != hash(m3));

	std::hash<sfz::Matrix<int32_t,2,2>> hasher;

	REQUIRE(hasher(m1) == sfz::hash(m1));
	REQUIRE(hasher(m2) == sfz::hash(m2));
	REQUIRE(hasher(m3) == sfz::hash(m3));
}
