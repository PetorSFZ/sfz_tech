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

#ifdef __cplusplus
	static constexpr SfzRay create(f32x3 origin, f32x3 dir, f32 max_dist = SFZ_RAY_MAX_DIST)
	{
		SfzRay ray = {};
		ray.origin = origin;
		ray.dir = dir;
		ray.max_dist = max_dist;
		return ray;
	}

	static constexpr SfzRay createOffset(f32x3 origin, f32x3 dir, f32 min_dist, f32 max_dist = SFZ_RAY_MAX_DIST)
	{
		return SfzRay::create(origin + dir * min_dist, dir, max_dist);
	}
#endif
};

// Minimal ray vs AABB test
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func f32x3 sfzInvertRayDir(f32x3 dir)
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

#endif // SFZ_GEOM_H
