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

	return float4(totalOutput, 1.0);
}
