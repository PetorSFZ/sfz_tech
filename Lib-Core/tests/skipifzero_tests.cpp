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
#include <skipifzero_math.hpp>

#include <type_traits>

// Vector tests
// ------------------------------------------------------------------------------------------------

UTEST(Vec, vec2_specialization)
{
	// Data
	{
		i32x2 v;
		ASSERT_TRUE(sizeof(i32x2) == sizeof(int) * 2);
		v.data()[0] = 1;
		v.data()[1] = 2;
		ASSERT_TRUE(v.x == 1);
		ASSERT_TRUE(v.y == 2);
	}
	// Array pointer constructor
	{
		int arr[] = { 1, 2, 3 };
		i32x2 v1{ arr };
		i32x2 v2{ arr + 1 };
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 2);
		ASSERT_TRUE(v2[0] == 2);
		ASSERT_TRUE(v2[1] == 3);
	}
	// Fill constructor
	{
		i32x2 v1{ 3 };
		ASSERT_TRUE(v1.x == 3);
		ASSERT_TRUE(v1.y == 3);
	}
	// Constructor (x, y)
	{
		i32x2 v1{ 3, -1 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
	}
	// Cast constructor
	{
		i32x2 v1(f32x2(-1.0f, 1.0f));
		ASSERT_TRUE(v1.x == -1);
		ASSERT_TRUE(v1.y == 1);
	}
	// Access [] operator
	{
		i32x2 v;
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
		i32x3 v;
		ASSERT_TRUE(sizeof(i32x3) == sizeof(i32) * 3);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		ASSERT_TRUE(v.x == 1);
		ASSERT_TRUE(v.y == 2);
		ASSERT_TRUE(v.z == 3);
		ASSERT_TRUE(v.xy() == i32x2(1, 2));
		ASSERT_TRUE(v.yz() == i32x2(2, 3));
	}
	// Array pointer constructor
	{
		int arr[] = { 1, 2, 3, 4 };
		i32x3 v1{ arr };
		i32x3 v2{ arr + 1 };
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 2);
		ASSERT_TRUE(v1[2] == 3);
		ASSERT_TRUE(v2[0] == 2);
		ASSERT_TRUE(v2[1] == 3);
		ASSERT_TRUE(v2[2] == 4);
	}
	// Fill constructor
	{
		i32x3 v1{ 3 };
		ASSERT_TRUE(v1.x == 3);
		ASSERT_TRUE(v1.y == 3);
		ASSERT_TRUE(v1.z == 3);
	}
	// Constructor (x, y, z)
	{
		i32x3 v1{ 3, -1, -2 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
	}
	// Constructor (xy, z)
	{
		i32x3 v1{ i32x2{3, -1}, -2 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
	}
	// Constructor (x, yz)
	{
		i32x3 v1{ 3, i32x2{-1, -2} };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
	}
	// Cast constructor
	{
		i32x3 v1(f32x3(-1.0f, 1.0f, -2.0f));
		ASSERT_TRUE(v1.x == -1);
		ASSERT_TRUE(v1.y == 1);
		ASSERT_TRUE(v1.z == -2);
	}
	// Access operator []
	{
		i32x3 v;
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
		i32x4 v;
		ASSERT_TRUE(sizeof(i32x4) == sizeof(int) * 4);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		v.data()[3] = 4;
		ASSERT_TRUE(v.x == 1);
		ASSERT_TRUE(v.y == 2);
		ASSERT_TRUE(v.z == 3);
		ASSERT_TRUE(v.w == 4);
		ASSERT_TRUE(v.xyz() == (i32x3{1, 2, 3}));
		ASSERT_TRUE(v.yzw() == (i32x3{2, 3, 4}));
		ASSERT_TRUE(v.xy() == (i32x2{1, 2}));
		ASSERT_TRUE(v.zw() == (i32x2{3, 4}));
		ASSERT_TRUE(v.yz() == (i32x2{2, 3}));
	}
	// Array pointer constructor
	{
		int arr[] = { 1, 2, 3, 4, 5 };
		i32x4 v1{ arr };
		i32x4 v2{ arr + 1 };
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
		i32x4 v1{ 3 };
		ASSERT_TRUE(v1.x == 3);
		ASSERT_TRUE(v1.y == 3);
		ASSERT_TRUE(v1.z == 3);
		ASSERT_TRUE(v1.w == 3);
	}
	// Constructor (x, y, z, w)
	{
		i32x4 v1{ 3, -1, -2, 9 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (xyz, w)
	{
		i32x4 v1{ i32x3{3, -1, -2}, 9 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (x, yzw)
	{
		i32x4 v1{ 3, i32x3{-1, -2, 9} };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (xy, zw)
	{
		i32x4 v1{ i32x2{3, -1}, i32x2{-2, 9} };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (xy, z, w)
	{
		i32x4 v1{ i32x2{3, -1}, -2, 9 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (x, yz, w)
	{
		i32x4 v1{ 3, i32x2{-1, -2}, 9 };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Constructor (x, y, zw)
	{
		i32x4 v1{ 3, -1, i32x2{-2, 9} };
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -1);
		ASSERT_TRUE(v1[2] == -2);
		ASSERT_TRUE(v1[3] == 9);
	}
	// Cast constructor
	{
		i32x4 v1(f32x4(-1.0f, 1.0f, -2.0f, 4.0f));
		ASSERT_TRUE(v1.x == -1);
		ASSERT_TRUE(v1.y == 1);
		ASSERT_TRUE(v1.z == -2);
		ASSERT_TRUE(v1.w == 4);
	}
	// Access [] operator
	{
		i32x4 v;
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
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

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
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

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
		i32x3 v1{ 1, -2, 5 };

		i32x3 v3 = -v1;
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
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		i32x3 v3 = v1 * 3;
		ASSERT_TRUE(v3[0] == 3);
		ASSERT_TRUE(v3[1] == -6);
		ASSERT_TRUE(v3[2] == 15);
		i32x3 v4 = -3 * v2;
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
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		i32x3 v3 = v1 * v2;
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
		i32x2 v3 = i32x2{ 2, -2 } / 2;
		ASSERT_TRUE(v3[0] == 1);
		ASSERT_TRUE(v3[1] == -1);
		i32x2 v4 = -8 / i32x2(2, 4);
		ASSERT_TRUE(v4.x == -4);
		ASSERT_TRUE(v4.y == -2);
	}
	// Element-wise divison
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		auto v3 = v1 / v1;
		ASSERT_TRUE(v3[0] == 1);
		ASSERT_TRUE(v3[1] == 1);
		ASSERT_TRUE(v3[2] == 1);
	}
	// Addition assignment
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		v1 += v2;
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == -4);
		ASSERT_TRUE(v1[2] == 6);
	}
	// Subtraction assignment
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		v1 -= v2;
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 0);
		ASSERT_TRUE(v1[2] == 4);
	}
	// Multiplication by number assignment
	{
		i32x3 v1{ 1, -2, 5 };

		v1 *= 3;
		ASSERT_TRUE(v1[0] == 3);
		ASSERT_TRUE(v1[1] == -6);
		ASSERT_TRUE(v1[2] == 15);
	}
	// Element-wise multiplication assignment
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		v1 *= v2;
		ASSERT_TRUE(v1[0] == 0);
		ASSERT_TRUE(v1[1] == 4);
		ASSERT_TRUE(v1[2] == 5);
	}
	// Division by number assignment
	{
		i32x2 v3{ 2, -2 };
		v3 /= 2;
		ASSERT_TRUE(v3[0] == 1);
		ASSERT_TRUE(v3[1] == -1);
	}
	// Element-wise division assignment
	{
		i32x3 v1{ 1, -2, 5 };

		i32x3 v1Copy = v1; // Not necessary, just to remove warning from Clang.
		v1 /= v1Copy;
		ASSERT_TRUE(v1[0] == 1);
		ASSERT_TRUE(v1[1] == 1);
		ASSERT_TRUE(v1[2] == 1);
	}
}

UTEST(Vec, length_of_vectors)
{
	f32x2 v1(2.0f, 0.0f);
	f32 v2Arr[] = { -2.0f, 2.0f, 2.0f, -2.0f };
	f32x4 v2(v2Arr);

	ASSERT_TRUE(sfz::eqf(sfz::length(v1), 2.0f));
	ASSERT_TRUE(sfz::eqf(sfz::length(v2), 4.0f));
}

UTEST(Vec, normalizing_vector)
{
	f32x4 v1 = sfz::normalize(f32x4(-2.f, 2.f, -2.f, 2.f));
	ASSERT_TRUE(sfz::eqf(v1, f32x4(-0.5f, 0.5f, -0.5f, 0.5f)));
	ASSERT_TRUE(sfz::normalizeSafe(f32x3(0.0f)) == f32x3(0.0f));
}

UTEST(Vec, comparison_operators)
{
	i32x3 v1{ -4, 0, 0 };
	i32x3 v2{ 0, 2, 0 };
	i32x3 v3{ 0, 2, 0 };

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
		i32x3 v1{ 1, 0, -2 };
		i32x3 v2{ 6, 2, 2 };
		i32 scalarProduct = sfz::dot(v1, v2);

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
		i32x2 v1{ -3, 2 };
		i32 scalarProduct = sfz::dot(v1, v1);

		ASSERT_TRUE(scalarProduct == 13);

		ASSERT_TRUE(v1[0] == -3);
		ASSERT_TRUE(v1[1] == 2);
	}
}

UTEST(Vec, cross_product)
{
	// Correctness test
	{
		i32x3 v1{ -1, 4, 0 };
		i32x3 v2{ 1, -2, 3 };
		i32x3 res = sfz::cross(v1, v2);

		ASSERT_TRUE(res[0] == 12);
		ASSERT_TRUE(res[1] == 3);
		ASSERT_TRUE(res[2] == -2);
	}
	// 2nd correctness test
	{
		i32x3 v1{ -1, 4, 0 };
		i32x3 v2{ 1, -2, 3 };
		i32x3 res = sfz::cross(v2, v1);

		ASSERT_TRUE(res[0] == -12);
		ASSERT_TRUE(res[1] == -3);
		ASSERT_TRUE(res[2] == 2);
	}
	// A x A = 0
	{
		i32x3 v1{ -1, 4, 0 };
		i32x3 v2{ 1, -2, 3 };

		i32x3 res1 = sfz::cross(v1, v1);
		ASSERT_TRUE(res1[0] == 0);
		ASSERT_TRUE(res1[1] == 0);
		ASSERT_TRUE(res1[2] == 0);

		i32x3 res2 = sfz::cross(v2, v2);
		ASSERT_TRUE(res2[0] == 0);
		ASSERT_TRUE(res2[1] == 0);
		ASSERT_TRUE(res2[2] == 0);
	}
}

UTEST(Vec, element_sum)
{
	ASSERT_TRUE(sfz::elemSum(f32x2(1.0f, 2.0f)) == 3.0f);
	ASSERT_TRUE(sfz::elemSum(f32x3(1.0f, 2.0f, 3.0f)) == 6.0f);
	ASSERT_TRUE(sfz::elemSum(f32x4(1.0f, 2.0f, 3.0f, 4.0f)) == 10.0f);

	ASSERT_TRUE(sfz::elemSum(i32x2(1, 2)) == 3);
	ASSERT_TRUE(sfz::elemSum(i32x3(1, 2, 3)) == 6);
	ASSERT_TRUE(sfz::elemSum(i32x4(1, 2, 3, 4)) == 10);

	ASSERT_TRUE(sfz::elemSum(i32x2(0, 0)) == 0);
	ASSERT_TRUE(sfz::elemSum(i32x3(0, 0, 0)) == 0);
	ASSERT_TRUE(sfz::elemSum(i32x4(0, 0, 0, 0)) == 0);

	ASSERT_TRUE(sfz::elemSum(i32x2(-3, 3)) == 0);
	ASSERT_TRUE(sfz::elemSum(i32x3(-2, -1, 3)) == 0);
	ASSERT_TRUE(sfz::elemSum(i32x4(-4, -5, 10, -2)) == -1);
}

UTEST(Vec, element_max)
{
	ASSERT_TRUE(sfz::elemMax(f32x2(1.0f, 2.0f)) == 2.0f);
	ASSERT_TRUE(sfz::elemMax(f32x3(1.0f, 2.0f, 3.0f)) == 3.0f);
	ASSERT_TRUE(sfz::elemMax(f32x4(1.0f, 2.0f, 3.0f, 4.0f)) == 4.0f);

	ASSERT_TRUE(sfz::elemMax(i32x2(1, 2)) == 2);
	ASSERT_TRUE(sfz::elemMax(i32x3(1, 2, 3)) == 3);
	ASSERT_TRUE(sfz::elemMax(i32x4(1, 2, 3, 4)) == 4);

	ASSERT_TRUE(sfz::elemMax(i32x2(0, 0)) == 0);
	ASSERT_TRUE(sfz::elemMax(i32x3(0, 0, 0)) == 0);
	ASSERT_TRUE(sfz::elemMax(i32x4(0, 0, 0, 0)) == 0);

	ASSERT_TRUE(sfz::elemMax(i32x2(-3, 3)) == 3);
	ASSERT_TRUE(sfz::elemMax(i32x3(-2, -1, 3)) == 3);
	ASSERT_TRUE(sfz::elemMax(i32x4(-4, -5, 10, -2)) == 10);
}

UTEST(Vec, element_min)
{
	ASSERT_TRUE(sfz::elemMin(f32x2(1.0f, 2.0f)) == 1.0f);
	ASSERT_TRUE(sfz::elemMin(f32x3(1.0f, 2.0f, 3.0f)) == 1.0f);
	ASSERT_TRUE(sfz::elemMin(f32x4(1.0f, 2.0f, 3.0f, 4.0f)) == 1.0f);

	ASSERT_TRUE(sfz::elemMin(i32x2(1, 2)) == 1);
	ASSERT_TRUE(sfz::elemMin(i32x3(1, 2, 3)) == 1);
	ASSERT_TRUE(sfz::elemMin(i32x4(1, 2, 3, 4)) == 1);

	ASSERT_TRUE(sfz::elemMin(i32x2(0, 0)) == 0);
	ASSERT_TRUE(sfz::elemMin(i32x3(0, 0, 0)) == 0);
	ASSERT_TRUE(sfz::elemMin(i32x4(0, 0, 0, 0)) == 0);

	ASSERT_TRUE(sfz::elemMin(i32x2(-3, 3)) == -3);
	ASSERT_TRUE(sfz::elemMin(i32x3(-2, -1, 3)) == -2);
	ASSERT_TRUE(sfz::elemMin(i32x4(-4, -5, 10, -2)) == -5);
}

UTEST(Vec, is_proper_pod)
{
	ASSERT_TRUE(std::is_trivially_default_constructible<f32x2>::value);
	ASSERT_TRUE(std::is_trivially_default_constructible<i32x2>::value);
	ASSERT_TRUE(std::is_trivially_default_constructible<f32x3>::value);
	ASSERT_TRUE(std::is_trivially_default_constructible<i32x3>::value);

	ASSERT_TRUE(std::is_trivially_copyable<f32x2>::value);
	ASSERT_TRUE(std::is_trivially_copyable<i32x2>::value);
	ASSERT_TRUE(std::is_trivially_copyable<f32x3>::value);
	ASSERT_TRUE(std::is_trivially_copyable<i32x3>::value);

	ASSERT_TRUE(std::is_trivial<f32x2>::value);
	ASSERT_TRUE(std::is_trivial<i32x2>::value);
	ASSERT_TRUE(std::is_trivial<f32x3>::value);
	ASSERT_TRUE(std::is_trivial<i32x3>::value);

	ASSERT_TRUE(std::is_standard_layout<f32x2>::value);
	ASSERT_TRUE(std::is_standard_layout<i32x2>::value);
	ASSERT_TRUE(std::is_standard_layout<f32x3>::value);
	ASSERT_TRUE(std::is_standard_layout<i32x3>::value);

	ASSERT_TRUE(std::is_pod<f32x2>::value);
	ASSERT_TRUE(std::is_pod<i32x2>::value);
	ASSERT_TRUE(std::is_pod<f32x3>::value);
	ASSERT_TRUE(std::is_pod<i32x3>::value);
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
	// f32x2
	{
		ASSERT_TRUE(sfz::eqf(f32x2(2.0f), f32x2(2.0f + (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(f32x2(2.0f), f32x2(2.0f + (sfz::EQF_EPS * 1.05f))));
		ASSERT_TRUE(sfz::eqf(f32x2(2.0f), f32x2(2.0f - (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(f32x2(2.0f), f32x2(2.0f - (sfz::EQF_EPS * 1.05f))));
	}
	// f32x3
	{
		ASSERT_TRUE(sfz::eqf(f32x3(2.0f), f32x3(2.0f + (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(f32x3(2.0f), f32x3(2.0f + (sfz::EQF_EPS * 1.05f))));
		ASSERT_TRUE(sfz::eqf(f32x3(2.0f), f32x3(2.0f - (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(f32x3(2.0f), f32x3(2.0f - (sfz::EQF_EPS * 1.05f))));
	}
	// f32x4
	{
		ASSERT_TRUE(sfz::eqf(f32x4(2.0f), f32x4(2.0f + (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(f32x4(2.0f), f32x4(2.0f + (sfz::EQF_EPS * 1.05f))));
		ASSERT_TRUE(sfz::eqf(f32x4(2.0f), f32x4(2.0f - (sfz::EQF_EPS * 0.95f))));
		ASSERT_TRUE(!sfz::eqf(f32x4(2.0f), f32x4(2.0f - (sfz::EQF_EPS * 1.05f))));
	}
}

UTEST(Math, abs)
{
	ASSERT_TRUE(sfz::abs(-2.0f) == 2.0f);
	ASSERT_TRUE(sfz::abs(3.0f) == 3.0f);
	ASSERT_TRUE(sfz::abs(f32x2(-1.0f, 2.0f)) == f32x2(1.0, 2.0));
	ASSERT_TRUE(sfz::abs(f32x3(2.0f, -4.0f, -6.0f)) == f32x3(2.0f, 4.0f, 6.0f));
	ASSERT_TRUE(sfz::abs(f32x4(-4.0f, 2.0f, -4.0f, -1.0f)) == f32x4(4.0f, 2.0f, 4.0f, 1.0f));

	ASSERT_TRUE(sfz::abs(-2) == 2);
	ASSERT_TRUE(sfz::abs(3) == 3);
	ASSERT_TRUE(sfz::abs(i32x2(-1, 2)) == i32x2(1, 2));
	ASSERT_TRUE(sfz::abs(i32x3(2, -4, -6)) == i32x3(2, 4, 6));
	ASSERT_TRUE(sfz::abs(i32x4(-4, 2, -4, -1)) == i32x4(4, 2, 4, 1));
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
	ASSERT_TRUE(sfz::min(f32x4(1.0f, 2.0f, -3.0f, -4.0f), f32x4(2.0f, 1.0f, -5.0f, -2.0f)) == f32x4(1.0f, 1.0f, -5.0f, -4.0f));
	ASSERT_TRUE(sfz::min(i32x4(1, 2, -3, -4), i32x4(2, 1, -5, -2)) == i32x4(1, 1, -5, -4));

	ASSERT_TRUE(sfz::min(f32x4(1.0f, 2.0f, -3.0f, -4.0f), f32x4(-1.0f)) == f32x4(-1.0f, -1.0f, -3.0f, -4.0f));
	ASSERT_TRUE(sfz::min(i32x4(1, 2, -3, -4), i32x4(-1)) == i32x4(-1, -1, -3, -4));
}

UTEST(Math, max_vec)
{
	ASSERT_TRUE(sfz::max(f32x4(1.0f, 2.0f, -3.0f, -4.0f), f32x4(2.0f, 1.0f, -5.0f, -2.0f)) == f32x4(2.0f, 2.0f, -3.0f, -2.0f));
	ASSERT_TRUE(sfz::max(i32x4(1, 2, -3, -4), i32x4(2, 1, -5, -2)) == i32x4(2, 2, -3, -2));

	ASSERT_TRUE(sfz::max(f32x4(1.0f, 2.0f, -3.0f, -4.0f), f32x4(1.0f)) == f32x4(1.0f, 2.0f, 1.0f, 1.0f));
	ASSERT_TRUE(sfz::max(i32x4(1, 2, -3, -4), i32x4(1)) == i32x4(1, 2, 1, 1));
}

UTEST(Math, clamp)
{
	ASSERT_TRUE(sfz::clamp(i32x4(-2, 0, 2, 4), -1, 2) == i32x4(-1, 0, 2, 2));
	ASSERT_TRUE(sfz::clamp(i32x4(-2, 0, 2, 4), i32x4(0, -1, -1, 5), i32x4(1, 1, 1, 6)) == i32x4(0, 0, 1, 5));
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
		ASSERT_TRUE(sfz::sgn(f32x2(5.0f, -5.0f)) == f32x2(1.0f, -1.0f));
		ASSERT_TRUE(sfz::sgn(f32x2(-5.0f, 5.0)) == f32x2(-1.0f, 1.0f));
		ASSERT_TRUE(sfz::sgn(i32x2(6, -2)) == i32x2(1, -1));
		ASSERT_TRUE(sfz::sgn(i32x2(-7, 1)) == i32x2(-1, 1));
	}

	{
		ASSERT_TRUE(sfz::sgn(f32x3(5.0f, -5.0f, -2.0f)) == f32x3(1.0f, -1.0f, -1.0f));
		ASSERT_TRUE(sfz::sgn(f32x3(-5.0f, 5.0, 29.0f)) == f32x3(-1.0f, 1.0f, 1.0f));
		ASSERT_TRUE(sfz::sgn(i32x3(6, -2, 2)) == i32x3(1, -1, 1));
		ASSERT_TRUE(sfz::sgn(i32x3(-7, 1, 2)) == i32x3(-1, 1, 1));
	}

	{
		ASSERT_TRUE(sfz::sgn(f32x4(5.0f, -5.0f, -2.0f, 3.0f)) == f32x4(1.0f, -1.0f, -1.0f, 1.0f));
		ASSERT_TRUE(sfz::sgn(f32x4(-5.0f, 5.0, 29.0f, -9.0f)) == f32x4(-1.0f, 1.0f, 1.0f, -1.0f));
		ASSERT_TRUE(sfz::sgn(i32x4(6, -2, 2, -7)) == i32x4(1, -1, 1, -1));
		ASSERT_TRUE(sfz::sgn(i32x4(-7, 1, 2, -4)) == i32x4(-1, 1, 1, -1));
	}

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
