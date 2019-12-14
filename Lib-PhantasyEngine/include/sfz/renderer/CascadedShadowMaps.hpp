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

#pragma once

#include <skipifzero.hpp>

#include <sfz/math/Matrix.hpp>

namespace sfz {

using sfz::mat4;
using sfz::vec3;

// Cascaded shadow map calculator
// ------------------------------------------------------------------------------------------------

constexpr uint32_t MAX_NUM_CASCADED_SHADOW_MAP_LEVELS = 4;

struct CascadedShadowMapInfo final {

	// Number of cascaded shadow map levels (same as input to function)
	uint32_t numLevels = 0;

	// Maximum distance each shadow map level is valid for (same as input to function)
	float levelDists[MAX_NUM_CASCADED_SHADOW_MAP_LEVELS] = {};

	// View matrices for the levels shadow map camera
	mat4 viewMatrices[MAX_NUM_CASCADED_SHADOW_MAP_LEVELS] = {};

	// Projection matrix for the levels shadow map camera
	mat4 projMatrices[MAX_NUM_CASCADED_SHADOW_MAP_LEVELS] = {};

	// The "light" matrix for the level, i.e. transforms from camera's view space to light's clip
	// space scaled and translated by 0.5.
	//
	// E.g., to get a coordinate to sample in shadow map with in HLSL you should do:
	// float4 tmp = mul(lightMatrix, float4(viewSpacePos, 1.0));
	// tmp.xyz /= tmp.w;
	// tmp.y = 1.0 - tmp.y;
	// float lightDepth = shadowMap.Sample(sampler, tmp.xy).r;
	// // compare lightDepth and tmp.z here
	mat4 lightMatrices[MAX_NUM_CASCADED_SHADOW_MAP_LEVELS] = {};
};

// Calculates information necessary to render cascaded shadow maps for directional lighting.
//
// Assumes you are using reverse-z for shadow maps
//
// camRealViewMatrix: The view matrix to be used when calculating light matrices. Note that this
//                    is ONLY used for this purpose, i.e. the view matrix does not have to be the
//                    calculated from the camera properties provided. This is useful for debugging
//                    the shadows.
// lightDir: The direction of the light. NOT the direction towards the light.
// shadowHeightDist: The "height" of the shadow map. I.e. how much geometry should be covered from
//                   the view volume to towards the light.
// levelDists: A list with numLevels distances from the camera. Each distance indicates how much
//             area should be covered by each level of the cascaded shadow map. Note that levels
//             will never overlap, so the area for the first level is levelDists[0] - camNear, for
//             second level levelDists[1] - levelDists[0], etc.
CascadedShadowMapInfo calculateCascadedShadowMapInfo(
	vec3 camPos,
	vec3 camDir,
	vec3 camUp,
	float camVertFovDegs,
	float camAspect,
	float camNear,
	mat4 camRealViewMatrix,
	vec3 lightDir,
	float shadowHeightDist,
	uint32_t numLevels,
	const float* levelDists) noexcept;

} // namespace sfz
