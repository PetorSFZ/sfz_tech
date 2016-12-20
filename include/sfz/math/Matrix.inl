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
Matrix<T,H,W>::Matrix(const T* arrayPtr) noexcept
{
	T* data = this->data();
	for (uint32_t i = 0; i < (H * W); i++) {
		data[i] = arrayPtr[i];
	}
}

template<typename T, uint32_t H, uint32_t W>
Vector<T,H> Matrix<T,H,W>::columnAt(uint32_t x) const noexcept
{
	Vector<T,H> column;
	for (uint32_t y = 0; y < H; y++) {
		column[y] = this->at(y, x);
	}
	return column;
}

template<typename T, uint32_t H, uint32_t W>
void Matrix<T,H,W>::setColumn(uint32_t x, Vector<T,H> column) noexcept
{
	for (uint32_t y = 0; y < W; y++) {
		this->at(y, x) = column(y);
	}
}

// Matrix struct declaration: Matrix<T,2,2>
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,2,2>::Matrix(const T* arrayPtr) noexcept
{
	e00 = arrayPtr[0];
	e01 = arrayPtr[1];
	e10 = arrayPtr[2];
	e11 = arrayPtr[3];
}

template<typename T>
Matrix<T,2,2>::Matrix(T e00, T e01,
                      T e10, T e11) noexcept
:
	e00(e00), e01(e01),
	e10(e10), e11(e11)
{ }

template<typename T>
Matrix<T,2,2>::Matrix(Vector<T,2> row0,
                      Vector<T,2> row1) noexcept
{
	rows[0] = row0;
	rows[1] = row1;
}

template<typename T>
Matrix<T,2,2> Matrix<T,2,2>::fill(T value) noexcept
{
	return Matrix<T,2,2>(value, value,
	                     value, value);
}

template<typename T>
Matrix<T,2,2> Matrix<T,2,2>::identity() noexcept
{
	return Matrix<T,2,2>(T(1), T(0),
	                     T(0), T(1));
}

template<typename T>
Matrix<T,2,2> Matrix<T,2,2>::scaling(T scale) noexcept
{
	return Matrix<T,2,2>(scale, T(0),
	                     T(0), scale);
}

template<typename T>
Vector<T,2> Matrix<T,2,2>::columnAt(uint32_t x) const noexcept
{
	return Vector<T,2>(this->at(0, x), this->at(1, x));
}

template<typename T>
void Matrix<T,2,2>::setColumn(uint32_t x, Vector<T,2> column) noexcept
{
	this->set(0, x, column.x);
	this->set(1, x, column.y);
}

// Matrix struct declaration: Matrix<T,3,3>
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,3,3>::Matrix(const T* arrayPtr) noexcept
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
Matrix<T,3,3>::Matrix(T e00, T e01, T e02,
                      T e10, T e11, T e12,
                      T e20, T e21, T e22) noexcept
:
	e00(e00), e01(e01), e02(e02),
	e10(e10), e11(e11), e12(e12),
	e20(e20), e21(e21), e22(e22)
{ }

template<typename T>
Matrix<T,3,3>::Matrix(Vector<T,3> row0,
                      Vector<T,3> row1,
                      Vector<T,3> row2) noexcept
{
	rows[0] = row0;
	rows[1] = row1;
	rows[2] = row2;
}

template<typename T>
Matrix<T,3,3>::Matrix(const Matrix<T,3,4>& matrix) noexcept
{
	rows[0] = matrix.rows[0].xyz;
	rows[1] = matrix.rows[1].xyz;
	rows[2] = matrix.rows[2].xyz;
}

template<typename T>
Matrix<T,3,3>::Matrix(const Matrix<T,4,4>& matrix) noexcept
{
	rows[0] = matrix.rows[0].xyz;
	rows[1] = matrix.rows[1].xyz;
	rows[2] = matrix.rows[2].xyz;
}

template<typename T>
Matrix<T,3,3> Matrix<T,3,3>::fill(T value) noexcept
{
	return Matrix<T,3,3>(value, value, value,
	                     value, value, value,
	                     value, value, value);
}

template<typename T>
Matrix<T,3,3> Matrix<T,3,3>::identity() noexcept
{
	return Matrix<T,3,3>(T(1), T(0), T(0),
	                     T(0), T(1), T(0),
	                     T(0), T(0), T(1));
}

template<typename T>
Matrix<T,3,3> Matrix<T,3,3>::scaling(T scale) noexcept
{
	return Matrix<T,3,3>(scale, T(0), T(0),
	                     T(0), scale, T(0),
	                     T(0), T(0), scale);
}

template<typename T>
Vector<T,3> Matrix<T,3,3>::columnAt(uint32_t x) const noexcept
{
	return Vector<T,3>(this->at(0, x), this->at(1, x), this->at(2, x));
}

template<typename T>
void Matrix<T,3,3>::setColumn(uint32_t x, Vector<T,3> column) noexcept
{
	this->set(0, x, column.x);
	this->set(1, x, column.y);
	this->set(2, x, column.z);
}

// Matrix struct declaration: Matrix<T,3,4>
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,3,4>::Matrix(const T* arrayPtr) noexcept
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
Matrix<T,3,4>::Matrix(T e00, T e01, T e02, T e03,
                      T e10, T e11, T e12, T e13,
                      T e20, T e21, T e22, T e23) noexcept
:
	e00(e00), e01(e01), e02(e02), e03(e03),
	e10(e10), e11(e11), e12(e12), e13(e13),
	e20(e20), e21(e21), e22(e22), e23(e23)
{ }

template<typename T>
Matrix<T,3,4>::Matrix(Vector<T,4> row0,
                      Vector<T,4> row1,
                      Vector<T,4> row2) noexcept
{
	rows[0] = row0;
	rows[1] = row1;
	rows[2] = row2;
}

template<typename T>
Matrix<T,3,4>::Matrix(const Matrix<T,3,3>& matrix) noexcept
{
	rows[0] = vec4(matrix.rows[0], T(0));
	rows[1] = vec4(matrix.rows[1], T(0));
	rows[2] = vec4(matrix.rows[2], T(0));
}

template<typename T>
Matrix<T,3,4>::Matrix(const Matrix<T,4,4>& matrix) noexcept
{
	rows[0] = matrix.rows[0];
	rows[1] = matrix.rows[1];
	rows[2] = matrix.rows[2];
}

template<typename T>
Matrix<T,3,4> Matrix<T,3,4>::fill(T value) noexcept
{
	return Matrix<T,3,4>(value, value, value, value,
	                     value, value, value, value,
	                     value, value, value, value);
}

template<typename T>
Matrix<T,3,4> Matrix<T,3,4>::identity() noexcept
{
	return Matrix<T,3,4>(T(1), T(0), T(0), T(0),
	                     T(0), T(1), T(0), T(0),
	                     T(0), T(0), T(1), T(0));
}

template<typename T>
Matrix<T,3,4> Matrix<T,3,4>::scaling(T scale) noexcept
{
	return Matrix<T,3,4>(scale, T(0), T(0), T(0),
	                     T(0), scale, T(0), T(0),
	                     T(0), T(0), scale, T(0));
}

template<typename T>
Vector<T,3> Matrix<T,3,4>::columnAt(uint32_t x) const noexcept
{
	return Vector<T,3>(this->at(0, x), this->at(1, x), this->at(2, x));
}

template<typename T>
void Matrix<T,3,4>::setColumn(uint32_t x, Vector<T,3> column) noexcept
{
	this->set(0, x, column.x);
	this->set(1, x, column.y);
	this->set(2, x, column.z);
}

// Matrix struct declaration: Matrix<T,4,4>
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,4,4>::Matrix(const T* arrayPtr) noexcept
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
Matrix<T,4,4>::Matrix(T e00, T e01, T e02, T e03,
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
Matrix<T,4,4>::Matrix(Vector<T,4> row0,
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
Matrix<T,4,4>::Matrix(const Matrix<T,3,3>& matrix) noexcept
{
	rows[0] = vec4(matrix.rows[0], T(0));
	rows[1] = vec4(matrix.rows[1], T(0));
	rows[2] = vec4(matrix.rows[2], T(0));
	rows[3] = vec4(T(0), T(0), T(0), T(1));
}

template<typename T>
Matrix<T,4,4>::Matrix(const Matrix<T,3,4>& matrix) noexcept
{
	rows[0] = matrix.rows[0];
	rows[1] = matrix.rows[1];
	rows[2] = matrix.rows[2];
	rows[3] = vec4(T(0), T(0), T(0), T(1));
}

template<typename T>
Matrix<T,4,4> Matrix<T,4,4>::fill(T value) noexcept
{
	return Matrix<T,4,4>(value, value, value, value,
	                     value, value, value, value,
	                     value, value, value, value,
	                     value, value, value, value);
}

template<typename T>
Matrix<T,4,4> Matrix<T,4,4>::identity() noexcept
{
	return Matrix<T,4,4>(T(1), T(0), T(0), T(0),
	                     T(0), T(1), T(0), T(0),
	                     T(0), T(0), T(1), T(0),
	                     T(0), T(0), T(0), T(1));
}

template<typename T>
Matrix<T,4,4> Matrix<T,4,4>::scaling(T scale) noexcept
{
	return Matrix<T,4,4>(scale, T(0), T(0), T(0),
	                     T(0), scale, T(0), T(0),
	                     T(0), T(0), scale, T(0),
	                     T(0), T(0), T(0), T(1));
}

template<typename T>
Vector<T,4> Matrix<T,4,4>::columnAt(uint32_t x) const noexcept
{
	return Vector<T,4>(this->at(0, x), this->at(1, x), this->at(2, x), this->at(3, x));
}

template<typename T>
void Matrix<T,4,4>::setColumn(uint32_t x, Vector<T,4> column) noexcept
{
	this->set(0, x, column.x);
	this->set(1, x, column.y);
	this->set(2, x, column.z);
	this->set(3, x, column.w);
}

// Matrix functions
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
Matrix<T,H,W> elemMult(const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	Matrix<T,H,W> result;
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			result.at(y, x) = lhs.at(y, x) * rhs.at(y, x);
		}
	}
	return result;
}

template<typename T, uint32_t H, uint32_t W>
Matrix<T,W,H> transpose(const Matrix<T,H,W>& matrix) noexcept
{
	Matrix<T,W,H> result;
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			result.at(x, y) = matrix.at(y, x);
		}
	}
	return result;
}

template<typename T>
Vector<T,3> transformPoint(const Matrix<T,3,4>& m, const Vector<T,3>& p) noexcept
{
	Vector<T,4> v(p, T(1));
	return m * v;
}

template<typename T>
Vector<T,3> transformPoint(const Matrix<T,4,4>& m, const Vector<T,3>& p) noexcept
{
	Vector<T,4> v(p, T(1));
	v = m * v;
	return v.xyz / v.w;
}

template<typename T>
Vector<T,3> transformDir(const Matrix<T,3,4>& m, const Vector<T,3>& d) noexcept
{
	Vector<T, 4> v(d, T(0));
	return m * v;
}

template<typename T>
Vector<T,3> transformDir(const Matrix<T,4,4>& m, const Vector<T,3>& d) noexcept
{
	Vector<T,4> v(d, T(0));
	v = m * v;
	return v.xyz;
}

// Operators (arithmetic & assignment)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
Matrix<T,H,W>& operator+= (Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			lhs.at(y, x) += rhs.at(y, x);
		}
	}
	return lhs;
}

template<typename T, uint32_t H, uint32_t W>
Matrix<T,H,W>& operator-= (Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			lhs.at(y, x) -= rhs.at(y, x);
		}
	}
	return lhs;
}

template<typename T, uint32_t H, uint32_t W>
Matrix<T,H,W>& operator*= (Matrix<T,H,W>& lhs, T rhs) noexcept
{
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			lhs.at(y, x) *= rhs;
		}
	}
	return lhs;
}

template<typename T, uint32_t N>
Matrix<T,N,N>& operator*= (Matrix<T,N,N>& lhs, const Matrix<T,N,N>& rhs) noexcept
{
	return (lhs = lhs * rhs);
}

// Operators (arithmetic)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
Matrix<T,H,W> operator+ (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	Matrix<T,H,W> tmp = lhs;
	return (tmp += rhs);
}

template<typename T, uint32_t H, uint32_t W>
Matrix<T,H,W> operator- (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	Matrix<T,H,W> tmp = lhs;
	return (tmp -= rhs);
}

template<typename T, uint32_t H, uint32_t W>
Matrix<T,H,W> operator- (const Matrix<T,H,W>& matrix) noexcept
{
	Matrix<T,H,W> tmp = matrix;
	return (tmp *= T(-1));
}

template<typename T, uint32_t H, uint32_t S, uint32_t W>
Matrix<T,H,W> operator* (const Matrix<T,H,S>& lhs, const Matrix<T,S,W>& rhs) noexcept
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
Vector<T,H> operator* (const Matrix<T,H,W>& lhs, const Vector<T,W>& rhs) noexcept
{
	Vector<T,H> res;
	for (uint32_t i = 0; i < H; i++) {
		res[i] = dot(lhs.rows[i], rhs);
	}
	return res;
}

template<typename T, uint32_t H, uint32_t W>
Matrix<T,H,W> operator* (const Matrix<T,H,W>& lhs, T rhs) noexcept
{
	Matrix<T,H,W> tmp = lhs;
	return (tmp *= rhs);
}

template<typename T, uint32_t H, uint32_t W>
Matrix<T,H,W> operator* (T lhs, const Matrix<T,H,W>& rhs) noexcept
{
	return rhs * lhs;
}

// Operators (comparison)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
bool operator== (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	for (uint32_t y = 0; y < H; y++) {
		for (uint32_t x = 0; x < W; x++) {
			if (lhs.at(y, x) != rhs.at(y, x)) return false;
		}
	}
	return true;
}

template<typename T, uint32_t H, uint32_t W>
bool operator!= (const Matrix<T,H,W>& lhs, const Matrix<T,H,W>& rhs) noexcept
{
	return !(lhs == rhs);
}

} // namespace sfz
