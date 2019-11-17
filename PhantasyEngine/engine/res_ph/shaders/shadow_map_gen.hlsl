// Copyright 2019 Peter Hillerström and skipifzero, all rights reserved.

#include "res_ph/shaders/common.hlsl"

// Matrices input
// ------------------------------------------------------------------------------------------------

cbuffer StaticMatricesBuffer : register(b0) {
	row_major float4x4 projMatrix;
}

cbuffer DynamicMatricesBuffer : register(b1) {
	DynamicMatrices dynamicMatrices;
}

// Vertex shader
// ------------------------------------------------------------------------------------------------

struct VSInput {
	float3 position : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float2 texcoord : TEXCOORD2;
};

struct VSOutput {
	float4 position : SV_Position;
};

VSOutput VSMain(VSInput input)
{
	float4 tmpPosVS = mul(dynamicMatrices.modelViewMatrix, float4(input.position, 1.0));
	VSOutput output;
	output.position = mul(projMatrix, tmpPosVS);
	return output;
}

// Pixel shader
// ------------------------------------------------------------------------------------------------

void PSMain()
{
}
