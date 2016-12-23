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
#include <cmath> // std::sqrt

#include "sfz/CudaCompatibility.hpp"
#include "sfz/SimdIntrinsics.hpp"

namespace sfz {

using std::int32_t;
using std::uint32_t;

// Vector struct declaration
// ------------------------------------------------------------------------------------------------

/// A mathematical vector POD class that imitates a built-in primitive.
///
/// Typedefs are provided for float vectors (vec2, vec3 and vec4), (32-bit signed) integer
/// vectors (vec2i, vec3i, vec4i) and (32-bit) unsigned integer vectors (vec2u, vec3u, vec4u).
///
/// 2, 3 and 4 dimensional vectors are specialized to have more constructors and ways of accessing
/// data. For example, you can construct a vec3 with 3 floats (vec3(x, y, z)), or with a vec2 and a 
/// float (vec3(vec2(x,y), z) or vec3(x, vec2(y, z))). To access the x value of a vec3 v you can
/// write v[0], v.elements[0] or v.x, you can also access two adjacent elements as a vector by
/// writing v.xy or v.yz.
///
/// Satisfies the conditions of std::is_pod, std::is_trivial and std::is_standard_layout if used
/// with standard primitives.

template<typename T, uint32_t N>
struct Vector final {

	T elements[N];

	SFZ_CUDA_CALL T* data() noexcept { return elements; }
	SFZ_CUDA_CALL const T* data() const noexcept { return elements; }

	Vector() noexcept = default;
	Vector(const Vector<T,N>&) noexcept = default;
	Vector<T,N>& operator= (const Vector<T,N>&) noexcept = default;
	~Vector() noexcept = default;

	SFZ_CUDA_CALL explicit Vector(const T* arrayPtr) noexcept;

	template<typename T2>
	SFZ_CUDA_CALL explicit Vector(const Vector<T2,N>& other) noexcept;

	SFZ_CUDA_CALL T& operator[] (uint32_t index) noexcept;
	SFZ_CUDA_CALL T operator[] (uint32_t index) const noexcept;
};

template<typename T>
struct Vector<T,2> final {

	T x, y;

	SFZ_CUDA_CALL T* data() noexcept { return &x; }
	SFZ_CUDA_CALL const T* data() const noexcept { return &x; }

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
};

template<typename T>
struct Vector<T,3> final {
	union {
		struct { T x, y, z; };
		struct { Vector<T,2> xy; };
		struct { T xAlias; Vector<T,2> yz; };
	};

	SFZ_CUDA_CALL T* data() noexcept { return &x; }
	SFZ_CUDA_CALL const T* data() const noexcept { return &x; }

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

	SFZ_CUDA_CALL T* data() noexcept { return &x; }
	SFZ_CUDA_CALL const T* data() const noexcept { return &x; }

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

static_assert(sizeof(vec2) == 8, "vec2 is padded");
static_assert(sizeof(vec3) == 12, "vec3 is padded");
static_assert(sizeof(vec4) == 16, "vec4 is padded");

static_assert(sizeof(vec2i) == 8, "vec2i is padded");
static_assert(sizeof(vec3i) == 12, "vec3i is padded");
static_assert(sizeof(vec4i) == 16, "vec4i is padded");

static_assert(sizeof(vec2u) == 8, "vec2u is padded");
static_assert(sizeof(vec3u) == 12, "vec3u is padded");
static_assert(sizeof(vec4u) == 16, "vec4u is padded");

// Vector functions
// ------------------------------------------------------------------------------------------------

/// Calculates the dot product of two vectors
template<typename T, uint32_t N>
SFZ_CUDA_CALL T dot(Vector<T,N> lhs, Vector<T,N> rhs) noexcept;

/// Calculates length of the vector
SFZ_CUDA_CALL float length(vec2 v) noexcept;
SFZ_CUDA_CALL float length(vec3 v) noexcept;
SFZ_CUDA_CALL float length(vec4 v) noexcept;

/// Normalizes vector
SFZ_CUDA_CALL vec2 normalize(vec2 v) noexcept;
SFZ_CUDA_CALL vec3 normalize(vec3 v) noexcept;
SFZ_CUDA_CALL vec4 normalize(vec4 v) noexcept;

/// Normalizes vector, returns zero if vector is zero. Not as optimized as normalize().
SFZ_CUDA_CALL vec2 safeNormalize(vec2 v) noexcept;
SFZ_CUDA_CALL vec3 safeNormalize(vec3 v) noexcept;
SFZ_CUDA_CALL vec4 safeNormalize(vec4 v) noexcept;

/// Calculates the cross product of two vectors
template<typename T>
SFZ_CUDA_CALL Vector<T,3> cross(Vector<T,3> lhs, Vector<T,3> rhs) noexcept;

/// Calculates the sum of all the elements in the vector
template<typename T, uint32_t N>
SFZ_CUDA_CALL T elementSum(Vector<T,N> v) noexcept;

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

} // namespace sfz

#include "sfz/math/Vector.inl"
