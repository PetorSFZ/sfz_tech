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

#include "sfz/math/MathSupport.hpp"

using namespace sfz;

TEST_CASE("abs()", "[sfz::MathSupport]")
{
	SECTION("Integer variant") {
		vec4i v = vec4i(-1, 4, -4, -9);
		REQUIRE(sfz::abs(v) == vec4i(1, 4, 4, 9));
	}
	SECTION("Float variant") {
		vec4 v = vec4(-2.0f, 1.0f, -59.0f, -2.0f);
		REQUIRE(sfz::abs(v) == vec4(2.0f, 1.0f, 59.0f, 2.0f));
	}
}

TEST_CASE("sgn()", "[sfz::MathSupport]")
{
	SECTION("Integer scalar") {
		REQUIRE(sgn(-4) == -1);
		REQUIRE(sgn(-1) == -1);
		REQUIRE(sgn(0) == 0);
		REQUIRE(sgn(1) == 1);
		REQUIRE(sgn(5) == 1);
	}
	SECTION("Integer vector") {
		REQUIRE(sgn(vec4i(-4, 0, -2, 4)) == vec4i(-1, 0, -1, 1));
	}
	SECTION("Floating point scalar") {
		REQUIRE(sgn(-4.0f) == -1.0f);
		REQUIRE(sgn(-1.0f) == -1.0f);
		REQUIRE(sgn(1.0f) == 1.0f);
		REQUIRE(sgn(5.0f) == 1.0f);
		// Specifically don't test +0.0f and -0.0f, causes problem when compiling with fast-math.
	}
	SECTION("Floating point vector") {
		REQUIRE(sgn(vec4(-4.0f, -1.0f, 2.0f, 5.0f)) == vec4(-1.0f, -1.0f, 1.0f, 1.0f));
	}
}
