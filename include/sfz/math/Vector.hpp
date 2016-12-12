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

#include <algorithm> // std::min & std::max
#include <cstdint>
#include <cmath> // std::sqrt

#include "sfz/Assert.hpp"
#include "sfz/CudaCompatibility.hpp"

/// A mathematical vector POD class that imitates a built-in primitive.
///
/// Typedefs are provided for float vectors (vec2, vec3 and vec4), (32-bit signed) integer
/// vectors (vec2i, vec3i, vec4i) and (32-bit) unsigned integer vectors (vec2u, vec3u, vec4u).
/// Note that for integers some operations, such as as calculating the length, may give unexpected
/// results due to truncation or overflow.
///
/// 2, 3 and 4 dimensional vectors are specialized to have more constructors and ways of accessing
/// data. For example, you can construct a vec3 with 3 floats (vec3(x, y, z)), or with a vec2 and a 
/// float (vec3(vec2(x,y), z) or vec3(x, vec2(y, z))). To access the x value of a vec3 v you can
/// write v[0], v.elements[0] or v.x, you can also access two adjacent elements as a vector by
/// writing v.xy or v.yz.
///
/// Satisfies the conditions of std::is_pod, std::is_trivial and std::is_standard_layout if used
/// with standard primitives.

namespace sfz {

using std::int32_t;
using std::uint32_t;

// Vector struct declaration
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
struct Vector final {

	T elements[N];

	Vector() noexcept = default;
	Vector(const Vector<T,N>&) noexcept = default;
	Vector<T,N>& operator= (const Vector<T,N>&) noexcept = default;
	~Vector() noexcept = default;

	SFZ_CUDA_CALL explicit Vector(const T* arrayPtr) noexcept;

	template<typename T2>
	SFZ_CUDA_CALL explicit Vector(const Vector<T2,N>& other) noexcept;

	SFZ_CUDA_CALL T& operator[] (uint32_t index) noexcept;
	SFZ_CUDA_CALL T operator[] (uint32_t index) const noexcept;

	SFZ_CUDA_CALL T* elementsPtr() noexcept;
	SFZ_CUDA_CALL const T* elementsPtr() const noexcept;
};

template<typename T>
struct Vector<T,2> final {

	T x, y;

	Vector() noexcept = default;
	Vector(const Vector<T,2>&) noexcept = default;
	Vector<T,2>& operator= (const Vector<T,2>&) noexcept = default;
	~Vector() noexcept = default;

	SFZ_CUDA_CALL explicit Vector(const T* arrayPtr) noexcept;
	SFZ_CUDA_CALL explicit Vector(T value) noexcept;
	SFZ_CUDA_CALL Vector(T x, T y) noexcept;

	template<typename T2>
	SFZ_CUDA_CALL explicit Vector(const Vector<T2,2>& other) noexcept;

	SFZ_CUDA_CALL T& operator[] (uint32_t index) noexcept;
	SFZ_CUDA_CALL T operator[] (uint32_t index) const noexcept;

	SFZ_CUDA_CALL T* elementsPtr() noexcept;
	SFZ_CUDA_CALL const T* elementsPtr() const noexcept;
};

template<typename T>
struct Vector<T,3> final {
	union {
		struct { T x, y, z; };
		struct { Vector<T,2> xy; };
		struct { T xAlias; Vector<T,2> yz; };
	};

	Vector() noexcept = default;
	Vector(const Vector<T,3>&) noexcept = default;
	Vector<T,3>& operator= (const Vector<T,3>&) noexcept = default;
	~Vector() noexcept = default;

	SFZ_CUDA_CALL explicit Vector(const T* arrayPtr) noexcept;
	SFZ_CUDA_CALL explicit Vector(T value) noexcept;
	SFZ_CUDA_CALL Vector(T x, T y, T z) noexcept;
	SFZ_CUDA_CALL Vector(Vector<T,2> xy, T z) noexcept;
	SFZ_CUDA_CALL Vector(T x, Vector<T,2> yz) noexcept;

	template<typename T2>
	SFZ_CUDA_CALL explicit Vector(const Vector<T2,3>& other) noexcept;

	SFZ_CUDA_CALL T& operator[] (uint32_t index) noexcept;
	SFZ_CUDA_CALL T operator[] (uint32_t index) const noexcept;

	SFZ_CUDA_CALL T* elementsPtr() noexcept;
	SFZ_CUDA_CALL const T* elementsPtr() const noexcept;
};

template<typename T>
struct alignas(sizeof(T) * 4) Vector<T,4> final {
	union {
		struct { T x, y, z, w; };
		struct { Vector<T,3> xyz; };
		struct { T xAlias1; Vector<T,3> yzw; };
		struct { Vector<T,2> xy, zw; };
		struct { T xAlias2; Vector<T,2> yz; };
	};

	Vector() noexcept = default;
	Vector(const Vector<T,4>&) noexcept = default;
	Vector<T,4>& operator= (const Vector<T,4>&) noexcept = default;
	~Vector() noexcept = default;

	SFZ_CUDA_CALL explicit Vector(const T* arrayPtr) noexcept;
	SFZ_CUDA_CALL explicit Vector(T value) noexcept;
	SFZ_CUDA_CALL Vector(T x, T y, T z, T w) noexcept;
	SFZ_CUDA_CALL Vector(Vector<T,3> xyz, T w) noexcept;
	SFZ_CUDA_CALL Vector(T x, Vector<T,3> yzw) noexcept;
	SFZ_CUDA_CALL Vector(Vector<T,2> xy, Vector<T,2> zw) noexcept;
	SFZ_CUDA_CALL Vector(Vector<T,2> xy, T z, T w) noexcept;
	SFZ_CUDA_CALL Vector(T x, Vector<T,2> yz, T w) noexcept;
	SFZ_CUDA_CALL Vector(T x, T y, Vector<T,2> zw) noexcept;

	template<typename T2>
	SFZ_CUDA_CALL explicit Vector(const Vector<T2,4>& other) noexcept;

	SFZ_CUDA_CALL T& operator[] (uint32_t index) noexcept;
	SFZ_CUDA_CALL T operator[] (uint32_t index) const noexcept;

	SFZ_CUDA_CALL T* elementsPtr() noexcept;
	SFZ_CUDA_CALL const T* elementsPtr() const noexcept;
};

using vec2 = Vector<float,2>;
using vec3 = Vector<float,3>;
using vec4 = Vector<float,4>;

using vec2i = Vector<int32_t,2>;
using vec3i = Vector<int32_t,3>;
using vec4i = Vector<int32_t,4>;

using vec2u = Vector<uint32_t,2>;
using vec3u = Vector<uint32_t,3>;
using vec4u = Vector<uint32_t,4>;

// Vector functions
// ------------------------------------------------------------------------------------------------

/// Calculates length of the vector
template<typename T, uint32_t N>
SFZ_CUDA_CALL T length(const Vector<T,N>& vector) noexcept;

/// Calculates squared length of vector
template<typename T, uint32_t N>
SFZ_CUDA_CALL T squaredLength(const Vector<T,N>& vector) noexcept;

/// Normalizes vector
/// sfz_assert_debug: length of vector is not zero
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> normalize(const Vector<T,N>& vector) noexcept;

/// Normalizes vector, returns zero if vector is zero
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> safeNormalize(const Vector<T,N>& vector) noexcept;

/// Calculates the dot product of two vectors
template<typename T, uint32_t N>
SFZ_CUDA_CALL T dot(const Vector<T,N>& left, const Vector<T,N>& right) noexcept;

/// Calculates the cross product of two vectors
template<typename T>
SFZ_CUDA_CALL Vector<T,3> cross(const Vector<T,3>& left, const Vector<T,3>& right) noexcept;

/// Calculates the sum of all the elements in the vector
template<typename T, uint32_t N>
SFZ_CUDA_CALL T elementSum(const Vector<T,N>& vector) noexcept;

/// Returns the element-wise minimum of two vectors.
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> min(const Vector<T,N>& left, const Vector<T,N>& right) noexcept;

/// Returns the element-wise maximum of two vectors.
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> max(const Vector<T,N>& left, const Vector<T,N>& right) noexcept;

/// Returns the element-wise minimum of a vector and a scalar.
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> min(const Vector<T,N>& vector, T scalar) noexcept;
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> min(T scalar, const Vector<T,N>& vector) noexcept;

/// Returns the element-wise maximum of a vector and a scalar.
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> max(const Vector<T,N>& vector, T scalar) noexcept;
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> max(T scalar, const Vector<T,N>& vector) noexcept;

/// Returns the smallest element in a vector (as defined by the min function)
template<typename T, uint32_t N>
SFZ_CUDA_CALL T minElement(const Vector<T,N>& vector) noexcept;

/// Returns the largest element in a vector (as defined by the max function)
template<typename T, uint32_t N>
SFZ_CUDA_CALL T maxElement(const Vector<T,N>& vector) noexcept;

// Operators (arithmetic & assignment)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator+= (Vector<T,N>& left, const Vector<T,N>& right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator-= (Vector<T,N>& left, const Vector<T,N>& right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator*= (Vector<T,N>& left, T right) noexcept;

/// Element-wise multiplication assignment
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator*= (Vector<T,N>& left, const Vector<T,N>& right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator/= (Vector<T,N>& left, T right) noexcept;

/// Element-wise division assignment
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator/= (Vector<T,N>& left, const Vector<T,N>& right) noexcept;

// Operators (arithmetic)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator+ (const Vector<T,N>& left, const Vector<T,N>& right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator- (const Vector<T,N>& left, const Vector<T,N>& right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator- (const Vector<T,N>& vector) noexcept;

/// Element-wise multiplication of two vectors
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator* (const Vector<T,N>& left, const Vector<T,N>& right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator* (const Vector<T,N>& left, T right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator* (T left, const Vector<T,N>& right) noexcept;

/// Element-wise division of two vectors
template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator/ (const Vector<T,N>& left, const Vector<T,N>& right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator/ (const Vector<T,N>& left, T right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator/ (T left, const Vector<T,N>& right) noexcept;

// Operators (comparison)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
SFZ_CUDA_CALL bool operator== (const Vector<T,N>& left, const Vector<T,N>& right) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL bool operator!= (const Vector<T,N>& left, const Vector<T,N>& right) noexcept;

// Standard iterator functions
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
T* begin(Vector<T,N>& vector) noexcept;

template<typename T, uint32_t N>
const T* begin(const Vector<T,N>& vector) noexcept;

template<typename T, uint32_t N>
const T* cbegin(const Vector<T,N>& vector) noexcept;

template<typename T, uint32_t N>
T* end(Vector<T,N>& vector) noexcept;

template<typename T, uint32_t N>
const T* end(const Vector<T,N>& vector) noexcept;

template<typename T, uint32_t N>
const T* cend(const Vector<T,N>& vector) noexcept;

} // namespace sfz

#include "sfz/math/Vector.inl"
