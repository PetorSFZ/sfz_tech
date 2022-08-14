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

#include <sfz.h>
#include <sfz_math.h>
#include <sfz_matrix.h>

using namespace sfz;

TEST_CASE("SfzMat33")
{
	// Individual element constructor
	{
		SfzMat33 m1 = sfzMat33InitElems(
			1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f,
			7.0f, 8.0f, 9.0f);
		CHECK(m1.at(0, 0) == 1.0f);
		CHECK(m1.at(0, 1) == 2.0f);
		CHECK(m1.at(0, 2) == 3.0f);
		CHECK(m1.at(1, 0) == 4.0f);
		CHECK(m1.at(1, 1) == 5.0f);
		CHECK(m1.at(1, 2) == 6.0f);
		CHECK(m1.at(2, 0) == 7.0f);
		CHECK(m1.at(2, 1) == 8.0f);
		CHECK(m1.at(2, 2) == 9.0f);
		CHECK(m1.rows[0] == f32x3_init(1.0f, 2.0f, 3.0f));
		CHECK(m1.rows[1] == f32x3_init(4.0f, 5.0f, 6.0f));
		CHECK(m1.rows[2] == f32x3_init(7.0f, 8.0f, 9.0f));
		CHECK(m1.column(0) == f32x3_init(1.0f, 4.0f, 7.0f));
		CHECK(m1.column(1) == f32x3_init(2.0f, 5.0f, 8.0f));
		CHECK(m1.column(2) == f32x3_init(3.0f, 6.0f, 9.0f));
	}
	// Row constructor
	{
		SfzMat33 m1 = sfzMat33InitRows(
			f32x3_init(1.0f, 2.0f, 3.0f),
			f32x3_init(4.0f, 5.0f, 6.0f),
			f32x3_init(7.0f, 8.0f, 9.0f));
		CHECK(m1.at(0, 0) == 1.0f);
		CHECK(m1.at(0, 1) == 2.0f);
		CHECK(m1.at(0, 2) == 3.0f);
		CHECK(m1.at(1, 0) == 4.0f);
		CHECK(m1.at(1, 1) == 5.0f);
		CHECK(m1.at(1, 2) == 6.0f);
		CHECK(m1.at(2, 0) == 7.0f);
		CHECK(m1.at(2, 1) == 8.0f);
		CHECK(m1.at(2, 2) == 9.0f);
		CHECK(m1.rows[0] == f32x3_init(1.0f, 2.0f, 3.0f));
		CHECK(m1.rows[1] == f32x3_init(4.0f, 5.0f, 6.0f));
		CHECK(m1.rows[2] == f32x3_init(7.0f, 8.0f, 9.0f));
		CHECK(m1.column(0) == f32x3_init(1.0f, 4.0f, 7.0f));
		CHECK(m1.column(1) == f32x3_init(2.0f, 5.0f, 8.0f));
		CHECK(m1.column(2) == f32x3_init(3.0f, 6.0f, 9.0f));
	}
	// 4x4 matrix constructor
	{
		const SfzMat44 m1 = sfzMat44InitElems(
			1.0f, 2.0f, 3.0f, 4.0f,
			5.0f, 6.0f, 7.0f, 8.0f,
			9.0f, 10.0f, 11.0f, 12.0f,
			13.0f, 14.0f, 15.0f, 16.0f);
		const SfzMat33 m2 = sfzMat33FromMat44(m1);
		CHECK(m2.at(0, 0) == 1.0f);
		CHECK(m2.at(0, 1) == 2.0f);
		CHECK(m2.at(0, 2) == 3.0f);
		CHECK(m2.at(1, 0) == 5.0f);
		CHECK(m2.at(1, 1) == 6.0f);
		CHECK(m2.at(1, 2) == 7.0f);
		CHECK(m2.at(2, 0) == 9.0f);
		CHECK(m2.at(2, 1) == 10.0f);
		CHECK(m2.at(2, 2) == 11.0f);
	}
	// identity() constructor function
	{
		SfzMat33 ident = sfzMat33Identity();
		CHECK(ident.at(0, 0) == 1.0f);
		CHECK(ident.at(0, 1) == 0.0f);
		CHECK(ident.at(0, 2) == 0.0f);
		CHECK(ident.at(1, 0) == 0.0f);
		CHECK(ident.at(1, 1) == 1.0f);
		CHECK(ident.at(1, 2) == 0.0f);
		CHECK(ident.at(2, 0) == 0.0f);
		CHECK(ident.at(2, 1) == 0.0f);
		CHECK(ident.at(2, 2) == 1.0f);
	}
	// scaling3() constructor function
	{
		const SfzMat33 scale = sfzMat33Scaling3(f32x3_splat(2.0f));
		CHECK(scale.at(0, 0) == 2.0f);
		CHECK(scale.at(0, 1) == 0.0f);
		CHECK(scale.at(0, 2) == 0.0f);
		CHECK(scale.at(1, 0) == 0.0f);
		CHECK(scale.at(1, 1) == 2.0f);
		CHECK(scale.at(1, 2) == 0.0f);
		CHECK(scale.at(2, 0) == 0.0f);
		CHECK(scale.at(2, 1) == 0.0f);
		CHECK(scale.at(2, 2) == 2.0f);

		const SfzMat33 scale2 = sfzMat33Scaling3(f32x3_init(1.0f, 2.0f, 3.0f));
		CHECK(scale2.at(0, 0) == 1.0f);
		CHECK(scale2.at(0, 1) == 0.0f);
		CHECK(scale2.at(0, 2) == 0.0f);
		CHECK(scale2.at(1, 0) == 0.0f);
		CHECK(scale2.at(1, 1) == 2.0f);
		CHECK(scale2.at(1, 2) == 0.0f);
		CHECK(scale2.at(2, 0) == 0.0f);
		CHECK(scale2.at(2, 1) == 0.0f);
		CHECK(scale2.at(2, 2) == 3.0f);
	}
	// rotation3() constructor function
	{
		f32x3 startPoint = f32x3_init(1.0f, 0.0f, 0.0f);
		f32x3 axis = f32x3_init(1.0f, 1.0f, 0.0f);
		SfzMat33 rot = sfzMat33Rotation3(axis, SFZ_PI);
		CHECK(eqf(rot * startPoint, f32x3_init(0.0f, 1.0, 0.0f)));

		SfzMat33 xRot90 = sfzMat33Rotation3(f32x3_init(1.0f, 0.0f, 0.0f), SFZ_PI/2.0f);
		CHECK(eqf(xRot90.rows[0], f32x3_init(1.0f, 0.0f, 0.0f)));
		CHECK(eqf(xRot90.rows[1], f32x3_init(0.0f, 0.0f, -1.0f)));
		CHECK(eqf(xRot90.rows[2], f32x3_init(0.0f, 1.0f, 0.0f)));

		f32x3 v = xRot90 * f32x3_splat(1.0f);
		CHECK(eqf(v, f32x3_init(1.0f, -1.0f, 1.0f)));
	}
}

TEST_CASE("SfzMat44")
{
	// Individual element constructor
	{
		const SfzMat44 m1 = sfzMat44InitElems(
			1.0f, 2.0f, 3.0f, 4.0f,
			5.0f, 6.0f, 7.0f, 8.0f,
			9.0f, 10.0f, 11.0f, 12.0f,
			13.0f, 14.0f, 15.0f, 16.0f);
		CHECK(m1.at(0, 0) == 1.0f);
		CHECK(m1.at(0, 1) == 2.0f);
		CHECK(m1.at(0, 2) == 3.0f);
		CHECK(m1.at(0, 3) == 4.0f);
		CHECK(m1.at(1, 0) == 5.0f);
		CHECK(m1.at(1, 1) == 6.0f);
		CHECK(m1.at(1, 2) == 7.0f);
		CHECK(m1.at(1, 3) == 8.0f);
		CHECK(m1.at(2, 0) == 9.0f);
		CHECK(m1.at(2, 1) == 10.0f);
		CHECK(m1.at(2, 2) == 11.0f);
		CHECK(m1.at(2, 3) == 12.0f);
		CHECK(m1.at(3, 0) == 13.0f);
		CHECK(m1.at(3, 1) == 14.0f);
		CHECK(m1.at(3, 2) == 15.0f);
		CHECK(m1.at(3, 3) == 16.0f);
		CHECK(m1.rows[0] == f32x4_init(1.0f, 2.0f, 3.0f, 4.0f));
		CHECK(m1.rows[1] == f32x4_init(5.0f, 6.0f, 7.0f, 8.0f));
		CHECK(m1.rows[2] == f32x4_init(9.0f, 10.0f, 11.0f, 12.0f));
		CHECK(m1.rows[3] == f32x4_init(13.0f, 14.0f, 15.0f, 16.0f));
		CHECK(m1.column(0) == f32x4_init(1.0f, 5.0f, 9.0f, 13.0f));
		CHECK(m1.column(1) == f32x4_init(2.0f, 6.0f, 10.0f, 14.0f));
		CHECK(m1.column(2) == f32x4_init(3.0f, 7.0f, 11.0f, 15.0f));
		CHECK(m1.column(3) == f32x4_init(4.0f, 8.0f, 12.0f, 16.0f));
	}
	// Row constructor
	{
		const SfzMat44 m1 = sfzMat44InitRows(
			f32x4_init(1.0f, 2.0f, 3.0f, 4.0f),
			f32x4_init(5.0f, 6.0f, 7.0f, 8.0f),
			f32x4_init(9.0f, 10.0f, 11.0f, 12.0f),
			f32x4_init(13.0f, 14.0f, 15.0f, 16.0f));
		CHECK(m1.at(0, 0) == 1.0f);
		CHECK(m1.at(0, 1) == 2.0f);
		CHECK(m1.at(0, 2) == 3.0f);
		CHECK(m1.at(0, 3) == 4.0f);
		CHECK(m1.at(1, 0) == 5.0f);
		CHECK(m1.at(1, 1) == 6.0f);
		CHECK(m1.at(1, 2) == 7.0f);
		CHECK(m1.at(1, 3) == 8.0f);
		CHECK(m1.at(2, 0) == 9.0f);
		CHECK(m1.at(2, 1) == 10.0f);
		CHECK(m1.at(2, 2) == 11.0f);
		CHECK(m1.at(2, 3) == 12.0f);
		CHECK(m1.at(3, 0) == 13.0f);
		CHECK(m1.at(3, 1) == 14.0f);
		CHECK(m1.at(3, 2) == 15.0f);
		CHECK(m1.at(3, 3) == 16.0f);
		CHECK(m1.rows[0] == f32x4_init(1.0f, 2.0f, 3.0f, 4.0f));
		CHECK(m1.rows[1] == f32x4_init(5.0f, 6.0f, 7.0f, 8.0f));
		CHECK(m1.rows[2] == f32x4_init(9.0f, 10.0f, 11.0f, 12.0f));
		CHECK(m1.rows[3] == f32x4_init(13.0f, 14.0f, 15.0f, 16.0f));
		CHECK(m1.column(0) == f32x4_init(1.0f, 5.0f, 9.0f, 13.0f));
		CHECK(m1.column(1) == f32x4_init(2.0f, 6.0f, 10.0f, 14.0f));
		CHECK(m1.column(2) == f32x4_init(3.0f, 7.0f, 11.0f, 15.0f));
		CHECK(m1.column(3) == f32x4_init(4.0f, 8.0f, 12.0f, 16.0f));
	}
	// 3x3 matrix constructor
	{
		const SfzMat33 m1 = sfzMat33InitElems(
			1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f,
			7.0f, 8.0f, 9.0f);
		const SfzMat44 m2 = sfzMat44FromMat33(m1);
		CHECK(m2.at(0, 0) == 1.0f);
		CHECK(m2.at(0, 1) == 2.0f);
		CHECK(m2.at(0, 2) == 3.0f);
		CHECK(m2.at(0, 3) == 0.0f);
		CHECK(m2.at(1, 0) == 4.0f);
		CHECK(m2.at(1, 1) == 5.0f);
		CHECK(m2.at(1, 2) == 6.0f);
		CHECK(m2.at(1, 3) == 0.0f);
		CHECK(m2.at(2, 0) == 7.0f);
		CHECK(m2.at(2, 1) == 8.0f);
		CHECK(m2.at(2, 2) == 9.0f);
		CHECK(m2.at(2, 3) == 0.0f);
		CHECK(m2.at(3, 0) == 0.0f);
		CHECK(m2.at(3, 1) == 0.0f);
		CHECK(m2.at(3, 2) == 0.0f);
		CHECK(m2.at(3, 3) == 1.0f);
	}
	// identity() constructor function
	{
		SfzMat44 ident = sfzMat44Identity();
		CHECK(ident.at(0, 0) == 1.0f);
		CHECK(ident.at(0, 1) == 0.0f);
		CHECK(ident.at(0, 2) == 0.0f);
		CHECK(ident.at(0, 3) == 0.0f);
		CHECK(ident.at(1, 0) == 0.0f);
		CHECK(ident.at(1, 1) == 1.0f);
		CHECK(ident.at(1, 2) == 0.0f);
		CHECK(ident.at(1, 3) == 0.0f);
		CHECK(ident.at(2, 0) == 0.0f);
		CHECK(ident.at(2, 1) == 0.0f);
		CHECK(ident.at(2, 2) == 1.0f);
		CHECK(ident.at(2, 3) == 0.0f);
		CHECK(ident.at(3, 0) == 0.0f);
		CHECK(ident.at(3, 1) == 0.0f);
		CHECK(ident.at(3, 2) == 0.0f);
		CHECK(ident.at(3, 3) == 1.0f);
	}
	// scaling3() constructor function
	{
		SfzMat44 scale = sfzMat44Scaling3(f32x3_splat(2.0f));
		CHECK(scale.at(0, 0) == 2.0f);
		CHECK(scale.at(0, 1) == 0.0f);
		CHECK(scale.at(0, 2) == 0.0f);
		CHECK(scale.at(0, 3) == 0.0f);
		CHECK(scale.at(1, 0) == 0.0f);
		CHECK(scale.at(1, 1) == 2.0f);
		CHECK(scale.at(1, 2) == 0.0f);
		CHECK(scale.at(1, 3) == 0.0f);
		CHECK(scale.at(2, 0) == 0.0f);
		CHECK(scale.at(2, 1) == 0.0f);
		CHECK(scale.at(2, 2) == 2.0f);
		CHECK(scale.at(2, 3) == 0.0f);
		CHECK(scale.at(3, 0) == 0.0f);
		CHECK(scale.at(3, 1) == 0.0f);
		CHECK(scale.at(3, 2) == 0.0f);
		CHECK(scale.at(3, 3) == 1.0f);

		SfzMat44 scale2 = sfzMat44Scaling3(f32x3_init(1.0f, 2.0f, 3.0f));
		CHECK(scale2.at(0, 0) == 1.0f);
		CHECK(scale2.at(0, 1) == 0.0f);
		CHECK(scale2.at(0, 2) == 0.0f);
		CHECK(scale2.at(0, 3) == 0.0f);
		CHECK(scale2.at(1, 0) == 0.0f);
		CHECK(scale2.at(1, 1) == 2.0f);
		CHECK(scale2.at(1, 2) == 0.0f);
		CHECK(scale2.at(1, 3) == 0.0f);
		CHECK(scale2.at(2, 0) == 0.0f);
		CHECK(scale2.at(2, 1) == 0.0f);
		CHECK(scale2.at(2, 2) == 3.0f);
		CHECK(scale2.at(2, 3) == 0.0f);
		CHECK(scale2.at(3, 0) == 0.0f);
		CHECK(scale2.at(3, 1) == 0.0f);
		CHECK(scale2.at(3, 2) == 0.0f);
		CHECK(scale2.at(3, 3) == 1.0f);
	}
	// rotation3() constructor function
	{
		f32x4 startPoint = f32x4_init(1.0f, 0.0f, 0.0f, 1.0f);
		f32x3 axis = f32x3_init(1.0f, 1.0f, 0.0f);
		SfzMat44 rot = sfzMat44Rotation3(axis, SFZ_PI);
		CHECK(eqf(rot * startPoint, f32x4_init(0.0f, 1.0, 0.0f, 1.0f)));

		SfzMat44 xRot90 = sfzMat44Rotation3(f32x3_init(1.0f, 0.0f, 0.0f), SFZ_PI/2.0f);
		CHECK(eqf(xRot90.rows[0], f32x4_init(1.0f, 0.0f, 0.0f, 0.0f)));
		CHECK(eqf(xRot90.rows[1], f32x4_init(0.0f, 0.0f, -1.0f, 0.0f)));
		CHECK(eqf(xRot90.rows[2], f32x4_init(0.0f, 1.0f, 0.0f, 0.0f)));
		CHECK(eqf(xRot90.rows[3], f32x4_init(0.0f, 0.0f, 0.0f, 1.0f)));

		f32x4 v = xRot90 * f32x4_splat(1.0f);
		CHECK(eqf(v, f32x4_init(1.0f, -1.0f, 1.0f, 1.0f)));
	}
	// translation3() constructor function
	{
		f32x4 v1 = f32x4_init(1.0f, 1.0f, 1.0f, 1.0f);
		SfzMat44 m = sfzMat44Translation3(f32x3_init(-2.0f, 1.0f, 0.0f));
		CHECK(eqf(m.at(0, 0), 1.0f));
		CHECK(eqf(m.at(0, 1), 0.0f));
		CHECK(eqf(m.at(0, 2), 0.0f));
		CHECK(eqf(m.at(0, 3), -2.0f));
		CHECK(eqf(m.at(1, 0), 0.0f));
		CHECK(eqf(m.at(1, 1), 1.0f));
		CHECK(eqf(m.at(1, 2), 0.0f));
		CHECK(eqf(m.at(1, 3), 1.0f));
		CHECK(eqf(m.at(2, 0), 0.0f));
		CHECK(eqf(m.at(2, 1), 0.0f));
		CHECK(eqf(m.at(2, 2), 1.0f));
		CHECK(eqf(m.at(2, 3), 0.0f));
		CHECK(eqf(m.at(3, 0), 0.0f));
		CHECK(eqf(m.at(3, 1), 0.0f));
		CHECK(eqf(m.at(3, 2), 0.0f));
		CHECK(eqf(m.at(3, 3), 1.0f));
		f32x4 v2 = m * v1;
		CHECK(eqf(v2.x, -1.0f));
		CHECK(eqf(v2.y, 2.0f));
		CHECK(eqf(v2.z, 1.0f));
		CHECK(eqf(v2.w, 1.0f));
	}
}

TEST_CASE("Matrix: arithmetic_assignment_operators")
{
	// +=
	{
		SfzMat44 m1 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m2 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m3 = sfzMat44InitElems(
			-2.0f, -1.0f, 0.0f, 0.0f,
			3.0f, 33.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, 
			0.0f, 0.0f, 0.0f, 0.0f);

		m1 += m2;
		m2 += m3;

		CHECK(eqf(m1.at(0, 0), 2.0f));
		CHECK(eqf(m1.at(0, 1), 4.0f));
		CHECK(eqf(m1.at(1, 0), 6.0f));
		CHECK(eqf(m1.at(1, 1), 8.0f));

		CHECK(eqf(m2.at(0, 0), -1.0f));
		CHECK(eqf(m2.at(0, 1), 1.0f));
		CHECK(eqf(m2.at(1, 0), 6.0f));
		CHECK(eqf(m2.at(1, 1), 37.0f));
	}
	// -=
	{
		SfzMat44 m1 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m2 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m3 = sfzMat44InitElems(
			-2.0f, -1.0f, 0.0f, 0.0f,
			3.0f, 33.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);

		m1 -= m2;
		m2 -= m3;

		CHECK(eqf(m1.at(0, 0), 0.0f));
		CHECK(eqf(m1.at(0, 1), 0.0f));
		CHECK(eqf(m1.at(1, 0), 0.0f));
		CHECK(eqf(m1.at(1, 1), 0.0f));

		CHECK(eqf(m2.at(0, 0), 3.0f));
		CHECK(eqf(m2.at(0, 1), 3.0f));
		CHECK(eqf(m2.at(1, 0), 0.0f));
		CHECK(eqf(m2.at(1, 1), -29.0f));
	}
	// *= (scalar)
	{
		SfzMat44 m1 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m2 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m3 = sfzMat44InitElems(
			-2.0f, -1.0f, 0.0f, 0.0f,
			3.0f, 33.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);

		m1 *= 2.0f;
		CHECK(eqf(m1.at(0, 0), 2.0f));
		CHECK(eqf(m1.at(0, 1), 4.0f));
		CHECK(eqf(m1.at(1, 0), 6.0f));
		CHECK(eqf(m1.at(1, 1), 8.0f));

		m3 *= -1.0f;
		CHECK(eqf(m3.at(0, 0), 2.0f));
		CHECK(eqf(m3.at(0, 1), 1.0f));
		CHECK(eqf(m3.at(1, 0), -3.0f));
		CHECK(eqf(m3.at(1, 1), -33.0f));
	}
	// *= (matrix of same size)
	/*{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m3(-2.0f, -1.0f,
			3.0f, 33.0f);

		mat22 m4(1.0f, 0.0f,
		         0.0f, 1.0f);
		auto m1cpy = m1;
		m1cpy *= m4;

		CHECK(eqf(m1cpy.at(0, 0), 1.0f));
		CHECK(eqf(m1cpy.at(0, 1), 2.0f));
		CHECK(eqf(m1cpy.at(1, 0), 3.0f));
		CHECK(eqf(m1cpy.at(1, 1), 4.0f));

		m4 *= m1;
		CHECK(eqf(m4.at(0, 0), 1.0f));
		CHECK(eqf(m4.at(0, 1), 2.0f));
		CHECK(eqf(m4.at(1, 0), 3.0f));
		CHECK(eqf(m4.at(1, 1), 4.0f));
	}*/
}

TEST_CASE("Matrix: arithmetic_operators")
{
	// +
	{
		SfzMat44 m1 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m2 = sfzMat44InitElems(
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m3 = sfzMat44InitElems(
			1.0f, 2.0f, 3.0f, 0.0f,
			4.0f, 5.0f, 6.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m4 = sfzMat44InitElems(
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);

		SfzMat44 res1 = m1 + m2;
		CHECK(eqf(res1.at(0, 0), 1.0f));
		CHECK(eqf(res1.at(0, 1), 3.0f));
		CHECK(eqf(res1.at(1, 0), 3.0f));
		CHECK(eqf(res1.at(1, 1), 4.0f));

		SfzMat44 res2 = m3 + m3;
		CHECK(eqf(res2.at(0, 0), 2.0f));
		CHECK(eqf(res2.at(0, 1), 4.0f));
		CHECK(eqf(res2.at(0, 2), 6.0f));
		CHECK(eqf(res2.at(1, 0), 8.0f));
		CHECK(eqf(res2.at(1, 1), 10.0f));
		CHECK(eqf(res2.at(1, 2), 12.0f));
	}
	// -
	{
		SfzMat44 m1 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m2 = sfzMat44InitElems(
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m3 = sfzMat44InitElems(
			1.0f, 2.0f, 3.0f, 0.0f,
			4.0f, 5.0f, 6.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m4 = sfzMat44InitElems(
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);

		SfzMat44 res1 = m1 - m2;
		SfzMat44 res2 = m2 - m1;

		CHECK(eqf(res1.at(0, 0), 1.0f));
		CHECK(eqf(res1.at(0, 1), 1.0f));
		CHECK(eqf(res1.at(1, 0), 3.0f));
		CHECK(eqf(res1.at(1, 1), 4.0f));

		CHECK(eqf(res2.at(0, 0), -1.0f));
		CHECK(eqf(res2.at(0, 1), -1.0f));
		CHECK(eqf(res2.at(1, 0), -3.0f));
		CHECK(eqf(res2.at(1, 1), -4.0f));
	}
	// - (negation)
	{
		SfzMat44 m1 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m2 = sfzMat44InitElems(
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m3 = sfzMat44InitElems(
			1.0f, 2.0f, 3.0f, 0.0f,
			4.0f, 5.0f, 6.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m4 = sfzMat44InitElems(
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);

		SfzMat44 res1 = -m1;

		CHECK(eqf(res1.at(0, 0), -1.0f));
		CHECK(eqf(res1.at(0, 1), -2.0f));
		CHECK(eqf(res1.at(1, 0), -3.0f));
		CHECK(eqf(res1.at(1, 1), -4.0f));
	}
	// * (matrix)
	/*{
		mat44 m1(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		mat44 m2(
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		mat44 m3(
			1.0f, 2.0f, 3.0f, 0.0f,
			4.0f, 5.0f, 6.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		mat44 m4(
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);

		auto res1 = m1*m2;
		CHECK(eqf(res1.at(0, 0), 0.0f));
		CHECK(eqf(res1.at(0, 1), 1.0f));
		CHECK(eqf(res1.at(1, 0), 0.0f));
		CHECK(eqf(res1.at(1, 1), 3.0f));

		auto res2 = m2*m1;
		CHECK(eqf(res2.at(0, 0), 3.0f));
		CHECK(eqf(res2.at(0, 1), 4.0f));
		CHECK(eqf(res2.at(1, 0), 0.0f));
		CHECK(eqf(res2.at(1, 1), 0.0f));

		auto res3 = m3*m4;
		CHECK(eqf(res3.at(0, 0), 1.0f));
		CHECK(eqf(res3.at(0, 1), 2.0f));
		CHECK(eqf(res3.at(1, 0), 4.0f));
		CHECK(eqf(res3.at(1, 1), 5.0f));
	}
	// * (vector)
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(0.0f, 1.0f,
			0.0f, 0.0f);
		f32 m3arr[] = { 1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f };
		Mat<2, 3> m3(m3arr);
		f32 m4arr[] = { 1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f };
		Mat<3, 2> m4(m4arr);

		f32x2 v1(1.0f, -2.0f);

		f32x2 res1 = m1 * v1;
		CHECK(eqf(res1.x, -3.0f));
		CHECK(eqf(res1.y, -5.0f));

		f32x3 res2 = m4 * v1;
		CHECK(eqf(res2.x, 1.0f));
		CHECK(eqf(res2.y, -2.0f));
		CHECK(eqf(res2.z, 0.0f));

		mat34 m5(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f);
		CHECK(eqf(m5 * f32x4(1.0f), f32x3(10.0f, 26.0f, 42.0f)));
	}*/
	// * (scalar)
	{
		SfzMat44 m1 = sfzMat44InitElems(
			1.0f, 2.0f, 0.0f, 0.0f,
			3.0f, 4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
		SfzMat44 m2 = sfzMat44InitElems(
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);

		SfzMat44 res1 = m1 * 2.0f;
		CHECK(eqf(res1.at(0, 0), 2.0f));
		CHECK(eqf(res1.at(0, 1), 4.0f));
		CHECK(eqf(res1.at(1, 0), 6.0f));
		CHECK(eqf(res1.at(1, 1), 8.0f));

		SfzMat44 res2 = -1.0f * m2;
		CHECK(eqf(res2.at(0, 0), 0.0f));
		CHECK(eqf(res2.at(0, 1), -1.0f));
		CHECK(eqf(res2.at(1, 0), 0.0f));
		CHECK(eqf(res2.at(1, 1), 0.0f));
	}
}

TEST_CASE("Matrix: transpose")
{
	SfzMat44 m = sfzMat44InitElems(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	SfzMat44 mTransp = sfzMat44Transpose(m);
	CHECK(eqf(mTransp.rows[0], f32x4_init(1.0f, 5.0f, 9.0f, 13.0f)));
	CHECK(eqf(mTransp.rows[1], f32x4_init(2.0f, 6.0f, 10.0f, 14.0f)));
	CHECK(eqf(mTransp.rows[2], f32x4_init(3.0f, 7.0f, 11.0f, 15.0f)));
	CHECK(eqf(mTransp.rows[3], f32x4_init(4.0f, 8.0f, 12.0f, 16.0f)));
}

TEST_CASE("Matrix: transforming_3d_vector")
{
	// transformPoint() 4x4
	{
		SfzMat44 m = sfzMat44InitElems(
			2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		f32x3 v = f32x3_init(1.0f, 1.0f, 1.0f);
		f32x3 v2 = sfzMat44TransformPoint(m, v);
		CHECK(eqf(v2.x, 3.0f));
		CHECK(eqf(v2.y, 2.0f));
		CHECK(eqf(v2.z, 2.0f));
	}

	// transformDir() 4x4
	{
		SfzMat44 m = sfzMat44InitElems(
			2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		f32x3 v = f32x3_init(1.0f, 1.0f, 1.0f);

		f32x3 v2 = sfzMat44TransformDir(m, v);
		CHECK(eqf(v2.x, 2.0f));
		CHECK(eqf(v2.y, 2.0f));
		CHECK(eqf(v2.z, 2.0f));
	}
}

TEST_CASE("Matrix: determinants")
{
	SfzMat33 m2 = sfzMat33InitElems(
		-1.0f, 1.0f, 0.0f,
		3.0f, 5.0f, 1.0f,
		7.0f, 8.0f, 9.0f);
	CHECK(eqf(sfzMat33Determinant(m2), -57.0f));

	SfzMat33 m3 = sfzMat33InitElems(
		99.0f, -2.0f, 5.0f,
		8.0f, -4.0f, -1.0f,
		6.0f, 1.0f, -88.0f);
	CHECK(eqf(sfzMat33Determinant(m3), 33711.0f));

	SfzMat44 m4 = sfzMat44InitElems(
		1.0f, -2.0f, 1.0f, 3.0f,
		1.0f, 4.0f, -5.0f, 0.0f,
		-10.0f, 0.0f, 4.0f, 2.0f,
		-1.0f, 0.0f, 2.0f, 0.0f);
	CHECK(eqf(sfzMat44Determinant(m4), -204.0f));
}

TEST_CASE("Matrix: inverse")
{
	SfzMat33 m3 = sfzMat33InitElems(
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 2.0f,
		1.0f, 2.0f, 3.0f);
	SfzMat33 m3Inv = sfzMat33InitElems(
		1.0f, 1.0f, -1.0f,
		1.0f, -2.0f, 1.0f,
		-1.0f, 1.0f, 0.0f);
	SfzMat33 m3CalcInv = sfzMat33Inverse(m3);
	CHECK(eqf(m3CalcInv.rows[0], m3Inv.rows[0]));
	CHECK(eqf(m3CalcInv.rows[1], m3Inv.rows[1]));
	CHECK(eqf(m3CalcInv.rows[2], m3Inv.rows[2]));

	SfzMat44 m5 = sfzMat44InitElems(
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 2.0f, 3.0f,
		1.0f, 2.0f, 3.0f, 4.0f,
		1.0f, 2.0f, 2.0f, 1.0f);
	SfzMat44 m5Inv = sfzMat44InitElems(
		1.0f, 1.0f, -1.0f, 0.0f,
		2.0f, -3.0f, 2.0f, -1.0f,
		-3.0f, 3.0f, -2.0f, 2.0f,
		1.0f, -1.0f, 1.0f, -1.0f);
	SfzMat44 m5CalcInv = sfzMat44Inverse(m5);
	CHECK(eqf(m5CalcInv.rows[0], m5Inv.rows[0]));
	CHECK(eqf(m5CalcInv.rows[1], m5Inv.rows[1]));
	CHECK(eqf(m5CalcInv.rows[2], m5Inv.rows[2]));
	CHECK(eqf(m5CalcInv.rows[3], m5Inv.rows[3]));
}
