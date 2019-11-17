// Copyright 2019 Peter Hillerström and skipifzero, all rights reserved.

#include "res_ph/shaders/common.hlsl"

// Constant buffers
// ------------------------------------------------------------------------------------------------

cbuffer PushConstants1 : register(b0) {
	row_major float4x4 invProjMatrix;
}

cbuffer LightInfo : register(b1) {
	DirectionalLight dirLight;
	row_major float4x4 lightMatrix1;
	row_major float4x4 lightMatrix2;
	row_major float4x4 lightMatrix3;
	float levelDist1;
	float levelDist2;
	float levelDist3;
	float ___PADDING___;
}

// Textures and samplers
// ------------------------------------------------------------------------------------------------

Texture2D albedoTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D emissiveTex : register(t2);
Texture2D normalTex : register(t3);
Texture2D depthTex : register(t4);

Texture2D shadowMap1 : register(t5);
Texture2D shadowMap2 : register(t6);
Texture2D shadowMap3 : register(t7);

SamplerState nearestSampler : register(s0);
SamplerState linearSampler : register(s1);

// Vertex shader
// ------------------------------------------------------------------------------------------------

struct VSInput {
	float3 position : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float2 texcoord : TEXCOORD2;
};

struct VSOutput {
	float2 texcoord : PARAM_0;
	float4 position : SV_Position;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output;
	output.texcoord = input.texcoord;
	output.position = float4(input.position, 1.0);
	return output;
}

// Pixel shader
// ------------------------------------------------------------------------------------------------

struct PSInput {
	float2 texcoord : PARAM_0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
	// Read values from GBuffer
	float3 albedo = albedoTex.Sample(linearSampler, input.texcoord).rgb; // Gamma space
	float2 metallicRoughness = metallicRoughnessTex.Sample(linearSampler, input.texcoord).rg;
	float metallic = metallicRoughness.r; // Linear space
	float roughness = metallicRoughness.g; // Linear space
	float3 emissive = emissiveTex.Sample(linearSampler, input.texcoord).rgb; // Linear space
	float3 normal = normalTex.Sample(linearSampler, input.texcoord).rgb;
	float depth = depthTex.Sample(nearestSampler, input.texcoord).r;
	float3 pos = depthToViewSpacePos(depth, input.texcoord, invProjMatrix);

	// Convert gamma space to linear
	albedo = linearize(albedo);

	// Pixel's position and normal
	float3 p = pos;
	float3 n = normalize(normal);

	float3 v = normalize(-p); // to view
	float nDotV = dot(n, v);

	// Interpolation of normals sometimes makes them face away from the camera. Clamp
	// these to almost zero, to not break shading calculations.
	nDotV = max(0.001, nDotV);

	// Div by 0 in ggx() if roughness = 0
	roughness = max(roughness, 0.001);

	// Sample shadow map
	float shadow = 1.0;
	float absDepth = abs(p.z);
	if (absDepth < levelDist1) {
		shadow = sampleShadowMapPCF(shadowMap1, nearestSampler, lightMatrix1, p, 3, 4.0);
	}
	else if (absDepth < levelDist2) {
		shadow = sampleShadowMapPCF(shadowMap2, nearestSampler, lightMatrix2, p, 2, 2.0);
	}
	else if (absDepth < levelDist3) {
		shadow = sampleShadowMapPCF(shadowMap3, nearestSampler, lightMatrix3, p, 1);
	}

	float3 totalOutput = emissive;

	totalOutput += shadeDirectional(
		dirLight,
		shadow,
		p,
		n,
		v,
		nDotV,
		albedo,
		roughness,
		metallic);

	// TODO: Configurable ambient light hack
	totalOutput += 0.05 * albedo;

	// No negative output
	totalOutput = max(totalOutput, float3(0.0, 0.0, 0.0));

	/*if (absDepth < levelDist1) {
		totalOutput.r += 0.1;
	}
	else if (absDepth < levelDist2) {
		totalOutput.g += 0.1;
	}
	else if (absDepth < levelDist3) {
		totalOutput.b += 0.1;
	}*/

	//float4 forceUsage = 0.00001 *
	//	mul(invProjMatrix, shadow * float4((albedo * metallic * roughness + emissive * depth + normal), 1.0));
	float4 forceUsage = float4(0.0, 0.0, 0.0, 0.0);
	return float4(totalOutput, 1.0) + forceUsage;
}
