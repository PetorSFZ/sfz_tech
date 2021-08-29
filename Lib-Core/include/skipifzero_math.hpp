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

#include <math.h> // std::sqrt, std::fmodf

#include "skipifzero.hpp"

#ifdef _WIN32
#pragma warning(disable : 4201) // nonstandard extension: nameless struct/union
#endif

namespace sfz {

// Legacy vector primitives
// ------------------------------------------------------------------------------------------------

// These are legacy vector primitives, they will be removed once the dependency in the matrix
// types has been removed.

template<typename T, u32 N>
struct Vec final { static_assert(N != N, "Only 2, 3 and 4-dimensional vectors supported."); };

template<typename T>
struct Vec<T,2> final {

	T x, y;

	constexpr Vec() noexcept = default;
	constexpr Vec(const Vec<T,2>&) noexcept = default;
	constexpr Vec<T,2>& operator= (const Vec<T,2>&) noexcept = default;
	~Vec() noexcept = default;

	constexpr explicit Vec(const T* ptr) : x(ptr[0]), y(ptr[1]) { }
	constexpr explicit Vec(T val) : x(val), y(val) { }
	constexpr Vec(T x, T y) : x(x), y(y) { }

	constexpr Vec(f32x2 o) : x(T(o.x)), y(T(o.y)) {}
	constexpr operator f32x2() const { return f32x2(f32(x), f32(y)); }
	constexpr Vec(i32x2 o) : x(T(o.x)), y(T(o.y)) {}
	constexpr operator i32x2() const { return i32x2(i32(x), i32(y)); }

	template<typename T2>
	constexpr explicit Vec(Vec<T2,2> o) : x(T(o.x)), y(T(o.y)) { }

	constexpr T* data() noexcept { return &x; }
	constexpr const T* data() const noexcept { return &x; }
	constexpr T& operator[] (u32 idx) { sfz_assert(idx < 2); return data()[idx]; }
	constexpr T operator[] (u32 idx) const { sfz_assert(idx < 2); return data()[idx]; }

	constexpr Vec& operator+= (Vec o) { x += o.x; y += o.y; return *this; }
	constexpr Vec& operator-= (Vec o) { x -= o.x; y -= o.y; return *this; }
	constexpr Vec& operator*= (T s) { x *= s; y *= s; return *this; }
	constexpr Vec& operator*= (Vec o) { x *= o.x; y *= o.y; return *this; }
	constexpr Vec& operator/= (T s) { x /= s; y /= s; return *this; }
	constexpr Vec& operator/= (Vec o) { x /= o.x; y /= o.y; return *this; }

	constexpr Vec operator+ (Vec o) const { return Vec(*this) += o; }
	constexpr Vec operator- (Vec o) const { return Vec(*this) -= o; }
	constexpr Vec operator- () const { return Vec(-x, -y); }
	constexpr Vec operator* (Vec o) const { return Vec(*this) *= o; }
	constexpr Vec operator* (T s) const { return Vec(*this) *= s; }
	constexpr Vec operator/ (Vec o) const { return Vec(*this) /= o; }
	constexpr Vec operator/ (T s) const { return Vec(*this) /= s; }

	constexpr bool operator== (Vec o) const { return x == o.x && y == o.y; }
	constexpr bool operator!= (Vec o) const { return !(*this == o); }
};

template<typename T>
struct Vec<T,3> final {

	T x, y, z;

	Vec<T,2>& xy() { return *reinterpret_cast<Vec<T,2>*>(&x); }
	Vec<T,2>& yz() { return *reinterpret_cast<Vec<T,2>*>(&y); }
	constexpr Vec<T,2> xy() const { return *reinterpret_cast<const Vec<T,2>*>(&x); }
	constexpr Vec<T,2> yz() const { return *reinterpret_cast<const Vec<T,2>*>(&y); }

	constexpr Vec() noexcept = default;
	constexpr Vec(const Vec<T,3>&) noexcept = default;
	constexpr Vec<T,3>& operator= (const Vec<T,3>&) noexcept = default;
	~Vec() noexcept = default;

	constexpr explicit Vec(const T* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) { }
	constexpr explicit Vec(T val) : x(val), y(val), z(val) { }
	constexpr Vec(T x, T y, T z) : x(x), y(y), z(z) { }
	constexpr Vec(Vec<T,2> xy, T z) : x(xy.x), y(xy.y), z(z) { }
	constexpr Vec(T x, Vec<T,2> yz) : x(x), y(yz.x), z(yz.y) { }

	constexpr Vec(f32x3 o) : x(T(o.x)), y(T(o.y)), z(T(o.z)) {}
	constexpr operator f32x3() const { return f32x3(f32(x), f32(y), f32(z)); }
	constexpr Vec(i32x3 o) : x(T(o.x)), y(T(o.y)), z(T(o.z)) {}
	constexpr operator i32x3() const { return i32x3(i32(x), i32(y), i32(z)); }

	template<typename T2>
	constexpr explicit Vec(Vec<T2,3> o) : x(T(o.x)), y(T(o.y)), z(T(o.z)) { }

	constexpr T* data() { return &x; }
	constexpr const T* data() const { return &x; }
	constexpr T& operator[] (u32 idx) { sfz_assert(idx < 3); return data()[idx]; }
	constexpr T operator[] (u32 idx) const { sfz_assert(idx < 3); return data()[idx]; }

	constexpr Vec& operator+= (Vec o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr Vec& operator-= (Vec o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr Vec& operator*= (T s) { x *= s; y *= s; z *= s; return *this; }
	constexpr Vec& operator*= (Vec o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr Vec& operator/= (T s) { x /= s; y /= s; z /= s; return *this; }
	constexpr Vec& operator/= (Vec o) { x /= o.x; y /= o.y; z /= o.z; return *this; }

	constexpr Vec operator+ (Vec o) const { return Vec(*this) += o; }
	constexpr Vec operator- (Vec o) const { return Vec(*this) -= o; }
	constexpr Vec operator- () const { return Vec(-x, -y, -z); }
	constexpr Vec operator* (Vec o) const { return Vec(*this) *= o; }
	constexpr Vec operator* (T s) const { return Vec(*this) *= s; }
	constexpr Vec operator/ (Vec o) const { return Vec(*this) /= o; }
	constexpr Vec operator/ (T s) const { return Vec(*this) /= s; }

	constexpr bool operator== (Vec o) const { return x == o.x && y == o.y && z == o.z; }
	constexpr bool operator!= (Vec o) const { return !(*this == o); }
};

template<typename T>
struct Vec<T,4> final {

	T x, y, z, w;

	Vec<T,2>& xy() { return *reinterpret_cast<Vec<T,2>*>(&x); }
	Vec<T,2>& yz() { return *reinterpret_cast<Vec<T,2>*>(&y); }
	Vec<T,2>& zw() { return *reinterpret_cast<Vec<T,2>*>(&z); }
	Vec<T,3>& xyz() { return *reinterpret_cast<Vec<T,3>*>(&x); }
	Vec<T,3>& yzw() { return *reinterpret_cast<Vec<T,3>*>(&y); }
	constexpr Vec<T,2> xy() const { return *reinterpret_cast<const Vec<T,2>*>(&x); }
	constexpr Vec<T,2> yz() const { return *reinterpret_cast<const Vec<T,2>*>(&y); }
	constexpr Vec<T,2> zw() const { return *reinterpret_cast<const Vec<T,2>*>(&z); }
	constexpr Vec<T,3> xyz() const { return *reinterpret_cast<const Vec<T,3>*>(&x); }
	constexpr Vec<T,3> yzw() const { return *reinterpret_cast<const Vec<T,3>*>(&y); }

	constexpr Vec() noexcept = default;
	constexpr Vec(const Vec<T,4>&) noexcept = default;
	constexpr Vec<T,4>& operator= (const Vec<T,4>&) noexcept = default;
	~Vec() noexcept = default;

	constexpr explicit Vec(const T* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]), w(ptr[3]) { }
	constexpr explicit Vec(T val) : x(val), y(val), z(val), w(val) { }
	constexpr Vec(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
	constexpr Vec(Vec<T,3> xyz, T w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) { }
	constexpr Vec(T x, Vec<T,3> yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z) { }
	constexpr Vec(Vec<T,2> xy, Vec<T,2> zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) { }
	constexpr Vec(Vec<T,2> xy, T z, T w) : x(xy.x), y(xy.y), z(z), w(w) { }
	constexpr Vec(T x, Vec<T,2> yz, T w) : x(x), y(yz.x), z(yz.y), w(w) { }
	constexpr Vec(T x, T y, Vec<T,2> zw) : x(x), y(y), z(zw.x), w(zw.y) { }

	constexpr Vec(f32x4 o) : x(T(o.x)), y(T(o.y)), z(T(o.z)), w(T(o.w)) {}
	constexpr operator f32x4() const { return f32x4(f32(x), f32(y), f32(z), f32(w)); }
	constexpr Vec(i32x4 o) : x(T(o.x)), y(T(o.y)), z(T(o.z)), w(T(o.w)) {}
	constexpr operator i32x4() const { return i32x4(i32(x), i32(y), i32(z), i32(w)); }

	template<typename T2>
	constexpr explicit Vec(Vec<T2,4> o) : x(T(o.x)), y(T(o.y)), z(T(o.z)), w(T(o.w)) { }

	constexpr T* data() noexcept { return &x; }
	constexpr const T* data() const noexcept { return &x; }
	constexpr T& operator[] (u32 idx) { sfz_assert(idx < 4); return data()[idx]; }
	constexpr T operator[] (u32 idx) const { sfz_assert(idx < 4); return data()[idx]; }

	constexpr Vec& operator+= (Vec o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr Vec& operator-= (Vec o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr Vec& operator*= (T s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr Vec& operator*= (Vec o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr Vec& operator/= (T s) { x /= s; y /= s; z /= s; w /= s; return *this; }
	constexpr Vec& operator/= (Vec o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }

	constexpr Vec operator+ (Vec o) const { return Vec(*this) += o; }
	constexpr Vec operator- (Vec o) const { return Vec(*this) -= o; }
	constexpr Vec operator- () const { return Vec(-x, -y, -z, -w); }
	constexpr Vec operator* (Vec o) const { return Vec(*this) *= o; }
	constexpr Vec operator* (T s) const { return Vec(*this) *= s; }
	constexpr Vec operator/ (Vec o) const { return Vec(*this) /= o; }
	constexpr Vec operator/ (T s) const { return Vec(*this) /= s; }

	constexpr bool operator== (Vec o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (Vec o) const { return !(*this == o); }
};

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

template<u32 H, u32 W>
struct Mat final {

	Vec<f32,W> rows[H];

	constexpr f32* data() { return rows[0].data(); }
	constexpr const f32* data() const { return rows[0].data(); }

	Vec<f32,W>& row(u32 y) { sfz_assert(y < H); return rows[y]; }
	const Vec<f32,W>& row(u32 y) const { sfz_assert(y < H); return rows[y]; }

	Vec<f32,H> column(u32 x) const
	{
		sfz_assert(x < W);
		Vec<f32,H> column;
		for (u32 y = 0; y < H; y++) column[y] = this->at(y, x);
		return column;
	}
	void setColumn(u32 x, Vec<f32,H> col) { for (u32 y = 0; y < H; y++) at(y, x) = col[y]; }

	constexpr f32& at(u32 y, u32 x) { return row(y)[x]; }
	constexpr f32 at(u32 y, u32 x) const { return row(y)[x]; }

	constexpr Mat() noexcept = default;
	constexpr Mat(const Mat&) noexcept = default;
	constexpr Mat& operator= (const Mat&) noexcept = default;
	~Mat() noexcept = default;

	constexpr explicit Mat(const f32* ptr) { for (u32 i = 0; i < (H * W); i++) data()[i] = ptr[i]; }

	constexpr Mat(f32 e00, f32 e01, f32 e10, f32 e11) : Mat(Vec<f32,2>(e00, e01), Vec<f32,2>(e10, e11)) {}
	constexpr Mat(Vec<f32,2> row0, Vec<f32,2> row1)
	{
		static_assert(H == 2 && W == 2, "Only for 2x2 matrices");
		row(0) = row0; row(1) = row1;
	}

	constexpr Mat(f32 e00, f32 e01, f32 e02, f32 e10, f32 e11, f32 e12, f32 e20, f32 e21, f32 e22)
		: Mat(Vec<f32,3>(e00, e01, e02), Vec<f32,3>(e10, e11, e12), Vec<f32,3>(e20, e21, e22)) {}
	constexpr Mat(Vec<f32,3> row0, Vec<f32,3> row1, Vec<f32,3> row2)
	{
		static_assert(H == 3 && W == 3, "Only for 3x3 matrices");
		row(0) = row0; row(1) = row1; row(2) = row2;
	}

	constexpr Mat(f32 e00, f32 e01, f32 e02, f32 e03, f32 e10, f32 e11, f32 e12, f32 e13, f32 e20, f32 e21, f32 e22, f32 e23)
		: Mat(Vec<f32,4>(e00, e01, e02, e03), Vec<f32,4>(e10, e11, e12, e13), Vec<f32,4>(e20, e21, e22, e23)) {}
	constexpr Mat(Vec<f32,4> row0, Vec<f32,4> row1, Vec<f32,4> row2)
	{
		static_assert(H == 3 && W == 4, "Only for 3x4 matrices");
		row(0) = row0; row(1) = row1; row(2) = row2;
	}

	constexpr Mat(f32 e00, f32 e01, f32 e02, f32 e03, f32 e10, f32 e11, f32 e12, f32 e13, f32 e20, f32 e21, f32 e22, f32 e23, f32 e30, f32 e31, f32 e32, f32 e33)
		: Mat(Vec<f32,4>(e00, e01, e02, e03), Vec<f32,4>(e10, e11, e12, e13), Vec<f32,4>(e20, e21, e22, e23), Vec<f32,4>(e30, e31, e32, e33)) {}
	constexpr Mat(Vec<f32,4> row0, Vec<f32,4> row1, Vec<f32,4> row2, Vec<f32,4> row3)
	{
		static_assert(H == 4 && W == 4, "Only for 4x4 matrices");
		row(0) = row0; row(1) = row1; row(2) = row2; row(3) = row3;
	}

	// Constructs Matrix from one of difference size. Will add "identity" matrix if target is
	// bigger, and remove components from source if target is smaller.
	template<u32 OH, u32 OW>
	constexpr explicit Mat(const Mat<OH,OW>& o)
	{
		*this = Mat::identity();
		for (u32 y = 0, commonHeight = min(H, OH); y < commonHeight; y++) {
			for (u32 x = 0, commonWidth = min(W, OW); x < commonWidth; x++) {
				this->at(y, x) = o.at(y, x);
			}
		}
	}

	static constexpr Mat fill(f32 v) { Mat m; for (u32 i = 0; i < (H * W); i++) m.data()[i] = v; return m; }

	static constexpr Mat identity()
	{
		static_assert(W >= H, "Can't create identity for tall matrices");
		Mat tmp;
		u32 oneIdx = 0;
		for (u32 rowIdx = 0; rowIdx < H; rowIdx++) {
			tmp.row(rowIdx) = Vec<f32,W>(f32(0));
			tmp.row(rowIdx)[oneIdx] = f32(1);
			oneIdx += 1;
		}
		return tmp;
	}

	static constexpr Mat scaling2(f32 x, f32 y)
	{
		static_assert(H == 2 && W == 2, "Only for 2x2 matrices");
		return Mat(x, f32(0), f32(0), y);
	}
	static constexpr Mat scaling2(Vec<f32, 2> scale) { return Mat::scaling2(scale.x, scale.y); }
	static constexpr Mat scaling2(f32 scale) { return scaling2(scale, scale); }

	static constexpr Mat scaling3(f32 x, f32 y, f32 z)
	{
		static_assert(H >= 3 && W >= 3, "Only for 3x3 matrices and larger");
		return Mat(Mat<3,3>(x, f32(0), f32(0), f32(0), y, f32(0), f32(0), f32(0), z));
	}
	static constexpr Mat scaling3(Vec<f32,3> scale) { return Mat::scaling3(scale.x, scale.y, scale.z); }
	static constexpr Mat scaling3(f32 scale) { return scaling3(scale, scale, scale); }

	static Mat rotation3(Vec<f32,3> axis, f32 angleRad)
	{
		static_assert(H >= 3 && W >= 3, "Only for 3x3 matrices and larger");
		Vec<f32,3> r = normalize(axis);
		f32 c = (f32)cos(angleRad);
		f32 s = (f32)sin(angleRad);
		f32 cm1 = f32(1) - c;
		// Matrix by Goldman, page 71 of Real-Time Rendering.
		return Mat(Mat<3,3>(
			c + cm1 * r.x * r.x, cm1 * r.x * r.y - r.z * s, cm1 * r.x * r.z + r.y * s,
			cm1 * r.x * r.y + r.z * s, c + cm1 * r.y * r.y, cm1 * r.y * r.z - r.x * s,
			cm1 * r.x * r.z - r.y * s, cm1 * r.y * r.z + r.x * s, c + cm1 * r.z * r.z));
	}

	static constexpr Mat translation3(Vec<f32,3> t)
	{
		static_assert(H >= 3 && W >= 4, "Only for 3x4 matrices and larger");
		return Mat(Mat<3,4>(f32(1), f32(0), f32(0), t.x, f32(0), f32(1), f32(0), t.y, f32(0), f32(0), f32(1), t.z));
	}

	constexpr Mat& operator+= (const Mat& o) { for (u32 y = 0; y < H; y++) { row(y) += o.row(y); } return *this; }
	constexpr Mat& operator-= (const Mat& o) { for (u32 y = 0; y < H; y++) { row(y) -= o.row(y); } return *this; }
	constexpr Mat& operator*= (f32 s) { for (u32 y = 0; y < H; y++) { row(y) *= s; } return *this; }
	constexpr Mat& operator*= (const Mat& o) { return (*this = *this * o);}

	constexpr Mat operator+ (const Mat& o) const { return (Mat(*this) += o); }
	constexpr Mat operator- (const Mat& o) const { return (Mat(*this) -= o); }
	constexpr Mat operator- () const { return (Mat(*this) *= f32(-1)); }
	constexpr Mat operator* (f32 rhs) const { return (Mat(*this) *= rhs); }

	constexpr Vec<f32,H> operator* (const Vec<f32,W>& v) const
	{
		Vec<f32,H> res = {};
		for (u32 y = 0; y < H; y++) {
			for (u32 x = 0; x < W; x++) {
				res[y] += row(y)[x] * v[x];
			}
		}
		return res;
	}

	constexpr bool operator== (const Mat& o) const
	{
		for (u32 y = 0; y < H; y++) if (row(y) != o.row(y)) return false;
		return true;
	}
	constexpr bool operator!= (const Mat& o) const { return !(*this == o); }
};

using mat22 = Mat<2,2>; static_assert(sizeof(mat22) == sizeof(f32) * 4, "");
using mat33 = Mat<3,3>; static_assert(sizeof(mat33) == sizeof(f32) * 9, "");
using mat34 = Mat<3,4>; static_assert(sizeof(mat34) == sizeof(f32) * 12, "");
using mat44 = Mat<4,4>; static_assert(sizeof(mat44) == sizeof(f32) * 16, "");

using mat2 = mat22;
using mat3 = mat33;
using mat4 = mat44;

template<u32 H, u32 W>
constexpr Mat<H,W> operator* (f32 lhs, const Mat<H,W>& rhs) { return rhs * lhs; }

template<u32 H, u32 S, u32 W>
constexpr Mat<H,W> operator* (const Mat<H,S>& lhs, const Mat<S,W>& rhs)
{
	Mat<H,W> res = {};
	for (u32 y = 0; y < H; y++) {
		for (u32 x = 0; x < W; x++) {
			for (u32 s = 0; s < S; s++) {
				res.at(y, x) += lhs.row(y)[s] * rhs.column(x)[s];
			}
		}
	}
	return res;
}

template<u32 H, u32 W>
constexpr Mat<H,W> elemMult(const Mat<H,W>& lhs, const Mat<H,W>& rhs)
{
	Mat<H,W> res;
	for (u32 y = 0; y < H; y++) res.row(y) = lhs.row(y) * rhs.row(y);
	return res;
}

template<u32 H, u32 W>
constexpr Mat<W,H> transpose(const Mat<H,W>& m)
{
	Mat<W,H> res;
	for (u32 y = 0; y < H; y++) {
		for (u32 x = 0; x < W; x++) {
			res.at(x, y) = m.at(y, x);
		}
	}
	return res;
}

constexpr f32x3 transformPoint(const mat34& m, f32x3 p) { return m * f32x4(p, 1.0f); }
constexpr f32x3 transformPoint(const mat44& m, f32x3 p) { f32x4 v = m * f32x4(p, 1.0f); return v.xyz() / v.w; }

constexpr f32x3 transformDir(const mat34& m, f32x3 d) { return m * f32x4(d, 0.0f); }
constexpr f32x3 transformDir(const mat44& m, f32x3 d) { return (m * f32x4(d, 0.0f)).xyz(); }

constexpr f32 determinant(const Mat<2,2>& m) { return m.at(0,0)*m.at(1,1)- m.at(0,1)*m.at(1,0); }

constexpr f32 determinant(const Mat<3,3>& m)
{
	Vec<f32,3> e0 = m.rows[0]; Vec<f32,3> e1 = m.rows[1]; Vec<f32,3> e2 = m.rows[2];
	return
		e0[0] * e1[1] * e2[2]
		+ e0[1] * e1[2] * e2[0]
		+ e0[2] * e1[0] * e2[1]
		- e0[2] * e1[1] * e2[0]
		- e0[1] * e1[0] * e2[2]
		- e0[0] * e1[2] * e2[1];
}

constexpr f32 determinant(const Mat<4,4>& m)
{
	Vec<f32,4> e0 = m.rows[0]; Vec<f32,4> e1 = m.rows[1]; Vec<f32,4> e2 = m.rows[2]; Vec<f32,4> e3 = m.rows[3];
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

constexpr Mat<2,2> inverse(const Mat<2,2>& m)
{
	const f32 det = determinant(m);
	if (det == f32(0)) return Mat<2,2>::fill(f32(0));

	Mat<2,2> tmp(
		m.row(1)[1], -m.row(0)[1],
		-m.row(1)[0], m.row(0)[0]);
	return (f32(1) / det) * tmp;
}

constexpr Mat<3,3> inverse(const Mat<3,3>& m)
{
	const f32 det = determinant(m);
	if (det == 0) return Mat<3,3>::fill(f32(0));

	Vec<f32,3> e0 = m.row(0); Vec<f32,3> e1 = m.row(1); Vec<f32,3> e2 = m.row(2);

	const f32 A =  (e1[1] * e2[2] - e1[2] * e2[1]);
	const f32 B = -(e1[0] * e2[2] - e1[2] * e2[0]);
	const f32 C =  (e1[0] * e2[1] - e1[1] * e2[0]);
	const f32 D = -(e0[1] * e2[2] - e0[2] * e2[1]);
	const f32 E =  (e0[0] * e2[2] - e0[2] * e2[0]);
	const f32 F = -(e0[0] * e2[1] - e0[1] * e2[0]);
	const f32 G =  (e0[1] * e1[2] - e0[2] * e1[1]);
	const f32 H = -(e0[0] * e1[2] - e0[2] * e1[0]);
	const f32 I =  (e0[0] * e1[1] - e0[1] * e1[0]);

	Mat<3,3> tmp(
		A, D, G,
		B, E, H,
		C, F, I);
	return (f32(1) / det) * tmp;
}

constexpr Mat<4,4> inverse(const Mat<4,4>& m)
{
	const f32 det = determinant(m);
	if (det == 0) return Mat<4,4>::fill(f32(0));

	const f32
		m00 = m.row(0)[0], m01 = m.row(0)[1], m02 = m.row(0)[2], m03 = m.row(0)[3],
		m10 = m.row(1)[0], m11 = m.row(1)[1], m12 = m.row(1)[2], m13 = m.row(1)[3],
		m20 = m.row(2)[0], m21 = m.row(2)[1], m22 = m.row(2)[2], m23 = m.row(2)[3],
		m30 = m.row(3)[0], m31 = m.row(3)[1], m32 = m.row(3)[2], m33 = m.row(3)[3];

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

	Mat<4,4> tmp(
		b00, b01, b02, b03,
		b10, b11, b12, b13,
		b20, b21, b22, b23,
		b30, b31, b32, b33);
	return (f32(1) / det) * tmp;
}

// Quaternion primitive
// ------------------------------------------------------------------------------------------------

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
		struct { f32 x, y, z, w; };
		struct { f32x3 v; };
		f32x4 vector;
	};

	Quat() noexcept = default;
	constexpr Quat(const Quat&) noexcept = default;
	constexpr Quat& operator= (const Quat&) noexcept = default;
	~Quat() noexcept = default;

	constexpr Quat(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) { }
	constexpr Quat(f32x3 v, f32 w) : x(v.x), y(v.y), z(v.z), w(w) { }

	// Creates an identity quaternion representing a non-rotation, i.e. [0, 0, 0, 1]
	static constexpr Quat identity() { return Quat(0.0f, 0.0f, 0.0f, 1.0f); }

	// Creates a unit quaternion representing a (right-handed) rotation around the specified axis.
	// The given axis will be automatically normalized.
	static Quat rotationDeg(f32x3 axis, f32 angleDeg) { return Quat::rotationRad(axis, angleDeg * DEG_TO_RAD); }
	static Quat rotationRad(f32x3 axis, f32 angleRad)
	{
		const f32 halfAngleRad = angleRad * 0.5f;
		const f32x3 normalizedAxis = normalize(axis);
		return Quat(sinf(halfAngleRad) * normalizedAxis, cosf(halfAngleRad));
	}

	// Constructs a Quaternion from Euler angles. The rotation around the z axis is performed first,
	// then y and last x axis.
	static Quat fromEuler(f32 xDeg, f32 yDeg, f32 zDeg)
	{
		const f32 DEG_ANGLE_TO_RAD_HALF_ANGLE = (f32(3.14159265358979323846f) / f32(180.0f)) / f32(2.0f);

		f32 cosZ = (f32)cos(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		f32 sinZ = (f32)sin(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		f32 cosY = (f32)cos(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		f32 sinY = (f32)sin(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		f32 cosX = (f32)cos(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		f32 sinX = (f32)sin(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);

		Quat tmp;
		tmp.x = cosZ * sinX * cosY - sinZ * cosX * sinY;
		tmp.y = cosZ * cosX * sinY + sinZ * sinX * cosY;
		tmp.z = sinZ * cosX * cosY - cosZ * sinX * sinY;
		tmp.w = cosZ * cosX * cosY + sinZ * sinX * sinY;
		return tmp;
	}
	static Quat fromEuler(f32x3 anglesDeg) { return Quat::fromEuler(anglesDeg.x, anglesDeg.y, anglesDeg.z); }

	static Quat fromRotationMatrix(const mat33& m)
	{
		// Algorithm from page 205 of Game Engine Architecture 2nd Edition
		const f32x3& e0 = m.row(0); const f32x3& e1 = m.row(1); const f32x3& e2 = m.row(2);
		f32 trace = e0[0] + e1[1] + e2[2];

		Quat tmp;

		// Check the diagonal
		if (trace > 0.0f) {
			f32 s = (f32)sqrt(trace + 1.0f);
			tmp.w = s * 0.5f;

			f32 t = 0.5f / s;
			tmp.x = (e2[1] - e1[2]) * t;
			tmp.y = (e0[2] - e2[0]) * t;
			tmp.z = (e1[0] - e0[1]) * t;
		}
		else {
			// Diagonal is negative
			i32 i = 0;
			if (e1[1] > e0[0]) i = 1;
			if (e2[2] > m.at(i, i)) i = 2;

			constexpr i32 NEXT[3] = { 1, 2, 0 };
			i32 j = NEXT[i];
			i32 k = NEXT[j];

			f32 s = (f32)sqrt((m.at(i, j) - (m.at(j, j) + m.at(k, k))) + 1.0f);
			tmp.vector[i] = s * 0.5f;

			f32 t = (s != 0.0f) ? (0.5f / s) : s;

			tmp.vector[3] = (m.at(k, j) - m.at(j, k)) * t;
			tmp.vector[j] = (m.at(j, i) + m.at(i, j)) * t;
			tmp.vector[k] = (m.at(k, i) + m.at(i, k)) * t;
		}

		return tmp;
	}
	static Quat fromRotationMatrix(const mat34& m) { return Quat::fromRotationMatrix(mat33(m)); }

	// Returns the normalized axis which the quaternion rotates around, returns 0 vector for
	// identity Quaternion. Includes a safeNormalize() call, not necessarily super fast.
	f32x3 rotationAxis() const { return normalizeSafe(this->v); }

	// Returns the angle (degrees) this Quaternion rotates around the rotationAxis().
	f32 rotationAngleDeg() const
	{
		const f32 RAD_ANGLE_TO_DEG_NON_HALF_ANGLE = (180.0f / 3.14159265358979323846f) * 2.0f;
		f32 halfAngleRad = (f32)acos(this->w);
		return halfAngleRad * RAD_ANGLE_TO_DEG_NON_HALF_ANGLE;
	}

	// Returns a Euler angle (degrees) representation of this Quaternion. Assumes the Quaternion
	// is unit.
	f32x3 toEuler() const
	{
		const f32 RAD_ANGLE_TO_DEG = 180.0f / 3.14159265358979323846f;
		f32x3 tmp;
		tmp.x = atan2f(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y)) * RAD_ANGLE_TO_DEG;
		tmp.y = asinf(sfz::min(sfz::max(2.0f * (w * y - z * x), -1.0f), 1.0f)) * RAD_ANGLE_TO_DEG;
		tmp.z = atan2f(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z)) * RAD_ANGLE_TO_DEG;
		return tmp;
	}

	// Converts the given Quaternion into a Matrix. The normal variants assume that the Quaternion
	// is unit, while the "non-unit" variants make no such assumptions.
	constexpr mat33 toMat33() const
	{
		// Algorithm from Real-Time Rendering, page 76
		return mat33(
			1.0f - 2.0f*(y*y + z*z),  2.0f*(x*y - w*z),         2.0f*(x*z + w*y),
			2.0f*(x*y + w*z),         1.0f - 2.0f*(x*x + z*z),  2.0f*(y*z - w*x),
			2.0f*(x*z - w*y),         2.0f*(y*z + w*x),         1.0f - 2.0f*(x*x + y*y));
	}
	constexpr mat34 toMat34() const { return mat34(toMat33()); }
	constexpr mat44 toMat44() const { return mat44(toMat33()); }

	mat33 toMat33NonUnit() const
	{
		// Algorithm from Real-Time Rendering, page 76
		f32 s = 2.0f / length(vector);
		return mat33(
			1.0f - s*(y*y + z*z),  s*(x*y - w*z),         s*(x*z + w*y),
			s*(x*y + w*z),         1.0f - s*(x*x + z*z),  s*(y*z - w*x),
			s*(x*z - w*y),         s*(y*z + w*x),         1.0f - s*(x*x + y*y));
	}
	mat34 toMat34NonUnit() const { return mat34(toMat33NonUnit()); }
	mat44 toMat44NonUnit() const { return mat44(toMat33NonUnit()); }

	constexpr Quat& operator+= (Quat o) { this->vector += o.vector; return *this; }
	constexpr Quat& operator-= (Quat o) { this->vector -= o.vector; return *this; }
	constexpr Quat& operator*= (Quat o)
	{
		Quat tmp = {};
		tmp.v = cross(v, o.v) + o.w * v + w * o.v;
		tmp.w = w * o.w - dot(v, o.v);
		*this = tmp;
		return *this;
	}
	constexpr Quat& operator*= (f32 scalar) { vector *= scalar; return *this; }

	constexpr Quat operator+ (Quat o) const { return Quat(*this) += o; }
	constexpr Quat operator- (Quat o) const { return Quat(*this) -= o; }
	constexpr Quat operator* (Quat o) const { return Quat(*this) *= o; }
	constexpr Quat operator* (f32 scalar) const { return Quat(*this) *= scalar; }

	constexpr bool operator== (Quat o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (Quat o) const { return !(*this == o); }
};

static_assert(sizeof(Quat) == sizeof(f32x4), "");

constexpr Quat operator* (f32 scalar, Quat q) { return q *= scalar; }

// Calculates the length (norm) of the Quaternion. A unit Quaternion has length 1. If the
// Quaternions are used for rotations they should always be unit.
inline f32 length(Quat q) { return length(q.vector); }

// Normalizes the Quaternion into a unit Quaternion by dividing each component by the length.
inline Quat normalize(Quat q) { Quat tmp; tmp.vector = normalize(q.vector); return tmp; }

// Calculates the conjugate quaternion, i.e. [-v, w]. If the quaternion is unit length this is
// the same as the inverse.
constexpr Quat conjugate(Quat q) { return Quat(-q.v, q.w); }

// Calculates the inverse for any Quaternion, i.e. (1 / length(q)²) * conjugate(q). For unit
// Quaternions (which should be the most common case) the conjugate() function should be used
// instead as it is way faster.
constexpr Quat inverse(Quat q) { return (1.0f / dot(q.vector, q.vector)) * conjugate(q); }

// Rotates a vector with the specified Quaternion, using q * v * qInv. Either the inverse can
// be specified manually, or it can be calculated automatically from the given Quaternion. When
// it is calculated automatically it is assumed that the Quaternion is unit, so the inverse is
// the conjugate.
constexpr f32x3 rotate(Quat q, f32x3 v, Quat qInv)
{
	Quat tmp(v, 0.0f);
	tmp = q * tmp * qInv;
	return tmp.v;
}

constexpr f32x3 rotate(Quat q, f32x3 v) { return rotate(q, v, conjugate(q)); }

inline Quat lerp(Quat q0, Quat q1, f32 t)
{
	Quat tmp;
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

inline f32x3 rotateTowardsRad(f32x3 inDir, f32x3 targetDir, f32 angleRads)
{
	sfz_assert(eqf(length(inDir), 1.0f));
	sfz_assert(eqf(length(targetDir), 1.0f));
	sfz_assert(dot(inDir, targetDir) >= -0.9999f);
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < PI);
	f32x3 axis = cross(inDir, targetDir);
	sfz_assert(!eqf(axis, f32x3(0.0f)));
	Quat rotQuat = Quat::rotationRad(axis, angleRads);
	f32x3 newDir = rotate(rotQuat, inDir);
	return newDir;
}

inline f32x3 rotateTowardsRadClampSafe(f32x3 inDir, f32x3 targetDir, f32 angleRads)
{
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < PI);

	f32x3 inDirNorm = normalizeSafe(inDir);
	f32x3 targetDirNorm = normalizeSafe(targetDir);
	sfz_assert(!eqf(inDirNorm, f32x3(0.0f)));
	sfz_assert(!eqf(targetDirNorm, f32x3(0.0f)));

	// Case where vectors are the same, just return the target dir
	if (eqf(inDirNorm, targetDirNorm)) return targetDirNorm;

	// Case where vectors are exact opposite, slightly nudge input a bit
	if (eqf(inDirNorm, -targetDirNorm)) {
		inDirNorm = normalize(inDir + (f32x3(1.0f) - inDirNorm) * 0.025f);
		sfz_assert(!eqf(inDirNorm, -targetDirNorm));
	}

	// Case where angle is larger than the angle between the vectors
	if (angleRads >= acos(dot(inDirNorm, targetDirNorm))) return targetDirNorm;

	// At this point all annoying cases should be handled, just run the normal routine
	return rotateTowardsRad(inDirNorm, targetDirNorm, angleRads);
}

inline f32x3 rotateTowardsDeg(f32x3 inDir, f32x3 targetDir, f32 angleDegs)
{
	return rotateTowardsRad(inDir, targetDir, DEG_TO_RAD * angleDegs);
}

inline f32x3 rotateTowardsDegClampSafe(f32x3 inDir, f32x3 targetDir, f32 angleDegs)
{
	return rotateTowardsRadClampSafe(inDir, targetDir, DEG_TO_RAD * angleDegs);
}

} // namespace sfz

#endif
