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

#ifndef SKIPIFZERO_GEOMETRY_HPP
#define SKIPIFZERO_GEOMETRY_HPP
#pragma once

#include <cfloat>

#include "skipifzero.hpp"

namespace sfz {

// Ray
// ------------------------------------------------------------------------------------------------

struct Ray final {
	vec3 origin = vec3(0.0f);
	float minDist = 0.0f;
	vec3 dir = vec3(0.0f);
	float maxDist = FLT_MAX;

	Ray() = default;
	Ray(vec3 origin, vec3 dir, float minDist = 0.0f, float maxDist = FLT_MAX)
		: origin(origin), dir(dir), minDist(minDist), maxDist(maxDist) { sfz_assert(eqf(length(dir), 1.0f)); }
};
static_assert(sizeof(Ray) == sizeof(float) * 8, "Ray is padded");

// AABB
// ------------------------------------------------------------------------------------------------

struct AABB final {
	vec3 min, max;

	static constexpr AABB fromPosDims(vec3 pos, vec3 dims) { vec3 halfDims = dims * 0.5f; return { pos - halfDims, pos + halfDims }; }
	static constexpr AABB fromCorners(vec3 min, vec3 max) { return { min, max }; }

	constexpr vec3 pos() const { return (this->min + this->max) * 0.5f; }
	constexpr vec3 dims() const { return this->max - this->min; }
	constexpr float halfDimX() const { return (this->max.x - this->min.x) * 0.5f; }
	constexpr float halfDimY() const { return (this->max.y - this->min.y) * 0.5f; }
	constexpr float halfDimZ() const { return (this->max.z - this->min.z) * 0.5f; }
};
static_assert(sizeof(AABB) == sizeof(float) * 6, "AABB is padded");

// Ray VS AABB intersection test
// ------------------------------------------------------------------------------------------------

// Branchless ray vs AABB intersection test
//
// 2 versions, a "low-level" version that is a good buildig block for algorithms that do a lot of
// tests and a high-level version with some convenience features.

constexpr void rayVsAABB(vec3 origin, vec3 invDir, const AABB& aabb, float& tMinOut, float& tMaxOut)
{
	const vec3 t0 = (aabb.min - origin) * invDir;
	const vec3 t1 = (aabb.max - origin) * invDir;
	tMinOut = sfz::elemMax(sfz::min(t0, t1));
	tMaxOut = sfz::elemMin(sfz::max(t0, t1));
}

constexpr bool rayVsAABB(const Ray& ray, const AABB& aabb, float& tMinOut, float& tMaxOut)
{
	const vec3 origin = ray.origin + ray.dir * ray.minDist;
	const vec3 invDir = 1.0f / ray.dir;
	rayVsAABB(origin, invDir, aabb, tMinOut, tMaxOut);
	return tMaxOut > 0.0f && tMinOut < tMaxOut;
}

} // namespace sfz

#endif
