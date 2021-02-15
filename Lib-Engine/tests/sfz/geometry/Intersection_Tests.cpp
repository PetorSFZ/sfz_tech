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

#include <vector>

#include <skipifzero.hpp>
#include <skipifzero_geometry.hpp>
#include <skipifzero_math.hpp>

#include "sfz/geometry/AABB2D.hpp"
#include "sfz/geometry/Circle.hpp"
#include "sfz/geometry/Intersection.hpp"
#include "sfz/geometry/OBB.hpp"
#include "sfz/geometry/Plane.hpp"
#include "sfz/geometry/Sphere.hpp"

UTEST(Intersection, signed_distance_to_plane)
{
	using namespace sfz;

	Plane p{vec3{0.0f, 1.0f, 0.0f}, vec3{2.0f, 1.0f, 0.0f}};

	ASSERT_TRUE(eqf(p.signedDistance(vec3{2.0f, 3.0f, 0.0f}), 2.0f));
	ASSERT_TRUE(eqf(p.signedDistance(vec3{0.0f, 3.0f, 0.0f}), 2.0f));
	ASSERT_TRUE(eqf(p.signedDistance(vec3{2.0f, 0.0f, 0.0f}), -1.0f));
}

UTEST(Intersection, point_inside_aabb_test)
{
	using namespace sfz;

	AABB box{vec3{-1,-1,-1}, vec3{1,1,1}};

	ASSERT_TRUE(pointInside(box, vec3{0,0,0}));
	ASSERT_TRUE(!pointInside(box, vec3{-2,0,0}));
}

UTEST(Intersection, aabb_vs_aabb_test)
{
	using namespace sfz;

	AABB boxMid{vec3{-1.0f, -1.0f, -1.0f}, vec3{1.0f, 1.0f, 1.0f}};
	AABB boxMidSmall{vec3{-0.5f, -0.5f, -0.5f}, vec3{0.5f, 0.5f, 0.5f}};

	AABB boxFrontSmall{vec3{-0.5f, -0.5f, 0.75f}, vec3{0.5f, 0.5f, 1.75f}};
	AABB boxBackSmall{vec3{-0.5f, -0.5f, -1.75f}, vec3{0.5f, 0.5f, -0.75f}};

	AABB boxUpSmall{vec3{-0.5f, 0.75f, -0.5f}, vec3{0.5f, 1.75f, 0.5f}};
	AABB boxDownSmall{vec3{-0.5f, -1.75f, -0.5f}, vec3{0.5f, -0.75f, 0.5f}};

	AABB boxLeftSmall{vec3{-1.75f, -0.5f, -0.5f}, vec3{-0.75f, 0.5f, 0.5f}};
	AABB boxRightSMall{vec3{0.75f, -0.5f, -0.5f}, vec3{1.75f, 0.5f, 0.5f}};

	std::vector<AABB*> smallSurroundingBoxes;
	smallSurroundingBoxes.push_back(&boxFrontSmall);
	smallSurroundingBoxes.push_back(&boxBackSmall);
	smallSurroundingBoxes.push_back(&boxUpSmall);
	smallSurroundingBoxes.push_back(&boxDownSmall);
	smallSurroundingBoxes.push_back(&boxLeftSmall);
	smallSurroundingBoxes.push_back(&boxRightSMall);

	ASSERT_TRUE(intersects(boxMidSmall, boxMid));

	for (AABB* boxPtr : smallSurroundingBoxes) {
		ASSERT_TRUE(intersects(boxMid, *boxPtr));
	}

	for (AABB* boxPtr : smallSurroundingBoxes) {
		ASSERT_TRUE(!intersects(boxMidSmall, *boxPtr));
	}

	for (AABB* boxPtr1 : smallSurroundingBoxes) {
		for (AABB* boxPtr2 : smallSurroundingBoxes) {
			if (boxPtr1 == boxPtr2) { ASSERT_TRUE(intersects(*boxPtr1, *boxPtr2)); }
			else { ASSERT_TRUE(!intersects(*boxPtr1, *boxPtr2)); }
		}
	}
}

UTEST(Intersection, obb_vs_obb_test)
{
	using namespace sfz;

	const vec3 axisAlignedAxes[3] = {
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f)
	};
	vec3 smallExts{1.0f, 1.0f, 1.0f};
	vec3 bigExts{2.0f, 2.0f, 2.0f};

	OBB midSmallAA{vec3{0.0f, 0.0f, 0.0f}, axisAlignedAxes, smallExts};
	OBB midSmallLeftAA{vec3{-1.0f, 0.0f, 0.0f}, axisAlignedAxes, smallExts};
	OBB midSmallRightAA{vec3{1.0f, 0.0f, 0.0f}, axisAlignedAxes, smallExts};
	OBB midSmallDownAA{vec3{0.0f, -1.0f, 0.0f}, axisAlignedAxes, smallExts};
	OBB midSmallUpAA{vec3{0.0f, 1.0f, 0.0f}, axisAlignedAxes, smallExts};
	OBB midSmallBackAA{vec3{0.0f, 0.0f, -1.0f}, axisAlignedAxes, smallExts};
	OBB midSmallFrontAA{vec3{0.0f, 0.0f, 1.0f}, axisAlignedAxes, smallExts};
	std::vector<OBB*> smallSurroundingAABoxes;
	smallSurroundingAABoxes.push_back(&midSmallLeftAA);
	smallSurroundingAABoxes.push_back(&midSmallRightAA);
	smallSurroundingAABoxes.push_back(&midSmallDownAA);
	smallSurroundingAABoxes.push_back(&midSmallUpAA);
	smallSurroundingAABoxes.push_back(&midSmallBackAA);
	smallSurroundingAABoxes.push_back(&midSmallFrontAA);

	OBB midAA{vec3{0.0f, 0.0f, 0.0f}, axisAlignedAxes, bigExts};
	OBB midLeftAA{vec3{-1.0f, 0.0f, 0.0f}, axisAlignedAxes, bigExts};
	OBB midRightAA{vec3{1.0f, 0.0f, 0.0f}, axisAlignedAxes, bigExts};
	OBB midDownAA{vec3{0.0f, -1.0f, 0.0f}, axisAlignedAxes, bigExts};
	OBB midUpAA{vec3{0.0f, 1.0f, 0.0f}, axisAlignedAxes, bigExts};
	OBB midBackAA{vec3{0.0f, 0.0f, -1.0f}, axisAlignedAxes, bigExts};
	OBB midFrontAA{vec3{0.0f, 0.0f, 1.0f}, axisAlignedAxes, bigExts};
	std::vector<OBB*> surroundingAABoxes;
	surroundingAABoxes.push_back(&midLeftAA);
	surroundingAABoxes.push_back(&midRightAA);
	surroundingAABoxes.push_back(&midDownAA);
	surroundingAABoxes.push_back(&midUpAA);
	surroundingAABoxes.push_back(&midBackAA);
	surroundingAABoxes.push_back(&midFrontAA);

	for (OBB* smallPtr : smallSurroundingAABoxes) {
		ASSERT_TRUE(intersects(*smallPtr, midAA));
		ASSERT_TRUE(intersects(midAA, *smallPtr));
	}

	ASSERT_TRUE(!intersects(midSmallLeftAA, midSmallRightAA));
	ASSERT_TRUE(!intersects(midSmallDownAA, midSmallUpAA));
	ASSERT_TRUE(!intersects(midSmallBackAA, midSmallFrontAA));

	ASSERT_TRUE(!intersects(midSmallLeftAA, midRightAA));
	ASSERT_TRUE(!intersects(midSmallDownAA, midUpAA));
	ASSERT_TRUE(!intersects(midSmallBackAA, midFrontAA));

	ASSERT_TRUE(!intersects(midLeftAA, midSmallRightAA));
	ASSERT_TRUE(!intersects(midDownAA, midSmallUpAA));
	ASSERT_TRUE(!intersects(midBackAA, midSmallFrontAA));

	// Non-trivial edge case
	OBB nonTrivial1st = OBB(vec3(0.0f), axisAlignedAxes, vec3(2.0f));
	OBB nonTrivial2nd = OBB(vec3(2.0f), axisAlignedAxes, vec3(2.0f));
	nonTrivial2nd = nonTrivial2nd.transformOBB(quat::fromEuler(45.0f, 45.0f, 45.0f));

	ASSERT_TRUE(!intersects(nonTrivial1st, nonTrivial2nd));
}

UTEST(Intersection, sphere_vs_sphere_test)
{
	using namespace sfz;

	Sphere mid{vec3{0.0f, 0.0f, 0.0f}, 0.5f};
	Sphere midBig{vec3{0.0f, 0.0f, 0.0f}, 1.0f};
	Sphere aBitOff{vec3{-1.1f, 0.0f, 0.0f}, 0.5f};

	ASSERT_TRUE(intersects(mid, midBig));
	ASSERT_TRUE(intersects(midBig, aBitOff));
	ASSERT_TRUE(!intersects(mid, aBitOff));
}

UTEST(Intersection, circle_vs_circle_test)
{
	using namespace sfz;

	Circle mid{vec2{0.0f}, 1.0f};
	Circle midBig{vec2{0.0f}, 2.0f};
	Circle left{vec2{-2.1f, 0.0f}, 1.0f};

	ASSERT_TRUE(overlaps(mid, midBig));
	ASSERT_TRUE(!overlaps(mid, left));
	ASSERT_TRUE(overlaps(midBig, left));
}

UTEST(Intersection, aabb2d_vs_aabb2d_test)
{
	using namespace sfz;

	AABB2D mid{vec2{0.0f}, vec2{2.0f}};
	AABB2D midBig{vec2{0.0f}, vec2{4.0f}};
	AABB2D left{vec2{-2.1f, 0.0f}, vec2{2.0f}};

	ASSERT_TRUE(overlaps(mid, midBig));
	ASSERT_TRUE(!overlaps(mid, left));
	ASSERT_TRUE(overlaps(midBig, left));
}

UTEST(Intersection, aabb2d_vs_circle_test)
{
	using namespace sfz;

	AABB2D rMid{vec2{0.0f}, vec2{2.0f}};
	AABB2D rMidBig{vec2{0.0f}, vec2{4.0f}};
	AABB2D rLeft{vec2{-2.1f, 0.0f}, vec2{2.0f}};

	Circle cMid{vec2{0.0f}, 1.0f};
	Circle cMidBig{vec2{0.0f}, 2.0f};
	Circle cLeft{vec2{-2.1f, 0.0f}, 1.0f};

	ASSERT_TRUE(overlaps(rMid, cMid));
	ASSERT_TRUE(overlaps(rMid, cMidBig));
	ASSERT_TRUE(!overlaps(rMid, cLeft));

	ASSERT_TRUE(overlaps(rMidBig, cMid));
	ASSERT_TRUE(overlaps(rMidBig, cMidBig));
	ASSERT_TRUE(overlaps(rMidBig, cLeft));

	ASSERT_TRUE(!overlaps(rLeft, cMid));
	ASSERT_TRUE(overlaps(rLeft, cMidBig));
	ASSERT_TRUE(overlaps(rLeft, cLeft));

	ASSERT_TRUE(overlaps(cMid, rMid));
	ASSERT_TRUE(overlaps(cMid, rMidBig));
	ASSERT_TRUE(!overlaps(cMid, rLeft));

	ASSERT_TRUE(overlaps(cMidBig, rMid));
	ASSERT_TRUE(overlaps(cMidBig, rMidBig));
	ASSERT_TRUE(overlaps(cMidBig, rLeft));

	ASSERT_TRUE(!overlaps(cLeft, rMid));
	ASSERT_TRUE(overlaps(cLeft, rMidBig));
	ASSERT_TRUE(overlaps(cLeft, rLeft));
}

UTEST(Intersection, plane_vs_aabb_test)
{
	using namespace sfz;

	Plane p1{vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 0.5f, 0.0f}};
	Plane p2{vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 1.5f, 0.0f}};
	AABB aabb{vec3{1.0f, 1.0f, 1.0f}, vec3{3.0f, 3.0f, 3.0f}};

	ASSERT_TRUE(!intersects(p1, aabb));
	ASSERT_TRUE(intersects(p2, aabb));
}

UTEST(Intersection, plane_vs_obb_test)
{
	using namespace sfz;

	Plane p1{vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 0.5f, 0.0f}};
	Plane p2{vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 1.5f, 0.0f}};
	OBB obb{AABB{vec3{1.0f, 1.0f, 1.0f}, vec3{3.0f, 3.0f, 3.0f}}};

	ASSERT_TRUE(!intersects(p1, obb));
	ASSERT_TRUE(intersects(p2, obb));
}
