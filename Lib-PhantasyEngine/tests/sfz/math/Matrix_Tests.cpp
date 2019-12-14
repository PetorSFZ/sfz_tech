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

#include <skipifzero.hpp>

#include "sfz/math/MathPrimitiveToStrings.hpp"
#include "sfz/math/MathSupport.hpp"
#include "sfz/math/Matrix.hpp"
#include "sfz/math/ProjectionMatrices.hpp"

#include <type_traits>

using namespace sfz;

UTEST(Matrix, matrix_general_definition)
{
	// Array pointer constructor
	{
		const float arr1[] = {1.0f, 2.0f, 3.0f, 4.0f};
		Matrix<float,1,4> m1(arr1);
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);

		Matrix<float,4,1> m2(arr1);
		ASSERT_TRUE(m2.at(0, 0) == 1.0f);
		ASSERT_TRUE(m2.at(1, 0) == 2.0f);
		ASSERT_TRUE(m2.at(2, 0) == 3.0f);
		ASSERT_TRUE(m2.at(3, 0) == 4.0f);
		ASSERT_TRUE(m2.columnAt(0) == vec4(1.0f, 2.0f, 3.0f, 4.0f));

		const float arr2[] = {6.0f, 5.0f, 4.0f,
		                      3.0f, 2.0f, 1.0f};
		Matrix<float,2,3> m3(arr2);
		ASSERT_TRUE(m3.at(0, 0) == 6.0f);
		ASSERT_TRUE(m3.at(0, 1) == 5.0f);
		ASSERT_TRUE(m3.at(0, 2) == 4.0f);
		ASSERT_TRUE(m3.at(1, 0) == 3.0f);
		ASSERT_TRUE(m3.at(1, 1) == 2.0f);
		ASSERT_TRUE(m3.at(1, 2) == 1.0f);
		ASSERT_TRUE(m3.columnAt(0) == vec2(6.0f, 3.0f));
		ASSERT_TRUE(m3.columnAt(1) == vec2(5.0f, 2.0f));
		ASSERT_TRUE(m3.columnAt(2) == vec2(4.0f, 1.0f));
	}
}

UTEST(Matrix, matrix_2x2_specialization)
{
	// Array pointer constructor
	{
		const float arr1[] = {1.0f, 2.0f,
		                      3.0f, 4.0f};
		mat22 m1(arr1);
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e10 == 3.0f);
		ASSERT_TRUE(m1.e11 == 4.0f);
		ASSERT_TRUE(m1.rows[0] == vec2(1.0f, 2.0f));
		ASSERT_TRUE(m1.rows[1] == vec2(3.0f, 4.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec2(1.0f, 3.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec2(2.0f, 4.0f));
	}
	// Individual element constructor
	{
		mat22 m1(1.0f, 2.0f,
		         3.0f, 4.0f);
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e10 == 3.0f);
		ASSERT_TRUE(m1.e11 == 4.0f);
		ASSERT_TRUE(m1.rows[0] == vec2(1.0f, 2.0f));
		ASSERT_TRUE(m1.rows[1] == vec2(3.0f, 4.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec2(1.0f, 3.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec2(2.0f, 4.0f));
	}
	// Row constructor
	{
		mat22 m1(vec2(1.0f, 2.0f),
		         vec2(3.0f, 4.0f));
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e10 == 3.0f);
		ASSERT_TRUE(m1.e11 == 4.0f);
		ASSERT_TRUE(m1.rows[0] == vec2(1.0f, 2.0f));
		ASSERT_TRUE(m1.rows[1] == vec2(3.0f, 4.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec2(1.0f, 3.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec2(2.0f, 4.0f));
	}
	// fill() constructor function
	{
		mat22 zero = mat22::fill(0.0f);
		ASSERT_TRUE(zero.at(0, 0) == 0.0f);
		ASSERT_TRUE(zero.at(0, 1) == 0.0f);
		ASSERT_TRUE(zero.at(0, 2) == 0.0f);
		ASSERT_TRUE(zero.at(0, 3) == 0.0f);

		mat22 one = mat22::fill(1.0f);
		ASSERT_TRUE(one.at(0, 0) == 1.0f);
		ASSERT_TRUE(one.at(0, 1) == 1.0f);
		ASSERT_TRUE(one.at(0, 2) == 1.0f);
		ASSERT_TRUE(one.at(0, 3) == 1.0f);
	}
	// identity() constructor function
	{
		mat22 ident = mat22::identity();
		ASSERT_TRUE(ident.at(0, 0) == 1.0f);
		ASSERT_TRUE(ident.at(0, 1) == 0.0f);
		ASSERT_TRUE(ident.at(0, 2) == 0.0f);
		ASSERT_TRUE(ident.at(0, 3) == 1.0f);
	}
	// scaling2() constructor function
	{
		mat22 scale = mat22::scaling2(2.0f);
		ASSERT_TRUE(scale.at(0, 0) == 2.0f);
		ASSERT_TRUE(scale.at(0, 1) == 0.0f);
		ASSERT_TRUE(scale.at(0, 2) == 0.0f);
		ASSERT_TRUE(scale.at(0, 3) == 2.0f);

		mat22 scale2 = mat22::scaling2(vec2(1.0f, 2.0f));
		ASSERT_TRUE(scale2.at(0, 0) == 1.0f);
		ASSERT_TRUE(scale2.at(0, 1) == 0.0f);
		ASSERT_TRUE(scale2.at(0, 2) == 0.0f);
		ASSERT_TRUE(scale2.at(0, 3) == 2.0f);
	}
}

UTEST(Matrix, matrix_3x3_specialization)
{
	// Array pointer constructor
	{
		const float arr1[] = {1.0f, 2.0f, 3.0f,
		                      4.0f, 5.0f, 6.0f,
		                      7.0f, 8.0f, 9.0f};
		mat33 m1(arr1);
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(1, 0) == 4.0f);
		ASSERT_TRUE(m1.at(1, 1) == 5.0f);
		ASSERT_TRUE(m1.at(1, 2) == 6.0f);
		ASSERT_TRUE(m1.at(2, 0) == 7.0f);
		ASSERT_TRUE(m1.at(2, 1) == 8.0f);
		ASSERT_TRUE(m1.at(2, 2) == 9.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e02 == 3.0f);
		ASSERT_TRUE(m1.e10 == 4.0f);
		ASSERT_TRUE(m1.e11 == 5.0f);
		ASSERT_TRUE(m1.e12 == 6.0f);
		ASSERT_TRUE(m1.e20 == 7.0f);
		ASSERT_TRUE(m1.e21 == 8.0f);
		ASSERT_TRUE(m1.e22 == 9.0f);
		ASSERT_TRUE(m1.rows[0] == vec3(1.0f, 2.0f, 3.0f));
		ASSERT_TRUE(m1.rows[1] == vec3(4.0f, 5.0f, 6.0f));
		ASSERT_TRUE(m1.rows[2] == vec3(7.0f, 8.0f, 9.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec3(1.0f, 4.0f, 7.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec3(2.0f, 5.0f, 8.0f));
		ASSERT_TRUE(m1.columnAt(2) == vec3(3.0f, 6.0f, 9.0f));
	}
	// Individual element constructor
	{
		mat33 m1(1.0f, 2.0f, 3.0f,
		         4.0f, 5.0f, 6.0f,
		         7.0f, 8.0f, 9.0f);
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(1, 0) == 4.0f);
		ASSERT_TRUE(m1.at(1, 1) == 5.0f);
		ASSERT_TRUE(m1.at(1, 2) == 6.0f);
		ASSERT_TRUE(m1.at(2, 0) == 7.0f);
		ASSERT_TRUE(m1.at(2, 1) == 8.0f);
		ASSERT_TRUE(m1.at(2, 2) == 9.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e02 == 3.0f);
		ASSERT_TRUE(m1.e10 == 4.0f);
		ASSERT_TRUE(m1.e11 == 5.0f);
		ASSERT_TRUE(m1.e12 == 6.0f);
		ASSERT_TRUE(m1.e20 == 7.0f);
		ASSERT_TRUE(m1.e21 == 8.0f);
		ASSERT_TRUE(m1.e22 == 9.0f);
		ASSERT_TRUE(m1.rows[0] == vec3(1.0f, 2.0f, 3.0f));
		ASSERT_TRUE(m1.rows[1] == vec3(4.0f, 5.0f, 6.0f));
		ASSERT_TRUE(m1.rows[2] == vec3(7.0f, 8.0f, 9.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec3(1.0f, 4.0f, 7.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec3(2.0f, 5.0f, 8.0f));
		ASSERT_TRUE(m1.columnAt(2) == vec3(3.0f, 6.0f, 9.0f));
	}
	// Row constructor
	{
		mat33 m1(vec3(1.0f, 2.0f, 3.0f),
		         vec3(4.0f, 5.0f, 6.0f),
		         vec3(7.0f, 8.0f, 9.0f));
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(1, 0) == 4.0f);
		ASSERT_TRUE(m1.at(1, 1) == 5.0f);
		ASSERT_TRUE(m1.at(1, 2) == 6.0f);
		ASSERT_TRUE(m1.at(2, 0) == 7.0f);
		ASSERT_TRUE(m1.at(2, 1) == 8.0f);
		ASSERT_TRUE(m1.at(2, 2) == 9.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e02 == 3.0f);
		ASSERT_TRUE(m1.e10 == 4.0f);
		ASSERT_TRUE(m1.e11 == 5.0f);
		ASSERT_TRUE(m1.e12 == 6.0f);
		ASSERT_TRUE(m1.e20 == 7.0f);
		ASSERT_TRUE(m1.e21 == 8.0f);
		ASSERT_TRUE(m1.e22 == 9.0f);
		ASSERT_TRUE(m1.rows[0] == vec3(1.0f, 2.0f, 3.0f));
		ASSERT_TRUE(m1.rows[1] == vec3(4.0f, 5.0f, 6.0f));
		ASSERT_TRUE(m1.rows[2] == vec3(7.0f, 8.0f, 9.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec3(1.0f, 4.0f, 7.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec3(2.0f, 5.0f, 8.0f));
		ASSERT_TRUE(m1.columnAt(2) == vec3(3.0f, 6.0f, 9.0f));
	}
	// 3x4 matrix constructor
	{
		mat34 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f);
		mat33 m2(m1);
		ASSERT_TRUE(m2.at(0, 0) == 1.0f);
		ASSERT_TRUE(m2.at(0, 1) == 2.0f);
		ASSERT_TRUE(m2.at(0, 2) == 3.0f);
		ASSERT_TRUE(m2.at(1, 0) == 5.0f);
		ASSERT_TRUE(m2.at(1, 1) == 6.0f);
		ASSERT_TRUE(m2.at(1, 2) == 7.0f);
		ASSERT_TRUE(m2.at(2, 0) == 9.0f);
		ASSERT_TRUE(m2.at(2, 1) == 10.0f);
		ASSERT_TRUE(m2.at(2, 2) == 11.0f);
	}
	// 4x4 matrix constructor
	{
		mat44 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f,
		         13.0f, 14.0f, 15.0f, 16.0f);
		mat33 m2(m1);
		ASSERT_TRUE(m2.at(0, 0) == 1.0f);
		ASSERT_TRUE(m2.at(0, 1) == 2.0f);
		ASSERT_TRUE(m2.at(0, 2) == 3.0f);
		ASSERT_TRUE(m2.at(1, 0) == 5.0f);
		ASSERT_TRUE(m2.at(1, 1) == 6.0f);
		ASSERT_TRUE(m2.at(1, 2) == 7.0f);
		ASSERT_TRUE(m2.at(2, 0) == 9.0f);
		ASSERT_TRUE(m2.at(2, 1) == 10.0f);
		ASSERT_TRUE(m2.at(2, 2) == 11.0f);
	}
	// fill() constructor function
	{
		mat33 zero = mat33::fill(0.0f);
		ASSERT_TRUE(zero.at(0, 0) == 0.0f);
		ASSERT_TRUE(zero.at(0, 1) == 0.0f);
		ASSERT_TRUE(zero.at(0, 2) == 0.0f);
		ASSERT_TRUE(zero.at(1, 0) == 0.0f);
		ASSERT_TRUE(zero.at(1, 1) == 0.0f);
		ASSERT_TRUE(zero.at(1, 2) == 0.0f);
		ASSERT_TRUE(zero.at(2, 0) == 0.0f);
		ASSERT_TRUE(zero.at(2, 1) == 0.0f);
		ASSERT_TRUE(zero.at(2, 2) == 0.0f);

		mat33 one = mat33::fill(1.0f);
		ASSERT_TRUE(one.at(0, 0) == 1.0f);
		ASSERT_TRUE(one.at(0, 1) == 1.0f);
		ASSERT_TRUE(one.at(0, 2) == 1.0f);
		ASSERT_TRUE(one.at(1, 0) == 1.0f);
		ASSERT_TRUE(one.at(1, 1) == 1.0f);
		ASSERT_TRUE(one.at(1, 2) == 1.0f);
		ASSERT_TRUE(one.at(2, 0) == 1.0f);
		ASSERT_TRUE(one.at(2, 1) == 1.0f);
		ASSERT_TRUE(one.at(2, 2) == 1.0f);
	}
	// identity() constructor function
	{
		mat33 ident = mat33::identity();
		ASSERT_TRUE(ident.at(0, 0) == 1.0f);
		ASSERT_TRUE(ident.at(0, 1) == 0.0f);
		ASSERT_TRUE(ident.at(0, 2) == 0.0f);
		ASSERT_TRUE(ident.at(1, 0) == 0.0f);
		ASSERT_TRUE(ident.at(1, 1) == 1.0f);
		ASSERT_TRUE(ident.at(1, 2) == 0.0f);
		ASSERT_TRUE(ident.at(2, 0) == 0.0f);
		ASSERT_TRUE(ident.at(2, 1) == 0.0f);
		ASSERT_TRUE(ident.at(2, 2) == 1.0f);
	}
	// scaling3() constructor function
	{
		mat33 scale = mat33::scaling3(2.0f);
		ASSERT_TRUE(scale.at(0, 0) == 2.0f);
		ASSERT_TRUE(scale.at(0, 1) == 0.0f);
		ASSERT_TRUE(scale.at(0, 2) == 0.0f);
		ASSERT_TRUE(scale.at(1, 0) == 0.0f);
		ASSERT_TRUE(scale.at(1, 1) == 2.0f);
		ASSERT_TRUE(scale.at(1, 2) == 0.0f);
		ASSERT_TRUE(scale.at(2, 0) == 0.0f);
		ASSERT_TRUE(scale.at(2, 1) == 0.0f);
		ASSERT_TRUE(scale.at(2, 2) == 2.0f);

		mat33 scale2 = mat33::scaling3(vec3(1.0f, 2.0f, 3.0f));
		ASSERT_TRUE(scale2.at(0, 0) == 1.0f);
		ASSERT_TRUE(scale2.at(0, 1) == 0.0f);
		ASSERT_TRUE(scale2.at(0, 2) == 0.0f);
		ASSERT_TRUE(scale2.at(1, 0) == 0.0f);
		ASSERT_TRUE(scale2.at(1, 1) == 2.0f);
		ASSERT_TRUE(scale2.at(1, 2) == 0.0f);
		ASSERT_TRUE(scale2.at(2, 0) == 0.0f);
		ASSERT_TRUE(scale2.at(2, 1) == 0.0f);
		ASSERT_TRUE(scale2.at(2, 2) == 3.0f);
	}
	// rotation3() constructor function
	{
		vec3 startPoint(1.0f, 0.0f, 0.0f);
		vec3 axis = vec3(1.0f, 1.0f, 0.0f);
		mat33 rot = mat33::rotation3(axis, PI);
		ASSERT_TRUE(eqf(rot * startPoint, vec3(0.0f, 1.0, 0.0f)));

		mat33 xRot90 = mat33::rotation3(vec3(1.0f, 0.0f, 0.0f), PI/2.0f);
		ASSERT_TRUE(eqf(xRot90.row0, vec3(1.0f, 0.0f, 0.0f)));
		ASSERT_TRUE(eqf(xRot90.row1, vec3(0.0f, 0.0f, -1.0f)));
		ASSERT_TRUE(eqf(xRot90.row2, vec3(0.0f, 1.0f, 0.0f)));

		vec3 v = xRot90 * vec3(1.0f);
		ASSERT_TRUE(eqf(v, vec3(1.0f, -1.0f, 1.0f)));
	}
}

UTEST(Matrix, matrix_3x4_specialization)
{
	// Array pointer constructor
	{
		const float arr1[] = {1.0f, 2.0f, 3.0f, 4.0f,
		                      5.0f, 6.0f, 7.0f, 8.0f,
		                      9.0f, 10.0f, 11.0f, 12.0f};
		mat34 m1(arr1);
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);
		ASSERT_TRUE(m1.at(1, 0) == 5.0f);
		ASSERT_TRUE(m1.at(1, 1) == 6.0f);
		ASSERT_TRUE(m1.at(1, 2) == 7.0f);
		ASSERT_TRUE(m1.at(1, 3) == 8.0f);
		ASSERT_TRUE(m1.at(2, 0) == 9.0f);
		ASSERT_TRUE(m1.at(2, 1) == 10.0f);
		ASSERT_TRUE(m1.at(2, 2) == 11.0f);
		ASSERT_TRUE(m1.at(2, 3) == 12.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e02 == 3.0f);
		ASSERT_TRUE(m1.e03 == 4.0f);
		ASSERT_TRUE(m1.e10 == 5.0f);
		ASSERT_TRUE(m1.e11 == 6.0f);
		ASSERT_TRUE(m1.e12 == 7.0f);
		ASSERT_TRUE(m1.e13 == 8.0f);
		ASSERT_TRUE(m1.e20 == 9.0f);
		ASSERT_TRUE(m1.e21 == 10.0f);
		ASSERT_TRUE(m1.e22 == 11.0f);
		ASSERT_TRUE(m1.e23 == 12.0f);
		ASSERT_TRUE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		ASSERT_TRUE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec3(1.0f, 5.0f, 9.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec3(2.0f, 6.0f, 10.0f));
		ASSERT_TRUE(m1.columnAt(2) == vec3(3.0f, 7.0f, 11.0f));
		ASSERT_TRUE(m1.columnAt(3) == vec3(4.0f, 8.0f, 12.0f));
	}
	// Individual element constructor
	{
		mat34 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f);
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);
		ASSERT_TRUE(m1.at(1, 0) == 5.0f);
		ASSERT_TRUE(m1.at(1, 1) == 6.0f);
		ASSERT_TRUE(m1.at(1, 2) == 7.0f);
		ASSERT_TRUE(m1.at(1, 3) == 8.0f);
		ASSERT_TRUE(m1.at(2, 0) == 9.0f);
		ASSERT_TRUE(m1.at(2, 1) == 10.0f);
		ASSERT_TRUE(m1.at(2, 2) == 11.0f);
		ASSERT_TRUE(m1.at(2, 3) == 12.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e02 == 3.0f);
		ASSERT_TRUE(m1.e03 == 4.0f);
		ASSERT_TRUE(m1.e10 == 5.0f);
		ASSERT_TRUE(m1.e11 == 6.0f);
		ASSERT_TRUE(m1.e12 == 7.0f);
		ASSERT_TRUE(m1.e13 == 8.0f);
		ASSERT_TRUE(m1.e20 == 9.0f);
		ASSERT_TRUE(m1.e21 == 10.0f);
		ASSERT_TRUE(m1.e22 == 11.0f);
		ASSERT_TRUE(m1.e23 == 12.0f);
		ASSERT_TRUE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		ASSERT_TRUE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec3(1.0f, 5.0f, 9.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec3(2.0f, 6.0f, 10.0f));
		ASSERT_TRUE(m1.columnAt(2) == vec3(3.0f, 7.0f, 11.0f));
		ASSERT_TRUE(m1.columnAt(3) == vec3(4.0f, 8.0f, 12.0f));
	}
	// Row constructor
	{
		mat34 m1(vec4(1.0f, 2.0f, 3.0f, 4.0f),
		         vec4(5.0f, 6.0f, 7.0f, 8.0f),
		         vec4(9.0f, 10.0f, 11.0f, 12.0f));
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);
		ASSERT_TRUE(m1.at(1, 0) == 5.0f);
		ASSERT_TRUE(m1.at(1, 1) == 6.0f);
		ASSERT_TRUE(m1.at(1, 2) == 7.0f);
		ASSERT_TRUE(m1.at(1, 3) == 8.0f);
		ASSERT_TRUE(m1.at(2, 0) == 9.0f);
		ASSERT_TRUE(m1.at(2, 1) == 10.0f);
		ASSERT_TRUE(m1.at(2, 2) == 11.0f);
		ASSERT_TRUE(m1.at(2, 3) == 12.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e02 == 3.0f);
		ASSERT_TRUE(m1.e03 == 4.0f);
		ASSERT_TRUE(m1.e10 == 5.0f);
		ASSERT_TRUE(m1.e11 == 6.0f);
		ASSERT_TRUE(m1.e12 == 7.0f);
		ASSERT_TRUE(m1.e13 == 8.0f);
		ASSERT_TRUE(m1.e20 == 9.0f);
		ASSERT_TRUE(m1.e21 == 10.0f);
		ASSERT_TRUE(m1.e22 == 11.0f);
		ASSERT_TRUE(m1.e23 == 12.0f);
		ASSERT_TRUE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		ASSERT_TRUE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec3(1.0f, 5.0f, 9.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec3(2.0f, 6.0f, 10.0f));
		ASSERT_TRUE(m1.columnAt(2) == vec3(3.0f, 7.0f, 11.0f));
		ASSERT_TRUE(m1.columnAt(3) == vec3(4.0f, 8.0f, 12.0f));
	}
	// 3x3 matrix constructor
	{
		mat33 m1(1.0f, 2.0f, 3.0f,
		         4.0f, 5.0f, 6.0f,
		         7.0f, 8.0f, 9.0f);
		mat34 m2(m1);
		ASSERT_TRUE(m2.at(0, 0) == 1.0f);
		ASSERT_TRUE(m2.at(0, 1) == 2.0f);
		ASSERT_TRUE(m2.at(0, 2) == 3.0f);
		ASSERT_TRUE(m2.at(0, 3) == 0.0f);
		ASSERT_TRUE(m2.at(1, 0) == 4.0f);
		ASSERT_TRUE(m2.at(1, 1) == 5.0f);
		ASSERT_TRUE(m2.at(1, 2) == 6.0f);
		ASSERT_TRUE(m2.at(1, 3) == 0.0f);
		ASSERT_TRUE(m2.at(2, 0) == 7.0f);
		ASSERT_TRUE(m2.at(2, 1) == 8.0f);
		ASSERT_TRUE(m2.at(2, 2) == 9.0f);
		ASSERT_TRUE(m2.at(2, 3) == 0.0f);
	}
	// 4x4 matrix constructor
	{
		mat44 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f,
		         13.0f, 14.0f, 15.0f, 16.0f);
		mat34 m2(m1);
		ASSERT_TRUE(m2.at(0, 0) == 1.0f);
		ASSERT_TRUE(m2.at(0, 1) == 2.0f);
		ASSERT_TRUE(m2.at(0, 2) == 3.0f);
		ASSERT_TRUE(m2.at(0, 3) == 4.0f);
		ASSERT_TRUE(m2.at(1, 0) == 5.0f);
		ASSERT_TRUE(m2.at(1, 1) == 6.0f);
		ASSERT_TRUE(m2.at(1, 2) == 7.0f);
		ASSERT_TRUE(m2.at(1, 3) == 8.0f);
		ASSERT_TRUE(m2.at(2, 0) == 9.0f);
		ASSERT_TRUE(m2.at(2, 1) == 10.0f);
		ASSERT_TRUE(m2.at(2, 2) == 11.0f);
		ASSERT_TRUE(m2.at(2, 3) == 12.0f);
	}
	// fill() constructor function
	{
		mat34 zero = mat34::fill(0.0f);
		ASSERT_TRUE(zero.at(0, 0) == 0.0f);
		ASSERT_TRUE(zero.at(0, 1) == 0.0f);
		ASSERT_TRUE(zero.at(0, 2) == 0.0f);
		ASSERT_TRUE(zero.at(0, 3) == 0.0f);
		ASSERT_TRUE(zero.at(1, 0) == 0.0f);
		ASSERT_TRUE(zero.at(1, 1) == 0.0f);
		ASSERT_TRUE(zero.at(1, 2) == 0.0f);
		ASSERT_TRUE(zero.at(1, 3) == 0.0f);
		ASSERT_TRUE(zero.at(2, 0) == 0.0f);
		ASSERT_TRUE(zero.at(2, 1) == 0.0f);
		ASSERT_TRUE(zero.at(2, 2) == 0.0f);
		ASSERT_TRUE(zero.at(2, 3) == 0.0f);

		mat34 one = mat34::fill(1.0f);
		ASSERT_TRUE(one.at(0, 0) == 1.0f);
		ASSERT_TRUE(one.at(0, 1) == 1.0f);
		ASSERT_TRUE(one.at(0, 2) == 1.0f);
		ASSERT_TRUE(one.at(0, 3) == 1.0f);
		ASSERT_TRUE(one.at(1, 0) == 1.0f);
		ASSERT_TRUE(one.at(1, 1) == 1.0f);
		ASSERT_TRUE(one.at(1, 2) == 1.0f);
		ASSERT_TRUE(one.at(1, 3) == 1.0f);
		ASSERT_TRUE(one.at(2, 0) == 1.0f);
		ASSERT_TRUE(one.at(2, 1) == 1.0f);
		ASSERT_TRUE(one.at(2, 2) == 1.0f);
		ASSERT_TRUE(one.at(2, 3) == 1.0f);
	}
	// identity() constructor function
	{
		mat34 ident = mat34::identity();
		ASSERT_TRUE(ident.at(0, 0) == 1.0f);
		ASSERT_TRUE(ident.at(0, 1) == 0.0f);
		ASSERT_TRUE(ident.at(0, 2) == 0.0f);
		ASSERT_TRUE(ident.at(0, 3) == 0.0f);
		ASSERT_TRUE(ident.at(1, 0) == 0.0f);
		ASSERT_TRUE(ident.at(1, 1) == 1.0f);
		ASSERT_TRUE(ident.at(1, 2) == 0.0f);
		ASSERT_TRUE(ident.at(1, 3) == 0.0f);
		ASSERT_TRUE(ident.at(2, 0) == 0.0f);
		ASSERT_TRUE(ident.at(2, 1) == 0.0f);
		ASSERT_TRUE(ident.at(2, 2) == 1.0f);
		ASSERT_TRUE(ident.at(2, 3) == 0.0f);
	}
	// scaling3() constructor function
	{
		mat34 scale = mat34::scaling3(2.0f);
		ASSERT_TRUE(scale.at(0, 0) == 2.0f);
		ASSERT_TRUE(scale.at(0, 1) == 0.0f);
		ASSERT_TRUE(scale.at(0, 2) == 0.0f);
		ASSERT_TRUE(scale.at(0, 3) == 0.0f);
		ASSERT_TRUE(scale.at(1, 0) == 0.0f);
		ASSERT_TRUE(scale.at(1, 1) == 2.0f);
		ASSERT_TRUE(scale.at(1, 2) == 0.0f);
		ASSERT_TRUE(scale.at(1, 3) == 0.0f);
		ASSERT_TRUE(scale.at(2, 0) == 0.0f);
		ASSERT_TRUE(scale.at(2, 1) == 0.0f);
		ASSERT_TRUE(scale.at(2, 2) == 2.0f);
		ASSERT_TRUE(scale.at(2, 3) == 0.0f);

		mat34 scale2 = mat34::scaling3(vec3(1.0f, 2.0f, 3.0f));
		ASSERT_TRUE(scale2.at(0, 0) == 1.0f);
		ASSERT_TRUE(scale2.at(0, 1) == 0.0f);
		ASSERT_TRUE(scale2.at(0, 2) == 0.0f);
		ASSERT_TRUE(scale2.at(0, 3) == 0.0f);
		ASSERT_TRUE(scale2.at(1, 0) == 0.0f);
		ASSERT_TRUE(scale2.at(1, 1) == 2.0f);
		ASSERT_TRUE(scale2.at(1, 2) == 0.0f);
		ASSERT_TRUE(scale2.at(1, 3) == 0.0f);
		ASSERT_TRUE(scale2.at(2, 0) == 0.0f);
		ASSERT_TRUE(scale2.at(2, 1) == 0.0f);
		ASSERT_TRUE(scale2.at(2, 2) == 3.0f);
		ASSERT_TRUE(scale2.at(2, 3) == 0.0f);
	}
	// rotation3() constructor function
	{
		vec3 startPoint(1.0f, 0.0f, 0.0f);
		vec3 axis = vec3(1.0f, 1.0f, 0.0f);
		mat34 rot = mat34::rotation3(axis, PI);
		ASSERT_TRUE(eqf(transformPoint(rot, startPoint), vec3(0.0f, 1.0, 0.0f)));

		mat34 xRot90 = mat34::rotation3(vec3(1.0f, 0.0f, 0.0f), PI/2.0f);
		ASSERT_TRUE(eqf(xRot90.row0, vec4(1.0f, 0.0f, 0.0f, 0.0f)));
		ASSERT_TRUE(eqf(xRot90.row1, vec4(0.0f, 0.0f, -1.0f, 0.0f)));
		ASSERT_TRUE(eqf(xRot90.row2, vec4(0.0f, 1.0f, 0.0f, 0.0f)));

		vec3 v = transformPoint(xRot90, vec3(1.0f));
		ASSERT_TRUE(eqf(v, vec3(1.0f, -1.0f, 1.0f)));
	}
	// translation3() constructor function
	{
		vec4 v1(1.0f, 1.0f, 1.0f, 1.0f);
		mat44 m = mat44::translation3(vec3(-2.0f, 1.0f, 0.0f));
		ASSERT_TRUE(eqf(m.at(0, 0), 1.0f));
		ASSERT_TRUE(eqf(m.at(0, 1), 0.0f));
		ASSERT_TRUE(eqf(m.at(0, 2), 0.0f));
		ASSERT_TRUE(eqf(m.at(0, 3), -2.0f));
		ASSERT_TRUE(eqf(m.at(1, 0), 0.0f));
		ASSERT_TRUE(eqf(m.at(1, 1), 1.0f));
		ASSERT_TRUE(eqf(m.at(1, 2), 0.0f));
		ASSERT_TRUE(eqf(m.at(1, 3), 1.0f));
		ASSERT_TRUE(eqf(m.at(2, 0), 0.0f));
		ASSERT_TRUE(eqf(m.at(2, 1), 0.0f));
		ASSERT_TRUE(eqf(m.at(2, 2), 1.0f));
		ASSERT_TRUE(eqf(m.at(2, 3), 0.0f));
		ASSERT_TRUE(eqf(m.at(3, 0), 0.0f));
		ASSERT_TRUE(eqf(m.at(3, 1), 0.0f));
		ASSERT_TRUE(eqf(m.at(3, 2), 0.0f));
		ASSERT_TRUE(eqf(m.at(3, 3), 1.0f));
		vec4 v2 = m * v1;
		ASSERT_TRUE(eqf(v2.x, -1.0f));
		ASSERT_TRUE(eqf(v2.y, 2.0f));
		ASSERT_TRUE(eqf(v2.z, 1.0f));
		ASSERT_TRUE(eqf(v2.w, 1.0f));
	}
}

UTEST(Matrix, matrix_4x4_specialization)
{
	// Array pointer constructor
	{
		const float arr1[] = {1.0f, 2.0f, 3.0f, 4.0f,
		                      5.0f, 6.0f, 7.0f, 8.0f,
		                      9.0f, 10.0f, 11.0f, 12.0f,
		                      13.0f, 14.0f, 15.0f, 16.0f};
		mat44 m1(arr1);
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);
		ASSERT_TRUE(m1.at(1, 0) == 5.0f);
		ASSERT_TRUE(m1.at(1, 1) == 6.0f);
		ASSERT_TRUE(m1.at(1, 2) == 7.0f);
		ASSERT_TRUE(m1.at(1, 3) == 8.0f);
		ASSERT_TRUE(m1.at(2, 0) == 9.0f);
		ASSERT_TRUE(m1.at(2, 1) == 10.0f);
		ASSERT_TRUE(m1.at(2, 2) == 11.0f);
		ASSERT_TRUE(m1.at(2, 3) == 12.0f);
		ASSERT_TRUE(m1.at(3, 0) == 13.0f);
		ASSERT_TRUE(m1.at(3, 1) == 14.0f);
		ASSERT_TRUE(m1.at(3, 2) == 15.0f);
		ASSERT_TRUE(m1.at(3, 3) == 16.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e02 == 3.0f);
		ASSERT_TRUE(m1.e03 == 4.0f);
		ASSERT_TRUE(m1.e10 == 5.0f);
		ASSERT_TRUE(m1.e11 == 6.0f);
		ASSERT_TRUE(m1.e12 == 7.0f);
		ASSERT_TRUE(m1.e13 == 8.0f);
		ASSERT_TRUE(m1.e20 == 9.0f);
		ASSERT_TRUE(m1.e21 == 10.0f);
		ASSERT_TRUE(m1.e22 == 11.0f);
		ASSERT_TRUE(m1.e23 == 12.0f);
		ASSERT_TRUE(m1.e30 == 13.0f);
		ASSERT_TRUE(m1.e31 == 14.0f);
		ASSERT_TRUE(m1.e32 == 15.0f);
		ASSERT_TRUE(m1.e33 == 16.0f);
		ASSERT_TRUE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		ASSERT_TRUE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		ASSERT_TRUE(m1.rows[3] == vec4(13.0f, 14.0f, 15.0f, 16.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec4(1.0f, 5.0f, 9.0f, 13.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec4(2.0f, 6.0f, 10.0f, 14.0f));
		ASSERT_TRUE(m1.columnAt(2) == vec4(3.0f, 7.0f, 11.0f, 15.0f));
		ASSERT_TRUE(m1.columnAt(3) == vec4(4.0f, 8.0f, 12.0f, 16.0f));
		ASSERT_TRUE(m1.row012 == mat34(arr1));
	}
	// Individual element constructor
	{
		mat44 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f,
		         13.0f, 14.0f, 15.0f, 16.0f);
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);
		ASSERT_TRUE(m1.at(1, 0) == 5.0f);
		ASSERT_TRUE(m1.at(1, 1) == 6.0f);
		ASSERT_TRUE(m1.at(1, 2) == 7.0f);
		ASSERT_TRUE(m1.at(1, 3) == 8.0f);
		ASSERT_TRUE(m1.at(2, 0) == 9.0f);
		ASSERT_TRUE(m1.at(2, 1) == 10.0f);
		ASSERT_TRUE(m1.at(2, 2) == 11.0f);
		ASSERT_TRUE(m1.at(2, 3) == 12.0f);
		ASSERT_TRUE(m1.at(3, 0) == 13.0f);
		ASSERT_TRUE(m1.at(3, 1) == 14.0f);
		ASSERT_TRUE(m1.at(3, 2) == 15.0f);
		ASSERT_TRUE(m1.at(3, 3) == 16.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e02 == 3.0f);
		ASSERT_TRUE(m1.e03 == 4.0f);
		ASSERT_TRUE(m1.e10 == 5.0f);
		ASSERT_TRUE(m1.e11 == 6.0f);
		ASSERT_TRUE(m1.e12 == 7.0f);
		ASSERT_TRUE(m1.e13 == 8.0f);
		ASSERT_TRUE(m1.e20 == 9.0f);
		ASSERT_TRUE(m1.e21 == 10.0f);
		ASSERT_TRUE(m1.e22 == 11.0f);
		ASSERT_TRUE(m1.e23 == 12.0f);
		ASSERT_TRUE(m1.e30 == 13.0f);
		ASSERT_TRUE(m1.e31 == 14.0f);
		ASSERT_TRUE(m1.e32 == 15.0f);
		ASSERT_TRUE(m1.e33 == 16.0f);
		ASSERT_TRUE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		ASSERT_TRUE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		ASSERT_TRUE(m1.rows[3] == vec4(13.0f, 14.0f, 15.0f, 16.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec4(1.0f, 5.0f, 9.0f, 13.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec4(2.0f, 6.0f, 10.0f, 14.0f));
		ASSERT_TRUE(m1.columnAt(2) == vec4(3.0f, 7.0f, 11.0f, 15.0f));
		ASSERT_TRUE(m1.columnAt(3) == vec4(4.0f, 8.0f, 12.0f, 16.0f));
	}
	// Row constructor
	{
		mat44 m1(vec4(1.0f, 2.0f, 3.0f, 4.0f),
		         vec4(5.0f, 6.0f, 7.0f, 8.0f),
		         vec4(9.0f, 10.0f, 11.0f, 12.0f),
		         vec4(13.0f, 14.0f, 15.0f, 16.0f));
		ASSERT_TRUE(m1.at(0, 0) == 1.0f);
		ASSERT_TRUE(m1.at(0, 1) == 2.0f);
		ASSERT_TRUE(m1.at(0, 2) == 3.0f);
		ASSERT_TRUE(m1.at(0, 3) == 4.0f);
		ASSERT_TRUE(m1.at(1, 0) == 5.0f);
		ASSERT_TRUE(m1.at(1, 1) == 6.0f);
		ASSERT_TRUE(m1.at(1, 2) == 7.0f);
		ASSERT_TRUE(m1.at(1, 3) == 8.0f);
		ASSERT_TRUE(m1.at(2, 0) == 9.0f);
		ASSERT_TRUE(m1.at(2, 1) == 10.0f);
		ASSERT_TRUE(m1.at(2, 2) == 11.0f);
		ASSERT_TRUE(m1.at(2, 3) == 12.0f);
		ASSERT_TRUE(m1.at(3, 0) == 13.0f);
		ASSERT_TRUE(m1.at(3, 1) == 14.0f);
		ASSERT_TRUE(m1.at(3, 2) == 15.0f);
		ASSERT_TRUE(m1.at(3, 3) == 16.0f);
		ASSERT_TRUE(m1.e00 == 1.0f);
		ASSERT_TRUE(m1.e01 == 2.0f);
		ASSERT_TRUE(m1.e02 == 3.0f);
		ASSERT_TRUE(m1.e03 == 4.0f);
		ASSERT_TRUE(m1.e10 == 5.0f);
		ASSERT_TRUE(m1.e11 == 6.0f);
		ASSERT_TRUE(m1.e12 == 7.0f);
		ASSERT_TRUE(m1.e13 == 8.0f);
		ASSERT_TRUE(m1.e20 == 9.0f);
		ASSERT_TRUE(m1.e21 == 10.0f);
		ASSERT_TRUE(m1.e22 == 11.0f);
		ASSERT_TRUE(m1.e23 == 12.0f);
		ASSERT_TRUE(m1.e30 == 13.0f);
		ASSERT_TRUE(m1.e31 == 14.0f);
		ASSERT_TRUE(m1.e32 == 15.0f);
		ASSERT_TRUE(m1.e33 == 16.0f);
		ASSERT_TRUE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		ASSERT_TRUE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		ASSERT_TRUE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		ASSERT_TRUE(m1.rows[3] == vec4(13.0f, 14.0f, 15.0f, 16.0f));
		ASSERT_TRUE(m1.columnAt(0) == vec4(1.0f, 5.0f, 9.0f, 13.0f));
		ASSERT_TRUE(m1.columnAt(1) == vec4(2.0f, 6.0f, 10.0f, 14.0f));
		ASSERT_TRUE(m1.columnAt(2) == vec4(3.0f, 7.0f, 11.0f, 15.0f));
		ASSERT_TRUE(m1.columnAt(3) == vec4(4.0f, 8.0f, 12.0f, 16.0f));
	}
	// 3x3 matrix constructor
	{
		mat33 m1(1.0f, 2.0f, 3.0f,
		         4.0f, 5.0f, 6.0f,
		         7.0f, 8.0f, 9.0f);
		mat44 m2(m1);
		ASSERT_TRUE(m2.at(0, 0) == 1.0f);
		ASSERT_TRUE(m2.at(0, 1) == 2.0f);
		ASSERT_TRUE(m2.at(0, 2) == 3.0f);
		ASSERT_TRUE(m2.at(0, 3) == 0.0f);
		ASSERT_TRUE(m2.at(1, 0) == 4.0f);
		ASSERT_TRUE(m2.at(1, 1) == 5.0f);
		ASSERT_TRUE(m2.at(1, 2) == 6.0f);
		ASSERT_TRUE(m2.at(1, 3) == 0.0f);
		ASSERT_TRUE(m2.at(2, 0) == 7.0f);
		ASSERT_TRUE(m2.at(2, 1) == 8.0f);
		ASSERT_TRUE(m2.at(2, 2) == 9.0f);
		ASSERT_TRUE(m2.at(2, 3) == 0.0f);
		ASSERT_TRUE(m2.at(3, 0) == 0.0f);
		ASSERT_TRUE(m2.at(3, 1) == 0.0f);
		ASSERT_TRUE(m2.at(3, 2) == 0.0f);
		ASSERT_TRUE(m2.at(3, 3) == 1.0f);
	}
	// 4x3 matrix constructor
	{
		mat34 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f);
		mat44 m2(m1);
		ASSERT_TRUE(m2.at(0, 0) == 1.0f);
		ASSERT_TRUE(m2.at(0, 1) == 2.0f);
		ASSERT_TRUE(m2.at(0, 2) == 3.0f);
		ASSERT_TRUE(m2.at(0, 3) == 4.0f);
		ASSERT_TRUE(m2.at(1, 0) == 5.0f);
		ASSERT_TRUE(m2.at(1, 1) == 6.0f);
		ASSERT_TRUE(m2.at(1, 2) == 7.0f);
		ASSERT_TRUE(m2.at(1, 3) == 8.0f);
		ASSERT_TRUE(m2.at(2, 0) == 9.0f);
		ASSERT_TRUE(m2.at(2, 1) == 10.0f);
		ASSERT_TRUE(m2.at(2, 2) == 11.0f);
		ASSERT_TRUE(m2.at(2, 3) == 12.0f);
		ASSERT_TRUE(m2.at(3, 0) == 0.0f);
		ASSERT_TRUE(m2.at(3, 1) == 0.0f);
		ASSERT_TRUE(m2.at(3, 2) == 0.0f);
		ASSERT_TRUE(m2.at(3, 3) == 1.0f);
		ASSERT_TRUE(m2.row012 == m1);
	}
	// fill() constructor function
	{
		mat44 zero = mat44::fill(0.0f);
		ASSERT_TRUE(zero.at(0, 0) == 0.0f);
		ASSERT_TRUE(zero.at(0, 1) == 0.0f);
		ASSERT_TRUE(zero.at(0, 2) == 0.0f);
		ASSERT_TRUE(zero.at(0, 3) == 0.0f);
		ASSERT_TRUE(zero.at(1, 0) == 0.0f);
		ASSERT_TRUE(zero.at(1, 1) == 0.0f);
		ASSERT_TRUE(zero.at(1, 2) == 0.0f);
		ASSERT_TRUE(zero.at(1, 3) == 0.0f);
		ASSERT_TRUE(zero.at(2, 0) == 0.0f);
		ASSERT_TRUE(zero.at(2, 1) == 0.0f);
		ASSERT_TRUE(zero.at(2, 2) == 0.0f);
		ASSERT_TRUE(zero.at(2, 3) == 0.0f);
		ASSERT_TRUE(zero.at(3, 0) == 0.0f);
		ASSERT_TRUE(zero.at(3, 1) == 0.0f);
		ASSERT_TRUE(zero.at(3, 2) == 0.0f);
		ASSERT_TRUE(zero.at(3, 3) == 0.0f);

		mat44 one = mat44::fill(1.0f);
		ASSERT_TRUE(one.at(0, 0) == 1.0f);
		ASSERT_TRUE(one.at(0, 1) == 1.0f);
		ASSERT_TRUE(one.at(0, 2) == 1.0f);
		ASSERT_TRUE(one.at(0, 3) == 1.0f);
		ASSERT_TRUE(one.at(1, 0) == 1.0f);
		ASSERT_TRUE(one.at(1, 1) == 1.0f);
		ASSERT_TRUE(one.at(1, 2) == 1.0f);
		ASSERT_TRUE(one.at(1, 3) == 1.0f);
		ASSERT_TRUE(one.at(2, 0) == 1.0f);
		ASSERT_TRUE(one.at(2, 1) == 1.0f);
		ASSERT_TRUE(one.at(2, 2) == 1.0f);
		ASSERT_TRUE(one.at(2, 3) == 1.0f);
		ASSERT_TRUE(one.at(3, 0) == 1.0f);
		ASSERT_TRUE(one.at(3, 1) == 1.0f);
		ASSERT_TRUE(one.at(3, 2) == 1.0f);
		ASSERT_TRUE(one.at(3, 3) == 1.0f);
	}
	// identity() constructor function
	{
		mat44 ident = mat44::identity();
		ASSERT_TRUE(ident.at(0, 0) == 1.0f);
		ASSERT_TRUE(ident.at(0, 1) == 0.0f);
		ASSERT_TRUE(ident.at(0, 2) == 0.0f);
		ASSERT_TRUE(ident.at(0, 3) == 0.0f);
		ASSERT_TRUE(ident.at(1, 0) == 0.0f);
		ASSERT_TRUE(ident.at(1, 1) == 1.0f);
		ASSERT_TRUE(ident.at(1, 2) == 0.0f);
		ASSERT_TRUE(ident.at(1, 3) == 0.0f);
		ASSERT_TRUE(ident.at(2, 0) == 0.0f);
		ASSERT_TRUE(ident.at(2, 1) == 0.0f);
		ASSERT_TRUE(ident.at(2, 2) == 1.0f);
		ASSERT_TRUE(ident.at(2, 3) == 0.0f);
		ASSERT_TRUE(ident.at(3, 0) == 0.0f);
		ASSERT_TRUE(ident.at(3, 1) == 0.0f);
		ASSERT_TRUE(ident.at(3, 2) == 0.0f);
		ASSERT_TRUE(ident.at(3, 3) == 1.0f);
	}
	// scaling3() constructor function
	{
		mat44 scale = mat44::scaling3(2.0f);
		ASSERT_TRUE(scale.at(0, 0) == 2.0f);
		ASSERT_TRUE(scale.at(0, 1) == 0.0f);
		ASSERT_TRUE(scale.at(0, 2) == 0.0f);
		ASSERT_TRUE(scale.at(0, 3) == 0.0f);
		ASSERT_TRUE(scale.at(1, 0) == 0.0f);
		ASSERT_TRUE(scale.at(1, 1) == 2.0f);
		ASSERT_TRUE(scale.at(1, 2) == 0.0f);
		ASSERT_TRUE(scale.at(1, 3) == 0.0f);
		ASSERT_TRUE(scale.at(2, 0) == 0.0f);
		ASSERT_TRUE(scale.at(2, 1) == 0.0f);
		ASSERT_TRUE(scale.at(2, 2) == 2.0f);
		ASSERT_TRUE(scale.at(2, 3) == 0.0f);
		ASSERT_TRUE(scale.at(3, 0) == 0.0f);
		ASSERT_TRUE(scale.at(3, 1) == 0.0f);
		ASSERT_TRUE(scale.at(3, 2) == 0.0f);
		ASSERT_TRUE(scale.at(3, 3) == 1.0f);

		mat44 scale2 = mat44::scaling3(vec3(1.0f, 2.0f, 3.0f));
		ASSERT_TRUE(scale2.at(0, 0) == 1.0f);
		ASSERT_TRUE(scale2.at(0, 1) == 0.0f);
		ASSERT_TRUE(scale2.at(0, 2) == 0.0f);
		ASSERT_TRUE(scale2.at(0, 3) == 0.0f);
		ASSERT_TRUE(scale2.at(1, 0) == 0.0f);
		ASSERT_TRUE(scale2.at(1, 1) == 2.0f);
		ASSERT_TRUE(scale2.at(1, 2) == 0.0f);
		ASSERT_TRUE(scale2.at(1, 3) == 0.0f);
		ASSERT_TRUE(scale2.at(2, 0) == 0.0f);
		ASSERT_TRUE(scale2.at(2, 1) == 0.0f);
		ASSERT_TRUE(scale2.at(2, 2) == 3.0f);
		ASSERT_TRUE(scale2.at(2, 3) == 0.0f);
		ASSERT_TRUE(scale2.at(3, 0) == 0.0f);
		ASSERT_TRUE(scale2.at(3, 1) == 0.0f);
		ASSERT_TRUE(scale2.at(3, 2) == 0.0f);
		ASSERT_TRUE(scale2.at(3, 3) == 1.0f);
	}
	// rotation3() constructor function
	{
		vec4 startPoint(1.0f, 0.0f, 0.0f, 1.0f);
		vec3 axis = vec3(1.0f, 1.0f, 0.0f);
		mat44 rot = mat44::rotation3(axis, PI);
		ASSERT_TRUE(eqf(rot * startPoint, vec4(0.0f, 1.0, 0.0f, 1.0f)));

		mat44 xRot90 = mat44::rotation3(vec3(1.0f, 0.0f, 0.0f), PI/2.0f);
		ASSERT_TRUE(eqf(xRot90.row0, vec4(1.0f, 0.0f, 0.0f, 0.0f)));
		ASSERT_TRUE(eqf(xRot90.row1, vec4(0.0f, 0.0f, -1.0f, 0.0f)));
		ASSERT_TRUE(eqf(xRot90.row2, vec4(0.0f, 1.0f, 0.0f, 0.0f)));
		ASSERT_TRUE(eqf(xRot90.row3, vec4(0.0f, 0.0f, 0.0f, 1.0f)));

		vec4 v = xRot90 * vec4(1.0f);
		ASSERT_TRUE(eqf(v, vec4(1.0f, -1.0f, 1.0f, 1.0f)));
	}
	// translation3() constructor function
	{
		vec4 v1(1.0f, 1.0f, 1.0f, 1.0f);
		mat44 m = mat44::translation3(vec3(-2.0f, 1.0f, 0.0f));
		ASSERT_TRUE(eqf(m.at(0, 0), 1.0f));
		ASSERT_TRUE(eqf(m.at(0, 1), 0.0f));
		ASSERT_TRUE(eqf(m.at(0, 2), 0.0f));
		ASSERT_TRUE(eqf(m.at(0, 3), -2.0f));
		ASSERT_TRUE(eqf(m.at(1, 0), 0.0f));
		ASSERT_TRUE(eqf(m.at(1, 1), 1.0f));
		ASSERT_TRUE(eqf(m.at(1, 2), 0.0f));
		ASSERT_TRUE(eqf(m.at(1, 3), 1.0f));
		ASSERT_TRUE(eqf(m.at(2, 0), 0.0f));
		ASSERT_TRUE(eqf(m.at(2, 1), 0.0f));
		ASSERT_TRUE(eqf(m.at(2, 2), 1.0f));
		ASSERT_TRUE(eqf(m.at(2, 3), 0.0f));
		ASSERT_TRUE(eqf(m.at(3, 0), 0.0f));
		ASSERT_TRUE(eqf(m.at(3, 1), 0.0f));
		ASSERT_TRUE(eqf(m.at(3, 2), 0.0f));
		ASSERT_TRUE(eqf(m.at(3, 3), 1.0f));
		vec4 v2 = m * v1;
		ASSERT_TRUE(eqf(v2.x, -1.0f));
		ASSERT_TRUE(eqf(v2.y, 2.0f));
		ASSERT_TRUE(eqf(v2.z, 1.0f));
		ASSERT_TRUE(eqf(v2.w, 1.0f));
	}
}

UTEST(Matrix, arithmetic_assignment_operators)
{
	// +=
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m3(-2.0f, -1.0f,
			3.0f, 33.0f);

		m1 += m2;
		m2 += m3;

		ASSERT_TRUE(eqf(m1.at(0, 0), 2.0f));
		ASSERT_TRUE(eqf(m1.at(0, 1), 4.0f));
		ASSERT_TRUE(eqf(m1.at(1, 0), 6.0f));
		ASSERT_TRUE(eqf(m1.at(1, 1), 8.0f));

		ASSERT_TRUE(eqf(m2.at(0, 0), -1.0f));
		ASSERT_TRUE(eqf(m2.at(0, 1), 1.0f));
		ASSERT_TRUE(eqf(m2.at(1, 0), 6.0f));
		ASSERT_TRUE(eqf(m2.at(1, 1), 37.0f));
	}
	// -=
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m3(-2.0f, -1.0f,
			3.0f, 33.0f);

		m1 -= m2;
		m2 -= m3;

		ASSERT_TRUE(eqf(m1.at(0, 0), 0.0f));
		ASSERT_TRUE(eqf(m1.at(0, 1), 0.0f));
		ASSERT_TRUE(eqf(m1.at(1, 0), 0.0f));
		ASSERT_TRUE(eqf(m1.at(1, 1), 0.0f));

		ASSERT_TRUE(eqf(m2.at(0, 0), 3.0f));
		ASSERT_TRUE(eqf(m2.at(0, 1), 3.0f));
		ASSERT_TRUE(eqf(m2.at(1, 0), 0.0f));
		ASSERT_TRUE(eqf(m2.at(1, 1), -29.0f));
	}
	// *= (scalar)
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m3(-2.0f, -1.0f,
			3.0f, 33.0f);

		m1 *= 2.0f;
		ASSERT_TRUE(eqf(m1.at(0, 0), 2.0f));
		ASSERT_TRUE(eqf(m1.at(0, 1), 4.0f));
		ASSERT_TRUE(eqf(m1.at(1, 0), 6.0f));
		ASSERT_TRUE(eqf(m1.at(1, 1), 8.0f));

		m3 *= -1.0f;
		ASSERT_TRUE(eqf(m3.at(0, 0), 2.0f));
		ASSERT_TRUE(eqf(m3.at(0, 1), 1.0f));
		ASSERT_TRUE(eqf(m3.at(1, 0), -3.0f));
		ASSERT_TRUE(eqf(m3.at(1, 1), -33.0f));
	}
	// *= (matrix of same size)
	{
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

		ASSERT_TRUE(eqf(m1cpy.at(0, 0), 1.0f));
		ASSERT_TRUE(eqf(m1cpy.at(0, 1), 2.0f));
		ASSERT_TRUE(eqf(m1cpy.at(1, 0), 3.0f));
		ASSERT_TRUE(eqf(m1cpy.at(1, 1), 4.0f));

		m4 *= m1;
		ASSERT_TRUE(eqf(m4.at(0, 0), 1.0f));
		ASSERT_TRUE(eqf(m4.at(0, 1), 2.0f));
		ASSERT_TRUE(eqf(m4.at(1, 0), 3.0f));
		ASSERT_TRUE(eqf(m4.at(1, 1), 4.0f));
	}
}

UTEST(Matrix, arithmetic_operators)
{
	// +
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(0.0f, 1.0f,
			0.0f, 0.0f);
		float m3arr[] = { 1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f };
		Matrix<float, 2, 3> m3(m3arr);
		float m4arr[] = { 1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f };
		Matrix<float, 3, 2> m4(m4arr);

		auto res1 = m1 + m2;
		ASSERT_TRUE(eqf(res1.at(0, 0), 1.0f));
		ASSERT_TRUE(eqf(res1.at(0, 1), 3.0f));
		ASSERT_TRUE(eqf(res1.at(1, 0), 3.0f));
		ASSERT_TRUE(eqf(res1.at(1, 1), 4.0f));

		auto res2 = m3 + m3;
		ASSERT_TRUE(eqf(res2.at(0, 0), 2.0f));
		ASSERT_TRUE(eqf(res2.at(0, 1), 4.0f));
		ASSERT_TRUE(eqf(res2.at(0, 2), 6.0f));
		ASSERT_TRUE(eqf(res2.at(1, 0), 8.0f));
		ASSERT_TRUE(eqf(res2.at(1, 1), 10.0f));
		ASSERT_TRUE(eqf(res2.at(1, 2), 12.0f));
	}
	// -
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(0.0f, 1.0f,
			0.0f, 0.0f);
		float m3arr[] = { 1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f };
		Matrix<float, 2, 3> m3(m3arr);
		float m4arr[] = { 1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f };
		Matrix<float, 3, 2> m4(m4arr);

		auto res1 = m1 - m2;
		auto res2 = m2 - m1;

		ASSERT_TRUE(res1 != res2);

		ASSERT_TRUE(eqf(res1.at(0, 0), 1.0f));
		ASSERT_TRUE(eqf(res1.at(0, 1), 1.0f));
		ASSERT_TRUE(eqf(res1.at(1, 0), 3.0f));
		ASSERT_TRUE(eqf(res1.at(1, 1), 4.0f));

		ASSERT_TRUE(eqf(res2.at(0, 0), -1.0f));
		ASSERT_TRUE(eqf(res2.at(0, 1), -1.0f));
		ASSERT_TRUE(eqf(res2.at(1, 0), -3.0f));
		ASSERT_TRUE(eqf(res2.at(1, 1), -4.0f));
	}
	// - (negation)
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(0.0f, 1.0f,
			0.0f, 0.0f);
		float m3arr[] = { 1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f };
		Matrix<float, 2, 3> m3(m3arr);
		float m4arr[] = { 1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f };
		Matrix<float, 3, 2> m4(m4arr);

		auto res1 = -m1;

		ASSERT_TRUE(eqf(res1.at(0, 0), -1.0f));
		ASSERT_TRUE(eqf(res1.at(0, 1), -2.0f));
		ASSERT_TRUE(eqf(res1.at(1, 0), -3.0f));
		ASSERT_TRUE(eqf(res1.at(1, 1), -4.0f));
	}
	// * (matrix)
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(0.0f, 1.0f,
			0.0f, 0.0f);
		float m3arr[] = { 1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f };
		Matrix<float, 2, 3> m3(m3arr);
		float m4arr[] = { 1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f };
		Matrix<float, 3, 2> m4(m4arr);

		auto res1 = m1*m2;
		ASSERT_TRUE(eqf(res1.at(0, 0), 0.0f));
		ASSERT_TRUE(eqf(res1.at(0, 1), 1.0f));
		ASSERT_TRUE(eqf(res1.at(1, 0), 0.0f));
		ASSERT_TRUE(eqf(res1.at(1, 1), 3.0f));

		auto res2 = m2*m1;
		ASSERT_TRUE(eqf(res2.at(0, 0), 3.0f));
		ASSERT_TRUE(eqf(res2.at(0, 1), 4.0f));
		ASSERT_TRUE(eqf(res2.at(1, 0), 0.0f));
		ASSERT_TRUE(eqf(res2.at(1, 1), 0.0f));

		auto res3 = m3*m4;
		ASSERT_TRUE(eqf(res3.at(0, 0), 1.0f));
		ASSERT_TRUE(eqf(res3.at(0, 1), 2.0f));
		ASSERT_TRUE(eqf(res3.at(1, 0), 4.0f));
		ASSERT_TRUE(eqf(res3.at(1, 1), 5.0f));
	}
	// * (vector)
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(0.0f, 1.0f,
			0.0f, 0.0f);
		float m3arr[] = { 1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f };
		Matrix<float, 2, 3> m3(m3arr);
		float m4arr[] = { 1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f };
		Matrix<float, 3, 2> m4(m4arr);

		vec2 v1(1.0f, -2.0f);

		vec2 res1 = m1 * v1;
		ASSERT_TRUE(eqf(res1.x, -3.0f));
		ASSERT_TRUE(eqf(res1.y, -5.0f));

		vec3 res2 = m4 * v1;
		ASSERT_TRUE(eqf(res2.x, 1.0f));
		ASSERT_TRUE(eqf(res2.y, -2.0f));
		ASSERT_TRUE(eqf(res2.z, 0.0f));

		mat34 m5(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f);
		ASSERT_TRUE(eqf(m5 * vec4(1.0f), vec3(10.0f, 26.0f, 42.0f)));
	}
	// * (scalar)
	{
		mat22 m1(1.0f, 2.0f,
			3.0f, 4.0f);
		mat22 m2(0.0f, 1.0f,
			0.0f, 0.0f);
		float m3arr[] = { 1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f };
		Matrix<float, 2, 3> m3(m3arr);
		float m4arr[] = { 1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f };
		Matrix<float, 3, 2> m4(m4arr);

		auto res1 = m1 * 2.0f;
		ASSERT_TRUE(eqf(res1.at(0, 0), 2.0f));
		ASSERT_TRUE(eqf(res1.at(0, 1), 4.0f));
		ASSERT_TRUE(eqf(res1.at(1, 0), 6.0f));
		ASSERT_TRUE(eqf(res1.at(1, 1), 8.0f));

		auto res2 = -1.0f * m2;
		ASSERT_TRUE(eqf(res2.at(0, 0), 0.0f));
		ASSERT_TRUE(eqf(res2.at(0, 1), -1.0f));
		ASSERT_TRUE(eqf(res2.at(1, 0), 0.0f));
		ASSERT_TRUE(eqf(res2.at(1, 1), 0.0f));
	}
}

UTEST(Matrix, comparison_operators)
{
	mat22 m1(1.0f, 2.0f,
	         3.0f, 4.0f);
	mat22 m2(1.0f, 2.0f,
	         3.0f, 4.0f);
	mat22 m3(-2.0f, -1.0f,
	         3.0f, 33.0f);

	ASSERT_TRUE(m1 == m2);
	ASSERT_TRUE(m2 == m1);
	ASSERT_TRUE(!(m1 == m3));
	ASSERT_TRUE(!(m3 == m1));
	ASSERT_TRUE(!(m2 == m3));
	ASSERT_TRUE(!(m3 == m2));

	ASSERT_TRUE(m1 != m3);
	ASSERT_TRUE(m3 != m1);
	ASSERT_TRUE(m2 != m3);
	ASSERT_TRUE(m3 != m2);
	ASSERT_TRUE(!(m1 != m2));
	ASSERT_TRUE(!(m2 != m1));
}

UTEST(Matrix, element_wise_multiplication)
{
	mat22 m(1.0f, 2.0f,
		3.0f, 4.0f);
	mat22 res = elemMult(m, m);

	ASSERT_TRUE(eqf(res.at(0, 0), 1.0f));
	ASSERT_TRUE(eqf(res.at(0, 1), 4.0f));
	ASSERT_TRUE(eqf(res.at(1, 0), 9.0f));
	ASSERT_TRUE(eqf(res.at(1, 1), 16.0f));
}

UTEST(Matrix, transpose)
{
	float arr[] ={1.0f, 2.0f, 3.0f,
		4.0f, 5.0f, 6.0f};
	Matrix<float, 2, 3> m1(arr);

	Matrix<float, 3, 2> m2 = transpose(m1);
	ASSERT_TRUE(eqf(m2.at(0, 0), 1.0f));
	ASSERT_TRUE(eqf(m2.at(0, 1), 4.0f));
	ASSERT_TRUE(eqf(m2.at(1, 0), 2.0f));
	ASSERT_TRUE(eqf(m2.at(1, 1), 5.0f));
	ASSERT_TRUE(eqf(m2.at(2, 0), 3.0f));
	ASSERT_TRUE(eqf(m2.at(2, 1), 6.0f));

	mat44 m3(1.0f, 2.0f, 3.0f, 4.0f,
	         5.0f, 6.0f, 7.0f, 8.0f,
	         9.0f, 10.0f, 11.0f, 12.0f,
	         13.0f, 14.0f, 15.0f, 16.0f);
	mat44 m3transp = transpose(m3);
	ASSERT_TRUE(eqf(m3transp.row0, vec4(1.0f, 5.0f, 9.0f, 13.0f)));
	ASSERT_TRUE(eqf(m3transp.row1, vec4(2.0f, 6.0f, 10.0f, 14.0f)));
	ASSERT_TRUE(eqf(m3transp.row2, vec4(3.0f, 7.0f, 11.0f, 15.0f)));
	ASSERT_TRUE(eqf(m3transp.row3, vec4(4.0f, 8.0f, 12.0f, 16.0f)));
}

UTEST(Matrix, transforming_3d_vector_with_3x4_and_4x4_matrix)
{
	// transformPoint() 3x4
	{
		mat34 m1(2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f);
		mat44 m2(2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		vec3 v(1.0f, 1.0f, 1.0f);

		vec3 v2 = transformPoint(m1, v);
		ASSERT_TRUE(eqf(v2.x, 3.0f));
		ASSERT_TRUE(eqf(v2.y, 2.0f));
		ASSERT_TRUE(eqf(v2.z, 2.0f));
	}
	// transformPoint() 4x4
	{
		mat34 m1(2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f);
		mat44 m2(2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		vec3 v(1.0f, 1.0f, 1.0f);

		vec3 v2 = transformPoint(m2, v);
		ASSERT_TRUE(eqf(v2.x, 3.0f));
		ASSERT_TRUE(eqf(v2.y, 2.0f));
		ASSERT_TRUE(eqf(v2.z, 2.0f));
	}
	// transformDir() 3x4
	{
		mat34 m1(2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f);
		mat44 m2(2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		vec3 v(1.0f, 1.0f, 1.0f);

		vec3 v2 = transformDir(m1, v);
		ASSERT_TRUE(eqf(v2.x, 2.0f));
		ASSERT_TRUE(eqf(v2.y, 2.0f));
		ASSERT_TRUE(eqf(v2.z, 2.0f));
	}
	// transformDir() 4x4
	{
		mat34 m1(2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f);
		mat44 m2(2.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		vec3 v(1.0f, 1.0f, 1.0f);

		vec3 v2 = transformDir(m2, v);
		ASSERT_TRUE(eqf(v2.x, 2.0f));
		ASSERT_TRUE(eqf(v2.y, 2.0f));
		ASSERT_TRUE(eqf(v2.z, 2.0f));
	}
}

UTEST(Matrix, determinants)
{
	mat22 m1(1.0f, 2.0f,
	         3.0f, 4.0f);
	ASSERT_TRUE(eqf(determinant(m1), -2.0f));

	mat33 m2(-1.0f, 1.0f, 0.0f,
	         3.0f, 5.0f, 1.0f,
	         7.0f, 8.0f, 9.0f);
	ASSERT_TRUE(eqf(determinant(m2), -57.0f));

	mat33 m3(99.0f, -2.0f, 5.0f,
	         8.0f, -4.0f, -1.0f,
	         6.0f, 1.0f, -88.0f);
	ASSERT_TRUE(eqf(determinant(m3), 33711.0f));

	mat44 m4(1.0f, -2.0f, 1.0f, 3.0f,
	         1.0f, 4.0f, -5.0f, 0.0f,
	         -10.0f, 0.0f, 4.0f, 2.0f,
	         -1.0f, 0.0f, 2.0f, 0.0f);
	ASSERT_TRUE(eqf(determinant(m4), -204.0f));
}

UTEST(Matrix, inverse)
{
	mat22 m1(1.0f, 1.0f,
	         1.0f, 2.0f);
	mat22 m1Inv(2.0f, -1.0f,
	            -1.0f, 1.0f);
	mat22 m1CalcInv = inverse(m1);
	ASSERT_TRUE(eqf(m1CalcInv.row0, m1Inv.row0));
	ASSERT_TRUE(eqf(m1CalcInv.row1, m1Inv.row1));

	mat33 m3(1.0f, 1.0f, 1.0f,
	         1.0f, 1.0f, 2.0f,
	         1.0f, 2.0f, 3.0f);
	mat33 m3Inv(1.0f, 1.0f, -1.0f,
	            1.0f, -2.0f, 1.0f,
	            -1.0f, 1.0f, 0.0f);
	mat33 m3CalcInv = inverse(m3);
	ASSERT_TRUE(eqf(m3CalcInv.row0, m3Inv.row0));
	ASSERT_TRUE(eqf(m3CalcInv.row1, m3Inv.row1));
	ASSERT_TRUE(eqf(m3CalcInv.row2, m3Inv.row2));

	mat44 m5(1.0f, 1.0f, 1.0f, 1.0f,
	         1.0f, 1.0f, 2.0f, 3.0f,
	         1.0f, 2.0f, 3.0f, 4.0f,
	         1.0f, 2.0f, 2.0f, 1.0f);
	mat44 m5Inv(1.0f, 1.0f, -1.0f, 0.0f,
	            2.0f, -3.0f, 2.0f, -1.0f,
	            -3.0f, 3.0f, -2.0f, 2.0f,
	            1.0f, -1.0f, 1.0f, -1.0f);
	mat44 m5CalcInv = inverse(m5);
	ASSERT_TRUE(eqf(m5CalcInv.row0, m5Inv.row0));
	ASSERT_TRUE(eqf(m5CalcInv.row1, m5Inv.row1));
	ASSERT_TRUE(eqf(m5CalcInv.row2, m5Inv.row2));
	ASSERT_TRUE(eqf(m5CalcInv.row3, m5Inv.row3));
}

UTEST(Matrix, matrix_is_proper_pod)
{
	ASSERT_TRUE(std::is_trivially_default_constructible<sfz::mat4>::value);
	ASSERT_TRUE(std::is_trivially_copyable<sfz::mat4>::value);
	ASSERT_TRUE(std::is_trivial<sfz::mat4>::value);
	ASSERT_TRUE(std::is_standard_layout<sfz::mat4>::value);
	ASSERT_TRUE(std::is_pod<sfz::mat4>::value);
}

UTEST(Matrix, matrix_to_string)
{
	mat4 m1{{1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}, {9.0f, 10.0f, 11.0f, 12.0f}, {13.0f, 14.0f, 15.0f, 16.0f}};
	auto mStr1 = toString(m1, false, 1);
	ASSERT_TRUE(mStr1 == "[[1.0, 2.0, 3.0, 4.0], [5.0, 6.0, 7.0, 8.0], [9.0, 10.0, 11.0, 12.0], [13.0, 14.0, 15.0, 16.0]]");
}
