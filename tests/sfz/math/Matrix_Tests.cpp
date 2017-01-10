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

#include "sfz/math/MathPrimitiveToStrings.hpp"
#include "sfz/math/MathSupport.hpp"
#include "sfz/math/Matrix.hpp"
#include "sfz/math/ProjectionMatrices.hpp"
#include "sfz/math/Vector.hpp"

#include <type_traits>

using namespace sfz;

TEST_CASE("Matrix<T,H,W> general definition", "[sfz::Matrix]")
{
	SECTION("Array pointer constructor") {
		const float arr1[] = {1.0f, 2.0f, 3.0f, 4.0f};
		Matrix<float,1,4> m1(arr1);
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));

		Matrix<float,4,1> m2(arr1);
		REQUIRE(m2.at(0, 0) == 1.0f);
		REQUIRE(m2.at(1, 0) == 2.0f);
		REQUIRE(m2.at(2, 0) == 3.0f);
		REQUIRE(m2.at(3, 0) == 4.0f);
		REQUIRE(m2.columnAt(0) == vec4(1.0f, 2.0f, 3.0f, 4.0f));

		const float arr2[] = {6.0f, 5.0f, 4.0f,
		                      3.0f, 2.0f, 1.0f};
		Matrix<float,2,3> m3(arr2);
		REQUIRE(m3.at(0, 0) == 6.0f);
		REQUIRE(m3.at(0, 1) == 5.0f);
		REQUIRE(m3.at(0, 2) == 4.0f);
		REQUIRE(m3.at(1, 0) == 3.0f);
		REQUIRE(m3.at(1, 1) == 2.0f);
		REQUIRE(m3.at(1, 2) == 1.0f);
		REQUIRE(m3.rows[0] == vec3(6.0f, 5.0f, 4.0f));
		REQUIRE(m3.rows[1] == vec3(3.0f, 2.0f, 1.0f));
		REQUIRE(m3.columnAt(0) == vec2(6.0f, 3.0f));
		REQUIRE(m3.columnAt(1) == vec2(5.0f, 2.0f));
		REQUIRE(m3.columnAt(2) == vec2(4.0f, 1.0f));
	}
}

TEST_CASE("Matrix<T,2,2> specialization", "[sfz::Matrix]")
{
	SECTION("Array pointer constructor") {
		const float arr1[] = {1.0f, 2.0f,
		                      3.0f, 4.0f};
		mat22 m1(arr1);
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e10 == 3.0f);
		REQUIRE(m1.e11 == 4.0f);
		REQUIRE(m1.rows[0] == vec2(1.0f, 2.0f));
		REQUIRE(m1.rows[1] == vec2(3.0f, 4.0f));
		REQUIRE(m1.columnAt(0) == vec2(1.0f, 3.0f));
		REQUIRE(m1.columnAt(1) == vec2(2.0f, 4.0f));
	}
	SECTION("Individual element constructor") {
		mat22 m1(1.0f, 2.0f,
		         3.0f, 4.0f);
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e10 == 3.0f);
		REQUIRE(m1.e11 == 4.0f);
		REQUIRE(m1.rows[0] == vec2(1.0f, 2.0f));
		REQUIRE(m1.rows[1] == vec2(3.0f, 4.0f));
		REQUIRE(m1.columnAt(0) == vec2(1.0f, 3.0f));
		REQUIRE(m1.columnAt(1) == vec2(2.0f, 4.0f));
	}
	SECTION("Row constructor") {
		mat22 m1(vec2(1.0f, 2.0f),
		         vec2(3.0f, 4.0f));
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e10 == 3.0f);
		REQUIRE(m1.e11 == 4.0f);
		REQUIRE(m1.rows[0] == vec2(1.0f, 2.0f));
		REQUIRE(m1.rows[1] == vec2(3.0f, 4.0f));
		REQUIRE(m1.columnAt(0) == vec2(1.0f, 3.0f));
		REQUIRE(m1.columnAt(1) == vec2(2.0f, 4.0f));
	}
	SECTION("fill() constructor function") {
		mat22 zero = mat22::fill(0.0f);
		REQUIRE(zero.at(0, 0) == 0.0f);
		REQUIRE(zero.at(0, 1) == 0.0f);
		REQUIRE(zero.at(0, 2) == 0.0f);
		REQUIRE(zero.at(0, 3) == 0.0f);
		
		mat22 one = mat22::fill(1.0f);
		REQUIRE(one.at(0, 0) == 1.0f);
		REQUIRE(one.at(0, 1) == 1.0f);
		REQUIRE(one.at(0, 2) == 1.0f);
		REQUIRE(one.at(0, 3) == 1.0f);
	}
	SECTION("identity() constructor function") {
		mat22 ident = mat22::identity();
		REQUIRE(ident.at(0, 0) == 1.0f);
		REQUIRE(ident.at(0, 1) == 0.0f);
		REQUIRE(ident.at(0, 2) == 0.0f);
		REQUIRE(ident.at(0, 3) == 1.0f);
	}
	SECTION("scaling2() constructor function") {
		mat22 scale = mat22::scaling2(2.0f);
		REQUIRE(scale.at(0, 0) == 2.0f);
		REQUIRE(scale.at(0, 1) == 0.0f);
		REQUIRE(scale.at(0, 2) == 0.0f);
		REQUIRE(scale.at(0, 3) == 2.0f);

		mat22 scale2 = mat22::scaling2(vec2(1.0f, 2.0f));
		REQUIRE(scale2.at(0, 0) == 1.0f);
		REQUIRE(scale2.at(0, 1) == 0.0f);
		REQUIRE(scale2.at(0, 2) == 0.0f);
		REQUIRE(scale2.at(0, 3) == 2.0f);
	}
}

TEST_CASE("Matrix<T,3,3> specialization", "[sfz::Matrix]")
{
	SECTION("Array pointer constructor") {
		const float arr1[] = {1.0f, 2.0f, 3.0f,
		                      4.0f, 5.0f, 6.0f,
		                      7.0f, 8.0f, 9.0f};
		mat33 m1(arr1);
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(1, 0) == 4.0f);
		REQUIRE(m1.at(1, 1) == 5.0f);
		REQUIRE(m1.at(1, 2) == 6.0f);
		REQUIRE(m1.at(2, 0) == 7.0f);
		REQUIRE(m1.at(2, 1) == 8.0f);
		REQUIRE(m1.at(2, 2) == 9.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e02 == 3.0f);
		REQUIRE(m1.e10 == 4.0f);
		REQUIRE(m1.e11 == 5.0f);
		REQUIRE(m1.e12 == 6.0f);
		REQUIRE(m1.e20 == 7.0f);
		REQUIRE(m1.e21 == 8.0f);
		REQUIRE(m1.e22 == 9.0f);
		REQUIRE(m1.rows[0] == vec3(1.0f, 2.0f, 3.0f));
		REQUIRE(m1.rows[1] == vec3(4.0f, 5.0f, 6.0f));
		REQUIRE(m1.rows[2] == vec3(7.0f, 8.0f, 9.0f));
		REQUIRE(m1.columnAt(0) == vec3(1.0f, 4.0f, 7.0f));
		REQUIRE(m1.columnAt(1) == vec3(2.0f, 5.0f, 8.0f));
		REQUIRE(m1.columnAt(2) == vec3(3.0f, 6.0f, 9.0f));
	}
	SECTION("Individual element constructor") {
		mat33 m1(1.0f, 2.0f, 3.0f,
		         4.0f, 5.0f, 6.0f,
		         7.0f, 8.0f, 9.0f);
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(1, 0) == 4.0f);
		REQUIRE(m1.at(1, 1) == 5.0f);
		REQUIRE(m1.at(1, 2) == 6.0f);
		REQUIRE(m1.at(2, 0) == 7.0f);
		REQUIRE(m1.at(2, 1) == 8.0f);
		REQUIRE(m1.at(2, 2) == 9.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e02 == 3.0f);
		REQUIRE(m1.e10 == 4.0f);
		REQUIRE(m1.e11 == 5.0f);
		REQUIRE(m1.e12 == 6.0f);
		REQUIRE(m1.e20 == 7.0f);
		REQUIRE(m1.e21 == 8.0f);
		REQUIRE(m1.e22 == 9.0f);
		REQUIRE(m1.rows[0] == vec3(1.0f, 2.0f, 3.0f));
		REQUIRE(m1.rows[1] == vec3(4.0f, 5.0f, 6.0f));
		REQUIRE(m1.rows[2] == vec3(7.0f, 8.0f, 9.0f));
		REQUIRE(m1.columnAt(0) == vec3(1.0f, 4.0f, 7.0f));
		REQUIRE(m1.columnAt(1) == vec3(2.0f, 5.0f, 8.0f));
		REQUIRE(m1.columnAt(2) == vec3(3.0f, 6.0f, 9.0f));
	}
	SECTION("Row constructor") {
		mat33 m1(vec3(1.0f, 2.0f, 3.0f),
		         vec3(4.0f, 5.0f, 6.0f),
		         vec3(7.0f, 8.0f, 9.0f));
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(1, 0) == 4.0f);
		REQUIRE(m1.at(1, 1) == 5.0f);
		REQUIRE(m1.at(1, 2) == 6.0f);
		REQUIRE(m1.at(2, 0) == 7.0f);
		REQUIRE(m1.at(2, 1) == 8.0f);
		REQUIRE(m1.at(2, 2) == 9.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e02 == 3.0f);
		REQUIRE(m1.e10 == 4.0f);
		REQUIRE(m1.e11 == 5.0f);
		REQUIRE(m1.e12 == 6.0f);
		REQUIRE(m1.e20 == 7.0f);
		REQUIRE(m1.e21 == 8.0f);
		REQUIRE(m1.e22 == 9.0f);
		REQUIRE(m1.rows[0] == vec3(1.0f, 2.0f, 3.0f));
		REQUIRE(m1.rows[1] == vec3(4.0f, 5.0f, 6.0f));
		REQUIRE(m1.rows[2] == vec3(7.0f, 8.0f, 9.0f));
		REQUIRE(m1.columnAt(0) == vec3(1.0f, 4.0f, 7.0f));
		REQUIRE(m1.columnAt(1) == vec3(2.0f, 5.0f, 8.0f));
		REQUIRE(m1.columnAt(2) == vec3(3.0f, 6.0f, 9.0f));
	}
	SECTION("3x4 matrix constructor") {
		mat34 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f);
		mat33 m2(m1);
		REQUIRE(m2.at(0, 0) == 1.0f);
		REQUIRE(m2.at(0, 1) == 2.0f);
		REQUIRE(m2.at(0, 2) == 3.0f);
		REQUIRE(m2.at(1, 0) == 5.0f);
		REQUIRE(m2.at(1, 1) == 6.0f);
		REQUIRE(m2.at(1, 2) == 7.0f);
		REQUIRE(m2.at(2, 0) == 9.0f);
		REQUIRE(m2.at(2, 1) == 10.0f);
		REQUIRE(m2.at(2, 2) == 11.0f);
	}
	SECTION("4x4 matrix constructor") {
		mat44 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f,
		         13.0f, 14.0f, 15.0f, 16.0f);
		mat33 m2(m1);
		REQUIRE(m2.at(0, 0) == 1.0f);
		REQUIRE(m2.at(0, 1) == 2.0f);
		REQUIRE(m2.at(0, 2) == 3.0f);
		REQUIRE(m2.at(1, 0) == 5.0f);
		REQUIRE(m2.at(1, 1) == 6.0f);
		REQUIRE(m2.at(1, 2) == 7.0f);
		REQUIRE(m2.at(2, 0) == 9.0f);
		REQUIRE(m2.at(2, 1) == 10.0f);
		REQUIRE(m2.at(2, 2) == 11.0f);
	}
	SECTION("fill() constructor function") {
		mat33 zero = mat33::fill(0.0f);
		REQUIRE(zero.at(0, 0) == 0.0f);
		REQUIRE(zero.at(0, 1) == 0.0f);
		REQUIRE(zero.at(0, 2) == 0.0f);
		REQUIRE(zero.at(1, 0) == 0.0f);
		REQUIRE(zero.at(1, 1) == 0.0f);
		REQUIRE(zero.at(1, 2) == 0.0f);
		REQUIRE(zero.at(2, 0) == 0.0f);
		REQUIRE(zero.at(2, 1) == 0.0f);
		REQUIRE(zero.at(2, 2) == 0.0f);
		
		mat33 one = mat33::fill(1.0f);
		REQUIRE(one.at(0, 0) == 1.0f);
		REQUIRE(one.at(0, 1) == 1.0f);
		REQUIRE(one.at(0, 2) == 1.0f);
		REQUIRE(one.at(1, 0) == 1.0f);
		REQUIRE(one.at(1, 1) == 1.0f);
		REQUIRE(one.at(1, 2) == 1.0f);
		REQUIRE(one.at(2, 0) == 1.0f);
		REQUIRE(one.at(2, 1) == 1.0f);
		REQUIRE(one.at(2, 2) == 1.0f);
	}
	SECTION("identity() constructor function") {
		mat33 ident = mat33::identity();
		REQUIRE(ident.at(0, 0) == 1.0f);
		REQUIRE(ident.at(0, 1) == 0.0f);
		REQUIRE(ident.at(0, 2) == 0.0f);
		REQUIRE(ident.at(1, 0) == 0.0f);
		REQUIRE(ident.at(1, 1) == 1.0f);
		REQUIRE(ident.at(1, 2) == 0.0f);
		REQUIRE(ident.at(2, 0) == 0.0f);
		REQUIRE(ident.at(2, 1) == 0.0f);
		REQUIRE(ident.at(2, 2) == 1.0f);
	}
	SECTION("scaling3() constructor function") {
		mat33 scale = mat33::scaling3(2.0f);
		REQUIRE(scale.at(0, 0) == 2.0f);
		REQUIRE(scale.at(0, 1) == 0.0f);
		REQUIRE(scale.at(0, 2) == 0.0f);
		REQUIRE(scale.at(1, 0) == 0.0f);
		REQUIRE(scale.at(1, 1) == 2.0f);
		REQUIRE(scale.at(1, 2) == 0.0f);
		REQUIRE(scale.at(2, 0) == 0.0f);
		REQUIRE(scale.at(2, 1) == 0.0f);
		REQUIRE(scale.at(2, 2) == 2.0f);

		mat33 scale2 = mat33::scaling3(vec3(1.0f, 2.0f, 3.0f));
		REQUIRE(scale2.at(0, 0) == 1.0f);
		REQUIRE(scale2.at(0, 1) == 0.0f);
		REQUIRE(scale2.at(0, 2) == 0.0f);
		REQUIRE(scale2.at(1, 0) == 0.0f);
		REQUIRE(scale2.at(1, 1) == 2.0f);
		REQUIRE(scale2.at(1, 2) == 0.0f);
		REQUIRE(scale2.at(2, 0) == 0.0f);
		REQUIRE(scale2.at(2, 1) == 0.0f);
		REQUIRE(scale2.at(2, 2) == 3.0f);
	}
	SECTION("rotation3() constructor function") {
		vec3 startPoint(1.0f, 0.0f, 0.0f);
		vec3 axis = vec3(1.0f, 1.0f, 0.0f);
		mat33 rot = mat33::rotation3(axis, PI);
		REQUIRE(approxEqual(rot * startPoint, vec3(0.0f, 1.0, 0.0f)));

		mat33 xRot90 = mat33::rotation3(vec3(1.0f, 0.0f, 0.0f), PI/2.0f);
		REQUIRE(approxEqual(xRot90, mat33(1.0f, 0.0f, 0.0f,
		                                  0.0f, 0.0f, -1.0f,
		                                  0.0f, 1.0f, 0.0f)));

		vec3 v = xRot90 * vec3(1.0f);
		REQUIRE(approxEqual(v, vec3(1.0f, -1.0f, 1.0f)));
	}
}

TEST_CASE("Matrix<T,3,4> specialization", "[sfz::Matrix]")
{
	SECTION("Array pointer constructor") {
		const float arr1[] = {1.0f, 2.0f, 3.0f, 4.0f,
		                      5.0f, 6.0f, 7.0f, 8.0f,
		                      9.0f, 10.0f, 11.0f, 12.0f};
		mat34 m1(arr1);
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.at(1, 0) == 5.0f);
		REQUIRE(m1.at(1, 1) == 6.0f);
		REQUIRE(m1.at(1, 2) == 7.0f);
		REQUIRE(m1.at(1, 3) == 8.0f);
		REQUIRE(m1.at(2, 0) == 9.0f);
		REQUIRE(m1.at(2, 1) == 10.0f);
		REQUIRE(m1.at(2, 2) == 11.0f);
		REQUIRE(m1.at(2, 3) == 12.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e02 == 3.0f);
		REQUIRE(m1.e03 == 4.0f);
		REQUIRE(m1.e10 == 5.0f);
		REQUIRE(m1.e11 == 6.0f);
		REQUIRE(m1.e12 == 7.0f);
		REQUIRE(m1.e13 == 8.0f);
		REQUIRE(m1.e20 == 9.0f);
		REQUIRE(m1.e21 == 10.0f);
		REQUIRE(m1.e22 == 11.0f);
		REQUIRE(m1.e23 == 12.0f);
		REQUIRE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		REQUIRE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		REQUIRE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		REQUIRE(m1.columnAt(0) == vec3(1.0f, 5.0f, 9.0f));
		REQUIRE(m1.columnAt(1) == vec3(2.0f, 6.0f, 10.0f));
		REQUIRE(m1.columnAt(2) == vec3(3.0f, 7.0f, 11.0f));
		REQUIRE(m1.columnAt(3) == vec3(4.0f, 8.0f, 12.0f));
	}
	SECTION("Individual element constructor") {
		mat34 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f);
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.at(1, 0) == 5.0f);
		REQUIRE(m1.at(1, 1) == 6.0f);
		REQUIRE(m1.at(1, 2) == 7.0f);
		REQUIRE(m1.at(1, 3) == 8.0f);
		REQUIRE(m1.at(2, 0) == 9.0f);
		REQUIRE(m1.at(2, 1) == 10.0f);
		REQUIRE(m1.at(2, 2) == 11.0f);
		REQUIRE(m1.at(2, 3) == 12.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e02 == 3.0f);
		REQUIRE(m1.e03 == 4.0f);
		REQUIRE(m1.e10 == 5.0f);
		REQUIRE(m1.e11 == 6.0f);
		REQUIRE(m1.e12 == 7.0f);
		REQUIRE(m1.e13 == 8.0f);
		REQUIRE(m1.e20 == 9.0f);
		REQUIRE(m1.e21 == 10.0f);
		REQUIRE(m1.e22 == 11.0f);
		REQUIRE(m1.e23 == 12.0f);
		REQUIRE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		REQUIRE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		REQUIRE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		REQUIRE(m1.columnAt(0) == vec3(1.0f, 5.0f, 9.0f));
		REQUIRE(m1.columnAt(1) == vec3(2.0f, 6.0f, 10.0f));
		REQUIRE(m1.columnAt(2) == vec3(3.0f, 7.0f, 11.0f));
		REQUIRE(m1.columnAt(3) == vec3(4.0f, 8.0f, 12.0f));
	}
	SECTION("Row constructor") {
		mat34 m1(vec4(1.0f, 2.0f, 3.0f, 4.0f),
		         vec4(5.0f, 6.0f, 7.0f, 8.0f),
		         vec4(9.0f, 10.0f, 11.0f, 12.0f));
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.at(1, 0) == 5.0f);
		REQUIRE(m1.at(1, 1) == 6.0f);
		REQUIRE(m1.at(1, 2) == 7.0f);
		REQUIRE(m1.at(1, 3) == 8.0f);
		REQUIRE(m1.at(2, 0) == 9.0f);
		REQUIRE(m1.at(2, 1) == 10.0f);
		REQUIRE(m1.at(2, 2) == 11.0f);
		REQUIRE(m1.at(2, 3) == 12.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e02 == 3.0f);
		REQUIRE(m1.e03 == 4.0f);
		REQUIRE(m1.e10 == 5.0f);
		REQUIRE(m1.e11 == 6.0f);
		REQUIRE(m1.e12 == 7.0f);
		REQUIRE(m1.e13 == 8.0f);
		REQUIRE(m1.e20 == 9.0f);
		REQUIRE(m1.e21 == 10.0f);
		REQUIRE(m1.e22 == 11.0f);
		REQUIRE(m1.e23 == 12.0f);
		REQUIRE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		REQUIRE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		REQUIRE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		REQUIRE(m1.columnAt(0) == vec3(1.0f, 5.0f, 9.0f));
		REQUIRE(m1.columnAt(1) == vec3(2.0f, 6.0f, 10.0f));
		REQUIRE(m1.columnAt(2) == vec3(3.0f, 7.0f, 11.0f));
		REQUIRE(m1.columnAt(3) == vec3(4.0f, 8.0f, 12.0f));
	}
	SECTION("3x3 matrix constructor") {
		mat33 m1(1.0f, 2.0f, 3.0f,
		         4.0f, 5.0f, 6.0f,
		         7.0f, 8.0f, 9.0f);
		mat34 m2(m1);
		REQUIRE(m2.at(0, 0) == 1.0f);
		REQUIRE(m2.at(0, 1) == 2.0f);
		REQUIRE(m2.at(0, 2) == 3.0f);
		REQUIRE(m2.at(0, 3) == 0.0f);
		REQUIRE(m2.at(1, 0) == 4.0f);
		REQUIRE(m2.at(1, 1) == 5.0f);
		REQUIRE(m2.at(1, 2) == 6.0f);
		REQUIRE(m2.at(1, 3) == 0.0f);
		REQUIRE(m2.at(2, 0) == 7.0f);
		REQUIRE(m2.at(2, 1) == 8.0f);
		REQUIRE(m2.at(2, 2) == 9.0f);
		REQUIRE(m2.at(2, 3) == 0.0f);
	}
	SECTION("4x4 matrix constructor") {
		mat44 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f,
		         13.0f, 14.0f, 15.0f, 16.0f);
		mat34 m2(m1);
		REQUIRE(m2.at(0, 0) == 1.0f);
		REQUIRE(m2.at(0, 1) == 2.0f);
		REQUIRE(m2.at(0, 2) == 3.0f);
		REQUIRE(m2.at(0, 3) == 4.0f);
		REQUIRE(m2.at(1, 0) == 5.0f);
		REQUIRE(m2.at(1, 1) == 6.0f);
		REQUIRE(m2.at(1, 2) == 7.0f);
		REQUIRE(m2.at(1, 3) == 8.0f);
		REQUIRE(m2.at(2, 0) == 9.0f);
		REQUIRE(m2.at(2, 1) == 10.0f);
		REQUIRE(m2.at(2, 2) == 11.0f);
		REQUIRE(m2.at(2, 3) == 12.0f);
	}
	SECTION("fill() constructor function") {
		mat34 zero = mat34::fill(0.0f);
		REQUIRE(zero.at(0, 0) == 0.0f);
		REQUIRE(zero.at(0, 1) == 0.0f);
		REQUIRE(zero.at(0, 2) == 0.0f);
		REQUIRE(zero.at(0, 3) == 0.0f);
		REQUIRE(zero.at(1, 0) == 0.0f);
		REQUIRE(zero.at(1, 1) == 0.0f);
		REQUIRE(zero.at(1, 2) == 0.0f);
		REQUIRE(zero.at(1, 3) == 0.0f);
		REQUIRE(zero.at(2, 0) == 0.0f);
		REQUIRE(zero.at(2, 1) == 0.0f);
		REQUIRE(zero.at(2, 2) == 0.0f);
		REQUIRE(zero.at(2, 3) == 0.0f);

		mat34 one = mat34::fill(1.0f);
		REQUIRE(one.at(0, 0) == 1.0f);
		REQUIRE(one.at(0, 1) == 1.0f);
		REQUIRE(one.at(0, 2) == 1.0f);
		REQUIRE(one.at(0, 3) == 1.0f);
		REQUIRE(one.at(1, 0) == 1.0f);
		REQUIRE(one.at(1, 1) == 1.0f);
		REQUIRE(one.at(1, 2) == 1.0f);
		REQUIRE(one.at(1, 3) == 1.0f);
		REQUIRE(one.at(2, 0) == 1.0f);
		REQUIRE(one.at(2, 1) == 1.0f);
		REQUIRE(one.at(2, 2) == 1.0f);
		REQUIRE(one.at(2, 3) == 1.0f);
	}
	SECTION("identity() constructor function") {
		mat34 ident = mat34::identity();
		REQUIRE(ident.at(0, 0) == 1.0f);
		REQUIRE(ident.at(0, 1) == 0.0f);
		REQUIRE(ident.at(0, 2) == 0.0f);
		REQUIRE(ident.at(0, 3) == 0.0f);
		REQUIRE(ident.at(1, 0) == 0.0f);
		REQUIRE(ident.at(1, 1) == 1.0f);
		REQUIRE(ident.at(1, 2) == 0.0f);
		REQUIRE(ident.at(1, 3) == 0.0f);
		REQUIRE(ident.at(2, 0) == 0.0f);
		REQUIRE(ident.at(2, 1) == 0.0f);
		REQUIRE(ident.at(2, 2) == 1.0f);
		REQUIRE(ident.at(2, 3) == 0.0f);
	}
	SECTION("scaling3() constructor function") {
		mat34 scale = mat34::scaling3(2.0f);
		REQUIRE(scale.at(0, 0) == 2.0f);
		REQUIRE(scale.at(0, 1) == 0.0f);
		REQUIRE(scale.at(0, 2) == 0.0f);
		REQUIRE(scale.at(0, 3) == 0.0f);
		REQUIRE(scale.at(1, 0) == 0.0f);
		REQUIRE(scale.at(1, 1) == 2.0f);
		REQUIRE(scale.at(1, 2) == 0.0f);
		REQUIRE(scale.at(1, 3) == 0.0f);
		REQUIRE(scale.at(2, 0) == 0.0f);
		REQUIRE(scale.at(2, 1) == 0.0f);
		REQUIRE(scale.at(2, 2) == 2.0f);
		REQUIRE(scale.at(2, 3) == 0.0f);

		mat34 scale2 = mat34::scaling3(vec3(1.0f, 2.0f, 3.0f));
		REQUIRE(scale2.at(0, 0) == 1.0f);
		REQUIRE(scale2.at(0, 1) == 0.0f);
		REQUIRE(scale2.at(0, 2) == 0.0f);
		REQUIRE(scale2.at(0, 3) == 0.0f);
		REQUIRE(scale2.at(1, 0) == 0.0f);
		REQUIRE(scale2.at(1, 1) == 2.0f);
		REQUIRE(scale2.at(1, 2) == 0.0f);
		REQUIRE(scale2.at(1, 3) == 0.0f);
		REQUIRE(scale2.at(2, 0) == 0.0f);
		REQUIRE(scale2.at(2, 1) == 0.0f);
		REQUIRE(scale2.at(2, 2) == 3.0f);
		REQUIRE(scale2.at(2, 3) == 0.0f);
	}
	SECTION("rotation3() constructor function") {
		vec3 startPoint(1.0f, 0.0f, 0.0f);
		vec3 axis = vec3(1.0f, 1.0f, 0.0f);
		mat34 rot = mat34::rotation3(axis, PI);
		REQUIRE(approxEqual(transformPoint(rot, startPoint), vec3(0.0f, 1.0, 0.0f)));

		mat34 xRot90 = mat34::rotation3(vec3(1.0f, 0.0f, 0.0f), PI/2.0f);
		REQUIRE(approxEqual(xRot90, mat34(1.0f, 0.0f, 0.0f, 0.0f,
		                                  0.0f, 0.0f, -1.0f, 0.0f,
		                                  0.0f, 1.0f, 0.0f, 0.0f)));

		vec3 v = transformPoint(xRot90, vec3(1.0f));
		REQUIRE(approxEqual(v, vec3(1.0f, -1.0f, 1.0f)));
	}
	SECTION("translation3() constructor function") {
		vec4 v1(1.0f, 1.0f, 1.0f, 1.0f);
		mat44 m = mat44::translation3(vec3(-2.0f, 1.0f, 0.0f));
		REQUIRE(approxEqual(m.at(0, 0), 1.0f));
		REQUIRE(approxEqual(m.at(0, 1), 0.0f));
		REQUIRE(approxEqual(m.at(0, 2), 0.0f));
		REQUIRE(approxEqual(m.at(0, 3), -2.0f));
		REQUIRE(approxEqual(m.at(1, 0), 0.0f));
		REQUIRE(approxEqual(m.at(1, 1), 1.0f));
		REQUIRE(approxEqual(m.at(1, 2), 0.0f));
		REQUIRE(approxEqual(m.at(1, 3), 1.0f));
		REQUIRE(approxEqual(m.at(2, 0), 0.0f));
		REQUIRE(approxEqual(m.at(2, 1), 0.0f));
		REQUIRE(approxEqual(m.at(2, 2), 1.0f));
		REQUIRE(approxEqual(m.at(2, 3), 0.0f));
		REQUIRE(approxEqual(m.at(3, 0), 0.0f));
		REQUIRE(approxEqual(m.at(3, 1), 0.0f));
		REQUIRE(approxEqual(m.at(3, 2), 0.0f));
		REQUIRE(approxEqual(m.at(3, 3), 1.0f));
		vec4 v2 = m * v1;
		REQUIRE(approxEqual(v2.x, -1.0f));
		REQUIRE(approxEqual(v2.y, 2.0f));
		REQUIRE(approxEqual(v2.z, 1.0f));
		REQUIRE(approxEqual(v2.w, 1.0f));
	}
}

TEST_CASE("Matrix<T,4,4> specialization", "[sfz::Matrix]")
{
	SECTION("Array pointer constructor") {
		const float arr1[] = {1.0f, 2.0f, 3.0f, 4.0f,
		                      5.0f, 6.0f, 7.0f, 8.0f,
		                      9.0f, 10.0f, 11.0f, 12.0f,
		                      13.0f, 14.0f, 15.0f, 16.0f};
		mat44 m1(arr1);
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.at(1, 0) == 5.0f);
		REQUIRE(m1.at(1, 1) == 6.0f);
		REQUIRE(m1.at(1, 2) == 7.0f);
		REQUIRE(m1.at(1, 3) == 8.0f);
		REQUIRE(m1.at(2, 0) == 9.0f);
		REQUIRE(m1.at(2, 1) == 10.0f);
		REQUIRE(m1.at(2, 2) == 11.0f);
		REQUIRE(m1.at(2, 3) == 12.0f);
		REQUIRE(m1.at(3, 0) == 13.0f);
		REQUIRE(m1.at(3, 1) == 14.0f);
		REQUIRE(m1.at(3, 2) == 15.0f);
		REQUIRE(m1.at(3, 3) == 16.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e02 == 3.0f);
		REQUIRE(m1.e03 == 4.0f);
		REQUIRE(m1.e10 == 5.0f);
		REQUIRE(m1.e11 == 6.0f);
		REQUIRE(m1.e12 == 7.0f);
		REQUIRE(m1.e13 == 8.0f);
		REQUIRE(m1.e20 == 9.0f);
		REQUIRE(m1.e21 == 10.0f);
		REQUIRE(m1.e22 == 11.0f);
		REQUIRE(m1.e23 == 12.0f);
		REQUIRE(m1.e30 == 13.0f);
		REQUIRE(m1.e31 == 14.0f);
		REQUIRE(m1.e32 == 15.0f);
		REQUIRE(m1.e33 == 16.0f);
		REQUIRE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		REQUIRE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		REQUIRE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		REQUIRE(m1.rows[3] == vec4(13.0f, 14.0f, 15.0f, 16.0f));
		REQUIRE(m1.columnAt(0) == vec4(1.0f, 5.0f, 9.0f, 13.0f));
		REQUIRE(m1.columnAt(1) == vec4(2.0f, 6.0f, 10.0f, 14.0f));
		REQUIRE(m1.columnAt(2) == vec4(3.0f, 7.0f, 11.0f, 15.0f));
		REQUIRE(m1.columnAt(3) == vec4(4.0f, 8.0f, 12.0f, 16.0f));
	}
	SECTION("Individual element constructor") {
		mat44 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f,
		         13.0f, 14.0f, 15.0f, 16.0f);
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.at(1, 0) == 5.0f);
		REQUIRE(m1.at(1, 1) == 6.0f);
		REQUIRE(m1.at(1, 2) == 7.0f);
		REQUIRE(m1.at(1, 3) == 8.0f);
		REQUIRE(m1.at(2, 0) == 9.0f);
		REQUIRE(m1.at(2, 1) == 10.0f);
		REQUIRE(m1.at(2, 2) == 11.0f);
		REQUIRE(m1.at(2, 3) == 12.0f);
		REQUIRE(m1.at(3, 0) == 13.0f);
		REQUIRE(m1.at(3, 1) == 14.0f);
		REQUIRE(m1.at(3, 2) == 15.0f);
		REQUIRE(m1.at(3, 3) == 16.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e02 == 3.0f);
		REQUIRE(m1.e03 == 4.0f);
		REQUIRE(m1.e10 == 5.0f);
		REQUIRE(m1.e11 == 6.0f);
		REQUIRE(m1.e12 == 7.0f);
		REQUIRE(m1.e13 == 8.0f);
		REQUIRE(m1.e20 == 9.0f);
		REQUIRE(m1.e21 == 10.0f);
		REQUIRE(m1.e22 == 11.0f);
		REQUIRE(m1.e23 == 12.0f);
		REQUIRE(m1.e30 == 13.0f);
		REQUIRE(m1.e31 == 14.0f);
		REQUIRE(m1.e32 == 15.0f);
		REQUIRE(m1.e33 == 16.0f);
		REQUIRE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		REQUIRE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		REQUIRE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		REQUIRE(m1.rows[3] == vec4(13.0f, 14.0f, 15.0f, 16.0f));
		REQUIRE(m1.columnAt(0) == vec4(1.0f, 5.0f, 9.0f, 13.0f));
		REQUIRE(m1.columnAt(1) == vec4(2.0f, 6.0f, 10.0f, 14.0f));
		REQUIRE(m1.columnAt(2) == vec4(3.0f, 7.0f, 11.0f, 15.0f));
		REQUIRE(m1.columnAt(3) == vec4(4.0f, 8.0f, 12.0f, 16.0f));
	}
	SECTION("Row constructor") {
		mat44 m1(vec4(1.0f, 2.0f, 3.0f, 4.0f),
		         vec4(5.0f, 6.0f, 7.0f, 8.0f),
		         vec4(9.0f, 10.0f, 11.0f, 12.0f),
		         vec4(13.0f, 14.0f, 15.0f, 16.0f));
		REQUIRE(m1.at(0, 0) == 1.0f);
		REQUIRE(m1.at(0, 1) == 2.0f);
		REQUIRE(m1.at(0, 2) == 3.0f);
		REQUIRE(m1.at(0, 3) == 4.0f);
		REQUIRE(m1.at(1, 0) == 5.0f);
		REQUIRE(m1.at(1, 1) == 6.0f);
		REQUIRE(m1.at(1, 2) == 7.0f);
		REQUIRE(m1.at(1, 3) == 8.0f);
		REQUIRE(m1.at(2, 0) == 9.0f);
		REQUIRE(m1.at(2, 1) == 10.0f);
		REQUIRE(m1.at(2, 2) == 11.0f);
		REQUIRE(m1.at(2, 3) == 12.0f);
		REQUIRE(m1.at(3, 0) == 13.0f);
		REQUIRE(m1.at(3, 1) == 14.0f);
		REQUIRE(m1.at(3, 2) == 15.0f);
		REQUIRE(m1.at(3, 3) == 16.0f);
		REQUIRE(m1.e00 == 1.0f);
		REQUIRE(m1.e01 == 2.0f);
		REQUIRE(m1.e02 == 3.0f);
		REQUIRE(m1.e03 == 4.0f);
		REQUIRE(m1.e10 == 5.0f);
		REQUIRE(m1.e11 == 6.0f);
		REQUIRE(m1.e12 == 7.0f);
		REQUIRE(m1.e13 == 8.0f);
		REQUIRE(m1.e20 == 9.0f);
		REQUIRE(m1.e21 == 10.0f);
		REQUIRE(m1.e22 == 11.0f);
		REQUIRE(m1.e23 == 12.0f);
		REQUIRE(m1.e30 == 13.0f);
		REQUIRE(m1.e31 == 14.0f);
		REQUIRE(m1.e32 == 15.0f);
		REQUIRE(m1.e33 == 16.0f);
		REQUIRE(m1.rows[0] == vec4(1.0f, 2.0f, 3.0f, 4.0f));
		REQUIRE(m1.rows[1] == vec4(5.0f, 6.0f, 7.0f, 8.0f));
		REQUIRE(m1.rows[2] == vec4(9.0f, 10.0f, 11.0f, 12.0f));
		REQUIRE(m1.rows[3] == vec4(13.0f, 14.0f, 15.0f, 16.0f));
		REQUIRE(m1.columnAt(0) == vec4(1.0f, 5.0f, 9.0f, 13.0f));
		REQUIRE(m1.columnAt(1) == vec4(2.0f, 6.0f, 10.0f, 14.0f));
		REQUIRE(m1.columnAt(2) == vec4(3.0f, 7.0f, 11.0f, 15.0f));
		REQUIRE(m1.columnAt(3) == vec4(4.0f, 8.0f, 12.0f, 16.0f));
	}
	SECTION("3x3 matrix constructor") {
		mat33 m1(1.0f, 2.0f, 3.0f,
		         4.0f, 5.0f, 6.0f,
		         7.0f, 8.0f, 9.0f);
		mat44 m2(m1);
		REQUIRE(m2.at(0, 0) == 1.0f);
		REQUIRE(m2.at(0, 1) == 2.0f);
		REQUIRE(m2.at(0, 2) == 3.0f);
		REQUIRE(m2.at(0, 3) == 0.0f);
		REQUIRE(m2.at(1, 0) == 4.0f);
		REQUIRE(m2.at(1, 1) == 5.0f);
		REQUIRE(m2.at(1, 2) == 6.0f);
		REQUIRE(m2.at(1, 3) == 0.0f);
		REQUIRE(m2.at(2, 0) == 7.0f);
		REQUIRE(m2.at(2, 1) == 8.0f);
		REQUIRE(m2.at(2, 2) == 9.0f);
		REQUIRE(m2.at(2, 3) == 0.0f);
		REQUIRE(m2.at(3, 0) == 0.0f);
		REQUIRE(m2.at(3, 1) == 0.0f);
		REQUIRE(m2.at(3, 2) == 0.0f);
		REQUIRE(m2.at(3, 3) == 1.0f);
	}
	SECTION("4x3 matrix constructor") {
		mat34 m1(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f);
		mat44 m2(m1);
		REQUIRE(m2.at(0, 0) == 1.0f);
		REQUIRE(m2.at(0, 1) == 2.0f);
		REQUIRE(m2.at(0, 2) == 3.0f);
		REQUIRE(m2.at(0, 3) == 4.0f);
		REQUIRE(m2.at(1, 0) == 5.0f);
		REQUIRE(m2.at(1, 1) == 6.0f);
		REQUIRE(m2.at(1, 2) == 7.0f);
		REQUIRE(m2.at(1, 3) == 8.0f);
		REQUIRE(m2.at(2, 0) == 9.0f);
		REQUIRE(m2.at(2, 1) == 10.0f);
		REQUIRE(m2.at(2, 2) == 11.0f);
		REQUIRE(m2.at(2, 3) == 12.0f);
		REQUIRE(m2.at(3, 0) == 0.0f);
		REQUIRE(m2.at(3, 1) == 0.0f);
		REQUIRE(m2.at(3, 2) == 0.0f);
		REQUIRE(m2.at(3, 3) == 1.0f);
	}
	SECTION("fill() constructor function") {
		mat44 zero = mat44::fill(0.0f);
		REQUIRE(zero.at(0, 0) == 0.0f);
		REQUIRE(zero.at(0, 1) == 0.0f);
		REQUIRE(zero.at(0, 2) == 0.0f);
		REQUIRE(zero.at(0, 3) == 0.0f);
		REQUIRE(zero.at(1, 0) == 0.0f);
		REQUIRE(zero.at(1, 1) == 0.0f);
		REQUIRE(zero.at(1, 2) == 0.0f);
		REQUIRE(zero.at(1, 3) == 0.0f);
		REQUIRE(zero.at(2, 0) == 0.0f);
		REQUIRE(zero.at(2, 1) == 0.0f);
		REQUIRE(zero.at(2, 2) == 0.0f);
		REQUIRE(zero.at(2, 3) == 0.0f);
		REQUIRE(zero.at(3, 0) == 0.0f);
		REQUIRE(zero.at(3, 1) == 0.0f);
		REQUIRE(zero.at(3, 2) == 0.0f);
		REQUIRE(zero.at(3, 3) == 0.0f);

		mat44 one = mat44::fill(1.0f);
		REQUIRE(one.at(0, 0) == 1.0f);
		REQUIRE(one.at(0, 1) == 1.0f);
		REQUIRE(one.at(0, 2) == 1.0f);
		REQUIRE(one.at(0, 3) == 1.0f);
		REQUIRE(one.at(1, 0) == 1.0f);
		REQUIRE(one.at(1, 1) == 1.0f);
		REQUIRE(one.at(1, 2) == 1.0f);
		REQUIRE(one.at(1, 3) == 1.0f);
		REQUIRE(one.at(2, 0) == 1.0f);
		REQUIRE(one.at(2, 1) == 1.0f);
		REQUIRE(one.at(2, 2) == 1.0f);
		REQUIRE(one.at(2, 3) == 1.0f);
		REQUIRE(one.at(3, 0) == 1.0f);
		REQUIRE(one.at(3, 1) == 1.0f);
		REQUIRE(one.at(3, 2) == 1.0f);
		REQUIRE(one.at(3, 3) == 1.0f);
	}
	SECTION("identity() constructor function") {
		mat44 ident = mat44::identity();
		REQUIRE(ident.at(0, 0) == 1.0f);
		REQUIRE(ident.at(0, 1) == 0.0f);
		REQUIRE(ident.at(0, 2) == 0.0f);
		REQUIRE(ident.at(0, 3) == 0.0f);
		REQUIRE(ident.at(1, 0) == 0.0f);
		REQUIRE(ident.at(1, 1) == 1.0f);
		REQUIRE(ident.at(1, 2) == 0.0f);
		REQUIRE(ident.at(1, 3) == 0.0f);
		REQUIRE(ident.at(2, 0) == 0.0f);
		REQUIRE(ident.at(2, 1) == 0.0f);
		REQUIRE(ident.at(2, 2) == 1.0f);
		REQUIRE(ident.at(2, 3) == 0.0f);
		REQUIRE(ident.at(3, 0) == 0.0f);
		REQUIRE(ident.at(3, 1) == 0.0f);
		REQUIRE(ident.at(3, 2) == 0.0f);
		REQUIRE(ident.at(3, 3) == 1.0f);
	}
	SECTION("scaling3() constructor function") {
		mat44 scale = mat44::scaling3(2.0f);
		REQUIRE(scale.at(0, 0) == 2.0f);
		REQUIRE(scale.at(0, 1) == 0.0f);
		REQUIRE(scale.at(0, 2) == 0.0f);
		REQUIRE(scale.at(0, 3) == 0.0f);
		REQUIRE(scale.at(1, 0) == 0.0f);
		REQUIRE(scale.at(1, 1) == 2.0f);
		REQUIRE(scale.at(1, 2) == 0.0f);
		REQUIRE(scale.at(1, 3) == 0.0f);
		REQUIRE(scale.at(2, 0) == 0.0f);
		REQUIRE(scale.at(2, 1) == 0.0f);
		REQUIRE(scale.at(2, 2) == 2.0f);
		REQUIRE(scale.at(2, 3) == 0.0f);
		REQUIRE(scale.at(3, 0) == 0.0f);
		REQUIRE(scale.at(3, 1) == 0.0f);
		REQUIRE(scale.at(3, 2) == 0.0f);
		REQUIRE(scale.at(3, 3) == 1.0f);

		mat44 scale2 = mat44::scaling3(vec3(1.0f, 2.0f, 3.0f));
		REQUIRE(scale2.at(0, 0) == 1.0f);
		REQUIRE(scale2.at(0, 1) == 0.0f);
		REQUIRE(scale2.at(0, 2) == 0.0f);
		REQUIRE(scale2.at(0, 3) == 0.0f);
		REQUIRE(scale2.at(1, 0) == 0.0f);
		REQUIRE(scale2.at(1, 1) == 2.0f);
		REQUIRE(scale2.at(1, 2) == 0.0f);
		REQUIRE(scale2.at(1, 3) == 0.0f);
		REQUIRE(scale2.at(2, 0) == 0.0f);
		REQUIRE(scale2.at(2, 1) == 0.0f);
		REQUIRE(scale2.at(2, 2) == 3.0f);
		REQUIRE(scale2.at(2, 3) == 0.0f);
		REQUIRE(scale2.at(3, 0) == 0.0f);
		REQUIRE(scale2.at(3, 1) == 0.0f);
		REQUIRE(scale2.at(3, 2) == 0.0f);
		REQUIRE(scale2.at(3, 3) == 1.0f);
	}
	SECTION("rotation3() constructor function") {
		vec4 startPoint(1.0f, 0.0f, 0.0f, 1.0f);
		vec3 axis = vec3(1.0f, 1.0f, 0.0f);
		mat44 rot = mat44::rotation3(axis, PI);
		REQUIRE(approxEqual(rot * startPoint, vec4(0.0f, 1.0, 0.0f, 1.0f)));

		mat44 xRot90 = mat44::rotation3(vec3(1.0f, 0.0f, 0.0f), PI/2.0f);
		REQUIRE(approxEqual(xRot90, mat44(1.0f, 0.0f, 0.0f, 0.0f,
		                                  0.0f, 0.0f, -1.0f, 0.0f,
		                                  0.0f, 1.0f, 0.0f, 0.0f,
		                                  0.0f, 0.0f, 0.0f, 1.0f)));

		vec4 v = xRot90 * vec4(1.0f);
		REQUIRE(approxEqual(v, vec4(1.0f, -1.0f, 1.0f, 1.0f)));
	}
	SECTION("translation3() constructor function") {
		vec4 v1(1.0f, 1.0f, 1.0f, 1.0f);
		mat44 m = mat44::translation3(vec3(-2.0f, 1.0f, 0.0f));
		REQUIRE(approxEqual(m.at(0, 0), 1.0f));
		REQUIRE(approxEqual(m.at(0, 1), 0.0f));
		REQUIRE(approxEqual(m.at(0, 2), 0.0f));
		REQUIRE(approxEqual(m.at(0, 3), -2.0f));
		REQUIRE(approxEqual(m.at(1, 0), 0.0f));
		REQUIRE(approxEqual(m.at(1, 1), 1.0f));
		REQUIRE(approxEqual(m.at(1, 2), 0.0f));
		REQUIRE(approxEqual(m.at(1, 3), 1.0f));
		REQUIRE(approxEqual(m.at(2, 0), 0.0f));
		REQUIRE(approxEqual(m.at(2, 1), 0.0f));
		REQUIRE(approxEqual(m.at(2, 2), 1.0f));
		REQUIRE(approxEqual(m.at(2, 3), 0.0f));
		REQUIRE(approxEqual(m.at(3, 0), 0.0f));
		REQUIRE(approxEqual(m.at(3, 1), 0.0f));
		REQUIRE(approxEqual(m.at(3, 2), 0.0f));
		REQUIRE(approxEqual(m.at(3, 3), 1.0f));
		vec4 v2 = m * v1;
		REQUIRE(approxEqual(v2.x, -1.0f));
		REQUIRE(approxEqual(v2.y, 2.0f));
		REQUIRE(approxEqual(v2.z, 1.0f));
		REQUIRE(approxEqual(v2.w, 1.0f));
	}
}

TEST_CASE("Arhitmetic & assignment operators", "[sfz::Matrix]")
{
	mat22 m1(1.0f, 2.0f,
	         3.0f, 4.0f);
	mat22 m2(1.0f, 2.0f,
	         3.0f, 4.0f);
	mat22 m3(-2.0f, -1.0f,
	         3.0f, 33.0f);

	SECTION("+=") {
		m1 += m2;
		m2 += m3;

		REQUIRE(approxEqual(m1.at(0, 0), 2.0f));
		REQUIRE(approxEqual(m1.at(0, 1), 4.0f));
		REQUIRE(approxEqual(m1.at(1, 0), 6.0f));
		REQUIRE(approxEqual(m1.at(1, 1), 8.0f));

		REQUIRE(approxEqual(m2.at(0, 0), -1.0f));
		REQUIRE(approxEqual(m2.at(0, 1), 1.0f));
		REQUIRE(approxEqual(m2.at(1, 0), 6.0f));
		REQUIRE(approxEqual(m2.at(1, 1), 37.0f));
	}
	SECTION("-=") {
		m1 -= m2;
		m2 -= m3;

		REQUIRE(approxEqual(m1.at(0, 0), 0.0f));
		REQUIRE(approxEqual(m1.at(0, 1), 0.0f));
		REQUIRE(approxEqual(m1.at(1, 0), 0.0f));
		REQUIRE(approxEqual(m1.at(1, 1), 0.0f));

		REQUIRE(approxEqual(m2.at(0, 0), 3.0f));
		REQUIRE(approxEqual(m2.at(0, 1), 3.0f));
		REQUIRE(approxEqual(m2.at(1, 0), 0.0f));
		REQUIRE(approxEqual(m2.at(1, 1), -29.0f));
	}
	SECTION("*= (scalar)") {
		m1 *= 2.0f;
		REQUIRE(approxEqual(m1.at(0, 0), 2.0f));
		REQUIRE(approxEqual(m1.at(0, 1), 4.0f));
		REQUIRE(approxEqual(m1.at(1, 0), 6.0f));
		REQUIRE(approxEqual(m1.at(1, 1), 8.0f));

		m3 *= -1.0f;
		REQUIRE(approxEqual(m3.at(0, 0), 2.0f));
		REQUIRE(approxEqual(m3.at(0, 1), 1.0f));
		REQUIRE(approxEqual(m3.at(1, 0), -3.0f));
		REQUIRE(approxEqual(m3.at(1, 1), -33.0f));
	}
	SECTION("*= (matrix of same size)") {
		mat22 m4(1.0f, 0.0f,
		         0.0f, 1.0f);
		auto m1cpy = m1;
		m1cpy *= m4;

		REQUIRE(approxEqual(m1cpy.at(0, 0), 1.0f));
		REQUIRE(approxEqual(m1cpy.at(0, 1), 2.0f));
		REQUIRE(approxEqual(m1cpy.at(1, 0), 3.0f));
		REQUIRE(approxEqual(m1cpy.at(1, 1), 4.0f));

		m4 *= m1;
		REQUIRE(approxEqual(m4.at(0, 0), 1.0f));
		REQUIRE(approxEqual(m4.at(0, 1), 2.0f));
		REQUIRE(approxEqual(m4.at(1, 0), 3.0f));
		REQUIRE(approxEqual(m4.at(1, 1), 4.0f));
	}
}

TEST_CASE("Arhitmetic operators", "[sfz::Matrix]")
{
	mat22 m1(1.0f, 2.0f,
	         3.0f, 4.0f);
	mat22 m2(0.0f, 1.0f,
	         0.0f, 0.0f);
	float m3arr[] = {1.0f, 2.0f, 3.0f,
	                 4.0f, 5.0f, 6.0f};
	Matrix<float,2,3> m3(m3arr);
	float m4arr[] = {1.0f, 0.0f,
	                 0.0f, 1.0f,
	                 0.0f, 0.0f};
	Matrix<float,3,2> m4(m4arr);

	SECTION("+") {
		auto res1 = m1 + m2;
		REQUIRE(approxEqual(res1.at(0, 0), 1.0f));
		REQUIRE(approxEqual(res1.at(0, 1), 3.0f));
		REQUIRE(approxEqual(res1.at(1, 0), 3.0f));
		REQUIRE(approxEqual(res1.at(1, 1), 4.0f));

		auto res2 = m3 + m3;
		REQUIRE(approxEqual(res2.at(0, 0), 2.0f));
		REQUIRE(approxEqual(res2.at(0, 1), 4.0f));
		REQUIRE(approxEqual(res2.at(0, 2), 6.0f));
		REQUIRE(approxEqual(res2.at(1, 0), 8.0f));
		REQUIRE(approxEqual(res2.at(1, 1), 10.0f));
		REQUIRE(approxEqual(res2.at(1, 2), 12.0f));
	}
	SECTION("-") {
		auto res1 = m1 - m2;
		auto res2 = m2 - m1;

		REQUIRE(res1 != res2);
		
		REQUIRE(approxEqual(res1.at(0, 0), 1.0f));
		REQUIRE(approxEqual(res1.at(0, 1), 1.0f));
		REQUIRE(approxEqual(res1.at(1, 0), 3.0f));
		REQUIRE(approxEqual(res1.at(1, 1), 4.0f));

		REQUIRE(approxEqual(res2.at(0, 0), -1.0f));
		REQUIRE(approxEqual(res2.at(0, 1), -1.0f));
		REQUIRE(approxEqual(res2.at(1, 0), -3.0f));
		REQUIRE(approxEqual(res2.at(1, 1), -4.0f));
	}
	SECTION("- (negation)") {
		auto res1 = -m1;

		REQUIRE(approxEqual(res1.at(0, 0), -1.0f));
		REQUIRE(approxEqual(res1.at(0, 1), -2.0f));
		REQUIRE(approxEqual(res1.at(1, 0), -3.0f));
		REQUIRE(approxEqual(res1.at(1, 1), -4.0f));
	}
	SECTION("* (matrix)") {
		auto res1 = m1*m2;
		REQUIRE(approxEqual(res1.at(0, 0), 0.0f));
		REQUIRE(approxEqual(res1.at(0, 1), 1.0f));
		REQUIRE(approxEqual(res1.at(1, 0), 0.0f));
		REQUIRE(approxEqual(res1.at(1, 1), 3.0f));

		auto res2 = m2*m1;
		REQUIRE(approxEqual(res2.at(0, 0), 3.0f));
		REQUIRE(approxEqual(res2.at(0, 1), 4.0f));
		REQUIRE(approxEqual(res2.at(1, 0), 0.0f));
		REQUIRE(approxEqual(res2.at(1, 1), 0.0f));

		auto res3 = m3*m4;
		REQUIRE(approxEqual(res3.at(0, 0), 1.0f));
		REQUIRE(approxEqual(res3.at(0, 1), 2.0f));
		REQUIRE(approxEqual(res3.at(1, 0), 4.0f));
		REQUIRE(approxEqual(res3.at(1, 1), 5.0f));
	}
	SECTION("* (vector)") {
		vec2 v1(1.0f, -2.0f);

		vec2 res1 = m1 * v1;
		REQUIRE(approxEqual(res1.x, -3.0f));
		REQUIRE(approxEqual(res1.y, -5.0f));

		vec3 res2 = m4 * v1;
		REQUIRE(approxEqual(res2.x, 1.0f));
		REQUIRE(approxEqual(res2.y, -2.0f));
		REQUIRE(approxEqual(res2.z, 0.0f));

		mat34 m5(1.0f, 2.0f, 3.0f, 4.0f,
		         5.0f, 6.0f, 7.0f, 8.0f,
		         9.0f, 10.0f, 11.0f, 12.0f);
		REQUIRE(approxEqual(m5 * vec4(1.0f), vec3(10.0f, 26.0f, 42.0f)));
	}
	SECTION("* (scalar)") {
		auto res1 = m1 * 2.0f;
		REQUIRE(approxEqual(res1.at(0, 0), 2.0f));
		REQUIRE(approxEqual(res1.at(0, 1), 4.0f));
		REQUIRE(approxEqual(res1.at(1, 0), 6.0f));
		REQUIRE(approxEqual(res1.at(1, 1), 8.0f));

		auto res2 = -1.0f * m2;
		REQUIRE(approxEqual(res2.at(0, 0), 0.0f));
		REQUIRE(approxEqual(res2.at(0, 1), -1.0f));
		REQUIRE(approxEqual(res2.at(1, 0), 0.0f));
		REQUIRE(approxEqual(res2.at(1, 1), 0.0f));
	}
}

TEST_CASE("Matrix comparison operators", "[sfz::Matrix]")
{
	mat22 m1(1.0f, 2.0f,
	         3.0f, 4.0f);
	mat22 m2(1.0f, 2.0f,
	         3.0f, 4.0f);
	mat22 m3(-2.0f, -1.0f,
	         3.0f, 33.0f);

	SECTION("==") {
		REQUIRE(m1 == m2);
		REQUIRE(m2 == m1);
		REQUIRE(!(m1 == m3));
		REQUIRE(!(m3 == m1));
		REQUIRE(!(m2 == m3));
		REQUIRE(!(m3 == m2));
	}
	SECTION("!=") {
		REQUIRE(m1 != m3);
		REQUIRE(m3 != m1);
		REQUIRE(m2 != m3);
		REQUIRE(m3 != m2);
		REQUIRE(!(m1 != m2));
		REQUIRE(!(m2 != m1));
	}
}

TEST_CASE("Element-wise multiplication", "[sfz::Matrix]")
{
	mat22 m(1.0f, 2.0f,
		3.0f, 4.0f);
	mat22 res = elemMult(m, m);

	REQUIRE(approxEqual(res.at(0, 0), 1.0f));
	REQUIRE(approxEqual(res.at(0, 1), 4.0f));
	REQUIRE(approxEqual(res.at(1, 0), 9.0f));
	REQUIRE(approxEqual(res.at(1, 1), 16.0f));
}

TEST_CASE("Transpose", "[sfz::Matrix]")
{
	float arr[] ={1.0f, 2.0f, 3.0f,
		4.0f, 5.0f, 6.0f};
	Matrix<float, 2, 3> m1(arr);

	Matrix<float, 3, 2> m2 = transpose(m1);
	REQUIRE(approxEqual(m2.at(0, 0), 1.0f));
	REQUIRE(approxEqual(m2.at(0, 1), 4.0f));
	REQUIRE(approxEqual(m2.at(1, 0), 2.0f));
	REQUIRE(approxEqual(m2.at(1, 1), 5.0f));
	REQUIRE(approxEqual(m2.at(2, 0), 3.0f));
	REQUIRE(approxEqual(m2.at(2, 1), 6.0f));

	mat44 m3(1.0f, 2.0f, 3.0f, 4.0f,
	         5.0f, 6.0f, 7.0f, 8.0f,
	         9.0f, 10.0f, 11.0f, 12.0f,
	         13.0f, 14.0f, 15.0f, 16.0f);
	REQUIRE(approxEqual(transpose(m3), mat44(1.0f, 5.0f, 9.0f, 13.0f,
	                                         2.0f, 6.0f, 10.0f, 14.0f,
	                                         3.0f, 7.0f, 11.0f, 15.0f,
	                                         4.0f, 8.0f, 12.0f, 16.0f)));
}

TEST_CASE("Transforming 3D vector with 3x4 and 4x4 matrix", "[sfz::MatrixSupport]")
{
	mat34 m1(2.0f, 0.0f, 0.0f, 1.0f,
	         0.0f, 2.0f, 0.0f, 0.0f,
	         0.0f, 0.0f, 2.0f, 0.0f);
	mat44 m2(2.0f, 0.0f, 0.0f, 1.0f,
	         0.0f, 2.0f, 0.0f, 0.0f,
	         0.0f, 0.0f, 2.0f, 0.0f,
	         0.0f, 0.0f, 0.0f, 1.0f);
	vec3 v(1.0f, 1.0f, 1.0f);

	SECTION("transformPoint() 3x4") {
		vec3 v2 = transformPoint(m1, v);
		REQUIRE(approxEqual(v2.x, 3.0f));
		REQUIRE(approxEqual(v2.y, 2.0f));
		REQUIRE(approxEqual(v2.z, 2.0f));
	}
	SECTION("transformPoint() 4x4") {
		vec3 v2 = transformPoint(m2, v);
		REQUIRE(approxEqual(v2.x, 3.0f));
		REQUIRE(approxEqual(v2.y, 2.0f));
		REQUIRE(approxEqual(v2.z, 2.0f));
	}
	SECTION("transformDir() 3x4") {
		vec3 v2 = transformDir(m1, v);
		REQUIRE(approxEqual(v2.x, 2.0f));
		REQUIRE(approxEqual(v2.y, 2.0f));
		REQUIRE(approxEqual(v2.z, 2.0f));
	}
	SECTION("transformDir() 4x4") {
		vec3 v2 = transformDir(m2, v);
		REQUIRE(approxEqual(v2.x, 2.0f));
		REQUIRE(approxEqual(v2.y, 2.0f));
		REQUIRE(approxEqual(v2.z, 2.0f));
	}
}

TEST_CASE("Determinants", "[sfz::MatrixSupport]")
{
	mat22 m1(1.0f, 2.0f,
	         3.0f, 4.0f);
	REQUIRE(approxEqual(determinant(m1), -2.0f));

	mat33 m2(-1.0f, 1.0f, 0.0f,
	         3.0f, 5.0f, 1.0f,
	         7.0f, 8.0f, 9.0f);
	REQUIRE(approxEqual(determinant(m2), -57.0f));

	mat33 m3(99.0f, -2.0f, 5.0f,
	         8.0f, -4.0f, -1.0f,
	         6.0f, 1.0f, -88.0f);
	REQUIRE(approxEqual(determinant(m3), 33711.0f));

	mat44 m4(1.0f, -2.0f, 1.0f, 3.0f,
	         1.0f, 4.0f, -5.0f, 0.0f,
	         -10.0f, 0.0f, 4.0f, 2.0f,
	         -1.0f, 0.0f, 2.0f, 0.0f);
	REQUIRE(approxEqual(determinant(m4), -204.0f));
}

TEST_CASE("Inverse", "[sfz::MatrixSupport]")
{
	mat22 m1(1.0f, 1.0f,
	         1.0f, 2.0f);
	mat22 m1Inv(2.0f, -1.0f,
	            -1.0f, 1.0f);
	REQUIRE(approxEqual(inverse(m1), m1Inv));
	REQUIRE(approxEqual(inverse(mat22::identity()), mat22::identity()));

	mat33 m3(1.0f, 1.0f, 1.0f,
	         1.0f, 1.0f, 2.0f,
	         1.0f, 2.0f, 3.0f);
	mat33 m3Inv(1.0f, 1.0f, -1.0f,
	            1.0f, -2.0f, 1.0f,
	            -1.0f, 1.0f, 0.0f);
	REQUIRE(approxEqual(inverse(m3), m3Inv));
	REQUIRE(approxEqual(inverse(mat33::identity()), mat33::identity()));

	mat44 m5(1.0f, 1.0f, 1.0f, 1.0f,
	         1.0f, 1.0f, 2.0f, 3.0f,
	         1.0f, 2.0f, 3.0f, 4.0f,
	         1.0f, 2.0f, 2.0f, 1.0f);
	mat44 m5Inv(1.0f, 1.0f, -1.0f, 0.0f,
	            2.0f, -3.0f, 2.0f, -1.0f,
	            -3.0f, 3.0f, -2.0f, 2.0f,
	            1.0f, -1.0f, 1.0f, -1.0f);
	REQUIRE(approxEqual(inverse(m5), m5Inv));
	REQUIRE(approxEqual(inverse(mat44::identity()), mat44::identity()));
}

TEST_CASE("Matrix is proper POD", "[sfz::Matrix]")
{
	REQUIRE(std::is_trivially_default_constructible<sfz::mat4>::value);
	REQUIRE(std::is_trivially_copyable<sfz::mat4>::value);
	REQUIRE(std::is_trivial<sfz::mat4>::value);
	REQUIRE(std::is_standard_layout<sfz::mat4>::value);
	REQUIRE(std::is_pod<sfz::mat4>::value);
}

TEST_CASE("Matrix toString()", "[sfz::Matrix]")
{
	mat4 m1{{1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}, {9.0f, 10.0f, 11.0f, 12.0f}, {13.0f, 14.0f, 15.0f, 16.0f}};
	auto mStr1 = toString(m1, false, 1);
	REQUIRE(mStr1 == "[[1.0, 2.0, 3.0, 4.0], [5.0, 6.0, 7.0, 8.0], [9.0, 10.0, 11.0, 12.0], [13.0, 14.0, 15.0, 16.0]]");
}
