// Copyright 2019 Peter Hillerström and skipifzero, all rights reserved.

#include "res_ph/shaders/common.hlsl"

// Push constants
// ------------------------------------------------------------------------------------------------

cbuffer Matrices : register(b0) {
	row_major float4x4 invProjMatrix;
}

// Point light inputs
// ------------------------------------------------------------------------------------------------

cbuffer PointLightsBuffer : register(b1) {
	uint numPointLights;
	uint3 ___PADDING___;
	PointLight pointLights[MAX_NUM_POINT_LIGHTS];
}

// Textures and samplers
// ------------------------------------------------------------------------------------------------

Texture2D albedoTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D depthTex : register(t3);

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
	float3 normal = normalTex.SampleLevel(linearSampler, texcoord, 0).rgb;
	float depth = depthTex.SampleLevel(nearestSampler, texcoord, 0).r;
	float3 pos = depthToViewSpacePos(depth, texcoord, invProjMatrix);

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

	float3 totalOutput = float3(0.0, 0.0, 0.0);

	// Point lights
	for (int i = 0; i < numPointLights; i++) {
		PointLight light = pointLights[i];

		float3 res = shadePointLight(
			light,
			p,
			n,
			v,
			nDotV,
			albedo,
			roughness,
			metallic);
		totalOutput += res;
	}

	// No negative output
	totalOutput = max(totalOutput, float3(0.0, 0.0, 0.0));

	// Write value
	outputTex[idx] += float4(totalOutput, 1.0);
}
