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

#ifndef SKIPIFZERO_HPP
#define SKIPIFZERO_HPP
#pragma once

#include <math.h> // std::sqrt, std::fmodf
#include <string.h> // memcpy()

#include "sfz.h"

#ifdef _WIN32
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4201) // nonstandard extension: nameless struct/union
#endif

#if defined(min) || defined(max)
#undef min
#undef max
#endif

namespace sfz {

// std::move() and std::forward() replacements
// ------------------------------------------------------------------------------------------------

// std::move() and std::forward() requires including a relatively expensive header (<utility>), and
// are fairly complex in their standard implementations. This is a significantly cheaper
// implementation that works for many use-cases.
//
// See: https://www.foonathan.net/2020/09/move-forward/

// Implementation of std::remove_reference_t
// See: https://en.cppreference.com/w/cpp/types/remove_reference
template<typename T> struct remove_ref { typedef T type; };
template<typename T> struct remove_ref<T&> { typedef T type; };
template<typename T> struct remove_ref<T&&> { typedef T type; };
template<typename T> using remove_ref_t = typename remove_ref<T>::type;

#define sfz_move(obj) static_cast<sfz::remove_ref_t<decltype(obj)>&&>(obj)
#define sfz_forward(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

// std::swap replacement
// ------------------------------------------------------------------------------------------------

// std::swap() is similarly to std::move() and std::forward() hidden in relatively expensive
// headers (<utility>), so we have our own simple implementation instead.

template<typename T>
void swap(T& lhs, T& rhs)
{
	T tmp = sfz_move(lhs);
	lhs = sfz_move(rhs);
	rhs = sfz_move(tmp);
}

// Memory functions
// ------------------------------------------------------------------------------------------------

// Swaps size bytes of memory between two buffers. Undefined behaviour if the buffers overlap, with
// the exception that its safe to call if both pointers are the same (i.e. point to the same buffer).
inline void memswp(void* __restrict a, void* __restrict b, u64 size)
{
	const u64 MEMSWP_TMP_BUFFER_SIZE = 64;
	u8 tmpBuffer[MEMSWP_TMP_BUFFER_SIZE];

	// Swap buffers in temp buffer sized chunks
	u64 bytesLeft = size;
	while (bytesLeft >= MEMSWP_TMP_BUFFER_SIZE) {
		memcpy(tmpBuffer, a, MEMSWP_TMP_BUFFER_SIZE);
		memcpy(a, b, MEMSWP_TMP_BUFFER_SIZE);
		memcpy(b, tmpBuffer, MEMSWP_TMP_BUFFER_SIZE);
		a = reinterpret_cast<u8*>(a) + MEMSWP_TMP_BUFFER_SIZE;
		b = reinterpret_cast<u8*>(b) + MEMSWP_TMP_BUFFER_SIZE;
		bytesLeft -= MEMSWP_TMP_BUFFER_SIZE;
	}

	// Swap remaining bytes
	if (bytesLeft > 0) {
		memcpy(tmpBuffer, a, bytesLeft);
		memcpy(a, b, bytesLeft);
		memcpy(b, tmpBuffer, bytesLeft);
	}
}

// Checks whether an u64 is a power of two or not
constexpr bool isPowerOfTwo(u64 value)
{
	// See https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
	return value != 0 && (value & (value - 1)) == 0;
}

// Checks whether a pointer is aligned to a given byte aligment
inline bool isAligned(const void* pointer, u64 alignment) noexcept
{
	sfz_assert(isPowerOfTwo(alignment));
	return ((uintptr_t)pointer & (alignment - 1)) == 0;
}

// Rounds up a given value so that it is evenly divisible by the given alignment
constexpr u64 roundUpAligned(u64 value, u64 alignment)
{
	return ((value + alignment - 1) / alignment) * alignment;
}

// Gives the offset needed to make the given value evenly divisible by the given alignment
constexpr u64 alignedDiff(u64 value, u64 alignment)
{
	return roundUpAligned(value, alignment) - value;
}

// DropType
// ------------------------------------------------------------------------------------------------

// A DropType is a type that is default constructible and move:able, but not copy:able.
//
// It must implement "void destroy()", which must destroy all members and reset the state of the
// type to the same state as if it was default constructed. It should be safe to call destroy()
// multiple times in a row.
//
// The move semantics of DropType's are implemented using sfz::memswp(), which means that all
// members of a DropType MUST be either trivially copyable (i.e. primitives such as integers,
// floats and pointers) or DropTypes themselves. This is currently not enforced, be careful.
//
// Usage:
//
// class SomeType final {
// public:
//     SFZ_DECLARE_DROP_TYPE(SomeType);
//     void destroy() { /* ... */ }
//     // ...
// };
#define SFZ_DECLARE_DROP_TYPE(T) \
	T() = default; \
	T(const T&) = delete; \
	T& operator= (const T&) = delete; \
	T(T&& other) noexcept { this->swap(other); } \
	T& operator= (T&& other) noexcept { this->swap(other); return *this; } \
	void swap(T& other) noexcept { sfz::memswp(this, &other, sizeof(T)); } \
	~T() noexcept { this->destroy(); }

// Alternate type definition
// ------------------------------------------------------------------------------------------------

struct NO_ALT_TYPE final { NO_ALT_TYPE() = delete; };

// Defines an alternate type for a given type. Mainly used to define alternate key types for hash
// maps. E.g., for a string type "const char*" can be defined as an alternate key type.
//
// Requirements of an alternate type:
//  * operator== (T, AltT) must be defined
//  * sfz::hash(T) and sfz::hash(AltT) must be defined
//  * sfz::hash(T) == sfz::hash(AltT)
//  * constructor T(AltT) must be defined
template<typename T>
struct AltType final {
	using AltT = NO_ALT_TYPE;
};

// Vector primitives
// ------------------------------------------------------------------------------------------------

// 2, 3 and 4-dimensional vector primitives that imitates built-in primitives.
//
// Functions very similar to GLSL vectors. Swizzling is not supported, but it is possible to access
// vector elements in different ways thanks to the union + nameless struct trick. E.g., you can
// access the last two elements of a f32x3 with v.yz.
//
// There are typedefs available for the primary primitives meant to be used. You should normally
// only use these typedefs and not the template explicitly, unless you have a very specific use-case
// which requires it.

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
	constexpr Vec& operator%= (T s) { x %= s; y %= s; return *this; }
	constexpr Vec& operator%= (Vec o) { x %= o.x; y %= o.y; return *this; }

	constexpr Vec operator+ (Vec o) const { return Vec(*this) += o; }
	constexpr Vec operator- (Vec o) const { return Vec(*this) -= o; }
	constexpr Vec operator- () const { return Vec(-x, -y); }
	constexpr Vec operator* (Vec o) const { return Vec(*this) *= o; }
	constexpr Vec operator* (T s) const { return Vec(*this) *= s; }
	constexpr Vec operator/ (Vec o) const { return Vec(*this) /= o; }
	constexpr Vec operator/ (T s) const { return Vec(*this) /= s; }
	constexpr Vec operator% (Vec o) const { return Vec(*this) %= o; }
	constexpr Vec operator% (T s) const { return Vec(*this) %= s; }

	constexpr bool operator== (Vec o) const { return x == o.x && y == o.y; }
	constexpr bool operator!= (Vec o) const { return !(*this == o); }
};

using f32x2 = Vec<f32, 2>;  static_assert(sizeof(f32x2) == sizeof(f32) * 2, "");
using i32x2 = Vec<i32, 2>;  static_assert(sizeof(i32x2) == sizeof(i32) * 2, "");
using u8x2 =  Vec<u8, 2>;   static_assert(sizeof(u8x2) == sizeof(u8) * 2, "");

template<typename T>
struct Vec<T,3> final {

	union {
		struct { T x, y, z; };
		struct { Vec<T,2> xy; };
		struct { T xAlias; Vec<T,2> yz; };
	};

	constexpr Vec() noexcept = default;
	constexpr Vec(const Vec<T,3>&) noexcept = default;
	constexpr Vec<T,3>& operator= (const Vec<T,3>&) noexcept = default;
	~Vec() noexcept = default;

	constexpr explicit Vec(const T* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) { }
	constexpr explicit Vec(T val) : x(val), y(val), z(val) { }
	constexpr Vec(T x, T y, T z) : x(x), y(y), z(z) { }
	constexpr Vec(Vec<T,2> xy, T z) : x(xy.x), y(xy.y), z(z) { }
	constexpr Vec(T x, Vec<T,2> yz) : x(x), y(yz.x), z(yz.y) { }

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
	constexpr Vec& operator%= (T s) { x %= s; y %= s; z %= s; return *this; }
	constexpr Vec& operator%= (Vec o) { x %= o.x; y %= o.y; z %= o.z; return *this; }

	constexpr Vec operator+ (Vec o) const { return Vec(*this) += o; }
	constexpr Vec operator- (Vec o) const { return Vec(*this) -= o; }
	constexpr Vec operator- () const { return Vec(-x, -y, -z); }
	constexpr Vec operator* (Vec o) const { return Vec(*this) *= o; }
	constexpr Vec operator* (T s) const { return Vec(*this) *= s; }
	constexpr Vec operator/ (Vec o) const { return Vec(*this) /= o; }
	constexpr Vec operator/ (T s) const { return Vec(*this) /= s; }
	constexpr Vec operator% (Vec o) const { return Vec(*this) %= o; }
	constexpr Vec operator% (T s) const { return Vec(*this) %= s; }

	constexpr bool operator== (Vec o) const { return x == o.x && y == o.y && z == o.z; }
	constexpr bool operator!= (Vec o) const { return !(*this == o); }
};

using f32x3 = Vec<f32, 3>;  static_assert(sizeof(f32x3) == sizeof(f32) * 3, "");
using i32x3 = Vec<i32, 3>;  static_assert(sizeof(i32x3) == sizeof(i32) * 3, "");

template<typename T>
struct Vec<T,4> final {

	union {
		struct { T x, y, z, w; };
		struct { Vec<T,3> xyz; };
		struct { T xAlias1; Vec<T,3> yzw; };
		struct { Vec<T,2> xy, zw; };
		struct { T xAlias2; Vec<T,2> yz; };
	};

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
	constexpr Vec& operator%= (T s) { x %= s; y %= s; z %= s; w %= s; return *this; }
	constexpr Vec& operator%= (Vec o) { x %= o.x; y %= o.y; z %= o.z; w %= o.w; return *this; }

	constexpr Vec operator+ (Vec o) const { return Vec(*this) += o; }
	constexpr Vec operator- (Vec o) const { return Vec(*this) -= o; }
	constexpr Vec operator- () const { return Vec(-x, -y, -z, -w); }
	constexpr Vec operator* (Vec o) const { return Vec(*this) *= o; }
	constexpr Vec operator* (T s) const { return Vec(*this) *= s; }
	constexpr Vec operator/ (Vec o) const { return Vec(*this) /= o; }
	constexpr Vec operator/ (T s) const { return Vec(*this) /= s; }
	constexpr Vec operator% (Vec o) const { return Vec(*this) %= o; }
	constexpr Vec operator% (T s) const { return Vec(*this) %= s; }

	constexpr bool operator== (Vec o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (Vec o) const { return !(*this == o); }
};

using f32x4 = Vec<f32, 4>;  static_assert(sizeof(f32x4) == sizeof(f32) * 4, "");
using i32x4 = Vec<i32, 4>;  static_assert(sizeof(i32x4) == sizeof(i32) * 4, "");
using u8x4 =  Vec<u8, 4>;   static_assert(sizeof(u8x4) == sizeof(u8) * 4, "");

template<typename T, u32 N>
constexpr Vec<T,N> operator* (T s, Vec<T,N> v) { return v * s; }

template<typename T, u32 N>
constexpr Vec<T,N> operator/ (T s, Vec<T,N> v) { return Vec<T,N>(s) / v; }

template<typename T, u32 N>
constexpr Vec<T,N> operator% (T s, Vec<T,N> v) { return Vec<T,N>(s) % v; }

template<typename T>
constexpr T dot(Vec<T,2> l, Vec<T,2> r) { return l.x * r.x + l.y * r.y; }

template<typename T>
constexpr T dot(Vec<T,3> l, Vec<T,3> r) { return l.x * r.x + l.y * r.y + l.z * r.z; }

template<typename T>
constexpr T dot(Vec<T,4> l, Vec<T,4> r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }

template<typename T>
constexpr Vec<T,3> cross(Vec<T,3> l, Vec<T,3> r)
{
	return Vec<T,3>(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x);
}

inline f32 length(f32x2 v) { return ::sqrtf(dot(v, v)); }
inline f32 length(f32x3 v) { return ::sqrtf(dot(v, v)); }
inline f32 length(f32x4 v) { return ::sqrtf(dot(v, v)); }

inline f32x2 normalize(f32x2 v) { return v * (1.0f / length(v)); }
inline f32x3 normalize(f32x3 v) { return v * (1.0f / length(v)); }
inline f32x4 normalize(f32x4 v) { return v * (1.0f / length(v)); }

inline f32x2 normalizeSafe(f32x2 v) { f32 tmp = length(v); return tmp == 0.0f ? v : v * (1.0f / tmp); }
inline f32x3 normalizeSafe(f32x3 v) { f32 tmp = length(v); return tmp == 0.0f ? v : v * (1.0f / tmp); }
inline f32x4 normalizeSafe(f32x4 v) { f32 tmp = length(v); return tmp == 0.0f ? v : v * (1.0f / tmp); }

template<typename T> constexpr T elemSum(Vec<T,2> v) { return v.x + v.y; }
template<typename T> constexpr T elemSum(Vec<T,3> v) { return v.x + v.y + v.z; }
template<typename T> constexpr T elemSum(Vec<T,4> v) { return v.x + v.y + v.z + v.w; }

constexpr f32 divSafe(f32 n, f32 d, f32 eps = 0.000'000'1f) { return d == 0.0f ? n / eps : n / d; }
constexpr f32x2 divSafe(f32x2 n, f32x2 d, f32 eps = 0.000'000'1f) { return f32x2(divSafe(n.x, d.x, eps), divSafe(n.y, d.y, eps)); }
constexpr f32x3 divSafe(f32x3 n, f32x3 d, f32 eps = 0.000'000'1f) { return f32x3(divSafe(n.x, d.x, eps), divSafe(n.y, d.y, eps), divSafe(n.z, d.z, eps)); }
constexpr f32x4 divSafe(f32x4 n, f32x4 d, f32 eps = 0.000'000'1f) { return f32x4(divSafe(n.x, d.x, eps), divSafe(n.y, d.y, eps), divSafe(n.z, d.z, eps), divSafe(n.w, d.w, eps)); }

// Math functions
// ------------------------------------------------------------------------------------------------

constexpr f32 PI = 3.14159265358979323846f;
constexpr f32 DEG_TO_RAD = PI / 180.0f;
constexpr f32 RAD_TO_DEG = 180.0f / PI;

constexpr f32 EQF_EPS = 0.001f;
constexpr bool eqf(f32 l, f32 r, f32 eps = EQF_EPS) { return (l <= (r + eps)) && (l >= (r - eps)); }
constexpr bool eqf(f32x2 l, f32x2 r, f32 eps = EQF_EPS) { return eqf(l.x, r.x, eps) && eqf(l.y, r.y, eps); }
constexpr bool eqf(f32x3 l, f32x3 r, f32 eps = EQF_EPS) { return eqf(l.xy, r.xy, eps) && eqf(l.z, r.z, eps); }
constexpr bool eqf(f32x4 l, f32x4 r, f32 eps = EQF_EPS) { return eqf(l.xyz, r.xyz, eps) && eqf(l.w, r.w, eps); }

constexpr f32 abs(f32 v) { return v >= 0.0f ? v : -v; }
constexpr f32x2 abs(f32x2 v) { return f32x2(sfz::abs(v.x), sfz::abs(v.y)); }
constexpr f32x3 abs(f32x3 v) { return f32x3(sfz::abs(v.xy), sfz::abs(v.z)); }
constexpr f32x4 abs(f32x4 v) { return f32x4(sfz::abs(v.xyz), sfz::abs(v.w)); }

constexpr i32 abs(i32 v) { return v >= 0 ? v : -v; }
constexpr i32x2 abs(i32x2 v) { return i32x2(sfz::abs(v.x), sfz::abs(v.y)); }
constexpr i32x3 abs(i32x3 v) { return i32x3(sfz::abs(v.xy),sfz::abs(v.z)); }
constexpr i32x4 abs(i32x4 v) { return i32x4(sfz::abs(v.xyz), sfz::abs(v.w)); }

constexpr f32 min(f32 l, f32 r) { return (l < r) ? l : r; }
constexpr i32 min(i32 l, i32 r) { return (l < r) ? l : r; }
constexpr u32 min(u32 l, u32 r) { return (l < r) ? l : r; }
constexpr u8 min(u8 l, u8 r) { return (l < r) ? l : r; }

template<typename T, u32 N>
constexpr Vec<T,N> min(Vec<T,N> l, Vec<T,N> r)
{
	Vec<T,N> tmp;
	for (u32 i = 0; i < N; i++) tmp[i] = sfz::min(l[i], r[i]);
	return tmp;
}

template<typename T, u32 N> constexpr Vec<T,N> min(Vec<T,N> l, T r) { return sfz::min(l, Vec<T,N>(r)); }
template<typename T, u32 N> constexpr Vec<T,N> min(T l, Vec<T,N> r) { return sfz::min(r, l); }

constexpr f32 max(f32 l, f32 r) { return (l < r) ? r : l; }
constexpr i32 max(i32 l, i32 r) { return (l < r) ? r : l; }
constexpr u32 max(u32 l, u32 r) { return (l < r) ? r : l; }
constexpr u8 max(u8 l, u8 r) { return (l < r) ? r : l; }

template<typename T, u32 N>
constexpr Vec<T,N> max(Vec<T,N> l, Vec<T,N> r)
{
	Vec<T,N> tmp;
	for (u32 i = 0; i < N; i++) tmp[i] = sfz::max(l[i], r[i]);
	return tmp;
}

template<typename T, u32 N> constexpr Vec<T,N> max(Vec<T,N> l, T r) { return sfz::max(l, Vec<T,N>(r)); }
template<typename T, u32 N> constexpr Vec<T,N> max(T l, Vec<T,N> r) { return sfz::max(r, l); }

template<typename T> constexpr T elemMax(Vec<T,2> v) { return sfz::max(v.x, v.y); }
template<typename T> constexpr T elemMax(Vec<T,3> v) { return sfz::max(sfz::max(v.x, v.y), v.z); }
template<typename T> constexpr T elemMax(Vec<T,4> v) { return sfz::max(sfz::max(sfz::max(v.x, v.y), v.z), v.w); }

template<typename T> constexpr T elemMin(Vec<T,2> v) { return sfz::min(v.x, v.y); }
template<typename T> constexpr T elemMin(Vec<T,3> v) { return sfz::min(sfz::min(v.x, v.y), v.z); }
template<typename T> constexpr T elemMin(Vec<T,4> v) { return sfz::min(sfz::min(sfz::min(v.x, v.y), v.z), v.w); }

template<typename ArgT, typename LimitT>
constexpr ArgT clamp(const ArgT& val, const LimitT& minVal, const LimitT& maxVal)
{
	return sfz::max(minVal, sfz::min(val, maxVal));
}

template<typename T> constexpr T sgn(T v) { return v < T(0) ? T(-1) : T(1); }
template<typename T> constexpr Vec<T,2> sgn(Vec<T,2> v) { return Vec<T,2> (sgn(v.x), sgn(v.y)); }
template<typename T> constexpr Vec<T,3> sgn(Vec<T,3> v) { return Vec<T,3> (sgn(v.x), sgn(v.y), sgn(v.z)); }
template<typename T> constexpr Vec<T,4> sgn(Vec<T,4> v) { return Vec<T,4> (sgn(v.x), sgn(v.y), sgn(v.z), sgn(v.w)); }

constexpr f32 saturate(f32 v) { return sfz::clamp(v, 0.0f, 1.0f); }
constexpr i32 saturate(i32 v) { return sfz::clamp(v, 0, 255); }
constexpr u32 saturate(u32 v) { return sfz::clamp(v, 0u, 255u); }

template<typename T> constexpr Vec<T,2> saturate(Vec<T,2> v) { return Vec<T,2>(sfz::saturate(v.x), sfz::saturate(v.y)); }
template<typename T> constexpr Vec<T,3> saturate(Vec<T,3> v) { return Vec<T,3>(sfz::saturate(v.xy), sfz::saturate(v.z)); }
template<typename T> constexpr Vec<T,4> saturate(Vec<T,4> v) { return Vec<T,4>(sfz::saturate(v.xyz), sfz::saturate(v.w)); }

constexpr f32 lerp(f32 v0, f32 v1, f32 t) { return (1.0f - t) * v0 + t * v1; }
constexpr f32x2 lerp(f32x2 v0, f32x2 v1, f32 t) { return (1.0f - t) * v0 + t * v1; }
constexpr f32x3 lerp(f32x3 v0, f32x3 v1, f32 t) { return (1.0f - t) * v0 + t * v1; }
constexpr f32x4 lerp(f32x4 v0, f32x4 v1, f32 t) { return (1.0f - t) * v0 + t * v1; }

inline f32 fmod(f32 n, f32 dnm) { return ::fmodf(n, dnm); }
inline f32x2 fmod(f32x2 n, f32 dnm) { return f32x2(fmod(n.x, dnm), fmod(n.y, dnm)); }
inline f32x2 fmod(f32x2 n, f32x2 dnm) { return f32x2(fmod(n.x, dnm.x), fmod(n.y, dnm.y)); }
inline f32x3 fmod(f32x3 n, f32 dnm) { return f32x3(fmod(n.x, dnm), fmod(n.y, dnm), fmod(n.z, dnm)); }
inline f32x3 fmod(f32x3 n, f32x3 dnm) { return f32x3(fmod(n.x, dnm.x), fmod(n.y, dnm.y), fmod(n.z, dnm.z)); }
inline f32x4 fmod(f32x4 n, f32 dnm) { return f32x4(fmod(n.x, dnm), fmod(n.y, dnm), fmod(n.z, dnm), fmod(n.w, dnm)); }
inline f32x4 fmod(f32x4 n, f32x4 dnm) { return f32x4(fmod(n.x, dnm.x), fmod(n.y, dnm.y), fmod(n.z, dnm.z), fmod(n.w, dnm.w)); }

inline f32 floor(f32 v) { return ::floorf(v); }
inline f32x2 floor(f32x2 v) { return f32x2(floor(v.x), floor(v.y)); }
inline f32x3 floor(f32x3 v) { return f32x3(floor(v.x), floor(v.y), floor(v.z)); }
inline f32x4 floor(f32x4 v) { return f32x4(floor(v.x), floor(v.y), floor(v.z), floor(v.w)); }

inline f32 ceil(f32 v) { return ::ceilf(v); }
inline f32x2 ceil(f32x2 v) { return f32x2(ceil(v.x), ceil(v.y)); }
inline f32x3 ceil(f32x3 v) { return f32x3(ceil(v.x), ceil(v.y), ceil(v.z)); }
inline f32x4 ceil(f32x4 v) { return f32x4(ceil(v.x), ceil(v.y), ceil(v.z), ceil(v.w)); }

inline f32 round(f32 v) { return ::roundf(v); }
inline f32x2 round(f32x2 v) { return f32x2(round(v.x), round(v.y)); }
inline f32x3 round(f32x3 v) { return f32x3(round(v.x), round(v.y), round(v.z)); }
inline f32x4 round(f32x4 v) { return f32x4(round(v.x), round(v.y), round(v.z), round(v.w)); }

} // namespace sfz

using sfz::f32x2;
using sfz::f32x3;
using sfz::f32x4;

using sfz::i32x2;
using sfz::i32x3;
using sfz::i32x4;

using sfz::u8x2;
using sfz::u8x4;

#endif
