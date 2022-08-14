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

#include <doctest.h>

#include "sfz.h"
#include "sfz_math.h"
#include "skipifzero_geometry.hpp"

using sfz::AABB;
using sfz::eqf;

// AABB tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("AABB: ray_vs_aabb")
{
	{
		AABB aabb = AABB::fromPosDims(f32x3_splat(0.0f), f32x3_splat(1.0f));

		{
			SfzRay ray = SfzRay::create(f32x3_splat(0.0f), f32x3_init(1.0f, 0.0f, 0.0f));
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, 0.0f));
			CHECK(eqf(tMin, -0.5f));
			CHECK(eqf(tMax, 0.5f));
		}

		{
			SfzRay ray = SfzRay::create(f32x3_init(0.0f, 2.0f, 0.0f), f32x3_init(0.0f, -1.0f, 0.0f));
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, 1.5f));
			CHECK(eqf(tMin, 1.5f));
			CHECK(eqf(tMax, 2.5f));
		}

		{
			SfzRay ray = SfzRay::create(f32x3_init(0.0f, 2.0f, 0.0f), f32x3_init(0.0f, 1.0f, 0.0f));
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, -1.0f));
			CHECK(eqf(tMax, -1.5f));
			CHECK(eqf(tMin, -2.5f));
		}

		{
			SfzRay ray = SfzRay::create(f32x3_init(-1.0f, 0.0f, 0.0f), f32x3_init(1.0f, 0.0f, 0.0f), 0.49999f);
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, -1.0f));
			CHECK(eqf(tMin, 0.5f));
			CHECK(eqf(tMax, 1.5f));
		}

		{
			SfzRay ray = SfzRay::create(f32x3_init(-1.0f, 0.0f, 0.0f), f32x3_init(1.0f, 0.0f, 0.0f), 1.0f);
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, 0.5f));
			CHECK(eqf(tMin, 0.5f));
			CHECK(eqf(tMax, 1.5f));
		}

		{
			SfzRay ray = SfzRay::create(aabb.max, f32x3_init(0.0f, 0.0f, -1.0f));
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, 0.0f));
			CHECK(eqf(tMin, 0.0f));
			CHECK(eqf(tMax, 0.0f));
		}
	}

	{
		AABB aabb = AABB::fromPosDims(f32x3_splat(1.0f), f32x3_splat(2.0f));

		{
			SfzRay ray = SfzRay::create(f32x3_splat(0.0f), f32x3_normalize(f32x3_splat(1.0f)));
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, 0.0f));
			CHECK(eqf(tMin, 0.0f));
			CHECK(eqf(tMax, 3.46410161f));
		}

		{
			SfzRay ray = SfzRay::create(f32x3_splat(2.0f), f32x3_normalize(f32x3_splat(-1.0f)));
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, 0.0f));
			CHECK(eqf(tMin, 0.0f));
			CHECK(eqf(tMax, 3.46410161f));
		}

		{
			SfzRay ray = SfzRay::create(f32x3_init(2.0f, 2.0f, 4.0f - 0.00001f), f32x3_normalize(f32x3_splat(-1.0f)));
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, 3.46410161f));
			CHECK(eqf(tMin, 3.46410161f, 0.01f));
			CHECK(eqf(tMax, 3.46410161f, 0.01f));
		}

		{
			SfzRay ray = SfzRay::create(f32x3_init(2.0f, 2.0f, 4.0f + 0.00001f), f32x3_normalize(f32x3_splat(-1.0f)));
			f32 tMin = SFZ_RAY_MAX_DIST;
			f32 tMax = -SFZ_RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			CHECK(eqf(t, -1.0f));
			CHECK(eqf(tMin, 3.46410161f, 0.01f));
			CHECK(eqf(tMax, 3.46410161f, 0.01f));
		}
	}
}
