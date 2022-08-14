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

#ifndef SFZ_MATRIX_H
#define SFZ_MATRIX_H
#pragma once

#include "sfz.h"

// Matrix 3x3 Operators
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

constexpr SfzMat33& operator+= (SfzMat33& lhs, const SfzMat33& rhs) { for (u32 y = 0; y < 3; y++) { lhs.rows[y] += rhs.rows[y]; } return lhs; }
constexpr SfzMat33& operator-= (SfzMat33& lhs, const SfzMat33& rhs) { for (u32 y = 0; y < 3; y++) { lhs.rows[y] -= rhs.rows[y]; } return lhs; }
constexpr SfzMat33& operator*= (SfzMat33& lhs, f32 s) { for (u32 y = 0; y < 3; y++) { lhs.rows[y] *= s; } return lhs; }
constexpr SfzMat33 operator* (SfzMat33 lhs, SfzMat33 rhs)
{
	SfzMat33 res = {};
	for (u32 y = 0; y < 3; y++) {
		for (u32 x = 0; x < 3; x++) {
			for (u32 s = 0; s < 3; s++) {
				res.at(y, x) += lhs.rows[y][s] * rhs.column(x)[s];
			}
		}
	}
	return res;
}
constexpr SfzMat33& operator*= (SfzMat33& lhs, const SfzMat33& rhs) { return (lhs = lhs * rhs); }
constexpr SfzMat33 operator* (SfzMat33 lhs, f32 rhs) { return (lhs *= rhs); }
constexpr SfzMat33 operator* (f32 lhs, SfzMat33 rhs) { return rhs * lhs; }
constexpr SfzMat33 operator+ (SfzMat33 lhs, SfzMat33 rhs) { return (lhs += rhs); }
constexpr SfzMat33 operator- (SfzMat33 lhs, SfzMat33 rhs) { return (lhs -= rhs); }
constexpr SfzMat33 operator- (SfzMat33 m) { return (m *= -1.0f); }
constexpr f32x3 operator* (SfzMat33 m, f32x3 v)
{
	return f32x3_init(f32x3_dot(m.rows[0], v), f32x3_dot(m.rows[1], v), f32x3_dot(m.rows[2], v));
}

#endif

// Matrix 4x4 Operators
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

constexpr SfzMat44& operator+= (SfzMat44& lhs, const SfzMat44& rhs) { for (u32 y = 0; y < 4; y++) { lhs.rows[y] += rhs.rows[y]; } return lhs; }
constexpr SfzMat44& operator-= (SfzMat44& lhs, const SfzMat44& rhs) { for (u32 y = 0; y < 4; y++) { lhs.rows[y] -= rhs.rows[y]; } return lhs; }
constexpr SfzMat44& operator*= (SfzMat44& lhs, f32 s) { for (u32 y = 0; y < 4; y++) { lhs.rows[y] *= s; } return lhs; }
constexpr SfzMat44 operator* (SfzMat44 lhs, SfzMat44 rhs)
{
	SfzMat44 res = {};
	for (u32 y = 0; y < 4; y++) {
		for (u32 x = 0; x < 4; x++) {
			for (u32 s = 0; s < 4; s++) {
				res.at(y, x) += lhs.rows[y][s] * rhs.column(x)[s];
			}
		}
	}
	return res;
}
constexpr SfzMat44& operator*= (SfzMat44& lhs, const SfzMat44& rhs) { return (lhs = lhs * rhs); }
constexpr SfzMat44 operator* (SfzMat44 lhs, f32 rhs) { return (lhs *= rhs); }
constexpr SfzMat44 operator* (f32 lhs, SfzMat44 rhs) { return rhs * lhs; }
constexpr SfzMat44 operator+ (SfzMat44 lhs, SfzMat44 rhs) { return (lhs += rhs); }
constexpr SfzMat44 operator- (SfzMat44 lhs, SfzMat44 rhs) { return (lhs -= rhs); }
constexpr SfzMat44 operator- (SfzMat44 m) { return (m *= -1.0f); }
constexpr f32x4 operator* (SfzMat44 m, f32x4 v)
{
	return f32x4_init(
		f32x4_dot(m.rows[0], v), f32x4_dot(m.rows[1], v), f32x4_dot(m.rows[2], v), f32x4_dot(m.rows[3], v));
}

#endif

// Matrix 3x3 Functions
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func SfzMat33 sfzMat33FromMat44(const SfzMat44 m)
{
	return sfzMat33InitRows(m.rows[0].xyz(), m.rows[1].xyz(), m.rows[2].xyz());
}

sfz_constexpr_func SfzMat33 sfzMat33Identity()
{
	return sfzMat33InitElems(
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f);
}

sfz_constexpr_func SfzMat33 sfzMat33Scaling3(f32x3 scale)
{
	return sfzMat33InitElems(
		scale.x, 0.0f, 0.0f,
		0.0f, scale.y, 0.0f,
		0.0f, 0.0f, scale.z);
}

#ifdef __cplusplus

inline SfzMat33 sfzMat33Rotation3(f32x3 axis, f32 angleRad)
{
	const f32x3 r = f32x3_normalize(axis);
	const f32 c = sfz_cos(angleRad);
	const f32 s = sfz_sin(angleRad);
	const f32 cm1 = 1.0f - c;
	// Matrix by Goldman, page 71 of Real-Time Rendering.
	return sfzMat33InitElems(
		c + cm1 * r.x * r.x, cm1 * r.x * r.y - r.z * s, cm1 * r.x * r.z + r.y * s,
		cm1 * r.x * r.y + r.z * s, c + cm1 * r.y * r.y, cm1 * r.y * r.z - r.x * s,
		cm1 * r.x * r.z - r.y * s, cm1 * r.y * r.z + r.x * s, c + cm1 * r.z * r.z);
}

constexpr SfzMat33 sfzMat33Transpose(SfzMat33 m)
{
	SfzMat33 res = {};
	for (u32 y = 0; y < 3; y++) {
		for (u32 x = 0; x < 3; x++) {
			res.at(x, y) = m.at(y, x);
		}
	}
	return res;
}

constexpr f32 sfzMat33Determinant(SfzMat33 m)
{
	const f32x3 e0 = m.rows[0]; const f32x3 e1 = m.rows[1]; const f32x3 e2 = m.rows[2];
	return
		e0[0] * e1[1] * e2[2] +
		e0[1] * e1[2] * e2[0] +
		e0[2] * e1[0] * e2[1] -
		e0[2] * e1[1] * e2[0] -
		e0[1] * e1[0] * e2[2] -
		e0[0] * e1[2] * e2[1];
}

constexpr SfzMat33 sfzMat33Inverse(SfzMat33 m)
{
	const f32 det = sfzMat33Determinant(m);
	SfzMat33 res = {};
	if (det == 0) return res;

	const f32x3 e0 = m.rows[0]; const f32x3 e1 = m.rows[1]; const f32x3 e2 = m.rows[2];

	const f32 A = (e1[1] * e2[2] - e1[2] * e2[1]);
	const f32 B = -(e1[0] * e2[2] - e1[2] * e2[0]);
	const f32 C = (e1[0] * e2[1] - e1[1] * e2[0]);
	const f32 D = -(e0[1] * e2[2] - e0[2] * e2[1]);
	const f32 E = (e0[0] * e2[2] - e0[2] * e2[0]);
	const f32 F = -(e0[0] * e2[1] - e0[1] * e2[0]);
	const f32 G = (e0[1] * e1[2] - e0[2] * e1[1]);
	const f32 H = -(e0[0] * e1[2] - e0[2] * e1[0]);
	const f32 I = (e0[0] * e1[1] - e0[1] * e1[0]);

	res = sfzMat33InitElems(
		A, D, G,
		B, E, H,
		C, F, I);
	return (1.0f / det) * res;
}

#endif

// Matrix 4x4 Functions
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func SfzMat44 sfzMat44FromMat33(SfzMat33 m)
{
	return sfzMat44InitRows(
		f32x4_init3(m.rows[0], 0.0f),
		f32x4_init3(m.rows[1], 0.0f),
		f32x4_init3(m.rows[2], 0.0f),
		f32x4_init(0.0f, 0.0f, 0.0f, 1.0f));
}

sfz_constexpr_func SfzMat44 sfzMat44Identity()
{
	return sfzMat44InitElems(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

sfz_constexpr_func SfzMat44 sfzMat44Scaling3(f32x3 scale)
{
	return sfzMat44InitElems(
		scale.x, 0.0f, 0.0f, 0.0f,
		0.0f, scale.y, 0.0f, 0.0f,
		0.0f, 0.0f, scale.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

sfz_constexpr_func SfzMat44 sfzMat44Translation3(f32x3 t)
{
	return sfzMat44InitElems(
		1.0f, 0.0f, 0.0f, t.x,
		0.0f, 1.0f, 0.0f, t.y,
		0.0f, 0.0f, 1.0f, t.z,
		0.0f, 0.0f, 0.0f, 1.0f);
}

#ifdef __cplusplus

inline SfzMat44 sfzMat44Rotation3(f32x3 axis, f32 angleRad)
{
	const f32x3 r = f32x3_normalize(axis);
	const f32 c = sfz_cos(angleRad);
	const f32 s = sfz_sin(angleRad);
	const f32 cm1 = 1.0f - c;
	// Matrix by Goldman, page 71 of Real-Time Rendering.
	return sfzMat44InitElems(
		c + cm1 * r.x * r.x, cm1 * r.x * r.y - r.z * s, cm1 * r.x * r.z + r.y * s, 0.0f,
		cm1 * r.x * r.y + r.z * s, c + cm1 * r.y * r.y, cm1 * r.y * r.z - r.x * s, 0.0f,
		cm1 * r.x * r.z - r.y * s, cm1 * r.y * r.z + r.x * s, c + cm1 * r.z * r.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

constexpr f32x3 sfzMat44TransformPoint(SfzMat44 m, f32x3 p) { const f32x4 v = m * f32x4_init3(p, 1.0f); return v.xyz() / v.w; }
constexpr f32x3 sfzMat44TransformDir(SfzMat44 m, f32x3 d) { const f32x4 v = m * f32x4_init3(d, 0.0f); return v.xyz(); }

constexpr SfzMat44 sfzMat44Transpose(SfzMat44 m)
{
	SfzMat44 res = {};
	for (u32 y = 0; y < 4; y++) {
		for (u32 x = 0; x < 4; x++) {
			res.at(x, y) = m.at(y, x);
		}
	}
	return res;
}

constexpr f32 sfzMat44Determinant(SfzMat44 m)
{
	const f32x4 e0 = m.rows[0];
	const f32x4 e1 = m.rows[1];
	const f32x4 e2 = m.rows[2];
	const f32x4 e3 = m.rows[3];
	return
		e0[0] * e1[1] * e2[2] * e3[3] + e0[0] * e1[2] * e2[3] * e3[1] + e0[0] * e1[3] * e2[1] * e3[2] +
		e0[1] * e1[0] * e2[3] * e3[2] + e0[1] * e1[2] * e2[0] * e3[3] + e0[1] * e1[3] * e2[2] * e3[0] +
		e0[2] * e1[0] * e2[1] * e3[3] + e0[2] * e1[1] * e2[3] * e3[0] + e0[2] * e1[3] * e2[0] * e3[1] +
		e0[3] * e1[0] * e2[2] * e3[1] + e0[3] * e1[1] * e2[0] * e3[2] + e0[3] * e1[2] * e2[1] * e3[0] -
		e0[0] * e1[1] * e2[3] * e3[2] - e0[0] * e1[2] * e2[1] * e3[3] - e0[0] * e1[3] * e2[2] * e3[1] -
		e0[1] * e1[0] * e2[2] * e3[3] - e0[1] * e1[2] * e2[3] * e3[0] - e0[1] * e1[3] * e2[0] * e3[2] -
		e0[2] * e1[0] * e2[3] * e3[1] - e0[2] * e1[1] * e2[0] * e3[3] - e0[2] * e1[3] * e2[1] * e3[0] -
		e0[3] * e1[0] * e2[1] * e3[2] - e0[3] * e1[1] * e2[2] * e3[0] - e0[3] * e1[2] * e2[0] * e3[1];
}

constexpr SfzMat44 sfzMat44Inverse(SfzMat44 m)
{
	const f32 det = sfzMat44Determinant(m);
	SfzMat44 res = {};
	if (det == 0) res;

	const f32
		m00 = m.rows[0][0], m01 = m.rows[0][1], m02 = m.rows[0][2], m03 = m.rows[0][3],
		m10 = m.rows[1][0], m11 = m.rows[1][1], m12 = m.rows[1][2], m13 = m.rows[1][3],
		m20 = m.rows[2][0], m21 = m.rows[2][1], m22 = m.rows[2][2], m23 = m.rows[2][3],
		m30 = m.rows[3][0], m31 = m.rows[3][1], m32 = m.rows[3][2], m33 = m.rows[3][3];

	const f32 b00 = m11*m22*m33 + m12*m23*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 - m13*m22*m31;
	const f32 b01 = m01*m23*m32 + m02*m21*m33 + m03*m22*m31 - m01*m22*m33 - m02*m23*m31 - m03*m21*m32;
	const f32 b02 = m01*m12*m33 + m02*m13*m31 + m03*m11*m32 - m01*m13*m32 - m02*m11*m33 - m03*m12*m31;
	const f32 b03 = m01*m13*m22 + m02*m11*m23 + m03*m12*m21 - m01*m12*m23 - m02*m13*m21 - m03*m11*m22;
	const f32 b10 = m10*m23*m32 + m12*m20*m33 + m13*m22*m30 - m10*m22*m33 - m12*m23*m30 - m13*m20*m32;
	const f32 b11 = m00*m22*m33 + m02*m23*m30 + m03*m20*m32 - m00*m23*m32 - m02*m20*m33 - m03*m22*m30;
	const f32 b12 = m00*m13*m32 + m02*m10*m33 + m03*m12*m30 - m00*m12*m33 - m02*m13*m30 - m03*m10*m32;
	const f32 b13 = m00*m12*m23 + m02*m13*m20 + m03*m10*m22 - m00*m13*m22 - m02*m10*m23 - m03*m12*m20;
	const f32 b20 = m10*m21*m33 + m11*m23*m30 + m13*m20*m31 - m01*m23*m31 - m11*m20*m33 - m13*m21*m30;
	const f32 b21 = m00*m23*m31 + m01*m20*m33 + m03*m21*m30 - m00*m21*m33 - m01*m23*m30 - m03*m20*m31;
	const f32 b22 = m00*m11*m33 + m01*m13*m30 + m03*m10*m31 - m00*m13*m31 - m01*m10*m33 - m03*m11*m30;
	const f32 b23 = m00*m13*m21 + m01*m10*m23 + m03*m11*m20 - m00*m11*m23 - m01*m13*m20 - m03*m10*m21;
	const f32 b30 = m10*m22*m31 + m11*m20*m32 + m12*m21*m30 - m10*m21*m32 - m11*m22*m30 - m12*m20*m31;
	const f32 b31 = m00*m21*m32 + m01*m22*m30 + m02*m20*m31 - m00*m22*m31 - m01*m20*m32 - m02*m21*m30;
	const f32 b32 = m00*m12*m31 + m01*m10*m32 + m02*m11*m30 - m00*m11*m32 - m01*m12*m30 - m02*m10*m31;
	const f32 b33 = m00*m11*m22 + m01*m12*m20 + m02*m10*m21 - m00*m12*m21 - m01*m10*m22 - m02*m11*m20;

	res = sfzMat44InitElems(
		b00, b01, b02, b03,
		b10, b11, b12, b13,
		b20, b21, b22, b23,
		b30, b31, b32, b33);
	return (1.0f / det) * res;
}

#endif

#endif
