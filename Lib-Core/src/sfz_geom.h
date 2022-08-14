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
	f32 maxDist;

#ifdef __cplusplus
	static constexpr SfzRay create(f32x3 origin, f32x3 dir, f32 maxDist = SFZ_RAY_MAX_DIST)
	{
		SfzRay ray = {};
		ray.origin = origin;
		ray.dir = dir;
		ray.maxDist = maxDist;
		return ray;
	}

	static constexpr SfzRay createOffset(f32x3 origin, f32x3 dir, f32 minDist, f32 maxDist = SFZ_RAY_MAX_DIST)
	{
		return SfzRay::create(origin + dir * minDist, dir, maxDist);
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

sfz_extern_c sfz_constexpr_func void sfzRayVsAABB(
	f32x3 origin, f32x3 invDir, f32x3 aabbMin, f32x3 aabbMax, f32* tMinOut, f32* tMaxOut)
{
	const f32x3 t0 = (aabbMin - origin) * invDir;
	const f32x3 t1 = (aabbMax - origin) * invDir;
	const f32x3 tmpTMin = f32x3_min(t0, t1);
	const f32x3 tmpTMax = f32x3_max(t0, t1);
	*tMinOut = f32_max(f32_max(tmpTMin.x, tmpTMin.y), tmpTMin.z);
	*tMaxOut = f32_min(f32_min(tmpTMax.x, tmpTMax.y), tmpTMax.z);
}

#endif // SFZ_GEOM_H
