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

// Extern "C" macro
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus
#define SFZ_EXTERN_C extern "C"
#else
#define SFZ_EXTERN_C
#endif


// Declare struct macro
// ------------------------------------------------------------------------------------------------

// Macro to declare structs. Used to handle slight differences between C and C++.
#ifdef __cplusplus
#define sfz_struct(name) struct name final
#else
#define sfz_struct(name) \
	struct name; \
	typedef struct name name; \
	struct name
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


#endif // SFZ_H
