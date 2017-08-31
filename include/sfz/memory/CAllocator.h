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

#ifdef __cplusplus
#include <cstdint>
using std::uint64_t;
#else
#include <stdint.h>
#endif

// C interface
#ifdef __cplusplus
extern "C" {
#endif

// sfzAllocator struct
// ------------------------------------------------------------------------------------------------

/// A C struct wrapper around sfz::Allocator, see Allocator.hpp. Member implData must be passed
/// into both the allocate and deallocate functions.
typedef struct {
	void* (*allocate)(void* implData, uint64_t size, uint64_t alignment, const char* name);
	void (*deallocate)(void* implData, void* pointer);
	const char* (*getName)(void* implData);
	void* implData;
} sfzAllocator;

#define SFZ_C_ALLOCATE(allocator, size, alignment, name) (allocator)->allocate((allocator)->implData, (size), (alignment), (name))
#define SFZ_C_DEALLOCATE(allocator, pointer) (allocator)->deallocate((allocator)->implData, (pointer))
#define SFZ_C_GET_NAME(allocator) (allocator)->getName((allocator)->implData);

// End C interface
#ifdef __cplusplus
}
#endif
