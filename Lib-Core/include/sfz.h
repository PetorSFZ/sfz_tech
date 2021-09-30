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

#if defined(min) || defined(max)
#undef min
#undef max
#endif

// C/C++ compatibility macros
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus
#define SFZ_EXTERN_C extern "C"
#else
#define SFZ_EXTERN_C
#endif

#ifdef __cplusplus
#define SFZ_NODISCARD [[nodiscard]]
#else
#define SFZ_NODISCARD
#endif

#ifdef __cplusplus
#define sfz_struct(name) struct name final
#else
#define sfz_struct(name) \
	struct name; \
	typedef struct name name; \
	struct name
#endif

#ifdef __cplusplus
#define sfz_constant constexpr
#else
#define sfz_constant static const
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

#ifdef __cplusplus

static_assert(sizeof(i8) == 1, "i8 is wrong size");
static_assert(sizeof(i16) == 2, "i16 is wrong size");
static_assert(sizeof(i32) == 4, "i32 is wrong size");
static_assert(sizeof(i64) == 8, "i64 is wrong size");

static_assert(sizeof(u8) == 1, "u8 is wrong size");
static_assert(sizeof(u16) == 2, "u16 is wrong size");
static_assert(sizeof(u32) == 4, "u32 is wrong size");
static_assert(sizeof(u64) == 8, "u64 is wrong size");

static_assert(sizeof(f32) == 4, "f32 is wrong size");
static_assert(sizeof(f64) == 8, "f64 is wrong size");

namespace sfz {

constexpr i8 min(i8 l, i8 r) { return (l < r) ? l : r; }
constexpr i16 min(i16 l, i16 r) { return (l < r) ? l : r; }
constexpr i32 min(i32 l, i32 r) { return (l < r) ? l : r; }
constexpr i64 min(i64 l, i64 r) { return (l < r) ? l : r; }

constexpr u8 min(u8 l, u8 r) { return (l < r) ? l : r; }
constexpr u16 min(u16 l, u16 r) { return (l < r) ? l : r; }
constexpr u32 min(u32 l, u32 r) { return (l < r) ? l : r; }
constexpr u64 min(u64 l, u64 r) { return (l < r) ? l : r; }

constexpr f32 min(f32 l, f32 r) { return (l < r) ? l : r; }
constexpr f64 min(f64 l, f64 r) { return (l < r) ? l : r; }

constexpr i8 max(i8 l, i8 r) { return (l < r) ? r : l; }
constexpr i16 max(i16 l, i16 r) { return (l < r) ? r : l; }
constexpr i32 max(i32 l, i32 r) { return (l < r) ? r : l; }
constexpr i64 max(i64 l, i64 r) { return (l < r) ? r : l; }

constexpr u8 max(u8 l, u8 r) { return (l < r) ? r : l; }
constexpr u16 max(u16 l, u16 r) { return (l < r) ? r : l; }
constexpr u32 max(u32 l, u32 r) { return (l < r) ? r : l; }
constexpr u64 max(u64 l, u64 r) { return (l < r) ? r : l; }

constexpr f32 max(f32 l, f32 r) { return (l < r) ? r : l; }
constexpr f64 max(f64 l, f64 r) { return (l < r) ? r : l; }

constexpr i32 clamp(i32 v, i32 minVal, i32 maxVal) { return sfz::max(minVal, sfz::min(v, maxVal)); }
constexpr u32 clamp(u32 v, u32 minVal, u32 maxVal) { return sfz::max(minVal, sfz::min(v, maxVal)); }
constexpr f32 clamp(f32 v, f32 minVal, f32 maxVal) { return sfz::max(minVal, sfz::min(v, maxVal)); }

} // namespace sfz

#endif

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


// Assert macro
// ------------------------------------------------------------------------------------------------

// Assert macros. Lots of magic here to avoid including assert.h or other headers.
//
// sfz_assert() => No-op when NDEBUG is defined (i.e. in release builds)
// sfz_assert_hard() => ALways runs, even in release builds.

#if defined(_MSC_VER)

#ifdef _DLL
SFZ_EXTERN_C __declspec(dllimport) __declspec(noreturn) void __cdecl abort(void);
#else
SFZ_EXTERN_C __declspec(noreturn) void __cdecl abort(void);
#endif
SFZ_EXTERN_C void __cdecl __debugbreak(void);

#ifndef NDEBUG
#define sfz_assert(cond) do { if (!(cond)) { __debugbreak(); abort(); } } while(0)
#else
#define sfz_assert(cond) do { (void)sizeof(cond); } while(0)
#endif

#define sfz_assert_hard(cond) do { if (!(cond)) { __debugbreak(); abort(); } } while(0)

#else
#error "Not implemented for this compiler"
#endif


// Vector primitives (forward declares)
// ------------------------------------------------------------------------------------------------

struct f32x2;
struct f32x3;
struct f32x4;

struct i32x2;
struct i32x3;
struct i32x4;

struct u8x2;
struct u8x4;


// Vector primitives (f32)
// ------------------------------------------------------------------------------------------------

sfz_struct(f32x2) {
	f32 x, y;

#ifdef __cplusplus
	f32x2() = default;
	constexpr explicit f32x2(const f32* ptr) : x(ptr[0]), y(ptr[1]) {}
	constexpr explicit f32x2(f32 val) : x(val), y(val) {}
	constexpr f32x2(f32 x, f32 y) : x(x), y(y) {}
	constexpr explicit f32x2(const i32x2& o);
	constexpr explicit f32x2(const u8x2& o);

	constexpr f32* data() { return &x; }
	constexpr const f32* data() const { return &x; }
	constexpr f32& operator[] (u32 idx) { sfz_assert(idx < 2); return data()[idx]; }
	constexpr f32 operator[] (u32 idx) const { sfz_assert(idx < 2); return data()[idx]; }

	constexpr f32x2& operator+= (f32x2 o) { x += o.x; y += o.y; return *this; }
	constexpr f32x2& operator-= (f32x2 o) { x -= o.x; y -= o.y; return *this; }
	constexpr f32x2& operator*= (f32 s) { x *= s; y *= s; return *this; }
	constexpr f32x2& operator*= (f32x2 o) { x *= o.x; y *= o.y; return *this; }
	constexpr f32x2& operator/= (f32 s) { x /= s; y /= s; return *this; }
	constexpr f32x2& operator/= (f32x2 o) { x /= o.x; y /= o.y; return *this; }

	constexpr f32x2 operator+ (f32x2 o) const { return f32x2(*this) += o; }
	constexpr f32x2 operator- (f32x2 o) const { return f32x2(*this) -= o; }
	constexpr f32x2 operator- () const { return f32x2(-x, -y); }
	constexpr f32x2 operator* (f32x2 o) const { return f32x2(*this) *= o; }
	constexpr f32x2 operator* (f32 s) const { return f32x2(*this) *= s; }
	constexpr f32x2 operator/ (f32x2 o) const { return f32x2(*this) /= o; }
	constexpr f32x2 operator/ (f32 s) const { return f32x2(*this) /= s; }

	constexpr bool operator== (f32x2 o) const { return x == o.x && y == o.y; }
	constexpr bool operator!= (f32x2 o) const { return !(*this == o); }
#endif
};

sfz_struct(f32x3) {
	f32 x, y, z;

#ifdef __cplusplus
	f32x3() = default;
	constexpr explicit f32x3(const f32* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) {}
	constexpr explicit f32x3(f32 val) : x(val), y(val), z(val) {}
	constexpr f32x3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
	constexpr f32x3(f32x2 xy, f32 z) : x(xy.x), y(xy.y), z(z) {}
	constexpr f32x3(f32 x, f32x2 yz) : x(x), y(yz.x), z(yz.y) {}
	constexpr explicit f32x3(const i32x3& o);

	f32x2& xy() { return *reinterpret_cast<f32x2*>(&x); }
	f32x2& yz() { return *reinterpret_cast<f32x2*>(&y); }
	constexpr f32x2 xy() const { return f32x2(x, y); }
	constexpr f32x2 yz() const { return f32x2(y, z); }

	constexpr f32* data() { return &x; }
	constexpr const f32* data() const { return &x; }
	constexpr f32& operator[] (u32 idx) { sfz_assert(idx < 3); return data()[idx]; }
	constexpr f32 operator[] (u32 idx) const { sfz_assert(idx < 3); return data()[idx]; }

	constexpr f32x3& operator+= (f32x3 o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr f32x3& operator-= (f32x3 o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr f32x3& operator*= (f32 s) { x *= s; y *= s; z *= s; return *this; }
	constexpr f32x3& operator*= (f32x3 o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr f32x3& operator/= (f32 s) { x /= s; y /= s; z /= s; return *this; }
	constexpr f32x3& operator/= (f32x3 o) { x /= o.x; y /= o.y; z /= o.z; return *this; }

	constexpr f32x3 operator+ (f32x3 o) const { return f32x3(*this) += o; }
	constexpr f32x3 operator- (f32x3 o) const { return f32x3(*this) -= o; }
	constexpr f32x3 operator- () const { return f32x3(-x, -y, -z); }
	constexpr f32x3 operator* (f32x3 o) const { return f32x3(*this) *= o; }
	constexpr f32x3 operator* (f32 s) const { return f32x3(*this) *= s; }
	constexpr f32x3 operator/ (f32x3 o) const { return f32x3(*this) /= o; }
	constexpr f32x3 operator/ (f32 s) const { return f32x3(*this) /= s; }

	constexpr bool operator== (f32x3 o) const { return x == o.x && y == o.y && z == o.z; }
	constexpr bool operator!= (f32x3 o) const { return !(*this == o); }
#endif
};

sfz_struct(f32x4) {
	f32 x, y, z, w;

#ifdef __cplusplus
	f32x4() = default;
	constexpr explicit f32x4(const f32* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]), w(ptr[3]) {}
	constexpr explicit f32x4(f32 val) : x(val), y(val), z(val), w(val) {}
	constexpr f32x4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
	constexpr f32x4(f32x3 xyz, f32 w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
	constexpr f32x4(f32 x, f32x3 yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z) {}
	constexpr f32x4(f32x2 xy, f32x2 zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}
	constexpr f32x4(f32x2 xy, f32 z, f32 w) : x(xy.x), y(xy.y), z(z), w(w) {}
	constexpr f32x4(f32 x, f32x2 yz, f32 w) : x(x), y(yz.x), z(yz.y), w(w) {}
	constexpr f32x4(f32 x, f32 y, f32x2 zw) : x(x), y(y), z(zw.x), w(zw.y) {}
	constexpr explicit f32x4(const i32x4& o);
	constexpr explicit f32x4(const u8x4& o);

	f32x2& xy() { return *reinterpret_cast<f32x2*>(&x); }
	f32x2& yz() { return *reinterpret_cast<f32x2*>(&y); }
	f32x2& zw() { return *reinterpret_cast<f32x2*>(&z); }
	f32x3& xyz() { return *reinterpret_cast<f32x3*>(&x); }
	f32x3& yzw() { return *reinterpret_cast<f32x3*>(&y); }
	constexpr f32x2 xy() const { return f32x2(x, y); }
	constexpr f32x2 yz() const { return f32x2(y, z); }
	constexpr f32x2 zw() const { return f32x2(z, w); }
	constexpr f32x3 xyz() const { return f32x3(x, y, z); }
	constexpr f32x3 yzw() const { return f32x3(y, z, w); }

	constexpr f32* data() { return &x; }
	constexpr const f32* data() const { return &x; }
	constexpr f32& operator[] (u32 idx) { sfz_assert(idx < 4); return data()[idx]; }
	constexpr f32 operator[] (u32 idx) const { sfz_assert(idx < 4); return data()[idx]; }

	constexpr f32x4& operator+= (f32x4 o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr f32x4& operator-= (f32x4 o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr f32x4& operator*= (f32 s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr f32x4& operator*= (f32x4 o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr f32x4& operator/= (f32 s) { x /= s; y /= s; z /= s; w /= s; return *this; }
	constexpr f32x4& operator/= (f32x4 o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
	
	constexpr f32x4 operator+ (f32x4 o) const { return f32x4(*this) += o; }
	constexpr f32x4 operator- (f32x4 o) const { return f32x4(*this) -= o; }
	constexpr f32x4 operator- () const { return f32x4(-x, -y, -z, -w); }
	constexpr f32x4 operator* (f32x4 o) const { return f32x4(*this) *= o; }
	constexpr f32x4 operator* (f32 s) const { return f32x4(*this) *= s; }
	constexpr f32x4 operator/ (f32x4 o) const { return f32x4(*this) /= o; }
	constexpr f32x4 operator/ (f32 s) const { return f32x4(*this) /= s; }
	
	constexpr bool operator== (f32x4 o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (f32x4 o) const { return !(*this == o); }
#endif
};

#ifdef __cplusplus

constexpr f32x2 operator* (f32 s, f32x2 v) { return v * s; }
constexpr f32x3 operator* (f32 s, f32x3 v) { return v * s; }
constexpr f32x4 operator* (f32 s, f32x4 v) { return v * s; }

constexpr f32x2 operator/ (f32 s, f32x2 v) { return f32x2(s) / v; }
constexpr f32x3 operator/ (f32 s, f32x3 v) { return f32x3(s) / v; }
constexpr f32x4 operator/ (f32 s, f32x4 v) { return f32x4(s) / v; }

namespace sfz {

constexpr f32 dot(f32x2 l, f32x2 r) { return l.x * r.x + l.y * r.y; }
constexpr f32 dot(f32x3 l, f32x3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
constexpr f32 dot(f32x4 l, f32x4 r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }
constexpr f32x3 cross(f32x3 l, f32x3 r) { return f32x3(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x); }

constexpr f32x2 min(f32x2 l, f32x2 r) { return f32x2(sfz::min(l.x, r.x), sfz::min(l.y, r.y)); }
constexpr f32x3 min(f32x3 l, f32x3 r) { return f32x3(sfz::min(l.x, r.x), sfz::min(l.y, r.y), sfz::min(l.z, r.z)); }
constexpr f32x4 min(f32x4 l, f32x4 r) { return f32x4(sfz::min(l.x, r.x), sfz::min(l.y, r.y), sfz::min(l.z, r.z), sfz::min(l.w, r.w)); }

constexpr f32x2 max(f32x2 l, f32x2 r) { return f32x2(sfz::max(l.x, r.x), sfz::max(l.y, r.y)); }
constexpr f32x3 max(f32x3 l, f32x3 r) { return f32x3(sfz::max(l.x, r.x), sfz::max(l.y, r.y), sfz::max(l.z, r.z)); }
constexpr f32x4 max(f32x4 l, f32x4 r) { return f32x4(sfz::max(l.x, r.x), sfz::max(l.y, r.y), sfz::max(l.z, r.z), sfz::max(l.w, r.w)); }

constexpr f32x2 clamp(f32x2 v, f32x2 minVal, f32x2 maxVal) { return sfz::max(minVal, sfz::min(v, maxVal)); }
constexpr f32x2 clamp(f32x2 v, f32 minVal, f32 maxVal) { return sfz::clamp(v, f32x2(minVal), f32x2(maxVal)); }
constexpr f32x3 clamp(f32x3 v, f32x3 minVal, f32x3 maxVal) { return sfz::max(minVal, sfz::min(v, maxVal)); }
constexpr f32x3 clamp(f32x3 v, f32 minVal, f32 maxVal) { return sfz::clamp(v, f32x3(minVal), f32x3(maxVal)); }
constexpr f32x4 clamp(f32x4 v, f32x4 minVal, f32x4 maxVal) { return sfz::max(minVal, sfz::min(v, maxVal)); }
constexpr f32x4 clamp(f32x4 v, f32 minVal, f32 maxVal) { return sfz::clamp(v, f32x4(minVal), f32x4(maxVal)); }

} // namespace sfz

static_assert(sizeof(f32x2) == sizeof(f32) * 2 && alignof(f32x2) == alignof(f32), "");
static_assert(sizeof(f32x3) == sizeof(f32) * 3 && alignof(f32x3) == alignof(f32), "");
static_assert(sizeof(f32x4) == sizeof(f32) * 4 && alignof(f32x4) == alignof(f32), "");

#endif


// Vector primitives (i32)
// ------------------------------------------------------------------------------------------------

sfz_struct(i32x2) {
	i32 x, y;

#ifdef __cplusplus
	i32x2() = default;
	constexpr explicit i32x2(const i32* ptr) : x(ptr[0]), y(ptr[1]) {}
	constexpr explicit i32x2(i32 val) : x(val), y(val) {}
	constexpr i32x2(i32 x, i32 y) : x(x), y(y) {}
	constexpr explicit i32x2(const f32x2& o);
	constexpr explicit i32x2(const u8x2& o);

	constexpr i32* data() { return &x; }
	constexpr const i32* data() const { return &x; }
	constexpr i32& operator[] (u32 idx) { sfz_assert(idx < 2); return data()[idx]; }
	constexpr i32 operator[] (u32 idx) const { sfz_assert(idx < 2); return data()[idx]; }

	constexpr i32x2& operator+= (i32x2 o) { x += o.x; y += o.y; return *this; }
	constexpr i32x2& operator-= (i32x2 o) { x -= o.x; y -= o.y; return *this; }
	constexpr i32x2& operator*= (i32 s) { x *= s; y *= s; return *this; }
	constexpr i32x2& operator*= (i32x2 o) { x *= o.x; y *= o.y; return *this; }
	constexpr i32x2& operator/= (i32 s) { x /= s; y /= s; return *this; }
	constexpr i32x2& operator/= (i32x2 o) { x /= o.x; y /= o.y; return *this; }
	constexpr i32x2& operator%= (i32 s) { x %= s; y %= s; return *this; }
	constexpr i32x2& operator%= (i32x2 o) { x %= o.x; y %= o.y; return *this; }

	constexpr i32x2 operator+ (i32x2 o) const { return i32x2(*this) += o; }
	constexpr i32x2 operator- (i32x2 o) const { return i32x2(*this) -= o; }
	constexpr i32x2 operator- () const { return i32x2(-x, -y); }
	constexpr i32x2 operator* (i32x2 o) const { return i32x2(*this) *= o; }
	constexpr i32x2 operator* (i32 s) const { return i32x2(*this) *= s; }
	constexpr i32x2 operator/ (i32x2 o) const { return i32x2(*this) /= o; }
	constexpr i32x2 operator/ (i32 s) const { return i32x2(*this) /= s; }
	constexpr i32x2 operator% (i32x2 o) const { return i32x2(*this) %= o; }
	constexpr i32x2 operator% (i32 s) const { return i32x2(*this) %= s; }

	constexpr bool operator== (i32x2 o) const { return x == o.x && y == o.y; }
	constexpr bool operator!= (i32x2 o) const { return !(*this == o); }
#endif
};

sfz_struct(i32x3) {
	i32 x, y, z;

#ifdef __cplusplus
	i32x3() = default;
	constexpr explicit i32x3(const i32* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) {}
	constexpr explicit i32x3(i32 val) : x(val), y(val), z(val) {}
	constexpr i32x3(i32 x, i32 y, i32 z) : x(x), y(y), z(z) {}
	constexpr i32x3(i32x2 xy, i32 z) : x(xy.x), y(xy.y), z(z) {}
	constexpr i32x3(i32 x, i32x2 yz) : x(x), y(yz.x), z(yz.y) {}
	constexpr explicit i32x3(const f32x3& o);

	i32x2& xy() { return *reinterpret_cast<i32x2*>(&x); }
	i32x2& yz() { return *reinterpret_cast<i32x2*>(&y); }
	constexpr i32x2 xy() const { return i32x2(x, y); }
	constexpr i32x2 yz() const { return i32x2(y, z); }

	constexpr i32* data() { return &x; }
	constexpr const i32* data() const { return &x; }
	constexpr i32& operator[] (u32 idx) { sfz_assert(idx < 3); return data()[idx]; }
	constexpr i32 operator[] (u32 idx) const { sfz_assert(idx < 3); return data()[idx]; }

	constexpr i32x3& operator+= (i32x3 o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr i32x3& operator-= (i32x3 o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr i32x3& operator*= (i32 s) { x *= s; y *= s; z *= s; return *this; }
	constexpr i32x3& operator*= (i32x3 o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr i32x3& operator/= (i32 s) { x /= s; y /= s; z /= s; return *this; }
	constexpr i32x3& operator/= (i32x3 o) { x /= o.x; y /= o.y; z /= o.z; return *this; }
	constexpr i32x3& operator%= (i32 s) { x %= s; y %= s; z %= s; return *this; }
	constexpr i32x3& operator%= (i32x3 o) { x %= o.x; y %= o.y; z %= o.z; return *this; }

	constexpr i32x3 operator+ (i32x3 o) const { return i32x3(*this) += o; }
	constexpr i32x3 operator- (i32x3 o) const { return i32x3(*this) -= o; }
	constexpr i32x3 operator- () const { return i32x3(-x, -y, -z); }
	constexpr i32x3 operator* (i32x3 o) const { return i32x3(*this) *= o; }
	constexpr i32x3 operator* (i32 s) const { return i32x3(*this) *= s; }
	constexpr i32x3 operator/ (i32x3 o) const { return i32x3(*this) /= o; }
	constexpr i32x3 operator/ (i32 s) const { return i32x3(*this) /= s; }
	constexpr i32x3 operator% (i32x3 o) const { return i32x3(*this) %= o; }
	constexpr i32x3 operator% (i32 s) const { return i32x3(*this) %= s; }

	constexpr bool operator== (i32x3 o) const { return x == o.x && y == o.y && z == o.z; }
	constexpr bool operator!= (i32x3 o) const { return !(*this == o); }
#endif
};

sfz_struct(i32x4) {
	i32 x, y, z, w;

#ifdef __cplusplus
	i32x4() = default;
	constexpr explicit i32x4(const i32* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]), w(ptr[3]) {}
	constexpr explicit i32x4(i32 val) : x(val), y(val), z(val), w(val) {}
	constexpr i32x4(i32 x, i32 y, i32 z, i32 w) : x(x), y(y), z(z), w(w) {}
	constexpr i32x4(i32x3 xyz, i32 w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
	constexpr i32x4(i32 x, i32x3 yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z) {}
	constexpr i32x4(i32x2 xy, i32x2 zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}
	constexpr i32x4(i32x2 xy, i32 z, i32 w) : x(xy.x), y(xy.y), z(z), w(w) {}
	constexpr i32x4(i32 x, i32x2 yz, i32 w) : x(x), y(yz.x), z(yz.y), w(w) {}
	constexpr i32x4(i32 x, i32 y, i32x2 zw) : x(x), y(y), z(zw.x), w(zw.y) {}
	constexpr explicit i32x4(const f32x4& o);
	constexpr explicit i32x4(const u8x4& o);

	i32x2& xy() { return *reinterpret_cast<i32x2*>(&x); }
	i32x2& yz() { return *reinterpret_cast<i32x2*>(&y); }
	i32x2& zw() { return *reinterpret_cast<i32x2*>(&z); }
	i32x3& xyz() { return *reinterpret_cast<i32x3*>(&x); }
	i32x3& yzw() { return *reinterpret_cast<i32x3*>(&y); }
	constexpr i32x2 xy() const { return i32x2(x, y); }
	constexpr i32x2 yz() const { return i32x2(y, z); }
	constexpr i32x2 zw() const { return i32x2(z, w); }
	constexpr i32x3 xyz() const { return i32x3(x, y, z); }
	constexpr i32x3 yzw() const { return i32x3(y, z, w); }

	constexpr i32* data() { return &x; }
	constexpr const i32* data() const { return &x; }
	constexpr i32& operator[] (u32 idx) { sfz_assert(idx < 4); return data()[idx]; }
	constexpr i32 operator[] (u32 idx) const { sfz_assert(idx < 4); return data()[idx]; }

	constexpr i32x4& operator+= (i32x4 o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr i32x4& operator-= (i32x4 o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr i32x4& operator*= (i32 s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr i32x4& operator*= (i32x4 o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr i32x4& operator/= (i32 s) { x /= s; y /= s; z /= s; w /= s; return *this; }
	constexpr i32x4& operator/= (i32x4 o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
	constexpr i32x4& operator%= (i32 s) { x %= s; y %= s; z %= s; w %= s; return *this; }
	constexpr i32x4& operator%= (i32x4 o) { x %= o.x; y %= o.y; z %= o.z; w %= o.w; return *this; }

	constexpr i32x4 operator+ (i32x4 o) const { return i32x4(*this) += o; }
	constexpr i32x4 operator- (i32x4 o) const { return i32x4(*this) -= o; }
	constexpr i32x4 operator- () const { return i32x4(-x, -y, -z, -w); }
	constexpr i32x4 operator* (i32x4 o) const { return i32x4(*this) *= o; }
	constexpr i32x4 operator* (i32 s) const { return i32x4(*this) *= s; }
	constexpr i32x4 operator/ (i32x4 o) const { return i32x4(*this) /= o; }
	constexpr i32x4 operator/ (i32 s) const { return i32x4(*this) /= s; }
	constexpr i32x4 operator% (i32x4 o) const { return i32x4(*this) %= o; }
	constexpr i32x4 operator% (i32 s) const { return i32x4(*this) %= s; }

	constexpr bool operator== (i32x4 o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (i32x4 o) const { return !(*this == o); }
#endif
};

#ifdef __cplusplus

constexpr i32x2 operator* (i32 s, i32x2 v) { return v * s; }
constexpr i32x3 operator* (i32 s, i32x3 v) { return v * s; }
constexpr i32x4 operator* (i32 s, i32x4 v) { return v * s; }

constexpr i32x2 operator/ (i32 s, i32x2 v) { return i32x2(s) / v; }
constexpr i32x3 operator/ (i32 s, i32x3 v) { return i32x3(s) / v; }
constexpr i32x4 operator/ (i32 s, i32x4 v) { return i32x4(s) / v; }

namespace sfz {

constexpr i32 dot(i32x2 l, i32x2 r) { return l.x * r.x + l.y * r.y; }
constexpr i32 dot(i32x3 l, i32x3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
constexpr i32 dot(i32x4 l, i32x4 r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }
constexpr i32x3 cross(i32x3 l, i32x3 r) { return i32x3(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x); }

constexpr i32x2 min(i32x2 l, i32x2 r) { return i32x2(sfz::min(l.x, r.x), sfz::min(l.y, r.y)); }
constexpr i32x3 min(i32x3 l, i32x3 r) { return i32x3(sfz::min(l.x, r.x), sfz::min(l.y, r.y), sfz::min(l.z, r.z)); }
constexpr i32x4 min(i32x4 l, i32x4 r) { return i32x4(sfz::min(l.x, r.x), sfz::min(l.y, r.y), sfz::min(l.z, r.z), sfz::min(l.w, r.w)); }

constexpr i32x2 max(i32x2 l, i32x2 r) { return i32x2(sfz::max(l.x, r.x), sfz::max(l.y, r.y)); }
constexpr i32x3 max(i32x3 l, i32x3 r) { return i32x3(sfz::max(l.x, r.x), sfz::max(l.y, r.y), sfz::max(l.z, r.z)); }
constexpr i32x4 max(i32x4 l, i32x4 r) { return i32x4(sfz::max(l.x, r.x), sfz::max(l.y, r.y), sfz::max(l.z, r.z), sfz::max(l.w, r.w)); }

constexpr i32x2 clamp(i32x2 v, i32x2 minVal, i32x2 maxVal) { return sfz::max(minVal, sfz::min(v, maxVal)); }
constexpr i32x2 clamp(i32x2 v, i32 minVal, i32 maxVal) { return sfz::clamp(v, i32x2(minVal), i32x2(maxVal)); }
constexpr i32x3 clamp(i32x3 v, i32x3 minVal, i32x3 maxVal) { return sfz::max(minVal, sfz::min(v, maxVal)); }
constexpr i32x3 clamp(i32x3 v, i32 minVal, i32 maxVal) { return sfz::clamp(v, i32x3(minVal), i32x3(maxVal)); }
constexpr i32x4 clamp(i32x4 v, i32x4 minVal, i32x4 maxVal) { return sfz::max(minVal, sfz::min(v, maxVal)); }
constexpr i32x4 clamp(i32x4 v, i32 minVal, i32 maxVal) { return sfz::clamp(v, i32x4(minVal), i32x4(maxVal)); }

} // namespace sfz

static_assert(sizeof(i32x2) == sizeof(i32) * 2 && alignof(i32x2) == alignof(i32), "");
static_assert(sizeof(i32x3) == sizeof(i32) * 3 && alignof(i32x3) == alignof(i32), "");
static_assert(sizeof(i32x4) == sizeof(i32) * 4 && alignof(i32x4) == alignof(i32), "");

#endif


// Vector primitives (u8)
// ------------------------------------------------------------------------------------------------

sfz_struct(u8x2) {
	u8 x, y;

#ifdef __cplusplus
	u8x2() = default;
	constexpr explicit u8x2(const u8* ptr) : x(ptr[0]), y(ptr[1]) {}
	constexpr explicit u8x2(u8 val) : x(val), y(val) {}
	constexpr u8x2(u8 x, u8 y) : x(x), y(y) {}
	constexpr explicit u8x2(const f32x2& o);
	constexpr explicit u8x2(const i32x2& o);

	constexpr u8x2& operator+= (u8x2 o) { x += o.x; y += o.y; return *this; }
	constexpr u8x2& operator-= (u8x2 o) { x -= o.x; y -= o.y; return *this; }
	constexpr u8x2& operator*= (u8 s) { x *= s; y *= s; return *this; }
	constexpr u8x2& operator*= (u8x2 o) { x *= o.x; y *= o.y; return *this; }
	constexpr u8x2& operator/= (u8 s) { x /= s; y /= s; return *this; }
	constexpr u8x2& operator/= (u8x2 o) { x /= o.x; y /= o.y; return *this; }

	constexpr u8x2 operator+ (u8x2 o) const { return u8x2(*this) += o; }
	constexpr u8x2 operator- (u8x2 o) const { return u8x2(*this) -= o; }
	constexpr u8x2 operator- () const { return u8x2(-x, -y); }
	constexpr u8x2 operator* (u8x2 o) const { return u8x2(*this) *= o; }
	constexpr u8x2 operator* (u8 s) const { return u8x2(*this) *= s; }
	constexpr u8x2 operator/ (u8x2 o) const { return u8x2(*this) /= o; }
	constexpr u8x2 operator/ (u8 s) const { return u8x2(*this) /= s; }

	constexpr bool operator== (u8x2 o) const { return x == o.x && y == o.y; }
	constexpr bool operator!= (u8x2 o) const { return !(*this == o); }
#endif
};

sfz_struct(u8x4) {
	u8 x, y, z, w;

#ifdef __cplusplus
	u8x4() = default;
	constexpr explicit u8x4(const u8* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]), w(ptr[3]) {}
	constexpr explicit u8x4(u8 val) : x(val), y(val), z(val), w(val) {}
	constexpr u8x4(u8 x, u8 y, u8 z, u8 w) : x(x), y(y), z(z), w(w) {}
	constexpr u8x4(u8x2 xy, u8x2 zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}
	constexpr u8x4(u8x2 xy, u8 z, u8 w) : x(xy.x), y(xy.y), z(z), w(w) {}
	constexpr u8x4(u8 x, u8x2 yz, u8 w) : x(x), y(yz.x), z(yz.y), w(w) {}
	constexpr u8x4(u8 x, u8 y, u8x2 zw) : x(x), y(y), z(zw.x), w(zw.y) {}
	constexpr explicit u8x4(const f32x4& o);
	constexpr explicit u8x4(const i32x4& o);

	u8x2& xy() { return *reinterpret_cast<u8x2*>(&x); }
	u8x2& yz() { return *reinterpret_cast<u8x2*>(&y); }
	u8x2& zw() { return *reinterpret_cast<u8x2*>(&z); }
	constexpr u8x2 xy() const { return u8x2(x, y); }
	constexpr u8x2 yz() const { return u8x2(y, z); }
	constexpr u8x2 zw() const { return u8x2(z, w); }

	constexpr u8x4& operator+= (u8x4 o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr u8x4& operator-= (u8x4 o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr u8x4& operator*= (u8 s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr u8x4& operator*= (u8x4 o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr u8x4& operator/= (u8 s) { x /= s; y /= s; z /= s; w /= s; return *this; }
	constexpr u8x4& operator/= (u8x4 o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }

	constexpr u8x4 operator+ (u8x4 o) const { return u8x4(*this) += o; }
	constexpr u8x4 operator- (u8x4 o) const { return u8x4(*this) -= o; }
	constexpr u8x4 operator- () const { return u8x4(-x, -y, -z, -w); }
	constexpr u8x4 operator* (u8x4 o) const { return u8x4(*this) *= o; }
	constexpr u8x4 operator* (u8 s) const { return u8x4(*this) *= s; }
	constexpr u8x4 operator/ (u8x4 o) const { return u8x4(*this) /= o; }
	constexpr u8x4 operator/ (u8 s) const { return u8x4(*this) /= s; }

	constexpr bool operator== (u8x4 o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (u8x4 o) const { return !(*this == o); }
#endif
};

#ifdef __cplusplus

constexpr u8x2 operator* (u8 s, u8x2 v) { return v * s; }
constexpr u8x4 operator* (u8 s, u8x4 v) { return v * s; }

static_assert(sizeof(u8x2) == sizeof(u8) * 2 && alignof(u8x2) == alignof(u8), "");
static_assert(sizeof(u8x4) == sizeof(u8) * 4 && alignof(u8x4) == alignof(u8), "");

#endif


// Vector primitives (common)
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

constexpr f32x2::f32x2(const i32x2& o) : x(f32(o.x)), y(f32(o.y)) {}
constexpr f32x2::f32x2(const u8x2& o) : x(f32(o.x)), y(f32(o.y)) {}
constexpr f32x3::f32x3(const i32x3& o) : x(f32(o.x)), y(f32(o.y)), z(f32(o.z)) {}
constexpr f32x4::f32x4(const i32x4& o) : x(f32(o.x)), y(f32(o.y)), z(f32(o.z)), w(f32(o.w)) {}
constexpr f32x4::f32x4(const u8x4& o) : x(f32(o.x)), y(f32(o.y)), z(f32(o.z)), w(f32(o.w)) {}

constexpr i32x2::i32x2(const f32x2& o) : x(i32(o.x)), y(i32(o.y)) {}
constexpr i32x2::i32x2(const u8x2& o) : x(i32(o.x)), y(i32(o.y)) {}
constexpr i32x3::i32x3(const f32x3& o) : x(i32(o.x)), y(i32(o.y)), z(i32(o.z)) {}
constexpr i32x4::i32x4(const f32x4& o) : x(i32(o.x)), y(i32(o.y)), z(i32(o.z)), w(i32(o.w)) {}
constexpr i32x4::i32x4(const u8x4& o) : x(i32(o.x)), y(i32(o.y)), z(i32(o.z)), w(i32(o.w)) {}

constexpr u8x2::u8x2(const f32x2& o) : x(u8(o.x)), y(u8(o.y)) {}
constexpr u8x2::u8x2(const i32x2& o) : x(u8(o.x)), y(u8(o.y)) {}
constexpr u8x4::u8x4(const f32x4& o) : x(u8(o.x)), y(u8(o.y)), z(u8(o.z)), w(u8(o.w)) {}
constexpr u8x4::u8x4(const i32x4& o) : x(u8(o.x)), y(u8(o.y)), z(u8(o.z)), w(u8(o.w)) {}

#endif


// Debug information
// ------------------------------------------------------------------------------------------------

// Tiny struct that contains debug information, i.e. file, line number and a message.
// Note that all members are mandatory and MUST be compile-time constants, especially the strings.
sfz_struct(SfzDbgInfo) {
	const char* staticMsg;
	const char* file;
	u32 line;
};

// Tiny macro that creates a SfzDbgInfo struct with current file and line number. Message must be a
// compile time constant, i.e. string must be valid for the remaining duration of the program.
#define sfz_dbg(staticMsg) SfzDbgInfo{staticMsg, __FILE__, __LINE__}


// Allocator
// ------------------------------------------------------------------------------------------------

// Allocates size bytes aligned to align, returns null on failure.
typedef void* SfzAllocFunc(void* implData, SfzDbgInfo dbg, u64 size, u64 align);

// Deallocates memory previously allocated with the same allocator. Deallocating null is required
// to be safe and no-op. Attempting to deallocate memory allocated with another allocator is
// potentially catastrophic undefined behavior.
typedef void SfzDeallocFunc(void* implData, void* ptr);

// A memory allocator.
// * Typically a few allocators are created and then kept alive for the remaining duration of
//   the program.
// * Typically, pointers to allocators (Allocator*) are passed around and stored.
// * It is the responsibility of the creator of the allocator instance to ensure that all users
//   that have been provided a pointer have freed all their memory and are done using the allocator
//   before the allocator itself is removed. Often this means that an allocator need to be kept
//   alive for the remaining lifetime of the program.
sfz_struct(SfzAllocator) {
	void* implData;
	SfzAllocFunc* allocFunc;
	SfzDeallocFunc* deallocFunc;

#ifdef __cplusplus
	void* alloc(SfzDbgInfo dbg, u64 size, u64 align = 32) { return allocFunc(implData, dbg, size, align); }
	void dealloc(void* ptr) { return deallocFunc(implData, ptr); }
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
	
	static SfzHandle create(u32 idx, u8 version)
	{
		sfz_assert((idx & SFZ_HANDLE_INDEX_MASK) == idx);
		sfz_assert((version & u8(0x7F)) == version);
		sfz_assert(version != 0);
		SfzHandle handle;
		handle.bits = (u32(version) << SFZ_HANDLE_INDEX_NUM_BITS) | idx;
		return handle;
	}

	constexpr bool operator== (SfzHandle other) const { return this->bits == other.bits; }
	constexpr bool operator!= (SfzHandle other) const { return this->bits != other.bits; }
#endif
};

sfz_constant SfzHandle SFZ_NULL_HANDLE = {};

#ifdef __cplusplus
#define SFZ_TYPED_HANDLE(name) \
struct name final { \
	SfzHandle h; \
	bool operator== (name other) const { return this->h == other.h; } \
	bool operator!= (name other) const { return this->h != other.h; } \
	bool operator== (SfzHandle other) const { return this->h == other; } \
	bool operator!= (SfzHandle other) const { return this->h != other; } \
};
#else
#define SFZ_TYPED_HANDLE(name) typedef struct name { SfzHandle h; } name;
#endif

#endif // SFZ_H
