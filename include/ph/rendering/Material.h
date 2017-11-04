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
#include <sfz/math/Vector.hpp>
using std::int32_t;
#else
#include <stdint.h>
#endif

#include "ph/ExternC.h"

// C Material struct
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	int32_t albedoTexIndex, roughnessTexIndex, metallicTexIndex, padding;
	float albedo[4];
	float roughness, metallic, padding2, padding3;
} phMaterial;

// C++ Material struct
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace ph {

using sfz::vec4;

struct Material final {
	int32_t albedoTexIndex = -1;
	int32_t roughnessTexIndex = -1;
	int32_t metallicTexIndex = -1;
	int32_t padding;
	vec4 albedo = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	float roughness = 0.0f;
	float metallic = 0.0f;
	float padding2;
	float padding3;
};

static_assert(sizeof(phMaterial) == sizeof(int32_t) * 12, "phMaterial is padded");
static_assert(sizeof(phMaterial) == sizeof(Material), "Material is padded");

} // namespace ph

#endif
