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

#include "sfz/math/Vector.hpp"
#include "sfz/math/MathPrimitiveToStrings.hpp"
#include "sfz/math/MathSupport.hpp"

#include <type_traits>

TEST_CASE("Vec<T,2> specialization", "[sfz::Vec]")
{
	sfz::Vec<int,2> v;
	SECTION("Data") {
		REQUIRE(sizeof(sfz::Vec<int,2>) == sizeof(int)*2);
		v.data()[0] = 1;
		v.data()[1] = 2;
		REQUIRE(v.x == 1);
		REQUIRE(v.y == 2);
	}
	SECTION("Array pointer constructor") {
		int arr[] = {1, 2, 3};
		sfz::Vec<int,2> v1{arr};
		sfz::Vec<int,2> v2{arr+1};
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 2);
		REQUIRE(v2[0] == 2);
		REQUIRE(v2[1] == 3);
	}
	SECTION("Fill constructor") {
		sfz::Vec<int,2> v1{3};
		REQUIRE(v1.x == 3);
		REQUIRE(v1.y == 3);
	}
	SECTION("Constructor (x, y)") {
		sfz::Vec<int,2> v1{3, -1};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
	}
	SECTION("Cast constructor") {
		sfz::vec2_i32 v1(sfz::vec2(-1.0f, 1.0f));
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

TEST_CASE("Vec<T,3> specialization", "[sfz::Vec]")
{
	sfz::Vec<int,3> v;
	SECTION("Data") {
		REQUIRE(sizeof(sfz::Vec<int,3>) == sizeof(int)*3);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		REQUIRE(v.x == 1);
		REQUIRE(v.y == 2);
		REQUIRE(v.z == 3);
		REQUIRE(v.xy == (sfz::Vec<int,2>{1, 2}));
		REQUIRE(v.yz == (sfz::Vec<int,2>{2, 3}));
	}
	SECTION("Array pointer constructor") {
		int arr[] = {1, 2, 3, 4};
		sfz::Vec<int,3> v1{arr};
		sfz::Vec<int,3> v2{arr+1};
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 2);
		REQUIRE(v1[2] == 3);
		REQUIRE(v2[0] == 2);
		REQUIRE(v2[1] == 3);
		REQUIRE(v2[2] == 4);
	}
	SECTION("Fill constructor") {
		sfz::Vec<int,3> v1{3};
		REQUIRE(v1.x == 3);
		REQUIRE(v1.y == 3);
		REQUIRE(v1.z == 3);
	}
	SECTION("Constructor (x, y, z)") {
		sfz::Vec<int,3> v1{3, -1, -2};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
	}
	SECTION("Constructor (xy, z)") {
		sfz::Vec<int,3> v1{sfz::Vec<int,2>{3, -1}, -2};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
	}
	SECTION("Constructor (x, yz)") {
		sfz::Vec<int,3> v1{3, sfz::Vec<int,2>{-1, -2}};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
	}
	SECTION("Cast constructor") {
		sfz::vec3_i32 v1(sfz::vec3(-1.0f, 1.0f, -2.0f));
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

TEST_CASE("Vec<T,4> specialization", "[sfz::Vec]")
{
	sfz::Vec<int,4> v;
	SECTION("Data") {
		REQUIRE(sizeof(sfz::Vec<int,4>) == sizeof(int)*4);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		v.data()[3] = 4;
		REQUIRE(v.x == 1);
		REQUIRE(v.y == 2);
		REQUIRE(v.z == 3);
		REQUIRE(v.w == 4);
		REQUIRE(v.xyz == (sfz::Vec<int,3>{1, 2, 3}));
		REQUIRE(v.yzw == (sfz::Vec<int,3>{2, 3, 4}));
		REQUIRE(v.xy == (sfz::Vec<int,2>{1, 2}));
		REQUIRE(v.zw == (sfz::Vec<int,2>{3, 4}));
		REQUIRE(v.yz == (sfz::Vec<int,2>{2, 3}));
	}
	SECTION("Array pointer constructor") {
		int arr[] = {1, 2, 3, 4, 5};
		sfz::Vec<int,4> v1{arr};
		sfz::Vec<int,4> v2{arr+1};
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
		sfz::Vec<int,4> v1{3};
		REQUIRE(v1.x == 3);
		REQUIRE(v1.y == 3);
		REQUIRE(v1.z == 3);
		REQUIRE(v1.w == 3);
	}
	SECTION("Constructor (x, y, z, w)") {
		sfz::Vec<int,4> v1{3, -1, -2, 9};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (xyz, w)") {
		sfz::Vec<int,4> v1{sfz::Vec<int,3>{3, -1, -2}, 9};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (x, yzw)") {
		sfz::Vec<int,4> v1{3, sfz::Vec<int,3>{-1, -2, 9}};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (xy, zw)") {
		sfz::Vec<int,4> v1{sfz::Vec<int,2>{3, -1}, sfz::Vec<int,2>{-2, 9}};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (xy, z, w)") {
		sfz::Vec<int,4> v1{sfz::Vec<int,2>{3, -1}, -2, 9};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (x, yz, w)") {
		sfz::Vec<int,4> v1{3, sfz::Vec<int,2>{-1, -2}, 9};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Constructor (x, y, zw)") {
		sfz::Vec<int,4> v1{3, -1, sfz::Vec<int,2>{-2, 9}};
		REQUIRE(v1[0] == 3);
		REQUIRE(v1[1] == -1);
		REQUIRE(v1[2] == -2);
		REQUIRE(v1[3] == 9);
	}
	SECTION("Cast constructor") {
		sfz::vec4_i32 v1(sfz::vec4(-1.0f, 1.0f, -2.0f, 4.0f));
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

TEST_CASE("Arithmetic operators", "[sfz::Vec]")
{
	sfz::Vec<int, 3> v1{1, -2, 5};
	sfz::Vec<int, 3> v2{0, -2, 1};

	// Integrity check of base Vecs
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
		auto v3 = sfz::Vec<int,2>{2, -2}/2;
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
		sfz::Vec<int, 2> v3{2, -2};
		v3 /= 2;
		REQUIRE(v3[0] == 1);
		REQUIRE(v3[1] == -1);
	}
	SECTION("Element-wise division assignment") {
		auto v1Copy = v1; // Not necessary, just to remove warning from Clang.
		v1 /= v1Copy;
		REQUIRE(v1[0] == 1);
		REQUIRE(v1[1] == 1);
		REQUIRE(v1[2] == 1);
	}
}

TEST_CASE("Length of vector", "[sfz::Vec]")
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

TEST_CASE("Normalizing (making unit vector) vector", "[sfz::Vec]")
{
	sfz::Vec<float, 4> v1 = normalize(sfz::Vec<float, 4>{-2.f, 2.f, -2.f, 2.f});
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
	SECTION("normalizeSafe()") {
		REQUIRE(normalizeSafe(sfz::vec3(0.0f)) == sfz::vec3(0.0f));
	}
}

TEST_CASE("Comparison operators", "[sfz::Vec]")
{
	sfz::Vec<int, 3> v1{-4, 0, 0};
	sfz::Vec<int, 3> v2{0, 2, 0};
	sfz::Vec<int, 3> v3{0, 2, 0};

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

TEST_CASE("Dot (scalar) product", "[sfz::Vec]")
{
	using sfz::dot;
	SECTION("Correctness test") {
		sfz::Vec<int, 3> v1{1, 0, -2};
		sfz::Vec<int, 3> v2{6, 2, 2};
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
		sfz::Vec<int, 2> v1{-3, 2};
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

TEST_CASE("Cross product", "[sfz::Vec]")
{
	sfz::Vec<int, 3> v1{-1, 4, 0};
	sfz::Vec<int, 3> v2{1, -2, 3};

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

TEST_CASE("sfzMin() (Vector)", "[sfz::MathSupport]")
{
	using namespace sfz;
	SECTION("Vectors") {
		REQUIRE(sfzMin(vec4(1.0f, 2.0f, -3.0f, -4.0f), vec4(2.0f, 1.0f, -5.0f, -2.0f)) == vec4(1.0f, 1.0f, -5.0f, -4.0f));
		REQUIRE(sfzMin(vec4_i32(1, 2, -3, -4), vec4_i32(2, 1, -5, -2)) == vec4_i32(1, 1, -5, -4));
		REQUIRE(sfzMin(vec4_u32(1u, 2u, 3u, 4u), vec4_u32(2u, 1u, 5u, 2u)) == vec4_u32(1u, 1u, 3u, 2u));
	}
	SECTION("Vectors & scalars")
	{
		REQUIRE(sfzMin(vec4(1.0f, 2.0f, -3.0f, -4.0f), -1.0f) == vec4(-1.0f, -1.0f, -3.0f, -4.0f));
		REQUIRE(sfzMin(vec4_i32(1, 2, -3, -4), -1) == vec4_i32(-1, -1, -3, -4));
		REQUIRE(sfzMin(vec4_u32(1u, 2u, 3u, 4u), 2u) == vec4_u32(1u, 2u, 2u, 2u));
	}
}

TEST_CASE("sfzMax() (Vector)", "[sfz::MathSupport]")
{
	using namespace sfz;
	SECTION("Scalars") {
		REQUIRE(sfzMax(1.0f, 2.0f) == 2.0f);
		REQUIRE(sfzMax(-1.0f, -2.0f) == -1.0f);
		REQUIRE(sfzMax(1, 2) == 2);
		REQUIRE(sfzMax(-1, -2) == -1);
		REQUIRE(sfzMax(1u, 2u) == 2u);
		REQUIRE(sfzMax(3u, 2u) == 3u);
	}
	SECTION("Vectors") {
		REQUIRE(sfzMax(vec4(1.0f, 2.0f, -3.0f, -4.0f), vec4(2.0f, 1.0f, -5.0f, -2.0f)) == vec4(2.0f, 2.0f, -3.0f, -2.0f));
		REQUIRE(sfzMax(vec4_i32(1, 2, -3, -4), vec4_i32(2, 1, -5, -2)) == vec4_i32(2, 2, -3, -2));
		REQUIRE(sfzMax(vec4_u32(1u, 2u, 3u, 4u), vec4_u32(2u, 1u, 5u, 2u)) == vec4_u32(2u, 2u, 5u, 4u));
	}
	SECTION("Vectors & scalars")
	{
		REQUIRE(sfzMax(vec4(1.0f, 2.0f, -3.0f, -4.0f), 1.0f) == vec4(1.0f, 2.0f, 1.0f, 1.0f));
		REQUIRE(sfzMax(vec4_i32(1, 2, -3, -4), 1) == vec4_i32(1, 2, 1, 1));
		REQUIRE(sfzMax(vec4_u32(1u, 2u, 3u, 4u), 2u) == vec4_u32(2u, 2u, 3u, 4u));
	}
}

TEST_CASE("Converting to string", "[sfz::Vec]")
{
	using namespace sfz;
	vec3_i32 v{-1, 2, 10};
	REQUIRE(toString(v) == "[-1, 2, 10]");

	vec4 v2{1.0f, 2.0f, 3.0f, 4.0f};
	REQUIRE(toString(v2, 1) == "[1.0, 2.0, 3.0, 4.0]");
}

TEST_CASE("Is proper POD", "[sfz::Vec]")
{
	REQUIRE(std::is_trivially_default_constructible<sfz::vec2>::value);
	REQUIRE(std::is_trivially_default_constructible<sfz::vec2_i32>::value);
	REQUIRE(std::is_trivially_default_constructible<sfz::vec3>::value);
	REQUIRE(std::is_trivially_default_constructible<sfz::vec3_i32>::value);

	REQUIRE(std::is_trivially_copyable<sfz::vec2>::value);
	REQUIRE(std::is_trivially_copyable<sfz::vec2_i32>::value);
	REQUIRE(std::is_trivially_copyable<sfz::vec3>::value);
	REQUIRE(std::is_trivially_copyable<sfz::vec3_i32>::value);

	REQUIRE(std::is_trivial<sfz::vec2>::value);
	REQUIRE(std::is_trivial<sfz::vec2_i32>::value);
	REQUIRE(std::is_trivial<sfz::vec3>::value);
	REQUIRE(std::is_trivial<sfz::vec3_i32>::value);

	REQUIRE(std::is_standard_layout<sfz::vec2>::value);
	REQUIRE(std::is_standard_layout<sfz::vec2_i32>::value);
	REQUIRE(std::is_standard_layout<sfz::vec3>::value);
	REQUIRE(std::is_standard_layout<sfz::vec3_i32>::value);

	REQUIRE(std::is_pod<sfz::vec2>::value);
	REQUIRE(std::is_pod<sfz::vec2_i32>::value);
	REQUIRE(std::is_pod<sfz::vec3>::value);
	REQUIRE(std::is_pod<sfz::vec3_i32>::value);
}
