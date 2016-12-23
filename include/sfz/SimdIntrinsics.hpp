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

#include <immintrin.h> // Intel AVX intrinsics

namespace sfz {

// Matrix struct declaration
// ------------------------------------------------------------------------------------------------

/// Calculates the parameter mask for _mm_shuffle_ps()
/// Each parameter should be a number in the interval [0, 3], which specifies which slot to copy
/// parameters from.
#define SFZ_SHUFFLE_PS_PARAM(e0, e1, e2, e3) \
	(uint32_t(e0) | (uint32_t(e1) << 2u) | (uint32_t(e2) << 4u) | (uint32_t(e3) << 6u))

/// Replicates the specified element in all slots in the resulting vector
template<uint32_t element>
inline __m128 replicatePs(__m128 v) noexcept
{
	return _mm_shuffle_ps(v, v, SFZ_SHUFFLE_PS_PARAM(element, element, element, element));
}

} // namespace sfz
