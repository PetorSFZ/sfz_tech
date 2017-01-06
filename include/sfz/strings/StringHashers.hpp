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

#include "sfz/strings/DynString.hpp"
#include "sfz/strings/StackString.hpp"

namespace sfz {

using std::uint64_t;

// FNV-1A hash function, based on public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
// See http://isthe.com/chongo/tech/comp/fnv/
inline uint64_t fnv1aHash(const char* str) noexcept
{
	// 64 bit FNV-1 non-zero initial basis, equal to the FNV-0 hash of
	// "chongo <Landon Curt Noll> /\../\", ('\' is not an escape character in this string)
	const uint64_t INITIAL_VALUE = uint64_t(0xCBF29CE484222325);

	// 64-bit magic FNV-1a prime
	const uint64_t FNV_64_PRIME = uint64_t(0x100000001B3);

	// Hash all bytes in string
	uint64_t tmp = INITIAL_VALUE;
	while (char c = *str++) {
		
		// xor bottom with current byte
		tmp ^= uint64_t(c);

		// multiply with FNV magic prime
		tmp *= FNV_64_PRIME;
	}
	return tmp;
}

} // namespace sfz
