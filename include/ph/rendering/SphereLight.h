// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#ifdef __cplusplus
#include <cstdint>
using std::uint32_t;
#else
#include <stdint.h>
#endif

// C interface
#ifdef __cplusplus
extern "C" {
#endif

// SphereLight flags
// ------------------------------------------------------------------------------------------------

const uint32_t SPHERE_LIGHT_STATIC_SHADOWS_BIT = 1 << 0; // Static objects casts shadows
const uint32_t SPHERE_LIGHT_DYNAMIC_SHADOWS_BIT = 1 << 1; // Dynamic objects casts shadows

// SphereLight struct
// ------------------------------------------------------------------------------------------------

typedef struct {
	float pos[3];
	float radius; // Size of the light emitter, 0 makes it a point light
	float range; // Range of the emitted light
	float strength[3]; // Strength (and color) of light source
	uint32_t bitmaskFlags;
} phSphereLight;

// End C interface
#ifdef __cplusplus
}
#endif
