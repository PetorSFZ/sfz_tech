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

#include <cstdint>

#include "sfz/Assert.hpp"
#include "sfz/math/Vector.hpp"

#include "sfz/CudaCompatibility.hpp"
#include "sfz/SimdIntrinsics.hpp"

namespace sfz {

using std::int32_t;
using std::uint32_t;

// Matrix struct declaration
// ------------------------------------------------------------------------------------------------

/// A mathematical Matrix POD class that imitates a built-in primitive.
///
/// Uses row-major memory storage. Assumes vectors are column matrices, i.e. normal standard math
/// syntax. When uploading to OpenGL it needs to be transposed as OpenGL uses column-major storage.
/// OpenGL also assumes that vectors are column matrices, so only the storage layout is different.
/// This should not be confused with Direct3D which often assumes that vectors are row matrices.
/// When two indices are used the first one is always used to specify row (i.e. y-direction) and
/// the second one is used to specify column (i.e. x-direction).
///
/// The template is designed to be used with 32-bit floating points, other types may work, but no
/// guarantees are given.
///
/// Satisfies the conditions of std::is_pod, std::is_trivial and std::is_standard_layout if used
/// with float.
///
/// \param T the element type (float)
/// \param H the height (i.e. the amount of rows) of the Matrix
/// \param W the width (i.e. the amount of columns) of the Matrix

template<typename T, uint32_t H, uint32_t W>
struct Matrix final {
	
	Vector<T,W> rows[H];

	SFZ_CUDA_CALL T* data() noexcept { return &rows[0][0]; }
	SFZ_CUDA_CALL const T* data() const noexcept { return &rows[0][0]; }

	Matrix() noexcept = default;
	Matrix(const Matrix<T,H,W>&) noexcept = default;
	Matrix<T,H,W>& operator= (const Matrix<T,H,W>&) noexcept = default;
	~Matrix() noexcept = default;

	/// Constructs a matrix with the elements in an array (assumes array is in row-major order)
	SFZ_CUDA_CALL explicit Matrix(const T* arrayPtr) noexcept;

	SFZ_CUDA_CALL T& at(uint32_t y, uint32_t x) noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL T at(uint32_t y, uint32_t x) const noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL Vector<T,H> columnAt(uint32_t x) const noexcept;

	SFZ_CUDA_CALL void set(uint32_t y, uint32_t x, T value) noexcept { this->at(y, x) = value; }
	SFZ_CUDA_CALL void setColumn(uint32_t x, Vector<T,H> column) noexcept;
};

template<typename T>
struct Matrix<T,2,2> final {

	union {
		struct { Vector<T,2> rows[2]; };
		struct { Vector<T,2> row0, row1; };
		struct { T e00, e01,
		           e10, e11; };
	};

	SFZ_CUDA_CALL T* data() noexcept { return &e00; }
	SFZ_CUDA_CALL const T* data() const noexcept { return &e00; }

	Matrix() noexcept = default;
	Matrix(const Matrix<T,2,2>&) noexcept = default;
	Matrix<T,2,2>& operator= (const Matrix<T,2,2>&) noexcept = default;
	~Matrix() noexcept = default;

	/// Constructs a matrix with the elements in an array (assumes array is in row-major order)
	SFZ_CUDA_CALL explicit Matrix(const T* arrayPtr) noexcept;

	SFZ_CUDA_CALL Matrix(T e00, T e01,
	                     T e10, T e11) noexcept;

	SFZ_CUDA_CALL Matrix(Vector<T,2> row0,
	                     Vector<T,2> row1) noexcept;

	static SFZ_CUDA_CALL Matrix fill(T value) noexcept;
	static SFZ_CUDA_CALL Matrix identity() noexcept;
	static SFZ_CUDA_CALL Matrix scaling2(T scale) noexcept;
	static SFZ_CUDA_CALL Matrix scaling2(T x, T y) noexcept;
	static SFZ_CUDA_CALL Matrix scaling2(Vector<T,2> scale) noexcept;

	SFZ_CUDA_CALL T& at(uint32_t y, uint32_t x) noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL T at(uint32_t y, uint32_t x) const noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL Vector<T,2> columnAt(uint32_t x) const noexcept;

	SFZ_CUDA_CALL void set(uint32_t y, uint32_t x, T value) noexcept { this->at(y, x) = value; }
	SFZ_CUDA_CALL void setColumn(uint32_t x, Vector<T,2> column) noexcept;
};

template<typename T>
struct Matrix<T,3,3> final {

	union {
		struct { Vector<T,3> rows[3]; };
		struct { Vector<T,3> row0, row1, row2; };
		struct { T e00, e01, e02,
		           e10, e11, e12,
		           e20, e21, e22; };
	};

	SFZ_CUDA_CALL T* data() noexcept { return &e00; }
	SFZ_CUDA_CALL const T* data() const noexcept { return &e00; }

	Matrix() noexcept = default;
	Matrix(const Matrix<T,3,3>&) noexcept = default;
	Matrix<T,3,3>& operator= (const Matrix<T,3,3>&) noexcept = default;
	~Matrix() noexcept = default;

	/// Constructs a matrix with the elements in an array (assumes array is in row-major order)
	SFZ_CUDA_CALL explicit Matrix(const T* arrayPtr) noexcept;

	SFZ_CUDA_CALL Matrix(T e00, T e01, T e02,
	                     T e10, T e11, T e12,
	                     T e20, T e21, T e22) noexcept;

	SFZ_CUDA_CALL Matrix(Vector<T,3> row0,
	                     Vector<T,3> row1,
	                     Vector<T,3> row2) noexcept;

	/// Constructs a 3x3 matrix by removing elements from the specified matrix
	SFZ_CUDA_CALL explicit Matrix(const Matrix<T,3,4>& matrix) noexcept;
	SFZ_CUDA_CALL explicit Matrix(const Matrix<T,4,4>& matrix) noexcept;

	static SFZ_CUDA_CALL Matrix fill(T value) noexcept;
	static SFZ_CUDA_CALL Matrix identity() noexcept;
	static SFZ_CUDA_CALL Matrix scaling3(T scale) noexcept;
	static SFZ_CUDA_CALL Matrix scaling3(T x, T y, T z) noexcept;
	static SFZ_CUDA_CALL Matrix scaling3(Vector<T,3> scale) noexcept;
	static SFZ_CUDA_CALL Matrix rotation3(Vector<T,3> axis, T angleRad) noexcept;

	SFZ_CUDA_CALL T& at(uint32_t y, uint32_t x) noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL T at(uint32_t y, uint32_t x) const noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL Vector<T,3> columnAt(uint32_t x) const noexcept;

	SFZ_CUDA_CALL void set(uint32_t y, uint32_t x, T value) noexcept { this->at(y, x) = value; }
	SFZ_CUDA_CALL void setColumn(uint32_t x, Vector<T,3> column) noexcept;
};

template<typename T>
struct alignas(16) Matrix<T,3,4> final {

	union {
		struct { Vector<T,4> rows[3]; };
		struct { Vector<T,4> row0, row1, row2; };
		struct { T e00, e01, e02, e03,
		           e10, e11, e12, e13,
		           e20, e21, e22, e23; };
	};

	SFZ_CUDA_CALL T* data() noexcept { return &e00; }
	SFZ_CUDA_CALL const T* data() const noexcept { return &e00; }

	Matrix() noexcept = default;
	Matrix(const Matrix<T,3,4>&) noexcept = default;
	Matrix<T,3,4>& operator= (const Matrix<T,3,4>&) noexcept = default;
	~Matrix() noexcept = default;

	/// Constructs a matrix with the elements in an array (assumes array is in row-major order)
	SFZ_CUDA_CALL explicit Matrix(const T* arrayPtr) noexcept;

	SFZ_CUDA_CALL Matrix(T e00, T e01, T e02, T e03,
	                     T e10, T e11, T e12, T e13,
	                     T e20, T e21, T e22, T e23) noexcept;

	SFZ_CUDA_CALL Matrix(Vector<T,4> row0,
	                     Vector<T,4> row1,
	                     Vector<T,4> row2) noexcept;

	/// Constructs a 3x4 matrix by placing the specified matrix ontop a 3x4 identity-like matrix.
	SFZ_CUDA_CALL explicit Matrix(const Matrix<T,3,3>& matrix) noexcept;
	SFZ_CUDA_CALL explicit Matrix(const Matrix<T,4,4>& matrix) noexcept;

	static SFZ_CUDA_CALL Matrix fill(T value) noexcept;
	static SFZ_CUDA_CALL Matrix identity() noexcept; // Identity-like, identity does not exist for 3x4.
	static SFZ_CUDA_CALL Matrix scaling3(T scale) noexcept;
	static SFZ_CUDA_CALL Matrix scaling3(T x, T y, T z) noexcept;
	static SFZ_CUDA_CALL Matrix scaling3(Vector<T,3> scale) noexcept;
	static SFZ_CUDA_CALL Matrix rotation3(Vector<T,3> axis, T angleRad) noexcept;
	static SFZ_CUDA_CALL Matrix translation3(Vector<T,3> delta) noexcept;

	SFZ_CUDA_CALL T& at(uint32_t y, uint32_t x) noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL T at(uint32_t y, uint32_t x) const noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL Vector<T,3> columnAt(uint32_t x) const noexcept;

	SFZ_CUDA_CALL void set(uint32_t y, uint32_t x, T value) noexcept { this->at(y, x) = value; }
	SFZ_CUDA_CALL void setColumn(uint32_t x, Vector<T,3> column) noexcept;
};

template<typename T>
struct alignas(16) Matrix<T,4,4> final {

	union {
		struct { Vector<T,4> rows[4]; };
		struct { Vector<T,4> row0, row1, row2, row3; };
		struct { T e00, e01, e02, e03,
		           e10, e11, e12, e13,
		           e20, e21, e22, e23,
		           e30, e31, e32, e33; };
	};

	SFZ_CUDA_CALL T* data() noexcept { return &e00; }
	SFZ_CUDA_CALL const T* data() const noexcept { return &e00; }

	Matrix() noexcept = default;
	Matrix(const Matrix<T,4,4>&) noexcept = default;
	Matrix<T,4,4>& operator= (const Matrix<T,4,4>&) noexcept = default;
	~Matrix() noexcept = default;

	/// Constructs a matrix with the elements in an array (assumes array is in row-major order)
	SFZ_CUDA_CALL explicit Matrix(const T* arrayPtr) noexcept;

	SFZ_CUDA_CALL Matrix(T e00, T e01, T e02, T e03,
	                     T e10, T e11, T e12, T e13,
	                     T e20, T e21, T e22, T e23,
	                     T e30, T e31, T e32, T e33) noexcept;

	SFZ_CUDA_CALL Matrix(Vector<T,4> row0,
	                     Vector<T,4> row1,
	                     Vector<T,4> row2,
	                     Vector<T,4> row3) noexcept;

	/// Constructs a 4x4 matrix by placing the specified matrix ontop a 4x4 identity matrix.
	SFZ_CUDA_CALL explicit Matrix(const Matrix<T,3,3>& matrix) noexcept;
	SFZ_CUDA_CALL explicit Matrix(const Matrix<T,3,4>& matrix) noexcept;

	static SFZ_CUDA_CALL Matrix fill(T value) noexcept;
	static SFZ_CUDA_CALL Matrix identity() noexcept;
	static SFZ_CUDA_CALL Matrix scaling3(T scale) noexcept; // Note that the bottom right corner is 1 for 4x4
	static SFZ_CUDA_CALL Matrix scaling3(T x, T y, T z) noexcept; // Note that the bottom right corner is 1 for 4x4
	static SFZ_CUDA_CALL Matrix scaling3(Vector<T,3> scale) noexcept; // Note that the bottom right corner is 1 for 4x4
	static SFZ_CUDA_CALL Matrix rotation3(Vector<T,3> axis, T angleRad) noexcept;
	static SFZ_CUDA_CALL Matrix translation3(Vector<T,3> delta) noexcept;

	SFZ_CUDA_CALL T& at(uint32_t y, uint32_t x) noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL T at(uint32_t y, uint32_t x) const noexcept { return rows[y][x]; }
	SFZ_CUDA_CALL Vector<T,4> columnAt(uint32_t x) const noexcept;

	SFZ_CUDA_CALL void set(uint32_t y, uint32_t x, T value) noexcept { this->at(y, x) = value; }
	SFZ_CUDA_CALL void setColumn(uint32_t x, Vector<T,4> column) noexcept;
};

using mat22 = Matrix<float,2,2>;
using mat33 = Matrix<float,3,3>;
using mat34 = Matrix<float,3,4>;
using mat44 = Matrix<float,4,4>;

using mat2 = mat22;
using mat3 = mat33;
using mat4 = mat44;

static_assert(sizeof(mat22) == sizeof(float) * 2 * 2, "mat22 is padded");
static_assert(sizeof(mat33) == sizeof(float) * 3 * 3, "mat33 is padded");
static_assert(sizeof(mat34) == sizeof(float) * 3 * 4, "mat34 is padded");
static_assert(sizeof(mat44) == sizeof(float) * 4 * 4, "mat44 is padded");

// Matrix functions
// ------------------------------------------------------------------------------------------------

/// Element-wise multiplication of two matrices
template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> elemMult(const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept;

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,W,H> transpose(const Matrix<T,H,W>& m) noexcept;

template<typename T>
SFZ_CUDA_CALL Vector<T,3> transformPoint(const Matrix<T,3,4>& m, const Vector<T,3>& p) noexcept;

template<typename T>
SFZ_CUDA_CALL Vector<T,3> transformPoint(const Matrix<T,4,4>& m, const Vector<T,3>& p) noexcept;

template<typename T>
SFZ_CUDA_CALL Vector<T,3> transformDir(const Matrix<T,3,4>& m, const Vector<T,3>& d) noexcept;

template<typename T>
SFZ_CUDA_CALL Vector<T,3> transformDir(const Matrix<T,4,4>& m, const Vector<T,3>& d) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL T determinant(const Matrix<T,N,N>& m) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Matrix<T,N,N> inverse(const Matrix<T,N,N>& m) noexcept;

// Operators (arithmetic & assignment)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W>& operator+= (Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept;

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W>& operator-= (Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept;

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W>& operator*= (Matrix<T,H,W>& lhs, T rhs) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Matrix<T,N,N>& operator*= (Matrix<T,N,N>& lhs, const Matrix<T,N,N>& rhs) noexcept;

// Operators (arithmetic)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator+ (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept;

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator- (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept;

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator- (const Matrix<T,H,W>& matrix) noexcept;

template<typename T, uint32_t H, uint32_t S, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator* (const Matrix<T,H,S>& lhs, const Matrix<T,S,W>& rhs) noexcept;

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Vector<T,H> operator* (const Matrix<T,H,W>& lhs, const Vector<T,W>& rhs) noexcept;

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator* (const Matrix<T,H,W>& lhs, T rhs) noexcept;

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator* (T lhs, const Matrix<T,H,W>& rhs) noexcept;

// Operators (comparison)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL bool operator== (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept;

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL bool operator!= (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept;

} // namespace sfz

#include "sfz/math/Matrix.inl"
