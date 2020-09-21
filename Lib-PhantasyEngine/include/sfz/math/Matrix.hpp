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

#pragma once

#include <skipifzero.hpp>

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
struct Matrix final {

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

	constexpr Matrix() noexcept = default;
	constexpr Matrix(const Matrix&) noexcept = default;
	constexpr Matrix& operator= (const Matrix&) noexcept = default;
	~Matrix() noexcept = default;

	constexpr explicit Matrix(const T* ptr) { for (uint32_t i = 0; i < (H * W); i++) data()[i] = ptr[i]; }

	constexpr Matrix(T e00, T e01, T e10, T e11) : Matrix(Vec<T,2>(e00, e01), Vec<T,2>(e10, e11)) {}
	constexpr Matrix(Vec<T,2> row0, Vec<T,2> row1)
	{
		static_assert(H == 2 && W == 2, "Only for 2x2 matrices");
		row(0) = row0; row(1) = row1;
	}

	constexpr Matrix(T e00, T e01, T e02, T e10, T e11, T e12, T e20, T e21, T e22)
		: Matrix(Vec<T,3>(e00, e01, e02), Vec<T,3>(e10, e11, e12), Vec<T,3>(e20, e21, e22)) {}
	constexpr Matrix(Vec<T,3> row0, Vec<T,3> row1, Vec<T,3> row2)
	{
		static_assert(H == 3 && W == 3, "Only for 3x3 matrices");
		row(0) = row0; row(1) = row1; row(2) = row2;
	}

	constexpr Matrix(T e00, T e01, T e02, T e03, T e10, T e11, T e12, T e13, T e20, T e21, T e22, T e23)
		: Matrix(Vec<T,4>(e00, e01, e02, e03), Vec<T,4>(e10, e11, e12, e13), Vec<T,4>(e20, e21, e22, e23)) {}
	constexpr Matrix(Vec<T,4> row0, Vec<T,4> row1, Vec<T,4> row2)
	{
		static_assert(H == 3 && W == 4, "Only for 3x4 matrices");
		row(0) = row0; row(1) = row1; row(2) = row2;
	}

	constexpr Matrix(T e00, T e01, T e02, T e03, T e10, T e11, T e12, T e13, T e20, T e21, T e22, T e23, T e30, T e31, T e32, T e33)
		: Matrix(Vec<T,4>(e00, e01, e02, e03), Vec<T,4>(e10, e11, e12, e13), Vec<T,4>(e20, e21, e22, e23), Vec<T,4>(e30, e31, e32, e33)) {}
	constexpr Matrix(Vec<T,4> row0, Vec<T,4> row1, Vec<T,4> row2, Vec<T,4> row3)
	{
		static_assert(H == 4 && W == 4, "Only for 4x4 matrices");
		row(0) = row0; row(1) = row1; row(2) = row2; row(3) = row3;
	}

	// Constructs Matrix from one of difference size. Will add "identity" matrix if target is
	// bigger, and remove components from source if target is smaller.
	template<uint32_t OH, uint32_t OW>
	constexpr explicit Matrix(const Matrix<T,OH,OW>& o)
	{
		*this = Matrix::identity();
		for (uint32_t y = 0, commonHeight = min(H, OH); y < commonHeight; y++) {
			for (uint32_t x = 0, commonWidth = min(W, OW); x < commonWidth; x++) {
				this->at(y, x) = o.at(y, x);
			}
		}
	}

	static constexpr Matrix fill(T v) { Matrix m; for (uint32_t i = 0; i < (H * W); i++) m.data()[i] = v; return m; }

	static constexpr Matrix identity()
	{
		static_assert(W >= H, "Can't create identity for tall matrices");
		Matrix tmp;
		uint32_t oneIdx = 0;
		for (uint32_t rowIdx = 0; rowIdx < H; rowIdx++) {
			tmp.row(rowIdx) = Vec<T,W>(T(0));
			tmp.row(rowIdx)[oneIdx] = T(1);
			oneIdx += 1;
		}
		return tmp;
	}

	static constexpr Matrix scaling2(T x, T y)
	{
		static_assert(H == 2 && W == 2, "Only for 2x2 matrices");
		return Matrix(x, T(0), T(0), y);
	}
	static constexpr Matrix scaling2(Vec<T, 2> scale) { return Matrix::scaling2(scale.x, scale.y); }
	static constexpr Matrix scaling2(T scale) { return scaling2(scale, scale); }

	static constexpr Matrix scaling3(T x, T y, T z)
	{
		static_assert(H >= 3 && W >= 3, "Only for 3x3 matrices and larger");
		return Matrix(Matrix<T,3,3>(x, T(0), T(0), T(0), y, T(0), T(0), T(0), z));
	}
	static constexpr Matrix scaling3(Vec<T,3> scale) { return Matrix::scaling3(scale.x, scale.y, scale.z); }
	static constexpr Matrix scaling3(T scale) { return scaling3(scale, scale, scale); }

	static Matrix rotation3(Vec<T,3> axis, T angleRad)
	{
		static_assert(H >= 3 && W >= 3, "Only for 3x3 matrices and larger");
		Vec<T, 3> r = normalize(axis);
		T x = r.x; T y = r.y; T z = r.z;
		T c = cos(angleRad);
		T s = sin(angleRad);
		T cm1 = T(1) - c;
		// Matrix by Goldman, page 71 of Real-Time Rendering.
		return Matrix(Matrix<T,3,3>(
			c + cm1 * x * x, cm1 * x * y - z * s, cm1 * x * z + y * s,
			cm1 * x * y + z * s, c + cm1 * y * y, cm1 * y * z - x * s,
			cm1 * x * z - y * s, cm1 * y * z + x * s, c + cm1 * z * z));
	}

	static constexpr Matrix translation3(Vec<T,3> delta)
	{
		static_assert(H >= 3 && W >= 4, "Only for 3x4 matrices and larger");
		return Matrix(Matrix<T,3,4>(
			T(1), T(0), T(0), delta.x,
			T(0), T(1), T(0), delta.y,
			T(0), T(0), T(1), delta.z));
	}

	constexpr Matrix& operator+= (const Matrix& o)
	{
		for (uint32_t y = 0; y < H; y++) row(y) += o.row(y);
		return *this;
	}

	constexpr Matrix& operator-= (const Matrix& o)
	{
		for (uint32_t y = 0; y < H; y++) row(y) -= o.row(y);
		return *this;
	}

	constexpr Matrix& operator*= (T s)
	{
		for (uint32_t y = 0; y < H; y++) row(y) *= s;
		return *this;
	}

	constexpr Matrix& operator*= (const Matrix& o) { return (*this = *this * o);}

	constexpr Matrix operator+ (const Matrix& o) const { return (Matrix(*this) += o); }
	constexpr Matrix operator- (const Matrix& o) const { return (Matrix(*this) -= o); }
	constexpr Matrix operator- () const { return (Matrix(*this) *= T(-1)); }
	constexpr Matrix operator* (T rhs) const { return (Matrix(*this) *= rhs); }

	constexpr Vec<T,H> operator* (const Vec<T,W>& v) const
	{
		Vec<T,H> res;
		for (uint32_t y = 0; y < H; y++) res[y] = dot(row(y), v);
		return res;
	}

	constexpr bool operator== (const Matrix& o) const
	{
		for (uint32_t y = 0; y < H; y++) if (row(y) != o.row(y)) return false;
		return true;
	}
	constexpr bool operator!= (const Matrix& o) const { return !(*this == o); }
};

using mat22 = Matrix<float,2,2>; static_assert(sizeof(mat22) == sizeof(float) * 4, "");
using mat33 = Matrix<float,3,3>; static_assert(sizeof(mat33) == sizeof(float) * 9, "");
using mat34 = Matrix<float,3,4>; static_assert(sizeof(mat34) == sizeof(float) * 12, "");
using mat44 = Matrix<float,4,4>; static_assert(sizeof(mat44) == sizeof(float) * 16, "");

using mat2 = mat22;
using mat3 = mat33;
using mat4 = mat44;

template<typename T, uint32_t H, uint32_t W>
constexpr Matrix<T,H,W> operator* (T lhs, const Matrix<T,H,W>& rhs) { return rhs * lhs; }

template<typename T, uint32_t H, uint32_t S, uint32_t W>
constexpr Matrix<T,H,W> operator* (const Matrix<T,H,S>& lhs, const Matrix<T,S,W>& rhs)
{
	Matrix<T,H,W> res;
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			res.at(y, x) = dot(lhs.row(y), rhs.column(x));
		}
	}
	return res;
}

template<typename T, uint32_t H, uint32_t W>
constexpr Matrix<T,H,W> elemMult(const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs)
{
	Matrix<T,H,W> result;
	for (uint32_t y = 0; y < H; y++) result.row(y) = lhs.row(y) * rhs.row(y);
	return result;
}

template<typename T, uint32_t H, uint32_t W>
constexpr Matrix<T,W,H> transpose(const Matrix<T,H,W>& m)
{
	Matrix<T,W,H> result;
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			result.at(x, y) = m.at(y, x);
		}
	}
	return result;
}

template<typename T>
constexpr Vec<T,3> transformPoint(const Matrix<T,3,4>& m, const Vec<T,3>& p)
{
	Vec<T,4> v(p, T(1));
	return m * v;
}

template<typename T>
constexpr Vec<T,3> transformPoint(const Matrix<T,4,4>& m, const Vec<T,3>& p)
{
	Vec<T,4> v(p, T(1));
	v = m * v;
	return v.xyz / v.w;
}

template<typename T>
constexpr Vec<T,3> transformDir(const Matrix<T,3,4>& m, const Vec<T,3>& d)
{
	Vec<T,4> v(d, T(0));
	return m * v;
}

template<typename T>
constexpr Vec<T,3> transformDir(const Matrix<T,4,4>& m, const Vec<T,3>& d)
{
	Vec<T,4> v(d, T(0));
	v = m * v;
	return v.xyz;
}

template<typename T>
constexpr T determinant(const Matrix<T,2,2>& m) { return m.at(0,0)*m.at(1,1)- m.at(0,1)*m.at(1,0); }

template<typename T>
constexpr T determinant(const Matrix<T,3,3>& m)
{
	const Vec<T,3>& e0 = m.row(0);
	const Vec<T,3>& e1 = m.row(1);
	const Vec<T,3>& e2 = m.row(2);
	return
		e0[0] * e1[1] * e2[2]
		+ e0[1] * e1[2] * e2[0]
		+ e0[2] * e1[0] * e2[1]
		- e0[2] * e1[1] * e2[0]
		- e0[1] * e1[0] * e2[2]
		- e0[0] * e1[2] * e2[1];
}

template<typename T>
constexpr T determinant(const Matrix<T,4,4>& m)
{
	const Vec<T,4>& e0 = m.row(0);
	const Vec<T,4>& e1 = m.row(1);
	const Vec<T,4>& e2 = m.row(2);
	const Vec<T,4>& e3 = m.row(3);
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
constexpr Matrix<T,2,2> inverse(const Matrix<T,2,2>& m)
{
	const T det = determinant(m);
	if (det == T(0)) return Matrix<T,2,2>::fill(T(0));

	Matrix<T,2,2> tmp(
		m.row(1)[1], -m.row(0)[1],
		-m.row(1)[0], m.row(0)[0]);
	return (T(1) / det) * tmp;
}

template<typename T>
constexpr Matrix<T,3,3> inverse(const Matrix<T,3,3>& m)
{
	const T det = determinant(m);
	if (det == 0) return Matrix<T,3,3>::fill(T(0));

	const Vec<T,3>& e0 = m.row(0);
	const Vec<T,3>& e1 = m.row(1);
	const Vec<T,3>& e2 = m.row(2);

	const T A =  (e1[1] * e2[2] - e1[2] * e2[1]);
	const T B = -(e1[0] * e2[2] - e1[2] * e2[0]);
	const T C =  (e1[0] * e2[1] - e1[1] * e2[0]);
	const T D = -(e0[1] * e2[2] - e0[2] * e2[1]);
	const T E =  (e0[0] * e2[2] - e0[2] * e2[0]);
	const T F = -(e0[0] * e2[1] - e0[1] * e2[0]);
	const T G =  (e0[1] * e1[2] - e0[2] * e1[1]);
	const T H = -(e0[0] * e1[2] - e0[2] * e1[0]);
	const T I =  (e0[0] * e1[1] - e0[1] * e1[0]);

	Matrix<T,3,3> tmp(
		A, D, G,
		B, E, H,
		C, F, I);
	return (T(1) / det) * tmp;
}

template<typename T>
constexpr Matrix<T,4,4> inverse(const Matrix<T,4,4>& m)
{
	const T det = determinant(m);
	if (det == 0) return Matrix<T,4,4>::fill(T(0));

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

	Matrix<T,4,4> tmp(
		b00, b01, b02, b03,
		b10, b11, b12, b13,
		b20, b21, b22, b23,
		b30, b31, b32, b33);
	return (T(1) / det) * tmp;
}

} // namespace sfz
