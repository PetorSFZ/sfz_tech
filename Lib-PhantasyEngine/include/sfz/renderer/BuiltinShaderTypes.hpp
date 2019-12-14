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

#include <skipifzero.hpp>

namespace sfz {

using sfz::vec3;
using sfz::vec4;

// ShaderMaterial type
// ------------------------------------------------------------------------------------------------

// TODO: A lot of opportunity for optimization here.
// TODO: Replace vec4 with vec4_u8 for albedo and emissive?
// TODO: Replace all integers with a bitset?
// TODO: Replace roughness and metallic with u8 primitive?
struct ShaderMaterial final {
	vec4 albedo = vec4(1.0f);
	vec4 emissive = vec4(1.0f); // Alpha ignored
	float roughness = 1.0f;
	float metallic = 1.0f;
	int32_t hasAlbedoTex = 0;
	int32_t hasMetallicRoughnessTex = 0;
	int32_t hasNormalTex = 0;
	int32_t hasOcclusionTex = 0;
	int32_t hasEmissiveTex = 0;
	uint32_t ___PADDING___ = 0;
};
static_assert(sizeof(ShaderMaterial) == sizeof(uint32_t) * 16, "ShaderMaterial is padded");

// ShaderPointLight type
// ------------------------------------------------------------------------------------------------

struct ShaderPointLight final {
	vec3 posVS = vec3(0.0f);
	float range = 0.0f;
	vec3 strength = vec3(0.0f);
	uint32_t ___PADDING___ = 0;
};
static_assert(sizeof(ShaderPointLight) == sizeof(uint32_t) * 8);

// DirectionalLight type
// ------------------------------------------------------------------------------------------------

struct DirectionalLight final {
	vec3 lightDirVS = vec3(0.0f, -1.0f, 0.0);
	float ___PADDING0___ = 0.0f;
	vec3 strength = vec3(0.0f);
	float ___PADDING1___ = 0.0f;
};
static_assert(sizeof(DirectionalLight) == sizeof(uint32_t) * 8);

// ForwardShader specific limits
// ------------------------------------------------------------------------------------------------

constexpr uint32_t MAX_NUM_SHADER_MATERIALS = 128;

struct ForwardShaderMaterialsBuffer final {
	ShaderMaterial materials[MAX_NUM_SHADER_MATERIALS];
};
static_assert(
	sizeof(ForwardShaderMaterialsBuffer) == sizeof(ShaderMaterial) * MAX_NUM_SHADER_MATERIALS);

constexpr uint32_t MAX_NUM_SHADER_POINT_LIGHTS = 128;

struct ForwardShaderPointLightsBuffer final {
	uint32_t numPointLights = 0;
	uint32_t ___padding___[3] = {};
	ShaderPointLight pointLights[MAX_NUM_SHADER_POINT_LIGHTS];
};
static_assert(
	sizeof(ForwardShaderPointLightsBuffer) ==
	(sizeof(ShaderPointLight) * MAX_NUM_SHADER_POINT_LIGHTS + sizeof(uint32_t) * 4));

} // namespace sfz
