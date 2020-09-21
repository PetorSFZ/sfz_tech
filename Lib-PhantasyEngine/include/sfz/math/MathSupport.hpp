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

#include <algorithm>

#include <skipifzero.hpp>
#include <skipifzero_math.hpp>

#include "sfz/CudaCompatibility.hpp"
#include "sfz/SimdIntrinsics.hpp"

// Stop defining min and max, stupid Windows.h.
#undef min
#undef max

namespace sfz {

// rotateTowards()
// ------------------------------------------------------------------------------------------------

// Rotates a vector towards another vector by a given amount of radians. Both the input and the
// target vector must be normalized. In addition, they must not be the same vector or point in
// exact opposite directions.
//
// The variants marked "ClampSafe" handle annoying edge cases. If the angle specified is greater
// than the angle between the two vectors then the target vector will be returned. The input
// vectors are no longer assumed to be normalized. And if they happen to be invalid (i.e. the same
// vector or pointing in exact opposite directions) a sane default will be given.

SFZ_CUDA_CALL vec3 rotateTowardsRad(vec3 inDir, vec3 targetDir, float angleRads) noexcept;
SFZ_CUDA_CALL vec3 rotateTowardsRadClampSafe(vec3 inDir, vec3 targetDir, float angleRads) noexcept;

SFZ_CUDA_CALL vec3 rotateTowardsDeg(vec3 inDir, vec3 targetDir, float angleDegs) noexcept;
SFZ_CUDA_CALL vec3 rotateTowardsDegClampSafe(vec3 inDir, vec3 targetDir, float angleDegs) noexcept;

} // namespace sfz

#include "sfz/math/MathSupport.inl"
