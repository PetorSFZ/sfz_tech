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

#ifndef SFZ_H
#define SFZ_H
#pragma once

#if defined(min) || defined(max)
#undef min
#undef max
#endif


// C/C++ compatiblity and compiler specific macros
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus
#define sfz_extern_c extern "C"
#define sfz_nodiscard [[nodiscard]]
#define sfz_struct(name) struct name final
#define sfz_constant constexpr
#define sfz_constexpr_func constexpr
#define sfz_static_assert(cond) static_assert(cond, #cond)
#else
#define sfz_extern_c
#define sfz_nodiscard
#define sfz_struct(name) \
	struct name; \
	typedef struct name name; \
	struct name
#define sfz_constant static const
#define sfz_constexpr_func inline
#define sfz_static_assert(cond)
#endif

#define sfz_forceinline __forceinline

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif


// Scalar primitives
// ------------------------------------------------------------------------------------------------

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

sfz_constant i8 I8_MIN = -128;
sfz_constant i8 I8_MAX = 127;
sfz_constant i16 I16_MIN = -32768;
sfz_constant i16 I16_MAX = 32767;
sfz_constant i32 I32_MIN = -2147483647 - 1;
sfz_constant i32 I32_MAX = 2147483647;
sfz_constant i64 I64_MIN = -9223372036854775807 - 1;
sfz_constant i64 I64_MAX = 9223372036854775807;

sfz_constant u8 U8_MAX = 0xFF;
sfz_constant u16 U16_MAX = 0xFFFF;
sfz_constant u32 U32_MAX = 0xFFFFFFFF;
sfz_constant u64 U64_MAX = 0xFFFFFFFFFFFFFFFF;

sfz_constant f32 F32_MAX = 3.402823466e+38f;
sfz_constant f64 F64_MAX = 1.7976931348623158e+308;

sfz_constant f32 F32_EPS = 1.192092896e-7f; // Smallest val where (1.0f + F32_EPS != 1.0f)
sfz_constant f64 F64_EPS = 2.2204460492503131e-16; // Smallest val where (1.0 + F64_EPS != 1.0)

sfz_constant f32 SFZ_PI = 3.14159265358979323846f;
sfz_constant f32 SFZ_DEG_TO_RAD = SFZ_PI / 180.0f;
sfz_constant f32 SFZ_RAD_TO_DEG = 180.0f / SFZ_PI;


// Vector primitives
// ------------------------------------------------------------------------------------------------

sfz_struct(f32x2) {
	f32 x, y;

#ifdef __cplusplus
	constexpr f32* data() { return &x; }
	constexpr const f32* data() const { return &x; }
	constexpr f32& operator[] (u32 idx) { return data()[idx]; }
	constexpr f32 operator[] (u32 idx) const { return data()[idx]; }
#endif
};

sfz_constexpr_func f32x2 f32x2_init(f32 x, f32 y) { return f32x2{x, y}; }
sfz_constexpr_func f32x2 f32x2_splat(f32 val) { return f32x2_init(val, val); }

sfz_struct(f32x3) {
	f32 x, y, z;

#ifdef __cplusplus
	f32x2& xy() { return *reinterpret_cast<f32x2*>(&x); }
	constexpr f32x2 xy() const { return f32x2_init(x, y); }
	constexpr f32* data() { return &x; }
	constexpr const f32* data() const { return &x; }
	constexpr f32& operator[] (u32 idx) { return data()[idx]; }
	constexpr f32 operator[] (u32 idx) const { return data()[idx]; }
#endif
};

sfz_constexpr_func f32x3 f32x3_init(f32 x, f32 y, f32 z) { return f32x3{ x, y, z }; }
sfz_constexpr_func f32x3 f32x3_init2(f32x2 xy, f32 z) { return f32x3_init(xy.x, xy.y, z); }
sfz_constexpr_func f32x3 f32x3_splat(f32 val) { return f32x3_init(val, val, val); }

sfz_struct(f32x4) {
	f32 x, y, z, w;

#ifdef __cplusplus
	f32x2& xy() { return *reinterpret_cast<f32x2*>(&x); }
	f32x3& xyz() { return *reinterpret_cast<f32x3*>(&x); }
	constexpr f32x2 xy() const { return f32x2_init(x, y); }
	constexpr f32x3 xyz() const { return f32x3_init(x, y, z); }
	constexpr f32* data() { return &x; }
	constexpr const f32* data() const { return &x; }
	constexpr f32& operator[] (u32 idx) { return data()[idx]; }
	constexpr f32 operator[] (u32 idx) const { return data()[idx]; }
#endif
};

sfz_constexpr_func f32x4 f32x4_init(f32 x, f32 y, f32 z, f32 w) { return f32x4{ x, y, z, w }; }
sfz_constexpr_func f32x4 f32x4_init2(f32x2 xy, f32 z, f32 w) { return f32x4_init(xy.x, xy.y, z, w); }
sfz_constexpr_func f32x4 f32x4_init3(f32x3 xyz, f32 w) { return f32x4_init(xyz.x, xyz.y, xyz.z, w); }
sfz_constexpr_func f32x4 f32x4_splat(f32 val) { return f32x4_init(val, val, val, val); }


sfz_struct(i32x2) {
	i32 x, y;

#ifdef __cplusplus
	constexpr i32* data() { return &x; }
	constexpr const i32* data() const { return &x; }
	constexpr i32& operator[] (u32 idx) { return data()[idx]; }
	constexpr i32 operator[] (u32 idx) const { return data()[idx]; }
#endif
};

sfz_constexpr_func i32x2 i32x2_init(i32 x, i32 y) { return i32x2{ x, y }; }
sfz_constexpr_func i32x2 i32x2_splat(i32 val) { return i32x2_init(val, val); }

sfz_struct(i32x3) {
	i32 x, y, z;

#ifdef __cplusplus
	i32x2& xy() { return *reinterpret_cast<i32x2*>(&x); }
	constexpr i32x2 xy() const { return i32x2_init(x, y); }
	constexpr i32* data() { return &x; }
	constexpr const i32* data() const { return &x; }
	constexpr i32& operator[] (u32 idx) { return data()[idx]; }
	constexpr i32 operator[] (u32 idx) const { return data()[idx]; }
#endif
};

sfz_constexpr_func i32x3 i32x3_init(i32 x, i32 y, i32 z) { return i32x3{ x, y, z }; }
sfz_constexpr_func i32x3 i32x3_init2(i32x2 xy, i32 z) { return i32x3_init(xy.x, xy.y, z); }
sfz_constexpr_func i32x3 i32x3_splat(i32 val) { return i32x3_init(val, val, val); }

sfz_struct(i32x4) {
	i32 x, y, z, w;

#ifdef __cplusplus
	i32x2& xy() { return *reinterpret_cast<i32x2*>(&x); }
	i32x3& xyz() { return *reinterpret_cast<i32x3*>(&x); }
	constexpr i32x2 xy() const { return i32x2_init(x, y); }
	constexpr i32x3 xyz() const { return i32x3_init(x, y, z); }
	constexpr i32* data() { return &x; }
	constexpr const i32* data() const { return &x; }
	constexpr i32& operator[] (u32 idx) { return data()[idx]; }
	constexpr i32 operator[] (u32 idx) const { return data()[idx]; }
#endif
};

sfz_constexpr_func i32x4 i32x4_init(i32 x, i32 y, i32 z, i32 w) { return i32x4{ x, y, z, w }; }
sfz_constexpr_func i32x4 i32x4_init2(i32x2 xy, i32 z, i32 w) { return i32x4_init(xy.x, xy.y, z, w); }
sfz_constexpr_func i32x4 i32x4_init3(i32x3 xyz, i32 w) { return i32x4_init(xyz.x, xyz.y, xyz.z, w); }
sfz_constexpr_func i32x4 i32x4_splat(i32 val) { return i32x4_init(val, val, val, val); }


sfz_struct(u8x2) {
	u8 x, y;
};

sfz_constexpr_func u8x2 u8x2_init(u8 x, u8 y) { return u8x2{ x, y }; }
sfz_constexpr_func u8x2 u8x2_splat(u8 val) { return u8x2_init(val, val); }

sfz_struct(u8x4) {
	u8 x, y, z, w;

#ifdef __cplusplus
	u8x2& xy() { return *reinterpret_cast<u8x2*>(&x); }
	u8x2& zw() { return *reinterpret_cast<u8x2*>(&z); }
	constexpr u8x2 xy() const { return u8x2_init(x, y); }
	constexpr u8x2 zw() const { return u8x2_init(z, w); }
#endif
};

sfz_constexpr_func u8x4 u8x4_init(u8 x, u8 y, u8 z, u8 w) { return u8x4{ x, y, z, w }; }
sfz_constexpr_func u8x4 u8x4_init2(u8x2 xy, u8x2 zw) { return u8x4_init(xy.x, xy.y, zw.x, zw.y); }
sfz_constexpr_func u8x4 u8x4_splat(u8 val) { return u8x4_init(val, val, val, val); }


// Common math functions (excerpt from math.h)
// ------------------------------------------------------------------------------------------------

// math.h is unfortunately quite bloated, we only need very few things from it. Here we forward
// declare the minimal amount of stuff we need for each compiler/platform we support. You should
// prefer to use the "sfz_X()" variants, because we might potentially implement these using
// compiler specific intrinsics on some platforms.

#if defined(_MSC_VER)

#ifdef _DLL
#define SFZ_MATH_H_API __declspec(dllimport)
#else
#define SFZ_MATH_H_API
#endif

sfz_extern_c SFZ_MATH_H_API float __cdecl sqrtf(float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl cosf(float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl sinf(float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl tanf(float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl acosf(float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl asinf(float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl atan2f(float _Y, float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl roundf(float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl floorf(float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl ceilf(float _X);
sfz_extern_c SFZ_MATH_H_API float __cdecl fmodf(float _X, float _Y);

sfz_forceinline f32 sfz_sqrt(f32 x) { return sqrtf(x); }
sfz_forceinline f32 sfz_cos(f32 x) { return cosf(x); }
sfz_forceinline f32 sfz_sin(f32 x) { return sinf(x); }
sfz_forceinline f32 sfz_tan(f32 x) { return tanf(x); }
sfz_forceinline f32 sfz_acos(f32 x) { return acosf(x); }
sfz_forceinline f32 sfz_asin(f32 x) { return asinf(x); }
sfz_forceinline f32 sfz_atan2(f32 y, f32 x) { return atan2f(y, x); }

#else
#error "Not implemented for this compiler
#endif


// Math functions
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func f32 f32x2_dot(f32x2 l, f32x2 r) { return l.x * r.x + l.y * r.y; }
sfz_constexpr_func f32 f32x3_dot(f32x3 l, f32x3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
sfz_constexpr_func f32 f32x4_dot(f32x4 l, f32x4 r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }
sfz_constexpr_func i32 i32x2_dot(i32x2 l, i32x2 r) { return l.x * r.x + l.y * r.y; }
sfz_constexpr_func i32 i32x3_dot(i32x3 l, i32x3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
sfz_constexpr_func i32 i32x4_dot(i32x4 l, i32x4 r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }

sfz_constexpr_func f32x3 f32x3_cross(f32x3 l, f32x3 r) { return f32x3_init(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x); }
sfz_constexpr_func i32x3 i32x3_cross(i32x3 l, i32x3 r) { return i32x3_init(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x); }

inline f32 f32x2_length(f32x2 v) { return sfz_sqrt(f32x2_dot(v, v)); }
inline f32 f32x3_length(f32x3 v) { return sfz_sqrt(f32x3_dot(v, v)); }
inline f32 f32x4_length(f32x4 v) { return sfz_sqrt(f32x4_dot(v, v)); }

inline f32x2 f32x2_normalize(f32x2 v) { const f32 f = 1.0f / f32x2_length(v); return f32x2_init(v.x * f, v.y * f); }
inline f32x3 f32x3_normalize(f32x3 v) { const f32 f = 1.0f / f32x3_length(v); return f32x3_init(v.x * f, v.y * f, v.z * f); }
inline f32x4 f32x4_normalize(f32x4 v) { const f32 f = 1.0f / f32x4_length(v); return f32x4_init(v.x * f, v.y * f, v.z * f, v.w * f); }

inline f32x2 f32x2_normalizeSafe(f32x2 v) { const f32 len = f32x2_length(v); return len == 0.0f ? v : f32x2_init(v.x / len, v.y / len); }
inline f32x3 f32x3_normalizeSafe(f32x3 v) { const f32 len = f32x3_length(v); return len == 0.0f ? v : f32x3_init(v.x / len, v.y / len, v.z / len); }
inline f32x4 f32x4_normalizeSafe(f32x4 v) { const f32 len = f32x4_length(v); return len == 0.0f ? v : f32x4_init(v.x / len, v.y / len, v.z / len, v.w / len); }

sfz_constexpr_func i8 i8_abs(i8 v) { return v >= 0 ? v : -v; }
sfz_constexpr_func i16 i16_abs(i16 v) { return v >= 0 ? v : -v; }
sfz_constexpr_func i32 i32_abs(i32 v) { return v >= 0 ? v : -v; }
sfz_constexpr_func i64 i64_abs(i64 v) { return v >= 0 ? v : -v; }
sfz_constexpr_func f32 f32_abs(f32 v) { return v >= 0.0f ? v : -v; }
sfz_constexpr_func f64 f64_abs(f64 v) { return v >= 0.0 ? v : -v; }
sfz_constexpr_func f32x2 f32x2_abs(f32x2 v) { return f32x2_init(f32_abs(v.x), f32_abs(v.y)); }
sfz_constexpr_func f32x3 f32x3_abs(f32x3 v) { return f32x3_init(f32_abs(v.x), f32_abs(v.y), f32_abs(v.z)); }
sfz_constexpr_func f32x4 f32x4_abs(f32x4 v) { return f32x4_init(f32_abs(v.x), f32_abs(v.y), f32_abs(v.z), f32_abs(v.w)); }
sfz_constexpr_func i32x2 i32x2_abs(i32x2 v) { return i32x2_init(i32_abs(v.x), i32_abs(v.y)); }
sfz_constexpr_func i32x3 i32x3_abs(i32x3 v) { return i32x3_init(i32_abs(v.x), i32_abs(v.y), i32_abs(v.z)); }
sfz_constexpr_func i32x4 i32x4_abs(i32x4 v) { return i32x4_init(i32_abs(v.x), i32_abs(v.y), i32_abs(v.z), i32_abs(v.w)); }

sfz_constexpr_func i8 i8_min(i8 l, i8 r) { return (l < r) ? l : r; }
sfz_constexpr_func i16 i16_min(i16 l, i16 r) { return (l < r) ? l : r; }
sfz_constexpr_func i32 i32_min(i32 l, i32 r) { return (l < r) ? l : r; }
sfz_constexpr_func i64 i64_min(i64 l, i64 r) { return (l < r) ? l : r; }
sfz_constexpr_func u8 u8_min(u8 l, u8 r) { return (l < r) ? l : r; }
sfz_constexpr_func u16 u16_min(u16 l, u16 r) { return (l < r) ? l : r; }
sfz_constexpr_func u32 u32_min(u32 l, u32 r) { return (l < r) ? l : r; }
sfz_constexpr_func u64 u64_min(u64 l, u64 r) { return (l < r) ? l : r; }
sfz_constexpr_func f32 f32_min(f32 l, f32 r) { return (l < r) ? l : r; }
sfz_constexpr_func f64 f64_min(f64 l, f64 r) { return (l < r) ? l : r; }
sfz_constexpr_func f32x2 f32x2_min(f32x2 l, f32x2 r) { return f32x2_init(f32_min(l.x, r.x), f32_min(l.y, r.y)); }
sfz_constexpr_func f32x3 f32x3_min(f32x3 l, f32x3 r) { return f32x3_init(f32_min(l.x, r.x), f32_min(l.y, r.y), f32_min(l.z, r.z)); }
sfz_constexpr_func f32x4 f32x4_min(f32x4 l, f32x4 r) { return f32x4_init(f32_min(l.x, r.x), f32_min(l.y, r.y), f32_min(l.z, r.z), f32_min(l.w, r.w)); }
sfz_constexpr_func i32x2 i32x2_min(i32x2 l, i32x2 r) { return i32x2_init(i32_min(l.x, r.x), i32_min(l.y, r.y)); }
sfz_constexpr_func i32x3 i32x3_min(i32x3 l, i32x3 r) { return i32x3_init(i32_min(l.x, r.x), i32_min(l.y, r.y), i32_min(l.z, r.z)); }
sfz_constexpr_func i32x4 i32x4_min(i32x4 l, i32x4 r) { return i32x4_init(i32_min(l.x, r.x), i32_min(l.y, r.y), i32_min(l.z, r.z), i32_min(l.w, r.w)); }

sfz_constexpr_func i8 i8_max(i8 l, i8 r) { return (l < r) ? r : l; }
sfz_constexpr_func i16 i16_max(i16 l, i16 r) { return (l < r) ? r : l; }
sfz_constexpr_func i32 i32_max(i32 l, i32 r) { return (l < r) ? r : l; }
sfz_constexpr_func i64 i64_max(i64 l, i64 r) { return (l < r) ? r : l; }
sfz_constexpr_func u8 u8_max(u8 l, u8 r) { return (l < r) ? r : l; }
sfz_constexpr_func u16 u16_max(u16 l, u16 r) { return (l < r) ? r : l; }
sfz_constexpr_func u32 u32_max(u32 l, u32 r) { return (l < r) ? r : l; }
sfz_constexpr_func u64 u64_max(u64 l, u64 r) { return (l < r) ? r : l; }
sfz_constexpr_func f32 f32_max(f32 l, f32 r) { return (l < r) ? r : l; }
sfz_constexpr_func f64 f64_max(f64 l, f64 r) { return (l < r) ? r : l; }
sfz_constexpr_func f32x2 f32x2_max(f32x2 l, f32x2 r) { return f32x2_init(f32_max(l.x, r.x), f32_max(l.y, r.y)); }
sfz_constexpr_func f32x3 f32x3_max(f32x3 l, f32x3 r) { return f32x3_init(f32_max(l.x, r.x), f32_max(l.y, r.y), f32_max(l.z, r.z)); }
sfz_constexpr_func f32x4 f32x4_max(f32x4 l, f32x4 r) { return f32x4_init(f32_max(l.x, r.x), f32_max(l.y, r.y), f32_max(l.z, r.z), f32_max(l.w, r.w)); }
sfz_constexpr_func i32x2 i32x2_max(i32x2 l, i32x2 r) { return i32x2_init(i32_max(l.x, r.x), i32_max(l.y, r.y)); }
sfz_constexpr_func i32x3 i32x3_max(i32x3 l, i32x3 r) { return i32x3_init(i32_max(l.x, r.x), i32_max(l.y, r.y), i32_max(l.z, r.z)); }
sfz_constexpr_func i32x4 i32x4_max(i32x4 l, i32x4 r) { return i32x4_init(i32_max(l.x, r.x), i32_max(l.y, r.y), i32_max(l.z, r.z), i32_max(l.w, r.w)); }

sfz_constexpr_func i32 i32_clamp(i32 v, i32 minVal, i32 maxVal) { return i32_max(minVal, i32_min(v, maxVal)); }
sfz_constexpr_func u32 u32_clamp(u32 v, u32 minVal, u32 maxVal) { return u32_max(minVal, u32_min(v, maxVal)); }
sfz_constexpr_func f32 f32_clamp(f32 v, f32 minVal, f32 maxVal) { return f32_max(minVal, f32_min(v, maxVal)); }
sfz_constexpr_func f32x2 f32x2_clampv(f32x2 v, f32x2 minVal, f32x2 maxVal) { return f32x2_max(minVal, f32x2_min(v, maxVal)); }
sfz_constexpr_func f32x2 f32x2_clamps(f32x2 v, f32 minVal, f32 maxVal) { return f32x2_clampv(v, f32x2_splat(minVal), f32x2_splat(maxVal)); }
sfz_constexpr_func f32x3 f32x3_clampv(f32x3 v, f32x3 minVal, f32x3 maxVal) { return f32x3_max(minVal, f32x3_min(v, maxVal)); }
sfz_constexpr_func f32x3 f32x3_clamps(f32x3 v, f32 minVal, f32 maxVal) { return f32x3_clampv(v, f32x3_splat(minVal), f32x3_splat(maxVal)); }
sfz_constexpr_func f32x4 f32x4_clampv(f32x4 v, f32x4 minVal, f32x4 maxVal) { return f32x4_max(minVal, f32x4_min(v, maxVal)); }
sfz_constexpr_func f32x4 f32x4_clamps(f32x4 v, f32 minVal, f32 maxVal) { return f32x4_clampv(v, f32x4_splat(minVal), f32x4_splat(maxVal)); }
sfz_constexpr_func i32x2 i32x2_clampv(i32x2 v, i32x2 minVal, i32x2 maxVal) { return i32x2_max(minVal, i32x2_min(v, maxVal)); }
sfz_constexpr_func i32x2 i32x2_clamps(i32x2 v, i32 minVal, i32 maxVal) { return i32x2_clampv(v, i32x2_splat(minVal), i32x2_splat(maxVal)); }
sfz_constexpr_func i32x3 i32x3_clampv(i32x3 v, i32x3 minVal, i32x3 maxVal) { return i32x3_max(minVal, i32x3_min(v, maxVal)); }
sfz_constexpr_func i32x3 i32x3_clamps(i32x3 v, i32 minVal, i32 maxVal) { return i32x3_clampv(v, i32x3_splat(minVal), i32x3_splat(maxVal)); }
sfz_constexpr_func i32x4 i32x4_clampv(i32x4 v, i32x4 minVal, i32x4 maxVal) { return i32x4_max(minVal, i32x4_min(v, maxVal)); }
sfz_constexpr_func i32x4 i32x4_clamps(i32x4 v, i32 minVal, i32 maxVal) { return i32x4_clampv(v, i32x4_splat(minVal), i32x4_splat(maxVal)); }

sfz_constexpr_func f32x2 f32x2_from_i32(i32x2 o) { return f32x2_init(f32(o.x), f32(o.y)); }
sfz_constexpr_func f32x2 f32x2_from_u8(u8x2 o) { return f32x2_init(f32(o.x), f32(o.y)); }
sfz_constexpr_func f32x3 f32x3_from_i32(i32x3 o) { return f32x3_init(f32(o.x), f32(o.y), f32(o.z)); }
sfz_constexpr_func f32x4 f32x4_from_i32(i32x4 o) { return f32x4_init(f32(o.x), f32(o.y), f32(o.z), f32(o.w)); }
sfz_constexpr_func f32x4 f32x4_from_u8(u8x4 o) { return f32x4_init(f32(o.x), f32(o.y), f32(o.z), f32(o.w)); }

sfz_constexpr_func i32x2 i32x2_from_f32(f32x2 o) { return i32x2_init(i32(o.x), i32(o.y)); }
sfz_constexpr_func i32x2 i32x2_from_u8(u8x2 o) { return i32x2_init(i32(o.x), i32(o.y)); }
sfz_constexpr_func i32x3 i32x3_from_f32(f32x3 o) { return i32x3_init(i32(o.x), i32(o.y), i32(o.z)); }
sfz_constexpr_func i32x4 i32x4_from_f32(f32x4 o) { return i32x4_init(i32(o.x), i32(o.y), i32(o.z), i32(o.w)); }
sfz_constexpr_func i32x4 i32x4_from_u8(u8x4 o) { return i32x4_init(i32(o.x), i32(o.y), i32(o.z), i32(o.w)); }

sfz_constexpr_func u8x2 u8x2_from_f32(f32x2 o) { return u8x2_init(u8(o.x), u8(o.y)); }
sfz_constexpr_func u8x2 u8x2_from_i32(i32x2 o) { return u8x2_init(u8(o.x), u8(o.y)); }
sfz_constexpr_func u8x4 u8x4_from_f32(f32x4 o) { return u8x4_init(u8(o.x), u8(o.y), u8(o.z), u8(o.w)); }
sfz_constexpr_func u8x4 u8x4_from_i32(i32x4 o) { return u8x4_init(u8(o.x), u8(o.y), u8(o.z), u8(o.w)); }

sfz_forceinline f32 f32_floor(f32 v) { return floorf(v); }
sfz_forceinline f32x2 f32x2_floor(f32x2 v) { return f32x2_init(floorf(v.x), floorf(v.y)); }
sfz_forceinline f32x3 f32x3_floor(f32x3 v) { return f32x3_init(floorf(v.x), floorf(v.y), floorf(v.z)); }
sfz_forceinline f32x4 f32x4_floor(f32x4 v) { return f32x4_init(floorf(v.x), floorf(v.y), floorf(v.z), floorf(v.w)); }

sfz_constexpr_func u32 sfzRoundUpAlignedU32(u32 v, u32 align) { return ((v + align - 1) / align) * align; }
sfz_constexpr_func u64 sfzRoundUpAlignedU64(u64 v, u64 align) { return ((v + align - 1) / align) * align; }

// Vector operators
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

constexpr f32x2& operator+= (f32x2& v, f32x2 o) { v.x += o.x; v.y += o.y; return v; }
constexpr f32x2& operator-= (f32x2& v, f32x2 o) { v.x -= o.x; v.y -= o.y; return v; }
constexpr f32x2& operator*= (f32x2& v, f32 s) { v.x *= s; v.y *= s; return v; }
constexpr f32x2& operator*= (f32x2& v, f32x2 o) { v.x *= o.x; v.y *= o.y; return v; }
constexpr f32x2& operator/= (f32x2& v, f32 s) { v.x /= s; v.y /= s; return v; }
constexpr f32x2& operator/= (f32x2& v, f32x2 o) { v.x /= o.x; v.y /= o.y; return v; }

constexpr f32x2 operator+ (f32x2 v, f32x2 o) { return v += o; }
constexpr f32x2 operator- (f32x2 v, f32x2 o) { return v -= o; }
constexpr f32x2 operator- (f32x2 v) { return f32x2_init(-v.x, -v.y); }
constexpr f32x2 operator* (f32x2 v, f32x2 o) { return v *= o; }
constexpr f32x2 operator* (f32x2 v, f32 s) { return v *= s; }
constexpr f32x2 operator* (f32 s, f32x2 v) { return v * s; }
constexpr f32x2 operator/ (f32x2 v, f32x2 o) { return v /= o; }
constexpr f32x2 operator/ (f32x2 v, f32 s) { return v /= s; }
constexpr f32x2 operator/ (f32 s, f32x2 v) { return f32x2_splat(s) / v; }

constexpr bool operator== (f32x2 v, f32x2 o) { return v.x == o.x && v.y == o.y; }
constexpr bool operator!= (f32x2 v, f32x2 o) { return !(v == o); }

constexpr f32x3& operator+= (f32x3& v, f32x3 o) { v.x += o.x; v.y += o.y; v.z += o.z; return v; }
constexpr f32x3& operator-= (f32x3& v, f32x3 o) { v.x -= o.x; v.y -= o.y; v.z -= o.z; return v; }
constexpr f32x3& operator*= (f32x3& v, f32 s) { v.x *= s; v.y *= s; v.z *= s; return v; }
constexpr f32x3& operator*= (f32x3& v, f32x3 o) { v.x *= o.x; v.y *= o.y; v.z *= o.z; return v; }
constexpr f32x3& operator/= (f32x3& v, f32 s) { v.x /= s; v.y /= s; v.z /= s; return v; }
constexpr f32x3& operator/= (f32x3& v, f32x3 o) { v.x /= o.x; v.y /= o.y; v.z /= o.z; return v; }

constexpr f32x3 operator+ (f32x3 v, f32x3 o) { return v += o; }
constexpr f32x3 operator- (f32x3 v, f32x3 o) { return v -= o; }
constexpr f32x3 operator- (f32x3 v) { return f32x3_init(-v.x, -v.y, -v.z); }
constexpr f32x3 operator* (f32x3 v, f32x3 o) { return v *= o; }
constexpr f32x3 operator* (f32x3 v, f32 s) { return v *= s; }
constexpr f32x3 operator* (f32 s, f32x3 v) { return v * s; }
constexpr f32x3 operator/ (f32x3 v, f32x3 o) { return v /= o; }
constexpr f32x3 operator/ (f32x3 v, f32 s) { return v /= s; }
constexpr f32x3 operator/ (f32 s, f32x3 v) { return f32x3_splat(s) / v; }

constexpr bool operator== (f32x3 v, f32x3 o) { return v.x == o.x && v.y == o.y && v.z == o.z; }
constexpr bool operator!= (f32x3 v, f32x3 o) { return !(v == o); }

constexpr f32x4& operator+= (f32x4& v, f32x4 o) { v.x += o.x; v.y += o.y; v.z += o.z; v.w += o.w; return v; }
constexpr f32x4& operator-= (f32x4& v, f32x4 o) { v.x -= o.x; v.y -= o.y; v.z -= o.z; v.w -= o.w; return v; }
constexpr f32x4& operator*= (f32x4& v, f32 s) { v.x *= s; v.y *= s; v.z *= s; v.w *= s; return v; }
constexpr f32x4& operator*= (f32x4& v, f32x4 o) { v.x *= o.x; v.y *= o.y; v.z *= o.z; v.w *= o.w; return v; }
constexpr f32x4& operator/= (f32x4& v, f32 s) { v.x /= s; v.y /= s; v.z /= s; v.w /= s; return v; }
constexpr f32x4& operator/= (f32x4& v, f32x4 o) { v.x /= o.x; v.y /= o.y; v.z /= o.z; v.w /= o.w; return v; }

constexpr f32x4 operator+ (f32x4 v, f32x4 o) { return v += o; }
constexpr f32x4 operator- (f32x4 v, f32x4 o) { return v -= o; }
constexpr f32x4 operator- (f32x4 v) { return f32x4_init(-v.x, -v.y, -v.z, -v.w); }
constexpr f32x4 operator* (f32x4 v, f32x4 o) { return v *= o; }
constexpr f32x4 operator* (f32x4 v, f32 s) { return v *= s; }
constexpr f32x4 operator* (f32 s, f32x4 v) { return v * s; }
constexpr f32x4 operator/ (f32x4 v, f32x4 o) { return v /= o; }
constexpr f32x4 operator/ (f32x4 v, f32 s) { return v /= s; }
constexpr f32x4 operator/ (f32 s, f32x4 v) { return f32x4_splat(s) / v; }

constexpr bool operator== (f32x4 v, f32x4 o) { return v.x == o.x && v.y == o.y && v.z == o.z && v.w == o.w; }
constexpr bool operator!= (f32x4 v, f32x4 o) { return !(v == o); }


constexpr i32x2& operator+= (i32x2& v, i32x2 o) { v.x += o.x; v.y += o.y; return v; }
constexpr i32x2& operator-= (i32x2& v, i32x2 o) { v.x -= o.x; v.y -= o.y; return v; }
constexpr i32x2& operator*= (i32x2& v, i32 s) { v.x *= s; v.y *= s; return v; }
constexpr i32x2& operator*= (i32x2& v, i32x2 o) { v.x *= o.x; v.y *= o.y; return v; }
constexpr i32x2& operator/= (i32x2& v, i32 s) { v.x /= s; v.y /= s; return v; }
constexpr i32x2& operator/= (i32x2& v, i32x2 o) { v.x /= o.x; v.y /= o.y; return v; }
constexpr i32x2& operator%= (i32x2& v, i32 s) { v.x %= s; v.y %= s; return v; }
constexpr i32x2& operator%= (i32x2& v, i32x2 o) { v.x %= o.x; v.y %= o.y; return v; }

constexpr i32x2 operator+ (i32x2 v, i32x2 o) { return v += o; }
constexpr i32x2 operator- (i32x2 v, i32x2 o) { return v -= o; }
constexpr i32x2 operator- (i32x2 v) { return i32x2_init(-v.x, -v.y); }
constexpr i32x2 operator* (i32x2 v, i32x2 o) { return v *= o; }
constexpr i32x2 operator* (i32x2 v, i32 s) { return v *= s; }
constexpr i32x2 operator* (i32 s, i32x2 v) { return v * s; }
constexpr i32x2 operator/ (i32x2 v, i32x2 o) { return v /= o; }
constexpr i32x2 operator/ (i32x2 v, i32 s) { return v /= s; }
constexpr i32x2 operator/ (i32 s, i32x2 v) { return i32x2_splat(s) / v; }
constexpr i32x2 operator% (i32x2 v, i32x2 o) { return v %= o; }
constexpr i32x2 operator% (i32x2 v, i32 s) { return v %= s; }

constexpr bool operator== (i32x2 v, i32x2 o) { return v.x == o.x && v.y == o.y; }
constexpr bool operator!= (i32x2 v, i32x2 o) { return !(v == o); }

constexpr i32x3& operator+= (i32x3& v, i32x3 o) { v.x += o.x; v.y += o.y; v.z += o.z; return v; }
constexpr i32x3& operator-= (i32x3& v, i32x3 o) { v.x -= o.x; v.y -= o.y; v.z -= o.z; return v; }
constexpr i32x3& operator*= (i32x3& v, i32 s) { v.x *= s; v.y *= s; v.z *= s; return v; }
constexpr i32x3& operator*= (i32x3& v, i32x3 o) { v.x *= o.x; v.y *= o.y; v.z *= o.z; return v; }
constexpr i32x3& operator/= (i32x3& v, i32 s) { v.x /= s; v.y /= s; v.z /= s; return v; }
constexpr i32x3& operator/= (i32x3& v, i32x3 o) { v.x /= o.x; v.y /= o.y; v.z /= o.z; return v; }
constexpr i32x3& operator%= (i32x3& v, i32 s) { v.x %= s; v.y %= s; v.z %= s; return v; }
constexpr i32x3& operator%= (i32x3& v, i32x3 o) { v.x %= o.x; v.y %= o.y; v.z %= o.z; return v; }

constexpr i32x3 operator+ (i32x3 v, i32x3 o) { return v += o; }
constexpr i32x3 operator- (i32x3 v, i32x3 o) { return v -= o; }
constexpr i32x3 operator- (i32x3 v) { return i32x3_init(-v.x, -v.y, -v.z); }
constexpr i32x3 operator* (i32x3 v, i32x3 o) { return v *= o; }
constexpr i32x3 operator* (i32x3 v, i32 s) { return v *= s; }
constexpr i32x3 operator* (i32 s, i32x3 v) { return v * s; }
constexpr i32x3 operator/ (i32x3 v, i32x3 o) { return v /= o; }
constexpr i32x3 operator/ (i32x3 v, i32 s) { return v /= s; }
constexpr i32x3 operator/ (i32 s, i32x3 v) { return i32x3_splat(s) / v; }
constexpr i32x3 operator% (i32x3 v, i32x3 o) { return v %= o; }
constexpr i32x3 operator% (i32x3 v, i32 s) { return v %= s; }

constexpr bool operator== (i32x3 v, i32x3 o) { return v.x == o.x && v.y == o.y && v.z == o.z; }
constexpr bool operator!= (i32x3 v, i32x3 o) { return !(v == o); }

constexpr i32x4& operator+= (i32x4& v, i32x4 o) { v.x += o.x; v.y += o.y; v.z += o.z; v.w += o.w; return v; }
constexpr i32x4& operator-= (i32x4& v, i32x4 o) { v.x -= o.x; v.y -= o.y; v.z -= o.z; v.w -= o.w; return v; }
constexpr i32x4& operator*= (i32x4& v, i32 s) { v.x *= s; v.y *= s; v.z *= s; v.w *= s; return v; }
constexpr i32x4& operator*= (i32x4& v, i32x4 o) { v.x *= o.x; v.y *= o.y; v.z *= o.z; v.w *= o.w; return v; }
constexpr i32x4& operator/= (i32x4& v, i32 s) { v.x /= s; v.y /= s; v.z /= s; v.w /= s; return v; }
constexpr i32x4& operator/= (i32x4& v, i32x4 o) { v.x /= o.x; v.y /= o.y; v.z /= o.z; v.w /= o.w; return v; }
constexpr i32x4& operator%= (i32x4& v, i32 s) { v.x %= s; v.y %= s; v.z %= s; v.w %= s; return v; }
constexpr i32x4& operator%= (i32x4& v, i32x4 o) { v.x %= o.x; v.y %= o.y; v.z %= o.z; v.w %= o.w; return v; }

constexpr i32x4 operator+ (i32x4 v, i32x4 o) { return v += o; }
constexpr i32x4 operator- (i32x4 v, i32x4 o) { return v -= o; }
constexpr i32x4 operator- (i32x4 v) { return i32x4_init(-v.x, -v.y, -v.z, -v.w); }
constexpr i32x4 operator* (i32x4 v, i32x4 o) { return v *= o; }
constexpr i32x4 operator* (i32x4 v, i32 s) { return v *= s; }
constexpr i32x4 operator* (i32 s, i32x4 v) { return v * s; }
constexpr i32x4 operator/ (i32x4 v, i32x4 o) { return v /= o; }
constexpr i32x4 operator/ (i32x4 v, i32 s) { return v /= s; }
constexpr i32x4 operator/ (i32 s, i32x4 v) { return i32x4_splat(s) / v; }
constexpr i32x4 operator% (i32x4 v, i32x4 o) { return v %= o; }
constexpr i32x4 operator% (i32x4 v, i32 s) { return v %= s; }

constexpr bool operator== (i32x4 v, i32x4 o) { return v.x == o.x && v.y == o.y && v.z == o.z && v.w == o.w; }
constexpr bool operator!= (i32x4 v, i32x4 o) { return !(v == o); }


constexpr u8x2& operator+= (u8x2& v, u8x2 o) { v.x += o.x; v.y += o.y; return v; }
constexpr u8x2& operator-= (u8x2& v, u8x2 o) { v.x -= o.x; v.y -= o.y; return v; }
constexpr u8x2& operator*= (u8x2& v, u8 s) { v.x *= s; v.y *= s; return v; }
constexpr u8x2& operator*= (u8x2& v, u8x2 o) { v.x *= o.x; v.y *= o.y; return v; }
constexpr u8x2& operator/= (u8x2& v, u8 s) { v.x /= s; v.y /= s; return v; }
constexpr u8x2& operator/= (u8x2& v, u8x2 o) { v.x /= o.x; v.y /= o.y; return v; }

constexpr u8x2 operator+ (u8x2 v, u8x2 o) { return v += o; }
constexpr u8x2 operator- (u8x2 v, u8x2 o) { return v -= o; }
constexpr u8x2 operator* (u8x2 v, u8x2 o) { return v *= o; }
constexpr u8x2 operator* (u8x2 v, u8 s) { return v *= s; }
constexpr u8x2 operator* (u8 s, u8x2 v) { return v * s; }
constexpr u8x2 operator/ (u8x2 v, u8x2 o) { return v /= o; }
constexpr u8x2 operator/ (u8x2 v, u8 s) { return v /= s; }

constexpr bool operator== (u8x2 v, u8x2 o) { return v.x == o.x && v.y == o.y; }
constexpr bool operator!= (u8x2 v, u8x2 o) { return !(v == o); }

constexpr u8x4& operator+= (u8x4& v, u8x4 o) { v.x += o.x; v.y += o.y; v.z += o.z; v.w += o.w; return v; }
constexpr u8x4& operator-= (u8x4& v, u8x4 o) { v.x -= o.x; v.y -= o.y; v.z -= o.z; v.w -= o.w; return v; }
constexpr u8x4& operator*= (u8x4& v, u8 s) { v.x *= s; v.y *= s; v.z *= s; v.w *= s; return v; }
constexpr u8x4& operator*= (u8x4& v, u8x4 o) { v.x *= o.x; v.y *= o.y; v.z *= o.z; v.w *= o.w; return v; }
constexpr u8x4& operator/= (u8x4& v, u8 s) { v.x /= s; v.y /= s; v.z /= s; v.w /= s; return v; }
constexpr u8x4& operator/= (u8x4& v, u8x4 o) { v.x /= o.x; v.y /= o.y; v.z /= o.z; v.w /= o.w; return v; }

constexpr u8x4 operator+ (u8x4 v, u8x4 o) { return v += o; }
constexpr u8x4 operator- (u8x4 v, u8x4 o) { return v -= o; }
constexpr u8x4 operator* (u8x4 v, u8x4 o) { return v *= o; }
constexpr u8x4 operator* (u8x4 v, u8 s) { return v *= s; }
constexpr u8x4 operator* (u8 s, u8x4 v) { return v * s; }
constexpr u8x4 operator/ (u8x4 v, u8x4 o) { return v /= o; }
constexpr u8x4 operator/ (u8x4 v, u8 s) { return v /= s; }

constexpr bool operator== (u8x4 v, u8x4 o) { return v.x == o.x && v.y == o.y && v.z == o.z && v.w == o.w; }
constexpr bool operator!= (u8x4 v, u8x4 o) { return !(v == o); }

#endif


// Primitive static asserts
// ------------------------------------------------------------------------------------------------

sfz_static_assert(sizeof(bool) == 1); // a) We require c99 bools. b) We require bools to be sane.

sfz_static_assert(sizeof(i8) == 1);
sfz_static_assert(sizeof(i16) == 2);
sfz_static_assert(sizeof(i32) == 4);
sfz_static_assert(sizeof(i64) == 8);

sfz_static_assert(sizeof(u8) == 1);
sfz_static_assert(sizeof(u16) == 2);
sfz_static_assert(sizeof(u32) == 4);
sfz_static_assert(sizeof(u64) == 8);

sfz_static_assert(sizeof(f32) == 4);
sfz_static_assert(sizeof(f64) == 8);

sfz_static_assert(sizeof(f32x2) == sizeof(f32) * 2);
sfz_static_assert(sizeof(f32x3) == sizeof(f32) * 3);
sfz_static_assert(sizeof(f32x4) == sizeof(f32) * 4);

sfz_static_assert(sizeof(i32x2) == sizeof(i32) * 2);
sfz_static_assert(sizeof(i32x3) == sizeof(i32) * 3);
sfz_static_assert(sizeof(i32x4) == sizeof(i32) * 4);

sfz_static_assert(sizeof(u8x2) == sizeof(u8) * 2);
sfz_static_assert(sizeof(u8x4) == sizeof(u8) * 4);


// Matrix & quaternion types
// ------------------------------------------------------------------------------------------------

sfz_struct(SfzMat33) {
	f32x3 rows[3];

#ifdef __cplusplus
	constexpr f32& at(u32 y, u32 x) { return rows[y][x]; }
	constexpr f32 at(u32 y, u32 x) const { return rows[y][x]; }
	constexpr f32x3 column(u32 x) const { return f32x3_init(at(0, x), at(1, x), at(2, x)); }
	constexpr void setColumn(u32 x, f32x3 c) { at(0, x) = c.x; at(1, x) = c.y; at(2, x) = c.z; }
#endif
};

sfz_constexpr_func SfzMat33 sfzMat33InitRows(f32x3 row0, f32x3 row1, f32x3 row2) { return SfzMat33{ row0, row1, row2 }; }
sfz_constexpr_func SfzMat33 sfzMat33InitElems(
	f32 e00, f32 e01, f32 e02,
	f32 e10, f32 e11, f32 e12,
	f32 e20, f32 e21, f32 e22)
{
	return sfzMat33InitRows(
		f32x3_init(e00, e01, e02),
		f32x3_init(e10, e11, e12),
		f32x3_init(e20, e21, e22));
}

sfz_struct(SfzMat44) {
	f32x4 rows[4];

#ifdef __cplusplus
	constexpr f32& at(u32 y, u32 x) { return rows[y][x]; }
	constexpr f32 at(u32 y, u32 x) const { return rows[y][x]; }
	constexpr f32x4 column(u32 x) const { return f32x4_init(at(0, x), at(1, x), at(2, x), at(3, x)); }
	constexpr void setColumn(u32 x, f32x4 c) { at(0, x) = c.x; at(1, x) = c.y; at(2, x) = c.z; at(3, x) = c.w; }
#endif
};

sfz_constexpr_func SfzMat44 sfzMat44InitRows(f32x4 row0, f32x4 row1, f32x4 row2, f32x4 row3) { return SfzMat44{ row0, row1, row2, row3 }; }
sfz_constexpr_func SfzMat44 sfzMat44InitElems(
	f32 e00, f32 e01, f32 e02, f32 e03,
	f32 e10, f32 e11, f32 e12, f32 e13,
	f32 e20, f32 e21, f32 e22, f32 e23,
	f32 e30, f32 e31, f32 e32, f32 e33)
{
	return sfzMat44InitRows(
		f32x4_init(e00, e01, e02, e03),
		f32x4_init(e10, e11, e12, e13),
		f32x4_init(e20, e21, e22, e23),
		f32x4_init(e30, e31, e32, e33));
}

sfz_struct(SfzQuat) {
	// [v, w], v = [x, y, z] in the imaginary space, w is scalar real part
	// i*x + j*y + k*z + w
	f32x3 v;
	f32 w;
};

sfz_constexpr_func SfzQuat sfzQuatInit(f32x3 v, f32 w) { return SfzQuat{ v, w }; }


// Assert macro
// ------------------------------------------------------------------------------------------------

// Assert macros. Lots of magic here to avoid including assert.h or other headers.
//
// sfz_assert() => No-op when NDEBUG is defined (i.e. in release builds)
// sfz_assert_hard() => Always runs, even in release builds.

#if defined(_MSC_VER)

#ifdef _DLL
sfz_extern_c __declspec(dllimport) __declspec(noreturn) void __cdecl abort(void);
#else
sfz_extern_c __declspec(noreturn) void __cdecl abort(void);
#endif
sfz_extern_c void __cdecl __debugbreak(void);

#ifndef NDEBUG
#define sfz_assert(cond) \
	do { \
		if (!(cond)) { \
			__debugbreak(); \
			volatile int assert_dummy_val = 3; \
			(void)assert_dummy_val; \
		} \
	} while(0)
#else
#define sfz_assert(cond) \
	do { \
		(void)sizeof(cond); \
	} while(0)
#endif

#define sfz_assert_hard(cond) \
	do { \
		if (!(cond)) { \
			__debugbreak(); \
			volatile int assert_dummy_val = 3; \
			(void)assert_dummy_val; \
			abort(); \
		} \
	} while(0)

#else
#error "Not implemented for this compiler"
#endif


// Forward declare memcpy(), memove() and memset()
// ------------------------------------------------------------------------------------------------

#if defined(_MSC_VER)

sfz_extern_c void* __cdecl memcpy(void* _Dst, void const* _Src, u64 _Size);
sfz_extern_c void* __cdecl memmove(void* _Dst, void const* _Src, u64 _Size);
sfz_extern_c void* __cdecl memset(void* _Dst, i32 _Val, u64 _Size);

#else
#error "Not implemented for this compiler"
#endif


// Debug information
// ------------------------------------------------------------------------------------------------

// Tiny struct that contains debug information, i.e. file, line number and a message.
// Note that all members are mandatory and MUST be compile-time constants, especially the strings.
sfz_struct(SfzDbgInfo) {
	const char* static_msg;
	const char* file;
	u32 line;
};

// Tiny macro that creates a SfzDbgInfo struct with current file and line number. Message must be a
// compile time constant, i.e. string must be valid for the remaining duration of the program.
#define sfz_dbg(static_msg) SfzDbgInfo{static_msg, __FILE__, u32(__LINE__)}


// Allocator
// ------------------------------------------------------------------------------------------------

// Allocates size bytes aligned to align, returns null on failure.
typedef void* SfzAllocFunc(void* impl_data, SfzDbgInfo dbg, u64 size, u64 align);

// Deallocates memory previously allocated with the same allocator. Deallocating null is required
// to be safe and no-op. Attempting to deallocate memory allocated with another allocator is
// potentially catastrophic undefined behavior.
typedef void SfzDeallocFunc(void* impl_data, void* ptr);

// A memory allocator.
// * Typically a few allocators are created and then kept alive for the remaining duration of
//   the program.
// * Typically, pointers to allocators (Allocator*) are passed around and stored.
// * It is the responsibility of the creator of the allocator instance to ensure that all users
//   that have been provided a pointer have freed all their memory and are done using the allocator
//   before the allocator itself is removed. Often this means that an allocator need to be kept
//   alive for the remaining lifetime of the program.
sfz_struct(SfzAllocator) {
	void* impl_data;
	SfzAllocFunc* alloc_func;
	SfzDeallocFunc* dealloc_func;

#ifdef __cplusplus
	void* alloc(SfzDbgInfo dbg, u64 size, u64 align = 32) { return alloc_func(impl_data, dbg, size, align); }
	void dealloc(void* ptr) { return dealloc_func(impl_data, ptr); }
#endif
};


// Handle
// ------------------------------------------------------------------------------------------------

// A handle used to represent objects in various datastructures.
//
// A handle can store up to 16 777 216 (2^24) different indices. The remaining 8 bits are used to
// store lightweight metadata. 7 bits are used for version, which is typically used to find invalid
// handles when an index is reused. The last bit is reserved for internal datastructure usage, and
// should be ignored by users receiving handles.
//
// A version can be in the range [1, 127]. Zero (0) is reserved as invalid. As a consequence, a
// value of 0 (for all the 32 raw bits) is used to represent null.

sfz_constant u32 SFZ_HANDLE_INDEX_NUM_BITS = 24;
sfz_constant u32 SFZ_HANDLE_INDEX_MASK = 0x00FFFFFFu; // 24 bits index
sfz_constant u32 SFZ_HANDLE_VERSION_MASK = 0x7F000000u; // 7 bits version (1 bit reserved for internal usage)

sfz_struct(SfzHandle) {
	u32 bits;

#ifdef __cplusplus
	u32 idx() const { return bits & SFZ_HANDLE_INDEX_MASK; }
	u8 version() const { return u8((bits & SFZ_HANDLE_VERSION_MASK) >> SFZ_HANDLE_INDEX_NUM_BITS); }
	constexpr bool operator== (SfzHandle other) const { return this->bits == other.bits; }
	constexpr bool operator!= (SfzHandle other) const { return this->bits != other.bits; }
#endif
};

sfz_constant SfzHandle SFZ_NULL_HANDLE = {};

inline SfzHandle sfzHandleInit(u32 idx, u8 version)
{
	sfz_assert((idx & SFZ_HANDLE_INDEX_MASK) == idx);
	sfz_assert((version & u8(0x7F)) == version);
	sfz_assert(version != 0);
	return SfzHandle{ (u32(version) << SFZ_HANDLE_INDEX_NUM_BITS) | idx };
}


// String types
// ------------------------------------------------------------------------------------------------

sfz_struct(SfzStrViewConst) {
	const char* str; // Null-terminated string
	u32 capacity; // Max size of string(i.e.underlying backing memory size)
};

sfz_struct(SfzStrView) {
	char* str; // Null-terminated string
	u32 capacity; // Max size of string (i.e. underlying backing memory size)

#ifdef __cplusplus
	constexpr operator SfzStrViewConst() const { return SfzStrViewConst{ str, capacity }; }
#endif
};

sfz_constexpr_func SfzStrViewConst sfzStrViewToConst(SfzStrView v) { return SfzStrViewConst{ v.str, v.capacity }; }

sfz_struct(SfzStr32) {
	char str[32];
};

sfz_struct(SfzStr96) {
	char str[96];
};

sfz_struct(SfzStr320) {
	char str[320];
};

sfz_struct(SfzStr2560) {
	char str[2560];
};

sfz_constexpr_func SfzStrView sfzStr32ToView(SfzStr32* s) { return SfzStrView{ s->str, 32 }; }
sfz_constexpr_func SfzStrViewConst sfzStr32ToViewConst(const SfzStr32* s) { return SfzStrViewConst{ s->str, 32 }; }

sfz_constexpr_func SfzStrView sfzStr96ToView(SfzStr96* s) { return SfzStrView{ s->str, 96 }; }
sfz_constexpr_func SfzStrViewConst sfzStr96ToViewConst(const SfzStr96* s) { return SfzStrViewConst{ s->str, 96 }; }

sfz_constexpr_func SfzStrView sfzStr320ToView(SfzStr320* s) { return SfzStrView{ s->str, 320 }; }
sfz_constexpr_func SfzStrViewConst sfzStr320ToViewConst(const SfzStr320* s) { return SfzStrViewConst{ s->str, 320 }; }

sfz_constexpr_func SfzStrView sfzStr2560ToView(SfzStr2560* s) { return SfzStrView{ s->str, 2560 }; }
sfz_constexpr_func SfzStrViewConst sfzStr2560ToViewConst(const SfzStr2560* s) { return SfzStrViewConst{ s->str, 2560 }; }

// The hash of a string, it's "ID". Used to cheaply compare strings (e.g. in an hash map). 0 is
// reserved for invalid hashes, recommended to initialize as "SfzStrID anID = SFZ_NULL_STR_ID;".
sfz_struct(SfzStrID) {
	u64 id;

#ifdef __cplusplus
	constexpr bool operator== (SfzStrID o) const { return this->id == o.id; }
	constexpr bool operator!= (SfzStrID o) const { return this->id != o.id; }
#endif
};

sfz_constant SfzStrID SFZ_NULL_STR_ID = {};

#endif // SFZ_H
