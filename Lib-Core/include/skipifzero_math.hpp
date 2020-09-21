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

#ifndef SKIPIFZERO_MATH_HPP
#define SKIPIFZERO_MATH_HPP
#pragma once

#include "skipifzero.hpp"

namespace sfz {

// Matrix primitive
// ------------------------------------------------------------------------------------------------

// A matrix primitive with H rows of W columns.
//
// Uses column-vectors, but with row-major memory storage. I.e., if you access the first row (in
// memory) you get the first component of all column vectors. When uploading to OpenGL it needs to
// be transposed as OpenGL uses column-major storage. OpenGL also uses column-vectors, so only the
// storage layout is different. This should not be confused with Direct3D which often uses
// row-vectors. When two indices are used the first one is always used to specify row (i.e.
// y-direction) and the second one is used to specify column (i.e. x-direction).

template<typename T, uint32_t H, uint32_t W>
struct Mat final {

	Vec<T,W> rows[H];

	constexpr T* data() { return rows[0].data(); }
	constexpr const T* data() const { return rows[0].data(); }

	Vec<T,W>& row(uint32_t y) { sfz_assert(y < H); return rows[y]; }
	const Vec<T,W>& row(uint32_t y) const { sfz_assert(y < H); return rows[y]; }

	Vec<T,H> column(uint32_t x) const
	{
		sfz_assert(x < W);
		Vec<T,H> column;
		for (uint32_t y = 0; y < H; y++) column[y] = this->at(y, x);
		return column;
	}
	void setColumn(uint32_t x, Vec<T,H> col) { for (uint32_t y = 0; y < H; y++) at(y, x) = col[y]; }

	constexpr T& at(uint32_t y, uint32_t x) { return row(y)[x]; }
	constexpr T at(uint32_t y, uint32_t x) const { return row(y)[x]; }

	constexpr Mat() noexcept = default;
	constexpr Mat(const Mat&) noexcept = default;
	constexpr Mat& operator= (const Mat&) noexcept = default;
	~Mat() noexcept = default;

	constexpr explicit Mat(const T* ptr) { for (uint32_t i = 0; i < (H * W); i++) data()[i] = ptr[i]; }

	constexpr Mat(T e00, T e01, T e10, T e11) : Mat(Vec<T,2>(e00, e01), Vec<T,2>(e10, e11)) {}
	constexpr Mat(Vec<T,2> row0, Vec<T,2> row1)
	{
		static_assert(H == 2 && W == 2, "Only for 2x2 matrices");
		row(0) = row0; row(1) = row1;
	}

	constexpr Mat(T e00, T e01, T e02, T e10, T e11, T e12, T e20, T e21, T e22)
		: Mat(Vec<T,3>(e00, e01, e02), Vec<T,3>(e10, e11, e12), Vec<T,3>(e20, e21, e22)) {}
	constexpr Mat(Vec<T,3> row0, Vec<T,3> row1, Vec<T,3> row2)
	{
		static_assert(H == 3 && W == 3, "Only for 3x3 matrices");
		row(0) = row0; row(1) = row1; row(2) = row2;
	}

	constexpr Mat(T e00, T e01, T e02, T e03, T e10, T e11, T e12, T e13, T e20, T e21, T e22, T e23)
		: Mat(Vec<T,4>(e00, e01, e02, e03), Vec<T,4>(e10, e11, e12, e13), Vec<T,4>(e20, e21, e22, e23)) {}
	constexpr Mat(Vec<T,4> row0, Vec<T,4> row1, Vec<T,4> row2)
	{
		static_assert(H == 3 && W == 4, "Only for 3x4 matrices");
		row(0) = row0; row(1) = row1; row(2) = row2;
	}

	constexpr Mat(T e00, T e01, T e02, T e03, T e10, T e11, T e12, T e13, T e20, T e21, T e22, T e23, T e30, T e31, T e32, T e33)
		: Mat(Vec<T,4>(e00, e01, e02, e03), Vec<T,4>(e10, e11, e12, e13), Vec<T,4>(e20, e21, e22, e23), Vec<T,4>(e30, e31, e32, e33)) {}
	constexpr Mat(Vec<T,4> row0, Vec<T,4> row1, Vec<T,4> row2, Vec<T,4> row3)
	{
		static_assert(H == 4 && W == 4, "Only for 4x4 matrices");
		row(0) = row0; row(1) = row1; row(2) = row2; row(3) = row3;
	}

	// Constructs Matrix from one of difference size. Will add "identity" matrix if target is
	// bigger, and remove components from source if target is smaller.
	template<uint32_t OH, uint32_t OW>
	constexpr explicit Mat(const Mat<T,OH,OW>& o)
	{
		*this = Mat::identity();
		for (uint32_t y = 0, commonHeight = min(H, OH); y < commonHeight; y++) {
			for (uint32_t x = 0, commonWidth = min(W, OW); x < commonWidth; x++) {
				this->at(y, x) = o.at(y, x);
			}
		}
	}

	static constexpr Mat fill(T v) { Mat m; for (uint32_t i = 0; i < (H * W); i++) m.data()[i] = v; return m; }

	static constexpr Mat identity()
	{
		static_assert(W >= H, "Can't create identity for tall matrices");
		Mat tmp;
		uint32_t oneIdx = 0;
		for (uint32_t rowIdx = 0; rowIdx < H; rowIdx++) {
			tmp.row(rowIdx) = Vec<T,W>(T(0));
			tmp.row(rowIdx)[oneIdx] = T(1);
			oneIdx += 1;
		}
		return tmp;
	}

	static constexpr Mat scaling2(T x, T y)
	{
		static_assert(H == 2 && W == 2, "Only for 2x2 matrices");
		return Mat(x, T(0), T(0), y);
	}
	static constexpr Mat scaling2(Vec<T, 2> scale) { return Mat::scaling2(scale.x, scale.y); }
	static constexpr Mat scaling2(T scale) { return scaling2(scale, scale); }

	static constexpr Mat scaling3(T x, T y, T z)
	{
		static_assert(H >= 3 && W >= 3, "Only for 3x3 matrices and larger");
		return Mat(Mat<T,3,3>(x, T(0), T(0), T(0), y, T(0), T(0), T(0), z));
	}
	static constexpr Mat scaling3(Vec<T,3> scale) { return Mat::scaling3(scale.x, scale.y, scale.z); }
	static constexpr Mat scaling3(T scale) { return scaling3(scale, scale, scale); }

	static Mat rotation3(Vec<T,3> axis, T angleRad)
	{
		static_assert(H >= 3 && W >= 3, "Only for 3x3 matrices and larger");
		Vec<T,3> r = normalize(axis);
		T c = cos(angleRad);
		T s = sin(angleRad);
		T cm1 = T(1) - c;
		// Matrix by Goldman, page 71 of Real-Time Rendering.
		return Mat(Mat<T,3,3>(
			c + cm1 * r.x * r.x, cm1 * r.x * r.y - r.z * s, cm1 * r.x * r.z + r.y * s,
			cm1 * r.x * r.y + r.z * s, c + cm1 * r.y * r.y, cm1 * r.y * r.z - r.x * s,
			cm1 * r.x * r.z - r.y * s, cm1 * r.y * r.z + r.x * s, c + cm1 * r.z * r.z));
	}

	static constexpr Mat translation3(Vec<T,3> t)
	{
		static_assert(H >= 3 && W >= 4, "Only for 3x4 matrices and larger");
		return Mat(Mat<T,3,4>(T(1), T(0), T(0), t.x, T(0), T(1), T(0), t.y, T(0), T(0), T(1), t.z));
	}

	constexpr Mat& operator+= (const Mat& o) { for (uint32_t y = 0; y < H; y++) { row(y) += o.row(y); } return *this; }
	constexpr Mat& operator-= (const Mat& o) { for (uint32_t y = 0; y < H; y++) { row(y) -= o.row(y); } return *this; }
	constexpr Mat& operator*= (T s) { for (uint32_t y = 0; y < H; y++) { row(y) *= s; } return *this; }
	constexpr Mat& operator*= (const Mat& o) { return (*this = *this * o);}

	constexpr Mat operator+ (const Mat& o) const { return (Mat(*this) += o); }
	constexpr Mat operator- (const Mat& o) const { return (Mat(*this) -= o); }
	constexpr Mat operator- () const { return (Mat(*this) *= T(-1)); }
	constexpr Mat operator* (T rhs) const { return (Mat(*this) *= rhs); }

	constexpr Vec<T,H> operator* (const Vec<T,W>& v) const
	{
		Vec<T,H> res;
		for (uint32_t y = 0; y < H; y++) res[y] = dot(row(y), v);
		return res;
	}

	constexpr bool operator== (const Mat& o) const
	{
		for (uint32_t y = 0; y < H; y++) if (row(y) != o.row(y)) return false;
		return true;
	}
	constexpr bool operator!= (const Mat& o) const { return !(*this == o); }
};

using mat22 = Mat<float,2,2>; static_assert(sizeof(mat22) == sizeof(float) * 4, "");
using mat33 = Mat<float,3,3>; static_assert(sizeof(mat33) == sizeof(float) * 9, "");
using mat34 = Mat<float,3,4>; static_assert(sizeof(mat34) == sizeof(float) * 12, "");
using mat44 = Mat<float,4,4>; static_assert(sizeof(mat44) == sizeof(float) * 16, "");

using mat2 = mat22;
using mat3 = mat33;
using mat4 = mat44;

template<typename T, uint32_t H, uint32_t W>
constexpr Mat<T,H,W> operator* (T lhs, const Mat<T,H,W>& rhs) { return rhs * lhs; }

template<typename T, uint32_t H, uint32_t S, uint32_t W>
constexpr Mat<T,H,W> operator* (const Mat<T,H,S>& lhs, const Mat<T,S,W>& rhs)
{
	Mat<T,H,W> res;
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			res.at(y, x) = dot(lhs.row(y), rhs.column(x));
		}
	}
	return res;
}

template<typename T, uint32_t H, uint32_t W>
constexpr Mat<T,H,W> elemMult(const Mat<T,H,W>& lhs, const Mat<T,H,W>& rhs)
{
	Mat<T,H,W> res;
	for (uint32_t y = 0; y < H; y++) res.row(y) = lhs.row(y) * rhs.row(y);
	return res;
}

template<typename T, uint32_t H, uint32_t W>
constexpr Mat<T,W,H> transpose(const Mat<T,H,W>& m)
{
	Mat<T,W,H> res;
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			res.at(x, y) = m.at(y, x);
		}
	}
	return res;
}

template<typename T>
constexpr Vec<T,3> transformPoint(const Mat<T,3,4>& m, Vec<T,3> p) { return m * Vec<T,4>(p, T(1)); }

template<typename T>
constexpr Vec<T,3> transformPoint(const Mat<T,4,4>& m, Vec<T,3> p) { auto v = m * Vec<T,4>(p, T(1)); return v.xyz / v.w; }

template<typename T>
constexpr Vec<T,3> transformDir(const Mat<T,3,4>& m, Vec<T,3> d) { return m * Vec<T,4>(d, T(0)); }

template<typename T>
constexpr Vec<T,3> transformDir(const Mat<T,4,4>& m, Vec<T,3> d) { return (m * Vec<T,4>(d, T(0))).xyz; }

template<typename T>
constexpr T determinant(const Mat<T,2,2>& m) { return m.at(0,0)*m.at(1,1)- m.at(0,1)*m.at(1,0); }

template<typename T>
constexpr T determinant(const Mat<T,3,3>& m)
{
	Vec<T,3> e0 = m.row(0); Vec<T,3> e1 = m.row(1); Vec<T,3> e2 = m.row(2);
	return
		e0[0] * e1[1] * e2[2]
		+ e0[1] * e1[2] * e2[0]
		+ e0[2] * e1[0] * e2[1]
		- e0[2] * e1[1] * e2[0]
		- e0[1] * e1[0] * e2[2]
		- e0[0] * e1[2] * e2[1];
}

template<typename T>
constexpr T determinant(const Mat<T,4,4>& m)
{
	Vec<T,4> e0 = m.row(0); Vec<T,4> e1 = m.row(1); Vec<T,4> e2 = m.row(2); Vec<T,4> e3 = m.row(3);
	return
		e0[0]*e1[1]*e2[2]*e3[3] + e0[0]*e1[2]*e2[3]*e3[1] + e0[0]*e1[3]*e2[1]*e3[2]
		+ e0[1]*e1[0]*e2[3]*e3[2] + e0[1]*e1[2]*e2[0]*e3[3] + e0[1]*e1[3]*e2[2]*e3[0]
		+ e0[2]*e1[0]*e2[1]*e3[3] + e0[2]*e1[1]*e2[3]*e3[0] + e0[2]*e1[3]*e2[0]*e3[1]
		+ e0[3]*e1[0]*e2[2]*e3[1] + e0[3]*e1[1]*e2[0]*e3[2] + e0[3]*e1[2]*e2[1]*e3[0]
		- e0[0]*e1[1]*e2[3]*e3[2] - e0[0]*e1[2]*e2[1]*e3[3] - e0[0]*e1[3]*e2[2]*e3[1]
		- e0[1]*e1[0]*e2[2]*e3[3] - e0[1]*e1[2]*e2[3]*e3[0] - e0[1]*e1[3]*e2[0]*e3[2]
		- e0[2]*e1[0]*e2[3]*e3[1] - e0[2]*e1[1]*e2[0]*e3[3] - e0[2]*e1[3]*e2[1]*e3[0]
		- e0[3]*e1[0]*e2[1]*e3[2] - e0[3]*e1[1]*e2[2]*e3[0] - e0[3]*e1[2]*e2[0]*e3[1];
}

template<typename T>
constexpr Mat<T,2,2> inverse(const Mat<T,2,2>& m)
{
	const T det = determinant(m);
	if (det == T(0)) return Mat<T,2,2>::fill(T(0));

	Mat<T,2,2> tmp(
		m.row(1)[1], -m.row(0)[1],
		-m.row(1)[0], m.row(0)[0]);
	return (T(1) / det) * tmp;
}

template<typename T>
constexpr Mat<T,3,3> inverse(const Mat<T,3,3>& m)
{
	const T det = determinant(m);
	if (det == 0) return Mat<T,3,3>::fill(T(0));

	Vec<T,3> e0 = m.row(0); Vec<T,3> e1 = m.row(1); Vec<T,3> e2 = m.row(2);

	const T A =  (e1[1] * e2[2] - e1[2] * e2[1]);
	const T B = -(e1[0] * e2[2] - e1[2] * e2[0]);
	const T C =  (e1[0] * e2[1] - e1[1] * e2[0]);
	const T D = -(e0[1] * e2[2] - e0[2] * e2[1]);
	const T E =  (e0[0] * e2[2] - e0[2] * e2[0]);
	const T F = -(e0[0] * e2[1] - e0[1] * e2[0]);
	const T G =  (e0[1] * e1[2] - e0[2] * e1[1]);
	const T H = -(e0[0] * e1[2] - e0[2] * e1[0]);
	const T I =  (e0[0] * e1[1] - e0[1] * e1[0]);

	Mat<T,3,3> tmp(
		A, D, G,
		B, E, H,
		C, F, I);
	return (T(1) / det) * tmp;
}

template<typename T>
constexpr Mat<T,4,4> inverse(const Mat<T,4,4>& m)
{
	const T det = determinant(m);
	if (det == 0) return Mat<T,4,4>::fill(T(0));

	const T
		m00 = m.row(0)[0], m01 = m.row(0)[1], m02 = m.row(0)[2], m03 = m.row(0)[3],
		m10 = m.row(1)[0], m11 = m.row(1)[1], m12 = m.row(1)[2], m13 = m.row(1)[3],
		m20 = m.row(2)[0], m21 = m.row(2)[1], m22 = m.row(2)[2], m23 = m.row(2)[3],
		m30 = m.row(3)[0], m31 = m.row(3)[1], m32 = m.row(3)[2], m33 = m.row(3)[3];

	const T b00 = m11*m22*m33 + m12*m23*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 - m13*m22*m31;
	const T b01 = m01*m23*m32 + m02*m21*m33 + m03*m22*m31 - m01*m22*m33 - m02*m23*m31 - m03*m21*m32;
	const T b02 = m01*m12*m33 + m02*m13*m31 + m03*m11*m32 - m01*m13*m32 - m02*m11*m33 - m03*m12*m31;
	const T b03 = m01*m13*m22 + m02*m11*m23 + m03*m12*m21 - m01*m12*m23 - m02*m13*m21 - m03*m11*m22;
	const T b10 = m10*m23*m32 + m12*m20*m33 + m13*m22*m30 - m10*m22*m33 - m12*m23*m30 - m13*m20*m32;
	const T b11 = m00*m22*m33 + m02*m23*m30 + m03*m20*m32 - m00*m23*m32 - m02*m20*m33 - m03*m22*m30;
	const T b12 = m00*m13*m32 + m02*m10*m33 + m03*m12*m30 - m00*m12*m33 - m02*m13*m30 - m03*m10*m32;
	const T b13 = m00*m12*m23 + m02*m13*m20 + m03*m10*m22 - m00*m13*m22 - m02*m10*m23 - m03*m12*m20;
	const T b20 = m10*m21*m33 + m11*m23*m30 + m13*m20*m31 - m01*m23*m31 - m11*m20*m33 - m13*m21*m30;
	const T b21 = m00*m23*m31 + m01*m20*m33 + m03*m21*m30 - m00*m21*m33 - m01*m23*m30 - m03*m20*m31;
	const T b22 = m00*m11*m33 + m01*m13*m30 + m03*m10*m31 - m00*m13*m31 - m01*m10*m33 - m03*m11*m30;
	const T b23 = m00*m13*m21 + m01*m10*m23 + m03*m11*m20 - m00*m11*m23 - m01*m13*m20 - m03*m10*m21;
	const T b30 = m10*m22*m31 + m11*m20*m32 + m12*m21*m30 - m10*m21*m32 - m11*m22*m30 - m12*m20*m31;
	const T b31 = m00*m21*m32 + m01*m22*m30 + m02*m20*m31 - m00*m22*m31 - m01*m20*m32 - m02*m21*m30;
	const T b32 = m00*m12*m31 + m01*m10*m32 + m02*m11*m30 - m00*m11*m32 - m01*m12*m30 - m02*m10*m31;
	const T b33 = m00*m11*m22 + m01*m12*m20 + m02*m10*m21 - m00*m12*m21 - m01*m10*m22 - m02*m11*m20;

	Mat<T,4,4> tmp(
		b00, b01, b02, b03,
		b10, b11, b12, b13,
		b20, b21, b22, b23,
		b30, b31, b32, b33);
	return (T(1) / det) * tmp;
}

// Quaternion primitive
// ------------------------------------------------------------------------------------------------

template<typename T>
struct Quat final {

	// i*x + j*y + k*z + w
	// or
	// [v, w], v = [x, y, z] in the imaginary space, w is scalar real part
	// where
	// i² = j² = k² = -1
	// j*k = -k*j = i
	// k*i = -i*k = j
	// i*j = -j*i = k
	union {
		struct { T x, y, z, w; };
		struct { Vec<T,3> v; };
		Vec<T,4> vector;
	};

	constexpr Quat() noexcept = default;
	constexpr Quat(const Quat&) noexcept = default;
	constexpr Quat& operator= (const Quat&) noexcept = default;
	~Quat() noexcept = default;

	constexpr Quat(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
	constexpr Quat(Vec<T,3> v, T w) : x(v.x), y(v.y), z(v.z), w(w) { }

	// Creates an identity quaternion representing a non-rotation, i.e. [0, 0, 0, 1]
	static constexpr Quat identity() { return Quat(T(0), T(0), T(0), T(1)); }

	// Creates a unit quaternion representing a (right-handed) rotation around the specified axis.
	// The given axis will be automatically normalized.
	static Quat rotationDeg(Vec<T,3> axis, T angleDeg) { return Quat::rotationRad(axis, angleDeg * DEG_TO_RAD); }
	static Quat rotationRad(Vec<T,3> axis, T angleRad)
	{
		const T halfAngleRad = angleRad * T(0.5);
		const Vec<T,3> normalizedAxis = normalize(axis);
		return Quat(sin(halfAngleRad) * normalizedAxis, cos(halfAngleRad));
	}

	// Constructs a Quaternion from Euler angles. The rotation around the z axis is performed first,
	// then y and last x axis.
	static Quat fromEuler(T xDeg, T yDeg, T zDeg)
	{
		const T DEG_ANGLE_TO_RAD_HALF_ANGLE = (T(3.14159265358979323846f) / T(180.0f)) / T(2.0f);

		T cosZ = cos(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T sinZ = sin(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T cosY = cos(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T sinY = sin(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T cosX = cos(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T sinX = sin(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);

		Quat tmp;
		tmp.x = cosZ * sinX * cosY - sinZ * cosX * sinY;
		tmp.y = cosZ * cosX * sinY + sinZ * sinX * cosY;
		tmp.z = sinZ * cosX * cosY - cosZ * sinX * sinY;
		tmp.w = cosZ * cosX * cosY + sinZ * sinX * sinY;
		return tmp;
	}
	static Quat fromEuler(Vec<T,3> anglesDeg) { return Quat::fromEuler(anglesDeg.x, anglesDeg.y, anglesDeg.z); }

	static Quat fromRotationMatrix(const Mat<T,3,3>& m)
	{
		// Algorithm from page 205 of Game Engine Architecture 2nd Edition
		const Vec<T,3>& e0 = m.row(0); const Vec<T,3>& e1 = m.row(1); const Vec<T,3>& e2 = m.row(2);
		T trace = e0[0] + e1[1] + e2[2];

		Quat tmp;

		// Check the diagonal
		if (trace > T(0)) {
			T s = std::sqrt(trace + T(1));
			tmp.w = s * T(0.5);

			T t = T(0.5) / s;
			tmp.x = (e2[1] - e1[2]) * t;
			tmp.y = (e0[2] - e2[0]) * t;
			tmp.z = (e1[0] - e0[1]) * t;
		}
		else {
			// Diagonal is negative
			int i = 0;
			if (e1[1] > e0[0]) i = 1;
			if (e2[2] > m.at(i, i)) i = 2;

			constexpr int NEXT[3] = { 1, 2, 0 };
			int j = NEXT[i];
			int k = NEXT[j];

			T s = sqrt((m.at(i, j) - (m.at(j, j) + m.at(k, k))) + T(1));
			tmp.vector[i] = s * T(0.5);

			T t = (s != T(0.0)) ? (T(0.5) / s) : s;

			tmp.vector[3] = (m.at(k, j) - m.at(j, k)) * t;
			tmp.vector[j] = (m.at(j, i) + m.at(i, j)) * t;
			tmp.vector[k] = (m.at(k, i) + m.at(i, k)) * t;
		}

		return tmp;
	}
	static Quat fromRotationMatrix(const Mat<T,3,4>& m) { return Quat::fromRotationMatrix(Mat<T,3,3>(m)); }

	// Returns the normalized axis which the quaternion rotates around, returns 0 vector for
	// identity Quaternion. Includes a safeNormalize() call, not necessarily super fast.
	Vec<T,3> rotationAxis() const { return normalizeSafe(this->v); }

	// Returns the angle (degrees) this Quaternion rotates around the rotationAxis().
	T rotationAngleDeg() const
	{
		const T RAD_ANGLE_TO_DEG_NON_HALF_ANGLE = (T(180) / T(3.14159265358979323846)) * T(2);
		T halfAngleRad = acos(this->w);
		return halfAngleRad * RAD_ANGLE_TO_DEG_NON_HALF_ANGLE;
	}

	// Returns a Euler angle (degrees) representation of this Quaternion. Assumes the Quaternion
	// is unit.
	Vec<T,3> toEuler() const
	{
		const T RAD_ANGLE_TO_DEG = T(180) / T(3.14159265358979323846);
		Vec<T,3> tmp;
		tmp.x = atan2(T(2) * (w * x + y * z), T(1) - T(2) * (x * x + y * y)) * RAD_ANGLE_TO_DEG;
		tmp.y = asin(sfz::min(sfz::max(2.0f * (w * y - z * x), -T(1)), T(1))) * RAD_ANGLE_TO_DEG;
		tmp.z = atan2(T(2) * (w * z + x * y), T(1) - T(2) * (y * y + z * z)) * RAD_ANGLE_TO_DEG;
		return tmp;
	}

	// Converts the given Quaternion into a Matrix. The normal variants assume that the Quaternion
	// is unit, while the "non-unit" variants make no such assumptions.
	constexpr Mat<T,3,3> toMat33() const
	{
		// Algorithm from Real-Time Rendering, page 76
		return Mat<T,3,3>(T(1) - T(2)*(y*y + z*z),  T(2)*(x*y - w*z),         T(2)*(x*z + w*y),
		                  T(2)*(x*y + w*z),         T(1) - T(2)*(x*x + z*z),  T(2)*(y*z - w*x),
		                  T(2)*(x*z - w*y),         T(2)*(y*z + w*x),         T(1) - T(2)*(x*x + y*y));
	}
	constexpr Mat<T,3,4> toMat34() const { return Mat<T,3,4>(toMat33()); }
	constexpr Mat<T,4,4> toMat44() const { return Mat<T,4,4>(toMat33()); }

	Mat<T,3,3> toMat33NonUnit() const
	{
		// Algorithm from Real-Time Rendering, page 76
		T s = T(2) / length(vector);
		return Mat<T,3,3>(T(1) - s*(y*y + z*z),  s*(x*y - w*z),         s*(x*z + w*y),
		                  s*(x*y + w*z),         T(1) - s*(x*x + z*z),  s*(y*z - w*x),
		                  s*(x*z - w*y),         s*(y*z + w*x),         T(1) - s*(x*x + y*y));
	}
	Mat<T,3,4> toMat34NonUnit() const { return Mat<T,3,4>(toMat33NonUnit()); }
	Mat<T,4,4> toMat44NonUnit() const { return Mat<T,4,4>(toMat33NonUnit()); }

	constexpr Quat& operator+= (Quat o) { this->vector += o.vector; return *this; }
	constexpr Quat& operator-= (Quat o) { this->vector -= o.vector; return *this; }
	constexpr Quat& operator*= (Quat o)
	{
		Quat tmp;
		tmp.v = cross(v, o.v) + o.w * v + w * o.v;
		tmp.w = w * o.w - dot(v, o.v);
		*this = tmp;
		return *this;
	}
	constexpr Quat& operator*= (T scalar) { vector *= scalar; return *this; }

	constexpr Quat operator+ (Quat o) const { return Quat(*this) += o; }
	constexpr Quat operator- (Quat o) const { return Quat(*this) -= o; }
	constexpr Quat operator* (Quat o) const { return Quat(*this) *= o; }
	constexpr Quat operator* (T scalar) const { return Quat(*this) *= scalar; }

	constexpr bool operator== (Quat o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (Quat o) const { return !(*this == o); }
};

using quat = Quat<float>; static_assert(sizeof(quat) == sizeof(vec4), "");

template<typename T>
constexpr Quat<T> operator* (T scalar, Quat<T> q) { return q *= scalar; }

// Calculates the length (norm) of the Quaternion. A unit Quaternion has length 1. If the
// Quaternions are used for rotations they should always be unit.
template<typename T>
float length(Quat<T> q) { return length(q.vector); }

// Normalizes the Quaternion into a unit Quaternion by dividing each component by the length.
template<typename T>
Quat<T> normalize(Quat<T> q) { Quat<T> tmp; tmp.vector = normalize(q.vector); return tmp; }

// Calculates the conjugate quaternion, i.e. [-v, w]. If the quaternion is unit length this is
// the same as the inverse.
template<typename T>
constexpr Quat<T> conjugate(Quat<T> q) { return Quat<T>(-q.v, q.w); }

// Calculates the inverse for any Quaternion, i.e. (1 / length(q)²) * conjugate(q). For unit
// Quaternions (which should be the most common case) the conjugate() function should be used
// instead as it is way faster.
template<typename T>
constexpr Quat<T> inverse(Quat<T> q) { return (T(1) / dot(q.vector, q.vector)) * conjugate(q); }

// Rotates a vector with the specified Quaternion, using q * v * qInv. Either the inverse can
// be specified manually, or it can be calculated automatically from the given Quaternion. When
// it is calculated automatically it is assumed that the Quaternion is unit, so the inverse is
// the conjugate.
template<typename T>
constexpr Vec<T,3> rotate(Quat<T> q, Vec<T,3> v, Quat<T> qInv)
{
	Quat<T> tmp(v, T(0));
	tmp = q * tmp * qInv;
	return tmp.v;
}

template<typename T>
constexpr Vec<T,3> rotate(Quat<T> q, Vec<T,3> v) { return rotate(q, v, conjugate(q)); }

template<typename T>
Quat<T> lerp(Quat<T> q0, Quat<T> q1, T t)
{
	Quat<T> tmp;
	tmp.vector = lerp(q0.vector, q1.vector, t);
	tmp = normalize(tmp);
	return tmp;
}

// rotateTowards()
// ------------------------------------------------------------------------------------------------

// Rotates a vector towards another vector by a given amount of radians. Both the input and the
// target vector must be normalized. In addition, they must not be the same vector or point in
// exact opposite directions.
//
// The variants marked "ClampSafe" handle annoying edge cases. If the angle specified is greater
// than the angle between the two vectors then the target vector will be returned. The input
// vectors are no longer assumed to be normalized. And if they happen to be invalid (i.e. the same
// vector or pointing in exact opposite directions) a sane default will be given.

inline vec3 rotateTowardsRad(vec3 inDir, vec3 targetDir, float angleRads)
{
	sfz_assert(eqf(length(inDir), 1.0f));
	sfz_assert(eqf(length(targetDir), 1.0f));
	sfz_assert(dot(inDir, targetDir) >= -0.99f);
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < PI);
	vec3 axis = cross(inDir, targetDir);
	sfz_assert(!eqf(axis, vec3(0.0f)));
	quat rotQuat = quat::rotationRad(axis, angleRads);
	vec3 newDir = rotate(rotQuat, inDir);
	return newDir;
}

inline vec3 rotateTowardsRadClampSafe(vec3 inDir, vec3 targetDir, float angleRads)
{
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < PI);

	vec3 inDirNorm = normalizeSafe(inDir);
	vec3 targetDirNorm = normalizeSafe(targetDir);
	sfz_assert(!eqf(inDirNorm, vec3(0.0f)));
	sfz_assert(!eqf(targetDirNorm, vec3(0.0f)));

	// Case where vectors are the same, just return the target dir
	if (eqf(inDirNorm, targetDirNorm)) return targetDirNorm;

	// Case where vectors are exact opposite, slightly nudge input a bit
	if (eqf(inDirNorm, -targetDirNorm)) {
		inDirNorm = normalize(inDir + (vec3(1.0f) - inDirNorm) * 0.025f);
		sfz_assert(!eqf(inDirNorm, -targetDirNorm));
	}

	// Case where angle is larger than the angle between the vectors
	if (angleRads >= acos(dot(inDirNorm, targetDirNorm))) return targetDirNorm;

	// At this point all annoying cases should be handled, just run the normal routine
	return rotateTowardsRad(inDirNorm, targetDirNorm, angleRads);
}

inline vec3 rotateTowardsDeg(vec3 inDir, vec3 targetDir, float angleDegs)
{
	return rotateTowardsRad(inDir, targetDir, DEG_TO_RAD * angleDegs);
}

inline vec3 rotateTowardsDegClampSafe(vec3 inDir, vec3 targetDir, float angleDegs)
{
	return rotateTowardsRadClampSafe(inDir, targetDir, DEG_TO_RAD * angleDegs);
}

} // namespace sfz

#endif
