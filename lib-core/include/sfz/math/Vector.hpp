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

#include "sfz/math/MinMax.hpp"

namespace sfz {

// Vector struct declaration
// ------------------------------------------------------------------------------------------------

// A mathematical vector POD class that imitates a built-in primitive.
//
// 2, 3 and 4 dimensional vectors are specialized to have more constructors and ways of accessing
// data. For example, you can construct a vec3 with 3 floats (vec3(x, y, z)), or with a vec2 and a
// float (vec3(vec2(x,y), z) or vec3(x, vec2(y, z))). To access the x value of a vec3 v you can
// write v[0], v.data()[0] or v.x, you can also access two adjacent elements as a vector by
// writing v.xy or v.yz.
//
// Satisfies the conditions of std::is_pod, std::is_trivial and std::is_standard_layout if used
// with standard primitives.

template<typename T, uint32_t N>
struct Vector final {

	T elements[N];

	constexpr T* data() { return elements; }
	constexpr const T* data() const { return elements; }

	Vector() noexcept = default;
	Vector(const Vector<T,N>&) noexcept = default;
	Vector<T,N>& operator= (const Vector<T,N>&) noexcept = default;
	~Vector() noexcept = default;

	constexpr explicit Vector(const T* arrayPtr)
	{
		for (uint32_t i = 0; i < N; i++) this->elements[i] = arrayPtr[i];
	}

	template<typename T2>
	constexpr explicit Vector(const Vector<T2,N>& other)
	{
		for (uint32_t i = 0; i < N; i++) elements[i] = T(other[i]);
	}

	constexpr T& operator[] (uint32_t index) { return elements[index]; }
	constexpr T operator[] (uint32_t index) const { return elements[index]; }

	constexpr Vector& operator+= (Vector o) { for (uint32_t i = 0; i < N; i++) *this[i] += o[i]; return *this; }
	constexpr Vector& operator-= (Vector o) { for (uint32_t i = 0; i < N; i++) *this[i] -= o[i]; return *this; }
	constexpr Vector& operator*= (T s) { for (uint32_t i = 0; i < N; i++) *this[i] *= s; return *this; }
	constexpr Vector& operator*= (Vector o) { for (uint32_t i = 0; i < N; i++) *this[i] *= o[i]; return *this; }
	constexpr Vector& operator/= (T s) { for (uint32_t i = 0; i < N; i++) *this[i] /= s; return *this; }
	constexpr Vector& operator/= (Vector o) { for (uint32_t i = 0; i < N; i++) *this[i] /= o[i]; return *this; }

	constexpr Vector operator- () const
	{
		Vector tmp;
		for (uint32_t i = 0; i < N; i++) tmp[i] = -(*this[i]);
		return tmp;
	}

	constexpr bool operator== (Vector o) const
	{
		bool res = true;
		for (uint32_t i = 0; i < N; i++) res = res && (*this[i] != o[i]);
		return res;
	}
	constexpr bool operator!= (Vector o) const { return !(*this == o); }
};

template<typename T>
struct Vector<T,2> final {

	T x, y;

	constexpr T* data() noexcept { return &x; }
	constexpr const T* data() const noexcept { return &x; }

	Vector() noexcept = default;
	Vector(const Vector<T,2>&) noexcept = default;
	Vector<T,2>& operator= (const Vector<T,2>&) noexcept = default;
	~Vector() noexcept = default;

	constexpr explicit Vector(const T* ptr) : x(ptr[0]), y(ptr[1]) { }
	constexpr explicit Vector(T val) : x(val), y(val) { }
	constexpr Vector(T x, T y) : x(x), y(y) { }

	template<typename T2>
	constexpr explicit Vector(Vector<T2,2> o) : x(T(o.x)), y(T(o.y)) { }

	constexpr T& operator[] (uint32_t index) { return data()[index]; }
	constexpr T operator[] (uint32_t index) const { return data()[index]; }

	constexpr Vector& operator+= (Vector o) { x += o.x; y += o.y; return *this; }
	constexpr Vector& operator-= (Vector o) { x -= o.x; y -= o.y; return *this; }
	constexpr Vector& operator*= (T s) { x *= s; y *= s; return *this; }
	constexpr Vector& operator*= (Vector o) { x *= o.x; y *= o.y; return *this; }
	constexpr Vector& operator/= (T s) { x /= s; y /= s; return *this; }
	constexpr Vector& operator/= (Vector o) { x /= o.x; y /= o.y; return *this; }

	constexpr Vector operator- () const { return Vector(-x, -y); }

	constexpr bool operator== (Vector o) const { return x == o.x && y == o.y; }
	constexpr bool operator!= (Vector o) const { return !(*this == o); }
};

template<typename T>
struct Vector<T,3> final {

	union {
		struct { T x, y, z; };
		struct { Vector<T,2> xy; };
		struct { T xAlias; Vector<T,2> yz; };
	};

	constexpr T* data() { return &x; }
	constexpr const T* data() const { return &x; }

	Vector() noexcept = default;
	Vector(const Vector<T,3>&) noexcept = default;
	Vector<T,3>& operator= (const Vector<T,3>&) noexcept = default;
	~Vector() noexcept = default;

	constexpr explicit Vector(const T* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) { }
	constexpr explicit Vector(T val) : x(val), y(val), z(val) { }
	constexpr Vector(T x, T y, T z) : x(x), y(y), z(z) { }
	constexpr Vector(Vector<T,2> xy, T z) : x(xy.x), y(xy.y), z(z) { }
	constexpr Vector(T x, Vector<T,2> yz) : x(x), y(yz.x), z(yz.y) { }

	template<typename T2>
	constexpr explicit Vector(Vector<T2,3> o) : x(T(o.x)), y(T(o.y)), z(T(o.z)) { }

	constexpr T& operator[] (uint32_t index) { return data()[index]; }
	constexpr T operator[] (uint32_t index) const { return data()[index]; }

	constexpr Vector& operator+= (Vector o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr Vector& operator-= (Vector o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr Vector& operator*= (T s) { x *= s; y *= s; z *= s; return *this; }
	constexpr Vector& operator*= (Vector o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr Vector& operator/= (T s) { x /= s; y /= s; z /= s; return *this; }
	constexpr Vector& operator/= (Vector o) { x /= o.x; y /= o.y; z /= o.z; return *this; }

	constexpr Vector operator- () const { return Vector(-x, -y, -z); }

	constexpr bool operator== (Vector o) const { return x == o.x && y == o.y && z == o.z; }
	constexpr bool operator!= (Vector o) const { return !(*this == o); }
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

	constexpr T* data() noexcept { return &x; }
	constexpr const T* data() const noexcept { return &x; }

	Vector() noexcept = default;
	Vector(const Vector<T,4>&) noexcept = default;
	Vector<T,4>& operator= (const Vector<T,4>&) noexcept = default;
	~Vector() noexcept = default;

	constexpr explicit Vector(const T* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]), w(ptr[3]) { }
	constexpr explicit Vector(T val) : x(val), y(val), z(val), w(val) { }
	constexpr Vector(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
	constexpr Vector(Vector<T,3> xyz, T w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) { }
	constexpr Vector(T x, Vector<T,3> yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z) { }
	constexpr Vector(Vector<T,2> xy, Vector<T,2> zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) { }
	constexpr Vector(Vector<T,2> xy, T z, T w) : x(xy.x), y(xy.y), z(z), w(w) { }
	constexpr Vector(T x, Vector<T,2> yz, T w) : x(x), y(yz.x), z(yz.y), w(w) { }
	constexpr Vector(T x, T y, Vector<T,2> zw) : x(x), y(y), z(zw.x), w(zw.y) { }

	template<typename T2>
	constexpr explicit Vector(Vector<T2,4> o) : x(T(o.x)), y(T(o.y)), z(T(o.z)), w(T(o.w)) { }

	constexpr T& operator[] (uint32_t index) { return data()[index]; }
	constexpr T operator[] (uint32_t index) const { return data()[index]; }

	constexpr Vector& operator+= (Vector o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr Vector& operator-= (Vector o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr Vector& operator*= (T s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr Vector& operator*= (Vector o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr Vector& operator/= (T s) { x /= s; y /= s; z /= s; w /= s; return *this; }
	constexpr Vector& operator/= (Vector o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }

	constexpr Vector operator- () const { return Vector(-x, -y, -z, -w); }

	constexpr bool operator== (Vector o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (Vector o) const { return !(*this == o); }
};

// Typedefs
// ------------------------------------------------------------------------------------------------

using vec2 = Vector<float,2>;
using vec3 = Vector<float,3>;
using vec4 = Vector<float,4>;
static_assert(sizeof(vec2) == sizeof(float) * 2, "vec2 is padded");
static_assert(sizeof(vec3) == sizeof(float) * 3, "vec3 is padded");
static_assert(sizeof(vec4) == sizeof(float) * 4, "vec4 is padded");
static_assert(alignof(vec4) == 16, "vec4 is not 16-byte aligned");

using vec2_s32 = Vector<int32_t,2>;
using vec3_s32 = Vector<int32_t,3>;
using vec4_s32 = Vector<int32_t,4>;
static_assert(sizeof(vec2_s32) == sizeof(int32_t) * 2, "vec2_s32 is padded");
static_assert(sizeof(vec3_s32) == sizeof(int32_t) * 3, "vec3_s32 is padded");
static_assert(sizeof(vec4_s32) == sizeof(int32_t) * 4, "vec4_s32 is padded");
static_assert(alignof(vec4_s32) == 16, "vec4_s32 is not 16-byte aligned");

using vec2_u32 = Vector<uint32_t,2>;
using vec3_u32 = Vector<uint32_t,3>;
using vec4_u32 = Vector<uint32_t,4>;
static_assert(sizeof(vec2_u32) == sizeof(uint32_t) * 2, "vec2_u32 is padded");
static_assert(sizeof(vec3_u32) == sizeof(uint32_t) * 3, "vec3_u32 is padded");
static_assert(sizeof(vec4_u32) == sizeof(uint32_t) * 4, "vec4_u32 is padded");
static_assert(alignof(vec4_u32) == 16, "vec4_u32 is not 16-byte aligned");

using vec2_s8 = Vector<int8_t,2>;
using vec3_s8 = Vector<int8_t,3>;
using vec4_s8 = Vector<int8_t,4>;
static_assert(sizeof(vec2_s8) == sizeof(int8_t) * 2, "vec2_s8 is padded");
static_assert(sizeof(vec3_s8) == sizeof(int8_t) * 3, "vec3_s8 is padded");
static_assert(sizeof(vec4_s8) == sizeof(int8_t) * 4, "vec4_s8 is padded");
static_assert(alignof(vec4_s8) == 4, "vec4_s8 is not 4-byte aligned");

using vec2_u8 = Vector<uint8_t,2>;
using vec3_u8 = Vector<uint8_t,3>;
using vec4_u8 = Vector<uint8_t,4>;
static_assert(sizeof(vec2_u8) == sizeof(uint8_t) * 2, "vec2_u8 is padded");
static_assert(sizeof(vec3_u8) == sizeof(uint8_t) * 3, "vec3_u8 is padded");
static_assert(sizeof(vec4_u8) == sizeof(uint8_t) * 4, "vec4_u8 is padded");
static_assert(alignof(vec4_u8) == 4, "vec4_u8 is not 4-byte aligned");

// Free-standing operators
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
constexpr Vector<T,N> operator+ (Vector<T,N> l, Vector<T,N> r) { return l += r; }

template<typename T, uint32_t N>
constexpr Vector<T,N> operator- (Vector<T,N> l, Vector<T,N> r) { return l -= r; }

template<typename T, uint32_t N>
constexpr Vector<T,N> operator* (Vector<T,N> l, Vector<T,N> r) { return l *= r; }

template<typename T, uint32_t N>
constexpr Vector<T,N> operator* (Vector<T,N> l, T r) { return l *= r; }

template<typename T, uint32_t N>
constexpr Vector<T,N> operator* (T l, Vector<T,N> r) { return r * l; }

template<typename T, uint32_t N>
constexpr Vector<T,N> operator/ (Vector<T,N> l, Vector<T,N> r) { return l /= r; }

template<typename T, uint32_t N>
constexpr Vector<T,N> operator/ (Vector<T,N> l, T r) { return l /= r; }

template<typename T, uint32_t N>
constexpr Vector<T,N> operator/ (T l, Vector<T,N> r) { return r / l; }

// Dot & cross products
// ------------------------------------------------------------------------------------------------

template<typename T>
constexpr T dot(Vector<T,2> l, Vector<T,2> r) { return l.x * r.x + l.y * r.y; }

template<typename T>
constexpr T dot(Vector<T,3> l, Vector<T,3> r) { return l.x * r.x + l.y * r.y + l.z * r.z; }

template<typename T>
constexpr T dot(Vector<T,4> l, Vector<T,4> r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }

template<typename T>
constexpr Vector<T,3> cross(Vector<T,3> l, Vector<T,3> r)
{
	return Vector<T,3>(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x);
}

// Other functions
// ------------------------------------------------------------------------------------------------

inline float length(vec2 v) { return std::sqrt(dot(v, v)); }
inline float length(vec3 v) { return std::sqrt(dot(v, v)); }
inline float length(vec4 v) { return std::sqrt(dot(v, v)); }

inline vec2 normalize(vec2 v) { return v * (1.0f / length(v)); }
inline vec3 normalize(vec3 v) { return v * (1.0f / length(v)); }
inline vec4 normalize(vec4 v) { return v * (1.0f / length(v)); }

inline vec2 safeNormalize(vec2 v) { float tmp = length(v); return (tmp == 0.0f) ? v : (v / tmp); }
inline vec3 safeNormalize(vec3 v) { float tmp = length(v); return (tmp == 0.0f) ? v : (v / tmp); }
inline vec4 safeNormalize(vec4 v) { float tmp = length(v); return (tmp == 0.0f) ? v : (v / tmp); }

} // namespace sfz

// Vector overloads of sfzMin() and sfzMax()
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMin(sfz::Vector<T,N> lhs, sfz::Vector<T,N> rhs)
{
	sfz::Vector<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) tmp[i] = sfzMin(lhs[i], rhs[i]);
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMin(T lhs, sfz::Vector<T,N> rhs)
{
	sfz::Vector<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) tmp[i] = sfzMin(lhs, rhs[i]);
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMin(sfz::Vector<T,N> lhs, T rhs)
{
	return sfzMin(rhs, lhs);
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMax(sfz::Vector<T,N> lhs, sfz::Vector<T,N> rhs)
{
	sfz::Vector<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) tmp[i] = sfzMax(lhs[i], rhs[i]);
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMax(T lhs, sfz::Vector<T,N> rhs)
{
	sfz::Vector<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) tmp[i] = sfzMax(lhs, rhs[i]);
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMax(sfz::Vector<T,N> lhs, T rhs)
{
	return sfzMax(rhs, lhs);
}
