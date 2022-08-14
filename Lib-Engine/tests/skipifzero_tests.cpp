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

#include <string.h>

#include "sfz.h"
#include "sfz_cpp.hpp"
#include "sfz_math.h"

// Vector tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("Vec: vec2_specialization")
{
	// Data
	{
		i32x2 v;
		CHECK(sizeof(i32x2) == sizeof(int) * 2);
		v.data()[0] = 1;
		v.data()[1] = 2;
		CHECK(v.x == 1);
		CHECK(v.y == 2);
	}
	// Fill constructor
	{
		i32x2 v1 = i32x2_splat(3);
		CHECK(v1.x == 3);
		CHECK(v1.y == 3);
	}
	// Constructor (x, y)
	{
		i32x2 v1{ 3, -1 };
		CHECK(v1[0] == 3);
		CHECK(v1[1] == -1);
	}
	// Cast constructor
	{
		i32x2 v1 = i32x2_from_f32(f32x2_init(-1.0f, 1.0f));
		CHECK(v1.x == -1);
		CHECK(v1.y == 1);
	}
	// Access [] operator
	{
		i32x2 v;
		v[0] = 4;
		v[1] = -2;
		CHECK(v[0] == 4);
		CHECK(v[1] == -2);
	}
}

TEST_CASE("Vec: vec3_specialization")
{
	// Data
	{
		i32x3 v;
		CHECK(sizeof(i32x3) == sizeof(i32) * 3);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		CHECK(v.x == 1);
		CHECK(v.y == 2);
		CHECK(v.z == 3);
		CHECK(v.xy() == i32x2_init(1, 2));
	}
	// Fill constructor
	{
		i32x3 v1 = i32x3_splat(3);
		CHECK(v1.x == 3);
		CHECK(v1.y == 3);
		CHECK(v1.z == 3);
	}
	// Constructor (x, y, z)
	{
		i32x3 v1 = i32x3_init(3, -1, -2);
		CHECK(v1[0] == 3);
		CHECK(v1[1] == -1);
		CHECK(v1[2] == -2);
	}
	// Constructor (xy, z)
	{
		i32x3 v1 = i32x3_init2(i32x2_init(3, -1), -2);
		CHECK(v1[0] == 3);
		CHECK(v1[1] == -1);
		CHECK(v1[2] == -2);
	}
	// Cast constructor
	{
		i32x3 v1 = i32x3_from_f32(f32x3_init(-1.0f, 1.0f, -2.0f));
		CHECK(v1.x == -1);
		CHECK(v1.y == 1);
		CHECK(v1.z == -2);
	}
	// Access operator []
	{
		i32x3 v;
		v[0] = 4;
		v[1] = -2;
		v[2] = 1;
		CHECK(v[0] == 4);
		CHECK(v[1] == -2);
		CHECK(v[2] == 1);
	}
}

TEST_CASE("Vec: vec4_specialization")
{
	// Data
	{
		i32x4 v;
		CHECK(sizeof(i32x4) == sizeof(int) * 4);
		v.data()[0] = 1;
		v.data()[1] = 2;
		v.data()[2] = 3;
		v.data()[3] = 4;
		CHECK(v.x == 1);
		CHECK(v.y == 2);
		CHECK(v.z == 3);
		CHECK(v.w == 4);
		CHECK(v.xyz() == (i32x3{1, 2, 3}));
		CHECK(v.xy() == (i32x2{1, 2}));
	}
	// Fill constructor
	{
		i32x4 v1 = i32x4_splat(3);
		CHECK(v1.x == 3);
		CHECK(v1.y == 3);
		CHECK(v1.z == 3);
		CHECK(v1.w == 3);
	}
	// Constructor (x, y, z, w)
	{
		i32x4 v1{ 3, -1, -2, 9 };
		CHECK(v1[0] == 3);
		CHECK(v1[1] == -1);
		CHECK(v1[2] == -2);
		CHECK(v1[3] == 9);
	}
	// Constructor (xyz, w)
	{
		i32x4 v1 = i32x4_init3(i32x3_init(3, -1, -2), 9);
		CHECK(v1[0] == 3);
		CHECK(v1[1] == -1);
		CHECK(v1[2] == -2);
		CHECK(v1[3] == 9);
	}
	// Constructor (xy, z, w)
	{
		i32x4 v1 = i32x4_init2(i32x2_init(3, -1), -2, 9);
		CHECK(v1[0] == 3);
		CHECK(v1[1] == -1);
		CHECK(v1[2] == -2);
		CHECK(v1[3] == 9);
	}
	// Cast constructor
	{
		i32x4 v1 = i32x4_from_f32(f32x4_init(-1.0f, 1.0f, -2.0f, 4.0f));
		CHECK(v1.x == -1);
		CHECK(v1.y == 1);
		CHECK(v1.z == -2);
		CHECK(v1.w == 4);
	}
	// Access [] operator
	{
		i32x4 v;
		v[0] = 4;
		v[1] = -2;
		v[2] = 1;
		v[3] = 9;
		CHECK(v[0] == 4);
		CHECK(v[1] == -2);
		CHECK(v[2] == 1);
		CHECK(v[3] == 9);
	}
}

TEST_CASE("Vec: arithmetic_operators")
{
	// Addition
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		auto v3 = v1 + v2;
		CHECK(v3[0] == 1);
		CHECK(v3[1] == -4);
		CHECK(v3[2] == 6);
		// Integrity check of base vectors
		CHECK(v1[0] == 1);
		CHECK(v1[1] == -2);
		CHECK(v1[2] == 5);
		CHECK(v2[0] == 0);
		CHECK(v2[1] == -2);
		CHECK(v2[2] == 1);
	}
	// Subtraction
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		auto v3 = v1 - v2;
		CHECK(v3[0] == 1);
		CHECK(v3[1] == 0);
		CHECK(v3[2] == 4);
		auto v4 = v2 - v1;
		CHECK(v4[0] == -1);
		CHECK(v4[1] == 0);
		CHECK(v4[2] == -4);
		// Integrity check of base vectors
		CHECK(v1[0] == 1);
		CHECK(v1[1] == -2);
		CHECK(v1[2] == 5);
		CHECK(v2[0] == 0);
		CHECK(v2[1] == -2);
		CHECK(v2[2] == 1);
	}
	// Negating (-x)
	{
		i32x3 v1{ 1, -2, 5 };

		i32x3 v3 = -v1;
		CHECK(v3[0] == -1);
		CHECK(v3[1] == 2);
		CHECK(v3[2] == -5);
		// Integrity check of base vectors
		CHECK(v1[0] == 1);
		CHECK(v1[1] == -2);
		CHECK(v1[2] == 5);
	}
	// Multiplication by number
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		i32x3 v3 = v1 * 3;
		CHECK(v3[0] == 3);
		CHECK(v3[1] == -6);
		CHECK(v3[2] == 15);
		i32x3 v4 = -3 * v2;
		CHECK(v4[0] == 0);
		CHECK(v4[1] == 6);
		CHECK(v4[2] == -3);
		// Integrity check of base vectors
		CHECK(v1[0] == 1);
		CHECK(v1[1] == -2);
		CHECK(v1[2] == 5);
		CHECK(v2[0] == 0);
		CHECK(v2[1] == -2);
		CHECK(v2[2] == 1);
	}
	// Element-wise multiplication
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		i32x3 v3 = v1 * v2;
		CHECK(v3[0] == 0);
		CHECK(v3[1] == 4);
		CHECK(v3[2] == 5);
		// Integrity check of base vectors
		CHECK(v1[0] == 1);
		CHECK(v1[1] == -2);
		CHECK(v1[2] == 5);
		CHECK(v2[0] == 0);
		CHECK(v2[1] == -2);
		CHECK(v2[2] == 1);
	}
	// Division by number
	{
		i32x2 v3 = i32x2{ 2, -2 } / 2;
		CHECK(v3[0] == 1);
		CHECK(v3[1] == -1);
		i32x2 v4 = -8 / i32x2_init(2, 4);
		CHECK(v4.x == -4);
		CHECK(v4.y == -2);
	}
	// Element-wise divison
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		auto v3 = v1 / v1;
		CHECK(v3[0] == 1);
		CHECK(v3[1] == 1);
		CHECK(v3[2] == 1);
	}
	// Addition assignment
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		v1 += v2;
		CHECK(v1[0] == 1);
		CHECK(v1[1] == -4);
		CHECK(v1[2] == 6);
	}
	// Subtraction assignment
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		v1 -= v2;
		CHECK(v1[0] == 1);
		CHECK(v1[1] == 0);
		CHECK(v1[2] == 4);
	}
	// Multiplication by number assignment
	{
		i32x3 v1{ 1, -2, 5 };

		v1 *= 3;
		CHECK(v1[0] == 3);
		CHECK(v1[1] == -6);
		CHECK(v1[2] == 15);
	}
	// Element-wise multiplication assignment
	{
		i32x3 v1{ 1, -2, 5 };
		i32x3 v2{ 0, -2, 1 };

		v1 *= v2;
		CHECK(v1[0] == 0);
		CHECK(v1[1] == 4);
		CHECK(v1[2] == 5);
	}
	// Division by number assignment
	{
		i32x2 v3{ 2, -2 };
		v3 /= 2;
		CHECK(v3[0] == 1);
		CHECK(v3[1] == -1);
	}
	// Element-wise division assignment
	{
		i32x3 v1{ 1, -2, 5 };

		i32x3 v1Copy = v1; // Not necessary, just to remove warning from Clang.
		v1 /= v1Copy;
		CHECK(v1[0] == 1);
		CHECK(v1[1] == 1);
		CHECK(v1[2] == 1);
	}
}

TEST_CASE("Vec: length_of_vectors")
{
	f32x2 v1 = f32x2_init(2.0f, 0.0f);
	f32x4 v2 = f32x4_init(-2.0f, 2.0f, 2.0f, -2.0f);
	CHECK(sfz::eqf(f32x2_length(v1), 2.0f));
	CHECK(sfz::eqf(f32x4_length(v2), 4.0f));
}

TEST_CASE("Vec: normalizing_vector")
{
	f32x4 v1 = f32x4_normalize(f32x4_init(-2.f, 2.f, -2.f, 2.f));
	CHECK(sfz::eqf(v1, f32x4_init(-0.5f, 0.5f, -0.5f, 0.5f)));
	CHECK(f32x3_normalizeSafe(f32x3_splat(0.0f)) == f32x3_splat(0.0f));
}

TEST_CASE("Vec: comparison_operators")
{
	i32x3 v1{ -4, 0, 0 };
	i32x3 v2{ 0, 2, 0 };
	i32x3 v3{ 0, 2, 0 };

	CHECK(v1 == v1);
	CHECK(v2 == v2);
	CHECK(v3 == v3);
	CHECK(v2 == v3);
	CHECK(v3 == v2);
	CHECK(v1 != v2);
	CHECK(v2 != v1);
}

TEST_CASE("Vec: dot_product")
{
	// Correctness test
	{
		i32x3 v1 = i32x3_init(1, 0, -2);
		i32x3 v2 = i32x3_init(6, 2, 2 );
		i32 scalarProduct = i32x3_dot(v1, v2);

		CHECK(scalarProduct == 2);

		CHECK(v1[0] == 1);
		CHECK(v1[1] == 0);
		CHECK(v1[2] == -2);
		CHECK(v2[0] == 6);
		CHECK(v2[1] == 2);
		CHECK(v2[2] == 2);
	}
	// Using same vector twice
	{
		i32x2 v1 = i32x2_init(-3, 2);
		i32 scalarProduct = i32x2_dot(v1, v1);

		CHECK(scalarProduct == 13);

		CHECK(v1[0] == -3);
		CHECK(v1[1] == 2);
	}
}

TEST_CASE("Vec: cross_product")
{
	// Correctness test
	{
		i32x3 v1 = i32x3_init(-1, 4, 0);
		i32x3 v2 = i32x3_init(1, -2, 3);
		i32x3 res = i32x3_cross(v1, v2);

		CHECK(res[0] == 12);
		CHECK(res[1] == 3);
		CHECK(res[2] == -2);
	}
	// 2nd correctness test
	{
		i32x3 v1 = i32x3_init(-1, 4, 0);
		i32x3 v2 = i32x3_init(1, -2, 3);
		i32x3 res = i32x3_cross(v2, v1);

		CHECK(res[0] == -12);
		CHECK(res[1] == -3);
		CHECK(res[2] == 2);
	}
	// A x A = 0
	{
		i32x3 v1 = i32x3_init(-1, 4, 0);
		i32x3 v2 = i32x3_init(1, -2, 3);

		i32x3 res1 = i32x3_cross(v1, v1);
		CHECK(res1[0] == 0);
		CHECK(res1[1] == 0);
		CHECK(res1[2] == 0);

		i32x3 res2 = i32x3_cross(v2, v2);
		CHECK(res2[0] == 0);
		CHECK(res2[1] == 0);
		CHECK(res2[2] == 0);
	}
}

TEST_CASE("Vec: element_sum")
{
	CHECK(sfz::elemSum(f32x2_init(1.0f, 2.0f)) == 3.0f);
	CHECK(sfz::elemSum(f32x3_init(1.0f, 2.0f, 3.0f)) == 6.0f);
	CHECK(sfz::elemSum(f32x4_init(1.0f, 2.0f, 3.0f, 4.0f)) == 10.0f);

	CHECK(sfz::elemSum(i32x2_init(1, 2)) == 3);
	CHECK(sfz::elemSum(i32x3_init(1, 2, 3)) == 6);
	CHECK(sfz::elemSum(i32x4_init(1, 2, 3, 4)) == 10);

	CHECK(sfz::elemSum(i32x2_init(0, 0)) == 0);
	CHECK(sfz::elemSum(i32x3_init(0, 0, 0)) == 0);
	CHECK(sfz::elemSum(i32x4_init(0, 0, 0, 0)) == 0);

	CHECK(sfz::elemSum(i32x2_init(-3, 3)) == 0);
	CHECK(sfz::elemSum(i32x3_init(-2, -1, 3)) == 0);
	CHECK(sfz::elemSum(i32x4_init(-4, -5, 10, -2)) == -1);
}

TEST_CASE("Vec: element_max")
{
	CHECK(sfz::elemMax(f32x2_init(1.0f, 2.0f)) == 2.0f);
	CHECK(sfz::elemMax(f32x3_init(1.0f, 2.0f, 3.0f)) == 3.0f);
	CHECK(sfz::elemMax(f32x4_init(1.0f, 2.0f, 3.0f, 4.0f)) == 4.0f);

	CHECK(sfz::elemMax(i32x2_init(1, 2)) == 2);
	CHECK(sfz::elemMax(i32x3_init(1, 2, 3)) == 3);
	CHECK(sfz::elemMax(i32x4_init(1, 2, 3, 4)) == 4);

	CHECK(sfz::elemMax(i32x2_init(0, 0)) == 0);
	CHECK(sfz::elemMax(i32x3_init(0, 0, 0)) == 0);
	CHECK(sfz::elemMax(i32x4_init(0, 0, 0, 0)) == 0);

	CHECK(sfz::elemMax(i32x2_init(-3, 3)) == 3);
	CHECK(sfz::elemMax(i32x3_init(-2, -1, 3)) == 3);
	CHECK(sfz::elemMax(i32x4_init(-4, -5, 10, -2)) == 10);
}

TEST_CASE("Vec: element_min")
{
	CHECK(sfz::elemMin(f32x2_init(1.0f, 2.0f)) == 1.0f);
	CHECK(sfz::elemMin(f32x3_init(1.0f, 2.0f, 3.0f)) == 1.0f);
	CHECK(sfz::elemMin(f32x4_init(1.0f, 2.0f, 3.0f, 4.0f)) == 1.0f);

	CHECK(sfz::elemMin(i32x2_init(1, 2)) == 1);
	CHECK(sfz::elemMin(i32x3_init(1, 2, 3)) == 1);
	CHECK(sfz::elemMin(i32x4_init(1, 2, 3, 4)) == 1);

	CHECK(sfz::elemMin(i32x2_init(0, 0)) == 0);
	CHECK(sfz::elemMin(i32x3_init(0, 0, 0)) == 0);
	CHECK(sfz::elemMin(i32x4_init(0, 0, 0, 0)) == 0);

	CHECK(sfz::elemMin(i32x2_init(-3, 3)) == -3);
	CHECK(sfz::elemMin(i32x3_init(-2, -1, 3)) == -2);
	CHECK(sfz::elemMin(i32x4_init(-4, -5, 10, -2)) == -5);
}

// Math functions
// ------------------------------------------------------------------------------------------------

TEST_CASE("Math: eqf")
{
	// f32
	{
		CHECK(sfz::eqf(2.0f, 2.0f + (sfz::EQF_EPS * 0.95f)));
		CHECK(!sfz::eqf(2.0f, 2.0f + (sfz::EQF_EPS * 1.05f)));
		CHECK(sfz::eqf(2.0f, 2.0f - (sfz::EQF_EPS * 0.95f)));
		CHECK(!sfz::eqf(2.0f, 2.0f - (sfz::EQF_EPS * 1.05f)));
	}
	// f32x2
	{
		CHECK(sfz::eqf(f32x2_splat(2.0f), f32x2_splat(2.0f + (sfz::EQF_EPS * 0.95f))));
		CHECK(!sfz::eqf(f32x2_splat(2.0f), f32x2_splat(2.0f + (sfz::EQF_EPS * 1.05f))));
		CHECK(sfz::eqf(f32x2_splat(2.0f), f32x2_splat(2.0f - (sfz::EQF_EPS * 0.95f))));
		CHECK(!sfz::eqf(f32x2_splat(2.0f), f32x2_splat(2.0f - (sfz::EQF_EPS * 1.05f))));
	}
	// f32x3
	{
		CHECK(sfz::eqf(f32x3_splat(2.0f), f32x3_splat(2.0f + (sfz::EQF_EPS * 0.95f))));
		CHECK(!sfz::eqf(f32x3_splat(2.0f), f32x3_splat(2.0f + (sfz::EQF_EPS * 1.05f))));
		CHECK(sfz::eqf(f32x3_splat(2.0f), f32x3_splat(2.0f - (sfz::EQF_EPS * 0.95f))));
		CHECK(!sfz::eqf(f32x3_splat(2.0f), f32x3_splat(2.0f - (sfz::EQF_EPS * 1.05f))));
	}
	// f32x4
	{
		CHECK(sfz::eqf(f32x4_splat(2.0f), f32x4_splat(2.0f + (sfz::EQF_EPS * 0.95f))));
		CHECK(!sfz::eqf(f32x4_splat(2.0f), f32x4_splat(2.0f + (sfz::EQF_EPS * 1.05f))));
		CHECK(sfz::eqf(f32x4_splat(2.0f), f32x4_splat(2.0f - (sfz::EQF_EPS * 0.95f))));
		CHECK(!sfz::eqf(f32x4_splat(2.0f), f32x4_splat(2.0f - (sfz::EQF_EPS * 1.05f))));
	}
}

TEST_CASE("Math: abs")
{
	CHECK(f32_abs(-2.0f) == 2.0f);
	CHECK(f32_abs(3.0f) == 3.0f);
	CHECK(f32x2_abs(f32x2_init(-1.0f, 2.0f)) == f32x2_init(1.0, 2.0));
	CHECK(f32x3_abs(f32x3_init(2.0f, -4.0f, -6.0f)) == f32x3_init(2.0f, 4.0f, 6.0f));
	CHECK(f32x4_abs(f32x4_init(-4.0f, 2.0f, -4.0f, -1.0f)) == f32x4_init(4.0f, 2.0f, 4.0f, 1.0f));

	CHECK(i32_abs(-2) == 2);
	CHECK(i32_abs(3) == 3);
	CHECK(i32x2_abs(i32x2_init(-1, 2)) == i32x2_init(1, 2));
	CHECK(i32x3_abs(i32x3_init(2, -4, -6)) == i32x3_init(2, 4, 6));
	CHECK(i32x4_abs(i32x4_init(-4, 2, -4, -1)) == i32x4_init(4, 2, 4, 1));
}

TEST_CASE("Math: min_float")
{
	CHECK(f32_min(0.0f, 0.0f) == 0.0f);

	CHECK(f32_min(-1.0f, 0.0f) == -1.0f);
	CHECK(f32_min(0.0f, -1.0f) == -1.0f);

	CHECK(f32_min(-1.0f, -2.0f) == -2.0f);
	CHECK(f32_min(-2.0f, -1.0f) == -2.0f);

	CHECK(f32_min(1.0f, 0.0f) == 0.0f);
	CHECK(f32_min(0.0f, 1.0f) == 0.0f);

	CHECK(f32_min(1.0f, 2.0f) == 1.0f);
	CHECK(f32_min(2.0f, 1.0f) == 1.0f);
}

TEST_CASE("Math: max_float")
{
	CHECK(f32_max(0.0f, 0.0f) == 0.0f);

	CHECK(f32_max(-1.0f, 0.0f) == 0.0f);
	CHECK(f32_max(0.0f, -1.0f) == 0.0f);

	CHECK(f32_max(-1.0f, -2.0f) == -1.0f);
	CHECK(f32_max(-2.0f, -1.0f) == -1.0f);

	CHECK(f32_max(1.0f, 0.0f) == 1.0f);
	CHECK(f32_max(0.0f, 1.0f) == 1.0f);

	CHECK(f32_max(1.0f, 2.0f) == 2.0f);
	CHECK(f32_max(2.0f, 1.0f) == 2.0f);
}

TEST_CASE("Math: min_int32")
{
	CHECK(i32_min(0, 0) == 0);

	CHECK(i32_min(-1, 0) == -1);
	CHECK(i32_min(0, -1) == -1);

	CHECK(i32_min(-1, -2) == -2);
	CHECK(i32_min(-2, -1) == -2);

	CHECK(i32_min(1, 0) == 0);
	CHECK(i32_min(0, 1) == 0);

	CHECK(i32_min(1, 2) == 1);
	CHECK(i32_min(2, 1) == 1);
}

TEST_CASE("Math: max_int32")
{
	CHECK(i32_max(0, 0) == 0);

	CHECK(i32_max(-1, 0) == 0);
	CHECK(i32_max(0, -1) == 0);

	CHECK(i32_max(-1, -2) == -1);
	CHECK(i32_max(-2, -1) == -1);

	CHECK(i32_max(1, 0) == 1);
	CHECK(i32_max(0, 1) == 1);

	CHECK(i32_max(1, 2) == 2);
	CHECK(i32_max(2, 1) == 2);
}

TEST_CASE("Math: min_uint32")
{
	CHECK(u32_min(0u, 0u) == 0u);

	CHECK(u32_min(1u, 0u) == 0u);
	CHECK(u32_min(0u, 1u) == 0u);

	CHECK(u32_min(1u, 2u) == 1u);
	CHECK(u32_min(2u, 1u) == 1u);
}

TEST_CASE("Math: max_uint32")
{
	CHECK(u32_max(0u, 0u) == 0u);

	CHECK(u32_max(1u, 0u) == 1u);
	CHECK(u32_max(0u, 1u) == 1u);

	CHECK(u32_max(1u, 2u) == 2u);
	CHECK(u32_max(2u, 1u) == 2u);
}

TEST_CASE("Math: min_vec")
{
	CHECK(f32x4_min(f32x4_init(1.0f, 2.0f, -3.0f, -4.0f), f32x4_init(2.0f, 1.0f, -5.0f, -2.0f)) == f32x4_init(1.0f, 1.0f, -5.0f, -4.0f));
	CHECK(i32x4_min(i32x4_init(1, 2, -3, -4), i32x4_init(2, 1, -5, -2)) == i32x4_init(1, 1, -5, -4));

	CHECK(f32x4_min(f32x4_init(1.0f, 2.0f, -3.0f, -4.0f), f32x4_splat(-1.0f)) == f32x4_init(-1.0f, -1.0f, -3.0f, -4.0f));
	CHECK(i32x4_min(i32x4_init(1, 2, -3, -4), i32x4_splat(-1)) == i32x4_init(-1, -1, -3, -4));
}

TEST_CASE("Math: max_vec")
{
	CHECK(f32x4_max(f32x4_init(1.0f, 2.0f, -3.0f, -4.0f), f32x4_init(2.0f, 1.0f, -5.0f, -2.0f)) == f32x4_init(2.0f, 2.0f, -3.0f, -2.0f));
	CHECK(i32x4_max(i32x4_init(1, 2, -3, -4), i32x4_init(2, 1, -5, -2)) == i32x4_init(2, 2, -3, -2));

	CHECK(f32x4_max(f32x4_init(1.0f, 2.0f, -3.0f, -4.0f), f32x4_splat(1.0f)) == f32x4_init(1.0f, 2.0f, 1.0f, 1.0f));
	CHECK(i32x4_max(i32x4_init(1, 2, -3, -4), i32x4_splat(1)) == i32x4_init(1, 2, 1, 1));
}

TEST_CASE("Math: clamp")
{
	CHECK(i32x4_clamps(i32x4_init(-2, 0, 2, 4), -1, 2) == i32x4_init(-1, 0, 2, 2));
	CHECK(i32x4_clampv(i32x4_init(-2, 0, 2, 4), i32x4_init(0, -1, -1, 5), i32x4_init(1, 1, 1, 6)) == i32x4_init(0, 0, 1, 5));
}

TEST_CASE("Math: sgn")
{
	{
		CHECK(sfz::sgn(0.0f) == 1.0f);
		CHECK(sfz::sgn(-4.0f) == -1.0f);
		CHECK(sfz::sgn(0) == 1);
		CHECK(sfz::sgn(-4) == -1);
	}

	{
		CHECK(sfz::sgn(f32x2_init(5.0f, -5.0f)) == f32x2_init(1.0f, -1.0f));
		CHECK(sfz::sgn(f32x2_init(-5.0f, 5.0)) == f32x2_init(-1.0f, 1.0f));
		CHECK(sfz::sgn(i32x2_init(6, -2)) == i32x2_init(1, -1));
		CHECK(sfz::sgn(i32x2_init(-7, 1)) == i32x2_init(-1, 1));
	}

	{
		CHECK(sfz::sgn(f32x3_init(5.0f, -5.0f, -2.0f)) == f32x3_init(1.0f, -1.0f, -1.0f));
		CHECK(sfz::sgn(f32x3_init(-5.0f, 5.0, 29.0f)) == f32x3_init(-1.0f, 1.0f, 1.0f));
		CHECK(sfz::sgn(i32x3_init(6, -2, 2)) == i32x3_init(1, -1, 1));
		CHECK(sfz::sgn(i32x3_init(-7, 1, 2)) == i32x3_init(-1, 1, 1));
	}

	{
		CHECK(sfz::sgn(f32x4_init(5.0f, -5.0f, -2.0f, 3.0f)) == f32x4_init(1.0f, -1.0f, -1.0f, 1.0f));
		CHECK(sfz::sgn(f32x4_init(-5.0f, 5.0, 29.0f, -9.0f)) == f32x4_init(-1.0f, 1.0f, 1.0f, -1.0f));
		CHECK(sfz::sgn(i32x4_init(6, -2, 2, -7)) == i32x4_init(1, -1, 1, -1));
		CHECK(sfz::sgn(i32x4_init(-7, 1, 2, -4)) == i32x4_init(-1, 1, 1, -1));
	}

}

// Memory functions
// ------------------------------------------------------------------------------------------------

TEST_CASE("Memory: memswp")
{
	{
		constexpr char STR1[] = "HELLO WORLD";
		constexpr char STR2[] = "FOO_BAR_AND_SUCH";
		char buffer1[256] = {};
		char buffer2[256] = {};
		memcpy(buffer1, STR1, sizeof(STR1));
		memcpy(buffer2, STR2, sizeof(STR2));
		CHECK(strncmp(buffer1, STR1, 256) == 0);
		CHECK(strncmp(buffer2, STR2, 256) == 0);
		sfz_memswp(buffer1, buffer2, u32_max(u32(sizeof(STR1)), u32(sizeof(STR2))));
		CHECK(strncmp(buffer2, STR1, 256) == 0);
		CHECK(strncmp(buffer1, STR2, 256) == 0);
		sfz_memswp(buffer1, buffer2, 256);
		CHECK(strncmp(buffer1, STR1, 256) == 0);
		CHECK(strncmp(buffer2, STR2, 256) == 0);
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
		sfz_memswp(buffer1, buffer2 + 5, sizeof(buffer1));
		for (u32 i = 0; i < NUM_ELEMS; i++) {
			CHECK(buffer1[i] == (i * i));
			CHECK(buffer2[i + 5] == i);
		}
		CHECK(buffer2[0] == 0);
		CHECK(buffer2[1] == 0);
		CHECK(buffer2[2] == 0);
		CHECK(buffer2[3] == 0);
		CHECK(buffer2[4] == 0);
		CHECK(buffer2[NUM_ELEMS + 5 + 0] == 0);
		CHECK(buffer2[NUM_ELEMS + 5 + 1] == 0);
		CHECK(buffer2[NUM_ELEMS + 5 + 2] == 0);
		CHECK(buffer2[NUM_ELEMS + 5 + 3] == 0);
		CHECK(buffer2[NUM_ELEMS + 5 + 4] == 0);
	}
}
