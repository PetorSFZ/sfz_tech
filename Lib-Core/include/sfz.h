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
	
	SfzHandle() = default;
	constexpr SfzHandle(u32 idx, u8 version)
		: bits((u32(version) << SFZ_HANDLE_INDEX_NUM_BITS) | idx)
	{
		sfz_assert((idx & SFZ_HANDLE_INDEX_MASK) == idx);
		sfz_assert((version & u8(0x7F)) == version);
		sfz_assert(version != 0);
	}

	constexpr bool operator== (SfzHandle other) const { return this->bits == other.bits; }
	constexpr bool operator!= (SfzHandle other) const { return this->bits != other.bits; }
#endif
};

sfz_constant SfzHandle SFZ_NULL_HANDLE = {};

#endif // SFZ_H
