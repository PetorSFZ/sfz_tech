#include "sfz/renderer/PbrLUTs.hpp"

#include <SDL.h>

#include "sfz/Logging.hpp"

namespace sfz {

// Specular BRDF LUT
// ------------------------------------------------------------------------------------------------

// Port of: https://www.shadertoy.com/view/3lXXDB
// See also: https://bruop.github.io/ibl/

// Taken from https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/genbrdflut.frag
// Based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
static vec2 hammersley(uint32_t i, uint32_t N)
{
	// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	uint32_t bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10f;
	return vec2(float(i) /float(N), rdi);
}

// From the filament docs. Geometric Shadowing function
// https://google.github.io/filament/Filament.html#toc4.4.2
static float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
	float a2 = pow(roughness, 4.0f);
	float GGXV = NoL * sqrt(NoV * NoV * (1.0f - a2) + a2);
	float GGXL = NoV * sqrt(NoL * NoL * (1.0f - a2) + a2);
	return 0.5f / (GGXV + GGXL);
}

// Based on Karis 2014
static vec3 importanceSampleGGX(vec2 Xi, float roughness, vec3 N)
{
	float a = roughness * roughness;

	// Sample in spherical coordinates
	float Phi = 2.0f * PI * Xi.x;
	float CosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a*a - 1.0f) * Xi.y));
	float SinTheta = sqrt(1.0f - CosTheta * CosTheta);
	
	// Construct tangent space vector
	vec3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;

	// Tangent to world space
	vec3 UpVector = abs(N.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
	vec3 TangentX = normalize(cross(UpVector, N));
	vec3 TangentY = cross(N, TangentX);
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

// Karis 2014
static vec2 integrateBRDF(float roughness, float NoV)
{
	vec3 V;
	V.x = sqrt(1.0f - NoV * NoV); // sin
	V.y = 0.0f;
	V.z = NoV; // cos

	// N points straight upwards for this integration
	const vec3 N = vec3(0.0f, 0.0f, 1.0f);

	float A = 0.0f;
	float B = 0.0f;
	const uint32_t numSamples = 1024u;

	for (uint32_t i = 0u; i < numSamples; i++) {
		vec2 Xi = hammersley(i, numSamples);
		// Sample microfacet direction
		vec3 H = importanceSampleGGX(Xi, roughness, N);

		// Get the light direction
		vec3 L = 2.0f * dot(V, H) * H - V;

		float NoL = saturate(dot(N, L));
		float NoH = saturate(dot(N, H));
		float VoH = saturate(dot(V, H));

		if(NoL > 0.0f) {
			float V_pdf = V_SmithGGXCorrelated(NoV, NoL, roughness) * VoH * NoL / NoH;
			float Fc = pow(1.0f - VoH, 5.0f);
			A += (1.0f - Fc) * V_pdf;
			B += Fc * V_pdf;
		}
	}

	return 4.0f * vec2(A, B) / float(numSamples);
}

Image genSpecularBrdfLut(Allocator* allocator)
{
	constexpr uint32_t RES = 128;

	const uint64_t perfTickBefore = SDL_GetPerformanceCounter();

	Image lut = Image::allocate(RES, RES, ImageType::RG_F32, allocator);
	ImageView view = lut;

	for (uint32_t y = 0; y < RES; y++) {
		vec2* dstRow = view.rowPtr<vec2>(y);

		for (uint32_t x = 0; x < RES; x++) {
			vec2& dst = dstRow[x];
			
			constexpr vec2 eps = vec2(0.5f);
			vec2 uv = (vec2(float(x), float(y)) + eps) / vec2(float(RES));
			
			float a = uv.y;
			float mu = uv.x;
			dst = integrateBRDF(a, mu);
		}
	}

	const uint64_t perfTickAfter = SDL_GetPerformanceCounter();
	const uint64_t tickDiff = perfTickAfter - perfTickBefore;
	const uint64_t ticksPerSecond = SDL_GetPerformanceFrequency();
	float milliSecs = float(double(tickDiff) / double(ticksPerSecond)) * 1000.0f;
	SFZ_INFO("Renderer", "Generated specular PBR LUT, took: %.2fms", milliSecs);

	return lut;
}

} // namespace sfz
