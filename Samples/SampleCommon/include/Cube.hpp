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

#pragma once

#include <cstdint>

// Cube model
// ------------------------------------------------------------------------------------------------

constexpr float CUBE_POSITIONS[] = {
	// x, y, z
	// Left
	-0.5f, -0.5f, -0.5f, // 0, left-bottom-back
	-0.5f, -0.5f, 0.5f, // 1, left-bottom-front
	-0.5f, 0.5f, -0.5f, // 2, left-top-back
	-0.5f, 0.5f, 0.5f, // 3, left-top-front

	// Right
	0.5f, -0.5f, -0.5f, // 4, right-bottom-back
	0.5f, -0.5f, 0.5f, // 5, right-bottom-front
	0.5f, 0.5f, -0.5f, // 6, right-top-back
	0.5f, 0.5f, 0.5f, // 7, right-top-front

	// Bottom
	-0.5f, -0.5f, -0.5f, // 8, left-bottom-back
	-0.5f, -0.5f, 0.5f, // 9, left-bottom-front
	0.5f, -0.5f, -0.5f, // 10, right-bottom-back
	0.5f, -0.5f, 0.5f, // 11, right-bottom-front

	// Top
	-0.5f, 0.5f, -0.5f, // 12, left-top-back
	-0.5f, 0.5f, 0.5f, // 13, left-top-front
	0.5f, 0.5f, -0.5f, // 14, right-top-back
	0.5f, 0.5f, 0.5f, // 15, right-top-front

	// Back
	-0.5f, -0.5f, -0.5f, // 16, left-bottom-back
	-0.5f, 0.5f, -0.5f, // 17, left-top-back
	0.5f, -0.5f, -0.5f, // 18, right-bottom-back
	0.5f, 0.5f, -0.5f, // 19, right-top-back

	// Front
	-0.5f, -0.5f, 0.5f, // 20, left-bottom-front
	-0.5f, 0.5f, 0.5f, // 21, left-top-front
	0.5f, -0.5f, 0.5f, // 22, right-bottom-front
	0.5f, 0.5f, 0.5f  // 23, right-top-front
};

constexpr float CUBE_NORMALS[] = {
	// x, y, z
	// Left
	-1.0f, 0.0f, 0.0f, // 0, left-bottom-back
	-1.0f, 0.0f, 0.0f, // 1, left-bottom-front
	-1.0f, 0.0f, 0.0f, // 2, left-top-back
	-1.0f, 0.0f, 0.0f, // 3, left-top-front

	// Right
	1.0f, 0.0f, 0.0f, // 4, right-bottom-back
	1.0f, 0.0f, 0.0f, // 5, right-bottom-front
	1.0f, 0.0f, 0.0f, // 6, right-top-back
	1.0f, 0.0f, 0.0f, // 7, right-top-front

	// Bottom
	0.0f, -1.0f, 0.0f, // 8, left-bottom-back
	0.0f, -1.0f, 0.0f, // 9, left-bottom-front
	0.0f, -1.0f, 0.0f, // 10, right-bottom-back
	0.0f, -1.0f, 0.0f, // 11, right-bottom-front

	// Top
	0.0f, 1.0f, 0.0f, // 12, left-top-back
	0.0f, 1.0f, 0.0f, // 13, left-top-front
	0.0f, 1.0f, 0.0f, // 14, right-top-back
	0.0f, 1.0f, 0.0f, // 15, right-top-front

	// Back
	0.0f, 0.0f, -1.0f, // 16, left-bottom-back
	0.0f, 0.0f, -1.0f, // 17, left-top-back
	0.0f, 0.0f, -1.0f, // 18, right-bottom-back
	0.0f, 0.0f, -1.0f, // 19, right-top-back

	// Front
	0.0f, 0.0f, 1.0f, // 20, left-bottom-front
	0.0f, 0.0f, 1.0f, // 21, left-top-front
	0.0f, 0.0f, 1.0f, // 22, right-bottom-front
	0.0f, 0.0f, 1.0f  // 23, right-top-front
};

constexpr float CUBE_TEXCOORDS[] = {
	// u, v
	// Left
	0.0f, 0.0f, // 0, left-bottom-back
	1.0f, 0.0f, // 1, left-bottom-front
	0.0f, 1.0f, // 2, left-top-back
	1.0f, 1.0f, // 3, left-top-front

	// Right
	1.0f, 0.0f, // 4, right-bottom-back
	0.0f, 0.0f, // 5, right-bottom-front
	1.0f, 1.0f, // 6, right-top-back
	0.0f, 1.0f, // 7, right-top-front

	// Bottom
	0.0f, 0.0f, // 8, left-bottom-back
	0.0f, 1.0f, // 9, left-bottom-front
	1.0f, 0.0f, // 10, right-bottom-back
	1.0f, 1.0f, // 11, right-bottom-front

	// Top
	0.0f, 1.0f, // 12, left-top-back
	0.0f, 0.0f, // 13, left-top-front
	1.0f, 1.0f, // 14, right-top-back
	1.0f, 0.0f, // 15, right-top-front

	// Back
	1.0f, 0.0f, // 16, left-bottom-back
	1.0f, 1.0f, // 17, left-top-back
	0.0f, 0.0f, // 18, right-bottom-back
	0.0f, 1.0f, // 19, right-top-back

	// Front
	0.0f, 0.0f, // 20, left-bottom-front
	0.0f, 1.0f, // 21, left-top-front
	1.0f, 0.0f, // 22, right-bottom-front
	1.0f, 1.0f  // 23, right-top-front
};

/*constexpr uint32_t CUBE_INDICES[] = {
	// Left
	0, 1, 2,
	3, 2, 1,

	// Right
	5, 4, 7,
	6, 7, 4,

	// Bottom
	8, 10, 9,
	11, 9, 10,

	// Top
	13, 15, 12,
	14, 12, 15,

	// Back
	18, 16, 19,
	17, 19, 16,

	// Front
	20, 22, 21,
	23, 21, 22
};*/

constexpr uint32_t CUBE_INDICES[] = {
	// Left
	2, 1, 0,
	1, 2, 3,

	// Right
	7, 4, 5,
	4, 7, 6,

	// Bottom
	9, 10, 8,
	10, 9, 11,

	// Top
	12, 15, 13,
	15, 12, 14,

	// Back
	19, 16, 18,
	16, 19, 17,

	// Front
	21, 22, 20,
	22, 21, 23
};

constexpr uint32_t CUBE_NUM_VERTICES = sizeof(CUBE_POSITIONS) / (sizeof(float) * 3);
constexpr uint32_t CUBE_NUM_INDICES = sizeof(CUBE_INDICES) / sizeof(uint32_t);
