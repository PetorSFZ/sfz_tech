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

#include <skipifzero_math.hpp>

using namespace sfz;

static bool eqf(quat q1, quat q2, f32 eps = EQF_EPS) noexcept
{
	return sfz::eqf(q1.vector, q2.vector, eps);
}

UTEST(Quaternion, constructors)
{
	// (x,y,z,w) constructor
	{
		quat q(1.0f, 2.0f, 3.0f, 4.0f);
		ASSERT_TRUE(eqf(q, quat(1.0f, 2.0f, 3.0f, 4.0f)));
	}
	// (v,w) constructor
	{
		quat q(f32x3(4.0f, 3.0f, 2.0f), 1.0f);
		ASSERT_TRUE(eqf(q, quat(4.0f, 3.0f, 2.0f, 1.0f)));
	}
	// identity() constructor function
	{
		quat q = quat::identity();
		ASSERT_TRUE(eqf(q, quat(0.0f, 0.0f, 0.0f, 1.0f)));
		ASSERT_TRUE(eqf(q * q, quat(0.0f, 0.0f, 0.0f, 1.0f)));
		quat q2(1.0f, 2.0f, 3.0f, 4.0f);
		ASSERT_TRUE(eqf(q * q2, q2));
		ASSERT_TRUE(eqf(q2 * q, q2));
	}
	// rotation() constructor function
	{
		f32 angle = 60.0f;
		f32 halfAngleRad = (angle * DEG_TO_RAD) / 2.0f;
		f32x3 axis = normalize(f32x3(0.25f, 1.0f, 1.2f));
		quat rot1(::sinf(halfAngleRad) * axis, ::cosf(halfAngleRad));
		quat rot2 = quat::rotationDeg(axis, angle);
		ASSERT_TRUE(eqf(rot1, rot2));
		ASSERT_TRUE(eqf(rot2.rotationAxis(), normalize(f32x3(0.25f, 1.0f, 1.2f))));
		ASSERT_TRUE(eqf(rot2.rotationAngleDeg(), angle));
	}
	// fromEuler() constructor function
	{
		ASSERT_TRUE(eqf(quat::fromEuler(0.0f, 0.0f, 0.0f), quat::identity()));
		ASSERT_TRUE(eqf(quat::fromEuler(90.0f, 0.0f, 0.0f), quat::rotationDeg(f32x3(1.0f, 0.0f, 0.0f), 90.0f)));
		ASSERT_TRUE(eqf(quat::fromEuler(0.0f, 90.0f, 0.0f), quat::rotationDeg(f32x3(0.0f, 1.0f, 0.0f), 90.0f)));
		ASSERT_TRUE(eqf(quat::fromEuler(0.0f, 0.0f, 90.0f), quat::rotationDeg(f32x3(0.0f, 0.0f, 1.0f), 90.0f)));
		f32x3 angles(20.0f, 30.0f, 40.0f);
		ASSERT_TRUE(eqf(quat::fromEuler(angles).toEuler(), angles));
	}
	// fromRotationMatrix() constructor function
	{
		f32 angleDeg1 = 60.0f;
		f32 angleRad1 = angleDeg1 * DEG_TO_RAD;
		f32x3 axis = normalize(f32x3(0.25f, 1.0f, 1.2f));

		quat rotQuat1 = quat::rotationDeg(axis, angleDeg1);
		mat34 rotMat1 = mat34::rotation3(axis, angleRad1);
		quat rotQuat2 = quat::fromRotationMatrix(rotMat1);
		ASSERT_TRUE(eqf(rotQuat1, rotQuat2));

		f32 angleDeg2 = 190.0f;
		f32 angleRad2 = angleDeg2 * DEG_TO_RAD;

		quat rotQuat3 = quat::rotationDeg(axis, angleDeg2);
		mat34 rotMat2 = mat34::rotation3(axis, angleRad2);
		quat rotQuat4 = quat::fromRotationMatrix(rotMat2);
		ASSERT_TRUE(eqf(rotQuat3, rotQuat4, 0.04f));
	}
}

UTEST(Quaternion, operators)
{
	// Equality operators
	{
		quat q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		ASSERT_TRUE(q1 == quat(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(q2 == quat(-1.0f, 3.0f, 1.0f, 6.0f));
		ASSERT_TRUE(q1 != q2);
	}
	// + operator
	{
		quat q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		quat r1 = q1 + q2;
		ASSERT_TRUE(eqf(r1, quat(0.0f, 5.0f, 4.0f, 10.0f)));
	}
	// - operator
	{
		quat q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		quat r1 = q1 - q2;
		ASSERT_TRUE(eqf(r1, quat(2.0f, -1.0f, 2.0f, -2.0f)));
	}
	// * operator (Quaternion)
	{
		quat q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		quat l1(1.0f, 2.0f, 3.0f, 4.0f);
		quat r1(5.0f, 6.0f, 7.0f, 8.0f);
		ASSERT_TRUE(eqf(l1 * r1, quat(24.0f, 48.0f, 48.0f, -6.0f)));
		ASSERT_TRUE(eqf(r1 * l1, quat(32.0f, 32.0f, 56.0f, -6.0f)));

		quat l2(-1.0f, -4.0f, -2.0f, 6.0f);
		quat r2(-2.0f, 2.0f, -5.0f, 1.0f);
		ASSERT_TRUE(eqf(l2 * r2, quat(11.0f, 7.0f, -42.0f, 2.0f)));
		ASSERT_TRUE(eqf(r2 * l2, quat(-37.0f, 9.0f, -22.0f, 2.0f)));
	}
	// * operator (scalar)
	{
		quat q1(1.0f, 2.0f, 3.0f, 4.0f), q2(-1.0f, 3.0f, 1.0f, 6.0f);

		ASSERT_TRUE(eqf(2.0f * q1, quat(2.0f, 4.0f, 6.0f, 8.0f)));
		ASSERT_TRUE(eqf(q1 * 2.0f, quat(2.0f, 4.0f, 6.0f, 8.0f)));
	}
}

UTEST(Quaternion, quaternion_functions)
{
	// length()
	{
		ASSERT_TRUE(eqf(length(quat::identity()), 1.0f));
	}
	// conjugate()
	{
		quat q = conjugate(quat(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(eqf(q, quat(-1.0f, -2.0f, -3.0f, 4.0f)));
	}
	// inverse()
	{
		quat q = inverse(quat(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(eqf(q, quat(-1.0f / 30.0f, -1.0f / 15.0f, -1.0f / 10.0f, 2.0f / 15.0f)));
	}
	// rotate()
	{
		f32 halfAngle1 = (90.0f * DEG_TO_RAD) / 2.0f;
		quat rot1(::sinf(halfAngle1) * f32x3(0.0f, 1.0f, 0.0f), ::cosf(halfAngle1));
		f32x3 p = rotate(rot1, f32x3(1.0f, 0.0f, 0.0f));
		ASSERT_TRUE(eqf(p, f32x3(0.0f, 0.0f, -1.0f)));
		mat33 rot1mat = rot1.toMat33();
		ASSERT_TRUE(eqf(rot1mat * f32x3(1.0f, 0.0f, 0.0f), f32x3 (0.0f, 0.0f, -1.0f)));

		quat rot2 = quat::rotationDeg(f32x3(0.0f, 0.0f, 1.0f), 90.0f);
		f32x3 p2 = rotate(rot2, f32x3(1.0f, 0.0f, 0.0f));
		ASSERT_TRUE(eqf(p2, f32x3(0.0f, 1.0f, 0.0f)));
		mat33 rot2mat = rot2.toMat33();
		ASSERT_TRUE(eqf(rot2mat * f32x3(1.0f, 0.0f, 0.0f), f32x3(0.0f, 1.0f, 0.0f)));
	}
}

UTEST(Quaternion, lerp)
{
	quat q1 = quat::rotationDeg(f32x3(1.0f, 1.0f, 1.0f), 0.0f);
	quat q2 = quat::rotationDeg(f32x3(1.0f, 1.0f, 1.0f), 90.0f);
	quat q3 = quat::rotationDeg(f32x3(1.0f, 1.0f, 1.0f), 45.0f);
	ASSERT_TRUE(eqf(lerp(q1, q2, 0.5f).vector, q3.vector));
}
