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

#include "sfz/math/Quaternion.hpp"
#include "sfz/math/MathSupport.hpp"

using namespace sfz;

TEST_CASE("Quaternion Constructors", "[sfz::Quaternion]")
{
	SECTION("(x,y,z,w) constructor") {
		Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
		REQUIRE(approxEqual(q, Quaternion(1.0f, 2.0f, 3.0f, 4.0f)));
	}
	SECTION("(v,w) constructor") {
		Quaternion q(vec3(4.0f, 3.0f, 2.0f), 1.0f);
		REQUIRE(approxEqual(q, Quaternion(4.0f, 3.0f, 2.0f, 1.0f)));
	}
	SECTION("identity() constructor function") {
		Quaternion q = Quaternion::identity();
		REQUIRE(approxEqual(q, Quaternion(0.0f, 0.0f, 0.0f, 1.0f)));
		REQUIRE(approxEqual(q * q, Quaternion(0.0f, 0.0f, 0.0f, 1.0f)));
		Quaternion q2(1.0f, 2.0f, 3.0f, 4.0f);
		REQUIRE(approxEqual(q * q2, q2));
		REQUIRE(approxEqual(q2 * q, q2));
	}
	SECTION("rotation() constructor function") {
		float angle = 60.0f;
		float halfAngleRad = (angle * DEG_TO_RAD) / 2.0f;
		vec3 axis = normalize(vec3(0.25f, 1.0f, 1.2f));
		Quaternion rot1(std::sin(halfAngleRad) * axis, std::cos(halfAngleRad));
		Quaternion rot2 = Quaternion::rotation(axis, angle);
		REQUIRE(approxEqual(rot1, rot2));
		REQUIRE(approxEqual(rot2.rotationAxis(), normalize(vec3(0.25f, 1.0f, 1.2f))));
		REQUIRE(approxEqual(rot2.rotationAngleDeg(), angle));
	}
	SECTION("fromEuler() constructor function") {
		REQUIRE(approxEqual(Quaternion::fromEuler(0.0f, 0.0f, 0.0f), Quaternion::identity()));
		REQUIRE(approxEqual(Quaternion::fromEuler(90.0f, 0.0f, 0.0f), Quaternion::rotation(vec3(1.0f, 0.0f, 0.0f), 90.0f)));
		REQUIRE(approxEqual(Quaternion::fromEuler(0.0f, 90.0f, 0.0f), Quaternion::rotation(vec3(0.0f, 1.0f, 0.0f), 90.0f)));
		REQUIRE(approxEqual(Quaternion::fromEuler(0.0f, 0.0f, 90.0f), Quaternion::rotation(vec3(0.0f, 0.0f, 1.0f), 90.0f)));
		vec3 angles(20.0f, 30.0f, 40.0f);
		REQUIRE(approxEqual(Quaternion::fromEuler(angles).toEuler(), angles));
	}
}

TEST_CASE("Quaternion Operators", "[sfz::Quaternion]")
{
	Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

	SECTION("Equality operators") {
		REQUIRE(q1 == Quaternion(1.0f, 2.0f, 3.0f, 4.0f));
		REQUIRE(q2 == Quaternion(-1.0f, 3.0f, 1.0f, 6.0f));
		REQUIRE(q1 != q2);
	}
	SECTION("+ operator") {
		Quaternion r1 = q1 + q2;
		REQUIRE(approxEqual(r1, Quaternion(0.0f, 5.0f, 4.0f, 10.0f)));
	}
	SECTION("- operator") {
		Quaternion r1 = q1 - q2;
		REQUIRE(approxEqual(r1, Quaternion(2.0f, -1.0f, 2.0f, -2.0f)));
	}
	SECTION("* operator (Quaternion)") {
		Quaternion l1(1.0f, 2.0f, 3.0f, 4.0f);
		Quaternion r1(5.0f, 6.0f, 7.0f, 8.0f);
		REQUIRE(approxEqual(l1 * r1, Quaternion(24.0f, 48.0f, 48.0f, -6.0f)));
		REQUIRE(approxEqual(r1 * l1, Quaternion(32.0f, 32.0f, 56.0f, -6.0f)));

		Quaternion l2(-1.0f, -4.0f, -2.0f, 6.0f);
		Quaternion r2(-2.0f, 2.0f, -5.0f, 1.0f);
		REQUIRE(approxEqual(l2 * r2, Quaternion(11.0f, 7.0f, -42.0f, 2.0f)));
		REQUIRE(approxEqual(r2 * l2, Quaternion(-37.0f, 9.0f, -22.0f, 2.0f)));
	}
	SECTION("* operator (scalar)") {
		REQUIRE(approxEqual(2.0f * q1, Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
		REQUIRE(approxEqual(q1 * 2.0f, Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
	}
}

TEST_CASE("Quaternion functions", "[sfz::Quaternion]")
{
	SECTION("length()") {
		REQUIRE(approxEqual(length(Quaternion::identity()), 1.0f));
	}
	SECTION("conjugate()") {
		Quaternion q = conjugate(Quaternion(1.0f, 2.0f, 3.0f, 4.0f));
		REQUIRE(approxEqual(q, Quaternion(-1.0f, -2.0f, -3.0f, 4.0f)));
	}
	SECTION("inverse()") {
		Quaternion q = inverse(Quaternion(1.0f, 2.0f, 3.0f, 4.0f));
		REQUIRE(approxEqual(q, Quaternion(-1.0f / 30.0f, -1.0f / 15.0f, -1.0f / 10.0f, 2.0f / 15.0f)));
	}
	SECTION("rotate()") {
		float halfAngle1 = (90.0f * DEG_TO_RAD) / 2.0f;
		Quaternion rot1(std::sin(halfAngle1) * vec3(0.0f, 1.0f, 0.0f), std::cos(halfAngle1));
		vec3 p = rotate(rot1, vec3(1.0f, 0.0f, 0.0f));
		REQUIRE(approxEqual(p, vec3(0.0f, 0.0f, -1.0f)));
		mat33 rot1mat = rot1.toMat33();
		REQUIRE(approxEqual(rot1mat * vec3(1.0f, 0.0f, 0.0f), vec3 (0.0f, 0.0f, -1.0f)));

		Quaternion rot2 = Quaternion::rotation(vec3(0.0f, 0.0f, 1.0f), 90.0f);
		vec3 p2 = rotate(rot2, vec3(1.0f, 0.0f, 0.0f));
		REQUIRE(approxEqual(p2, vec3(0.0f, 1.0f, 0.0f)));
		mat33 rot2mat = rot2.toMat33();
		REQUIRE(approxEqual(rot2mat * vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));
	}
}
