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

// Vector math
// ------------------------------------------------------------------------------------------------

inline f32 length(f32x2 v) { return ::sqrtf(dot(v, v)); }
inline f32 length(f32x3 v) { return ::sqrtf(dot(v, v)); }
inline f32 length(f32x4 v) { return ::sqrtf(dot(v, v)); }

inline f32x2 normalize(f32x2 v) { return v * (1.0f / length(v)); }
inline f32x3 normalize(f32x3 v) { return v * (1.0f / length(v)); }
inline f32x4 normalize(f32x4 v) { return v * (1.0f / length(v)); }

inline f32x2 normalizeSafe(f32x2 v) { f32 tmp = length(v); return tmp == 0.0f ? v : v * (1.0f / tmp); }
inline f32x3 normalizeSafe(f32x3 v) { f32 tmp = length(v); return tmp == 0.0f ? v : v * (1.0f / tmp); }
inline f32x4 normalizeSafe(f32x4 v) { f32 tmp = length(v); return tmp == 0.0f ? v : v * (1.0f / tmp); }

constexpr f32 elemSum(f32x2 v) { return v.x + v.y; }
constexpr f32 elemSum(f32x3 v) { return v.x + v.y + v.z; }
constexpr f32 elemSum(f32x4 v) { return v.x + v.y + v.z + v.w; }

constexpr i32 elemSum(i32x2 v) { return v.x + v.y; }
constexpr i32 elemSum(i32x3 v) { return v.x + v.y + v.z; }
constexpr i32 elemSum(i32x4 v) { return v.x + v.y + v.z + v.w; }

constexpr f32 PI = 3.14159265358979323846f;
constexpr f32 DEG_TO_RAD = PI / 180.0f;
constexpr f32 RAD_TO_DEG = 180.0f / PI;

constexpr f32 EQF_EPS = 0.001f;
constexpr bool eqf(f32 l, f32 r, f32 eps = EQF_EPS) { return (l <= (r + eps)) && (l >= (r - eps)); }
constexpr bool eqf(f32x2 l, f32x2 r, f32 eps = EQF_EPS) { return eqf(l.x, r.x, eps) && eqf(l.y, r.y, eps); }
constexpr bool eqf(f32x3 l, f32x3 r, f32 eps = EQF_EPS) { return eqf(l.x, r.x, eps) && eqf(l.y, r.y, eps) && eqf(l.z, r.z, eps); }
constexpr bool eqf(f32x4 l, f32x4 r, f32 eps = EQF_EPS) { return eqf(l.x, r.x, eps) && eqf(l.y, r.y, eps) && eqf(l.z, r.z, eps) && eqf(l.w, r.w, eps); }

constexpr f32 abs(f32 v) { return v >= 0.0f ? v : -v; }
constexpr f32x2 abs(f32x2 v) { return f32x2(sfz::abs(v.x), sfz::abs(v.y)); }
constexpr f32x3 abs(f32x3 v) { return f32x3(sfz::abs(v.x), sfz::abs(v.y), sfz::abs(v.z)); }
constexpr f32x4 abs(f32x4 v) { return f32x4(sfz::abs(v.x), sfz::abs(v.y), sfz::abs(v.z), sfz::abs(v.w)); }

constexpr i32 abs(i32 v) { return v >= 0 ? v : -v; }
constexpr i32x2 abs(i32x2 v) { return i32x2(sfz::abs(v.x), sfz::abs(v.y)); }
constexpr i32x3 abs(i32x3 v) { return i32x3(sfz::abs(v.x), sfz::abs(v.y), sfz::abs(v.z)); }
constexpr i32x4 abs(i32x4 v) { return i32x4(sfz::abs(v.x), sfz::abs(v.y), sfz::abs(v.z), sfz::abs(v.w)); }

constexpr f32 elemMax(f32x2 v) { return sfz::max(v.x, v.y); }
constexpr f32 elemMax(f32x3 v) { return sfz::max(sfz::max(v.x, v.y), v.z); }
constexpr f32 elemMax(f32x4 v) { return sfz::max(sfz::max(sfz::max(v.x, v.y), v.z), v.w); }
constexpr i32 elemMax(i32x2 v) { return sfz::max(v.x, v.y); }
constexpr i32 elemMax(i32x3 v) { return sfz::max(sfz::max(v.x, v.y), v.z); }
constexpr i32 elemMax(i32x4 v) { return sfz::max(sfz::max(sfz::max(v.x, v.y), v.z), v.w); }

constexpr f32 elemMin(f32x2 v) { return sfz::min(v.x, v.y); }
constexpr f32 elemMin(f32x3 v) { return sfz::min(sfz::min(v.x, v.y), v.z); }
constexpr f32 elemMin(f32x4 v) { return sfz::min(sfz::min(sfz::min(v.x, v.y), v.z), v.w); }
constexpr i32 elemMin(i32x2 v) { return sfz::min(v.x, v.y); }
constexpr i32 elemMin(i32x3 v) { return sfz::min(sfz::min(v.x, v.y), v.z); }
constexpr i32 elemMin(i32x4 v) { return sfz::min(sfz::min(sfz::min(v.x, v.y), v.z), v.w); }

constexpr f32 sgn(f32 v) { return v < 0.0f ? -1.0f : 1.0f; }
constexpr i32 sgn(i32 v) { return v < 0 ? -1 : 1; }
constexpr f32x2 sgn(f32x2 v) { return f32x2(sgn(v.x), sgn(v.y)); }
constexpr f32x3 sgn(f32x3 v) { return f32x3(sgn(v.x), sgn(v.y), sgn(v.z)); }
constexpr f32x4 sgn(f32x4 v) { return f32x4(sgn(v.x), sgn(v.y), sgn(v.z), sgn(v.w)); }
constexpr i32x2 sgn(i32x2 v) { return i32x2(sgn(v.x), sgn(v.y)); }
constexpr i32x3 sgn(i32x3 v) { return i32x3(sgn(v.x), sgn(v.y), sgn(v.z)); }
constexpr i32x4 sgn(i32x4 v) { return i32x4(sgn(v.x), sgn(v.y), sgn(v.z), sgn(v.w)); }

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

#endif
