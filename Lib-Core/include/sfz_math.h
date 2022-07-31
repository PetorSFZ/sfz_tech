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

#ifndef SFZ_MATH_H
#define SFZ_MATH_H
#pragma once

#include "sfz.h"

// Power of 2
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C sfz_constexpr_func u32 sfzNextPow2_u32(u32 v)
{
	// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	if (v == 0) return 1;
	v -= 1;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v += 1;
	return v;
}

SFZ_EXTERN_C sfz_constexpr_func u64 sfzNextPow2_u64(u64 v)
{
	// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	if (v == 0) return 1;
	v -= 1;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v += 1;
	return v;
}

SFZ_EXTERN_C sfz_constexpr_func u32 sfzLog2OfPow2_u32(u32 pow2Value)
{
	// https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
	const u32 multiplyDeBruijnBitPosition2[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};
	return multiplyDeBruijnBitPosition2[(u32)(pow2Value * 0x077CB531U) >> 27];
}

SFZ_EXTERN_C sfz_constexpr_func u64 sfzLog2OfPow2_u64(u64 pow2Value)
{
	if (pow2Value == (u64(1) << u64(32))) return 32;
	const u32 highBits = u32(pow2Value >> u64(32));
	const u32 highBitsLog2 = sfzLog2OfPow2_u32(highBits);
	if (highBitsLog2 != 0) return highBitsLog2 + 32;
	const u32 lowBits = u32(pow2Value);
	const u32 lowBitsLog2 = sfzLog2OfPow2_u32(lowBits);
	return lowBitsLog2;
}

// Morton Encoding
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C sfz_constexpr_func u32 sfzSplitBy3_u32(u32 v)
{
	v &= 0x3FF; // Mask out the 10 lowest bits
	v = (v | (v << u32(16))) & u32(0xFF0000FFu);
	v = (v | (v << u32(8))) & u32(0x0F00F00Fu);
	v = (v | (v << u32(4))) & u32(0xC30C30C3u);
	v = (v | (v << u32(2))) & u32(0x49249249u);
	return v;
}

SFZ_EXTERN_C sfz_constexpr_func u32 sfzMortonEncode(i32x3 v)
{
	// https://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
	const u32 idx = 
		sfzSplitBy3_u32(u32(v.x)) |
		(sfzSplitBy3_u32(u32(v.y)) << u32(1)) | 
		(sfzSplitBy3_u32(u32(v.z)) << u32(2));
	return idx;
}

#endif // SFZ_MATH_H
