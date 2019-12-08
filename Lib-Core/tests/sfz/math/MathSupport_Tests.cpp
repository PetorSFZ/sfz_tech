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

#include "sfz/math/MathSupport.hpp"

using namespace sfz;

TEST_CASE("equalsApprox()", "[sfz::MathSupport]")
{
	SECTION("float") {
		REQUIRE(equalsApprox(2.0f, 2.0f + (EQUALS_APPROX_EPS * 0.95f)));
		REQUIRE(!equalsApprox(2.0f, 2.0f + (EQUALS_APPROX_EPS * 1.05f)));
		REQUIRE(equalsApprox(2.0f, 2.0f - (EQUALS_APPROX_EPS * 0.95f)));
		REQUIRE(!equalsApprox(2.0f, 2.0f - (EQUALS_APPROX_EPS * 1.05f)));
	}
	SECTION("vec2") {
		REQUIRE(equalsApprox(vec2(2.0f), vec2(2.0f + (EQUALS_APPROX_EPS * 0.95f))));
		REQUIRE(!equalsApprox(vec2(2.0f), vec2(2.0f + (EQUALS_APPROX_EPS * 1.05f))));
		REQUIRE(equalsApprox(vec2(2.0f), vec2(2.0f - (EQUALS_APPROX_EPS * 0.95f))));
		REQUIRE(!equalsApprox(vec2(2.0f), vec2(2.0f - (EQUALS_APPROX_EPS * 1.05f))));
	}
	SECTION("vec3") {
		REQUIRE(equalsApprox(vec3(2.0f), vec3(2.0f + (EQUALS_APPROX_EPS * 0.95f))));
		REQUIRE(!equalsApprox(vec3(2.0f), vec3(2.0f + (EQUALS_APPROX_EPS * 1.05f))));
		REQUIRE(equalsApprox(vec3(2.0f), vec3(2.0f - (EQUALS_APPROX_EPS * 0.95f))));
		REQUIRE(!equalsApprox(vec3(2.0f), vec3(2.0f - (EQUALS_APPROX_EPS * 1.05f))));
	}
	SECTION("vec4") {
		REQUIRE(equalsApprox(vec4(2.0f), vec4(2.0f + (EQUALS_APPROX_EPS * 0.95f))));
		REQUIRE(!equalsApprox(vec4(2.0f), vec4(2.0f + (EQUALS_APPROX_EPS * 1.05f))));
		REQUIRE(equalsApprox(vec4(2.0f), vec4(2.0f - (EQUALS_APPROX_EPS * 0.95f))));
		REQUIRE(!equalsApprox(vec4(2.0f), vec4(2.0f - (EQUALS_APPROX_EPS * 1.05f))));
	}
}

TEST_CASE("lerp()", "[sfz::MathSupport]")
{
	SECTION("Quaternion specialization") {
		Quaternion q1 = Quaternion::rotationDeg(vec3(1.0f, 1.0f, 1.0f), 0.0f);
		Quaternion q2 = Quaternion::rotationDeg(vec3(1.0f, 1.0f, 1.0f), 90.0f);
		Quaternion q3 = Quaternion::rotationDeg(vec3(1.0f, 1.0f, 1.0f), 45.0f);
		REQUIRE(equalsApprox(lerp(q1, q2, 0.5f).vector, q3.vector));
	}
}

TEST_CASE("clamp()", "[sfz::MathSupport]")
{
	REQUIRE(sfz::clamp(vec4_i32(-2, 0, 2, 4), -1, 2) == vec4_i32(-1, 0, 2, 2));
	REQUIRE(sfz::clamp(vec4_i32(-2, 0, 2, 4), vec4_i32(0, -1, -1, 5), vec4_i32(1, 1, 1, 6)) == vec4_i32(0, 0, 1, 5));
}

TEST_CASE("saturate()", "[sfz::MathSupport]")
{
	REQUIRE(sfz::saturate(4.0f) == 1.0f);
	REQUIRE(sfz::saturate(-1.0f) == 0.0f);
	REQUIRE(sfz::saturate(0.2f) == 0.2f);
	REQUIRE(sfz::saturate(vec4(4.0f, -1.0f, 0.2f, 0.4f)) == vec4(1.0f, 0.0f, 0.2f, 0.4f));
}

TEST_CASE("rotateTowards()", "[sfz::MathSupport]")
{
	const vec3 LEFT = vec3(-1.0f, 0.0f, 0.0f);
	const vec3 UP = vec3(0.0f, 1.0f, 0.0f);
	const vec3 LEFT_UP = normalize(vec3(-1.0f, 1.0f, 0.0f));

	SECTION("rotateTowardsDeg()") {
		vec3 newVec1 = rotateTowardsDeg(LEFT, UP, 45.0f);
		REQUIRE(equalsApprox(newVec1, LEFT_UP));
		vec3 newVec2 = rotateTowardsDeg(UP, LEFT, 45.0f);
		REQUIRE(equalsApprox(newVec2, LEFT_UP));
		vec3 newVec3 = rotateTowardsDeg(LEFT, UP, 90.0f);
		REQUIRE(equalsApprox(newVec3, UP));
		vec3 newVec4 = rotateTowardsDeg(UP, LEFT, 90.0f);
		REQUIRE(equalsApprox(newVec4, LEFT));
		vec3 newVec5 = rotateTowardsDeg(LEFT, UP, 0.0f);
		REQUIRE(equalsApprox(newVec5, LEFT));
		vec3 newVec6 = rotateTowardsDeg(UP, LEFT, 0.0f);
		REQUIRE(equalsApprox(newVec6, UP));
	}
	SECTION("rotateTowardsDegClampSafe()") {
		vec3 newVec1 = rotateTowardsDegClampSafe(LEFT, UP, 45.0f);
		REQUIRE(equalsApprox(newVec1, LEFT_UP));
		vec3 newVec2 = rotateTowardsDegClampSafe(UP, LEFT, 45.0f);
		REQUIRE(equalsApprox(newVec2, LEFT_UP));
		vec3 newVec3 = rotateTowardsDegClampSafe(LEFT, UP, 90.0f);
		REQUIRE(equalsApprox(newVec3, UP));
		vec3 newVec4 = rotateTowardsDegClampSafe(UP, LEFT, 90.0f);
		REQUIRE(equalsApprox(newVec4, LEFT));
		vec3 newVec5 = rotateTowardsDegClampSafe(LEFT, UP, 0.0f);
		REQUIRE(equalsApprox(newVec5, LEFT));
		vec3 newVec6 = rotateTowardsDegClampSafe(UP, LEFT, 0.0f);
		REQUIRE(equalsApprox(newVec6, UP));
		vec3 newVec7 = rotateTowardsDegClampSafe(LEFT, UP, 100.0f);
		REQUIRE(equalsApprox(newVec7, UP));
		vec3 newVec8 = rotateTowardsDegClampSafe(UP, LEFT, 100.0f);
		REQUIRE(equalsApprox(newVec8, LEFT));
	}
}
