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

#include "skipifzero.hpp"

#include <type_traits>

// Vector tests
// ------------------------------------------------------------------------------------------------

UTEST(Vec, vec2_specialization)
{
	// Data
	{
		sfz::Vec<int, 2> v;
		ASSERT_TRUE(sizeof(sfz::Vec<int, 2>) == sizeof(int) * 2);
		v.data()[0] = 1;
		v.data()[1] = 2;
		ASSERT_TRUE(v.x == 1);
		ASSERT_TRUE(v.y == 2);
	}
	// Array pointer constructor
	{
		int arr[] = { 1, 2, 3 };
		sfz::Vec<int, 2> v1{ arr };
		sfz::Vec<int, 2> v2{ arr + 1 };
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 2);
		ASSERT_TRUE(v2[0] == 2);
		ASSERT_TRUE(v2[1] == 3);
	}
	// Fill constructor
	{
		sfz::Vec<int, 2> v1{ 3 };
		ASSERT_TRUE(v1.x == 3);
		ASSERT_TRUE(v1.y == 3);
	}
	// Constructor (x, y)
	{
		sfz::Vec<int, 2> v1{ 3, -1 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
	}
	// Cast constructor
	{
		sfz::vec2_i32 v1(sfz::vec2(-1.0f, 1.0f));
		ASSERT_TRUE(v1.x == -1);
		ASSERT_TRUE(v1.y == 1);
	}
	// Access [] operator
	{
		sfz::Vec<int, 2> v;
		v[0] = 4;
		v[1] = -2;
		ASSERT_TRUE(v[0] == 4);
		ASSERT_TRUE(v[1] == -2);
	}
}

UTEST(Vec, vec3_specialization)
{
	// Data
	{
		sfz::Vec<int, 3> v;
		ASSERT_TRUE(sizeof(sfz::Vec<int, 3>) == sizeof(int) * 3);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		ASSERT_TRUE(v.x == 1);
		ASSERT_TRUE(v.y == 2);
		ASSERT_TRUE(v.z == 3);
		ASSERT_TRUE(v.xy == (sfz::Vec<int, 2>{1, 2}));
		ASSERT_TRUE(v.yz == (sfz::Vec<int, 2>{2, 3}));
	}
	// Array pointer constructor
	{
		int arr[] = { 1, 2, 3, 4 };
		sfz::Vec<int, 3> v1{ arr };
		sfz::Vec<int, 3> v2{ arr + 1 };
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 2);
		ASSERT_TRUE(v1[2] == 3);
		ASSERT_TRUE(v2[0] == 2);
		ASSERT_TRUE(v2[1] == 3);
		ASSERT_TRUE(v2[2] == 4);
	}
	// Fill constructor
	{
		sfz::Vec<int, 3> v1{ 3 };
		ASSERT_TRUE(v1.x == 3);
		ASSERT_TRUE(v1.y == 3);
		ASSERT_TRUE(v1.z == 3);
	}
	// Constructor (x, y, z)
	{
		sfz::Vec<int, 3> v1{ 3, -1, -2 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
	}
	// Constructor (xy, z)
	{
		sfz::Vec<int, 3> v1{ sfz::Vec<int, 2>{3, -1}, -2 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
	}
	// Constructor (x, yz)
	{
		sfz::Vec<int, 3> v1{ 3, sfz::Vec<int, 2>{-1, -2} };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
	}
	// Cast constructor
	{
		sfz::vec3_i32 v1(sfz::vec3(-1.0f, 1.0f, -2.0f));
		ASSERT_TRUE(v1.x == -1);
		ASSERT_TRUE(v1.y == 1);
		ASSERT_TRUE(v1.z == -2);
	}
	// Access operator []
	{
		sfz::Vec<int, 3> v;
		v[0] = 4;
		v[1] = -2;
		v[2] = 1;
		ASSERT_TRUE(v[0] == 4);
		ASSERT_TRUE(v[1] == -2);
		ASSERT_TRUE(v[2] == 1);
	}
}

UTEST(Vec, vec4_specialization)
{
	// Data
	{
		sfz::Vec<int, 4> v;
		ASSERT_TRUE(sizeof(sfz::Vec<int, 4>) == sizeof(int) * 4);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		v.data()[3] = 4;
		ASSERT_TRUE(v.x == 1);
		ASSERT_TRUE(v.y == 2);
		ASSERT_TRUE(v.z == 3);
		ASSERT_TRUE(v.w == 4);
		ASSERT_TRUE(v.xyz == (sfz::Vec<int, 3>{1, 2, 3}));
		ASSERT_TRUE(v.yzw == (sfz::Vec<int, 3>{2, 3, 4}));
		ASSERT_TRUE(v.xy == (sfz::Vec<int, 2>{1, 2}));
		ASSERT_TRUE(v.zw == (sfz::Vec<int, 2>{3, 4}));
		ASSERT_TRUE(v.yz == (sfz::Vec<int, 2>{2, 3}));
	}
	// Array pointer constructor
	{
		int arr[] = { 1, 2, 3, 4, 5 };
		sfz::Vec<int, 4> v1{ arr };
		sfz::Vec<int, 4> v2{ arr + 1 };
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 2);
		ASSERT_TRUE(v1[2] == 3);
		ASSERT_TRUE(v1[3] == 4);
		ASSERT_TRUE(v2[0] == 2);
		ASSERT_TRUE(v2[1] == 3);
		ASSERT_TRUE(v2[2] == 4);
		ASSERT_TRUE(v2[3] == 5);
	}
	// Fill constructor
	{
		sfz::Vec<int, 4> v1{ 3 };
		ASSERT_TRUE(v1.x == 3);
		ASSERT_TRUE(v1.y == 3);
		ASSERT_TRUE(v1.z == 3);
		ASSERT_TRUE(v1.w == 3);
	}
	// Constructor (x, y, z, w)
	{
		sfz::Vec<int, 4> v1{ 3, -1, -2, 9 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (xyz, w)
	{
		sfz::Vec<int, 4> v1{ sfz::Vec<int, 3>{3, -1, -2}, 9 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (x, yzw)
	{
		sfz::Vec<int, 4> v1{ 3, sfz::Vec<int, 3>{-1, -2, 9} };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (xy, zw)
	{
		sfz::Vec<int, 4> v1{ sfz::Vec<int, 2>{3, -1}, sfz::Vec<int, 2>{-2, 9} };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (xy, z, w)
	{
		sfz::Vec<int, 4> v1{ sfz::Vec<int, 2>{3, -1}, -2, 9 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (x, yz, w)
	{
		sfz::Vec<int, 4> v1{ 3, sfz::Vec<int, 2>{-1, -2}, 9 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (x, y, zw)
	{
		sfz::Vec<int, 4> v1{ 3, -1, sfz::Vec<int, 2>{-2, 9} };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Cast constructor
	{
		sfz::vec4_i32 v1(sfz::vec4(-1.0f, 1.0f, -2.0f, 4.0f));
		ASSERT_TRUE(v1.x == -1);
		ASSERT_TRUE(v1.y == 1);
		ASSERT_TRUE(v1.z == -2);
		ASSERT_TRUE(v1.w == 4);
	}
	// Access [] operator
	{
		sfz::Vec<int, 4> v;
		v[0] = 4;
		v[1] = -2;
		v[2] = 1;
		v[3] = 9;
		ASSERT_TRUE(v[0] == 4);
		ASSERT_TRUE(v[1] == -2);
		ASSERT_TRUE(v[2] == 1);
		ASSERT_TRUE(v[3] == 9);
	}
}

UTEST(Vec, arithmetic_operators)
{
	// Addition
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };
		sfz::Vec<int, 3> v2{ 0, -2, 1 };

		auto v3 = v1 + v2;
		ASSERT_TRUE(v3[0] == 1);
		ASSERT_TRUE(v3[1] == -4);
		ASSERT_TRUE(v3[2] == 6);
		// Integrity check of base vectors
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == -2);
		ASSERT_TRUE(v1[2] == 5);
		ASSERT_TRUE(v2[0] == 0);
		ASSERT_TRUE(v2[1] == -2);
		ASSERT_TRUE(v2[2] == 1);
	}
	// Subtraction
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };
		sfz::Vec<int, 3> v2{ 0, -2, 1 };

		auto v3 = v1 - v2;
		ASSERT_TRUE(v3[0] == 1);
		ASSERT_TRUE(v3[1] == 0);
		ASSERT_TRUE(v3[2] == 4);
		auto v4 = v2 - v1;
		ASSERT_TRUE(v4[0] == -1);
		ASSERT_TRUE(v4[1] == 0);
		ASSERT_TRUE(v4[2] == -4);
		// Integrity check of base vectors
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == -2);
		ASSERT_TRUE(v1[2] == 5);
		ASSERT_TRUE(v2[0] == 0);
		ASSERT_TRUE(v2[1] == -2);
		ASSERT_TRUE(v2[2] == 1);
	}
	// Negating (-x)
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };

		auto v3 = -v1;
		ASSERT_TRUE(v3[0] == -1);
		ASSERT_TRUE(v3[1] == 2);
		ASSERT_TRUE(v3[2] == -5);
		// Integrity check of base vectors
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == -2);
		ASSERT_TRUE(v1[2] == 5);
	}
	// Multiplication by number
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };
		sfz::Vec<int, 3> v2{ 0, -2, 1 };

		auto v3 = v1 * 3;
		ASSERT_TRUE(v3[0] == 3);
		ASSERT_TRUE(v3[1] == -6);
		ASSERT_TRUE(v3[2] == 15);
		auto v4 = -3 * v2;
		ASSERT_TRUE(v4[0] == 0);
		ASSERT_TRUE(v4[1] == 6);
		ASSERT_TRUE(v4[2] == -3);
		// Integrity check of base vectors
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == -2);
		ASSERT_TRUE(v1[2] == 5);
		ASSERT_TRUE(v2[0] == 0);
		ASSERT_TRUE(v2[1] == -2);
		ASSERT_TRUE(v2[2] == 1);
	}
	// Element-wise multiplication
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };
		sfz::Vec<int, 3> v2{ 0, -2, 1 };

		auto v3 = v1 * v2;
		ASSERT_TRUE(v3[0] == 0);
		ASSERT_TRUE(v3[1] == 4);
		ASSERT_TRUE(v3[2] == 5);
		// Integrity check of base vectors
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == -2);
		ASSERT_TRUE(v1[2] == 5);
		ASSERT_TRUE(v2[0] == 0);
		ASSERT_TRUE(v2[1] == -2);
		ASSERT_TRUE(v2[2] == 1);
	}
	// Division by number
	{
		auto v3 = sfz::Vec<int, 2>{ 2, -2 } / 2;
		ASSERT_TRUE(v3[0] == 1);
		ASSERT_TRUE(v3[1] == -1);
		auto v4 = -8 / sfz::vec2_i32(2, 4);
		ASSERT_TRUE(v4.x == -4);
		ASSERT_TRUE(v4.y == -2);
	}
	// Element-wise divison
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };
		sfz::Vec<int, 3> v2{ 0, -2, 1 };

		auto v3 = v1 / v1;
		ASSERT_TRUE(v3[0] == 1);
		ASSERT_TRUE(v3[1] == 1);
		ASSERT_TRUE(v3[2] == 1);
	}
	// Addition assignment
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };
		sfz::Vec<int, 3> v2{ 0, -2, 1 };

		v1 += v2;
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == -4);
		ASSERT_TRUE(v1[2] == 6);
	}
	// Subtraction assignment
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };
		sfz::Vec<int, 3> v2{ 0, -2, 1 };

		v1 -= v2;
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 0);
		ASSERT_TRUE(v1[2] == 4);
	}
	// Multiplication by number assignment
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };

		v1 *= 3;
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -6);
		ASSERT_TRUE(v1[2] == 15);
	}
	// Element-wise multiplication assignment
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };
		sfz::Vec<int, 3> v2{ 0, -2, 1 };

		v1 *= v2;
		ASSERT_TRUE(v1[0] == 0);
		ASSERT_TRUE(v1[1] == 4);
		ASSERT_TRUE(v1[2] == 5);
	}
	// Division by number assignment
	{
		sfz::Vec<int, 2> v3{ 2, -2 };
		v3 /= 2;
		ASSERT_TRUE(v3[0] == 1);
		ASSERT_TRUE(v3[1] == -1);
	}
	// Element-wise division assignment
	{
		sfz::Vec<int, 3> v1{ 1, -2, 5 };

		auto v1Copy = v1; // Not necessary, just to remove warning from Clang.
		v1 /= v1Copy;
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 1);
		ASSERT_TRUE(v1[2] == 1);
	}
}

UTEST(Vec, length_of_vectors)
{
	sfz::vec2 v1(2.0f, 0.0f);
	f32 v2Arr[] = { -2.0f, 2.0f, 2.0f, -2.0f };
	sfz::vec4 v2(v2Arr);

	ASSERT_TRUE(sfz::eqf(sfz::length(v1), 2.0f));
	ASSERT_TRUE(sfz::eqf(sfz::length(v2), 4.0f));
}

UTEST(Vec, normalizing_vector)
{
	sfz::vec4 v1 = sfz::normalize(sfz::vec4(-2.f, 2.f, -2.f, 2.f));
	ASSERT_TRUE(sfz::eqf(v1, sfz::vec4(-0.5f, 0.5f, -0.5f, 0.5f)));
	ASSERT_TRUE(sfz::normalizeSafe(sfz::vec3(0.0f)) == sfz::vec3(0.0f));
}

UTEST(Vec, comparison_operators)
{
	sfz::Vec<int, 3> v1{ -4, 0, 0 };
	sfz::Vec<int, 3> v2{ 0, 2, 0 };
	sfz::Vec<int, 3> v3{ 0, 2, 0 };

	ASSERT_TRUE(v1 == v1);
	ASSERT_TRUE(v2 == v2);
	ASSERT_TRUE(v3 == v3);
	ASSERT_TRUE(v2 == v3);
	ASSERT_TRUE(v3 == v2);
	ASSERT_TRUE(v1 != v2);
	ASSERT_TRUE(v2 != v1);
}

UTEST(Vec, dot_product)
{
	// Correctness test
	{
		sfz::Vec<int, 3> v1{ 1, 0, -2 };
		sfz::Vec<int, 3> v2{ 6, 2, 2 };
		int scalarProduct = dot(v1, v2);

		ASSERT_TRUE(scalarProduct == 2);

		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 0);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v2[0] == 6);
		ASSERT_TRUE(v2[1] == 2);
		ASSERT_TRUE(v2[2] == 2);
	}
	// Using same vector twice
	{
		sfz::Vec<int, 2> v1{ -3, 2 };
		int scalarProduct = dot(v1, v1);

		ASSERT_TRUE(scalarProduct == 13);

		ASSERT_TRUE(v1[0] == -3);
		ASSERT_TRUE(v1[1] == 2);
	}
}

UTEST(Vec, cross_product)
{
	// Correctness test
	{
		sfz::Vec<int, 3> v1{ -1, 4, 0 };
		sfz::Vec<int, 3> v2{ 1, -2, 3 };
		auto res = sfz::cross(v1, v2);

		ASSERT_TRUE(res[0] == 12);
		ASSERT_TRUE(res[1] == 3);
		ASSERT_TRUE(res[2] == -2);
	}
	// 2nd correctness test
	{
		sfz::Vec<int, 3> v1{ -1, 4, 0 };
		sfz::Vec<int, 3> v2{ 1, -2, 3 };
		auto res = sfz::cross(v2, v1);

		ASSERT_TRUE(res[0] == -12);
		ASSERT_TRUE(res[1] == -3);
		ASSERT_TRUE(res[2] == 2);
	}
	// A x A = 0
	{
		sfz::Vec<int, 3> v1{ -1, 4, 0 };
		sfz::Vec<int, 3> v2{ 1, -2, 3 };

		auto res1 = sfz::cross(v1, v1);
		ASSERT_TRUE(res1[0] == 0);
		ASSERT_TRUE(res1[1] == 0);
		ASSERT_TRUE(res1[2] == 0);

		auto res2 = sfz::cross(v2, v2);
		ASSERT_TRUE(res2[0] == 0);
		ASSERT_TRUE(res2[1] == 0);
		ASSERT_TRUE(res2[2] == 0);
	}
}

UTEST(Vec, element_sum)
{
	ASSERT_TRUE(sfz::elemSum(sfz::vec2(1.0f, 2.0f)) == 3.0f);
	ASSERT_TRUE(sfz::elemSum(sfz::vec3(1.0f, 2.0f, 3.0f)) == 6.0f);
	ASSERT_TRUE(sfz::elemSum(sfz::vec4(1.0f, 2.0f, 3.0f, 4.0f)) == 10.0f);

	ASSERT_TRUE(sfz::elemSum(sfz::vec2_i32(1, 2)) == 3);
	ASSERT_TRUE(sfz::elemSum(sfz::vec3_i32(1, 2, 3)) == 6);
	ASSERT_TRUE(sfz::elemSum(sfz::vec4_i32(1, 2, 3, 4)) == 10);

	ASSERT_TRUE(sfz::elemSum(sfz::vec2_i32(0, 0)) == 0);
	ASSERT_TRUE(sfz::elemSum(sfz::vec3_i32(0, 0, 0)) == 0);
	ASSERT_TRUE(sfz::elemSum(sfz::vec4_i32(0, 0, 0, 0)) == 0);

	ASSERT_TRUE(sfz::elemSum(sfz::vec2_i32(-3, 3)) == 0);
	ASSERT_TRUE(sfz::elemSum(sfz::vec3_i32(-2, -1, 3)) == 0);
	ASSERT_TRUE(sfz::elemSum(sfz::vec4_i32(-4, -5, 10, -2)) == -1);
}

UTEST(Vec, element_max)
{
	ASSERT_TRUE(sfz::elemMax(sfz::vec2(1.0f, 2.0f)) == 2.0f);
	ASSERT_TRUE(sfz::elemMax(sfz::vec3(1.0f, 2.0f, 3.0f)) == 3.0f);
	ASSERT_TRUE(sfz::elemMax(sfz::vec4(1.0f, 2.0f, 3.0f, 4.0f)) == 4.0f);

	ASSERT_TRUE(sfz::elemMax(sfz::vec2_i32(1, 2)) == 2);
	ASSERT_TRUE(sfz::elemMax(sfz::vec3_i32(1, 2, 3)) == 3);
	ASSERT_TRUE(sfz::elemMax(sfz::vec4_i32(1, 2, 3, 4)) == 4);

	ASSERT_TRUE(sfz::elemMax(sfz::vec2_i32(0, 0)) == 0);
	ASSERT_TRUE(sfz::elemMax(sfz::vec3_i32(0, 0, 0)) == 0);
	ASSERT_TRUE(sfz::elemMax(sfz::vec4_i32(0, 0, 0, 0)) == 0);

	ASSERT_TRUE(sfz::elemMax(sfz::vec2_i32(-3, 3)) == 3);
	ASSERT_TRUE(sfz::elemMax(sfz::vec3_i32(-2, -1, 3)) == 3);
	ASSERT_TRUE(sfz::elemMax(sfz::vec4_i32(-4, -5, 10, -2)) == 10);
}

UTEST(Vec, element_min)
{
	ASSERT_TRUE(sfz::elemMin(sfz::vec2(1.0f, 2.0f)) == 1.0f);
	ASSERT_TRUE(sfz::elemMin(sfz::vec3(1.0f, 2.0f, 3.0f)) == 1.0f);
	ASSERT_TRUE(sfz::elemMin(sfz::vec4(1.0f, 2.0f, 3.0f, 4.0f)) == 1.0f);

	ASSERT_TRUE(sfz::elemMin(sfz::vec2_i32(1, 2)) == 1);
	ASSERT_TRUE(sfz::elemMin(sfz::vec3_i32(1, 2, 3)) == 1);
	ASSERT_TRUE(sfz::elemMin(sfz::vec4_i32(1, 2, 3, 4)) == 1);

	ASSERT_TRUE(sfz::elemMin(sfz::vec2_i32(0, 0)) == 0);
	ASSERT_TRUE(sfz::elemMin(sfz::vec3_i32(0, 0, 0)) == 0);
	ASSERT_TRUE(sfz::elemMin(sfz::vec4_i32(0, 0, 0, 0)) == 0);

	ASSERT_TRUE(sfz::elemMin(sfz::vec2_i32(-3, 3)) == -3);
	ASSERT_TRUE(sfz::elemMin(sfz::vec3_i32(-2, -1, 3)) == -2);
	ASSERT_TRUE(sfz::elemMin(sfz::vec4_i32(-4, -5, 10, -2)) == -5);
}

UTEST(Vec, is_proper_pod)
{
	ASSERT_TRUE(std::is_trivially_default_constructible<sfz::vec2>::value);
	ASSERT_TRUE(std::is_trivially_default_constructible<sfz::vec2_i32>::value);
	ASSERT_TRUE(std::is_trivially_default_constructible<sfz::vec3>::value);
	ASSERT_TRUE(std::is_trivially_default_constructible<sfz::vec3_i32>::value);

	ASSERT_TRUE(std::is_trivially_copyable<sfz::vec2>::value);
	ASSERT_TRUE(std::is_trivially_copyable<sfz::vec2_i32>::value);
	ASSERT_TRUE(std::is_trivially_copyable<sfz::vec3>::value);
	ASSERT_TRUE(std::is_trivially_copyable<sfz::vec3_i32>::value);

	ASSERT_TRUE(std::is_trivial<sfz::vec2>::value);
	ASSERT_TRUE(std::is_trivial<sfz::vec2_i32>::value);
	ASSERT_TRUE(std::is_trivial<sfz::vec3>::value);
	ASSERT_TRUE(std::is_trivial<sfz::vec3_i32>::value);

	ASSERT_TRUE(std::is_standard_layout<sfz::vec2>::value);
	ASSERT_TRUE(std::is_standard_layout<sfz::vec2_i32>::value);
	ASSERT_TRUE(std::is_standard_layout<sfz::vec3>::value);
	ASSERT_TRUE(std::is_standard_layout<sfz::vec3_i32>::value);

	ASSERT_TRUE(std::is_pod<sfz::vec2>::value);
	ASSERT_TRUE(std::is_pod<sfz::vec2_i32>::value);
	ASSERT_TRUE(std::is_pod<sfz::vec3>::value);
	ASSERT_TRUE(std::is_pod<sfz::vec3_i32>::value);
}

// Math functions
// ------------------------------------------------------------------------------------------------

UTEST(Math, eqf)
{
	// f32
	{
		ASSERT_TRUE(sfz::eqf(2.0f, 2.0f + (sfz::EQF_EPS * 0.95f)));
		ASSERT_TRUE(!sfz::eqf(2.0f, 2.0f + (sfz::EQF_EPS * 1.05f)));
		ASSERT_TRUE(sfz::eqf(2.0f, 2.0f - (sfz::EQF_EPS * 0.95f)));
		ASSERT_TRUE(!sfz::eqf(2.0f, 2.0f - (sfz::EQF_EPS * 1.05f)));
	}
	// vec2
	{
		ASSERT_TRUE(sfz::eqf(sfz::vec2(2.0f), sfz::vec2(2.0f + (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(sfz::vec2(2.0f), sfz::vec2(2.0f + (sfz::EQF_EPS * 1.05f))));
		ASSERT_TRUE(sfz::eqf(sfz::vec2(2.0f), sfz::vec2(2.0f - (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(sfz::vec2(2.0f), sfz::vec2(2.0f - (sfz::EQF_EPS * 1.05f))));
	}
	// vec3
	{
		ASSERT_TRUE(sfz::eqf(sfz::vec3(2.0f), sfz::vec3(2.0f + (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(sfz::vec3(2.0f), sfz::vec3(2.0f + (sfz::EQF_EPS * 1.05f))));
		ASSERT_TRUE(sfz::eqf(sfz::vec3(2.0f), sfz::vec3(2.0f - (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(sfz::vec3(2.0f), sfz::vec3(2.0f - (sfz::EQF_EPS * 1.05f))));
	}
	// vec4
	{
		ASSERT_TRUE(sfz::eqf(sfz::vec4(2.0f), sfz::vec4(2.0f + (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(sfz::vec4(2.0f), sfz::vec4(2.0f + (sfz::EQF_EPS * 1.05f))));
		ASSERT_TRUE(sfz::eqf(sfz::vec4(2.0f), sfz::vec4(2.0f - (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(sfz::vec4(2.0f), sfz::vec4(2.0f - (sfz::EQF_EPS * 1.05f))));
	}
}

UTEST(Math, abs)
{
	ASSERT_TRUE(sfz::abs(-2.0f) == 2.0f);
	ASSERT_TRUE(sfz::abs(3.0f) == 3.0f);
	ASSERT_TRUE(sfz::abs(sfz::vec2(-1.0f, 2.0f)) == sfz::vec2(1.0, 2.0));
	ASSERT_TRUE(sfz::abs(sfz::vec3(2.0f, -4.0f, -6.0f)) == sfz::vec3(2.0f, 4.0f, 6.0f));
	ASSERT_TRUE(sfz::abs(sfz::vec4(-4.0f, 2.0f, -4.0f, -1.0f)) == sfz::vec4(4.0f, 2.0f, 4.0f, 1.0f));

	ASSERT_TRUE(sfz::abs(-2) == 2);
	ASSERT_TRUE(sfz::abs(3) == 3);
	ASSERT_TRUE(sfz::abs(sfz::vec2_i32(-1, 2)) == sfz::vec2_i32(1, 2));
	ASSERT_TRUE(sfz::abs(sfz::vec3_i32(2, -4, -6)) == sfz::vec3_i32(2, 4, 6));
	ASSERT_TRUE(sfz::abs(sfz::vec4_i32(-4, 2, -4, -1)) == sfz::vec4_i32(4, 2, 4, 1));
}

UTEST(Math, min_float)
{
	ASSERT_TRUE(sfz::min(0.0f, 0.0f) == 0.0f);

	ASSERT_TRUE(sfz::min(-1.0f, 0.0f) == -1.0f);
	ASSERT_TRUE(sfz::min(0.0f, -1.0f) == -1.0f);

	ASSERT_TRUE(sfz::min(-1.0f, -2.0f) == -2.0f);
	ASSERT_TRUE(sfz::min(-2.0f, -1.0f) == -2.0f);

	ASSERT_TRUE(sfz::min(1.0f, 0.0f) == 0.0f);
	ASSERT_TRUE(sfz::min(0.0f, 1.0f) == 0.0f);

	ASSERT_TRUE(sfz::min(1.0f, 2.0f) == 1.0f);
	ASSERT_TRUE(sfz::min(2.0f, 1.0f) == 1.0f);
}

UTEST(Math, max_float)
{
	ASSERT_TRUE(sfz::max(0.0f, 0.0f) == 0.0f);

	ASSERT_TRUE(sfz::max(-1.0f, 0.0f) == 0.0f);
	ASSERT_TRUE(sfz::max(0.0f, -1.0f) == 0.0f);

	ASSERT_TRUE(sfz::max(-1.0f, -2.0f) == -1.0f);
	ASSERT_TRUE(sfz::max(-2.0f, -1.0f) == -1.0f);

	ASSERT_TRUE(sfz::max(1.0f, 0.0f) == 1.0f);
	ASSERT_TRUE(sfz::max(0.0f, 1.0f) == 1.0f);

	ASSERT_TRUE(sfz::max(1.0f, 2.0f) == 2.0f);
	ASSERT_TRUE(sfz::max(2.0f, 1.0f) == 2.0f);
}

UTEST(Math, min_int32)
{
	ASSERT_TRUE(sfz::min(0, 0) == 0);

	ASSERT_TRUE(sfz::min(-1, 0) == -1);
	ASSERT_TRUE(sfz::min(0, -1) == -1);

	ASSERT_TRUE(sfz::min(-1, -2) == -2);
	ASSERT_TRUE(sfz::min(-2, -1) == -2);

	ASSERT_TRUE(sfz::min(1, 0) == 0);
	ASSERT_TRUE(sfz::min(0, 1) == 0);

	ASSERT_TRUE(sfz::min(1, 2) == 1);
	ASSERT_TRUE(sfz::min(2, 1) == 1);
}

UTEST(Math, max_int32)
{
	ASSERT_TRUE(sfz::max(0, 0) == 0);

	ASSERT_TRUE(sfz::max(-1, 0) == 0);
	ASSERT_TRUE(sfz::max(0, -1) == 0);

	ASSERT_TRUE(sfz::max(-1, -2) == -1);
	ASSERT_TRUE(sfz::max(-2, -1) == -1);

	ASSERT_TRUE(sfz::max(1, 0) == 1);
	ASSERT_TRUE(sfz::max(0, 1) == 1);

	ASSERT_TRUE(sfz::max(1, 2) == 2);
	ASSERT_TRUE(sfz::max(2, 1) == 2);
}

UTEST(Math, min_uint32)
{
	ASSERT_TRUE(sfz::min(0u, 0u) == 0u);

	ASSERT_TRUE(sfz::min(1u, 0u) == 0u);
	ASSERT_TRUE(sfz::min(0u, 1u) == 0u);

	ASSERT_TRUE(sfz::min(1u, 2u) == 1u);
	ASSERT_TRUE(sfz::min(2u, 1u) == 1u);
}

UTEST(Math, max_uint32)
{
	ASSERT_TRUE(sfz::max(0u, 0u) == 0u);

	ASSERT_TRUE(sfz::max(1u, 0u) == 1u);
	ASSERT_TRUE(sfz::max(0u, 1u) == 1u);

	ASSERT_TRUE(sfz::max(1u, 2u) == 2u);
	ASSERT_TRUE(sfz::max(2u, 1u) == 2u);
}

UTEST(Math, min_vec)
{
	ASSERT_TRUE(sfz::min(sfz::vec4(1.0f, 2.0f, -3.0f, -4.0f), sfz::vec4(2.0f, 1.0f, -5.0f, -2.0f)) == sfz::vec4(1.0f, 1.0f, -5.0f, -4.0f));
	ASSERT_TRUE(sfz::min(sfz::vec4_i32(1, 2, -3, -4), sfz::vec4_i32(2, 1, -5, -2)) == sfz::vec4_i32(1, 1, -5, -4));
	ASSERT_TRUE(sfz::min(sfz::vec4_u32(1u, 2u, 3u, 4u), sfz::vec4_u32(2u, 1u, 5u, 2u)) == sfz::vec4_u32(1u, 1u, 3u, 2u));

	ASSERT_TRUE(sfz::min(sfz::vec4(1.0f, 2.0f, -3.0f, -4.0f), -1.0f) == sfz::vec4(-1.0f, -1.0f, -3.0f, -4.0f));
	ASSERT_TRUE(sfz::min(sfz::vec4_i32(1, 2, -3, -4), -1) == sfz::vec4_i32(-1, -1, -3, -4));
	ASSERT_TRUE(sfz::min(sfz::vec4_u32(1u, 2u, 3u, 4u), 2u) == sfz::vec4_u32(1u, 2u, 2u, 2u));
}

UTEST(Math, max_vec)
{
	ASSERT_TRUE(sfz::max(sfz::vec4(1.0f, 2.0f, -3.0f, -4.0f), sfz::vec4(2.0f, 1.0f, -5.0f, -2.0f)) == sfz::vec4(2.0f, 2.0f, -3.0f, -2.0f));
	ASSERT_TRUE(sfz::max(sfz::vec4_i32(1, 2, -3, -4), sfz::vec4_i32(2, 1, -5, -2)) == sfz::vec4_i32(2, 2, -3, -2));
	ASSERT_TRUE(sfz::max(sfz::vec4_u32(1u, 2u, 3u, 4u), sfz::vec4_u32(2u, 1u, 5u, 2u)) == sfz::vec4_u32(2u, 2u, 5u, 4u));

	ASSERT_TRUE(sfz::max(sfz::vec4(1.0f, 2.0f, -3.0f, -4.0f), 1.0f) == sfz::vec4(1.0f, 2.0f, 1.0f, 1.0f));
	ASSERT_TRUE(sfz::max(sfz::vec4_i32(1, 2, -3, -4), 1) == sfz::vec4_i32(1, 2, 1, 1));
	ASSERT_TRUE(sfz::max(sfz::vec4_u32(1u, 2u, 3u, 4u), 2u) == sfz::vec4_u32(2u, 2u, 3u, 4u));
}

UTEST(Math, clamp)
{
	ASSERT_TRUE(sfz::clamp(sfz::vec4_i32(-2, 0, 2, 4), -1, 2) == sfz::vec4_i32(-1, 0, 2, 2));
	ASSERT_TRUE(sfz::clamp(sfz::vec4_i32(-2, 0, 2, 4), sfz::vec4_i32(0, -1, -1, 5), sfz::vec4_i32(1, 1, 1, 6)) == sfz::vec4_i32(0, 0, 1, 5));
}

UTEST(Math, sgn)
{
	{
		ASSERT_TRUE(sfz::sgn(0.0f) == 1.0f);
		ASSERT_TRUE(sfz::sgn(-4.0f) == -1.0f);
		ASSERT_TRUE(sfz::sgn(0) == 1);
		ASSERT_TRUE(sfz::sgn(-4) == -1);
	}

	{
		ASSERT_TRUE(sfz::sgn(sfz::vec2(5.0f, -5.0f)) == sfz::vec2(1.0f, -1.0f));
		ASSERT_TRUE(sfz::sgn(sfz::vec2(-5.0f, 5.0)) == sfz::vec2(-1.0f, 1.0f));
		ASSERT_TRUE(sfz::sgn(sfz::vec2_i32(6, -2)) == sfz::vec2_i32(1, -1));
		ASSERT_TRUE(sfz::sgn(sfz::vec2_i32(-7, 1)) == sfz::vec2_i32(-1, 1));
	}

	{
		ASSERT_TRUE(sfz::sgn(sfz::vec3(5.0f, -5.0f, -2.0f)) == sfz::vec3(1.0f, -1.0f, -1.0f));
		ASSERT_TRUE(sfz::sgn(sfz::vec3(-5.0f, 5.0, 29.0f)) == sfz::vec3(-1.0f, 1.0f, 1.0f));
		ASSERT_TRUE(sfz::sgn(sfz::vec3_i32(6, -2, 2)) == sfz::vec3_i32(1, -1, 1));
		ASSERT_TRUE(sfz::sgn(sfz::vec3_i32(-7, 1, 2)) == sfz::vec3_i32(-1, 1, 1));
	}

	{
		ASSERT_TRUE(sfz::sgn(sfz::vec4(5.0f, -5.0f, -2.0f, 3.0f)) == sfz::vec4(1.0f, -1.0f, -1.0f, 1.0f));
		ASSERT_TRUE(sfz::sgn(sfz::vec4(-5.0f, 5.0, 29.0f, -9.0f)) == sfz::vec4(-1.0f, 1.0f, 1.0f, -1.0f));
		ASSERT_TRUE(sfz::sgn(sfz::vec4_i32(6, -2, 2, -7)) == sfz::vec4_i32(1, -1, 1, -1));
		ASSERT_TRUE(sfz::sgn(sfz::vec4_i32(-7, 1, 2, -4)) == sfz::vec4_i32(-1, 1, 1, -1));
	}

}

UTEST(Math, saturate)
{
	ASSERT_TRUE(sfz::saturate(4.0f) == 1.0f);
	ASSERT_TRUE(sfz::saturate(-1.0f) == 0.0f);
	ASSERT_TRUE(sfz::saturate(0.2f) == 0.2f);
	ASSERT_TRUE(sfz::saturate(sfz::vec4(4.0f, -1.0f, 0.2f, 0.4f)) == sfz::vec4(1.0f, 0.0f, 0.2f, 0.4f));
}

// Memory functions
// ------------------------------------------------------------------------------------------------

UTEST(Memory, memswp)
{
	{
		constexpr char STR1[] = "HELLO WORLD";
		constexpr char STR2[] = "FOO_BAR_AND_SUCH";
		char buffer1[256] = {};
		char buffer2[256] = {};
		memcpy(buffer1, STR1, sizeof(STR1));
		memcpy(buffer2, STR2, sizeof(STR2));
		ASSERT_TRUE(strncmp(buffer1, STR1, 256) == 0);
		ASSERT_TRUE(strncmp(buffer2, STR2, 256) == 0);
		sfz::memswp(buffer1, buffer2, sfz::max(u32(sizeof(STR1)), u32(sizeof(STR2))));
		ASSERT_TRUE(strncmp(buffer2, STR1, 256) == 0);
		ASSERT_TRUE(strncmp(buffer1, STR2, 256) == 0);
		sfz::memswp(buffer1, buffer2, 256);
		ASSERT_TRUE(strncmp(buffer1, STR1, 256) == 0);
		ASSERT_TRUE(strncmp(buffer2, STR2, 256) == 0);
	}
	
	{
		constexpr u32 NUM_ELEMS = 217;
		u32 buffer1[NUM_ELEMS];
		for (u32 i = 0; i < NUM_ELEMS; i++) {
			buffer1[i] = i;
		}
		u32 buffer2[NUM_ELEMS + 10] = {};
		for (u32 i = 0; i < NUM_ELEMS; i++) {
			buffer2[i + 5] = i * i;
		}
		sfz::memswp(buffer1, buffer2 + 5, sizeof(buffer1));
		for (u32 i = 0; i < NUM_ELEMS; i++) {
			ASSERT_TRUE(buffer1[i] == (i * i));
			ASSERT_TRUE(buffer2[i + 5] == i);
		}
		ASSERT_TRUE(buffer2[0] == 0);
		ASSERT_TRUE(buffer2[1] == 0);
		ASSERT_TRUE(buffer2[2] == 0);
		ASSERT_TRUE(buffer2[3] == 0);
		ASSERT_TRUE(buffer2[4] == 0);
		ASSERT_TRUE(buffer2[NUM_ELEMS + 5 + 0] == 0);
		ASSERT_TRUE(buffer2[NUM_ELEMS + 5 + 1] == 0);
		ASSERT_TRUE(buffer2[NUM_ELEMS + 5 + 2] == 0);
		ASSERT_TRUE(buffer2[NUM_ELEMS + 5 + 3] == 0);
		ASSERT_TRUE(buffer2[NUM_ELEMS + 5 + 4] == 0);
	}
}
