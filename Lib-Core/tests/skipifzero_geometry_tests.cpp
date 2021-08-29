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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "utest.h"
#undef near
#undef far

#include "skipifzero.hpp"
#include "skipifzero_geometry.hpp"

using sfz::AABB;
using sfz::Ray;
using sfz::eqf;

// AABB tests
// ------------------------------------------------------------------------------------------------

UTEST(AABB, ray_vs_aabb)
{
	{
		AABB aabb = AABB::fromPosDims(f32x3(0.0f), f32x3(1.0f));

		{
			Ray ray = Ray(f32x3(0.0f), f32x3(1.0f, 0.0f, 0.0f));
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, 0.0f));
			ASSERT_TRUE(eqf(tMin, -0.5f));
			ASSERT_TRUE(eqf(tMax, 0.5f));
		}

		{
			Ray ray = Ray(f32x3(0.0f, 2.0f, 0.0f), f32x3(0.0f, -1.0f, 0.0f));
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, 1.5f));
			ASSERT_TRUE(eqf(tMin, 1.5f));
			ASSERT_TRUE(eqf(tMax, 2.5f));
		}

		{
			Ray ray = Ray(f32x3(0.0f, 2.0f, 0.0f), f32x3(0.0f, 1.0f, 0.0f));
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, -1.0f));
			ASSERT_TRUE(eqf(tMax, -1.5f));
			ASSERT_TRUE(eqf(tMin, -2.5f));
		}

		{
			Ray ray = Ray(f32x3(-1.0f, 0.0f, 0.0f), f32x3(1.0f, 0.0f, 0.0f), 0.49999f);
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, -1.0f));
			ASSERT_TRUE(eqf(tMin, 0.5f));
			ASSERT_TRUE(eqf(tMax, 1.5f));
		}

		{
			Ray ray = Ray(f32x3(-1.0f, 0.0f, 0.0f), f32x3(1.0f, 0.0f, 0.0f), 1.0f);
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, 0.5f));
			ASSERT_TRUE(eqf(tMin, 0.5f));
			ASSERT_TRUE(eqf(tMax, 1.5f));
		}

		{
			Ray ray = Ray(aabb.max, f32x3(0.0f, 0.0f, -1.0f));
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, 0.0f));
			ASSERT_TRUE(eqf(tMin, 0.0f));
			ASSERT_TRUE(eqf(tMax, 0.0f));
		}
	}

	{
		AABB aabb = AABB::fromPosDims(f32x3(1.0f), f32x3(2.0f));

		{
			Ray ray = Ray(f32x3(0.0f), sfz::normalize(f32x3(1.0f)));
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, 0.0f));
			ASSERT_TRUE(eqf(tMin, 0.0f));
			ASSERT_TRUE(eqf(tMax, 3.46410161f));
		}

		{
			Ray ray = Ray(f32x3(2.0f), sfz::normalize(f32x3(-1.0f)));
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, 0.0f));
			ASSERT_TRUE(eqf(tMin, 0.0f));
			ASSERT_TRUE(eqf(tMax, 3.46410161f));
		}

		{
			Ray ray = Ray(f32x3(2.0f, 2.0f, 4.0f - 0.00001f), sfz::normalize(f32x3(-1.0f)));
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, 3.46410161f));
			ASSERT_TRUE(eqf(tMin, 3.46410161f, 0.01f));
			ASSERT_TRUE(eqf(tMax, 3.46410161f, 0.01f));
		}

		{
			Ray ray = Ray(f32x3(2.0f, 2.0f, 4.0f + 0.00001f), sfz::normalize(f32x3(-1.0f)));
			f32 tMin = sfz::RAY_MAX_DIST;
			f32 tMax = -sfz::RAY_MAX_DIST;
			const f32 t = sfz::rayVsAABB(ray, aabb, &tMin, &tMax);
			ASSERT_TRUE(eqf(t, -1.0f));
			ASSERT_TRUE(eqf(tMin, 3.46410161f, 0.01f));
			ASSERT_TRUE(eqf(tMax, 3.46410161f, 0.01f));
		}
	}
}
