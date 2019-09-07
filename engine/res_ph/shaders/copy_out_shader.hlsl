// Copyright 2019 Peter Hillerström and skipifzero, all rights reserved.

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
	return float4(texture.Sample(texSampler, input.texcoord).rgb, 1.0);
	//return float4(texture.Sample(texSampler, input.texcoord).rgb, 1.0) * 0.001 + float4(input.texcoord, 0.0, 0.0);
}
