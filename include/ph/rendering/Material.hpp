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

#include <cstdint>

#include <sfz/math/Vector.hpp>

namespace ph {

using sfz::vec4_u8;

// Material struct
// ------------------------------------------------------------------------------------------------

struct Material final {
	vec4_u8 albedo = vec4_u8(0, 0, 0, 1);
	vec4_u8 emissive = vec4_u8(0, 0, 0, 0);
	uint8_t roughness = 0;
	uint8_t metallic = 0;

	uint16_t albedoTexIndex = uint16_t(~0);
	uint16_t metallicRoughnessTexIndex = uint16_t(~0);
	uint16_t normalTexIndex = uint16_t(~0);
	uint16_t occlusionTexIndex = uint16_t(~0);
	uint16_t emissiveTexIndex = uint16_t(~0);
};
static_assert(sizeof(Material) == sizeof(uint32_t) * 5, "Material is padded");

} // namespace ph
