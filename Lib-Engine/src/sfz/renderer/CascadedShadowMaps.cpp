// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include "sfz/renderer/CascadedShadowMaps.hpp"

#include <skipifzero_math.hpp>

#include <ZeroG.h>

namespace sfz {

// Cascaded shadow map calculator
// ------------------------------------------------------------------------------------------------

CascadedShadowMapInfo calculateCascadedShadowMapInfo(
	f32x3 camPos,
	f32x3 camDir,
	f32x3 camUp,
	f32 camVertFovDegs,
	f32 camAspect,
	f32 camNear,
	mat4 camRealViewMatrix,
	f32x3 lightDir,
	f32 shadowHeightDist,
	u32 numLevels,
	const f32* levelDists) noexcept
{
	sfz_assert(!sfz::eqf(camDir, f32x3(0.0f)));
	sfz_assert(!sfz::eqf(camUp, f32x3(0.0f)));
	sfz_assert(0.0f < camVertFovDegs);
	sfz_assert(camVertFovDegs < 180.0f);
	sfz_assert(0.0f < camAspect);
	sfz_assert(0.0f < camNear);
	sfz_assert(!sfz::eqf(lightDir, f32x3(0.0f)));
	sfz_assert(0.0f < shadowHeightDist);
	sfz_assert(0 < numLevels);
	sfz_assert(numLevels <= MAX_NUM_CASCADED_SHADOW_MAP_LEVELS);
	sfz_assert(camNear < levelDists[0]);
	for (u32 i = 1; i < numLevels; i++) {
		sfz_assert(levelDists[i - 1] < levelDists[i]);
	}

	// Calculate inverse view matrix
	mat4 invViewMatrix = sfz::inverse(camRealViewMatrix);

	// Calculate largest aspect ratio so we can pretend view frustrum has same width and height
	f32 largestFovRads = 0.0f;
	if (camAspect <= 1.0f) largestFovRads = camVertFovDegs * sfz::DEG_TO_RAD;
	else largestFovRads = camVertFovDegs * camAspect * sfz::DEG_TO_RAD;

	// Create return struct and fill with initial info
	CascadedShadowMapInfo info;
	info.numLevels = numLevels;

	for (u32 i = 0; i < numLevels; i++) {

		// Find mid point (of view frustrum) in the area covered by this cascaded level
		f32 prevDist = i == 0 ? camNear : levelDists[i - 1];
		f32 distToMid = prevDist + (levelDists[i] - prevDist) * 0.5f;
		f32x3 midPoint = camPos + camDir * distToMid;

		// Find size of view frustrum at maximum distance for this level
		f32 largestHeight = 2.0f * levelDists[i] * ::tanf(largestFovRads * 0.5f);

		// The light can be oriented many different ways compared to view frustrum, we can calculate
		// the worst case by using pythagoras and assuming the diagonal through the volume
		f32 worstCaseDim = ::sqrtf(largestHeight * largestHeight + largestHeight * largestHeight);
		worstCaseDim = ::sqrtf(worstCaseDim * worstCaseDim + largestHeight * largestHeight);

		// Calculate lights camera position
		f32x3 lightCamPos = midPoint + -lightDir * shadowHeightDist;
		f32x3 lightCamUp = camUp;
		if (sfz::eqf(sfz::abs(sfz::dot(sfz::normalize(camUp), sfz::normalize(lightDir))), 1.0f, 0.01f)) {
			lightCamUp = sfz::normalize(camUp + camDir);
		}

		// Create matrices for level
		info.levelDists[i] = levelDists[i];
		zgUtilCreateViewMatrix(
			info.viewMatrices[i].data(),
			lightCamPos.data(),
			lightDir.data(),
			lightCamUp.data());
		zgUtilCreateOrthographicProjectionReverse(
			info.projMatrices[i].data(),
			worstCaseDim,
			worstCaseDim,
			1.0f,
			shadowHeightDist + worstCaseDim * 0.5f);
		info.lightMatrices[i] = 
			mat4::translation3(f32x3(0.5f, 0.5f, 0.0f)) *
			mat4::scaling3(0.5f, 0.5f, 1.0f) *
			info.projMatrices[i] *
			info.viewMatrices[i] *
			invViewMatrix;
	}

	return info;
}

} // namespace sfz
