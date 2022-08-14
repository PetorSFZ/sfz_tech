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

#include <doctest.h>

#include <sfz.h>
#include <sfz_math.h>
#include <sfz_matrix.h>
#include <sfz_quat.h>

using namespace sfz;

static bool eqf(SfzQuat q1, SfzQuat q2, f32 eps = EQF_EPS)
{
	return sfz::eqf(q1.v, q2.v, eps) && sfz::eqf(q1.w, q2.w, eps);
}

TEST_CASE("Quaternion: constructors")
{
	// (v,w) constructor
	{
		SfzQuat q = sfzQuatInit(f32x3_init(4.0f, 3.0f, 2.0f), 1.0f);
		CHECK(eqf(q.v, f32x3_init(4.0f, 3.0f, 2.0f)));
		CHECK(eqf(q.w, 1.0f));
	}
	// identity() constructor function
	{
		SfzQuat q = sfzQuatIdentity();
		CHECK(eqf(q, sfzQuatInit(f32x3_init(0.0f, 0.0f, 0.0f), 1.0f)));
		CHECK(eqf(q * q, sfzQuatInit(f32x3_init(0.0f, 0.0f, 0.0f), 1.0f)));
		SfzQuat q2 = sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f);
		CHECK(eqf(q * q2, q2));
		CHECK(eqf(q2 * q, q2));
	}
	// rotation() constructor function
	{
		f32 angle = 60.0f;
		f32 halfAngleRad = (angle * SFZ_DEG_TO_RAD) / 2.0f;
		f32x3 axis = f32x3_normalize(f32x3_init(0.25f, 1.0f, 1.2f));
		SfzQuat rot1 = sfzQuatInit(sfz_sin(halfAngleRad) * axis, sfz_cos(halfAngleRad));
		SfzQuat rot2 = sfzQuatRotationDeg(axis, angle);
		CHECK(eqf(rot1, rot2));
		CHECK(eqf(sfzQuatRotationAxis(rot2), f32x3_normalize(f32x3_init(0.25f, 1.0f, 1.2f))));
		CHECK(eqf(sfzQuatRotationAngleDeg(rot2), angle));
	}
	// fromEuler() constructor function
	{
		CHECK(eqf(sfzQuatFromEuler(f32x3_init(0.0f, 0.0f, 0.0f)), sfzQuatIdentity()));
		CHECK(eqf(sfzQuatFromEuler(f32x3_init(90.0f, 0.0f, 0.0f)), sfzQuatRotationDeg(f32x3_init(1.0f, 0.0f, 0.0f), 90.0f)));
		CHECK(eqf(sfzQuatFromEuler(f32x3_init(0.0f, 90.0f, 0.0f)), sfzQuatRotationDeg(f32x3_init(0.0f, 1.0f, 0.0f), 90.0f)));
		CHECK(eqf(sfzQuatFromEuler(f32x3_init(0.0f, 0.0f, 90.0f)), sfzQuatRotationDeg(f32x3_init(0.0f, 0.0f, 1.0f), 90.0f)));
		f32x3 angles = f32x3_init(20.0f, 30.0f, 40.0f);
		CHECK(eqf(sfzQuatToEuler(sfzQuatFromEuler(angles)), angles));
	}
	// fromRotationMatrix() constructor function
	{
		f32 angleDeg1 = 60.0f;
		f32 angleRad1 = angleDeg1 * SFZ_DEG_TO_RAD;
		f32x3 axis = f32x3_normalize(f32x3_init(0.25f, 1.0f, 1.2f));

		SfzQuat rotQuat1 = sfzQuatRotationDeg(axis, angleDeg1);
		SfzMat33 rotMat1 = sfzMat33Rotation3(axis, angleRad1);
		SfzQuat rotQuat2 = sfzQuatFromRotationMatrix(rotMat1);
		CHECK(eqf(rotQuat1, rotQuat2));

		f32 angleDeg2 = 190.0f;
		f32 angleRad2 = angleDeg2 * SFZ_DEG_TO_RAD;

		SfzQuat rotQuat3 = sfzQuatRotationDeg(axis, angleDeg2);
		SfzMat33 rotMat2 = sfzMat33Rotation3(axis, angleRad2);
		SfzQuat rotQuat4 = sfzQuatFromRotationMatrix(rotMat2);
		CHECK(eqf(rotQuat3, rotQuat4, 0.04f));
	}
}

TEST_CASE("Quaternion: operators")
{
	// Equality operators
	{
		SfzQuat q1 = sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f);
		SfzQuat q2 = sfzQuatInit(f32x3_init(-1.0f, 3.0f, 1.0f), 6.0f);

		CHECK(eqf(q1, sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f)));
		CHECK(eqf(q2, sfzQuatInit(f32x3_init(-1.0f, 3.0f, 1.0f), 6.0f)));
		CHECK(!eqf(q1, q2));
	}
	// + operator
	{
		SfzQuat q1 = sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f);
		SfzQuat q2 = sfzQuatInit(f32x3_init(-1.0f, 3.0f, 1.0f), 6.0f);

		SfzQuat r1 = q1 + q2;
		CHECK(eqf(r1, sfzQuatInit(f32x3_init(0.0f, 5.0f, 4.0f), 10.0f)));
	}
	// - operator
	{
		SfzQuat q1 = sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f);
		SfzQuat q2 = sfzQuatInit(f32x3_init(-1.0f, 3.0f, 1.0f), 6.0f);

		SfzQuat r1 = q1 - q2;
		CHECK(eqf(r1, sfzQuatInit(f32x3_init(2.0f, -1.0f, 2.0f), -2.0f)));
	}
	// * operator (Quaternion)
	{
		SfzQuat q1 = sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f);
		SfzQuat q2 = sfzQuatInit(f32x3_init(-1.0f, 3.0f, 1.0f), 6.0f);

		SfzQuat l1 = sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f);
		SfzQuat r1 = sfzQuatInit(f32x3_init(5.0f, 6.0f, 7.0f), 8.0f);
		CHECK(eqf(l1 * r1, sfzQuatInit(f32x3_init(24.0f, 48.0f, 48.0f), -6.0f)));
		CHECK(eqf(r1 * l1, sfzQuatInit(f32x3_init(32.0f, 32.0f, 56.0f), -6.0f)));

		SfzQuat l2 = sfzQuatInit(f32x3_init(-1.0f, -4.0f, -2.0f), 6.0f);
		SfzQuat r2 = sfzQuatInit(f32x3_init(-2.0f, 2.0f, -5.0f), 1.0f);
		CHECK(eqf(l2 * r2, sfzQuatInit(f32x3_init(11.0f, 7.0f, -42.0f), 2.0f)));
		CHECK(eqf(r2 * l2, sfzQuatInit(f32x3_init(-37.0f, 9.0f, -22.0f), 2.0f)));
	}
	// * operator (scalar)
	{
		SfzQuat q1 = sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f);
		SfzQuat q2 = sfzQuatInit(f32x3_init(-1.0f, 3.0f, 1.0f), 6.0f);

		CHECK(eqf(2.0f * q1, sfzQuatInit(f32x3_init(2.0f, 4.0f, 6.0f), 8.0f)));
		CHECK(eqf(q1 * 2.0f, sfzQuatInit(f32x3_init(2.0f, 4.0f, 6.0f), 8.0f)));
	}
}

TEST_CASE("Quaternion: Quaternion_functions")
{
	// length()
	{
		CHECK(eqf(sfzQuatLength(sfzQuatIdentity()), 1.0f));
	}
	// conjugate()
	{
		SfzQuat q = sfzQuatConjugate(sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f));
		CHECK(eqf(q, sfzQuatInit(f32x3_init(-1.0f, -2.0f, -3.0f), 4.0f)));
	}
	// inverse()
	{
		SfzQuat q = sfzQuatInverse(sfzQuatInit(f32x3_init(1.0f, 2.0f, 3.0f), 4.0f));
		CHECK(eqf(q, sfzQuatInit(f32x3_init(-1.0f / 30.0f, -1.0f / 15.0f, -1.0f / 10.0f), 2.0f / 15.0f)));
	}
	// rotate()
	{
		f32 halfAngle1 = (90.0f * SFZ_DEG_TO_RAD) / 2.0f;
		SfzQuat rot1 = sfzQuatInit(::sinf(halfAngle1) * f32x3_init(0.0f, 1.0f, 0.0f), ::cosf(halfAngle1));
		f32x3 p = sfzQuatRotateUnit(rot1, f32x3_init(1.0f, 0.0f, 0.0f));
		CHECK(eqf(p, f32x3_init(0.0f, 0.0f, -1.0f)));
		SfzMat33 rot1mat = sfzQuatToMat33(rot1);
		CHECK(eqf(rot1mat * f32x3_init(1.0f, 0.0f, 0.0f), f32x3_init(0.0f, 0.0f, -1.0f)));

		SfzQuat rot2 = sfzQuatRotationDeg(f32x3_init(0.0f, 0.0f, 1.0f), 90.0f);
		f32x3 p2 = sfzQuatRotateUnit(rot2, f32x3_init(1.0f, 0.0f, 0.0f));
		CHECK(eqf(p2, f32x3_init(0.0f, 1.0f, 0.0f)));
		SfzMat33 rot2mat = sfzQuatToMat33(rot2);
		CHECK(eqf(rot2mat * f32x3_init(1.0f, 0.0f, 0.0f), f32x3_init(0.0f, 1.0f, 0.0f)));
	}
}

TEST_CASE("Quaternion: lerp")
{
	SfzQuat q1 = sfzQuatRotationDeg(f32x3_init(1.0f, 1.0f, 1.0f), 0.0f);
	SfzQuat q2 = sfzQuatRotationDeg(f32x3_init(1.0f, 1.0f, 1.0f), 90.0f);
	SfzQuat q3 = sfzQuatRotationDeg(f32x3_init(1.0f, 1.0f, 1.0f), 45.0f);
	CHECK(eqf(sfzQuatLerp(q1, q2, 0.5f), q3));
}
