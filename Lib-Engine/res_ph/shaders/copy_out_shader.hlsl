// Copyright 2019 Peter Hillerström and skipifzero, all rights reserved.

#include "res_ph/shaders/common.hlsl"

// Push constants
// ------------------------------------------------------------------------------------------------

cbuffer ResolutionBuffer : register(b0) {
	uint windowWidth;
	uint windowHeight;
	uint2 ___RESOLUTION_BUFFER_PADDING__;
}

// Textures and samplers
// ------------------------------------------------------------------------------------------------

Texture2D texture : register(t0);
SamplerState texSampler : register(s0);

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
	// Get texture dimensions and number of mip levels
	uint texWidth = 0;
	uint texHeight = 0;
	uint texNumMipLevels = 0;
	texture.GetDimensions(0, texWidth, texHeight, texNumMipLevels);

	// Just do a single linearly interpolated sample if texture is of lower resolution than window
	float3 linearLightAvg = float3(0.0, 0.0, 0.0);
	if (texWidth <= windowWidth && texHeight <= windowHeight) {
		linearLightAvg = max(texture.Sample(texSampler, input.texcoord).rgb, float3(0.0, 0.0, 0.0));
	}

	// Otherwise, do 4x4 linearly interpolated samples and take the average of them
	else {
		// Calculate some base constants
		float2 windowPixelSize = float2(1.0f, 1.0f) / float2(windowWidth, windowHeight);
		float2 diff = windowPixelSize * (1.0 / 8.0);
		float2 centerCoord = input.texcoord;
		float2 baseCoord = centerCoord - diff * 3.0;

		// Take 16 samples in 4x4 grid
		float3 total = float3(0.0, 0.0, 0.0);
		for (uint y = 0; y < 4; y++) {
			for (uint x = 0; x < 4; x++) {
				float2 coord = baseCoord + float2(diff.x * x, diff.y * y);
				total += max(texture.Sample(texSampler, coord).rgb, float3(0.0, 0.0, 0.0));
			}
		}

		// Average
		linearLightAvg = total / 16.0;
	}

	// Apply gamma correction
	float3 finalResult = applyGammaCorrection(linearLightAvg);

	return float4(finalResult, 1.0);
}
