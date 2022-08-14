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

#include "sfz.h"
#include "sfz_quat.h"
#include "sfz_math.h"
#include "sfz_matrix.h"

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

inline f32x3 rotateTowardsDeg(f32x3 inDir, f32x3 targetDir, f32 angleDegs)
{
	sfz_assert(sfz::eqf(f32x3_length(inDir), 1.0f));
	sfz_assert(sfz::eqf(f32x3_length(targetDir), 1.0f));
	sfz_assert(f32x3_dot(inDir, targetDir) >= -0.9999f);
	const f32 angleRads = angleDegs * SFZ_DEG_TO_RAD;
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < SFZ_PI);
	f32x3 axis = f32x3_cross(inDir, targetDir);
	sfz_assert(!sfz::eqf(axis, f32x3_splat(0.0f)));
	SfzQuat rotQuat = sfzQuatRotationRad(axis, angleRads);
	f32x3 newDir = sfzQuatRotateUnit(rotQuat, inDir);
	return newDir;
}

inline f32x3 rotateTowardsDegClampSafe(f32x3 inDir, f32x3 targetDir, f32 angleDegs)
{
	const f32 angleRads = angleDegs * SFZ_DEG_TO_RAD;
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < SFZ_PI);

	f32x3 inDirNorm = f32x3_normalizeSafe(inDir);
	f32x3 targetDirNorm = f32x3_normalizeSafe(targetDir);
	sfz_assert(!sfz::eqf(inDirNorm, f32x3_splat(0.0f)));
	sfz_assert(!sfz::eqf(targetDirNorm, f32x3_splat(0.0f)));

	// Case where vectors are the same, just return the target dir
	if (sfz::eqf(inDirNorm, targetDirNorm)) return targetDirNorm;

	// Case where vectors are exact opposite, slightly nudge input a bit
	if (sfz::eqf(inDirNorm, -targetDirNorm)) {
		inDirNorm = f32x3_normalize(inDir + (f32x3_splat(1.0f) - inDirNorm) * 0.025f);
		sfz_assert(!sfz::eqf(inDirNorm, -targetDirNorm));
	}

	// Case where angle is larger than the angle between the vectors
	if (angleRads >= sfz_acos(f32x3_dot(inDirNorm, targetDirNorm))) return targetDirNorm;

	// At this point all annoying cases should be handled, just run the normal routine
	return rotateTowardsDeg(inDirNorm, targetDirNorm, angleDegs);
}
