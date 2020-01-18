cbuffer AnotherCB : register(b0) {
	uint numVectors;
	uint __padding0;
	uint __padding1;
	uint __padding2;
};

RWStructuredBuffer<float4> srcBuffer : register(u0);
RWStructuredBuffer<float4> dstBuffer : register(u1);

[numthreads(64, 1, 1)]
void mainCS(
	uint3 groupIdx : SV_GroupID, // Index of group
	uint groupFlatIdx : SV_GroupIndex, // Flattened version of group index
	uint3 groupThreadIdx : SV_GroupThreadID, // Index of thread within group
	uint3 dispatchThreadIdx : SV_DispatchThreadID) // Global index of thread
{
	// These should be equivalent
	uint idx = dispatchThreadIdx.x;
	//uint idx = groupIdx.x * 64 + groupThreadIdx.x;

	// Exit if out of range
	if (idx >= numVectors) return;

	// Read from source buffer and write to dest buffer
	float4 value = srcBuffer[idx];
	dstBuffer[idx] = value;
}
