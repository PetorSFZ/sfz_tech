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

// Unordered textures
// ------------------------------------------------------------------------------------------------

RWTexture2D<float4> outputTex : register(u0);

// Main
// ------------------------------------------------------------------------------------------------

[numthreads(32, 16, 1)]
void CSMain(
	uint3 groupIdx : SV_GroupID, // Index of group
	uint groupFlatIdx : SV_GroupIndex, // Flattened version of group index
	uint3 groupThreadIdx : SV_GroupThreadID, // Index of thread within group
	uint3 dispatchThreadIdx : SV_DispatchThreadID) // Global index of thread
{
	// Get output texture dimensions
	uint outputTexWidth = 0;
	uint outputTexHeight = 0;
	outputTex.GetDimensions(outputTexWidth, outputTexHeight);

	// Get thread index and exit if out of range
	uint2 idx = dispatchThreadIdx.xy;
	if (idx.x >= outputTexWidth || idx.y >= outputTexHeight) return;

	// Calculate a texcoord
	float2 texcoord = float2(idx) / float2(outputTexWidth, outputTexHeight);

	// Read values from GBuffer
	float3 albedo = albedoTex.SampleLevel(linearSampler, texcoord, 0).rgb; // Gamma space
	float2 metallicRoughness = metallicRoughnessTex.SampleLevel(linearSampler, texcoord, 0).rg;
	float metallic = metallicRoughness.r; // Linear space
	float roughness = metallicRoughness.g; // Linear space
	float3 emissive = emissiveTex.SampleLevel(linearSampler, texcoord, 0).rgb; // Linear space
	float3 normal = normalTex.SampleLevel(linearSampler, texcoord, 0).rgb;
	float depth = depthTex.SampleLevel(nearestSampler, texcoord, 0).r;
	float3 pos = depthToViewSpacePos(depth, texcoord, invProjMatrix);

	// Convert gamma space to linear
	albedo = linearize(albedo);

	// Pixel's position and normal
	float3 p = pos;
	float3 n = float3(0.0, 0.0, -1.0); // Normal towards camera if none specified
	if (!(normal.x == 0.0 && normal.y == 0.0 && normal.z == 0.0)) {
		n = normalize(normal);
	}

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
	
	// Write value
	outputTex[idx] = float4(totalOutput, 1.0) + forceUsage;
}
