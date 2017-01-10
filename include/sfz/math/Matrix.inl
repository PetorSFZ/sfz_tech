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

namespace sfz {

// Matrix struct declaration: Matrix<T,H,W>
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W>::Matrix(const T* arrayPtr) noexcept
{
	T* data = this->data();
	for (uint32_t i = 0; i < (H * W); i++) {
		data[i] = arrayPtr[i];
	}
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Vector<T,H> Matrix<T,H,W>::columnAt(uint32_t x) const noexcept
{
	Vector<T,H> column;
	for (uint32_t y = 0; y < H; y++) {
		column[y] = this->at(y, x);
	}
	return column;
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL void Matrix<T,H,W>::setColumn(uint32_t x, Vector<T,H> column) noexcept
{
	for (uint32_t y = 0; y < W; y++) {
		this->at(y, x) = column(y);
	}
}

// Matrix struct declaration: Matrix<T,2,2>
// ------------------------------------------------------------------------------------------------

template<typename T>
SFZ_CUDA_CALL Matrix<T,2,2>::Matrix(const T* arrayPtr) noexcept
{
	e00 = arrayPtr[0];
	e01 = arrayPtr[1];
	e10 = arrayPtr[2];
	e11 = arrayPtr[3];
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,2,2>::Matrix(T e00, T e01,
                                    T e10, T e11) noexcept
:
	e00(e00), e01(e01),
	e10(e10), e11(e11)
{ }

template<typename T>
SFZ_CUDA_CALL Matrix<T,2,2>::Matrix(Vector<T,2> row0,
                                    Vector<T,2> row1) noexcept
{
	rows[0] = row0;
	rows[1] = row1;
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,2,2> Matrix<T,2,2>::fill(T value) noexcept
{
	return Matrix<T,2,2>(value, value,
	                     value, value);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,2,2> Matrix<T,2,2>::identity() noexcept
{
	return Matrix<T,2,2>(T(1), T(0),
	                     T(0), T(1));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,2,2> Matrix<T,2,2>::scaling2(T scale) noexcept
{
	return Matrix<T,2,2>(scale, T(0),
	                     T(0), scale);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,2,2> Matrix<T,2,2>::scaling2(T x, T y) noexcept
{
	return Matrix<T,2,2>(x, T(0),
	                     T(0), y);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,2,2> Matrix<T,2,2>::scaling2(Vector<T,2> scale) noexcept
{
	return Matrix<T,2,2>::scaling2(scale.x, scale.y);
}

template<typename T>
SFZ_CUDA_CALL Vector<T,2> Matrix<T,2,2>::columnAt(uint32_t x) const noexcept
{
	return Vector<T,2>(this->at(0, x), this->at(1, x));
}

template<typename T>
SFZ_CUDA_CALL void Matrix<T,2,2>::setColumn(uint32_t x, Vector<T,2> column) noexcept
{
	this->set(0, x, column.x);
	this->set(1, x, column.y);
}

// Matrix struct declaration: Matrix<T,3,3>
// ------------------------------------------------------------------------------------------------

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3>::Matrix(const T* arrayPtr) noexcept
{
	e00 = arrayPtr[0];
	e01 = arrayPtr[1];
	e02 = arrayPtr[2];

	e10 = arrayPtr[3];
	e11 = arrayPtr[4];
	e12 = arrayPtr[5];

	e20 = arrayPtr[6];
	e21 = arrayPtr[7];
	e22 = arrayPtr[8];
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3>::Matrix(T e00, T e01, T e02,
                                    T e10, T e11, T e12,
                                    T e20, T e21, T e22) noexcept
:
	e00(e00), e01(e01), e02(e02),
	e10(e10), e11(e11), e12(e12),
	e20(e20), e21(e21), e22(e22)
{ }

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3>::Matrix(Vector<T,3> row0,
                                    Vector<T,3> row1,
                                    Vector<T,3> row2) noexcept
{
	rows[0] = row0;
	rows[1] = row1;
	rows[2] = row2;
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3>::Matrix(const Matrix<T,3,4>& matrix) noexcept
{
	rows[0] = matrix.rows[0].xyz;
	rows[1] = matrix.rows[1].xyz;
	rows[2] = matrix.rows[2].xyz;
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3>::Matrix(const Matrix<T,4,4>& matrix) noexcept
{
	rows[0] = matrix.rows[0].xyz;
	rows[1] = matrix.rows[1].xyz;
	rows[2] = matrix.rows[2].xyz;
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3> Matrix<T,3,3>::fill(T value) noexcept
{
	return Matrix<T,3,3>(value, value, value,
	                     value, value, value,
	                     value, value, value);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3> Matrix<T,3,3>::identity() noexcept
{
	return Matrix<T,3,3>(T(1), T(0), T(0),
	                     T(0), T(1), T(0),
	                     T(0), T(0), T(1));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3> Matrix<T,3,3>::scaling3(T scale) noexcept
{
	return Matrix<T,3,3>(scale, T(0), T(0),
	                     T(0), scale, T(0),
	                     T(0), T(0), scale);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3> Matrix<T,3,3>::scaling3(T x, T y, T z) noexcept
{
	return Matrix<T,3,3>(x, T(0), T(0),
	                     T(0), y, T(0),
	                     T(0), T(0), z);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3> Matrix<T,3,3>::scaling3(Vector<T,3> scale) noexcept
{
	return Matrix<T,3,3>::scaling3(scale.x, scale.y, scale.z);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3> Matrix<T,3,3>::rotation3(Vector<T,3> axis, T angleRad) noexcept
{
	using std::cos;
	using std::sin;
	Vector<T,3> r = normalize(axis);
	T x = r.x;
	T y = r.y;
	T z = r.z;
	T c = cos(angleRad);
	T s = sin(angleRad);
	T cm1 = T(1) - c;
	// Matrix by Goldman, page 71 of Real-Time Rendering.
	return Matrix<T,3,3>(c + cm1*x*x, cm1*x*y - z*s, cm1*x*z + y*s,
	                     cm1*x*y + z*s, c + cm1*y*y, cm1*y*z - x*s,
	                     cm1*x*z - y*s, cm1*y*z + x*s, c + cm1*z*z);
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3> Matrix<T,3,3>::columnAt(uint32_t x) const noexcept
{
	return Vector<T,3>(this->at(0, x), this->at(1, x), this->at(2, x));
}

template<typename T>
SFZ_CUDA_CALL void Matrix<T,3,3>::setColumn(uint32_t x, Vector<T,3> column) noexcept
{
	this->set(0, x, column.x);
	this->set(1, x, column.y);
	this->set(2, x, column.z);
}

// Matrix struct declaration: Matrix<T,3,4>
// ------------------------------------------------------------------------------------------------

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4>::Matrix(const T* arrayPtr) noexcept
{
	e00 = arrayPtr[0];
	e01 = arrayPtr[1];
	e02 = arrayPtr[2];
	e03 = arrayPtr[3];

	e10 = arrayPtr[4];
	e11 = arrayPtr[5];
	e12 = arrayPtr[6];
	e13 = arrayPtr[7];

	e20 = arrayPtr[8];
	e21 = arrayPtr[9];
	e22 = arrayPtr[10];
	e23 = arrayPtr[11];
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4>::Matrix(T e00, T e01, T e02, T e03,
                                    T e10, T e11, T e12, T e13,
                                    T e20, T e21, T e22, T e23) noexcept
:
	e00(e00), e01(e01), e02(e02), e03(e03),
	e10(e10), e11(e11), e12(e12), e13(e13),
	e20(e20), e21(e21), e22(e22), e23(e23)
{ }

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4>::Matrix(Vector<T,4> row0,
                                    Vector<T,4> row1,
                                    Vector<T,4> row2) noexcept
{
	rows[0] = row0;
	rows[1] = row1;
	rows[2] = row2;
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4>::Matrix(const Matrix<T,3,3>& matrix) noexcept
{
	rows[0] = vec4(matrix.rows[0], T(0));
	rows[1] = vec4(matrix.rows[1], T(0));
	rows[2] = vec4(matrix.rows[2], T(0));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4>::Matrix(const Matrix<T,4,4>& matrix) noexcept
{
	rows[0] = matrix.rows[0];
	rows[1] = matrix.rows[1];
	rows[2] = matrix.rows[2];
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4> Matrix<T,3,4>::fill(T value) noexcept
{
	return Matrix<T,3,4>(value, value, value, value,
	                     value, value, value, value,
	                     value, value, value, value);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4> Matrix<T,3,4>::identity() noexcept
{
	return Matrix<T,3,4>(T(1), T(0), T(0), T(0),
	                     T(0), T(1), T(0), T(0),
	                     T(0), T(0), T(1), T(0));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4> Matrix<T,3,4>::scaling3(T scale) noexcept
{
	return Matrix<T,3,4>(scale, T(0), T(0), T(0),
	                     T(0), scale, T(0), T(0),
	                     T(0), T(0), scale, T(0));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4> Matrix<T,3,4>::scaling3(T x, T y, T z) noexcept
{
	return Matrix<T,3,4>(x, T(0), T(0), T(0),
	                     T(0), y, T(0), T(0),
	                     T(0), T(0), z, T(0));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4> Matrix<T,3,4>::scaling3(Vector<T,3> scale) noexcept
{
	return Matrix<T,3,4>::scaling3(scale.x, scale.y, scale.z);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4> Matrix<T,3,4>::rotation3(Vector<T,3> axis, T angleRad) noexcept
{
	using std::cos;
	using std::sin;
	Vector<T,3> r = normalize(axis);
	T x = r.x;
	T y = r.y;
	T z = r.z;
	T c = cos(angleRad);
	T s = sin(angleRad);
	T cm1 = T(1) - c;
	// Matrix by Goldman, page 71 of Real-Time Rendering.
	return Matrix<T,3,4>(c + cm1*x*x, cm1*x*y - z*s, cm1*x*z + y*s, T(0),
	                     cm1*x*y + z*s, c + cm1*y*y, cm1*y*z - x*s, T(0),
	                     cm1*x*z - y*s, cm1*y*z + x*s, c + cm1*z*z, T(0));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,4> Matrix<T,3,4>::translation3(Vector<T,3> delta) noexcept
{
	return Matrix<T,3,4>(T(1), T(0), T(0), delta.x,
	                     T(0), T(1), T(0), delta.y,
	                     T(0), T(0), T(1), delta.z);
}


template<typename T>
SFZ_CUDA_CALL Vector<T,3> Matrix<T,3,4>::columnAt(uint32_t x) const noexcept
{
	return Vector<T,3>(this->at(0, x), this->at(1, x), this->at(2, x));
}

template<typename T>
SFZ_CUDA_CALL void Matrix<T,3,4>::setColumn(uint32_t x, Vector<T,3> column) noexcept
{
	this->set(0, x, column.x);
	this->set(1, x, column.y);
	this->set(2, x, column.z);
}

// Matrix struct declaration: Matrix<T,4,4>
// ------------------------------------------------------------------------------------------------

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4>::Matrix(const T* arrayPtr) noexcept
{
	e00 = arrayPtr[0];
	e01 = arrayPtr[1];
	e02 = arrayPtr[2];
	e03 = arrayPtr[3];

	e10 = arrayPtr[4];
	e11 = arrayPtr[5];
	e12 = arrayPtr[6];
	e13 = arrayPtr[7];

	e20 = arrayPtr[8];
	e21 = arrayPtr[9];
	e22 = arrayPtr[10];
	e23 = arrayPtr[11];

	e30 = arrayPtr[12];
	e31 = arrayPtr[13];
	e32 = arrayPtr[14];
	e33 = arrayPtr[15];
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4>::Matrix(T e00, T e01, T e02, T e03,
                                    T e10, T e11, T e12, T e13,
                                    T e20, T e21, T e22, T e23,
                                    T e30, T e31, T e32, T e33) noexcept
:
	e00(e00), e01(e01), e02(e02), e03(e03),
	e10(e10), e11(e11), e12(e12), e13(e13),
	e20(e20), e21(e21), e22(e22), e23(e23),
	e30(e30), e31(e31), e32(e32), e33(e33)
{ }

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4>::Matrix(Vector<T,4> row0,
                                    Vector<T,4> row1,
                                    Vector<T,4> row2,
                                    Vector<T,4> row3) noexcept
{
	rows[0] = row0;
	rows[1] = row1;
	rows[2] = row2;
	rows[3] = row3;
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4>::Matrix(const Matrix<T,3,3>& matrix) noexcept
{
	rows[0] = vec4(matrix.rows[0], T(0));
	rows[1] = vec4(matrix.rows[1], T(0));
	rows[2] = vec4(matrix.rows[2], T(0));
	rows[3] = vec4(T(0), T(0), T(0), T(1));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4>::Matrix(const Matrix<T,3,4>& matrix) noexcept
{
	rows[0] = matrix.rows[0];
	rows[1] = matrix.rows[1];
	rows[2] = matrix.rows[2];
	rows[3] = vec4(T(0), T(0), T(0), T(1));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4> Matrix<T,4,4>::fill(T value) noexcept
{
	return Matrix<T,4,4>(value, value, value, value,
	                     value, value, value, value,
	                     value, value, value, value,
	                     value, value, value, value);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4> Matrix<T,4,4>::identity() noexcept
{
	return Matrix<T,4,4>(T(1), T(0), T(0), T(0),
	                     T(0), T(1), T(0), T(0),
	                     T(0), T(0), T(1), T(0),
	                     T(0), T(0), T(0), T(1));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4> Matrix<T,4,4>::scaling3(T scale) noexcept
{
	return Matrix<T,4,4>(scale, T(0), T(0), T(0),
	                     T(0), scale, T(0), T(0),
	                     T(0), T(0), scale, T(0),
	                     T(0), T(0), T(0), T(1));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4> Matrix<T,4,4>::scaling3(T x, T y, T z) noexcept
{
	return Matrix<T,4,4>(x, T(0), T(0), T(0),
	                     T(0), y, T(0), T(0),
	                     T(0), T(0), z, T(0),
	                     T(0), T(0), T(0), T(1));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4> Matrix<T,4,4>::scaling3(Vector<T,3> scale) noexcept
{
	return Matrix<T,4,4>::scaling3(scale.x, scale.y, scale.z);
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4> Matrix<T,4,4>::rotation3(Vector<T,3> axis, T angleRad) noexcept
{
	using std::cos;
	using std::sin;
	Vector<T,3> r = normalize(axis);
	T x = r.x;
	T y = r.y;
	T z = r.z;
	T c = cos(angleRad);
	T s = sin(angleRad);
	T cm1 = T(1) - c;
	// Matrix by Goldman, page 71 of Real-Time Rendering.
	return Matrix<T,4,4>(c + cm1*x*x, cm1*x*y - z*s, cm1*x*z + y*s, T(0),
	                     cm1*x*y + z*s, c + cm1*y*y, cm1*y*z - x*s, T(0),
	                     cm1*x*z - y*s, cm1*y*z + x*s, c + cm1*z*z, T(0),
	                     T(0), T(0), T(0), T(1));
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4> Matrix<T,4,4>::translation3(Vector<T,3> delta) noexcept
{
	return Matrix<T,4,4>(T(1), T(0), T(0), delta.x,
	                     T(0), T(1), T(0), delta.y,
	                     T(0), T(0), T(1), delta.z,
	                     T(0), T(0), T(0), T(1));
}

template<typename T>
SFZ_CUDA_CALL Vector<T,4> Matrix<T,4,4>::columnAt(uint32_t x) const noexcept
{
	return Vector<T,4>(this->at(0, x), this->at(1, x), this->at(2, x), this->at(3, x));
}

template<typename T>
SFZ_CUDA_CALL void Matrix<T,4,4>::setColumn(uint32_t x, Vector<T,4> column) noexcept
{
	this->set(0, x, column.x);
	this->set(1, x, column.y);
	this->set(2, x, column.z);
	this->set(3, x, column.w);
}

// Matrix functions
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> elemMult(const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	Matrix<T,H,W> result;
	for (uint32_t y = 0; y < H; y++) {
		result.rows[y] = lhs.rows[y] * rhs.rows[y];
	}
	return result;
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,W,H> transpose(const Matrix<T,H,W>& m) noexcept
{
	Matrix<T,W,H> result;
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			result.at(x, y) = m.at(y, x);
		}
	}
	return result;
}

template<>
SFZ_CUDA_CALL mat44 transpose(const mat44& m) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	mat44 tmp;
	tmp.setColumn(0, m.row0);
	tmp.setColumn(1, m.row1);
	tmp.setColumn(2, m.row2);
	tmp.setColumn(3, m.row3);
	return tmp;
#else
	// Load matrix rows into SSE registers
	const __m128 row0 = _mm_load_ps(m.row0.data());
	const __m128 row1 = _mm_load_ps(m.row1.data());
	const __m128 row2 = _mm_load_ps(m.row2.data());
	const __m128 row3 = _mm_load_ps(m.row3.data());
	
	// Transpose matrix
	__m128 col0 = row0;
	__m128 col1 = row1;
	__m128 col2 = row2;
	__m128 col3 = row3;
	_MM_TRANSPOSE4_PS(col0, col1, col2, col3);

	// Return result
	mat44 tmp;
	_mm_store_ps(tmp.row0.data(), col0);
	_mm_store_ps(tmp.row1.data(), col1);
	_mm_store_ps(tmp.row2.data(), col2);
	_mm_store_ps(tmp.row3.data(), col3);
	return tmp;
#endif
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3> transformPoint(const Matrix<T,3,4>& m, const Vector<T,3>& p) noexcept
{
	Vector<T,4> v(p, T(1));
	return m * v;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3> transformPoint(const Matrix<T,4,4>& m, const Vector<T,3>& p) noexcept
{
	Vector<T,4> v(p, T(1));
	v = m * v;
	return v.xyz / v.w;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3> transformDir(const Matrix<T,3,4>& m, const Vector<T,3>& d) noexcept
{
	Vector<T, 4> v(d, T(0));
	return m * v;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3> transformDir(const Matrix<T,4,4>& m, const Vector<T,3>& d) noexcept
{
	Vector<T,4> v(d, T(0));
	v = m * v;
	return v.xyz;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL T determinant(const Matrix<T,N,N>& m) noexcept
{
	static_assert(N != N, "determinant() not implemented for general case");
}

template<typename T>
SFZ_CUDA_CALL T determinant(const Matrix<T,2,2>& m) noexcept
{
	return m.e00 * m.e11 - m.e01 * m.e10;
}

template<typename T>
SFZ_CUDA_CALL T determinant(const Matrix<T,3,3>& m) noexcept
{
	return m.e00 * m.e11 * m.e22
	     + m.e01 * m.e12 * m.e20
	     + m.e02 * m.e10 * m.e21
	     - m.e02 * m.e11 * m.e20
	     - m.e01 * m.e10 * m.e22
	     - m.e00 * m.e12 * m.e21;
}

template<typename T>
SFZ_CUDA_CALL T determinant(const Matrix<T,4,4>& m) noexcept
{
	return m.e00*m.e11*m.e22*m.e33 + m.e00*m.e12*m.e23*m.e31 + m.e00*m.e13*m.e21*m.e32
	     + m.e01*m.e10*m.e23*m.e32 + m.e01*m.e12*m.e20*m.e33 + m.e01*m.e13*m.e22*m.e30
	     + m.e02*m.e10*m.e21*m.e33 + m.e02*m.e11*m.e23*m.e30 + m.e02*m.e13*m.e20*m.e31
	     + m.e03*m.e10*m.e22*m.e31 + m.e03*m.e11*m.e20*m.e32 + m.e03*m.e12*m.e21*m.e30
	     - m.e00*m.e11*m.e23*m.e32 - m.e00*m.e12*m.e21*m.e33 - m.e00*m.e13*m.e22*m.e31
	     - m.e01*m.e10*m.e22*m.e33 - m.e01*m.e12*m.e23*m.e30 - m.e01*m.e13*m.e20*m.e32
	     - m.e02*m.e10*m.e23*m.e31 - m.e02*m.e11*m.e20*m.e33 - m.e02*m.e13*m.e21*m.e30
	     - m.e03*m.e10*m.e21*m.e32 - m.e03*m.e11*m.e22*m.e30 - m.e03*m.e12*m.e20*m.e31;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Matrix<T,N,N> inverse(const Matrix<T,N,N>& m) noexcept
{
	static_assert(N != N, "inverse() not implemented for general case");
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,2,2> inverse(const Matrix<T,2,2>& m) noexcept
{
	const T det = determinant(m);
	if (det == T(0)) return Matrix<T,2,2>::fill(T(0));

	Matrix<T,2,2> tmp(m.e11, -m.e01,
	                  -m.e10, m.e00);
	return (T(1) / det) * tmp;
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,3,3> inverse(const Matrix<T,3,3>& m) noexcept
{
	const T det = determinant(m);
	if (det == 0) return Matrix<T,3,3>::fill(T(0));

	const T A =  (m.e11 * m.e22 - m.e12 * m.e21);
	const T B = -(m.e10 * m.e22 - m.e12 * m.e20);
	const T C =  (m.e10 * m.e21 - m.e11 * m.e20);
	const T D = -(m.e01 * m.e22 - m.e02 * m.e21);
	const T E =  (m.e00 * m.e22 - m.e02 * m.e20);
	const T F = -(m.e00 * m.e21 - m.e01 * m.e20);
	const T G =  (m.e01 * m.e12 - m.e02 * m.e11);
	const T H = -(m.e00 * m.e12 - m.e02 * m.e10);
	const T I =  (m.e00 * m.e11 - m.e01 * m.e10);

	Matrix<T,3,3> tmp(A, D, G,
	                  B, E, H,
	                  C, F, I);
	return (T(1) / det) * tmp;
}

template<typename T>
SFZ_CUDA_CALL Matrix<T,4,4> inverse(const Matrix<T,4,4>& m) noexcept
{
	const T det = determinant(m);
	if (det == 0) return Matrix<T,4,4>(T(0), T(0), T(0), T(0),
	                                   T(0), T(0), T(0), T(0),
	                                   T(0), T(0), T(0), T(0),
	                                   T(0), T(0), T(0), T(0));

	const T m00 = m.e00, m01 = m.e01, m02 = m.e02, m03 = m.e03,
	        m10 = m.e10, m11 = m.e11, m12 = m.e12, m13 = m.e13,
	        m20 = m.e20, m21 = m.e21, m22 = m.e22, m23 = m.e23,
	        m30 = m.e30, m31 = m.e31, m32 = m.e32, m33 = m.e33;

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

	Matrix<T,4,4> tmp(b00, b01, b02, b03,
	                  b10, b11, b12, b13,
	                  b20, b21, b22, b23,
	                  b30, b31, b32, b33);
	return (T(1) / det) * tmp;
}

// Operators (arithmetic & assignment)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W>& operator+= (Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	for (uint32_t y = 0; y < H; y++) {
		lhs.rows[y] += rhs.rows[y];
	}
	return lhs;
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W>& operator-= (Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	for (uint32_t y = 0; y < H; y++) {
		lhs.rows[y] -= rhs.rows[y];
	}
	return lhs;
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W>& operator*= (Matrix<T,H,W>& lhs, T rhs) noexcept
{
	for (uint32_t y = 0; y < H; y++) {
		lhs.rows[y] *= rhs;
	}
	return lhs;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Matrix<T,N,N>& operator*= (Matrix<T,N,N>& lhs, const Matrix<T,N,N>& rhs) noexcept
{
	return (lhs = lhs * rhs);
}

// Operators (arithmetic)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator+ (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	Matrix<T,H,W> tmp = lhs;
	return (tmp += rhs);
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator- (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	Matrix<T,H,W> tmp = lhs;
	return (tmp -= rhs);
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator- (const Matrix<T,H,W>& matrix) noexcept
{
	Matrix<T,H,W> tmp = matrix;
	return (tmp *= T(-1));
}

template<typename T, uint32_t H, uint32_t S, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator* (const Matrix<T,H,S>& lhs, const Matrix<T,S,W>& rhs) noexcept
{
	Matrix<T,H,W> res;
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			res.at(y, x) = dot(lhs.rows[y], rhs.columnAt(x));
		}
	}
	return res;
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Vector<T,H> operator* (const Matrix<T,H,W>& lhs, const Vector<T,W>& rhs) noexcept
{
	Vector<T,H> res;
	for (uint32_t y = 0; y < H; y++) {
		res[y] = dot(lhs.rows[y], rhs);
	}
	return res;
}

template<>
SFZ_CUDA_CALL vec3 operator* (const mat34& lhs, const vec4& rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec3 res;
	res.x = dot(lhs.row0, rhs);
	res.y = dot(lhs.row1, rhs);
	res.z = dot(lhs.row2, rhs);
	return res;
#else
	// Load matrix rows into SSE registers
	const __m128 row0 = _mm_load_ps(lhs.row0.data());
	const __m128 row1 = _mm_load_ps(lhs.row1.data());
	const __m128 row2 = _mm_load_ps(lhs.row2.data());
	const __m128 row3 = _mm_set1_ps(0.0f);
	
	// Transpose matrix
	__m128 col0 = row0;
	__m128 col1 = row1;
	__m128 col2 = row2;
	__m128 col3 = row3;
	_MM_TRANSPOSE4_PS(col0, col1, col2, col3);

	// Load vector and replicate each element into all four slots in four vectors
	const __m128 v = _mm_load_ps(rhs.data());
	const __m128 vxxxx = replicatePs<0>(v);
	const __m128 vyyyy = replicatePs<1>(v);
	const __m128 vzzzz = replicatePs<2>(v);
	const __m128 vwwww = replicatePs<3>(v);

	// Perform computations
	const __m128 t0 = _mm_mul_ps(col0, vxxxx);
	const __m128 t1 = _mm_mul_ps(col1, vyyyy);
	const __m128 t2 = _mm_mul_ps(col2, vzzzz);
	const __m128 t3 = _mm_mul_ps(col3, vwwww);
	const __m128 result = _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3));
	
	// Return result
	vec4 tmp;
	_mm_store_ps(tmp.data(), result);
	return tmp.xyz;
#endif
}

template<>
SFZ_CUDA_CALL vec4 operator* (const mat44& lhs, const vec4& rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4 res;
	res.x = dot(lhs.row0, rhs);
	res.y = dot(lhs.row1, rhs);
	res.z = dot(lhs.row2, rhs);
	res.w = dot(lhs.row3, rhs);
	return res;
#else
	// Load matrix rows into SSE registers
	const __m128 row0 = _mm_load_ps(lhs.row0.data());
	const __m128 row1 = _mm_load_ps(lhs.row1.data());
	const __m128 row2 = _mm_load_ps(lhs.row2.data());
	const __m128 row3 = _mm_load_ps(lhs.row3.data());
	
	// Transpose matrix
	__m128 col0 = row0;
	__m128 col1 = row1;
	__m128 col2 = row2;
	__m128 col3 = row3;
	_MM_TRANSPOSE4_PS(col0, col1, col2, col3);

	// Load vector and replicate each element into all four slots in four vectors
	const __m128 v = _mm_load_ps(rhs.data());
	const __m128 vxxxx = replicatePs<0>(v);
	const __m128 vyyyy = replicatePs<1>(v);
	const __m128 vzzzz = replicatePs<2>(v);
	const __m128 vwwww = replicatePs<3>(v);

	// Perform computations
	const __m128 t0 = _mm_mul_ps(col0, vxxxx);
	const __m128 t1 = _mm_mul_ps(col1, vyyyy);
	const __m128 t2 = _mm_mul_ps(col2, vzzzz);
	const __m128 t3 = _mm_mul_ps(col3, vwwww);
	const __m128 result = _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3));
	
	// Return result
	vec4 tmp;
	_mm_store_ps(tmp.data(), result);
	return tmp;
#endif
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator* (const Matrix<T,H,W>& lhs, T rhs) noexcept
{
	Matrix<T,H,W> tmp = lhs;
	return (tmp *= rhs);
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL Matrix<T,H,W> operator* (T lhs, const Matrix<T,H,W>& rhs) noexcept
{
	return rhs * lhs;
}

// Operators (comparison)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL bool operator== (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	for (uint32_t y = 0; y < H; y++) {
		if (lhs.rows[y] != rhs.rows[y]) return false;
	}
	return true;
}

template<typename T, uint32_t H, uint32_t W>
SFZ_CUDA_CALL bool operator!= (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	return !(lhs == rhs);
}

} // namespace sfz
