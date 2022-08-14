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

#ifndef SFZ_QUAT_H
#define SFZ_QUAT_H
#pragma once

#include "sfz.h"

// Quaternion operators
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

constexpr SfzQuat& operator+= (SfzQuat& lhs, SfzQuat rhs) { lhs.v += rhs.v; lhs.w += rhs.w; return lhs; }
constexpr SfzQuat& operator-= (SfzQuat& lhs, SfzQuat rhs) { lhs.v -= rhs.v; lhs.w -= rhs.w; return lhs; }
constexpr SfzQuat& operator*= (SfzQuat& lhs, SfzQuat rhs)
{
	SfzQuat tmp = {};
	tmp.v = f32x3_cross(lhs.v, rhs.v) + rhs.w * lhs.v + lhs.w * rhs.v;
	tmp.w = lhs.w * rhs.w - f32x3_dot(lhs.v, rhs.v);
	lhs = tmp;
	return lhs;
}
constexpr SfzQuat& operator*= (SfzQuat& q, f32 scalar) { q.v *= scalar; q.w *= scalar; return q; }

constexpr SfzQuat operator+ (SfzQuat lhs, SfzQuat rhs) { return lhs += rhs; }
constexpr SfzQuat operator- (SfzQuat lhs, SfzQuat rhs) { return lhs -= rhs; }
constexpr SfzQuat operator* (SfzQuat lhs, SfzQuat rhs) { return lhs *= rhs; }
constexpr SfzQuat operator* (SfzQuat q, f32 scalar) { return q *= scalar; }
constexpr SfzQuat operator* (f32 scalar, SfzQuat q) { return q *= scalar; }

#endif

// Quaternion functions
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func SfzQuat sfzQuatIdentity() { return sfzQuatInit(f32x3_init(0.0f, 0.0f, 0.0f), 1.0f); }

#ifdef __cplusplus

// Creates a unit quaternion representing a (right-handed) rotation around the specified axis.
// The given axis will be automatically normalized.
inline SfzQuat sfzQuatRotationRad(f32x3 axis, f32 angleRad)
{
	const f32 halfAngleRad = angleRad * 0.5f;
	const f32x3 normalizedAxis = f32x3_normalize(axis);
	return sfzQuatInit(sinf(halfAngleRad) * normalizedAxis, cosf(halfAngleRad));
}
inline SfzQuat sfzQuatRotationDeg(f32x3 axis, f32 angleDeg) { return sfzQuatRotationRad(axis, angleDeg * SFZ_DEG_TO_RAD); }

// Constructs a Quaternion from Euler angles. The rotation around the z axis is performed first,
// then y and last x axis.
inline SfzQuat sfzQuatFromEuler(f32x3 anglesDeg)
{
	f32 xDeg = anglesDeg.x, yDeg = anglesDeg.y, zDeg = anglesDeg.z;
	const f32 DEG_ANGLE_TO_RAD_HALF_ANGLE = (f32(3.14159265358979323846f) / f32(180.0f)) / f32(2.0f);

	f32 cosZ = sfz_cos(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	f32 sinZ = sfz_sin(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	f32 cosY = sfz_cos(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	f32 sinY = sfz_sin(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	f32 cosX = sfz_cos(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	f32 sinX = sfz_sin(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);

	SfzQuat tmp;
	tmp.v.x = cosZ * sinX * cosY - sinZ * cosX * sinY;
	tmp.v.y = cosZ * cosX * sinY + sinZ * sinX * cosY;
	tmp.v.z = sinZ * cosX * cosY - cosZ * sinX * sinY;
	tmp.w = cosZ * cosX * cosY + sinZ * sinX * sinY;
	return tmp;
}

inline SfzQuat sfzQuatFromRotationMatrix(SfzMat33 m)
{
	// Algorithm from page 205 of Game Engine Architecture 2nd Edition
	const f32x3 e0 = m.rows[0]; const f32x3 e1 = m.rows[1]; const f32x3 e2 = m.rows[2];
	f32 trace = e0[0] + e1[1] + e2[2];

	SfzQuat tmp;

	// Check the diagonal
	if (trace > 0.0f) {
		f32 s = sfz_sqrt(trace + 1.0f);
		tmp.w = s * 0.5f;

		f32 t = 0.5f / s;
		tmp.v.x = (e2[1] - e1[2]) * t;
		tmp.v.y = (e0[2] - e2[0]) * t;
		tmp.v.z = (e1[0] - e0[1]) * t;
	}
	else {
		// Diagonal is negative
		i32 i = 0;
		if (e1[1] > e0[0]) i = 1;
		if (e2[2] > m.at(i, i)) i = 2;

		constexpr i32 NEXT[3] = { 1, 2, 0 };
		i32 j = NEXT[i];
		i32 k = NEXT[j];

		f32 s = sfz_sqrt((m.at(i, j) - (m.at(j, j) + m.at(k, k))) + 1.0f);
		tmp.v[i] = s * 0.5f;

		f32 t = (s != 0.0f) ? (0.5f / s) : s;

		tmp.w = (m.at(k, j) - m.at(j, k)) * t;
		tmp.v[j] = (m.at(j, i) + m.at(i, j)) * t;
		tmp.v[k] = (m.at(k, i) + m.at(i, k)) * t;
	}

	return tmp;
}

// Returns the normalized axis which the quaternion rotates around, returns 0 vector for
// identity Quaternion. Includes a safeNormalize() call, not necessarily super fast.
inline f32x3 sfzQuatRotationAxis(SfzQuat q) { return f32x3_normalizeSafe(q.v); }

// Returns the angle (degrees) this Quaternion rotates around the rotationAxis().
inline f32 sfzQuatRotationAngleDeg(SfzQuat q)
{
	const f32 RAD_ANGLE_TO_DEG_NON_HALF_ANGLE = (180.0f / 3.14159265358979323846f) * 2.0f;
	f32 halfAngleRad = sfz_acos(q.w);
	return halfAngleRad * RAD_ANGLE_TO_DEG_NON_HALF_ANGLE;
}

// Returns a Euler angle (degrees) representation of this Quaternion. Assumes the Quaternion
// is unit.
inline f32x3 sfzQuatToEuler(SfzQuat q)
{
	const f32 x = q.v.x, y = q.v.y, z = q.v.z, w = q.w;
	const f32 RAD_ANGLE_TO_DEG = 180.0f / 3.14159265358979323846f;
	f32x3 tmp;
	tmp.x = sfz_atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y)) * RAD_ANGLE_TO_DEG;
	tmp.y = sfz_asin(f32_min(f32_max(2.0f * (w * y - z * x), -1.0f), 1.0f)) * RAD_ANGLE_TO_DEG;
	tmp.z = sfz_atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z)) * RAD_ANGLE_TO_DEG;
	return tmp;
}

// Converts the given Quaternion into a Matrix. The normal variants assume that the Quaternion
// is unit, while the "non-unit" variants make no such assumptions.
constexpr SfzMat33 sfzQuatToMat33(SfzQuat q)
{
	const f32 x = q.v.x, y = q.v.y, z = q.v.z, w = q.w;
	// Algorithm from Real-Time Rendering, page 76
	return sfzMat33InitElems(
		1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y),
		2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x),
		2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y));
}

inline SfzMat33 sfzQuatToMat33NonUnit(SfzQuat q)
{
	const f32 x = q.v.x, y = q.v.y, z = q.v.z, w = q.w;
	// Algorithm from Real-Time Rendering, page 76
	f32 s = 2.0f / f32x4_length(f32x4_init3(q.v, q.w));
	return sfzMat33InitElems(
		1.0f - s * (y * y + z * z), s * (x * y - w * z), s * (x * z + w * y),
		s * (x * y + w * z), 1.0f - s * (x * x + z * z), s * (y * z - w * x),
		s * (x * z - w * y), s * (y * z + w * x), 1.0f - s * (x * x + y * y));
}

// Calculates the length (norm) of the Quaternion. A unit Quaternion has length 1. If the
// Quaternions are used for rotations they should always be unit.
inline f32 sfzQuatLength(SfzQuat q) { return f32x4_length(f32x4_init3(q.v, q.w)); }

// Normalizes the Quaternion into a unit Quaternion by dividing each component by the length.
inline SfzQuat sfzQuatNormalize(SfzQuat q) { const f32x4 tmp = f32x4_normalize(f32x4_init3(q.v, q.w)); return sfzQuatInit(tmp.xyz(), tmp.w); }

// Calculates the conjugate quaternion, i.e. [-v, w]. If the quaternion is unit length this is
// the same as the inverse.
constexpr SfzQuat sfzQuatConjugate(SfzQuat q) { return sfzQuatInit(-q.v, q.w); }

// Calculates the inverse for any Quaternion, i.e. (1 / length(q)²) * conjugate(q). For unit
// Quaternions (which should be the most common case) the conjugate() function should be used
// instead as it is way faster.
constexpr SfzQuat sfzQuatInverse(SfzQuat q) { return (1.0f / f32x4_dot(f32x4_init3(q.v, q.w), f32x4_init3(q.v, q.w))) * sfzQuatConjugate(q); }

// Rotates a vector with the specified Quaternion, using q * v * qInv. Either the inverse can
// be specified manually, or it can be calculated automatically from the given Quaternion. When
// it is calculated automatically it is assumed that the Quaternion is unit, so the inverse is
// the conjugate.
constexpr f32x3 sfzQuatRotate(SfzQuat q, f32x3 v, SfzQuat qInv)
{
	SfzQuat tmp = sfzQuatInit(v, 0.0f);
	tmp = q * tmp * qInv;
	return tmp.v;
}

constexpr f32x3 sfzQuatRotateUnit(SfzQuat q, f32x3 v) { return sfzQuatRotate(q, v, sfzQuatConjugate(q)); }

inline SfzQuat sfzQuatLerp(SfzQuat q0, SfzQuat q1, f32 t)
{
	const f32x4 v0 = f32x4_init3(q0.v, q0.w);
	const f32x4 v1 = f32x4_init3(q1.v, q1.w);
	f32x4 tmp = (1.0f - t) * v0 + t * v1;
	tmp = f32x4_normalize(tmp);
	return sfzQuatInit(tmp.xyz(), tmp.w);
}

#endif

#endif
