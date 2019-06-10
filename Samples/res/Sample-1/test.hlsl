struct Transforms {
	row_major float4x4 modelViewProjMatrix;
	row_major float4x4 normalMatrix;
};
ConstantBuffer<Transforms> transforms : register(b0);

cbuffer AnotherCB : register(b1) {
	float offsets;
	float __padding0;
	float __padding1;
	float __padding2;
};

Texture2D texture : register(t0);

SamplerState textureSampler : register(s0);

struct VSInput {
	float3 position : ATTRIBUTE_LOCATION_0;
	float3 normal : ATTRIBUTE_LOCATION_1;
	float2 texcoord : ATTRIBUTE_LOCATION_2;
};

struct VSOutput {
	float3 normal : PARAM_0;
	float2 texcoord : PARAM_1;
	float4 position : SV_Position;
};

struct PSInput {
	float3 normal : PARAM_0;
	float2 texcoord : PARAM_1;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output;

	float4 offsetPos = float4(input.position.x + offsets, input.position.yz, 1.0f);
	output.position = mul(transforms.modelViewProjMatrix, offsetPos);

	output.normal = normalize(mul(transforms.normalMatrix, float4(input.normal, 0.0f))).xyz;
	output.texcoord = input.texcoord;

	//output.position = float4(input.position.x + offsets, input.position.yz, 1.0f);
	//output.color = float4(input.color, 1.0);
	//output.color = float4(input.color.xy, AConstantBuffer.someMatrix._m00, 1.0f);

	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	// Get texture dimensions and number of mip levels
	uint texWidth = 0;
	uint texHeight = 0;
	uint texNumMipLevels = 0;
	texture.GetDimensions(0, texWidth, texHeight, texNumMipLevels);

	// Calculate texture integer coordinate
	int coordX = (texWidth - 1) * input.texcoord.x;
	int coordY = (texHeight - 1) * input.texcoord.y;
	uint2 coord = uint2(coordX, coordY);

	return float4(texture.Sample(textureSampler, input.texcoord).xyz, 1.0);

	// Small hack to ensure sampler is used
	//float4 val = float4(texture.Sample(textureSampler, input.texcoord).xyz, 1.0);
	//return float4(texture.mips[1][coord].xyz, 1.0) * 0.75 + val * 0.25;
}
