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

#include <cstdint>

#include "sfz/PushWarnings.hpp"
#include "catch2/catch.hpp"
#include "sfz/PopWarnings.hpp"

#include "sfz/math/MinMax.hpp"

TEST_CASE("sfzMin() float tests", "[sfzMin]")
{
	REQUIRE(sfzMin(0.0f, 0.0f) == 0.0f);

	REQUIRE(sfzMin(-1.0f, 0.0f) == -1.0f);
	REQUIRE(sfzMin(0.0f, -1.0f) == -1.0f);

	REQUIRE(sfzMin(-1.0f, -2.0f) == -2.0f);
	REQUIRE(sfzMin(-2.0f, -1.0f) == -2.0f);

	REQUIRE(sfzMin(1.0f, 0.0f) == 0.0f);
	REQUIRE(sfzMin(0.0f, 1.0f) == 0.0f);

	REQUIRE(sfzMin(1.0f, 2.0f) == 1.0f);
	REQUIRE(sfzMin(2.0f, 1.0f) == 1.0f);
}

TEST_CASE("sfzMax() float tests", "[sfzMax]")
{
	REQUIRE(sfzMax(0.0f, 0.0f) == 0.0f);

	REQUIRE(sfzMax(-1.0f, 0.0f) == 0.0f);
	REQUIRE(sfzMax(0.0f, -1.0f) == 0.0f);

	REQUIRE(sfzMax(-1.0f, -2.0f) == -1.0f);
	REQUIRE(sfzMax(-2.0f, -1.0f) == -1.0f);

	REQUIRE(sfzMax(1.0f, 0.0f) == 1.0f);
	REQUIRE(sfzMax(0.0f, 1.0f) == 1.0f);

	REQUIRE(sfzMax(1.0f, 2.0f) == 2.0f);
	REQUIRE(sfzMax(2.0f, 1.0f) == 2.0f);
}

TEST_CASE("sfzMin() int32_t tests", "[sfzMin]")
{
	REQUIRE(sfzMin(0, 0) == 0);

	REQUIRE(sfzMin(-1, 0) == -1);
	REQUIRE(sfzMin(0, -1) == -1);

	REQUIRE(sfzMin(-1, -2) == -2);
	REQUIRE(sfzMin(-2, -1) == -2);

	REQUIRE(sfzMin(1, 0) == 0);
	REQUIRE(sfzMin(0, 1) == 0);

	REQUIRE(sfzMin(1, 2) == 1);
	REQUIRE(sfzMin(2, 1) == 1);
}

TEST_CASE("sfzMax() int32_t tests", "[sfzMax]")
{
	REQUIRE(sfzMax(0, 0) == 0);

	REQUIRE(sfzMax(-1, 0) == 0);
	REQUIRE(sfzMax(0, -1) == 0);

	REQUIRE(sfzMax(-1, -2) == -1);
	REQUIRE(sfzMax(-2, -1) == -1);

	REQUIRE(sfzMax(1, 0) == 1);
	REQUIRE(sfzMax(0, 1) == 1);

	REQUIRE(sfzMax(1, 2) == 2);
	REQUIRE(sfzMax(2, 1) == 2);
}

TEST_CASE("sfzMin() uint32_t tests", "[sfzMin]")
{
	REQUIRE(sfzMin(0u, 0u) == 0u);

	REQUIRE(sfzMin(1u, 0u) == 0u);
	REQUIRE(sfzMin(0u, 1u) == 0u);

	REQUIRE(sfzMin(1u, 2u) == 1u);
	REQUIRE(sfzMin(2u, 1u) == 1u);
}

TEST_CASE("sfzMax() uint32_t tests", "[sfzMax]")
{
	REQUIRE(sfzMax(0u, 0u) == 0u);

	REQUIRE(sfzMax(1u, 0u) == 1u);
	REQUIRE(sfzMax(0u, 1u) == 1u);

	REQUIRE(sfzMax(1u, 2u) == 2u);
	REQUIRE(sfzMax(2u, 1u) == 2u);
}
