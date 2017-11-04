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
#include <sfz/math/Vector.hpp>
#endif

#include "ph/ExternC.h"

// C CameraData struct
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	float pos[3]; float near;
	float dir[3]; float far;
	float up[3]; float vertFovDeg;
} phCameraData;

// C++ CameraData struct
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace ph {

using sfz::vec3;

struct CameraData final {
	vec3 pos = vec3(0.0f); float near = 0.0f;
	vec3 dir = vec3(0.0f); float far = 0.0f;
	vec3 up = vec3(0.0f); float vertFovDeg = 0.0f;
};

static_assert(sizeof(phCameraData) == sizeof(float) * 12, "phCameraData is padded");
static_assert(sizeof(phCameraData) == sizeof(CameraData), "ph::CameraData is padded");
static_assert(alignof(phCameraData) <= alignof(CameraData), "phCameraData has higher alignment requirements");

} // namespace ph

#endif
