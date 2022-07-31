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

#ifndef SFZ_MATRIX_H
#define SFZ_MATRIX_H
#pragma once

#include "sfz.h"

// Matrix (4x4)
// ------------------------------------------------------------------------------------------------

sfz_struct(SfzMat44) {
	f32x4 rows[4];
};

// A 4x4 matrix where the last row is omitted because it usually just contains (0, 0, 0, 1).
sfz_struct(SfzMat34) {
	f32x4 rows[3];
};

#endif
