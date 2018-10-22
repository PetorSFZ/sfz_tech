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

// Material struct
// ------------------------------------------------------------------------------------------------

/// A rendering material used in Phantasy Engine.
///
/// PhantasyEngine (currently) exclusively uses roughness-metallic pbr materials. This might change
/// in the future. When (if) this struct is changed or updated the version of the renderer interface
/// is also updated.
///
/// A note regarding factors and textures:
/// For most information both a factor and a texture index is available. If the texture index is
/// "null" (in this case ~0, all bits set to 1), then only the factor is used. However, if the
/// texture is available the factor should be multiplied with the value read from the texture.
/// (Same as in glTF)
///
/// Example shader pseudocode for loading albedo:
/// Material m = materials[vertex.materialIndex];
/// vec4_u8 albedo = m.albedo;
/// if (m.albedoTexIndex != uint16_t(~0)) {
///     Texture albedoTex = textures[m.albedoTexIndex];
//      albedo *= texFetch(albedoTex, vertex.texcoord);
/// }
/// // TODO: albedo is in gamma space, need to linearize before shading
struct phMaterial {
	sfz::vec4_u8 albedo = sfz::vec4_u8(255, 255, 255, 255);
	sfz::vec3_u8 emissive = sfz::vec3_u8(255, 255, 255);
	uint8_t ___PADDING_UNUSED___ = 0;
	uint8_t roughness = 255;
	uint8_t metallic = 255;

	uint16_t albedoTexIndex = uint16_t(~0);
	uint16_t metallicRoughnessTexIndex = uint16_t(~0);
	uint16_t normalTexIndex = uint16_t(~0);
	uint16_t occlusionTexIndex = uint16_t(~0);
	uint16_t emissiveTexIndex = uint16_t(~0);
};
static_assert(sizeof(phMaterial) == sizeof(uint32_t) * 5, "phMaterial is padded");
