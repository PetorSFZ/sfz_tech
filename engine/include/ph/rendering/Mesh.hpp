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

#include <sfz/containers/DynArray.hpp>
#include <sfz/strings/StringID.hpp>

#include <ph/rendering/MeshView.hpp>

namespace ph {

using sfz::DynArray;
using sfz::StringID;

// Material unbound
// ------------------------------------------------------------------------------------------------

// An unbound version of phMaterial that uses StringID instead of indices to textures.
struct MaterialUnbound final {
	sfz::vec4_u8 albedo = sfz::vec4_u8(255, 255, 255, 255);
	sfz::vec3_u8 emissive = sfz::vec3_u8(255, 255, 255);
	uint8_t roughness = 255;
	uint8_t metallic = 255;

	StringID albedoTex = StringID::invalid();
	StringID metallicRoughnessTex = StringID::invalid();
	StringID normalTex = StringID::invalid();
	StringID occlusionTex = StringID::invalid();
	StringID emissiveTex = StringID::invalid();
};

// Mesh component
// ------------------------------------------------------------------------------------------------

struct MeshComponent final {
	DynArray<uint32_t> indices;
	uint32_t materialIdx = ~0u;
};

// Mesh
// ------------------------------------------------------------------------------------------------

struct Mesh final {
	DynArray<phVertex> vertices;
	DynArray<MeshComponent> components;
	DynArray<MaterialUnbound> materials;
};

} // namespace ph
