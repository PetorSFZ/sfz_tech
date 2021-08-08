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

#include <cassert>
#include <cmath> // std::sqrt, std::fmodf
#include <cstdint>
#include <cstdlib> // std::abort()
#include <cstring> // memcpy()
#include <new> // placement new
#include <type_traits>
#include <utility> // std::move, std::forward, std::swap

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

// Assert macros
// ------------------------------------------------------------------------------------------------

#ifndef NDEBUG
#define sfz_assert(cond) do { if (!(cond)) { assert(cond); } } while(0)
#else
#define sfz_assert(cond) do { (void)sizeof(cond); } while(0)
#endif

#define sfz_assert_hard(cond) do { if (!(cond)) { assert(cond); abort(); } } while(0)

// Allocator Interface
// ------------------------------------------------------------------------------------------------

// The Allocator interface.
//
// * Allocators are instance based and can therefore be decided at runtime.
// * Typically classes should not own or create allocators, only keep simple pointers (Allocator*).
// * Typically allocator pointers should be moved/copied when a class is moved/copied.
// * Typically equality operators (==, !=) should ignore allocator pointers.
// * It is the responsibility of the creator of the allocator instance to ensure that all users
//   that have been provided a pointer have freed all their memory and are done using the allocator
//   before the allocator itself is removed. Often this means that an allocator need to be kept
//   alive for the remaining lifetime of the program.
// * All virtual methods are marked noexcept, meaning an allocator may never throw exceptions.
class Allocator {
public:
	virtual ~Allocator() noexcept {}

	// Allocates memory with the specified byte alignment, returns nullptr on failure.
	virtual void* allocate(
		SfzDbgInfo dbg, uint64_t size, uint64_t alignment = 32) noexcept = 0;

	// Deallocates memory previously allocated with this instance.
	//
	// Deallocating nullptr is required to be a no-op. Deallocating pointers not allocated by this
	// instance is undefined behavior, and may result in catastrophic failure.
	virtual void deallocate(void* pointer) noexcept = 0;

	// Constructs a new object of type T, similar to operator new. Guarantees 32-byte alignment.
	template<typename T, typename... Args>
	T* newObject(SfzDbgInfo dbg, Args&&... args) noexcept
	{
		// Allocate memory (minimum 32-byte alignment), return nullptr on failure
		void* memPtr = this->allocate(dbg, sizeof(T), alignof(T) < 32 ? 32 : alignof(T));
		if (memPtr == nullptr) return nullptr;

		// Creates object (placement new), terminates program if constructor throws exception.
		return new(memPtr) T(std::forward<Args>(args)...);
	}

	// Deletes an object created with this allocator, similar to operator delete.
	template<typename T>
	void deleteObject(T*& pointer) noexcept
	{
		if (pointer == nullptr) return;
		pointer->~T(); // Call destructor, will terminate program if it throws exception.
		this->deallocate(pointer);
		pointer = nullptr; // Set callers pointer to nullptr, an attempt to avoid dangling pointers.
	}
};

// Memory functions
// ------------------------------------------------------------------------------------------------

// Swaps size bytes of memory between two buffers. Undefined behaviour if the buffers overlap, with
// the exception that its safe to call if both pointers are the same (i.e. point to the same buffer).
inline void memswp(void* __restrict a, void* __restrict b, uint64_t size)
{
	const uint64_t MEMSWP_TMP_BUFFER_SIZE = 64;
	uint8_t tmpBuffer[MEMSWP_TMP_BUFFER_SIZE];

	// Swap buffers in temp buffer sized chunks
	uint64_t bytesLeft = size;
	while (bytesLeft >= MEMSWP_TMP_BUFFER_SIZE) {
		memcpy(tmpBuffer, a, MEMSWP_TMP_BUFFER_SIZE);
		memcpy(a, b, MEMSWP_TMP_BUFFER_SIZE);
		memcpy(b, tmpBuffer, MEMSWP_TMP_BUFFER_SIZE);
		a = reinterpret_cast<uint8_t*>(a) + MEMSWP_TMP_BUFFER_SIZE;
		b = reinterpret_cast<uint8_t*>(b) + MEMSWP_TMP_BUFFER_SIZE;
		bytesLeft -= MEMSWP_TMP_BUFFER_SIZE;
	}

	// Swap remaining bytes
	if (bytesLeft > 0) {
		memcpy(tmpBuffer, a, bytesLeft);
		memcpy(a, b, bytesLeft);
		memcpy(b, tmpBuffer, bytesLeft);
	}
}

// Checks whether an uint64_t is a power of two or not
constexpr bool isPowerOfTwo(uint64_t value)
{
	// See https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
	return value != 0 && (value & (value - 1)) == 0;
}

// Checks whether a pointer is aligned to a given byte aligment
inline bool isAligned(const void* pointer, uint64_t alignment) noexcept
{
	sfz_assert(isPowerOfTwo(alignment));
	return ((uintptr_t)pointer & (alignment - 1)) == 0;
}

// Rounds up a given value so that it is evenly divisible by the given alignment
constexpr uint64_t roundUpAligned(uint64_t value, uint64_t alignment)
{
	return ((value + alignment - 1) / alignment) * alignment;
}

// Gives the offset needed to make the given value evenly divisible by the given alignment
constexpr uint64_t alignedDiff(uint64_t value, uint64_t alignment)
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
	~T() noexcept \
	{ \
		static_assert(std::is_final_v<T>, "DropType's must be marked final"); \
		static_assert(!std::is_polymorphic_v<T>, "DropType's may not have any virtual methods"); \
		static_assert(std::is_default_constructible_v<T>, "DropType's MUST be default constructible"); \
		static_assert(!std::is_copy_constructible_v<T>, "DropType's MUST NOT be copy constructible"); \
		static_assert(!std::is_copy_assignable_v<T>, "DropType's MUST NOT be copy assignable"); \
		static_assert(std::is_move_constructible_v<T>, "DropType's MUST be move constructible"); \
		static_assert(std::is_move_assignable_v<T>, "DropType's MUST be move assignable"); \
		this->destroy(); \
	}

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
// access the last two elements of a vec3 with v.yz.
//
// There are typedefs available for the primary primitives meant to be used. You should normally
// only use these typedefs and not the template explicitly, unless you have a very specific use-case
// which requires it.

template<typename T, uint32_t N>
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
	constexpr T& operator[] (uint32_t idx) { sfz_assert(idx < 2); return data()[idx]; }
	constexpr T operator[] (uint32_t idx) const { sfz_assert(idx < 2); return data()[idx]; }

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

using vec2 = Vec<float, 2>;         static_assert(sizeof(vec2) == sizeof(float) * 2, "");
using vec2_i32 = Vec<int32_t, 2>;   static_assert(sizeof(vec2_i32) == sizeof(int32_t) * 2, "");
using vec2_u32 = Vec<uint32_t, 2>;  static_assert(sizeof(vec2_u32) == sizeof(uint32_t) * 2, "");
using vec2_u8 = Vec<uint8_t, 2>;    static_assert(sizeof(vec2_u8) == sizeof(uint8_t) * 2, "");

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
	constexpr T& operator[] (uint32_t idx) { sfz_assert(idx < 3); return data()[idx]; }
	constexpr T operator[] (uint32_t idx) const { sfz_assert(idx < 3); return data()[idx]; }

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

using vec3 = Vec<float, 3>;         static_assert(sizeof(vec3) == sizeof(float) * 3, "");
using vec3_i32 = Vec<int32_t, 3>;   static_assert(sizeof(vec3_i32) == sizeof(int32_t) * 3, "");
using vec3_u32 = Vec<uint32_t, 3>;  static_assert(sizeof(vec3_u32) == sizeof(uint32_t) * 3, "");
using vec3_u8 = Vec<uint8_t, 3>;    static_assert(sizeof(vec3_u8) == sizeof(uint8_t) * 3, "");

template<typename T>
struct alignas(sizeof(T) * 4) Vec<T,4> final {

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
	constexpr T& operator[] (uint32_t idx) { sfz_assert(idx < 4); return data()[idx]; }
	constexpr T operator[] (uint32_t idx) const { sfz_assert(idx < 4); return data()[idx]; }

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

using vec4 = Vec<float, 4>;         static_assert(sizeof(vec4) == sizeof(float) * 4, "");
using vec4_i32 = Vec<int32_t, 4>;   static_assert(sizeof(vec4_i32) == sizeof(int32_t) * 4, "");
using vec4_u32 = Vec<uint32_t, 4>;  static_assert(sizeof(vec4_u32) == sizeof(uint32_t) * 4, "");
using vec4_u8 = Vec<uint8_t, 4>;    static_assert(sizeof(vec4_u8) == sizeof(uint8_t) * 4, "");

static_assert(alignof(vec4) == 16, "");
static_assert(alignof(vec4_i32) == 16, "");
static_assert(alignof(vec4_u32) == 16, "");
static_assert(alignof(vec4_u8) == 4, "");

template<typename T, uint32_t N>
constexpr Vec<T,N> operator* (T s, Vec<T,N> v) { return v * s; }

template<typename T, uint32_t N>
constexpr Vec<T,N> operator/ (T s, Vec<T,N> v) { return Vec<T,N>(s) / v; }

template<typename T, uint32_t N>
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

inline float length(vec2 v) { return std::sqrt(dot(v, v)); }
inline float length(vec3 v) { return std::sqrt(dot(v, v)); }
inline float length(vec4 v) { return std::sqrt(dot(v, v)); }

inline vec2 normalize(vec2 v) { return v * (1.0f / length(v)); }
inline vec3 normalize(vec3 v) { return v * (1.0f / length(v)); }
inline vec4 normalize(vec4 v) { return v * (1.0f / length(v)); }

inline vec2 normalizeSafe(vec2 v) { float tmp = length(v); return tmp == 0.0f ? v : v * (1.0f / tmp); }
inline vec3 normalizeSafe(vec3 v) { float tmp = length(v); return tmp == 0.0f ? v : v * (1.0f / tmp); }
inline vec4 normalizeSafe(vec4 v) { float tmp = length(v); return tmp == 0.0f ? v : v * (1.0f / tmp); }

template<typename T> constexpr T elemSum(Vec<T,2> v) { return v.x + v.y; }
template<typename T> constexpr T elemSum(Vec<T,3> v) { return v.x + v.y + v.z; }
template<typename T> constexpr T elemSum(Vec<T,4> v) { return v.x + v.y + v.z + v.w; }

constexpr float divSafe(float n, float d, float eps = 0.000'000'1f) { return d == 0.0f ? n / eps : n / d; }
constexpr vec2 divSafe(vec2 n, vec2 d, float eps = 0.000'000'1f) { return vec2(divSafe(n.x, d.x, eps), divSafe(n.y, d.y, eps)); }
constexpr vec3 divSafe(vec3 n, vec3 d, float eps = 0.000'000'1f) { return vec3(divSafe(n.x, d.x, eps), divSafe(n.y, d.y, eps), divSafe(n.z, d.z, eps)); }
constexpr vec4 divSafe(vec4 n, vec4 d, float eps = 0.000'000'1f) { return vec4(divSafe(n.x, d.x, eps), divSafe(n.y, d.y, eps), divSafe(n.z, d.z, eps), divSafe(n.w, d.w, eps)); }

// Math functions
// ------------------------------------------------------------------------------------------------

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

constexpr float EQF_EPS = 0.001f;
constexpr bool eqf(float l, float r, float eps = EQF_EPS) { return (l <= (r + eps)) && (l >= (r - eps)); }
constexpr bool eqf(vec2 l, vec2 r, float eps = EQF_EPS) { return eqf(l.x, r.x, eps) && eqf(l.y, r.y, eps); }
constexpr bool eqf(vec3 l, vec3 r, float eps = EQF_EPS) { return eqf(l.xy, r.xy, eps) && eqf(l.z, r.z, eps); }
constexpr bool eqf(vec4 l, vec4 r, float eps = EQF_EPS) { return eqf(l.xyz, r.xyz, eps) && eqf(l.w, r.w, eps); }

constexpr float abs(float v) { return v >= 0.0f ? v : -v; }
constexpr vec2 abs(vec2 v) { return vec2(sfz::abs(v.x), sfz::abs(v.y)); }
constexpr vec3 abs(vec3 v) { return vec3(sfz::abs(v.xy), sfz::abs(v.z)); }
constexpr vec4 abs(vec4 v) { return vec4(sfz::abs(v.xyz), sfz::abs(v.w)); }

constexpr int32_t abs(int32_t v) { return v >= 0 ? v : -v; }
constexpr vec2_i32 abs(vec2_i32 v) { return vec2_i32(sfz::abs(v.x), sfz::abs(v.y)); }
constexpr vec3_i32 abs(vec3_i32 v) { return vec3_i32(sfz::abs(v.xy),sfz::abs(v.z)); }
constexpr vec4_i32 abs(vec4_i32 v) { return vec4_i32(sfz::abs(v.xyz), sfz::abs(v.w)); }

constexpr float min(float l, float r) { return (l < r) ? l : r; }
constexpr int32_t min(int32_t l, int32_t r) { return (l < r) ? l : r; }
constexpr uint32_t min(uint32_t l, uint32_t r) { return (l < r) ? l : r; }
constexpr uint8_t min(uint8_t l, uint8_t r) { return (l < r) ? l : r; }

template<typename T, uint32_t N>
constexpr Vec<T,N> min(Vec<T,N> l, Vec<T,N> r)
{
	Vec<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) tmp[i] = sfz::min(l[i], r[i]);
	return tmp;
}

template<typename T, uint32_t N> constexpr Vec<T,N> min(Vec<T,N> l, T r) { return sfz::min(l, Vec<T,N>(r)); }
template<typename T, uint32_t N> constexpr Vec<T,N> min(T l, Vec<T,N> r) { return sfz::min(r, l); }

constexpr float max(float l, float r) { return (l < r) ? r : l; }
constexpr int32_t max(int32_t l, int32_t r) { return (l < r) ? r : l; }
constexpr uint32_t max(uint32_t l, uint32_t r) { return (l < r) ? r : l; }
constexpr uint8_t max(uint8_t l, uint8_t r) { return (l < r) ? r : l; }

template<typename T, uint32_t N>
constexpr Vec<T,N> max(Vec<T,N> l, Vec<T,N> r)
{
	Vec<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) tmp[i] = sfz::max(l[i], r[i]);
	return tmp;
}

template<typename T, uint32_t N> constexpr Vec<T,N> max(Vec<T,N> l, T r) { return sfz::max(l, Vec<T,N>(r)); }
template<typename T, uint32_t N> constexpr Vec<T,N> max(T l, Vec<T,N> r) { return sfz::max(r, l); }

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

constexpr float saturate(float v) { return sfz::clamp(v, 0.0f, 1.0f); }
constexpr int32_t saturate(int32_t v) { return sfz::clamp(v, 0, 255); }
constexpr uint32_t saturate(uint32_t v) { return sfz::clamp(v, 0u, 255u); }

template<typename T> constexpr Vec<T,2> saturate(Vec<T,2> v) { return Vec<T,2>(sfz::saturate(v.x), sfz::saturate(v.y)); }
template<typename T> constexpr Vec<T,3> saturate(Vec<T,3> v) { return Vec<T,3>(sfz::saturate(v.xy), sfz::saturate(v.z)); }
template<typename T> constexpr Vec<T,4> saturate(Vec<T,4> v) { return Vec<T,4>(sfz::saturate(v.xyz), sfz::saturate(v.w)); }

constexpr float lerp(float v0, float v1, float t) { return (1.0f - t) * v0 + t * v1; }
constexpr vec2 lerp(vec2 v0, vec2 v1, float t) { return (1.0f - t) * v0 + t * v1; }
constexpr vec3 lerp(vec3 v0, vec3 v1, float t) { return (1.0f - t) * v0 + t * v1; }
constexpr vec4 lerp(vec4 v0, vec4 v1, float t) { return (1.0f - t) * v0 + t * v1; }

inline float fmod(float n, float dnm) { return std::fmodf(n, dnm); }
inline vec2 fmod(vec2 n, float dnm) { return vec2(fmod(n.x, dnm), fmod(n.y, dnm)); }
inline vec2 fmod(vec2 n, vec2 dnm) { return vec2(fmod(n.x, dnm.x), fmod(n.y, dnm.y)); }
inline vec3 fmod(vec3 n, float dnm) { return vec3(fmod(n.x, dnm), fmod(n.y, dnm), fmod(n.z, dnm)); }
inline vec3 fmod(vec3 n, vec3 dnm) { return vec3(fmod(n.x, dnm.x), fmod(n.y, dnm.y), fmod(n.z, dnm.z)); }
inline vec4 fmod(vec4 n, float dnm) { return vec4(fmod(n.x, dnm), fmod(n.y, dnm), fmod(n.z, dnm), fmod(n.w, dnm)); }
inline vec4 fmod(vec4 n, vec4 dnm) { return vec4(fmod(n.x, dnm.x), fmod(n.y, dnm.y), fmod(n.z, dnm.z), fmod(n.w, dnm.w)); }

inline float floor(float v) { return std::floorf(v); }
inline vec2 floor(vec2 v) { return vec2(floor(v.x), floor(v.y)); }
inline vec3 floor(vec3 v) { return vec3(floor(v.x), floor(v.y), floor(v.z)); }
inline vec4 floor(vec4 v) { return vec4(floor(v.x), floor(v.y), floor(v.z), floor(v.w)); }

inline float ceil(float v) { return std::ceilf(v); }
inline vec2 ceil(vec2 v) { return vec2(ceil(v.x), ceil(v.y)); }
inline vec3 ceil(vec3 v) { return vec3(ceil(v.x), ceil(v.y), ceil(v.z)); }
inline vec4 ceil(vec4 v) { return vec4(ceil(v.x), ceil(v.y), ceil(v.z), ceil(v.w)); }

inline float round(float v) { return std::roundf(v); }
inline vec2 round(vec2 v) { return vec2(round(v.x), round(v.y)); }
inline vec3 round(vec3 v) { return vec3(round(v.x), round(v.y), round(v.z)); }
inline vec4 round(vec4 v) { return vec4(round(v.x), round(v.y), round(v.z), round(v.w)); }

} // namespace sfz

#endif
