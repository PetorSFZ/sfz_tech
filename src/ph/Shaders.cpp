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

#include "ph/Shaders.hpp"

namespace ph {

// Header
// ------------------------------------------------------------------------------------------------

const char* SHADER_HEADER_SRC =

#ifdef __EMSCRIPTEN__
R"(
precision mediump float;
#define PH_WEB_GL 1
#define PH_VERTEX_IN attribute
#define PH_VERTEX_OUT varying
#define PH_FRAGMENT_IN varying
#define PH_TEXREAD(sampler, coord) texture2D(sampler, coord)
)"
#else
R"(
#version 330
precision highp float;
#define PH_DESKTOP_GL 1
#define PH_VERTEX_IN in
#define PH_VERTEX_OUT out
#define PH_FRAGMENT_IN in
#define PH_TEXREAD(sampler, coord) texture(sampler, coord)
)"
#endif

R"(
// Structs
// ------------------------------------------------------------------------------------------------

// Material struct
struct Material {
	vec4 albedo;
	vec3 emissive;
	float roughness;
	float metallic;

	int hasAlbedoTexture;
	int hasMetallicRoughnessTexture;
	int hasNormalTexture;
	int hasOcclusionTexture;
	int hasEmissiveTexture;
};

// SphereLight struct
struct SphereLight {
	vec3 vsPos;
	float radius;
	float range;
	vec3 strength;
};

)";

// Forward shading
// ------------------------------------------------------------------------------------------------

const char* VERTEX_SHADER_SRC = R"(

// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_VERTEX_IN vec3 inPos;
PH_VERTEX_IN vec3 inNormal;
PH_VERTEX_IN vec2 inTexcoord;

// Output
PH_VERTEX_OUT vec3 vsPos;
PH_VERTEX_OUT vec3 vsNormal;
PH_VERTEX_OUT vec2 texcoord;

// Uniforms
uniform mat4 uProjMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uModelMatrix;
uniform mat4 uNormalMatrix; // inverse(transpose(modelViewMatrix)) for non-uniform scaling

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	vec4 vsPosTmp = uViewMatrix * uModelMatrix * vec4(inPos, 1.0);

	vsPos = vsPosTmp.xyz / vsPosTmp.w; // Unsure if division necessary.
	vsNormal = (uNormalMatrix * vec4(inNormal, 0.0)).xyz;
	texcoord = inTexcoord;

	gl_Position = uProjMatrix * vsPosTmp;
}

)";

const char* FRAGMENT_SHADER_SRC = R"(

// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_FRAGMENT_IN vec3 vsPos;
PH_FRAGMENT_IN vec3 vsNormal;
PH_FRAGMENT_IN vec2 texcoord;

// Output
#ifdef PH_DESKTOP_GL
out vec4 fragOut;
#endif

// Uniforms (material)
uniform Material uMaterial;
uniform sampler2D uAlbedoTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uOcclusionTexture;
uniform sampler2D uEmissiveTexture;

// Uniforms (dynamic spherelights)
const int MAX_NUM_DYNAMIC_SPHERE_LIGHTS = 32;
uniform SphereLight uDynamicSphereLights[MAX_NUM_DYNAMIC_SPHERE_LIGHTS];
uniform int uNumDynamicSphereLights;

// Gamma Correction Functions
// ------------------------------------------------------------------------------------------------

const vec3 gamma = vec3(2.2);

vec3 linearize(vec3 rgbGamma)
{
	return pow(rgbGamma, gamma);
}

vec4 linearize(vec4 rgbaGamma)
{
	return vec4(linearize(rgbaGamma.rgb), rgbaGamma.a);
}

vec3 applyGammaCorrection(vec3 linearValue)
{
	return pow(linearValue, vec3(1.0 / gamma));
}

vec4 applyGammaCorrection(vec4 linearValue)
{
	return vec4(applyGammaCorrection(linearValue.rgb), linearValue.a);
}

// PBR shading functions
// ------------------------------------------------------------------------------------------------

const float PI = 3.14159265359;

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
vec3 fresnelSchlick(float nDotL, vec3 f0)
{
	return f0 + (vec3(1.0) - f0) * clamp(pow(1.0 - nDotL, 5.0), 0.0, 1.0);
}

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	// Albedo (Gamma space)
	vec3 albedo = uMaterial.albedo.rgb;
	float alpha = uMaterial.albedo.a;
	if (uMaterial.hasAlbedoTexture != 0) {
		vec4 tmp = PH_TEXREAD(uAlbedoTexture, texcoord);
		albedo *= tmp.rgb;
		alpha *= tmp.a;
	}
	albedo = linearize(albedo);

	// Skip fragment if it is transparent
	if (alpha < 0.1) discard;

	// Metallic & Roughness (Linear space)
	float metallic = uMaterial.metallic;
	float roughness = uMaterial.roughness;
	if (uMaterial.hasMetallicRoughnessTexture != 0) {
		vec2 tmp = PH_TEXREAD(uMetallicRoughnessTexture, texcoord).rg;
		metallic *= tmp.r;
		roughness *= tmp.g;
	}
	roughness = max(roughness, 0.01); // Div by 0 in ggx() if roughness = 0

	// Emissive (Linear space??? TODO: Maybe gamma)
	vec3 emissive = uMaterial.emissive.rgb;
	if (uMaterial.hasEmissiveTexture != 0) {
		emissive *= PH_TEXREAD(uEmissiveTexture, texcoord).rgb;
	}

	// Fragment's position and normal
	vec3 p = vsPos;
	vec3 n = normalize(vsNormal);

	vec3 v = normalize(-p); // to view
	float nDotV = dot(n, v);

	// Interpolation of normals sometimes makes them face away from the camera. Clamp
	// these to almost zero, to not break shading calculations.
	nDotV = max(0.001, nDotV);

	vec3 totalOutput = emissive;

	for (int i = 0; i < MAX_NUM_DYNAMIC_SPHERE_LIGHTS; i++) {

		// Skip if we are out of light sources
		if (i >= uNumDynamicSphereLights) break;

		// Retrieve light source
		SphereLight light = uDynamicSphereLights[i];

		// Shading parameters
		vec3 toLight = light.vsPos - p;
		float toLightDist = length(toLight);
		vec3 l = toLight * (1.0 / toLightDist); // to light
		vec3 h = normalize(l + v); // half vector (normal of microfacet)

		// If nDotL is <= 0 then the light source is not in the hemisphere of the surface, i.e.
		// no shading needs to be performed
		float nDotL = dot(n, l);
		if (nDotL <= 0.0) continue;

		// Lambert diffuse
		vec3 diffuse = albedo / PI;

		// Cook-Torrance specular
		// Normal distribution function
		float nDotH = max(dot(n, h), 0.0); // max() should be superfluous here
		float ctD = ggx(nDotH, roughness * roughness);

		// Geometric self-shadowing term
		float k = pow(roughness + 1.0, 2.0) / 8.0;
		float ctG = geometricSchlick(nDotL, nDotV, k);

		// Fresnel function
		// Assume all dielectrics have a f0 of 0.04, for metals we assume f0 == albedo
		vec3 f0 = mix(vec3(0.04), albedo, metallic);
		vec3 ctF = fresnelSchlick(nDotV, f0);

		// Calculate final Cook-Torrance specular value
		vec3 specular = ctD * ctF * ctG / (4.0 * nDotL * nDotV);

		// Calculates light strength
		float shadow = 1.0; // TODO: Shadow map
		float fallofNumerator = pow(clamp(1.0 - pow(toLightDist / light.range, 4.0), 0.0, 1.0), 2.0);
		float fallofDenominator = (toLightDist * toLightDist + 1.0);
		float falloff = fallofNumerator / fallofDenominator;
		vec3 lightContrib = falloff * light.strength * shadow;

		vec3 ks = ctF;
		vec3 kd = (1.0 - ks) * (1.0 - metallic);

		// "Solves" reflectance equation under the assumption that the light source is a point light
		// and that there is no global illumination.
		totalOutput += (kd * diffuse + specular) * lightContrib * nDotL;
	}

	vec4 outTmp = vec4(applyGammaCorrection(totalOutput), 1.0);
#ifdef PH_WEB_GL
	gl_FragColor = outTmp;
#else
	fragOut = outTmp;
#endif
}

)";

// Copy out shader
// ------------------------------------------------------------------------------------------------

const char* COPY_OUT_SHADER_SRC = R"(

// Input
PH_FRAGMENT_IN vec2 texcoord;

// Output
#ifdef PH_DESKTOP_GL
out vec4 fragOut;
#endif

// Uniforms
uniform sampler2D uTexture;

void main()
{
	vec3 val = PH_TEXREAD(uTexture, texcoord).rgb;

#ifdef PH_WEB_GL
	gl_FragColor = vec4(val, 1.0);
#else
	fragOut = vec4(val, 1.0);
#endif
}

)";

} // namespace ph
