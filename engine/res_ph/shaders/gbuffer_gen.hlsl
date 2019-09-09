// Copyright 2019 Peter Hillerström and skipifzero, all rights reserved.

#include "res_ph/shaders/common.hlsl"

// Matrices input
// ------------------------------------------------------------------------------------------------

cbuffer StaticMatricesBuffer : register(b0) {
	row_major float4x4 projMatrix;
}

cbuffer DynamicMatricesBuffer : register(b1) {
	DynamicMatrices dynamicMatrices;
}

// Materials inputs
// ------------------------------------------------------------------------------------------------

cbuffer MaterialIndex : register(b2) {
	uint materialIdx;
	uint3 ___MATERIAL_INDEX_BUFFER_PADDING___;
}

cbuffer MaterialsBuffer : register(b3) {
	Material materials[MAX_NUM_MATERIALS];
}

// Texture and sampler inputs
// ------------------------------------------------------------------------------------------------

Texture2D albedoTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
//Texture2D normalTex : register(t2);
//Texture2D occlusionTex : register(t3);
Texture2D emissiveTex : register(t2);

SamplerState texSampler : register(s0);

// Vertex shader
// ------------------------------------------------------------------------------------------------

struct VSInput {
	float3 position : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float2 texcoord : TEXCOORD2;
};

struct VSOutput {
	float3 posVS : PARAM_0;
	float3 normalVS : PARAM_1;
	float2 texcoord : PARAM_2;
	float4 position : SV_Position;
};

VSOutput VSMain(VSInput input)
{
	float4 tmpPosVS = mul(dynamicMatrices.modelViewMatrix, float4(input.position, 1.0));

	VSOutput output;

	output.posVS = tmpPosVS.xyz / tmpPosVS.w;
	output.normalVS = mul(dynamicMatrices.normalMatrix, float4(input.normal, 0.0)).xyz;
	output.texcoord = input.texcoord;
	output.position = mul(projMatrix, tmpPosVS);

	return output;
}

// Pixel shader
// ------------------------------------------------------------------------------------------------

struct PSInput {
	float3 posVS : PARAM_0;
	float3 normalVS : PARAM_1;
	float2 texcoord : PARAM_2;
};

struct PSOutput {
	float4 albedo : SV_TARGET0;
	float2 metallicRoughness : SV_TARGET1;
	float4 emissive : SV_TARGET2;
	float4 normal : SV_TARGET3;
};

PSOutput PSMain(PSInput input)
{
	// Pick out material
	Material m = materials[materialIdx];

	// Albedo (Gamma space)
	float3 albedo = m.albedo.rgb;
	float alpha = m.albedo.a;
	if (m.hasAlbedoTex != 0) {
		float4 tmp = albedoTex.Sample(texSampler, input.texcoord);
		albedo *= tmp.rgb;
		alpha *= tmp.a;
	}

	// Skip pixel if it is transparent
	if (alpha < 0.1) discard;

	// Metallic & Roughness (Linear space)
	float metallic = m.metallic;
	float roughness = m.roughness;
	if (m.hasMetallicRoughnessTex != 0) {
		float2 tmp = metallicRoughnessTex.Sample(texSampler, input.texcoord).rg;
		metallic *= tmp.r;
		roughness *= tmp.g;
	}

	// Emissive (Linear space)
	float3 emissive = m.emissive.rgb;
	if (m.hasEmissiveTex != 0) {
		emissive *= emissiveTex.Sample(texSampler, input.texcoord).rgb;
	}

	// Group together input and write to GBuffer
	PSOutput output;
	output.albedo = float4(albedo, alpha);
	output.metallicRoughness = float2(metallic, roughness);
	output.emissive = float4(emissive, 1.0);
	output.normal = float4(input.normalVS, 1.0);
	return output;
}
