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
#include "sfz/math/MatrixSupport.hpp"
#include "sfz/math/ProjectionMatrices.hpp"
#include "sfz/math/Vector.hpp"

#include <type_traits>

using namespace sfz;

TEST_CASE("Constructors", "[sfz::Matrix]")
{
	SECTION("Array pointer constructor)") {
		const int rowMajor[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
		const int colMajor[] = {1, 4, 7, 2, 5, 8, 3, 6, 9};
		sfz::mat3i m1{rowMajor, true};
		sfz::mat3i m2{colMajor, false};
		sfz::mat3i m3{rowMajor, false};
		sfz::mat3i m4{colMajor, true};
		
		REQUIRE(m1 == m2);
		REQUIRE(m3 == m4);
	}
	SECTION("Initializer list constructor") {
		sfz::mat3i m1{{1, 2, 3},
		              {4, 5, 6},
		              {7, 8, 9}};

		REQUIRE(m1.at(0, 0) == 1);
		REQUIRE(m1.at(0, 1) == 2);
		REQUIRE(m1.at(0, 2) == 3);
		REQUIRE(m1.at(1, 0) == 4);
		REQUIRE(m1.at(1, 1) == 5);
		REQUIRE(m1.at(1, 2) == 6);
		REQUIRE(m1.at(2, 0) == 7);
		REQUIRE(m1.at(2, 1) == 8);
		REQUIRE(m1.at(2, 2) == 9);

		REQUIRE(m1.elements[0][0] == 1);
		REQUIRE(m1.elements[0][1] == 4);
		REQUIRE(m1.elements[0][2] == 7);
		REQUIRE(m1.elements[1][0] == 2);
		REQUIRE(m1.elements[1][1] == 5);
		REQUIRE(m1.elements[1][2] == 8);
		REQUIRE(m1.elements[2][0] == 3);
		REQUIRE(m1.elements[2][1] == 6);
		REQUIRE(m1.elements[2][2] == 9);

		sfz::mat2i m2{{1}};

		REQUIRE(m2.at(0, 0) == 1);
		REQUIRE(m2.at(0, 1) == 0);
		REQUIRE(m2.at(1, 0) == 0);
		REQUIRE(m2.at(1, 1) == 0);

		REQUIRE(m2.elements[0][0] == 1);
		REQUIRE(m2.elements[0][1] == 0);
		REQUIRE(m2.elements[1][0] == 0);
		REQUIRE(m2.elements[1][1] == 0);

		sfz::Matrix<int, 2, 3> m3{{1, 2, 3}, {4, 5, 6}};

		REQUIRE(m3.at(0, 0) == 1);
		REQUIRE(m3.at(0, 1) == 2);
		REQUIRE(m3.at(0, 2) == 3);
		REQUIRE(m3.at(1, 0) == 4);
		REQUIRE(m3.at(1, 1) == 5);
		REQUIRE(m3.at(1, 2) == 6);

		REQUIRE(m3.elements[0][0] == 1);
		REQUIRE(m3.elements[0][1] == 4);
		REQUIRE(m3.elements[1][0] == 2);
		REQUIRE(m3.elements[1][1] == 5);
		REQUIRE(m3.elements[2][0] == 3);
		REQUIRE(m3.elements[2][1] == 6);

		sfz::Matrix<int, 3, 2> m4{{1, 0}, {0, 1}, {0, 0}};

		REQUIRE(m4.at(0, 0) == 1);
		REQUIRE(m4.at(0, 1) == 0);
		REQUIRE(m4.at(1, 0) == 0);
		REQUIRE(m4.at(1, 1) == 1);
		REQUIRE(m4.at(2, 0) == 0);
		REQUIRE(m4.at(2, 1) == 0);

		REQUIRE(m4.elements[0][0] == 1);
		REQUIRE(m4.elements[0][1] == 0);
		REQUIRE(m4.elements[0][2] == 0);
		REQUIRE(m4.elements[1][0] == 0);
		REQUIRE(m4.elements[1][1] == 1);
		REQUIRE(m4.elements[1][2] == 0);
	}
}

TEST_CASE("data()", "[sfz::Matrix]") {
	sfz::mat2i m1{{1, 2},
	              {3, 4}};
	const sfz::mat2i m2{{-1, -2},
	                    {-3, -4}};
	
	int* ptr1 = m1.data();
	const int* ptr2 = m2.data();

	REQUIRE(ptr1[0] == 1);
	REQUIRE(ptr1[1] == 3);
	REQUIRE(ptr1[2] == 2);
	REQUIRE(ptr1[3] == 4);

	REQUIRE(ptr2[0] == -1);
	REQUIRE(ptr2[1] == -3);
	REQUIRE(ptr2[2] == -2);
	REQUIRE(ptr2[3] == -4);
}

TEST_CASE("Row and column getters/setters", "[sfz::Matrix]")
{
	sfz::Matrix<int, 2, 3> m{{1, 2, 3},
	                         {4, 5, 6}};

	SECTION("rowAt()") {
		sfz::Vector<int, 3> row1 = m.rowAt(0);
		sfz::Vector<int, 3> row2 = m.rowAt(1);

		REQUIRE(m.at(0, 0) == row1[0]);
		REQUIRE(m.at(0, 1) == row1[1]);
		REQUIRE(m.at(0, 2) == row1[2]);
		REQUIRE(m.at(1, 0) == row2[0]);
		REQUIRE(m.at(1, 1) == row2[1]);
		REQUIRE(m.at(1, 2) == row2[2]);
	}
	SECTION("columnAt()") {
		sfz::Vector<int, 2> col1 = m.columnAt(0);
		sfz::Vector<int, 2> col2 = m.columnAt(1);
		sfz::Vector<int, 2> col3 = m.columnAt(2);

		REQUIRE(m.at(0, 0) == col1[0]);
		REQUIRE(m.at(1, 0) == col1[1]);
		REQUIRE(m.at(0, 1) == col2[0]);
		REQUIRE(m.at(1, 1) == col2[1]);
		REQUIRE(m.at(0, 2) == col3[0]);
		REQUIRE(m.at(1, 2) == col3[1]);
	}
	SECTION("setRow()") {
		m.setRow(1, sfz::Vector<int,3>{-1, -2, -3});

		REQUIRE(m.at(0, 0) == 1);
		REQUIRE(m.at(0, 1) == 2);
		REQUIRE(m.at(0, 2) == 3);
		REQUIRE(m.at(1, 0) == -1);
		REQUIRE(m.at(1, 1) == -2);
		REQUIRE(m.at(1, 2) == -3);
	}
	SECTION("seColumn()") {
		m.setColumn(1, sfz::Vector<int,2>{-1, -2});

		REQUIRE(m.at(0, 0) == 1);
		REQUIRE(m.at(0, 1) == -1);
		REQUIRE(m.at(0, 2) == 3);
		REQUIRE(m.at(1, 0) == 4);
		REQUIRE(m.at(1, 1) == -2);
		REQUIRE(m.at(1, 2) == 6);
	}
}

TEST_CASE("Element-wise multiplication", "[sfz::Matrix]")
{
	sfz::mat2i m1{{1, 2},
	              {3, 4}};
	sfz::mat2i m2{{1, 2},
	              {3, 4}};

	auto res1 = elemMult(m1, m2);
	auto res2 = elemMult(m2, m1);

	REQUIRE(res1 == res2);
	REQUIRE(res1.at(0, 0) == 1);
	REQUIRE(res1.at(0, 1) == 4);
	REQUIRE(res1.at(1, 0) == 9);
	REQUIRE(res1.at(1, 1) == 16);
}

TEST_CASE("Transpose", "[sfz::Matrix]")
{
	sfz::mat2i m1{{1, 2},
	              {3, 4}};
	sfz::Matrix<int, 2, 3> m2{{1, 2, 3},
	                          {4, 5, 6}};

	sfz::mat2i res1 = transpose(m1);
	REQUIRE(res1 != m1);
	REQUIRE(transpose(res1) == m1);

	REQUIRE(res1.at(0, 0) == 1);
	REQUIRE(res1.at(0, 1) == 3);
	REQUIRE(res1.at(1, 0) == 2);
	REQUIRE(res1.at(1, 1) == 4);

	sfz::Matrix<int, 3, 2> res2 = transpose(m2);
	REQUIRE(transpose(res2) == m2);

	REQUIRE(res2.at(0, 0) == 1);
	REQUIRE(res2.at(0, 1) == 4);
	REQUIRE(res2.at(1, 0) == 2);
	REQUIRE(res2.at(1, 1) == 5);
	REQUIRE(res2.at(2, 0) == 3);
	REQUIRE(res2.at(2, 1) == 6);
}

TEST_CASE("Arhitmetic & assignment operators", "[sfz::Matrix]")
{
	sfz::mat2i m1{{1, 2},
	              {3, 4}};
	sfz::mat2i m2{{1, 2},
	              {3, 4}};
	sfz::mat2i m3{{-2, -1},
	              {3, 33}};

	SECTION("+=") {
		m1 += m2;
		m2 += m3;

		REQUIRE(m1.at(0, 0) == 2);
		REQUIRE(m1.at(0, 1) == 4);
		REQUIRE(m1.at(1, 0) == 6);
		REQUIRE(m1.at(1, 1) == 8);

		REQUIRE(m2.at(0, 0) == -1);
		REQUIRE(m2.at(0, 1) == 1);
		REQUIRE(m2.at(1, 0) == 6);
		REQUIRE(m2.at(1, 1) == 37);
	}
	SECTION("-=") {
		m1 -= m2;
		m2 -= m3;

		REQUIRE(m1.at(0, 0) == 0);
		REQUIRE(m1.at(0, 1) == 0);
		REQUIRE(m1.at(1, 0) == 0);
		REQUIRE(m1.at(1, 1) == 0);

		REQUIRE(m2.at(0, 0) == 3);
		REQUIRE(m2.at(0, 1) == 3);
		REQUIRE(m2.at(1, 0) == 0);
		REQUIRE(m2.at(1, 1) == -29);
	}
	SECTION("*= (scalar)") {
		m1 *= 2;
		REQUIRE(m1.at(0, 0) == 2);
		REQUIRE(m1.at(0, 1) == 4);
		REQUIRE(m1.at(1, 0) == 6);
		REQUIRE(m1.at(1, 1) == 8);	

		m3 *= -1;
		REQUIRE(m3.at(0, 0) == 2);
		REQUIRE(m3.at(0, 1) == 1);
		REQUIRE(m3.at(1, 0) == -3);
		REQUIRE(m3.at(1, 1) == -33);
	}
	SECTION("*= (matrix of same size)") {
		sfz::mat2i m4{{1, 0}, {0, 1}};
		auto m1cpy = m1;
		m1cpy *= m4;

		REQUIRE(m1cpy.at(0, 0) == 1);
		REQUIRE(m1cpy.at(0, 1) == 2);
		REQUIRE(m1cpy.at(1, 0) == 3);
		REQUIRE(m1cpy.at(1, 1) == 4);

		m4 *= m1;
		REQUIRE(m4.at(0, 0) == 1);
		REQUIRE(m4.at(0, 1) == 2);
		REQUIRE(m4.at(1, 0) == 3);
		REQUIRE(m4.at(1, 1) == 4);
	}
}

TEST_CASE("Arhitmetic operators", "[sfz::Matrix]")
{
	sfz::mat2i m1{{1, 2}, {3, 4}};
	sfz::mat2i m2{{0, 1}, {0, 0}};
	sfz::Matrix<int, 2, 3> m3{{1, 2, 3}, {4, 5, 6}};
	sfz::Matrix<int, 3, 2> m4{{1, 0}, {0, 1}, {0, 0}};

	SECTION("+") {
		auto res1 = m1 + m2;
		auto res2 = m2 + m1;

		REQUIRE(res1 == res2);
		REQUIRE(res1.at(0, 0) == 1);
		REQUIRE(res1.at(0, 1) == 3);
		REQUIRE(res1.at(1, 0) == 3);
		REQUIRE(res1.at(1, 1) == 4);

		auto res3 = m3 + m3;
		REQUIRE(res3.at(0, 0) == 2);
		REQUIRE(res3.at(0, 1) == 4);
		REQUIRE(res3.at(0, 2) == 6);
		REQUIRE(res3.at(1, 0) == 8);
		REQUIRE(res3.at(1, 1) == 10);
		REQUIRE(res3.at(1, 2) == 12);
	}
	SECTION("-") {
		auto res1 = m1 - m2;
		auto res2 = m2 - m1;

		REQUIRE(res1 != res2);
		
		REQUIRE(res1.at(0, 0) == 1);
		REQUIRE(res1.at(0, 1) == 1);
		REQUIRE(res1.at(1, 0) == 3);
		REQUIRE(res1.at(1, 1) == 4);
		
		REQUIRE(res2.at(0, 0) == -1);
		REQUIRE(res2.at(0, 1) == -1);
		REQUIRE(res2.at(1, 0) == -3);
		REQUIRE(res2.at(1, 1) == -4);
	}
	SECTION("- (negation)") {
		auto res1 = -m1;

		REQUIRE(res1.at(0, 0) == -1);
		REQUIRE(res1.at(0, 1) == -2);
		REQUIRE(res1.at(1, 0) == -3);
		REQUIRE(res1.at(1, 1) == -4);
	}
	SECTION("* (matrix)") {
		auto res1 = m1*m2;
		REQUIRE(res1.at(0, 0) == 0);
		REQUIRE(res1.at(0, 1) == 1);
		REQUIRE(res1.at(1, 0) == 0);
		REQUIRE(res1.at(1, 1) == 3);

		auto res2 = m2*m1;
		REQUIRE(res2.at(0, 0) == 3);
		REQUIRE(res2.at(0, 1) == 4);
		REQUIRE(res2.at(1, 0) == 0);
		REQUIRE(res2.at(1, 1) == 0);

		sfz::mat2i res3 = m3*m4;
		REQUIRE(res3.at(0, 0) == 1);
		REQUIRE(res3.at(0, 1) == 2);
		REQUIRE(res3.at(1, 0) == 4);
		REQUIRE(res3.at(1, 1) == 5);
	}
	SECTION("* (vector)") {
		vec2i v1{1, -2};

		vec2i res1 = m1*v1;
		REQUIRE(res1[0] == -3);
		REQUIRE(res1[1] == -5);

		vec3i res2 = m4*v1;
		REQUIRE(res2[0] == 1);
		REQUIRE(res2[1] == -2);
		REQUIRE(res2[2] == 0);
	}
	SECTION("* (scalar)") {
		auto res1 = m1*2;
		REQUIRE(res1.at(0, 0) == 2);
		REQUIRE(res1.at(0, 1) == 4);
		REQUIRE(res1.at(1, 0) == 6);
		REQUIRE(res1.at(1, 1) == 8);

		auto res2 = -1*m2;
		REQUIRE(res2.at(0, 0) == 0);
		REQUIRE(res2.at(0, 1) == -1);
		REQUIRE(res2.at(1, 0) == 0);
		REQUIRE(res2.at(1, 1) == 0);
	}
}

TEST_CASE("Matrix comparison operators", "[sfz::Matrix]")
{
	sfz::mat2i m1{{1, 2},
	              {3, 4}};
	sfz::mat2i m2{{1, 2},
	              {3, 4}};
	sfz::mat2i m3{{-2, -1},
	              {3, 33}};

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

TEST_CASE("Matrix toString()", "[sfz::Matrix]")
{
	mat4 m1{{1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}, {9.0f, 10.0f, 11.0f, 12.0f}, {13.0f, 14.0f, 15.0f, 16.0f}};
	auto mStr1 = toString(m1, false, 1);
	REQUIRE(mStr1 == "[[1.0, 2.0, 3.0, 4.0], [5.0, 6.0, 7.0, 8.0], [9.0, 10.0, 11.0, 12.0], [13.0, 14.0, 15.0, 16.0]]");
}

TEST_CASE("Matrix is proper POD", "[sfz::Matrix]")
{
	REQUIRE(std::is_trivially_default_constructible<sfz::mat4>::value);
	REQUIRE(std::is_trivially_copyable<sfz::mat4>::value);
	REQUIRE(std::is_trivial<sfz::mat4>::value);
	REQUIRE(std::is_standard_layout<sfz::mat4>::value);
	REQUIRE(std::is_pod<sfz::mat4>::value);
}


// MatrixSupport.hpp
// ------------------------------------------------------------------------------------------------

TEST_CASE("Resizing Matrices", "[sfz::MatrixSupport]")
{
	sfz::Matrix<int, 4, 4> m1{{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}};
	sfz::Matrix<int, 3, 3> m2{{1, 2, 3}, {5, 6, 7}, {9, 10, 11}};

	REQUIRE(sfz::toMat3(m1) == m2);
	REQUIRE(sfz::toMat3(sfz::toMat4(m2)) == m2);

	auto m3 = sfz::toMat4(m2);
	REQUIRE(m3.at(3, 0) == 0);
	REQUIRE(m3.at(3, 1) == 0);
	REQUIRE(m3.at(3, 2) == 0);
	REQUIRE(m3.at(3, 3) == 1);
	REQUIRE(m3.at(2, 3) == 0);
	REQUIRE(m3.at(1, 3) == 0);
	REQUIRE(m3.at(0, 3) == 0);
}

TEST_CASE("Transforming 3d vector with 4x4 matrix", "[sfz::MatrixSupport]")
{
	sfz::Matrix<int, 4, 4> m{{2, 0, 0, 1}, {0, 2, 0, 0}, {0, 0, 2, 0}, {0, 0, 0, 1}};
	sfz::Vector<int, 3> v{1, 1, 1};

	SECTION("transformPoint()") {
		sfz::Vector<int, 3> v2 = sfz::transformPoint(m, v);
		REQUIRE(v2[0] == 3);
		REQUIRE(v2[1] == 2);
		REQUIRE(v2[2] == 2);
	}
	SECTION("transformDir()") {
		sfz::Vector<int, 3> v2 = sfz::transformDir(m, v);
		REQUIRE(v2[0] == 2);
		REQUIRE(v2[1] == 2);
		REQUIRE(v2[2] == 2);
	}
}

TEST_CASE("Determinants", "[sfz::MatrixSupport]")
{
	sfz::Matrix<int, 2, 2> m1{{1,2}, {3,4}};
	REQUIRE(sfz::determinant(m1) == -2);

	sfz::Matrix<int, 3, 3> m2{{-1,1,0}, {3,5,1}, {7,8,9}};
	REQUIRE(sfz::determinant(m2) == -57);

	sfz::Matrix<int, 3, 3> m3{{99,-2,5}, {8,-4,-1}, {6,1,-88}};
	REQUIRE(sfz::determinant(m3) == 33711);

	sfz::Matrix<int, 4, 4> m4{{1, -2, 1, 3}, {1, 4, -5, 0}, {-10, 0, 4, 2}, {-1, 0, 2, 0}};
	REQUIRE(sfz::determinant(m4) == -204);
}

TEST_CASE("Inverse", "[sfz::MatrixSupport]")
{
	sfz::Matrix<int, 2, 2> m1{{1, 1}, {1, 2}};
	sfz::Matrix<int, 2, 2> m1Inv{{2, -1}, {-1, 1}};

	REQUIRE(sfz::inverse(m1) == m1Inv);

	sfz::Matrix<int, 2, 2> m2{{1, 0}, {0, 1}};
	REQUIRE(sfz::inverse(m2) == m2);

	sfz::Matrix<int, 3, 3> m3{{1, 1, 1}, {1, 1, 2}, {1, 2, 3}};
	sfz::Matrix<int, 3, 3> m3Inv{{1, 1, -1}, {1, -2, 1}, {-1, 1, 0}};
	REQUIRE(sfz::inverse(m3) == m3Inv);

	sfz::Matrix<int, 3, 3> m4{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	REQUIRE(sfz::inverse(m4) == m4);

	sfz::Matrix<int, 4, 4> m5{{1, 1, 1, 1}, {1, 1, 2, 3}, {1, 2, 3, 4}, {1, 2, 2, 1}};
	sfz::Matrix<int, 4, 4> m5Inv{{1, 1, -1, 0}, {2, -3, 2, -1}, {-3, 3, -2, 2}, {1, -1, 1, -1}};
	REQUIRE(sfz::inverse(m5) == m5Inv);

	sfz::Matrix<int, 4, 4> m6{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
	REQUIRE(sfz::inverse(m6) == m6);
}

TEST_CASE("Rotation matrices", "[sfz::MatrixSupport")
{
	sfz::vec4 v1{1, 1, 1, 1};

	SECTION("xRotationMatrix4()") {
		sfz::mat4 xRot90 = sfz::xRotationMatrix4(sfz::PI()/2);
		REQUIRE(approxEqual(xRot90, sfz::rotationMatrix4(sfz::vec3{1,0,0}, sfz::PI()/2.0f)));

		REQUIRE(approxEqual(xRot90.at(0, 0), 1.0f));
		REQUIRE(approxEqual(xRot90.at(0, 1), 0.0f));
		REQUIRE(approxEqual(xRot90.at(0, 2), 0.0f));
		REQUIRE(approxEqual(xRot90.at(0, 3), 0.0f));

		REQUIRE(approxEqual(xRot90.at(1, 0), 0.0f));
		REQUIRE(approxEqual(xRot90.at(1, 1), 0.0f));
		REQUIRE(approxEqual(xRot90.at(1, 2), -1.0f));
		REQUIRE(approxEqual(xRot90.at(1, 3), 0.0f));

		REQUIRE(approxEqual(xRot90.at(2, 0), 0.0f));
		REQUIRE(approxEqual(xRot90.at(2, 1), 1.0f));
		REQUIRE(approxEqual(xRot90.at(2, 2), 0.0f));
		REQUIRE(approxEqual(xRot90.at(2, 3), 0.0f));

		REQUIRE(approxEqual(xRot90.at(3, 0), 0.0f));
		REQUIRE(approxEqual(xRot90.at(3, 1), 0.0f));
		REQUIRE(approxEqual(xRot90.at(3, 2), 0.0f));
		REQUIRE(approxEqual(xRot90.at(3, 3), 1.0f));

		sfz::mat4 xRot180 = sfz::xRotationMatrix4(sfz::PI());
		REQUIRE(approxEqual(xRot180, sfz::rotationMatrix4(sfz::vec3{1,0,0}, sfz::PI())));

		REQUIRE(approxEqual(xRot180.at(0, 0), 1.0f));
		REQUIRE(approxEqual(xRot180.at(0, 1), 0.0f));
		REQUIRE(approxEqual(xRot180.at(0, 2), 0.0f));
		REQUIRE(approxEqual(xRot180.at(0, 3), 0.0f));

		REQUIRE(approxEqual(xRot180.at(1, 0), 0.0f));
		REQUIRE(approxEqual(xRot180.at(1, 1), -1.0f));
		REQUIRE(approxEqual(xRot180.at(1, 2), 0.0f));
		REQUIRE(approxEqual(xRot180.at(1, 3), 0.0f));

		REQUIRE(approxEqual(xRot180.at(2, 0), 0.0f));
		REQUIRE(approxEqual(xRot180.at(2, 1), 0.0f));
		REQUIRE(approxEqual(xRot180.at(2, 2), -1.0f));
		REQUIRE(approxEqual(xRot180.at(2, 3), 0.0f));

		REQUIRE(approxEqual(xRot180.at(3, 0), 0.0f));
		REQUIRE(approxEqual(xRot180.at(3, 1), 0.0f));
		REQUIRE(approxEqual(xRot180.at(3, 2), 0.0f));
		REQUIRE(approxEqual(xRot180.at(3, 3), 1.0f));

		auto v2 = xRot90*v1;
		REQUIRE(approxEqual(v2[0], 1.0f));
		REQUIRE(approxEqual(v2[1], -1.0f));
		REQUIRE(approxEqual(v2[2], 1.0f));
		REQUIRE(approxEqual(v2[3], 1.0f));

		auto v3 = xRot180*v1;
		REQUIRE(approxEqual(v3[0], 1.0f));
		REQUIRE(approxEqual(v3[1], -1.0f));
		REQUIRE(approxEqual(v3[2], -1.0f));
		REQUIRE(approxEqual(v3[3], 1.0f));
	}
	SECTION("yRotationMatrix4()") {
		sfz::mat4 yRot90 = sfz::yRotationMatrix4(sfz::PI()/2);
		REQUIRE(approxEqual(yRot90, sfz::rotationMatrix4(sfz::vec3{0,1,0}, sfz::PI()/2)));

		REQUIRE(approxEqual(yRot90.at(0, 0), 0.0f));
		REQUIRE(approxEqual(yRot90.at(0, 1), 0.0f));
		REQUIRE(approxEqual(yRot90.at(0, 2), 1.0f));
		REQUIRE(approxEqual(yRot90.at(0, 3), 0.0f));

		REQUIRE(approxEqual(yRot90.at(1, 0), 0.0f));
		REQUIRE(approxEqual(yRot90.at(1, 1), 1.0f));
		REQUIRE(approxEqual(yRot90.at(1, 2), 0.0f));
		REQUIRE(approxEqual(yRot90.at(1, 3), 0.0f));

		REQUIRE(approxEqual(yRot90.at(2, 0), -1.0f));
		REQUIRE(approxEqual(yRot90.at(2, 1), 0.0f));
		REQUIRE(approxEqual(yRot90.at(2, 2), 0.0f));
		REQUIRE(approxEqual(yRot90.at(2, 3), 0.0f));

		REQUIRE(approxEqual(yRot90.at(3, 0), 0.0f));
		REQUIRE(approxEqual(yRot90.at(3, 1), 0.0f));
		REQUIRE(approxEqual(yRot90.at(3, 2), 0.0f));
		REQUIRE(approxEqual(yRot90.at(3, 3), 1.0f));

		sfz::mat4 yRot180 = sfz::yRotationMatrix4(sfz::PI());
		REQUIRE(approxEqual(yRot180, sfz::rotationMatrix4(sfz::vec3{0,1,0}, sfz::PI())));

		REQUIRE(approxEqual(yRot180.at(0, 0), -1.0f));
		REQUIRE(approxEqual(yRot180.at(0, 1), 0.0f));
		REQUIRE(approxEqual(yRot180.at(0, 2), 0.0f));
		REQUIRE(approxEqual(yRot180.at(0, 3), 0.0f));

		REQUIRE(approxEqual(yRot180.at(1, 0), 0.0f));
		REQUIRE(approxEqual(yRot180.at(1, 1), 1.0f));
		REQUIRE(approxEqual(yRot180.at(1, 2), 0.0f));
		REQUIRE(approxEqual(yRot180.at(1, 3), 0.0f));

		REQUIRE(approxEqual(yRot180.at(2, 0), 0.0f));
		REQUIRE(approxEqual(yRot180.at(2, 1), 0.0f));
		REQUIRE(approxEqual(yRot180.at(2, 2), -1.0f));
		REQUIRE(approxEqual(yRot180.at(2, 3), 0.0f));

		REQUIRE(approxEqual(yRot180.at(3, 0), 0.0f));
		REQUIRE(approxEqual(yRot180.at(3, 1), 0.0f));
		REQUIRE(approxEqual(yRot180.at(3, 2), 0.0f));
		REQUIRE(approxEqual(yRot180.at(3, 3), 1.0f));

		auto v2 = yRot90*v1;
		REQUIRE(approxEqual(v2[0], 1.0f));
		REQUIRE(approxEqual(v2[1], 1.0f));
		REQUIRE(approxEqual(v2[2], -1.0f));
		REQUIRE(approxEqual(v2[3], 1.0f));

		auto v3 = yRot180*v1;
		REQUIRE(approxEqual(v3[0], -1.0f));
		REQUIRE(approxEqual(v3[1], 1.0f));
		REQUIRE(approxEqual(v3[2], -1.0f));
		REQUIRE(approxEqual(v3[3], 1.0f));
	}
	SECTION("zRotationMatrix4()") {
		sfz::mat4 zRot90 = sfz::zRotationMatrix4(sfz::PI()/2);
		REQUIRE(approxEqual(zRot90, sfz::rotationMatrix4(sfz::vec3{0,0,1}, sfz::PI()/2)));

		REQUIRE(approxEqual(zRot90.at(0, 0), 0.0f));
		REQUIRE(approxEqual(zRot90.at(0, 1), -1.0f));
		REQUIRE(approxEqual(zRot90.at(0, 2), 0.0f));
		REQUIRE(approxEqual(zRot90.at(0, 3), 0.0f));

		REQUIRE(approxEqual(zRot90.at(1, 0), 1.0f));
		REQUIRE(approxEqual(zRot90.at(1, 1), 0.0f));
		REQUIRE(approxEqual(zRot90.at(1, 2), 0.0f));
		REQUIRE(approxEqual(zRot90.at(1, 3), 0.0f));

		REQUIRE(approxEqual(zRot90.at(2, 0), 0.0f));
		REQUIRE(approxEqual(zRot90.at(2, 1), 0.0f));
		REQUIRE(approxEqual(zRot90.at(2, 2), 1.0f));
		REQUIRE(approxEqual(zRot90.at(2, 3), 0.0f));

		REQUIRE(approxEqual(zRot90.at(3, 0), 0.0f));
		REQUIRE(approxEqual(zRot90.at(3, 1), 0.0f));
		REQUIRE(approxEqual(zRot90.at(3, 2), 0.0f));
		REQUIRE(approxEqual(zRot90.at(3, 3), 1.0f));

		sfz::mat4 zRot180 = sfz::zRotationMatrix4(sfz::PI());
		REQUIRE(approxEqual(zRot180, sfz::rotationMatrix4(sfz::vec3{0,0,1}, sfz::PI())));

		REQUIRE(approxEqual(zRot180.at(0, 0), -1.0f));
		REQUIRE(approxEqual(zRot180.at(0, 1), 0.0f));
		REQUIRE(approxEqual(zRot180.at(0, 2), 0.0f));
		REQUIRE(approxEqual(zRot180.at(0, 3), 0.0f));

		REQUIRE(approxEqual(zRot180.at(1, 0), 0.0f));
		REQUIRE(approxEqual(zRot180.at(1, 1), -1.0f));
		REQUIRE(approxEqual(zRot180.at(1, 2), 0.0f));
		REQUIRE(approxEqual(zRot180.at(1, 3), 0.0f));

		REQUIRE(approxEqual(zRot180.at(2, 0), 0.0f));
		REQUIRE(approxEqual(zRot180.at(2, 1), 0.0f));
		REQUIRE(approxEqual(zRot180.at(2, 2), 1.0f));
		REQUIRE(approxEqual(zRot180.at(2, 3), 0.0f));

		REQUIRE(approxEqual(zRot180.at(3, 0), 0.0f));
		REQUIRE(approxEqual(zRot180.at(3, 1), 0.0f));
		REQUIRE(approxEqual(zRot180.at(3, 2), 0.0f));
		REQUIRE(approxEqual(zRot180.at(3, 3), 1.0f));

		auto v2 = zRot90*v1;
		REQUIRE(approxEqual(v2[0], -1.0f));
		REQUIRE(approxEqual(v2[1], 1.0f));
		REQUIRE(approxEqual(v2[2], 1.0f));
		REQUIRE(approxEqual(v2[3], 1.0f));

		auto v3 = zRot180*v1;
		REQUIRE(approxEqual(v3[0], -1.0f));
		REQUIRE(approxEqual(v3[1], -1.0f));
		REQUIRE(approxEqual(v3[2], 1.0f));
		REQUIRE(approxEqual(v3[3], 1.0f));
	}
	SECTION("rotationMatrix4()") {
		sfz::vec4 startPoint{1, 0, 0, 1};
		const sfz::vec4 expectedEndPoint{0, 1, 0, 1};
		sfz::vec3 axis = sfz::vec3{1, 1, 0} - sfz::vec3{0, 0, 0};
		sfz::mat4 rot = sfz::rotationMatrix4(axis, sfz::PI());

		auto res = rot * startPoint;
		
		REQUIRE(approxEqual(res[0], expectedEndPoint[0]));
		REQUIRE(approxEqual(res[1], expectedEndPoint[1]));
		REQUIRE(approxEqual(res[2], expectedEndPoint[2]));
		REQUIRE(approxEqual(res[3], expectedEndPoint[3]));
	}
	SECTION("3 and 4 variants are identical") {
		REQUIRE(approxEqual(sfz::xRotationMatrix4(2.5f), sfz::toMat4(sfz::xRotationMatrix3(2.5f))));
		REQUIRE(approxEqual(sfz::xRotationMatrix4(1.2f), sfz::toMat4(sfz::xRotationMatrix3(1.2f))));

		REQUIRE(approxEqual(sfz::yRotationMatrix4(2.5f), sfz::toMat4(sfz::yRotationMatrix3(2.5f))));
		REQUIRE(approxEqual(sfz::yRotationMatrix4(1.2f), sfz::toMat4(sfz::yRotationMatrix3(1.2f))));

		REQUIRE(approxEqual(sfz::zRotationMatrix4(2.5f), sfz::toMat4(sfz::zRotationMatrix3(2.5f))));
		REQUIRE(approxEqual(sfz::zRotationMatrix4(1.2f), sfz::toMat4(sfz::zRotationMatrix3(1.2f))));

		sfz::vec3 a{1, -1, 2};
		REQUIRE(approxEqual(sfz::rotationMatrix4(a, 2.5f), sfz::toMat4(sfz::rotationMatrix3(a, 2.5f))));
		REQUIRE(approxEqual(sfz::rotationMatrix4(a, 1.2f), sfz::toMat4(sfz::rotationMatrix3(a, 1.2f))));
	}
}


TEST_CASE("Transformation matrices", "[sfz::MatrixSupport]")
{
	sfz::vec4 v1{1, 1, 1, 1};
	SECTION("identityMatrix4()") {
		auto m = sfz::identityMatrix4<int>();
		REQUIRE(m == sfz::toMat4(sfz::identityMatrix3<int>()));
		
		REQUIRE(m.at(0, 0) == 1);
		REQUIRE(m.at(0, 1) == 0);
		REQUIRE(m.at(0, 2) == 0);
		REQUIRE(m.at(0, 3) == 0);

		REQUIRE(m.at(1, 0) == 0);
		REQUIRE(m.at(1, 1) == 1);
		REQUIRE(m.at(1, 2) == 0);
		REQUIRE(m.at(1, 3) == 0);

		REQUIRE(m.at(2, 0) == 0);
		REQUIRE(m.at(2, 1) == 0);
		REQUIRE(m.at(2, 2) == 1);
		REQUIRE(m.at(2, 3) == 0);

		REQUIRE(m.at(3, 0) == 0);
		REQUIRE(m.at(3, 1) == 0);
		REQUIRE(m.at(3, 2) == 0);
		REQUIRE(m.at(3, 3) == 1);
	}
	SECTION("scalingMatrix4(scaleFactor)") {
		auto m = sfz::scalingMatrix4(2.0f);
		REQUIRE(approxEqual(m, sfz::toMat4(sfz::scalingMatrix3(2.0f))));

		REQUIRE(approxEqual(m.at(0, 0), 2.0f));
		REQUIRE(approxEqual(m.at(0, 1), 0.0f));
		REQUIRE(approxEqual(m.at(0, 2), 0.0f));
		REQUIRE(approxEqual(m.at(0, 3), 0.0f));

		REQUIRE(approxEqual(m.at(1, 0), 0.0f));
		REQUIRE(approxEqual(m.at(1, 1), 2.0f));
		REQUIRE(approxEqual(m.at(1, 2), 0.0f));
		REQUIRE(approxEqual(m.at(1, 3), 0.0f));

		REQUIRE(approxEqual(m.at(2, 0), 0.0f));
		REQUIRE(approxEqual(m.at(2, 1), 0.0f));
		REQUIRE(approxEqual(m.at(2, 2), 2.0f));
		REQUIRE(approxEqual(m.at(2, 3), 0.0f));

		REQUIRE(approxEqual(m.at(3, 0), 0.0f));
		REQUIRE(approxEqual(m.at(3, 1), 0.0f));
		REQUIRE(approxEqual(m.at(3, 2), 0.0f));
		REQUIRE(approxEqual(m.at(3, 3), 1.0f));

		auto v2 = m*v1;
		REQUIRE(approxEqual(v2[0], 2.0f));
		REQUIRE(approxEqual(v2[1], 2.0f));
		REQUIRE(approxEqual(v2[2], 2.0f));
		REQUIRE(approxEqual(v2[3], 1.0f));
	}
	SECTION("scalingMatrix4(scaleX, scaleY, scaleZ)") {
		auto m = sfz::scalingMatrix4(2.0f, 3.0f, 4.0f);
		REQUIRE(approxEqual(m, sfz::toMat4(sfz::scalingMatrix3(2.0f, 3.0f, 4.0f))));

		REQUIRE(approxEqual(m.at(0, 0), 2.0f));
		REQUIRE(approxEqual(m.at(0, 1), 0.0f));
		REQUIRE(approxEqual(m.at(0, 2), 0.0f));
		REQUIRE(approxEqual(m.at(0, 3), 0.0f));

		REQUIRE(approxEqual(m.at(1, 0), 0.0f));
		REQUIRE(approxEqual(m.at(1, 1), 3.0f));
		REQUIRE(approxEqual(m.at(1, 2), 0.0f));
		REQUIRE(approxEqual(m.at(1, 3), 0.0f));

		REQUIRE(approxEqual(m.at(2, 0), 0.0f));
		REQUIRE(approxEqual(m.at(2, 1), 0.0f));
		REQUIRE(approxEqual(m.at(2, 2), 4.0f));
		REQUIRE(approxEqual(m.at(2, 3), 0.0f));

		REQUIRE(approxEqual(m.at(3, 0), 0.0f));
		REQUIRE(approxEqual(m.at(3, 1), 0.0f));
		REQUIRE(approxEqual(m.at(3, 2), 0.0f));
		REQUIRE(approxEqual(m.at(3, 3), 1.0f));

		auto v2 = m*v1;
		REQUIRE(approxEqual(v2[0], 2.0f));
		REQUIRE(approxEqual(v2[1], 3.0f));
		REQUIRE(approxEqual(v2[2], 4.0f));
		REQUIRE(approxEqual(v2[3], 1.0f));
	}
	SECTION("translationMatrix()") {
		auto m = sfz::translationMatrix(-2.0f, 1.0f, 0.0f);

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

		auto v2 = m*v1;
		REQUIRE(approxEqual(v2[0], -1.0f));
		REQUIRE(approxEqual(v2[1], 2.0f));
		REQUIRE(approxEqual(v2[2], 1.0f));
		REQUIRE(approxEqual(v2[3], 1.0f));
	}
}

TEST_CASE("Projection matrices (Standard OpenGL)", "[sfz::ProjectionMatrices]")
{
	SECTION("orthogonalProjectionGL()") {
		auto m = sfz::orthogonalProjectionGL(-4.f, -2.f, 3.f, 2.f, 10.f, -50.f);

		REQUIRE(approxEqual(m.at(0, 0), 0.285714f));
		REQUIRE(approxEqual(m.at(0, 1), 0.0f));
		REQUIRE(approxEqual(m.at(0, 2), 0.0f));
		REQUIRE(approxEqual(m.at(0, 3), 0.142857f));

		REQUIRE(approxEqual(m.at(1, 0), 0.0f));
		REQUIRE(approxEqual(m.at(1, 1), 0.5f));
		REQUIRE(approxEqual(m.at(1, 2), 0.0f));
		REQUIRE(approxEqual(m.at(1, 3), 0.0f));

		REQUIRE(approxEqual(m.at(2, 0), 0.0f));
		REQUIRE(approxEqual(m.at(2, 1), 0.0f));
		REQUIRE(approxEqual(m.at(2, 2), 0.0333333f));
		REQUIRE(approxEqual(m.at(2, 3), -0.666667f));

		REQUIRE(approxEqual(m.at(3, 0), 0.0f));
		REQUIRE(approxEqual(m.at(3, 1), 0.0f));
		REQUIRE(approxEqual(m.at(3, 2), 0.0f));
		REQUIRE(approxEqual(m.at(3, 3), 1.0f));
	}
	SECTION("orthogonalProjection2DGL()") {
		sfz::vec2 pos{-3.0f, 3.0f};
		sfz::vec2 dim{4.0f, 2.0f};
		auto m = sfz::orthogonalProjection2DGL(pos, dim);

		sfz::vec3 center3{pos[0], pos[1], 1.0f};
		sfz::vec3 left3{pos[0]-(dim[0]/2), pos[1], 1.0f};
		sfz::vec3 right3{pos[0]+(dim[0]/2), pos[1], 1.0f};
		sfz::vec3 bottom3{pos[0], pos[1]-(dim[1]/2), 1.0f};
		sfz::vec3 top3{pos[0], pos[1]+(dim[1]/2), 1.0f};

		REQUIRE(approxEqual(m*center3, sfz::vec3{0.0f, 0.0f, 1.0f}));
		REQUIRE(approxEqual(m*left3, sfz::vec3{-1.0f, 0.0f, 1.0f}));
		REQUIRE(approxEqual(m*right3, sfz::vec3{1.0f, 0.0f, 1.0f}));
		REQUIRE(approxEqual(m*bottom3, sfz::vec3{0.0f, -1.0f, 1.0f}));
		REQUIRE(approxEqual(m*top3, sfz::vec3{0.0f, 1.0f, 1.0f}));
	}
	SECTION("perspectiveProjectionGL()") {
		auto m = sfz::perspectiveProjectionGL(90.0f, 1.7778f, 0.01f, 500.0f);

		REQUIRE(approxEqual(m.at(0, 0), 0.562493f));
		REQUIRE(approxEqual(m.at(0, 1), 0.0f));
		REQUIRE(approxEqual(m.at(0, 2), 0.0f));
		REQUIRE(approxEqual(m.at(0, 3), 0.0f));

		REQUIRE(approxEqual(m.at(1, 0), 0.0f));
		REQUIRE(approxEqual(m.at(1, 1), 1.0f));
		REQUIRE(approxEqual(m.at(1, 2), 0.0f));
		REQUIRE(approxEqual(m.at(1, 3), 0.0f));

		REQUIRE(approxEqual(m.at(2, 0), 0.0f));
		REQUIRE(approxEqual(m.at(2, 1), 0.0f));
		REQUIRE(approxEqual(m.at(2, 2), -1.00004f));
		REQUIRE(approxEqual(m.at(2, 3), -0.0200004f));

		REQUIRE(approxEqual(m.at(3, 0), 0.0f));
		REQUIRE(approxEqual(m.at(3, 1), 0.0f));
		REQUIRE(approxEqual(m.at(3, 2), -1.0f));
		REQUIRE(approxEqual(m.at(3, 3), 0.0f));
	}
}

TEST_CASE("Tranform helper functions", "[sfz::Matrix]")
{
	sfz::mat4 m{{1, 2, 3, 4},
	            {5, 6, 7, 8},
	            {9, 10, 11, 12},
	            {13, 14, 15, 16}};

	const sfz::mat4 rotated = sfz::zRotationMatrix4(sfz::PI()/2)
	                           * sfz::xRotationMatrix4(-sfz::PI()/2);
	const sfz::vec3 rotatedForward{-1, 0, 0};
	const sfz::vec3 rotatedUp{0, 0, -1};
	const sfz::vec3 rotatedRight{0, 1, 0};

	SECTION("translation()") {
		auto v1 = translation(m);
		REQUIRE(approxEqual(m.at(0, 3), v1[0]));
		REQUIRE(approxEqual(m.at(1, 3), v1[1]));
		REQUIRE(approxEqual(m.at(2, 3), v1[2]));

		sfz::vec3 v2{-1, -2, -3};
		translation(m, v2);
		REQUIRE(approxEqual(m.at(0, 3), v2[0]));
		REQUIRE(approxEqual(m.at(1, 3), v2[1]));
		REQUIRE(approxEqual(m.at(2, 3), v2[2]));
	}
	SECTION("scaling()") {
		auto v1 = scaling(m);
		REQUIRE(approxEqual(m.at(0, 0), v1[0]));
		REQUIRE(approxEqual(m.at(1, 1), v1[1]));
		REQUIRE(approxEqual(m.at(2, 2), v1[2]));

		sfz::vec3 v2{-1, -2, -3};
		scaling(m, v2);
		REQUIRE(approxEqual(m.at(0, 0), v2[0]));
		REQUIRE(approxEqual(m.at(1, 1), v2[1]));
		REQUIRE(approxEqual(m.at(2, 2), v2[2]));
	}
	SECTION("forward()") {
		auto v1 = forward(m);
		REQUIRE(approxEqual(m.at(0, 2), v1[0]));
		REQUIRE(approxEqual(m.at(1, 2), v1[1]));
		REQUIRE(approxEqual(m.at(2, 2), v1[2]));

		sfz::vec3 v2{-1, -2, -3};
		forward(m, v2);
		REQUIRE(approxEqual(m.at(0, 2), v2[0]));
		REQUIRE(approxEqual(m.at(1, 2), v2[1]));
		REQUIRE(approxEqual(m.at(2, 2), v2[2]));

		auto v3 = forward(rotated);
		REQUIRE(approxEqual(v3, rotatedForward));
	}
	SECTION("backward()") {
		auto v1 = backward(m);
		REQUIRE(approxEqual(m.at(0, 2), -v1[0]));
		REQUIRE(approxEqual(m.at(1, 2), -v1[1]));
		REQUIRE(approxEqual(m.at(2, 2), -v1[2]));

		sfz::vec3 v2{-1, -2, -3};
		backward(m, v2);
		REQUIRE(approxEqual(m.at(0, 2), -v2[0]));
		REQUIRE(approxEqual(m.at(1, 2), -v2[1]));
		REQUIRE(approxEqual(m.at(2, 2), -v2[2]));

		auto v3 = backward(rotated);
		REQUIRE(approxEqual(v3, -rotatedForward));
	}
	SECTION("up()") {
		auto v1 = up(m);
		REQUIRE(approxEqual(m.at(0, 1), v1[0]));
		REQUIRE(approxEqual(m.at(1, 1), v1[1]));
		REQUIRE(approxEqual(m.at(2, 1), v1[2]));

		sfz::vec3 v2{-1, -2, -3};
		up(m, v2);
		REQUIRE(approxEqual(m.at(0, 1), v2[0]));
		REQUIRE(approxEqual(m.at(1, 1), v2[1]));
		REQUIRE(approxEqual(m.at(2, 1), v2[2]));

		auto v3 = up(rotated);
		REQUIRE(approxEqual(v3, rotatedUp));
	}
	SECTION("down()") {
		auto v1 = down(m);
		REQUIRE(approxEqual(m.at(0, 1), -v1[0]));
		REQUIRE(approxEqual(m.at(1, 1), -v1[1]));
		REQUIRE(approxEqual(m.at(2, 1), -v1[2]));

		sfz::vec3 v2{-1, -2, -3};
		down(m, v2);
		REQUIRE(approxEqual(m.at(0, 1), -v2[0]));
		REQUIRE(approxEqual(m.at(1, 1), -v2[1]));
		REQUIRE(approxEqual(m.at(2, 1), -v2[2]));

		auto v3 = down(rotated);
		REQUIRE(approxEqual(v3, -rotatedUp));
	}
	SECTION("right()") {
		auto v1 = right(m);
		REQUIRE(approxEqual(m.at(0, 0), v1[0]));
		REQUIRE(approxEqual(m.at(1, 0), v1[1]));
		REQUIRE(approxEqual(m.at(2, 0), v1[2]));

		sfz::vec3 v2{-1, -2, -3};
		right(m, v2);
		REQUIRE(approxEqual(m.at(0, 0), v2[0]));
		REQUIRE(approxEqual(m.at(1, 0), v2[1]));
		REQUIRE(approxEqual(m.at(2, 0), v2[2]));

		auto v3 = right(rotated);
		REQUIRE(approxEqual(v3, rotatedRight));
	}
	SECTION("left()") {
		auto v1 = left(m);
		REQUIRE(approxEqual(m.at(0, 0), -v1[0]));
		REQUIRE(approxEqual(m.at(1, 0), -v1[1]));
		REQUIRE(approxEqual(m.at(2, 0), -v1[2]));

		sfz::vec3 v2{-1, -2, -3};
		left(m, v2);
		REQUIRE(approxEqual(m.at(0, 0), -v2[0]));
		REQUIRE(approxEqual(m.at(1, 0), -v2[1]));
		REQUIRE(approxEqual(m.at(2, 0), -v2[2]));

		auto v3 = left(rotated);
		REQUIRE(approxEqual(v3, -rotatedRight));
	}
}
