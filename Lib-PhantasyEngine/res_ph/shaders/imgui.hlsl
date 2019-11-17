cbuffer TransformsCB : register(b0) {
	row_major float4x4 projMatrix;
}

struct VSInput {
	float2 position : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
	float4 color : TEXCOORD2;
};

struct VSOutput {
	float2 texcoord : PARAM_0;
	float4 color : PARAM_1;
	float4 position : SV_Position;
};

struct PSInput {
	float2 texcoord : PARAM_0;
	float4 color : PARAM_1;
};

Texture2D fontTexture : register(t0);

SamplerState fontSampler : register(s0);

VSOutput VSMain(VSInput input)
{
	VSOutput output;

	output.texcoord = input.texcoord;
	output.color = input.color;

	output.position = mul(projMatrix, float4(input.position, 0.0f, 1.0f));

	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float fontAlpha = fontTexture.Sample(fontSampler, input.texcoord).r;
	return float4(input.color.rgb, input.color.a * fontAlpha);
}
