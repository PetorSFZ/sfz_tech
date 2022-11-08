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

sfz_constexpr_func bool sfzIsPow2_u32(u32 v)
{
	// See https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
	return v != 0 && (v & (v - 1)) == 0;
}

sfz_constexpr_func bool sfzIsPow2_u64(u64 v)
{
	// See https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
	return v != 0 && (v & (v - 1)) == 0;
}

sfz_constexpr_func u32 sfzNextPow2_u32(u32 v)
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

sfz_constexpr_func u64 sfzNextPow2_u64(u64 v)
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

sfz_constexpr_func u32 sfzLog2OfPow2_u32(u32 pow2_val)
{
	// https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
	const u32 multiplyDeBruijnBitPosition2[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};
	return multiplyDeBruijnBitPosition2[(u32)(pow2_val * 0x077CB531U) >> 27];
}

sfz_constexpr_func u64 sfzLog2OfPow2_u64(u64 pow2_val)
{
	if (pow2_val == (u64(1) << u64(32))) return 32;
	const u32 high_bits = u32(pow2_val >> u64(32));
	const u32 high_bits_log2 = sfzLog2OfPow2_u32(high_bits);
	if (high_bits_log2 != 0) return high_bits_log2 + 32;
	const u32 low_bits = u32(pow2_val);
	const u32 low_bits_log2 = sfzLog2OfPow2_u32(low_bits);
	return low_bits_log2;
}

// Morton Encoding
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func u32 sfzSplitBy3_u32(u32 v)
{
	v &= 0x3FF; // Mask out the 10 lowest bits
	v = (v | (v << u32(16))) & u32(0xFF0000FFu);
	v = (v | (v << u32(8))) & u32(0x0F00F00Fu);
	v = (v | (v << u32(4))) & u32(0xC30C30C3u);
	v = (v | (v << u32(2))) & u32(0x49249249u);
	return v;
}

sfz_constexpr_func u32 sfzMortonEncode(i32x3 v)
{
	// https://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
	const u32 idx = 
		sfzSplitBy3_u32(u32(v.x)) |
		(sfzSplitBy3_u32(u32(v.y)) << u32(1)) | 
		(sfzSplitBy3_u32(u32(v.z)) << u32(2));
	return idx;
}

// Vector math
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace sfz {

constexpr f32 elemSum(f32x2 v) { return v.x + v.y; }
constexpr f32 elemSum(f32x3 v) { return v.x + v.y + v.z; }
constexpr f32 elemSum(f32x4 v) { return v.x + v.y + v.z + v.w; }

constexpr i32 elemSum(i32x2 v) { return v.x + v.y; }
constexpr i32 elemSum(i32x3 v) { return v.x + v.y + v.z; }
constexpr i32 elemSum(i32x4 v) { return v.x + v.y + v.z + v.w; }

constexpr f32 EQF_EPS = 0.001f;
constexpr bool eqf(f32 l, f32 r, f32 eps = EQF_EPS) { return (l <= (r + eps)) && (l >= (r - eps)); }
constexpr bool eqf(f32x2 l, f32x2 r, f32 eps = EQF_EPS) { return eqf(l.x, r.x, eps) && eqf(l.y, r.y, eps); }
constexpr bool eqf(f32x3 l, f32x3 r, f32 eps = EQF_EPS) { return eqf(l.x, r.x, eps) && eqf(l.y, r.y, eps) && eqf(l.z, r.z, eps); }
constexpr bool eqf(f32x4 l, f32x4 r, f32 eps = EQF_EPS) { return eqf(l.x, r.x, eps) && eqf(l.y, r.y, eps) && eqf(l.z, r.z, eps) && eqf(l.w, r.w, eps); }

constexpr f32 elemMax(f32x2 v) { return f32_max(v.x, v.y); }
constexpr f32 elemMax(f32x3 v) { return f32_max(f32_max(v.x, v.y), v.z); }
constexpr f32 elemMax(f32x4 v) { return f32_max(f32_max(f32_max(v.x, v.y), v.z), v.w); }
constexpr i32 elemMax(i32x2 v) { return i32_max(v.x, v.y); }
constexpr i32 elemMax(i32x3 v) { return i32_max(i32_max(v.x, v.y), v.z); }
constexpr i32 elemMax(i32x4 v) { return i32_max(i32_max(i32_max(v.x, v.y), v.z), v.w); }

constexpr f32 elemMin(f32x2 v) { return f32_min(v.x, v.y); }
constexpr f32 elemMin(f32x3 v) { return f32_min(f32_min(v.x, v.y), v.z); }
constexpr f32 elemMin(f32x4 v) { return f32_min(f32_min(f32_min(v.x, v.y), v.z), v.w); }
constexpr i32 elemMin(i32x2 v) { return i32_min(v.x, v.y); }
constexpr i32 elemMin(i32x3 v) { return i32_min(i32_min(v.x, v.y), v.z); }
constexpr i32 elemMin(i32x4 v) { return i32_min(i32_min(i32_min(v.x, v.y), v.z), v.w); }

constexpr f32 sgn(f32 v) { return v < 0.0f ? -1.0f : 1.0f; }
constexpr i32 sgn(i32 v) { return v < 0 ? -1 : 1; }
constexpr f32x2 sgn(f32x2 v) { return f32x2_init(sgn(v.x), sgn(v.y)); }
constexpr f32x3 sgn(f32x3 v) { return f32x3_init(sgn(v.x), sgn(v.y), sgn(v.z)); }
constexpr f32x4 sgn(f32x4 v) { return f32x4_init(sgn(v.x), sgn(v.y), sgn(v.z), sgn(v.w)); }
constexpr i32x2 sgn(i32x2 v) { return i32x2_init(sgn(v.x), sgn(v.y)); }
constexpr i32x3 sgn(i32x3 v) { return i32x3_init(sgn(v.x), sgn(v.y), sgn(v.z)); }
constexpr i32x4 sgn(i32x4 v) { return i32x4_init(sgn(v.x), sgn(v.y), sgn(v.z), sgn(v.w)); }

constexpr f32 lerp(f32 v0, f32 v1, f32 t) { return (1.0f - t) * v0 + t * v1; }
constexpr f32x2 lerp(f32x2 v0, f32x2 v1, f32 t) { return (1.0f - t) * v0 + t * v1; }
constexpr f32x3 lerp(f32x3 v0, f32x3 v1, f32 t) { return (1.0f - t) * v0 + t * v1; }
constexpr f32x4 lerp(f32x4 v0, f32x4 v1, f32 t) { return (1.0f - t) * v0 + t * v1; }

inline f32 floor(f32 v) { return floorf(v); }
inline f32x2 floor(f32x2 v) { return f32x2_init(floor(v.x), floor(v.y)); }
inline f32x3 floor(f32x3 v) { return f32x3_init(floor(v.x), floor(v.y), floor(v.z)); }
inline f32x4 floor(f32x4 v) { return f32x4_init(floor(v.x), floor(v.y), floor(v.z), floor(v.w)); }

inline f32 ceil(f32 v) { return ceilf(v); }
inline f32x2 ceil(f32x2 v) { return f32x2_init(ceil(v.x), ceil(v.y)); }
inline f32x3 ceil(f32x3 v) { return f32x3_init(ceil(v.x), ceil(v.y), ceil(v.z)); }
inline f32x4 ceil(f32x4 v) { return f32x4_init(ceil(v.x), ceil(v.y), ceil(v.z), ceil(v.w)); }

inline f32 round(f32 v) { return roundf(v); }
inline f32x2 round(f32x2 v) { return f32x2_init(round(v.x), round(v.y)); }
inline f32x3 round(f32x3 v) { return f32x3_init(round(v.x), round(v.y), round(v.z)); }
inline f32x4 round(f32x4 v) { return f32x4_init(round(v.x), round(v.y), round(v.z), round(v.w)); }

} // namespace sfz

#endif // __cplusplus

#endif // SFZ_MATH_H
