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

#include <sfz/geometry/OBB.hpp>

using namespace sfz;

UTEST(OBB, constructors)
{
	// Normal constructor
	{
		f32x3 pos = f32x3(1.0f, 2.0f, 3.0f);
		f32x3 xAxis = f32x3(0.0f, -1.0f, 0.0f);
		f32x3 yAxis = f32x3(1.0f, 0.0f, 0.0f);
		f32x3 zAxis = f32x3(0.0f, 0.0f, 1.0f);
		f32x3 extents = f32x3(4.0f, 5.0f, 6.0f);
		OBB obb = OBB(pos, xAxis, yAxis, zAxis, extents);
		ASSERT_TRUE(eqf(obb.center, pos));
		ASSERT_TRUE(eqf(obb.xAxis(), xAxis));
		ASSERT_TRUE(eqf(obb.yAxis(), yAxis));
		ASSERT_TRUE(eqf(obb.zAxis(), zAxis));
		ASSERT_TRUE(eqf(obb.halfExtents, extents * 0.5f));
	}
	// AABB constructor
	{
		f32x3 pos = f32x3(1.0f, 2.0f, 3.0f);
		f32x3 ext = f32x3(4.0f, 5.0f, 6.0f);
		AABB aabb = AABB::fromPosDims(pos, ext);
		OBB obb = OBB(aabb);
		ASSERT_TRUE(eqf(obb.center, pos));
		ASSERT_TRUE(eqf(obb.xAxis(), f32x3(1.0f, 0.0f, 0.0f)));
		ASSERT_TRUE(eqf(obb.yAxis(), f32x3(0.0f, 1.0f, 0.0f)));
		ASSERT_TRUE(eqf(obb.zAxis(), f32x3(0.0f, 0.0f, 1.0f)));
		ASSERT_TRUE(eqf(obb.halfExtents, ext * 0.5f));
	}
}

UTEST(OBB, transform_obb)
{
	OBB identityObb = OBB(AABB::fromPosDims(f32x3(0.0f), f32x3(1.0f)));
	ASSERT_TRUE(eqf(identityObb.center, f32x3(0.0f)));
	ASSERT_TRUE(eqf(identityObb.xAxis(), f32x3(1.0f, 0.0f, 0.0f)));
	ASSERT_TRUE(eqf(identityObb.yAxis(), f32x3(0.0f, 1.0f, 0.0f)));
	ASSERT_TRUE(eqf(identityObb.zAxis(), f32x3(0.0f, 0.0f, 1.0f)));
	ASSERT_TRUE(eqf(identityObb.halfExtents, f32x3(0.5f)));

	mat44 rot1 = mat44::rotation3(f32x3(0.0f, 0.0f, -1.0f), 3.1415926f * 0.5f);
	ASSERT_TRUE(eqf(transformDir(rot1, f32x3(0.0f, 1.0f, 0.0f)), f32x3(1.0f, 0.0f, 0.0f)));

	mat44 rot2 = mat44::rotation3(f32x3(1.0f, 0.0f, 0.0f), 3.1415926f * 0.5f);
	ASSERT_TRUE(eqf(transformDir(rot2, f32x3(0.0f, 1.0f, 0.0f)), f32x3(0.0f, 0.0f, 1.0f)));

	mat44 rot3 = rot2 * rot1;
	ASSERT_TRUE(eqf(transformDir(rot3, f32x3(1.0f, 0.0f, 0.0f)), f32x3(0.0f, 0.0f, -1.0f)));
	ASSERT_TRUE(eqf(transformDir(rot3, f32x3(0.0f, 1.0f, 0.0f)), f32x3(1.0f, 0.0f, 0.0f)));
	ASSERT_TRUE(eqf(transformDir(rot3, f32x3(0.0f, 0.0f, 1.0f)), f32x3(0.0f, -1.0f, 0.0f)));

	OBB obb1 = identityObb.transformOBB(mat34(rot3));
	ASSERT_TRUE(eqf(obb1.halfExtents, identityObb.halfExtents));
	ASSERT_TRUE(eqf(obb1.center, identityObb.center));
	ASSERT_TRUE(eqf(obb1.xAxis(), f32x3(0.0f, 0.0f, -1.0f)));
	ASSERT_TRUE(eqf(obb1.yAxis(), f32x3(1.0f, 0.0f, 0.0f)));
	ASSERT_TRUE(eqf(obb1.zAxis(), f32x3(0.0f, -1.0f, 0.0f)));

	mat4 scaleRot = rot3 * mat44::scaling3(4.0f, 5.0f, 6.0f);
	OBB obb2 = identityObb.transformOBB(mat34(scaleRot));
	ASSERT_TRUE(eqf(obb2.halfExtents, f32x3(2.0f, 2.5f, 3.0f), 0.01f));
	ASSERT_TRUE(eqf(obb2.center, identityObb.center));
	ASSERT_TRUE(eqf(obb2.xAxis(), f32x3(0.0f, 0.0f, -1.0f)));
	ASSERT_TRUE(eqf(obb2.yAxis(), f32x3(1.0f, 0.0f, 0.0f)));
	ASSERT_TRUE(eqf(obb2.zAxis(), f32x3(0.0f, -1.0f, 0.0f)));

	mat4 rotTranslScale = mat44::translation3(f32x3(1.0f, 2.0f, 3.0f)) * scaleRot;
	OBB obb3 = identityObb.transformOBB(mat34(rotTranslScale));
	ASSERT_TRUE(eqf(obb3.halfExtents, f32x3(2.0f, 2.5f, 3.0f), 0.01f));
	ASSERT_TRUE(eqf(obb3.center, f32x3(1.0f, 2.0f, 3.0f)));
	ASSERT_TRUE(eqf(obb3.xAxis(), f32x3(0.0f, 0.0f, -1.0f)));
	ASSERT_TRUE(eqf(obb3.yAxis(), f32x3(1.0f, 0.0f, 0.0f)));
	ASSERT_TRUE(eqf(obb3.zAxis(), f32x3(0.0f, -1.0f, 0.0f)));

	quat q = quat::fromRotationMatrix(mat34(rot3));
	OBB obb4 = identityObb.transformOBB(q);
	ASSERT_TRUE(eqf(obb4.halfExtents, identityObb.halfExtents));
	ASSERT_TRUE(eqf(obb4.center, identityObb.center));
	ASSERT_TRUE(eqf(obb4.xAxis(), f32x3(0.0f, 0.0f, -1.0f)));
	ASSERT_TRUE(eqf(obb4.yAxis(), f32x3(1.0f, 0.0f, 0.0f)));
	ASSERT_TRUE(eqf(obb4.zAxis(), f32x3(0.0f, -1.0f, 0.0f)));
}
