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

struct DirectionalLight {
	float3 lightDirVS;
	float ___PADDING0___;
	float3 strength;
	float ___PADDING1___;
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

// Shadow map helpers
// ------------------------------------------------------------------------------------------------

struct SampleShadowMapRes {

	// Depth stored in the shadow map
	float lightDepth;

	// Depth of the (view space) input coordinate, in the same space as the light depth
	float coordDepth;
};

// Samples a shadow map and returns the depth stored and the depth of the view space coordinate
SampleShadowMapRes sampleShadowMapDepth(
	Texture2D shadowMap,
	SamplerState samplerState,
	float4x4 lightMatrix,
	float3 posVS)
{
	float4 tmp = mul(lightMatrix, float4(posVS, 1.0));
	tmp.xyz /= tmp.w; // TODO: Unsure if necessary
	tmp.y = 1.0 - tmp.y;
	SampleShadowMapRes res;
	res.lightDepth = shadowMap.Sample(samplerState, tmp.xy).r;
	res.coordDepth = tmp.z;
	return res;
}

// Samples a (reverse-z) shadow map and returns 1.0 if view space position is in light, 0 otherwise
float sampleShadowMap(
	Texture2D shadowMap,
	SamplerState samplerState,
	float4x4 lightMatrix,
	float3 posVS)
{
	SampleShadowMapRes res = sampleShadowMapDepth(shadowMap, samplerState, lightMatrix, posVS);
	float shadow = res.coordDepth >= res.lightDepth ? 1.0 : 0.0;
	return shadow;
}

// Samples a (reverse-z) shadow map using PCF with a the specified kernel size.
float sampleShadowMapPCF(
	Texture2D shadowMap,
	SamplerState samplerState,
	float4x4 lightMatrix,
	float3 posVS,
	int kernelSize = 1,
	float midSampleWeight = 1.0)
{
	// Get shadow map dimensions and number of mip levels
	uint width = 0;
	uint height = 0;
	uint numMips = 0;
	shadowMap.GetDimensions(0, width, height, numMips);

	// Calculate texel size on shadow map from its dimensions
	float2 texelSize = float2(1.0, 1.0) / float2(width, height);

	// Transform view space coord to shadow map texture space
	float4 tmp = mul(lightMatrix, float4(posVS, 1.0));
	tmp.xyz /= tmp.w; // TODO: Unsure if necessary
	tmp.y = 1.0 - tmp.y;
	float2 centerCoord = tmp.xy;
	float posDepth = tmp.z;

	// Take (kernelSize * 2 + 1) to the power of two samples!
	float totalShadow = 0.0;
	for (int y = -kernelSize; y <= kernelSize; y++) {
		float yCoord = centerCoord.y + float(y) * texelSize.y;

		for (int x = -kernelSize; x <= kernelSize; x++) {
			float xCoord = centerCoord.x + float(x) * texelSize.x;

			// Sample shadow map
			float depthSample = shadowMap.Sample(samplerState, float2(xCoord, yCoord)).r;
			float shadowSample = posDepth >= depthSample ? 1.0 : 0.0;
			if (y == 0 && x == 0) {
				totalShadow += shadowSample * midSampleWeight; // Give mid sample extra weight
			}
			else {
				totalShadow += shadowSample;
			}

		}
	}

	// Normalize shadow
	uint totalNumSamples = kernelSize * 2 + 1;
	totalNumSamples = totalNumSamples * totalNumSamples;
	float normalizedShadow = totalShadow / (float(totalNumSamples - 1) + midSampleWeight);

	return normalizedShadow;
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

float3 shadeDirectional(
	DirectionalLight light,
	float shadow,
	float3 p,
	float3 n,
	float3 v,
	float nDotV,
	float3 albedo,
	float roughness,
	float metallic)
{
	// Shading parameters
	float3 toLight = -light.lightDirVS;
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
	float3 lightContrib = light.strength * shadow;

	float3 ks = ctF;
	float3 kd = (1.0 - ks) * (1.0 - metallic);

	// "Solves" reflectance equation under the assumption that the light source is a point light
	// and that there is no global illumination.
	return (kd * diffuse + specular) * lightContrib * nDotL;
}

float3 shadePointLight(
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
