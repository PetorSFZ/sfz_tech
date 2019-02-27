/*struct TmpConstantBuffer {
	matrix someMatrix;
};*/

//ConstantBuffer<TmpConstantBuffer> AConstantBuffer : register(b0);

cbuffer PushConstantCB : register(b0) {
	float4 pushColor;
};

cbuffer AnotherCB : register(b1) {
	float offset;
};

struct VSInput {
	float3 position : ATTRIBUTE_LOCATION_0;
	float3 color : ATTRIBUTE_LOCATION_1;
};

struct VSOutput {
	float4 color : COLOR;
	float4 position : SV_Position;
};

struct PSInput {
	float4 color : COLOR;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output;

	output.position = float4(input.position.x + offset, input.position.yz, 1.0f);
	output.color = float4(input.color, 1.0);
	//output.color = float4(input.color.xy, AConstantBuffer.someMatrix._m00, 1.0f);

	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	//return float4(0.0, 0.0, 1.0, 1.0);
	return input.color + pushColor;
}
