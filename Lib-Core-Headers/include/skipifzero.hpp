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

#include <cassert>
#include <cmath> // std::sqrt
#include <cstdint>
#include <cstdlib> // std::abort()
#include <new> // placement new
#include <utility> // std::move, std::forward, std::swap

namespace sfz {

// Assert macros
// ------------------------------------------------------------------------------------------------

#define sfz_assert(condition) assert(condition)

#define sfz_assert_hard(condition) if (!(condition)) { assert((condition)); std::abort(); }

// Debug information
// ------------------------------------------------------------------------------------------------

// Tiny struct that contains debug information, i.e. file, line number and a message.
// Note that the message MUST be a compile-time constant, it may NOT be dynamically allocated.
struct DbgInfo final {
	const char* staticMsg = ""; // MUST be a compile-time constant, pointer must always be valid.
	const char* file = "";
	uint32_t line = 0;
	DbgInfo() noexcept = default;
	DbgInfo(const char* staticMsg, const char* file, uint32_t line) noexcept :
		staticMsg(staticMsg), file(file), line(line) {}
};

// Tiny macro that creates a DbgInfo struct with current file and line number. Message must be a
// compile time constant, i.e. string must be valid for the remaining duration of the program.
#define sfz_dbg(staticMsg) sfz::DbgInfo(staticMsg, __FILE__, __LINE__)

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
		DbgInfo dbg, uint64_t size, uint64_t alignment = 32) noexcept = 0;

	// Deallocates memory previously allocated with this instance.
	//
	// Deallocating nullptr is required to be a no-op. Deallocating pointers not allocated by this
	// instance is undefined behavior, and may result in catastrophic failure.
	virtual void deallocate(void* pointer) noexcept = 0;

	// Constructs a new object of type T, similar to operator new. Guarantees 32-byte alignment.
	template<typename T, typename... Args>
	T* newObject(DbgInfo dbg, Args&&... args) noexcept
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

// Memory helpers
// ------------------------------------------------------------------------------------------------

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

// Rounds up a given offset so that it is evenly divisible by the given alignment
constexpr uint64_t roundUpAligned(uint64_t offset, uint64_t alignment)
{
	return ((offset + alignment - 1) / alignment) * alignment;
}

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
	constexpr T& operator[] (uint32_t index) { return data()[index]; }
	constexpr T operator[] (uint32_t index) const { return data()[index]; }

	constexpr Vec& operator+= (Vec o) { x += o.x; y += o.y; return *this; }
	constexpr Vec& operator-= (Vec o) { x -= o.x; y -= o.y; return *this; }
	constexpr Vec& operator*= (T s) { x *= s; y *= s; return *this; }
	constexpr Vec& operator*= (Vec o) { x *= o.x; y *= o.y; return *this; }
	constexpr Vec& operator/= (T s) { x /= s; y /= s; return *this; }
	constexpr Vec& operator/= (Vec o) { x /= o.x; y /= o.y; return *this; }

	constexpr Vec operator+ (Vec o) const { return Vec(*this) += o; }
	constexpr Vec operator- (Vec o) const { return Vec(*this) -= o; }
	constexpr Vec operator- () const { return Vec(-x, -y); }
	constexpr Vec operator* (Vec o) const { return Vec(*this) *= o; }
	constexpr Vec operator* (T s) const { return Vec(*this) *= s; }
	constexpr Vec operator/ (Vec o) const { return Vec(*this) /= o; }
	constexpr Vec operator/ (T s) const { return Vec(*this) /= s; }

	constexpr bool operator== (Vec o) const { return x == o.x && y == o.y; }
	constexpr bool operator!= (Vec o) const { return !(*this == o); }
};

using vec2 = Vec<float, 2>;         static_assert(sizeof(vec2) == sizeof(float) * 2);
using vec2_i32 = Vec<int32_t, 2>;   static_assert(sizeof(vec2_i32) == sizeof(int32_t) * 2);
using vec2_u32 = Vec<uint32_t, 2>;  static_assert(sizeof(vec2_u32) == sizeof(uint32_t) * 2);
using vec2_u8 = Vec<uint8_t, 2>;    static_assert(sizeof(vec2_u8) == sizeof(uint8_t) * 2);

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
	constexpr T& operator[] (uint32_t index) { return data()[index]; }
	constexpr T operator[] (uint32_t index) const { return data()[index]; }

	constexpr Vec& operator+= (Vec o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr Vec& operator-= (Vec o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr Vec& operator*= (T s) { x *= s; y *= s; z *= s; return *this; }
	constexpr Vec& operator*= (Vec o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr Vec& operator/= (T s) { x /= s; y /= s; z /= s; return *this; }
	constexpr Vec& operator/= (Vec o) { x /= o.x; y /= o.y; z /= o.z; return *this; }

	constexpr Vec operator+ (Vec o) const { return Vec(*this) += o; }
	constexpr Vec operator- (Vec o) const { return Vec(*this) -= o; }
	constexpr Vec operator- () const { return Vec(-x, -y, -z); }
	constexpr Vec operator* (Vec o) const { return Vec(*this) *= o; }
	constexpr Vec operator* (T s) const { return Vec(*this) *= s; }
	constexpr Vec operator/ (Vec o) const { return Vec(*this) /= o; }
	constexpr Vec operator/ (T s) const { return Vec(*this) /= s; }

	constexpr bool operator== (Vec o) const { return x == o.x && y == o.y && z == o.z; }
	constexpr bool operator!= (Vec o) const { return !(*this == o); }
};

using vec3 = Vec<float, 3>;         static_assert(sizeof(vec3) == sizeof(float) * 3);
using vec3_i32 = Vec<int32_t, 3>;   static_assert(sizeof(vec3_i32) == sizeof(int32_t) * 3);
using vec3_u32 = Vec<uint32_t, 3>;  static_assert(sizeof(vec3_u32) == sizeof(uint32_t) * 3);
using vec3_u8 = Vec<uint8_t, 3>;    static_assert(sizeof(vec3_u8) == sizeof(uint8_t) * 3);

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
	constexpr T& operator[] (uint32_t index) { return data()[index]; }
	constexpr T operator[] (uint32_t index) const { return data()[index]; }

	constexpr Vec& operator+= (Vec o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr Vec& operator-= (Vec o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr Vec& operator*= (T s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr Vec& operator*= (Vec o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr Vec& operator/= (T s) { x /= s; y /= s; z /= s; w /= s; return *this; }
	constexpr Vec& operator/= (Vec o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }

	constexpr Vec operator+ (Vec o) const { return Vec(*this) += o; }
	constexpr Vec operator- (Vec o) const { return Vec(*this) -= o; }
	constexpr Vec operator- () const { return Vec(-x, -y, -z, -w); }
	constexpr Vec operator* (Vec o) const { return Vec(*this) *= o; }
	constexpr Vec operator* (T s) const { return Vec(*this) *= s; }
	constexpr Vec operator/ (Vec o) const { return Vec(*this) /= o; }
	constexpr Vec operator/ (T s) const { return Vec(*this) /= s; }

	constexpr bool operator== (Vec o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (Vec o) const { return !(*this == o); }
};

using vec4 = Vec<float, 4>;         static_assert(sizeof(vec4) == sizeof(float) * 4);
using vec4_i32 = Vec<int32_t, 4>;   static_assert(sizeof(vec4_i32) == sizeof(int32_t) * 4);
using vec4_u32 = Vec<uint32_t, 4>;  static_assert(sizeof(vec4_u32) == sizeof(uint32_t) * 4);
using vec4_u8 = Vec<uint8_t, 4>;    static_assert(sizeof(vec4_u8) == sizeof(uint8_t) * 4);

static_assert(alignof(vec4) == 16);
static_assert(alignof(vec4_i32) == 16);
static_assert(alignof(vec4_u32) == 16);
static_assert(alignof(vec4_u8) == 4);

template<typename T, uint32_t N>
constexpr Vec<T,N> operator* (T s, Vec<T,N> v) { return v * s; }

template<typename T, uint32_t N>
constexpr Vec<T,N> operator/ (T s, Vec<T,N> v) { return v / s; }

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

} // namespace sfz

// sfzMin() & sfzMax()
// ------------------------------------------------------------------------------------------------

// Implementations of min() and max() mathematical operations, similar to std::min() and std::max().
//
// The reason this is necessary is because std::min()/std::max() generate really poor code on
// Visual Studio. The below implementation seem to generate exactly the assembly we want (i.e.
// the minss/maxss x86 instructions for T=float) on Visual Studio 2019. They also seem to generate
// equally good code to std::min()/std::max() on Clang.
//
// See my tweet about it for experiments: https://twitter.com/PetorSFZ/status/1165330439269167104

constexpr float sfzMin(float lhs, float rhs) noexcept { return (lhs < rhs) ? lhs : rhs; }
constexpr int32_t sfzMin(int32_t lhs, int32_t rhs) noexcept { return (lhs < rhs) ? lhs : rhs; }
constexpr uint32_t sfzMin(uint32_t lhs, uint32_t rhs) noexcept { return (lhs < rhs) ? lhs : rhs; }
constexpr int64_t sfzMin(int64_t lhs, int64_t rhs) noexcept { return (lhs < rhs) ? lhs : rhs; }
constexpr uint64_t sfzMin(uint64_t lhs, uint64_t rhs) noexcept { return (lhs < rhs) ? lhs : rhs; }

template<typename T, uint32_t N>
constexpr sfz::Vec<T,N> sfzMin(sfz::Vec<T,N> l, sfz::Vec<T,N> r)
{
	sfz::Vec<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) tmp[i] = (l[i] < r[i]) ? l[i] : r[i];
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vec<T,N> sfzMin(sfz::Vec<T,N> l, T r) { return sfzMin(l, sfz::Vec<T,N>(r)); }

template<typename T, uint32_t N>
constexpr sfz::Vec<T,N> sfzMin(T l, sfz::Vec<T,N> r) { return sfzMin(r, l); }

constexpr float sfzMax(float lhs, float rhs) noexcept { return (lhs < rhs) ? rhs : lhs; }
constexpr int32_t sfzMax(int32_t lhs, int32_t rhs) noexcept { return (lhs < rhs) ? rhs : lhs; }
constexpr uint32_t sfzMax(uint32_t lhs, uint32_t rhs) noexcept { return (lhs < rhs) ? rhs : lhs; }
constexpr int64_t sfzMax(int64_t lhs, int64_t rhs) noexcept { return (lhs < rhs) ? rhs : lhs; }
constexpr uint64_t sfzMax(uint64_t lhs, uint64_t rhs) noexcept { return (lhs < rhs) ? rhs : lhs; }

template<typename T, uint32_t N>
constexpr sfz::Vec<T,N> sfzMax(sfz::Vec<T,N> l, sfz::Vec<T,N> r)
{
	sfz::Vec<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) { tmp[i] = (l[i] < r[i]) ? r[i] : l[i]; }
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vec<T,N> sfzMax(sfz::Vec<T,N> l, T r) { return sfzMax(l, sfz::Vec<T,N>(r)); }

template<typename T, uint32_t N>
constexpr sfz::Vec<T,N> sfzMax(T l, sfz::Vec<T,N> r) { return sfzMax(r, l); }
