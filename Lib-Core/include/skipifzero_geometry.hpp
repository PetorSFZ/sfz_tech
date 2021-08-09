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

#include "skipifzero.hpp"

namespace sfz {

// Ray
// ------------------------------------------------------------------------------------------------

constexpr f32 RAY_MAX_DIST = 1'000'000.0f; // F32_MAX causes issues in some algorithms

struct Ray final {
	f32x3 origin = f32x3(0.0f);
	f32x3 dir = f32x3(0.0f);
	f32 maxDist = RAY_MAX_DIST;

	Ray() = default;
	Ray(f32x3 origin, f32x3 dir, f32 maxDist = RAY_MAX_DIST)
		: origin(origin), dir(dir), maxDist(maxDist)
	{
		sfz_assert(eqf(length(dir), 1.0f));
		sfz_assert(0.0f < maxDist && maxDist <= RAY_MAX_DIST);
	}

	static Ray createOffset(f32x3 origin, f32x3 dir, f32 minDist, f32 maxDist = RAY_MAX_DIST)
	{
		return Ray(origin + dir * minDist, dir, maxDist);
	}
};
static_assert(sizeof(Ray) == sizeof(f32) * 7, "Ray is padded");

// AABB
// ------------------------------------------------------------------------------------------------

struct AABB final {
	f32x3 min, max;

	static constexpr AABB fromPosDims(f32x3 pos, f32x3 dims) { f32x3 halfDims = dims * 0.5f; return { pos - halfDims, pos + halfDims }; }
	static constexpr AABB fromCorners(f32x3 min, f32x3 max) { return { min, max }; }

	constexpr f32x3 pos() const { return (this->min + this->max) * 0.5f; }
	constexpr f32x3 dims() const { return this->max - this->min; }
	constexpr f32 halfDimX() const { return (this->max.x - this->min.x) * 0.5f; }
	constexpr f32 halfDimY() const { return (this->max.y - this->min.y) * 0.5f; }
	constexpr f32 halfDimZ() const { return (this->max.z - this->min.z) * 0.5f; }
};
static_assert(sizeof(AABB) == sizeof(f32) * 6, "AABB is padded");

// Ray VS AABB intersection test
// ------------------------------------------------------------------------------------------------

// Branchless ray vs AABB intersection test
//
// 2 versions, a "low-level" version that is a good buildig block for algorithms that do a lot of
// tests and a high-level version with some convenience features.

constexpr void rayVsAABB(f32x3 origin, f32x3 invDir, const AABB& aabb, f32& tMinOut, f32& tMaxOut)
{
	const f32x3 t0 = (aabb.min - origin) * invDir;
	const f32x3 t1 = (aabb.max - origin) * invDir;
	tMinOut = sfz::elemMax(sfz::min(t0, t1));
	tMaxOut = sfz::elemMin(sfz::max(t0, t1));
}

// Returns distance to closest intersection with AABB, negative number on no hit
inline f32 rayVsAABB(const Ray& ray, const AABB& aabb, f32* tMinOut = nullptr, f32* tMaxOut = nullptr)
{
	const f32x3 origin = ray.origin;
	const f32x3 invDir = divSafe(f32x3(1.0f), ray.dir);
	f32 tMin, tMax;
	rayVsAABB(origin, invDir, aabb, tMin, tMax);
	if (tMinOut != nullptr) *tMinOut = tMin;
	if (tMaxOut != nullptr) *tMaxOut = tMax;
	const bool hit = tMin <= tMax && 0.0f <= tMax && tMin <= ray.maxDist;
	return hit ? sfz::max(0.0f, tMin) : -1.0f;
}

} // namespace sfz

#endif
