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

#include "sfz.h"
#include "sfz_geom.h"

namespace sfz {

// AABB
// ------------------------------------------------------------------------------------------------

struct AABB final {
	f32x3 min, max;

	static constexpr AABB fromPosDims(f32x3 pos, f32x3 dims) { f32x3 half_dims = dims * 0.5f; return { pos - half_dims, pos + half_dims }; }
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

// Returns distance to closest intersection with AABB, negative number on no hit
inline f32 rayVsAABB(const SfzRay& ray, const AABB& aabb, f32* t_min_out = nullptr, f32* t_max_out = nullptr)
{
	const f32x3 origin = ray.origin;
	const f32x3 inv_dir = sfzInvertRayDir(ray.dir);
	f32 t_min, t_max;
	sfzRayVsAABB(origin, inv_dir, aabb.min, aabb.max, &t_min, &t_max);
	if (t_min_out != nullptr) *t_min_out = t_min;
	if (t_max_out != nullptr) *t_max_out = t_max;
	const bool hit = t_min <= t_max && 0.0f <= t_max && t_min <= ray.max_dist;
	return hit ? f32_max(0.0f, t_min) : -1.0f;
}

} // namespace sfz

#endif
