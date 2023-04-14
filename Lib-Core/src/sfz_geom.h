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

#ifndef SFZ_GEOM_H
#define SFZ_GEOM_H
#pragma once

#include "sfz.h"

// Ray
// ------------------------------------------------------------------------------------------------

sfz_constant f32 SFZ_RAY_MAX_DIST = 1000000.0f; // F32_MAX causes issues in some algorithms

sfz_struct(SfzRay) {
	f32x3 origin;
	f32x3 dir;
	f32 max_dist;
};

sfz_constexpr_func SfzRay sfzRayCreate(f32x3 origin, f32x3 dir, f32 max_dist)
{
	SfzRay ray = {};
	ray.origin = origin;
	ray.dir = dir;
	ray.max_dist = max_dist;
	return ray;
}

sfz_constexpr_func SfzRay sfzRayCreateOffset(f32x3 origin, f32x3 dir, f32 min_dist, f32 max_dist)
{
	SfzRay ray = {};
	ray.origin = origin;
	ray.origin.x += dir.x * min_dist;
	ray.origin.y += dir.y * min_dist;
	ray.origin.z += dir.z * min_dist;
	ray.dir = dir;
	ray.max_dist = max_dist - min_dist;
	return ray;
}

sfz_constexpr_func f32x3 sfzRayInvertDir(f32x3 dir)
{
	sfz_constant f32 INV_MIN_EPS = 0.000'000'1f;
	sfz_constant f32 ZERO_DIV_RES = 1.0f / INV_MIN_EPS;
	const f32x3 invDir = f32x3_init(
		f32_abs(dir.x) < INV_MIN_EPS ? ZERO_DIV_RES : 1.0f / dir.x,
		f32_abs(dir.y) < INV_MIN_EPS ? ZERO_DIV_RES : 1.0f / dir.y,
		f32_abs(dir.z) < INV_MIN_EPS ? ZERO_DIV_RES : 1.0f / dir.z
	);
	return invDir;
}

// AABB
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func void sfzRayVsAABB(
	f32x3 origin, f32x3 inv_dir, f32x3 aabb_min, f32x3 aabb_max, f32* t_min_out, f32* t_max_out)
{
	const f32x3 t0 = (aabb_min - origin) * inv_dir;
	const f32x3 t1 = (aabb_max - origin) * inv_dir;
	const f32x3 tmp_t_min = f32x3_min(t0, t1);
	const f32x3 tmp_t_max = f32x3_max(t0, t1);
	*t_min_out = f32_max(f32_max(tmp_t_min.x, tmp_t_min.y), tmp_t_min.z);
	*t_max_out = f32_min(f32_min(tmp_t_max.x, tmp_t_max.y), tmp_t_max.z);
}

sfz_constexpr_func f32x3 sfzClosestPointOnAABB(f32x3 aabb_min, f32x3 aabb_max, f32x3 p)
{
	return f32x3_min(f32x3_max(aabb_min, p), aabb_max);
}

inline f32 sfzClosestDistToAABB(f32x3 aabb_min, f32x3 aabb_max, f32x3 p)
{
	const f32x3 closest_point = f32x3_min(f32x3_max(aabb_min, p), aabb_max);
	const f32x3 d = p - closest_point;
	const f32 dist = f32x3_length(d);
	return dist;
}

// Sphere
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func bool sfzSphereVsSphere(f32x3 p1, f32 r1, f32x3 p2, f32 r2)
{
	const f32x3 d = p1 - p2;
	const f32 dist_squared = f32x3_dot(d, d);
	const f32 radius_sum = r1 + r2;
	const f32 radius_sum_squared = radius_sum * radius_sum;
	return dist_squared <= radius_sum_squared;
}

sfz_constexpr_func bool sfzPointInSphere(f32x3 sphere_pos, f32 sphere_radius, f32x3 p)
{
	const f32x3 d = sphere_pos - p;
	const f32 dist_squared = f32x3_dot(d, d);
	const f32 radius_squared = sphere_radius * sphere_radius;
	return dist_squared <= radius_squared;
}

sfz_constexpr_func bool sfzSphereVsAABB(
	f32x3 sphere_pos, f32 sphere_radius, f32x3 aabb_min, f32x3 aabb_max)
{
	const f32x3 closest_point_on_aabb = f32x3_min(f32x3_max(aabb_min, sphere_pos), aabb_max);
	const f32x3 d = sphere_pos - closest_point_on_aabb;
	const f32 dist_squared = f32x3_dot(d, d);
	const f32 radius_squared = sphere_radius * sphere_radius;
	return dist_squared <= radius_squared;
}

#endif // SFZ_GEOM_H
