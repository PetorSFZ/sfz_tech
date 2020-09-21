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

namespace sfz {

// rotateTowards()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec3 rotateTowardsRad(vec3 inDir, vec3 targetDir, float angleRads) noexcept
{
	sfz_assert(eqf(length(inDir), 1.0f));
	sfz_assert(eqf(length(targetDir), 1.0f));
	sfz_assert(dot(inDir, targetDir) >= -0.99f);
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < PI);
	vec3 axis = cross(inDir, targetDir);
	sfz_assert(!eqf(axis, vec3(0.0f)));
	quat rotQuat = quat::rotationRad(axis, angleRads);
	vec3 newDir = rotate(rotQuat, inDir);
	return newDir;
}

SFZ_CUDA_CALL vec3 rotateTowardsRadClampSafe(vec3 inDir, vec3 targetDir, float angleRads) noexcept
{
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < PI);

	vec3 inDirNorm = normalizeSafe(inDir);
	vec3 targetDirNorm = normalizeSafe(targetDir);
	sfz_assert(!eqf(inDirNorm, vec3(0.0f)));
	sfz_assert(!eqf(targetDirNorm, vec3(0.0f)));

	// Case where vectors are the same, just return the target dir
	if (eqf(inDirNorm, targetDirNorm)) return targetDirNorm;

	// Case where vectors are exact opposite, slightly nudge input a bit
	if (eqf(inDirNorm, -targetDirNorm)) {
		inDirNorm = normalize(inDir + (vec3(1.0f) - inDirNorm) * 0.025f);
		sfz_assert(!eqf(inDirNorm, -targetDirNorm));
	}

	// Case where angle is larger than the angle between the vectors
	if (angleRads >= acos(dot(inDirNorm, targetDirNorm))) return targetDirNorm;

	// At this point all annoying cases should be handled, just run the normal routine
	return rotateTowardsRad(inDirNorm, targetDirNorm, angleRads);
}

SFZ_CUDA_CALL vec3 rotateTowardsDeg(vec3 inDir, vec3 targetDir, float angleDegs) noexcept
{
	return rotateTowardsRad(inDir, targetDir, DEG_TO_RAD * angleDegs);
}

SFZ_CUDA_CALL vec3 rotateTowardsDegClampSafe(vec3 inDir, vec3 targetDir, float angleDegs) noexcept
{
	return rotateTowardsRadClampSafe(inDir, targetDir, DEG_TO_RAD * angleDegs);
}

} // namespace sfz
