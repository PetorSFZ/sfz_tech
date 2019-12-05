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
#include "catch2/catch.hpp"
#include "sfz/PopWarnings.hpp"

#include <sfz/geometry/OBB.hpp>

using namespace sfz;

TEST_CASE("OBB Constructors", "[sfz::OBB]")
{
	SECTION("Normal constructor") {
		vec3 pos = vec3(1.0f, 2.0f, 3.0f);
		vec3 xAxis = vec3(0.0f, -1.0f, 0.0f);
		vec3 yAxis = vec3(1.0f, 0.0f, 0.0f);
		vec3 zAxis = vec3(0.0f, 0.0f, 1.0f);
		vec3 extents = vec3(4.0f, 5.0f, 6.0f);
		OBB obb = OBB(pos, xAxis, yAxis, zAxis, extents);
		REQUIRE(equalsApprox(obb.center, pos));
		REQUIRE(equalsApprox(obb.xAxis(), xAxis));
		REQUIRE(equalsApprox(obb.yAxis(), yAxis));
		REQUIRE(equalsApprox(obb.zAxis(), zAxis));
		REQUIRE(equalsApprox(obb.halfExtents, extents * 0.5f));
	}
	SECTION("AABB constructor") {
		vec3 pos = vec3(1.0f, 2.0f, 3.0f);
		vec3 ext = vec3(4.0f, 5.0f, 6.0f);
		AABB aabb = AABB(pos, ext.x, ext.y, ext.z);
		OBB obb = OBB(aabb);
		REQUIRE(equalsApprox(obb.center, pos));
		REQUIRE(equalsApprox(obb.xAxis(), vec3(1.0f, 0.0f, 0.0f)));
		REQUIRE(equalsApprox(obb.yAxis(), vec3(0.0f, 1.0f, 0.0f)));
		REQUIRE(equalsApprox(obb.zAxis(), vec3(0.0f, 0.0f, 1.0f)));
		REQUIRE(equalsApprox(obb.halfExtents, ext * 0.5f));
	}
}

TEST_CASE("OBB: transformOBB()", "[sfz:OBB]")
{
	OBB identityObb = OBB(AABB(vec3(0.0f), 1.0f, 1.0f, 1.0f));
	REQUIRE(equalsApprox(identityObb.center, vec3(0.0f)));
	REQUIRE(equalsApprox(identityObb.xAxis(), vec3(1.0f, 0.0f, 0.0f)));
	REQUIRE(equalsApprox(identityObb.yAxis(), vec3(0.0f, 1.0f, 0.0f)));
	REQUIRE(equalsApprox(identityObb.zAxis(), vec3(0.0f, 0.0f, 1.0f)));
	REQUIRE(equalsApprox(identityObb.halfExtents, vec3(0.5f)));

	mat44 rot1 = mat44::rotation3(vec3(0.0f, 0.0f, -1.0f), 3.1415926f * 0.5f);
	REQUIRE(equalsApprox(transformDir(rot1, vec3(0.0f, 1.0f, 0.0f)), vec3(1.0f, 0.0f, 0.0f)));

	mat44 rot2 = mat44::rotation3(vec3(1.0f, 0.0f, 0.0f), 3.1415926f * 0.5f);
	REQUIRE(equalsApprox(transformDir(rot2, vec3(0.0f, 1.0f, 0.0f)), vec3(0.0f, 0.0f, 1.0f)));

	mat44 rot3 = rot2 * rot1;
	REQUIRE(equalsApprox(transformDir(rot3, vec3(1.0f, 0.0f, 0.0f)), vec3(0.0f, 0.0f, -1.0f)));
	REQUIRE(equalsApprox(transformDir(rot3, vec3(0.0f, 1.0f, 0.0f)), vec3(1.0f, 0.0f, 0.0f)));
	REQUIRE(equalsApprox(transformDir(rot3, vec3(0.0f, 0.0f, 1.0f)), vec3(0.0f, -1.0f, 0.0f)));

	OBB obb1 = identityObb.transformOBB(rot3.row012);
	REQUIRE(equalsApprox(obb1.halfExtents, identityObb.halfExtents));
	REQUIRE(equalsApprox(obb1.center, identityObb.center));
	REQUIRE(equalsApprox(obb1.xAxis(), vec3(0.0f, 0.0f, -1.0f)));
	REQUIRE(equalsApprox(obb1.yAxis(), vec3(1.0f, 0.0f, 0.0f)));
	REQUIRE(equalsApprox(obb1.zAxis(), vec3(0.0f, -1.0f, 0.0f)));

	mat4 scaleRot = rot3 * mat44::scaling3(4.0f, 5.0f, 6.0f);
	OBB obb2 = identityObb.transformOBB(scaleRot.row012);
	REQUIRE(equalsApprox(obb2.halfExtents, vec3(2.0f, 2.5f, 3.0f), 0.01f));
	REQUIRE(equalsApprox(obb2.center, identityObb.center));
	REQUIRE(equalsApprox(obb2.xAxis(), vec3(0.0f, 0.0f, -1.0f)));
	REQUIRE(equalsApprox(obb2.yAxis(), vec3(1.0f, 0.0f, 0.0f)));
	REQUIRE(equalsApprox(obb2.zAxis(), vec3(0.0f, -1.0f, 0.0f)));

	mat4 rotTranslScale = mat44::translation3(vec3(1.0f, 2.0f, 3.0f)) * scaleRot;
	OBB obb3 = identityObb.transformOBB(rotTranslScale.row012);
	REQUIRE(equalsApprox(obb3.halfExtents, vec3(2.0f, 2.5f, 3.0f), 0.01f));
	REQUIRE(equalsApprox(obb3.center, vec3(1.0f, 2.0f, 3.0f)));
	REQUIRE(equalsApprox(obb3.xAxis(), vec3(0.0f, 0.0f, -1.0f)));
	REQUIRE(equalsApprox(obb3.yAxis(), vec3(1.0f, 0.0f, 0.0f)));
	REQUIRE(equalsApprox(obb3.zAxis(), vec3(0.0f, -1.0f, 0.0f)));

	Quaternion q = Quaternion::fromRotationMatrix(rot3.row012);
	OBB obb4 = identityObb.transformOBB(q);
	REQUIRE(equalsApprox(obb4.halfExtents, identityObb.halfExtents));
	REQUIRE(equalsApprox(obb4.center, identityObb.center));
	REQUIRE(equalsApprox(obb4.xAxis(), vec3(0.0f, 0.0f, -1.0f)));
	REQUIRE(equalsApprox(obb4.yAxis(), vec3(1.0f, 0.0f, 0.0f)));
	REQUIRE(equalsApprox(obb4.zAxis(), vec3(0.0f, -1.0f, 0.0f)));
}
