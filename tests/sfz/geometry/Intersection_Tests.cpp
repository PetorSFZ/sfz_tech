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

#include "sfz/PushWarnings.hpp"
#include "catch.hpp"
#include "sfz/PopWarnings.hpp"

#include <vector>

#include "sfz/Geometry.hpp"
#include "sfz/Math.hpp"

TEST_CASE("Signed distance to plane", "[sfz::Plane]")
{
	using namespace sfz;

	Plane p{vec3{0.0f, 1.0f, 0.0f}, vec3{2.0f, 1.0f, 0.0f}};

	REQUIRE(approxEqual(p.signedDistance(vec3{2.0f, 3.0f, 0.0f}), 2.0f));
	REQUIRE(approxEqual(p.signedDistance(vec3{0.0f, 3.0f, 0.0f}), 2.0f));
	REQUIRE(approxEqual(p.signedDistance(vec3{2.0f, 0.0f, 0.0f}), -1.0f));
}

TEST_CASE("Point inside AABB test", "[sfz::Intersection]")
{
	using namespace sfz;

	AABB box{vec3{-1,-1,-1}, vec3{1,1,1}};

	REQUIRE(pointInside(box, vec3{0,0,0}));
	REQUIRE(!pointInside(box, vec3{-2,0,0}));
}

TEST_CASE("AABB vs AABB test", "[sfz::Intersection]")
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

	REQUIRE(intersects(boxMidSmall, boxMid));

	for (AABB* boxPtr : smallSurroundingBoxes) {
		REQUIRE(intersects(boxMid, *boxPtr));
	}

	for (AABB* boxPtr : smallSurroundingBoxes) {
		REQUIRE(!intersects(boxMidSmall, *boxPtr));
	}

	for (AABB* boxPtr1 : smallSurroundingBoxes) {
		for (AABB* boxPtr2 : smallSurroundingBoxes) {
			if (boxPtr1 == boxPtr2) REQUIRE(intersects(*boxPtr1, *boxPtr2));
			else REQUIRE(!intersects(*boxPtr1, *boxPtr2));
		}
	}
}

TEST_CASE("OBB vs OBB test", "[sfz::Intersection]")
{
	using namespace sfz;

	OBBAxes axisAlignedAxes{{vec3{1.0f, 0.0f, 0.0f},
	                                    vec3{0.0f, 1.0f, 0.0f},
	                                    vec3{0.0f, 0.0f, 1.0f}}};
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
		REQUIRE(intersects(*smallPtr, midAA));
		REQUIRE(intersects(midAA, *smallPtr));
	}

	REQUIRE(!intersects(midSmallLeftAA, midSmallRightAA));
	REQUIRE(!intersects(midSmallDownAA, midSmallUpAA));
	REQUIRE(!intersects(midSmallBackAA, midSmallFrontAA));

	REQUIRE(!intersects(midSmallLeftAA, midRightAA));
	REQUIRE(!intersects(midSmallDownAA, midUpAA));
	REQUIRE(!intersects(midSmallBackAA, midFrontAA));

	REQUIRE(!intersects(midLeftAA, midSmallRightAA));
	REQUIRE(!intersects(midDownAA, midSmallUpAA));
	REQUIRE(!intersects(midBackAA, midSmallFrontAA));
}

TEST_CASE("Sphere vs Sphere test", "[sfz::Intersection]")
{
	using namespace sfz;

	Sphere mid{vec3{0.0f, 0.0f, 0.0f}, 0.5f};
	Sphere midBig{vec3{0.0f, 0.0f, 0.0f}, 1.0f};
	Sphere aBitOff{vec3{-1.1f, 0.0f, 0.0f}, 0.5f};

	REQUIRE(intersects(mid, midBig));
	REQUIRE(intersects(midBig, aBitOff));
	REQUIRE(!intersects(mid, aBitOff));
}

TEST_CASE("Circle vs Circle test", "[sfz::Intersection]")
{
	using namespace sfz;

	Circle mid{vec2{0.0f}, 1.0f};
	Circle midBig{vec2{0.0f}, 2.0f};
	Circle left{vec2{-2.1f, 0.0f}, 1.0f};

	REQUIRE(overlaps(mid, midBig));
	REQUIRE(!overlaps(mid, left));
	REQUIRE(overlaps(midBig, left));
}

TEST_CASE("AABB2D vs AABB2D test", "[sfz::Intersection]")
{
	using namespace sfz;

	AABB2D mid{vec2{0.0f}, vec2{2.0f}};
	AABB2D midBig{vec2{0.0f}, vec2{4.0f}};
	AABB2D left{vec2{-2.1f, 0.0f}, vec2{2.0f}};

	REQUIRE(overlaps(mid, midBig));
	REQUIRE(!overlaps(mid, left));
	REQUIRE(overlaps(midBig, left));
}

TEST_CASE("AABB2D vs Circle test", "[sfz::Intersection]")
{
	using namespace sfz;

	AABB2D rMid{vec2{0.0f}, vec2{2.0f}};
	AABB2D rMidBig{vec2{0.0f}, vec2{4.0f}};
	AABB2D rLeft{vec2{-2.1f, 0.0f}, vec2{2.0f}};

	Circle cMid{vec2{0.0f}, 1.0f};
	Circle cMidBig{vec2{0.0f}, 2.0f};
	Circle cLeft{vec2{-2.1f, 0.0f}, 1.0f};

	REQUIRE(overlaps(rMid, cMid));
	REQUIRE(overlaps(rMid, cMidBig));
	REQUIRE(!overlaps(rMid, cLeft));

	REQUIRE(overlaps(rMidBig, cMid));
	REQUIRE(overlaps(rMidBig, cMidBig));
	REQUIRE(overlaps(rMidBig, cLeft));

	REQUIRE(!overlaps(rLeft, cMid));
	REQUIRE(overlaps(rLeft, cMidBig));
	REQUIRE(overlaps(rLeft, cLeft));

	REQUIRE(overlaps(cMid, rMid));
	REQUIRE(overlaps(cMid, rMidBig));
	REQUIRE(!overlaps(cMid, rLeft));

	REQUIRE(overlaps(cMidBig, rMid));
	REQUIRE(overlaps(cMidBig, rMidBig));
	REQUIRE(overlaps(cMidBig, rLeft));

	REQUIRE(!overlaps(cLeft, rMid));
	REQUIRE(overlaps(cLeft, rMidBig));
	REQUIRE(overlaps(cLeft, rLeft));
}

TEST_CASE("Plane vs AABB test", "[sfz::Intersection]")
{
	using namespace sfz;

	Plane p1{vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 0.5f, 0.0f}};
	Plane p2{vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 1.5f, 0.0f}};
	AABB aabb{vec3{1.0f, 1.0f, 1.0f}, vec3{3.0f, 3.0f, 3.0f}};

	REQUIRE(!intersects(p1, aabb));
	REQUIRE(intersects(p2, aabb));
}

TEST_CASE("Plane vs OBB test", "[sfz::Intersection]")
{
	using namespace sfz;

	Plane p1{vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 0.5f, 0.0f}};
	Plane p2{vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 1.5f, 0.0f}};
	OBB obb{AABB{vec3{1.0f, 1.0f, 1.0f}, vec3{3.0f, 3.0f, 3.0f}}};

	REQUIRE(!intersects(p1, obb));
	REQUIRE(intersects(p2, obb));
}