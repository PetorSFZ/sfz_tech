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

TEST_CASE("sfzMin<float>()", "[sfzMin]")
{
	REQUIRE(sfzMin<float>(0.0f, 0.0f) == 0.0f);

	REQUIRE(sfzMin<float>(-1.0f, 0.0f) == -1.0f);
	REQUIRE(sfzMin<float>(0.0f, -1.0f) == -1.0f);

	REQUIRE(sfzMin<float>(-1.0f, -2.0f) == -2.0f);
	REQUIRE(sfzMin<float>(-2.0f, -1.0f) == -2.0f);

	REQUIRE(sfzMin<float>(1.0f, 0.0f) == 0.0f);
	REQUIRE(sfzMin<float>(0.0f, 1.0f) == 0.0f);

	REQUIRE(sfzMin<float>(1.0f, 2.0f) == 1.0f);
	REQUIRE(sfzMin<float>(2.0f, 1.0f) == 1.0f);
}

TEST_CASE("sfzMax<float>()", "[sfzMax]")
{
	REQUIRE(sfzMax<float>(0.0f, 0.0f) == 0.0f);

	REQUIRE(sfzMax<float>(-1.0f, 0.0f) == 0.0f);
	REQUIRE(sfzMax<float>(0.0f, -1.0f) == 0.0f);

	REQUIRE(sfzMax<float>(-1.0f, -2.0f) == -1.0f);
	REQUIRE(sfzMax<float>(-2.0f, -1.0f) == -1.0f);

	REQUIRE(sfzMax<float>(1.0f, 0.0f) == 1.0f);
	REQUIRE(sfzMax<float>(0.0f, 1.0f) == 1.0f);

	REQUIRE(sfzMax<float>(1.0f, 2.0f) == 2.0f);
	REQUIRE(sfzMax<float>(2.0f, 1.0f) == 2.0f);
}

TEST_CASE("sfzMin<int32_t>()", "[sfzMin]")
{
	REQUIRE(sfzMin<int32_t>(0, 0) == 0);

	REQUIRE(sfzMin<int32_t>(-1, 0) == -1);
	REQUIRE(sfzMin<int32_t>(0, -1) == -1);

	REQUIRE(sfzMin<int32_t>(-1, -2) == -2);
	REQUIRE(sfzMin<int32_t>(-2, -1) == -2);

	REQUIRE(sfzMin<int32_t>(1, 0) == 0);
	REQUIRE(sfzMin<int32_t>(0, 1) == 0);

	REQUIRE(sfzMin<int32_t>(1, 2) == 1);
	REQUIRE(sfzMin<int32_t>(2, 1) == 1);
}

TEST_CASE("sfzMax<int32_t>()", "[sfzMax]")
{
	REQUIRE(sfzMax<int32_t>(0, 0) == 0);

	REQUIRE(sfzMax<int32_t>(-1, 0) == 0);
	REQUIRE(sfzMax<int32_t>(0, -1) == 0);

	REQUIRE(sfzMax<int32_t>(-1, -2) == -1);
	REQUIRE(sfzMax<int32_t>(-2, -1) == -1);

	REQUIRE(sfzMax<int32_t>(1, 0) == 1);
	REQUIRE(sfzMax<int32_t>(0, 1) == 1);

	REQUIRE(sfzMax<int32_t>(1, 2) == 2);
	REQUIRE(sfzMax<int32_t>(2, 1) == 2);
}
