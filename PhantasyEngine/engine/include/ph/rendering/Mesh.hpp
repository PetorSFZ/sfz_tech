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
#include <sfz/math/Vector.hpp>
#include <sfz/strings/StringID.hpp>

namespace ph {

using sfz::DynArray;
using sfz::StringID;
using sfz::vec2;
using sfz::vec3;
using sfz::vec4_u8;

// Vertex struct
// ------------------------------------------------------------------------------------------------

struct Vertex {
	vec3 pos = vec3(0.0f);
	vec3 normal = vec3(0.0f);
	vec2 texcoord = vec2(0.0f);

	constexpr Vertex() noexcept = default;
	constexpr Vertex(vec3 pos, vec3 normal, vec2 texcoord) : pos(pos), normal(normal), texcoord(texcoord) {}
	constexpr Vertex(const Vertex&) noexcept = default;
	constexpr Vertex& operator= (const Vertex&) noexcept = default;
};
static_assert(sizeof(Vertex) == sizeof(float) * 8);

// Material struct
// ------------------------------------------------------------------------------------------------

// A roughness-metallic PBR material used by standard meshes in Phantasy Engine.
//
// A note regarding factors and textures:
// For most information both a factor and a texture index is available. The factor is mandatory,
// but the texture is optional. If a texture is available the value read from it should be
// multiplied by the factor (same as in glTF).
struct Material final {
	vec4_u8 albedo = vec4_u8(255, 255, 255, 255); // Gamma space
	uint8_t roughness = 255; // Linear space
	uint8_t metallic = 255; // Linear space
	vec3 emissive = vec3(1.0f); // Linear space, can be higher than 1.0
	
	StringID albedoTex = StringID::invalid();
	StringID metallicRoughnessTex = StringID::invalid();
	StringID normalTex = StringID::invalid();
	StringID occlusionTex = StringID::invalid();
	StringID emissiveTex = StringID::invalid();
};

// Mesh component
// ------------------------------------------------------------------------------------------------

struct MeshComponent final {
	uint32_t materialIdx = ~0u;
	uint32_t firstIndex = ~0u;
	uint32_t numIndices = 0;
};

// Mesh
// ------------------------------------------------------------------------------------------------

struct Mesh final {
	DynArray<Vertex> vertices;
	DynArray<uint32_t> indices;
	DynArray<Material> materials;
	DynArray<MeshComponent> components;
};

} // namespace ph
