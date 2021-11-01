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

	static SfzRay createFromPoints(f32x3 start, f32x3 end)
	{
		const f32x3 diff = end - start;
		const f32 len = sfz::length(diff);
		sfz_assert(len > 0.0001f);
		const f32x3 dir = diff / len;
		return SfzRay::create(start, dir, len);
	}
#endif
};

#endif // SFZ_GEOM_H
