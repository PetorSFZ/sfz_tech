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

namespace sfz {

using std::uintptr_t;

// Memory utils
// ------------------------------------------------------------------------------------------------

// Checks whether a pointer is aligned to a given byte aligment
// \param pointer the pointer to test
// \param alignment the byte aligment
inline bool isAligned(const void* pointer, uint64_t alignment) noexcept
{
	return ((uintptr_t)pointer & (alignment - 1)) == 0;
}

// Checks whether an uint64_t is a power of two or not
inline bool isPowerOfTwo(uint64_t value) noexcept
{
	// See https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
	return value != 0 && (value & (value - 1)) == 0;
}

// Checks whether an uint32_t is a power of two or not
inline bool isPowerOfTwo(uint32_t value) noexcept
{
	// See https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
	return value != 0 && (value & (value - 1)) == 0;
}


} // namespace sfz
