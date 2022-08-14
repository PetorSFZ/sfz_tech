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

#include <vector>

#include <sfz.h>
#include <skipifzero_geometry.hpp>

#include "sfz/geometry/AABB2D.hpp"
#include "sfz/geometry/Circle.hpp"
#include "sfz/geometry/Intersection.hpp"
#include "sfz/geometry/OBB.hpp"
#include "sfz/geometry/Plane.hpp"
#include "sfz/geometry/Sphere.hpp"

TEST_CASE("Intersection: signed_distance_to_plane")
{
	using namespace sfz;

	Plane p{f32x3{0.0f, 1.0f, 0.0f}, f32x3{2.0f, 1.0f, 0.0f}};

	CHECK(eqf(p.signedDistance(f32x3{2.0f, 3.0f, 0.0f}), 2.0f));
	CHECK(eqf(p.signedDistance(f32x3{0.0f, 3.0f, 0.0f}), 2.0f));
	CHECK(eqf(p.signedDistance(f32x3{2.0f, 0.0f, 0.0f}), -1.0f));
}

TEST_CASE("Intersection: point_inside_aabb_test")
{
	using namespace sfz;

	AABB box{f32x3{-1,-1,-1}, f32x3{1,1,1}};

	CHECK(pointInside(box, f32x3{0,0,0}));
	CHECK(!pointInside(box, f32x3{-2,0,0}));
}

TEST_CASE("Intersection: aabb_vs_aabb_test")
{
	using namespace sfz;

	AABB boxMid{f32x3{-1.0f, -1.0f, -1.0f}, f32x3{1.0f, 1.0f, 1.0f}};
	AABB boxMidSmall{f32x3{-0.5f, -0.5f, -0.5f}, f32x3{0.5f, 0.5f, 0.5f}};

	AABB boxFrontSmall{f32x3{-0.5f, -0.5f, 0.75f}, f32x3{0.5f, 0.5f, 1.75f}};
	AABB boxBackSmall{f32x3{-0.5f, -0.5f, -1.75f}, f32x3{0.5f, 0.5f, -0.75f}};

	AABB boxUpSmall{f32x3{-0.5f, 0.75f, -0.5f}, f32x3{0.5f, 1.75f, 0.5f}};
	AABB boxDownSmall{f32x3{-0.5f, -1.75f, -0.5f}, f32x3{0.5f, -0.75f, 0.5f}};

	AABB boxLeftSmall{f32x3{-1.75f, -0.5f, -0.5f}, f32x3{-0.75f, 0.5f, 0.5f}};
	AABB boxRightSMall{f32x3{0.75f, -0.5f, -0.5f}, f32x3{1.75f, 0.5f, 0.5f}};

	std::vector<AABB*> smallSurroundingBoxes;
	smallSurroundingBoxes.push_back(&boxFrontSmall);
	smallSurroundingBoxes.push_back(&boxBackSmall);
	smallSurroundingBoxes.push_back(&boxUpSmall);
	smallSurroundingBoxes.push_back(&boxDownSmall);
	smallSurroundingBoxes.push_back(&boxLeftSmall);
	smallSurroundingBoxes.push_back(&boxRightSMall);

	CHECK(intersects(boxMidSmall, boxMid));

	for (AABB* boxPtr : smallSurroundingBoxes) {
		CHECK(intersects(boxMid, *boxPtr));
	}

	for (AABB* boxPtr : smallSurroundingBoxes) {
		CHECK(!intersects(boxMidSmall, *boxPtr));
	}

	for (AABB* boxPtr1 : smallSurroundingBoxes) {
		for (AABB* boxPtr2 : smallSurroundingBoxes) {
			if (boxPtr1 == boxPtr2) { CHECK(intersects(*boxPtr1, *boxPtr2)); }
			else { CHECK(!intersects(*boxPtr1, *boxPtr2)); }
		}
	}
}

TEST_CASE("Intersection: obb_vs_obb_test")
{
	using namespace sfz;

	const f32x3 axisAlignedAxes[3] = {
		f32x3_init(1.0f, 0.0f, 0.0f),
		f32x3_init(0.0f, 1.0f, 0.0f),
		f32x3_init(0.0f, 0.0f, 1.0f)
	};
	f32x3 smallExts{1.0f, 1.0f, 1.0f};
	f32x3 bigExts{2.0f, 2.0f, 2.0f};

	OBB midSmallAA(f32x3_init(0.0f, 0.0f, 0.0f), axisAlignedAxes, smallExts);
	OBB midSmallLeftAA(f32x3_init(-1.0f, 0.0f, 0.0f), axisAlignedAxes, smallExts);
	OBB midSmallRightAA(f32x3_init(1.0f, 0.0f, 0.0f), axisAlignedAxes, smallExts);
	OBB midSmallDownAA(f32x3_init(0.0f, -1.0f, 0.0f), axisAlignedAxes, smallExts);
	OBB midSmallUpAA(f32x3_init(0.0f, 1.0f, 0.0f), axisAlignedAxes, smallExts);
	OBB midSmallBackAA(f32x3_init(0.0f, 0.0f, -1.0f), axisAlignedAxes, smallExts);
	OBB midSmallFrontAA(f32x3_init(0.0f, 0.0f, 1.0f), axisAlignedAxes, smallExts);
	std::vector<OBB*> smallSurroundingAABoxes;
	smallSurroundingAABoxes.push_back(&midSmallLeftAA);
	smallSurroundingAABoxes.push_back(&midSmallRightAA);
	smallSurroundingAABoxes.push_back(&midSmallDownAA);
	smallSurroundingAABoxes.push_back(&midSmallUpAA);
	smallSurroundingAABoxes.push_back(&midSmallBackAA);
	smallSurroundingAABoxes.push_back(&midSmallFrontAA);

	OBB midAA(f32x3_init(0.0f, 0.0f, 0.0f), axisAlignedAxes, bigExts);
	OBB midLeftAA(f32x3_init(-1.0f, 0.0f, 0.0f), axisAlignedAxes, bigExts);
	OBB midRightAA(f32x3_init(1.0f, 0.0f, 0.0f), axisAlignedAxes, bigExts);
	OBB midDownAA(f32x3_init(0.0f, -1.0f, 0.0f), axisAlignedAxes, bigExts);
	OBB midUpAA(f32x3_init(0.0f, 1.0f, 0.0f), axisAlignedAxes, bigExts);
	OBB midBackAA(f32x3_init(0.0f, 0.0f, -1.0f), axisAlignedAxes, bigExts);
	OBB midFrontAA(f32x3_init(0.0f, 0.0f, 1.0f), axisAlignedAxes, bigExts);
	std::vector<OBB*> surroundingAABoxes;
	surroundingAABoxes.push_back(&midLeftAA);
	surroundingAABoxes.push_back(&midRightAA);
	surroundingAABoxes.push_back(&midDownAA);
	surroundingAABoxes.push_back(&midUpAA);
	surroundingAABoxes.push_back(&midBackAA);
	surroundingAABoxes.push_back(&midFrontAA);

	for (OBB* smallPtr : smallSurroundingAABoxes) {
		CHECK(intersects(*smallPtr, midAA));
		CHECK(intersects(midAA, *smallPtr));
	}

	CHECK(!intersects(midSmallLeftAA, midSmallRightAA));
	CHECK(!intersects(midSmallDownAA, midSmallUpAA));
	CHECK(!intersects(midSmallBackAA, midSmallFrontAA));

	CHECK(!intersects(midSmallLeftAA, midRightAA));
	CHECK(!intersects(midSmallDownAA, midUpAA));
	CHECK(!intersects(midSmallBackAA, midFrontAA));

	CHECK(!intersects(midLeftAA, midSmallRightAA));
	CHECK(!intersects(midDownAA, midSmallUpAA));
	CHECK(!intersects(midBackAA, midSmallFrontAA));

	// Non-trivial edge case
	OBB nonTrivial1st = OBB(f32x3_splat(0.0f), axisAlignedAxes, f32x3_splat(2.0f));
	OBB nonTrivial2nd = OBB(f32x3_splat(2.0f), axisAlignedAxes, f32x3_splat(2.0f));
	nonTrivial2nd = nonTrivial2nd.transformOBB(sfzQuatFromEuler(f32x3_init(45.0f, 45.0f, 45.0f)));

	CHECK(!intersects(nonTrivial1st, nonTrivial2nd));
}

TEST_CASE("Intersection: sphere_vs_sphere_test")
{
	using namespace sfz;

	Sphere mid(f32x3_init(0.0f, 0.0f, 0.0f), 0.5f);
	Sphere midBig(f32x3_init(0.0f, 0.0f, 0.0f), 1.0f);
	Sphere aBitOff(f32x3_init(-1.1f, 0.0f, 0.0f), 0.5f);

	CHECK(intersects(mid, midBig));
	CHECK(intersects(midBig, aBitOff));
	CHECK(!intersects(mid, aBitOff));
}

TEST_CASE("Intersection: circle_vs_circle_test")
{
	using namespace sfz;

	Circle mid(f32x2_splat(0.0f), 1.0f);
	Circle midBig(f32x2_splat(0.0f), 2.0f);
	Circle left(f32x2_init(-2.1f, 0.0f), 1.0f);

	CHECK(overlaps(mid, midBig));
	CHECK(!overlaps(mid, left));
	CHECK(overlaps(midBig, left));
}

TEST_CASE("Intersection: aabb2d_vs_aabb2d_test")
{
	using namespace sfz;

	AABB2D mid(f32x2_splat(0.0f), f32x2_splat(2.0f));
	AABB2D midBig(f32x2_splat(0.0f), f32x2_splat(4.0f));
	AABB2D left(f32x2_init(-2.1f, 0.0f), f32x2_splat(2.0f));

	CHECK(overlaps(mid, midBig));
	CHECK(!overlaps(mid, left));
	CHECK(overlaps(midBig, left));
}

TEST_CASE("Intersection: aabb2d_vs_circle_test")
{
	using namespace sfz;

	AABB2D rMid(f32x2_splat(0.0f), f32x2_splat(2.0f));
	AABB2D rMidBig(f32x2_splat(0.0f), f32x2_splat(4.0f));
	AABB2D rLeft(f32x2_init(-2.1f, 0.0f), f32x2_splat(2.0f));

	Circle cMid(f32x2_splat(0.0f), 1.0f);
	Circle cMidBig(f32x2_splat(0.0f), 2.0f);
	Circle cLeft(f32x2_init(-2.1f, 0.0f), 1.0f);

	CHECK(overlaps(rMid, cMid));
	CHECK(overlaps(rMid, cMidBig));
	CHECK(!overlaps(rMid, cLeft));

	CHECK(overlaps(rMidBig, cMid));
	CHECK(overlaps(rMidBig, cMidBig));
	CHECK(overlaps(rMidBig, cLeft));

	CHECK(!overlaps(rLeft, cMid));
	CHECK(overlaps(rLeft, cMidBig));
	CHECK(overlaps(rLeft, cLeft));

	CHECK(overlaps(cMid, rMid));
	CHECK(overlaps(cMid, rMidBig));
	CHECK(!overlaps(cMid, rLeft));

	CHECK(overlaps(cMidBig, rMid));
	CHECK(overlaps(cMidBig, rMidBig));
	CHECK(overlaps(cMidBig, rLeft));

	CHECK(!overlaps(cLeft, rMid));
	CHECK(overlaps(cLeft, rMidBig));
	CHECK(overlaps(cLeft, rLeft));
}

TEST_CASE("Intersection: plane_vs_aabb_test")
{
	using namespace sfz;

	Plane p1(f32x3_init(0.0f, 1.0f, 0.0f), f32x3_init(0.0f, 0.5f, 0.0f));
	Plane p2(f32x3_init(0.0f, 1.0f, 0.0f), f32x3_init(0.0f, 1.5f, 0.0f));
	AABB aabb = AABB::fromCorners(f32x3_init(1.0f, 1.0f, 1.0f), f32x3_init(3.0f, 3.0f, 3.0f));

	CHECK(!intersects(p1, aabb));
	CHECK(intersects(p2, aabb));
}

TEST_CASE("Intersection: plane_vs_obb_test")
{
	using namespace sfz;

	Plane p1(f32x3_init(0.0f, 1.0f, 0.0f), f32x3_init(0.0f, 0.5f, 0.0f));
	Plane p2(f32x3_init(0.0f, 1.0f, 0.0f), f32x3_init(0.0f, 1.5f, 0.0f));
	OBB obb(AABB::fromCorners(f32x3_init(1.0f, 1.0f, 1.0f), f32x3_init(3.0f, 3.0f, 3.0f)));

	CHECK(!intersects(p1, obb));
	CHECK(intersects(p2, obb));
}
