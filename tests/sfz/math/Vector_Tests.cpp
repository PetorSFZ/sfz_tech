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

#include "sfz/math/Vector.hpp"
#include "sfz/math/MathPrimitiveToStrings.hpp"
#include "sfz/math/MathSupport.hpp"

#include <type_traits>

TEST_CASE("Vector<T,2> specialization", "[sfz::Vector]")
{
	sfz::Vector<int,2> v;
	SECTION("Data") {
		REQUIRE(sizeof(sfz::Vector<int,2>) == sizeof(int)*2);
		v.data()[0] = 1;
		v.data()[1] = 2;
		REQUIRE(v.x == 1);
		REQUIRE(v.y == 2);
	}
	SECTION("Array pointer constructor") {
		int arr[] = {1, 2, 3};
		sfz::Vector<int,2> v1{arr};
		sfz::Vector<int,2> v2{arr+1};
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 2);
		REQUIRE(v2[0] == 2);
		REQUIRE(v2[1] == 3);
	}
	SECTION("Fill constructor") {
		sfz::Vector<int,2> v1{3};
		REQUIRE(v1.x == 3);
		REQUIRE(v1.y == 3);
	}
	SECTION("Constructor (x, y)") {
		sfz::Vector<int,2> v1{3, -1};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
	}
	SECTION("Cast constructor") {
		sfz::vec2i v1(sfz::vec2(-1.0f, 1.0f));
		REQUIRE(v1.x == -1);
		REQUIRE(v1.y == 1);
	}
	SECTION("Access [] operator") {
		v[0] = 4;
		v[1] = -2;
		REQUIRE(v[0] == 4);
		REQUIRE(v[1] == -2);
	}
}

TEST_CASE("Vector<T,3> specialization", "[sfz::Vector]")
{
	sfz::Vector<int,3> v;
	SECTION("Data") {
		REQUIRE(sizeof(sfz::Vector<int,3>) == sizeof(int)*3);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		REQUIRE(v.x == 1);
		REQUIRE(v.y == 2);
		REQUIRE(v.z == 3);
		REQUIRE(v.xy == (sfz::Vector<int,2>{1, 2}));
		REQUIRE(v.yz == (sfz::Vector<int,2>{2, 3}));
	}
	SECTION("Array pointer constructor") {
		int arr[] = {1, 2, 3, 4};
		sfz::Vector<int,3> v1{arr};
		sfz::Vector<int,3> v2{arr+1};
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 2);
		REQUIRE(v1[2] == 3);
		REQUIRE(v2[0] == 2);
		REQUIRE(v2[1] == 3);
		REQUIRE(v2[2] == 4);
	}
	SECTION("Fill constructor") {
		sfz::Vector<int,3> v1{3};
		REQUIRE(v1.x == 3);
		REQUIRE(v1.y == 3);
		REQUIRE(v1.z == 3);
	}
	SECTION("Constructor (x, y, z)") {
		sfz::Vector<int,3> v1{3, -1, -2};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
	}
	SECTION("Constructor (xy, z)") {
		sfz::Vector<int,3> v1{sfz::Vector<int,2>{3, -1}, -2};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
	}
	SECTION("Constructor (x, yz)") {
		sfz::Vector<int,3> v1{3, sfz::Vector<int,2>{-1, -2}};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
	}
	SECTION("Cast constructor") {
		sfz::vec3i v1(sfz::vec3(-1.0f, 1.0f, -2.0f));
		REQUIRE(v1.x == -1);
		REQUIRE(v1.y == 1);
		REQUIRE(v1.z == -2);
	}
	SECTION("Access [] operator") {
		v[0] = 4;
		v[1] = -2;
		v[2] = 1;
		REQUIRE(v[0] == 4);
		REQUIRE(v[1] == -2);
		REQUIRE(v[2] == 1);
	}
}

TEST_CASE("Vector<T,4> specialization", "[sfz::Vector]")
{
	sfz::Vector<int,4> v;
	SECTION("Data") {
		REQUIRE(sizeof(sfz::Vector<int,4>) == sizeof(int)*4);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		v.data()[3] = 4;
		REQUIRE(v.x == 1);
		REQUIRE(v.y == 2);
		REQUIRE(v.z == 3);
		REQUIRE(v.w == 4);
		REQUIRE(v.xyz == (sfz::Vector<int,3>{1, 2, 3}));
		REQUIRE(v.yzw == (sfz::Vector<int,3>{2, 3, 4}));
		REQUIRE(v.xy == (sfz::Vector<int,2>{1, 2}));
		REQUIRE(v.zw == (sfz::Vector<int,2>{3, 4}));
		REQUIRE(v.yz == (sfz::Vector<int,2>{2, 3}));
	}
	SECTION("Array pointer constructor") {
		int arr[] = {1, 2, 3, 4, 5};
		sfz::Vector<int,4> v1{arr};
		sfz::Vector<int,4> v2{arr+1};
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 2);
		REQUIRE(v1[2] == 3);
		REQUIRE(v1[3] == 4);
		REQUIRE(v2[0] == 2);
		REQUIRE(v2[1] == 3);
		REQUIRE(v2[2] == 4);
		REQUIRE(v2[3] == 5);
	}
	SECTION("Fill constructor") {
		sfz::Vector<int,4> v1{3};
		REQUIRE(v1.x == 3);
		REQUIRE(v1.y == 3);
		REQUIRE(v1.z == 3);
		REQUIRE(v1.w == 3);
	}
	SECTION("Constructor (x, y, z, w)") {
		sfz::Vector<int,4> v1{3, -1, -2, 9};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (xyz, w)") {
		sfz::Vector<int,4> v1{sfz::Vector<int,3>{3, -1, -2}, 9};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (x, yzw)") {
		sfz::Vector<int,4> v1{3, sfz::Vector<int,3>{-1, -2, 9}};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (xy, zw)") {
		sfz::Vector<int,4> v1{sfz::Vector<int,2>{3, -1}, sfz::Vector<int,2>{-2, 9}};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (xy, z, w)") {
		sfz::Vector<int,4> v1{sfz::Vector<int,2>{3, -1}, -2, 9};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (x, yz, w)") {
		sfz::Vector<int,4> v1{3, sfz::Vector<int,2>{-1, -2}, 9};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (x, y, zw)") {
		sfz::Vector<int,4> v1{3, -1, sfz::Vector<int,2>{-2, 9}};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Cast constructor") {
		sfz::vec4i v1(sfz::vec4(-1.0f, 1.0f, -2.0f, 4.0f));
		REQUIRE(v1.x == -1);
		REQUIRE(v1.y == 1);
		REQUIRE(v1.z == -2);
		REQUIRE(v1.w == 4);
	}
	SECTION("Access [] operator") {
		v[0] = 4;
		v[1] = -2;
		v[2] = 1;
		v[3] = 9;
		REQUIRE(v[0] == 4);
		REQUIRE(v[1] == -2);
		REQUIRE(v[2] == 1);
		REQUIRE(v[3] == 9);
	}
}

TEST_CASE("Vector<T,N> general definition", "[sfz::Vector]")
{
	sfz::Vector<int,5> v;
	SECTION("Data") {
		REQUIRE(sizeof(sfz::Vector<int,5>) == sizeof(int)*5);
		REQUIRE(sizeof(v.elements) == sizeof(int)*5);
	}
	SECTION("Array pointer constructor") {
		int arr[] = {1, 2, 3, 4, 5, 6};
		sfz::Vector<int,5> v1{arr};
		sfz::Vector<int,5> v2{arr+1};
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 2);
		REQUIRE(v1[2] == 3);
		REQUIRE(v1[3] == 4);
		REQUIRE(v1[4] == 5);
		REQUIRE(v2[0] == 2);
		REQUIRE(v2[1] == 3);
		REQUIRE(v2[2] == 4);
		REQUIRE(v2[3] == 5);
		REQUIRE(v2[4] == 6);
	}
	SECTION("Cast constructor")
	{
		float numbers[] = {-1.0f, 1.0f, -2.0f, 4.0, -6.0f};
		sfz::Vector<float,5> vf(numbers);
		sfz::Vector<int,5> vi(vf);
		REQUIRE(vi[0] == -1);
		REQUIRE(vi[1] == 1);
		REQUIRE(vi[2] == -2);
		REQUIRE(vi[3] == 4);
		REQUIRE(vi[4] == -6);
	}
	SECTION("Access [] operator") {
		v[0] = 4;
		v[1] = -2;
		v[2] = 1;
		v[3] = 27;
		v[4] = -9;
		REQUIRE(v[0] == 4);
		REQUIRE(v[1] == -2);
		REQUIRE(v[2] == 1);
		REQUIRE(v[3] == 27);
		REQUIRE(v[4] == -9);
	}
}

TEST_CASE("Arithmetic operators", "[sfz::Vector]")
{
	sfz::Vector<int, 3> v1{1, -2, 5};
	sfz::Vector<int, 3> v2{0, -2, 1};

	// Integrity check of base vectors
	REQUIRE(v1[0] == 1);
	REQUIRE(v1[1] == -2);
	REQUIRE(v1[2] == 5);
	REQUIRE(v2[0] == 0);
	REQUIRE(v2[1] == -2);
	REQUIRE(v2[2] == 1);

	SECTION("Addition") {
		auto v3 = v1 + v2;
		REQUIRE(v3[0] == 1);
		REQUIRE(v3[1] == -4);
		REQUIRE(v3[2] == 6);
		// Integrity check of base vectors
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == -2);
		REQUIRE(v1[2] == 5);
		REQUIRE(v2[0] == 0);
		REQUIRE(v2[1] == -2);
		REQUIRE(v2[2] == 1);
	}
	SECTION("Subtraction") {
		auto v3 = v1 - v2;
		REQUIRE(v3[0] == 1);
		REQUIRE(v3[1] == 0);
		REQUIRE(v3[2] == 4);
		auto v4 = v2 - v1;
		REQUIRE(v4[0] == -1);
		REQUIRE(v4[1] == 0);
		REQUIRE(v4[2] == -4);
		// Integrity check of base vectors
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == -2);
		REQUIRE(v1[2] == 5);
		REQUIRE(v2[0] == 0);
		REQUIRE(v2[1] == -2);
		REQUIRE(v2[2] == 1);
	}
	SECTION("Negating (-x)") {
		auto v3 = -v1;
		REQUIRE(v3[0] == -1);
		REQUIRE(v3[1] == 2);
		REQUIRE(v3[2] == -5);
		// Integrity check of base vectors
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == -2);
		REQUIRE(v1[2] == 5);
		REQUIRE(v2[0] == 0);
		REQUIRE(v2[1] == -2);
		REQUIRE(v2[2] == 1);
	}
	SECTION("Multiplication by number") {
		auto v3 = v1*3;
		REQUIRE(v3[0] == 3);
		REQUIRE(v3[1] == -6);
		REQUIRE(v3[2] == 15);
		auto v4 = -3*v2;
		REQUIRE(v4[0] == 0);
		REQUIRE(v4[1] == 6);
		REQUIRE(v4[2] == -3);
		// Integrity check of base vectors
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == -2);
		REQUIRE(v1[2] == 5);
		REQUIRE(v2[0] == 0);
		REQUIRE(v2[1] == -2);
		REQUIRE(v2[2] == 1);
	}
	SECTION("Element-wise multiplication") {
		auto v3 = v1*v2;
		REQUIRE(v3[0] == 0);
		REQUIRE(v3[1] == 4);
		REQUIRE(v3[2] == 5);
		// Integrity check of base vectors
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == -2);
		REQUIRE(v1[2] == 5);
		REQUIRE(v2[0] == 0);
		REQUIRE(v2[1] == -2);
		REQUIRE(v2[2] == 1);
	}
	SECTION("Division by number") {
		auto v3 = sfz::Vector<int,2>{2, -2}/2;
		REQUIRE(v3[0] == 1);
		REQUIRE(v3[1] == -1);
	}
	SECTION("Element-wise division") {
		auto v3 = v1 / v1;
		REQUIRE(v3[0] == 1);
		REQUIRE(v3[1] == 1);
		REQUIRE(v3[2] == 1);
	}
	SECTION("Addition assignment") {
		v1 += v2;
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == -4);
		REQUIRE(v1[2] == 6);
	}
	SECTION("Subtraction assignment") {
		v1 -= v2;
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 0);
		REQUIRE(v1[2] == 4);
	}
	SECTION("Multiplication by number assignment") {
		v1 *= 3;
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -6);
		REQUIRE(v1[2] == 15);
	}
	SECTION("Element-wise multiplication assignment") {
		v1 *= v2;
		REQUIRE(v1[0] == 0);
		REQUIRE(v1[1] == 4);
		REQUIRE(v1[2] == 5);
	}
	SECTION("Division by number assignment") {
		sfz::Vector<int, 2> v3{2, -2};
		v3 /= 2;
		REQUIRE(v3[0] == 1);
		REQUIRE(v3[1] == -1);
	}
	SECTION("Element-wise division assignment") {
		v1 /= v1;
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 1);
		REQUIRE(v1[2] == 1);
	}
}

TEST_CASE("Length of vector", "[sfz::Vector]")
{
	using sfz::length;
	using sfz::approxEqual;
	sfz::vec2 v1(2.0f, 0.0f);
	float v2Arr[] = {-2.0f, 2.0f, 2.0f, -2.0f};
	sfz::vec4 v2(v2Arr);

	SECTION("length()") {
		REQUIRE(approxEqual(length(v1), 2.0f));
		REQUIRE(approxEqual(length(v2), 4.0f));
	}
}

TEST_CASE("Normalizing (making unit vector) vector", "[sfz::Vector]")
{
	sfz::Vector<float, 4> v1 = normalize(sfz::Vector<float, 4>{-2.f, 2.f, -2.f, 2.f});
	const float delta = 1e-3f;

	SECTION("Correct answer") {
		const float posLower = 0.5f - delta;
		const float posHigher = 0.5f + delta;
		const float negLower = -0.5f - delta;
		const float negHigher = -0.5f + delta;		

		REQUIRE(negLower <= v1[0]);
		REQUIRE(v1[0] <= negHigher);

		REQUIRE(posLower <= v1[1]);
		REQUIRE(v1[1] <= posHigher);

		REQUIRE(negLower <= v1[2]);
		REQUIRE(v1[2] <= negHigher);

		REQUIRE(posLower <= v1[3]);
		REQUIRE(v1[3] <= posHigher);
	}
	SECTION("safeNormalize()") {
		REQUIRE(safeNormalize(sfz::vec3(0.0f)) == sfz::vec3(0.0f));
	}
}

TEST_CASE("Comparison operators", "[sfz::Vector]")
{
	sfz::Vector<int, 3> v1{-4, 0, 0};
	sfz::Vector<int, 3> v2{0, 2, 0};
	sfz::Vector<int, 3> v3{0, 2, 0};

	SECTION("== and !=") {
		REQUIRE(v1 == v1);
		REQUIRE(v2 == v2);
		REQUIRE(v3 == v3);	
		REQUIRE(v2 == v3);
		REQUIRE(v3 == v2);
		REQUIRE(v1 != v2);
		REQUIRE(v2 != v1);
	}
}

TEST_CASE("Dot (scalar) product", "[sfz::Vector]")
{
	using sfz::dot;
	SECTION("Correctness test") {
		sfz::Vector<int, 3> v1{1, 0, -2};
		sfz::Vector<int, 3> v2{6, 2, 2};
		int scalarProduct = dot(v1, v2);
		
		REQUIRE(scalarProduct == 2);
		
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 0);
		REQUIRE(v1[2] == -2);
		REQUIRE(v2[0] == 6);
		REQUIRE(v2[1] == 2);
		REQUIRE(v2[2] == 2);
	}
	SECTION("Using same vector twice") {	
		sfz::Vector<int, 2> v1{-3, 2};
		int scalarProduct = dot(v1, v1);
		
		REQUIRE(scalarProduct == 13);
		
		REQUIRE(v1[0] == -3);
		REQUIRE(v1[1] == 2);	
	}
	SECTION("_mm_dp_ps()") {
		sfz::vec4 v1(1.0f, 2.0f, 3.0f, 4.0f);
		sfz::vec4 v2(3.0f, -1.0f, -2.0f, 5.0f);
		REQUIRE(sfz::approxEqual(sfz::dot(v1, v2), 15.0f));
	}
}

TEST_CASE("Cross product", "[sfz::Vector]")
{
	sfz::Vector<int, 3> v1{-1, 4, 0};
	sfz::Vector<int, 3> v2{1, -2, 3};

	SECTION("Correctness test") {
		auto res = sfz::cross(v1, v2);

		REQUIRE(res[0] == 12);
		REQUIRE(res[1] == 3);
		REQUIRE(res[2] == -2);
	}
	SECTION("2nd correctness test") {
		auto res = sfz::cross(v2, v1);

		REQUIRE(res[0] == -12);
		REQUIRE(res[1] == -3);
		REQUIRE(res[2] == 2);
	}
	SECTION("A x A = 0") {
		auto res1 = sfz::cross(v1, v1);
		REQUIRE(res1[0] == 0);
		REQUIRE(res1[1] == 0);
		REQUIRE(res1[2] == 0);

		auto res2 = sfz::cross(v2, v2);
		REQUIRE(res2[0] == 0);
		REQUIRE(res2[1] == 0);
		REQUIRE(res2[2] == 0);
	}
}

TEST_CASE("Sum of vector", "[sfz::Vector]")
{
	using sfz::elementSum;
	sfz::Vector<int, 4> v1{1, 2, -4, 9};
	REQUIRE(elementSum(v1) == 8);
}

TEST_CASE("Converting to string", "[sfz::Vector]")
{
	using namespace sfz;
	vec3i v{-1, 2, 10};
	REQUIRE(toString(v) == "[-1, 2, 10]");

	vec4 v2{1.0f, 2.0f, 3.0f, 4.0f};
	REQUIRE(toString(v2, 1) == "[1.0, 2.0, 3.0, 4.0]");
}

TEST_CASE("Is proper POD", "[sfz::Vector]")
{
	REQUIRE(std::is_trivially_default_constructible<sfz::vec2>::value);
	REQUIRE(std::is_trivially_default_constructible<sfz::vec2i>::value);
	REQUIRE(std::is_trivially_default_constructible<sfz::vec3>::value);
	REQUIRE(std::is_trivially_default_constructible<sfz::vec3i>::value);

	REQUIRE(std::is_trivially_copyable<sfz::vec2>::value);
	REQUIRE(std::is_trivially_copyable<sfz::vec2i>::value);
	REQUIRE(std::is_trivially_copyable<sfz::vec3>::value);
	REQUIRE(std::is_trivially_copyable<sfz::vec3i>::value);

	REQUIRE(std::is_trivial<sfz::vec2>::value);
	REQUIRE(std::is_trivial<sfz::vec2i>::value);
	REQUIRE(std::is_trivial<sfz::vec3>::value);
	REQUIRE(std::is_trivial<sfz::vec3i>::value);

	REQUIRE(std::is_standard_layout<sfz::vec2>::value);
	REQUIRE(std::is_standard_layout<sfz::vec2i>::value);
	REQUIRE(std::is_standard_layout<sfz::vec3>::value);
	REQUIRE(std::is_standard_layout<sfz::vec3i>::value);

	REQUIRE(std::is_pod<sfz::vec2>::value);
	REQUIRE(std::is_pod<sfz::vec2i>::value);
	REQUIRE(std::is_pod<sfz::vec3>::value);
	REQUIRE(std::is_pod<sfz::vec3i>::value);
}