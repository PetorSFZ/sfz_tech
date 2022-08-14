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

#include <sfz/geometry/OBB.hpp>

using namespace sfz;

TEST_CASE("OBB: constructors")
{
	// Normal constructor
	{
		f32x3 pos = f32x3_init(1.0f, 2.0f, 3.0f);
		f32x3 xAxis = f32x3_init(0.0f, -1.0f, 0.0f);
		f32x3 yAxis = f32x3_init(1.0f, 0.0f, 0.0f);
		f32x3 zAxis = f32x3_init(0.0f, 0.0f, 1.0f);
		f32x3 extents = f32x3_init(4.0f, 5.0f, 6.0f);
		OBB obb = OBB(pos, xAxis, yAxis, zAxis, extents);
		CHECK(eqf(obb.center, pos));
		CHECK(eqf(obb.xAxis(), xAxis));
		CHECK(eqf(obb.yAxis(), yAxis));
		CHECK(eqf(obb.zAxis(), zAxis));
		CHECK(eqf(obb.halfExtents, extents * 0.5f));
	}
	// AABB constructor
	{
		f32x3 pos = f32x3_init(1.0f, 2.0f, 3.0f);
		f32x3 ext = f32x3_init(4.0f, 5.0f, 6.0f);
		AABB aabb = AABB::fromPosDims(pos, ext);
		OBB obb = OBB(aabb);
		CHECK(eqf(obb.center, pos));
		CHECK(eqf(obb.xAxis(), f32x3_init(1.0f, 0.0f, 0.0f)));
		CHECK(eqf(obb.yAxis(), f32x3_init(0.0f, 1.0f, 0.0f)));
		CHECK(eqf(obb.zAxis(), f32x3_init(0.0f, 0.0f, 1.0f)));
		CHECK(eqf(obb.halfExtents, ext * 0.5f));
	}
}

TEST_CASE("OBB: transform_obb")
{
	OBB identityObb = OBB(AABB::fromPosDims(f32x3_splat(0.0f), f32x3_splat(1.0f)));
	CHECK(eqf(identityObb.center, f32x3_splat(0.0f)));
	CHECK(eqf(identityObb.xAxis(), f32x3_init(1.0f, 0.0f, 0.0f)));
	CHECK(eqf(identityObb.yAxis(), f32x3_init(0.0f, 1.0f, 0.0f)));
	CHECK(eqf(identityObb.zAxis(), f32x3_init(0.0f, 0.0f, 1.0f)));
	CHECK(eqf(identityObb.halfExtents, f32x3_splat(0.5f)));

	SfzMat44 rot1 = sfzMat44Rotation3(f32x3_init(0.0f, 0.0f, -1.0f), 3.1415926f * 0.5f);
	CHECK(eqf(sfzMat44TransformDir(rot1, f32x3_init(0.0f, 1.0f, 0.0f)), f32x3_init(1.0f, 0.0f, 0.0f)));

	SfzMat44  rot2 = sfzMat44Rotation3(f32x3_init(1.0f, 0.0f, 0.0f), 3.1415926f * 0.5f);
	CHECK(eqf(sfzMat44TransformDir(rot2, f32x3_init(0.0f, 1.0f, 0.0f)), f32x3_init(0.0f, 0.0f, 1.0f)));

	SfzMat44 rot3 = rot2 * rot1;
	CHECK(eqf(sfzMat44TransformDir(rot3, f32x3_init(1.0f, 0.0f, 0.0f)), f32x3_init(0.0f, 0.0f, -1.0f)));
	CHECK(eqf(sfzMat44TransformDir(rot3, f32x3_init(0.0f, 1.0f, 0.0f)), f32x3_init(1.0f, 0.0f, 0.0f)));
	CHECK(eqf(sfzMat44TransformDir(rot3, f32x3_init(0.0f, 0.0f, 1.0f)), f32x3_init(0.0f, -1.0f, 0.0f)));

	OBB obb1 = identityObb.transformOBB(rot3);
	CHECK(eqf(obb1.halfExtents, identityObb.halfExtents));
	CHECK(eqf(obb1.center, identityObb.center));
	CHECK(eqf(obb1.xAxis(), f32x3_init(0.0f, 0.0f, -1.0f)));
	CHECK(eqf(obb1.yAxis(), f32x3_init(1.0f, 0.0f, 0.0f)));
	CHECK(eqf(obb1.zAxis(), f32x3_init(0.0f, -1.0f, 0.0f)));

	SfzMat44 scaleRot = rot3 * sfzMat44Scaling3(f32x3_init(4.0f, 5.0f, 6.0f));
	OBB obb2 = identityObb.transformOBB(scaleRot);
	CHECK(eqf(obb2.halfExtents, f32x3_init(2.0f, 2.5f, 3.0f), 0.01f));
	CHECK(eqf(obb2.center, identityObb.center));
	CHECK(eqf(obb2.xAxis(), f32x3_init(0.0f, 0.0f, -1.0f)));
	CHECK(eqf(obb2.yAxis(), f32x3_init(1.0f, 0.0f, 0.0f)));
	CHECK(eqf(obb2.zAxis(), f32x3_init(0.0f, -1.0f, 0.0f)));

	SfzMat44 rotTranslScale = sfzMat44Translation3(f32x3_init(1.0f, 2.0f, 3.0f)) * scaleRot;
	OBB obb3 = identityObb.transformOBB(rotTranslScale);
	CHECK(eqf(obb3.halfExtents, f32x3_init(2.0f, 2.5f, 3.0f), 0.01f));
	CHECK(eqf(obb3.center, f32x3_init(1.0f, 2.0f, 3.0f)));
	CHECK(eqf(obb3.xAxis(), f32x3_init(0.0f, 0.0f, -1.0f)));
	CHECK(eqf(obb3.yAxis(), f32x3_init(1.0f, 0.0f, 0.0f)));
	CHECK(eqf(obb3.zAxis(), f32x3_init(0.0f, -1.0f, 0.0f)));

	SfzQuat q = sfzQuatFromRotationMatrix(sfzMat33FromMat44(rot3));
	OBB obb4 = identityObb.transformOBB(q);
	CHECK(eqf(obb4.halfExtents, identityObb.halfExtents));
	CHECK(eqf(obb4.center, identityObb.center));
	CHECK(eqf(obb4.xAxis(), f32x3_init(0.0f, 0.0f, -1.0f)));
	CHECK(eqf(obb4.yAxis(), f32x3_init(1.0f, 0.0f, 0.0f)));
	CHECK(eqf(obb4.zAxis(), f32x3_init(0.0f, -1.0f, 0.0f)));
}
