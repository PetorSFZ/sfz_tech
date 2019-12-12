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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "utest.h"
#undef near
#undef far

#include "sfz/math/Quaternion.hpp"
#include "sfz/math/MathSupport.hpp"

using namespace sfz;

static bool eqf(Quaternion q1, Quaternion q2, float eps = EQF_EPS) noexcept
{
	return sfz::eqf(q1.vector, q2.vector, eps);
}

UTEST(Quaternion, constructors)
{
	// (x,y,z,w) constructor
	{
		Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
		ASSERT_TRUE(eqf(q, Quaternion(1.0f, 2.0f, 3.0f, 4.0f)));
	}
	// (v,w) constructor
	{
		Quaternion q(vec3(4.0f, 3.0f, 2.0f), 1.0f);
		ASSERT_TRUE(eqf(q, Quaternion(4.0f, 3.0f, 2.0f, 1.0f)));
	}
	// identity() constructor function
	{
		Quaternion q = Quaternion::identity();
		ASSERT_TRUE(eqf(q, Quaternion(0.0f, 0.0f, 0.0f, 1.0f)));
		ASSERT_TRUE(eqf(q * q, Quaternion(0.0f, 0.0f, 0.0f, 1.0f)));
		Quaternion q2(1.0f, 2.0f, 3.0f, 4.0f);
		ASSERT_TRUE(eqf(q * q2, q2));
		ASSERT_TRUE(eqf(q2 * q, q2));
	}
	// rotation() constructor function
	{
		float angle = 60.0f;
		float halfAngleRad = (angle * DEG_TO_RAD) / 2.0f;
		vec3 axis = normalize(vec3(0.25f, 1.0f, 1.2f));
		Quaternion rot1(std::sin(halfAngleRad) * axis, std::cos(halfAngleRad));
		Quaternion rot2 = Quaternion::rotationDeg(axis, angle);
		ASSERT_TRUE(eqf(rot1, rot2));
		ASSERT_TRUE(eqf(rot2.rotationAxis(), normalize(vec3(0.25f, 1.0f, 1.2f))));
		ASSERT_TRUE(eqf(rot2.rotationAngleDeg(), angle));
	}
	// fromEuler() constructor function
	{
		ASSERT_TRUE(eqf(Quaternion::fromEuler(0.0f, 0.0f, 0.0f), Quaternion::identity()));
		ASSERT_TRUE(eqf(Quaternion::fromEuler(90.0f, 0.0f, 0.0f), Quaternion::rotationDeg(vec3(1.0f, 0.0f, 0.0f), 90.0f)));
		ASSERT_TRUE(eqf(Quaternion::fromEuler(0.0f, 90.0f, 0.0f), Quaternion::rotationDeg(vec3(0.0f, 1.0f, 0.0f), 90.0f)));
		ASSERT_TRUE(eqf(Quaternion::fromEuler(0.0f, 0.0f, 90.0f), Quaternion::rotationDeg(vec3(0.0f, 0.0f, 1.0f), 90.0f)));
		vec3 angles(20.0f, 30.0f, 40.0f);
		ASSERT_TRUE(eqf(Quaternion::fromEuler(angles).toEuler(), angles));
	}
	// fromRotationMatrix() constructor function
	{
		float angleDeg1 = 60.0f;
		float angleRad1 = angleDeg1 * DEG_TO_RAD;
		vec3 axis = normalize(vec3(0.25f, 1.0f, 1.2f));

		Quaternion rotQuat1 = Quaternion::rotationDeg(axis, angleDeg1);
		mat34 rotMat1 = mat34::rotation3(axis, angleRad1);
		Quaternion rotQuat2 = Quaternion::fromRotationMatrix(rotMat1);
		ASSERT_TRUE(eqf(rotQuat1, rotQuat2));

		float angleDeg2 = 190.0f;
		float angleRad2 = angleDeg2 * DEG_TO_RAD;

		Quaternion rotQuat3 = Quaternion::rotationDeg(axis, angleDeg2);
		mat34 rotMat2 = mat34::rotation3(axis, angleRad2);
		Quaternion rotQuat4 = Quaternion::fromRotationMatrix(rotMat2);
		ASSERT_TRUE(eqf(rotQuat3, rotQuat4, 0.04f));
	}
}

UTEST(Quaternion, operators)
{
	// Equality operators
	{
		Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		ASSERT_TRUE(q1 == Quaternion(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(q2 == Quaternion(-1.0f, 3.0f, 1.0f, 6.0f));
		ASSERT_TRUE(q1 != q2);
	}
	// + operator
	{
		Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		Quaternion r1 = q1 + q2;
		ASSERT_TRUE(eqf(r1, Quaternion(0.0f, 5.0f, 4.0f, 10.0f)));
	}
	// - operator
	{
		Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		Quaternion r1 = q1 - q2;
		ASSERT_TRUE(eqf(r1, Quaternion(2.0f, -1.0f, 2.0f, -2.0f)));
	}
	// * operator (Quaternion)
	{
		Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		Quaternion l1(1.0f, 2.0f, 3.0f, 4.0f);
		Quaternion r1(5.0f, 6.0f, 7.0f, 8.0f);
		ASSERT_TRUE(eqf(l1 * r1, Quaternion(24.0f, 48.0f, 48.0f, -6.0f)));
		ASSERT_TRUE(eqf(r1 * l1, Quaternion(32.0f, 32.0f, 56.0f, -6.0f)));

		Quaternion l2(-1.0f, -4.0f, -2.0f, 6.0f);
		Quaternion r2(-2.0f, 2.0f, -5.0f, 1.0f);
		ASSERT_TRUE(eqf(l2 * r2, Quaternion(11.0f, 7.0f, -42.0f, 2.0f)));
		ASSERT_TRUE(eqf(r2 * l2, Quaternion(-37.0f, 9.0f, -22.0f, 2.0f)));
	}
	// * operator (scalar)
	{
		Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		ASSERT_TRUE(eqf(2.0f * q1, Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
		ASSERT_TRUE(eqf(q1 * 2.0f, Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
	}
}

UTEST(Quaternion, quaternion_functions)
{
	// length()
	{
		ASSERT_TRUE(eqf(length(Quaternion::identity()), 1.0f));
	}
	// conjugate()
	{
		Quaternion q = conjugate(Quaternion(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(eqf(q, Quaternion(-1.0f, -2.0f, -3.0f, 4.0f)));
	}
	// inverse()
	{
		Quaternion q = inverse(Quaternion(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(eqf(q, Quaternion(-1.0f / 30.0f, -1.0f / 15.0f, -1.0f / 10.0f, 2.0f / 15.0f)));
	}
	// rotate()
	{
		float halfAngle1 = (90.0f * DEG_TO_RAD) / 2.0f;
		Quaternion rot1(std::sin(halfAngle1) * vec3(0.0f, 1.0f, 0.0f), std::cos(halfAngle1));
		vec3 p = rotate(rot1, vec3(1.0f, 0.0f, 0.0f));
		ASSERT_TRUE(eqf(p, vec3(0.0f, 0.0f, -1.0f)));
		mat33 rot1mat = rot1.toMat33();
		ASSERT_TRUE(eqf(rot1mat * vec3(1.0f, 0.0f, 0.0f), vec3 (0.0f, 0.0f, -1.0f)));

		Quaternion rot2 = Quaternion::rotationDeg(vec3(0.0f, 0.0f, 1.0f), 90.0f);
		vec3 p2 = rotate(rot2, vec3(1.0f, 0.0f, 0.0f));
		ASSERT_TRUE(eqf(p2, vec3(0.0f, 1.0f, 0.0f)));
		mat33 rot2mat = rot2.toMat33();
		ASSERT_TRUE(eqf(rot2mat * vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));
	}
}

UTEST(Quaternion, lerp)
{
	Quaternion q1 = Quaternion::rotationDeg(vec3(1.0f, 1.0f, 1.0f), 0.0f);
	Quaternion q2 = Quaternion::rotationDeg(vec3(1.0f, 1.0f, 1.0f), 90.0f);
	Quaternion q3 = Quaternion::rotationDeg(vec3(1.0f, 1.0f, 1.0f), 45.0f);
	ASSERT_TRUE(eqf(lerp(q1, q2, 0.5f).vector, q3.vector));
}
