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

// ShaderMaterial type
// ------------------------------------------------------------------------------------------------

// TODO: A lot of opportunity for optimization here.
// TODO: Replace f32x4 with u8x4 for albedo and emissive?
// TODO: Replace all integers with a bitset?
// TODO: Replace roughness and metallic with u8 primitive?
struct ShaderMaterial final {
	f32x4 albedo = f32x4(1.0f);
	f32x4 emissive = f32x4(1.0f); // Alpha ignored
	f32 roughness = 1.0f;
	f32 metallic = 1.0f;
	i32 hasAlbedoTex = 0;
	i32 hasMetallicRoughnessTex = 0;
	i32 hasNormalTex = 0;
	i32 hasOcclusionTex = 0;
	i32 hasEmissiveTex = 0;
	u32 ___PADDING___ = 0;
};
static_assert(sizeof(ShaderMaterial) == sizeof(u32) * 16, "ShaderMaterial is padded");

// ShaderPointLight type
// ------------------------------------------------------------------------------------------------

struct ShaderPointLight final {
	f32x3 posVS = f32x3(0.0f);
	f32 range = 0.0f;
	f32x3 strength = f32x3(0.0f);
	u32 ___PADDING___ = 0;
};
static_assert(sizeof(ShaderPointLight) == sizeof(u32) * 8);

// DirectionalLight type
// ------------------------------------------------------------------------------------------------

struct DirectionalLight final {
	f32x3 lightDirVS = f32x3(0.0f, -1.0f, 0.0);
	f32 ___PADDING0___ = 0.0f;
	f32x3 strength = f32x3(0.0f);
	f32 ___PADDING1___ = 0.0f;
};
static_assert(sizeof(DirectionalLight) == sizeof(u32) * 8);

// ForwardShader specific limits
// ------------------------------------------------------------------------------------------------

constexpr u32 MAX_NUM_SHADER_MATERIALS = 128;

struct ForwardShaderMaterialsBuffer final {
	ShaderMaterial materials[MAX_NUM_SHADER_MATERIALS];
};
static_assert(
	sizeof(ForwardShaderMaterialsBuffer) == sizeof(ShaderMaterial) * MAX_NUM_SHADER_MATERIALS);

constexpr u32 MAX_NUM_SHADER_POINT_LIGHTS = 128;

struct ForwardShaderPointLightsBuffer final {
	u32 numPointLights = 0;
	u32 ___padding___[3] = {};
	ShaderPointLight pointLights[MAX_NUM_SHADER_POINT_LIGHTS];
};
static_assert(
	sizeof(ForwardShaderPointLightsBuffer) ==
	(sizeof(ShaderPointLight) * MAX_NUM_SHADER_POINT_LIGHTS + sizeof(u32) * 4));

} // namespace sfz
