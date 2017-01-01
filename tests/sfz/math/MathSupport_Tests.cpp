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

TEST_CASE("approxEqual()", "[sfz::MathSupport]")
{
	SECTION("float") {
		REQUIRE(approxEqual(2.0f, 2.0f + (APPROX_EQUAL_EPS * 0.95f)));
		REQUIRE(!approxEqual(2.0f, 2.0f + (APPROX_EQUAL_EPS * 1.05f)));
		REQUIRE(approxEqual(2.0f, 2.0f - (APPROX_EQUAL_EPS * 0.95f)));
		REQUIRE(!approxEqual(2.0f, 2.0f - (APPROX_EQUAL_EPS * 1.05f)));
	}
	SECTION("vec2") {
		REQUIRE(approxEqual(vec2(2.0f), vec2(2.0f + (APPROX_EQUAL_EPS * 0.95f))));
		REQUIRE(!approxEqual(vec2(2.0f), vec2(2.0f + (APPROX_EQUAL_EPS * 1.05f))));
		REQUIRE(approxEqual(vec2(2.0f), vec2(2.0f - (APPROX_EQUAL_EPS * 0.95f))));
		REQUIRE(!approxEqual(vec2(2.0f), vec2(2.0f - (APPROX_EQUAL_EPS * 1.05f))));
	}
	SECTION("vec3") {
		REQUIRE(approxEqual(vec3(2.0f), vec3(2.0f + (APPROX_EQUAL_EPS * 0.95f))));
		REQUIRE(!approxEqual(vec3(2.0f), vec3(2.0f + (APPROX_EQUAL_EPS * 1.05f))));
		REQUIRE(approxEqual(vec3(2.0f), vec3(2.0f - (APPROX_EQUAL_EPS * 0.95f))));
		REQUIRE(!approxEqual(vec3(2.0f), vec3(2.0f - (APPROX_EQUAL_EPS * 1.05f))));
	}
	SECTION("vec4") {
		REQUIRE(approxEqual(vec4(2.0f), vec4(2.0f + (APPROX_EQUAL_EPS * 0.95f))));
		REQUIRE(!approxEqual(vec4(2.0f), vec4(2.0f + (APPROX_EQUAL_EPS * 1.05f))));
		REQUIRE(approxEqual(vec4(2.0f), vec4(2.0f - (APPROX_EQUAL_EPS * 0.95f))));
		REQUIRE(!approxEqual(vec4(2.0f), vec4(2.0f - (APPROX_EQUAL_EPS * 1.05f))));
	}
	SECTION("mat44") {
		REQUIRE(approxEqual(mat44::fill(2.0f), mat44::fill(2.0f + (APPROX_EQUAL_EPS * 0.95f))));
		REQUIRE(!approxEqual(mat44::fill(2.0f), mat44::fill(2.0f + (APPROX_EQUAL_EPS * 1.05f))));
		REQUIRE(approxEqual(mat44::fill(2.0f), mat44::fill(2.0f - (APPROX_EQUAL_EPS * 0.95f))));
		REQUIRE(!approxEqual(mat44::fill(2.0f), mat44::fill(2.0f - (APPROX_EQUAL_EPS * 1.05f))));
	}
	SECTION("Quaternion") {
		float base = 2.0;
		float lowOut = base - (APPROX_EQUAL_EPS * 1.05f);
		float lowIn = base - (APPROX_EQUAL_EPS * 0.95f);
		float highIn = base + (APPROX_EQUAL_EPS * 0.95f);
		float highOut = base + (APPROX_EQUAL_EPS * 1.05f);
		REQUIRE(!approxEqual(Quaternion(base, base, base, base), Quaternion(lowOut, lowOut, lowOut, lowOut)));
		REQUIRE(approxEqual(Quaternion(base, base, base, base), Quaternion(lowIn, lowIn, lowIn, lowIn)));
		REQUIRE(approxEqual(Quaternion(base, base, base, base), Quaternion(highIn, highIn, highIn, highIn)));
		REQUIRE(!approxEqual(Quaternion(base, base, base, base), Quaternion(highOut, highOut, highOut, highOut)));
	}
}

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

TEST_CASE("min()", "[sfz::MathSupport]")
{
	SECTION("Scalars") {
		REQUIRE(sfz::min(1.0f, 2.0f) == 1.0f);
		REQUIRE(sfz::min(-1.0f, -2.0f) == -2.0f);
		REQUIRE(sfz::min(1, 2) == 1);
		REQUIRE(sfz::min(-1, -2) == -2);
		REQUIRE(sfz::min(1u, 2u) == 1u);
		REQUIRE(sfz::min(3u, 2u) == 2u);
	}
	SECTION("Vectors") {
		REQUIRE(sfz::min(vec4(1.0f, 2.0f, -3.0f, -4.0f), vec4(2.0f, 1.0f, -5.0f, -2.0f)) == vec4(1.0f, 1.0f, -5.0f, -4.0f));
		REQUIRE(sfz::min(vec4i(1, 2, -3, -4), vec4i(2, 1, -5, -2)) == vec4i(1, 1, -5, -4));
		REQUIRE(sfz::min(vec4u(1u, 2u, 3u, 4u), vec4u(2u, 1u, 5u, 2u)) == vec4u(1u, 1u, 3u, 2u));
	}
	SECTION("Vectors & scalars")
	{
		REQUIRE(sfz::min(vec4(1.0f, 2.0f, -3.0f, -4.0f), -1.0f) == vec4(-1.0f, -1.0f, -3.0f, -4.0f));
		REQUIRE(sfz::min(vec4i(1, 2, -3, -4), -1) == vec4i(-1, -1, -3, -4));
		REQUIRE(sfz::min(vec4u(1u, 2u, 3u, 4u), 2u) == vec4u(1u, 2u, 2u, 2u));
	}
}

TEST_CASE("max()", "[sfz::MathSupport]")
{
	SECTION("Scalars") {
		REQUIRE(sfz::max(1.0f, 2.0f) == 2.0f);
		REQUIRE(sfz::max(-1.0f, -2.0f) == -1.0f);
		REQUIRE(sfz::max(1, 2) == 2);
		REQUIRE(sfz::max(-1, -2) == -1);
		REQUIRE(sfz::max(1u, 2u) == 2u);
		REQUIRE(sfz::max(3u, 2u) == 3u);
	}
	SECTION("Vectors") {
		REQUIRE(sfz::max(vec4(1.0f, 2.0f, -3.0f, -4.0f), vec4(2.0f, 1.0f, -5.0f, -2.0f)) == vec4(2.0f, 2.0f, -3.0f, -2.0f));
		REQUIRE(sfz::max(vec4i(1, 2, -3, -4), vec4i(2, 1, -5, -2)) == vec4i(2, 2, -3, -2));
		REQUIRE(sfz::max(vec4u(1u, 2u, 3u, 4u), vec4u(2u, 1u, 5u, 2u)) == vec4u(2u, 2u, 5u, 4u));
	}
	SECTION("Vectors & scalars")
	{
		REQUIRE(sfz::max(vec4(1.0f, 2.0f, -3.0f, -4.0f), 1.0f) == vec4(1.0f, 2.0f, 1.0f, 1.0f));
		REQUIRE(sfz::max(vec4i(1, 2, -3, -4), 1) == vec4i(1, 2, 1, 1));
		REQUIRE(sfz::max(vec4u(1u, 2u, 3u, 4u), 2u) == vec4u(2u, 2u, 3u, 4u));
	}
}

TEST_CASE("minElement() & maxElement()", "[sfz::MathSupport]")
{
	REQUIRE(sfz::minElement(vec4u(5u, 2u, 3u, 1u)) == 1u);
	REQUIRE(sfz::minElement(vec4i(1, 2, -4, 0)) == -4);
	REQUIRE(sfz::maxElement(vec4u(5u, 2u, 3u, 1u)) == 5u);
	REQUIRE(sfz::maxElement(vec4i(1, 2, -4, 0)) == 2);
}

TEST_CASE("lerp()", "[sfz::MathSupport]")
{
	SECTION("Quaternion specialization") {
		Quaternion q1 = Quaternion::rotation(vec3(1.0f, 1.0f, 1.0f), 0.0f);
		Quaternion q2 = Quaternion::rotation(vec3(1.0f, 1.0f, 1.0f), 90.0f);
		Quaternion q3 = Quaternion::rotation(vec3(1.0f, 1.0f, 1.0f), 45.0f);
		REQUIRE(approxEqual(lerp(q1, q2, 0.5f), q3));
	}
}

TEST_CASE("clamp()", "[sfz::MathSupport]")
{
	REQUIRE(sfz::clamp(vec4i(-2, 0, 2, 4), -1, 2) == vec4i(-1, 0, 2, 2));
	REQUIRE(sfz::clamp(vec4i(-2, 0, 2, 4), vec4i(0, -1, -1, 5), vec4i(1, 1, 1, 6)) == vec4i(0, 0, 1, 5));
}

TEST_CASE("saturate()", "[sfz::MathSupport]")
{
	REQUIRE(sfz::saturate(4.0f) == 1.0f);
	REQUIRE(sfz::saturate(-1.0f) == 0.0f);
	REQUIRE(sfz::saturate(0.2f) == 0.2f);
	REQUIRE(sfz::saturate(vec4(4.0f, -1.0f, 0.2f, 0.4f)) == vec4(1.0f, 0.0f, 0.2f, 0.4f));
}

TEST_CASE("fma()", "[sfz::MathSupport]")
{
	REQUIRE(approxEqual(sfz::fma(2.0f, -2.0f, 6.0f), 2.0f));
	REQUIRE(approxEqual(sfz::fma(vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(-1.0f, -2.0f, 3.0f, 4.0f),
	                    vec4(1.0f, 2.0f, 3.0f, 4.0f)), vec4(0.0f, -2.0f, 12.0f, 20.0f)));
}
