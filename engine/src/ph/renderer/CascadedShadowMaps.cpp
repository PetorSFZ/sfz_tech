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

#include "ph/renderer/CascadedShadowMaps.hpp"

#include <ZeroG-cpp.hpp>

#include <sfz/math/MathSupport.hpp>

namespace ph {

// Cascaded shadow map calculator
// ------------------------------------------------------------------------------------------------

CascadedShadowMapInfo calculateCascadedShadowMapInfo(
	vec3 camPos,
	vec3 camDir,
	vec3 camUp,
	float camVertFovDegs,
	float camAspect,
	float camNear,
	vec3 lightDir,
	float shadowHeightDist,
	uint32_t numLevels,
	const float* levelDists) noexcept
{
	sfz_assert_debug(!sfz::approxEqual(camDir, vec3(0.0f)));
	sfz_assert_debug(!sfz::approxEqual(camUp, vec3(0.0f)));
	sfz_assert_debug(0.0f < camVertFovDegs);
	sfz_assert_debug(camVertFovDegs < 180.0f);
	sfz_assert_debug(0.0f < camAspect);
	sfz_assert_debug(0.0f < camNear);
	sfz_assert_debug(!sfz::approxEqual(lightDir, vec3(0.0f)));
	sfz_assert_debug(0.0f < shadowHeightDist);
	sfz_assert_debug(0 < numLevels);
	sfz_assert_debug(numLevels <= MAX_NUM_CASCADED_SHADOW_MAP_LEVELS);
	sfz_assert_debug(camNear < levelDists[0]);
	for (uint32_t i = 1; i < numLevels; i++) {
		sfz_assert_debug(levelDists[i - 1] < levelDists[i]);
	}

	// Recreate view matrix and inverse view matrix
	mat4 viewMatrix;
	zg::createViewMatrix(viewMatrix.data(), camPos.data(), camDir.data(), camUp.data());
	mat4 invViewMatrix = sfz::inverse(viewMatrix);

	// Calculate largest aspect ratio so we can pretend view frustrum has same width and height
	float largestFovRads = 0.0f;
	if (camAspect <= 1.0f) largestFovRads = camVertFovDegs * sfz::DEG_TO_RAD;
	else largestFovRads = camVertFovDegs * camAspect * sfz::DEG_TO_RAD;

	// Create return struct and fill with initial info
	CascadedShadowMapInfo info;
	info.numLevels = numLevels;

	for (uint32_t i = 0; i < numLevels; i++) {

		// Find mid point (of view frustrum) in the area covered by this cascaded level
		float prevDist = i == 0 ? camNear : levelDists[i - 1];
		float distToMid = prevDist + (levelDists[i] - prevDist) * 0.5f;
		vec3 midPoint = camPos + camDir * distToMid;

		// Find size of view frustrum at maximum distance for this level
		float largestHeight = 2.0f * levelDists[i] * std::tan(largestFovRads * 0.5f);

		// The light can be oriented many different ways compared to view frustrum, we can calculate
		// the worst case by using pythagoras and assuming the diagonal through the volume
		float worstCaseDim = std::sqrtf(largestHeight * largestHeight + largestHeight * largestHeight);
		worstCaseDim = std::sqrt(worstCaseDim * worstCaseDim + largestHeight * largestHeight);

		// Calculate lights camera position
		vec3 lightCamPos = midPoint + -lightDir * shadowHeightDist;
		vec3 lightCamUp = camUp;
		if (sfz::approxEqual(sfz::abs(sfz::dot(sfz::normalize(camUp), sfz::normalize(lightDir))), 1.0f, 0.01f)) {
			lightCamUp = sfz::normalize(camUp + camDir);
		}

		// Create matrices for level
		info.levelDists[i] = levelDists[i];
		zg::createViewMatrix(
			info.viewMatrices[i].data(),
			lightCamPos.data(),
			lightDir.data(),
			lightCamUp.data());
		zg::createOrthographicProjectionReverse(
			info.projMatrices[i].data(),
			worstCaseDim,
			worstCaseDim,
			1.0f,
			shadowHeightDist + worstCaseDim * 0.5f);
		info.camViewToLightClip[i] = info.projMatrices[i] * info.viewMatrices[i] * invViewMatrix;
	}

	return info;
}

} // namespace ph
