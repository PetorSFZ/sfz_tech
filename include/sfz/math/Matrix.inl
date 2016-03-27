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

// Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename T, size_t M, size_t N>
Matrix<T,M,N>::Matrix(const T* arrayPtr, bool rowMajorData) noexcept
{
	static_assert(sizeof(elements) == (M*N*sizeof(T)), "Matrix is padded.");
	T* data = this->data();
	if (rowMajorData) {
		for (size_t i = 0; i < M; ++i) {
			for (size_t j = 0; j < N; ++j) {
				data[j*M + i] = arrayPtr[i*N + j];
			}
		}
	} else {
		for (size_t i = 0; i < M*N; ++i) {
			data[i] = arrayPtr[i];
		}
	}
}

template<typename T, size_t M, size_t N>
Matrix<T,M,N>::Matrix(std::initializer_list<std::initializer_list<T>> list) noexcept
{
	static_assert(sizeof(elements) == (M*N*sizeof(T)), "Matrix is padded.");

	for (size_t j = 0; j < N; ++j) {
		for (size_t i = 0; i < M; ++i) {
			elements[j][i] = T(0);
		}
	}

	sfz_assert_debug(list.size() <= M);
	const std::initializer_list<T>* rowsPtr = list.begin();
	for (size_t i = 0; i < list.size(); ++i) {
		sfz_assert_debug(rowsPtr[i].size() <= N);
		for (size_t j = 0; j < rowsPtr[i].size(); ++j) {
			elements[j][i] = rowsPtr[i].begin()[j];
		}
	}
}

// Member functions
// ------------------------------------------------------------------------------------------------

template<typename T, size_t M, size_t N>
T& Matrix<T,M,N>::at(size_t i, size_t j) noexcept
{
	sfz_assert_debug(i < M);
	sfz_assert_debug(j < N);
	return elements[j][i];
}

template<typename T, size_t M, size_t N>
T Matrix<T,M,N>::at(size_t i, size_t j) const noexcept
{
	sfz_assert_debug(i < M);
	sfz_assert_debug(j < N);
	return elements[j][i];
}

template<typename T, size_t M, size_t N>
Vector<T,N> Matrix<T,M,N>::rowAt(size_t i) const noexcept
{
	sfz_assert_debug(i < M);
	Vector<T,N> row;
	for (size_t j = 0; j < N; j++) {
		row[j] = elements[j][i];
	}
	return row;
}

template<typename T, size_t M, size_t N>
Vector<T,M> Matrix<T,M,N>::columnAt(size_t j) const noexcept
{
	sfz_assert_debug(j < N);
	Vector<T,M> column;
	for (size_t i = 0; i < M; i++) {
		column[i] = elements[j][i];
	}
	return column;
}

template<typename T, size_t M, size_t N>
void Matrix<T,M,N>::set(size_t i, size_t j, T value) noexcept
{
	sfz_assert_debug(i < M);
	sfz_assert_debug(j < N);
	elements[j][i] = value;
}

template<typename T, size_t M, size_t N>
void Matrix<T,M,N>::setRow(size_t i, const Vector<T,N>& row) noexcept
{
	sfz_assert_debug(i < M);
	size_t j = 0;
	for (auto element : row) {
		elements[j][i] = element;
		j++;
	}
}

template<typename T, size_t M, size_t N>
void Matrix<T,M,N>::setColumn(size_t j, const Vector<T,M>& column) noexcept
{
	sfz_assert_debug(j < N);
	size_t i = 0;
	for (auto element : column) {
		elements[j][i] = element;
		i++;
	}
}

// Matrix constants
// ------------------------------------------------------------------------------------------------

template<typename T, size_t M, size_t N>
Matrix<T,M,N> ZERO_MATRIX() noexcept
{
	static const Matrix<T,M,N> ZERO = []{ Matrix<T,M,N> temp; fill(temp, T(0)); return temp; }();
	return ZERO;
}

// Matrix functions
// ------------------------------------------------------------------------------------------------

template<typename T, size_t M, size_t N>
void fill(Matrix<T, M, N>& matrix, T value)
{
	for (size_t i = 0; i < M; ++i) {
		for (size_t j = 0; j < N; ++j) {
			matrix.elements[j][i] = value;
		}
	}
}

template<typename T, size_t M, size_t N>
Matrix<T,M,N> elemMult(const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs) noexcept
{
	Matrix<T,M,N> resMatrix;
	for (size_t i = 0; i < M; i++) {
		for (size_t j = 0; j < N; j++) {
			resMatrix.elements[j][i] = lhs.elements[j][i] * rhs.elements[j][i];
		}
	}
	return resMatrix;
}

template<typename T, size_t M, size_t N>
Matrix<T,N,M> transpose(const Matrix<T,M,N>& matrix) noexcept
{
	Matrix<T,N,M> resMatrix;
	for (size_t i = 0; i < N; i++) {
		for (size_t j = 0; j < M; j++) {
			resMatrix.elements[j][i] = matrix.elements[i][j];
		}
	}
	return resMatrix;
}

template<typename T, size_t M, size_t N>
size_t hash(const Matrix<T,M,N>& matrix) noexcept
{
	std::hash<T> hasher;
	size_t hash = 0;
	for (size_t i = 0; i < M; i++) {
		for (size_t j = 0; j < N; j++) {
			// hash_combine algorithm from boost
			hash ^= hasher(matrix.elements[j][i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}
	}
	return hash;
}

template<typename T, size_t M, size_t N>
std::string to_string(const Matrix<T,M,N>& matrix) noexcept
{
	using std::to_string;
	std::string str;
	str += "{ ";
	for (size_t i = 0; i < M; i++) {
		if (i > 0) {
			str += "  ";
		}
		str += "{";
		for (size_t j = 0; j < N; j++) {
			str += to_string(matrix.at(i, j));
			if (j < N-1) {
				str += ", ";
			}
		}
		str += "}";
		if (i < M-1) {
			str += ",\n";
		}
	}
	str += " }";
	return std::move(str);
}

// Operators (arithmetic & sssignment)
// ------------------------------------------------------------------------------------------------

template<typename T, size_t M, size_t N>
Matrix<T,M,N>& operator+= (Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs) noexcept
{
	for (size_t i = 0; i < M; ++i) {
		for (size_t j = 0; j < N; ++j) {
			lhs.elements[j][i] += rhs.elements[j][i];
		}
	}
	return lhs;
}

template<typename T, size_t M, size_t N>
Matrix<T,M,N>& operator-= (Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs) noexcept
{
	for (size_t i = 0; i < M; ++i) {
		for (size_t j = 0; j < N; ++j) {
			lhs.elements[j][i] -= rhs.elements[j][i];
		}
	}
	return lhs;
}

template<typename T, size_t M, size_t N>
Matrix<T,M,N>& operator*= (Matrix<T,M,N>& lhs, T rhs) noexcept
{
	for (size_t i = 0; i < M; i++) {
		for (size_t j = 0; j < N; j++) {
			lhs.elements[j][i] *= rhs;
		}
	}
	return lhs;
}

template<typename T, size_t N>
Matrix<T,N,N>& operator*= (Matrix<T,N,N>& lhs, const Matrix<T,N,N>& rhs) noexcept
{
	return (lhs = lhs * rhs);
}

// Operators (arithmetic)
// ------------------------------------------------------------------------------------------------

template<typename T, size_t M, size_t N>
Matrix<T,M,N> operator+ (const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs) noexcept
{
	Matrix<T,M,N> temp{lhs};
	return (temp += rhs);
}

template<typename T, size_t M, size_t N>
Matrix<T,M,N> operator- (const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs) noexcept
{
	Matrix<T,M,N> temp{lhs};
	return (temp -= rhs);
}

template<typename T, size_t M, size_t N>
Matrix<T,M,N> operator- (const Matrix<T,M,N>& matrix) noexcept
{
	Matrix<T,M,N> temp{matrix};
	return (temp *= T(-1));
}

template<typename T, size_t M, size_t N, size_t P>
Matrix<T,M,P> operator* (const Matrix<T,M,N>& lhs, const Matrix<T,N,P>& rhs) noexcept
{
	Matrix<T,M,P> resMatrix;
	for (size_t i = 0; i < M; i++) {
		for (size_t j = 0; j < P; j++) {
			T temp = 0;
			size_t jInnerThis = 0;
			size_t iInnerOther = 0;
			while (jInnerThis < M) {
				temp += lhs.elements[jInnerThis][i] * rhs.elements[j][iInnerOther];
				jInnerThis++;
				iInnerOther++;
			}
			resMatrix.elements[j][i] = temp;
		}
	}
	return resMatrix;
}

template<typename T, size_t M, size_t N>
Vector<T,M> operator* (const Matrix<T,M,N>& lhs, const Vector<T,N>& rhs) noexcept
{
	Vector<T,M> resVector;
	for (size_t i = 0; i < M; ++i) {
		T temp = 0;
		size_t jInnerThis = 0;
		for (size_t iVec = 0; iVec < N; ++iVec) {
			temp += lhs.elements[jInnerThis][i] * rhs.elements[iVec];
			jInnerThis += 1;
		}
		resVector[i] = temp;
	}
	return resVector;
}

template<typename T, size_t M, size_t N>
Matrix<T,M,N> operator* (const Matrix<T,M,N>& lhs, T rhs) noexcept
{
	Matrix<T,M,N> temp{lhs};
	return (temp *= rhs);
}

template<typename T, size_t M, size_t N>
Matrix<T,M,N> operator* (T lhs, const Matrix<T,M,N>& rhs) noexcept
{
	return rhs * lhs;
}

// Operators (comparison)
// ------------------------------------------------------------------------------------------------

template<typename T, size_t M, size_t N>
bool operator== (const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs) noexcept
{
	for (size_t i = 0; i < M; i++) {
		for (size_t j = 0; j < N; j++) {
			if (lhs.elements[j][i] != rhs.elements[j][i]) {
				return false;
			}
		}
	}
	return true;
}

template<typename T, size_t M, size_t N>
bool operator!= (const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs) noexcept
{
	return !(lhs == rhs);
}

} // namespace sfz
