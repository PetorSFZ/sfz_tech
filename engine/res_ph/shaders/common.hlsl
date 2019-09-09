// Copyright 2019 Peter Hillerström and skipifzero, all rights reserved.

// Materials
// ------------------------------------------------------------------------------------------------

static const uint MAX_NUM_MATERIALS = 128;

struct Material {
	float4 albedo;
	float4 emissive;
	float roughness;
	float metallic;
	int hasAlbedoTex;
	int hasMetallicRoughnessTex;
	int hasNormalTex;
	int hasOcclusionTex;
	int hasEmissiveTex;
	int ___PADDING___;
};

// Lights
// ------------------------------------------------------------------------------------------------

static const uint MAX_NUM_POINT_LIGHTS = 128;

struct PointLight {
	float3 posVS;
	float range;
	float3 strength;
	uint ___PADDING__;
};

// Standard vertex dynamic matrices
// ------------------------------------------------------------------------------------------------

struct DynamicMatrices {
	row_major float4x4 modelViewMatrix;

	// inverse(transpose(modelViewMatrix)) for non-uniform scaling
	// TODO: https://github.com/graphitemaster/normals_revisited
	row_major float4x4 normalMatrix;
};

// Depth helpers
// ------------------------------------------------------------------------------------------------

float3 depthToViewSpacePos(float depth, float2 fullscreenTexcoord, float4x4 invProjMatrix)
{
	float2 clipspaceXY;
	clipspaceXY.x = fullscreenTexcoord.x * 2.0 - 1.0;
	clipspaceXY.y = (1.0 - fullscreenTexcoord.y) * 2.0 - 1.0;
	float4 posTmp = mul(invProjMatrix, float4(clipspaceXY, depth, 1.0));
	float3 pos = posTmp.xyz / posTmp.w;
	return pos;
}

// Gamma correction functions
// ------------------------------------------------------------------------------------------------

static const float gamma = 2.2;
static const float3 gamma3 = float3(gamma, gamma, gamma);
static const float3 gammaInv3 = 1.0 / gamma3;

float3 linearize(float3 rgbGamma)
{
	return pow(rgbGamma, gamma3);
}

float4 linearize(float4 rgbaGamma)
{
	return float4(linearize(rgbaGamma.rgb), rgbaGamma.a);
}

float3 applyGammaCorrection(float3 linearValue)
{
	return pow(linearValue, gammaInv3);
}

float4 applyGammaCorrection(float4 linearValue)
{
	return float4(applyGammaCorrection(linearValue.rgb), linearValue.a);
}
