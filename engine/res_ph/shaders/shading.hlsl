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
Texture2D emissiveTex : register(t2);
Texture2D normalTex : register(t3);
Texture2D depthTex : register(t4);

SamplerState nearestSampler : register(s0);
SamplerState linearSampler : register(s1);

// Shading functions
// ------------------------------------------------------------------------------------------------

static const float PI = 3.14159265359;

// References used:
// https://de45xmedrsdbp.cloudfront.net/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// http://blog.selfshadow.com/publications/s2016-shading-course/
// http://www.codinglabs.net/article_physically_based_rendering_cook_torrance.aspx
// http://graphicrants.blogspot.se/2013/08/specular-brdf-reference.html

// Normal distribution function, GGX/Trowbridge-Reitz
// a = roughness^2, UE4 parameterization
// dot(n,h) term should be clamped to 0 if negative
float ggx(float nDotH, float a)
{
	float a2 = a * a;
	float div = PI * pow(nDotH * nDotH * (a2 - 1.0) + 1.0, 2.0);
	return a2 / div;
}

// Schlick's model adjusted to fit Smith's method
// k = a/2, where a = roughness^2, however, for analytical light sources (non image based)
// roughness is first remapped to roughness = (roughnessOrg + 1) / 2.
// Essentially, for analytical light sources:
// k = (roughness + 1)^2 / 8
// For image based lighting:
// k = roughness^2 / 2
float geometricSchlick(float nDotL, float nDotV, float k)
{
	float g1 = nDotL / (nDotL * (1.0 - k) + k);
	float g2 = nDotV / (nDotV * (1.0 - k) + k);
	return g1 * g2;
}

// Schlick's approximation. F0 should typically be 0.04 for dielectrics
float3 fresnelSchlick(float nDotL, float3 f0)
{
	return f0 + (float3(1.0, 1.0, 1.0) - f0) * clamp(pow(1.0 - nDotL, 5.0), 0.0, 1.0);
}

float3 shade(
	PointLight light,
	float3 p,
	float3 n,
	float3 v,
	float nDotV,
	float3 albedo,
	float roughness,
	float metallic)
{
	// Shading parameters
	float3 toLight = light.posVS - p;
	float toLightDist = length(toLight);
	float3 l = toLight * (1.0 / toLightDist); // to light
	float3 h = normalize(l + v); // half vector (normal of microfacet)

	// If nDotL is <= 0 then the light source is not in the hemisphere of the surface, i.e.
	// no shading needs to be performed
	float nDotL = dot(n, l);
	if (nDotL <= 0.0) return float3(0.0, 0.0, 0.0);

	// Lambert diffuse
	float3 diffuse = albedo / PI;

	// Cook-Torrance specular
	// Normal distribution function
	float nDotH = max(dot(n, h), 0.0); // max() should be superfluous here
	float ctD = ggx(nDotH, roughness * roughness);

	// Geometric self-shadowing term
	float k = pow(roughness + 1.0, 2.0) / 8.0;
	float ctG = geometricSchlick(nDotL, nDotV, k);

	// Fresnel function
	// Assume all dielectrics have a f0 of 0.04, for metals we assume f0 == albedo
	float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
	float3 ctF = fresnelSchlick(nDotV, f0);

	// Calculate final Cook-Torrance specular value
	float3 specular = ctD * ctF * ctG / (4.0 * nDotL * nDotV);

	// Calculates light strength
	float shadow = 1.0; // TODO: Shadow map
	float fallofNumerator = pow(clamp(1.0 - pow(toLightDist / light.range, 4.0), 0.0, 1.0), 2.0);
	float fallofDenominator = (toLightDist * toLightDist + 1.0);
	float falloff = fallofNumerator / fallofDenominator;
	float3 lightContrib = falloff * light.strength * shadow;

	float3 ks = ctF;
	float3 kd = (1.0 - ks) * (1.0 - metallic);

	// "Solves" reflectance equation under the assumption that the light source is a point light
	// and that there is no global illumination.
	return (kd * diffuse + specular) * lightContrib * nDotL;
}

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

	float3 totalOutput = emissive;

	// Point lights
	for (int i = 0; i < numPointLights; i++) {
		PointLight light = pointLights[i];

		float3 res = shade(
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

	// TODO: Configurable ambient light hack
	totalOutput += 0.1 * albedo;

	//float4 forceUsage = 0.00001 *
		//mul(invProjMatrix, float4((albedo * metallic * roughness + emissive * depth + normal), 1.0));
	float4 forceUsage = float4(0.0, 0.0, 0.0, 0.0);
	return float4(totalOutput, 1.0) + forceUsage;
}
