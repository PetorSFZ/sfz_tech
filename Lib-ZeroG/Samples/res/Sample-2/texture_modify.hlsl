RWTexture2D<float4> tex : register(u0);

[numthreads(64, 1, 1)]
void mainCS(
	uint3 groupIdx : SV_GroupID, // Index of group
	uint groupFlatIdx : SV_GroupIndex, // Flattened version of group index
	uint3 groupThreadIdx : SV_GroupThreadID, // Index of thread within group
	uint3 dispatchThreadIdx : SV_DispatchThreadID) // Global index of thread
{
	// Get texture dimensions
	uint texWidth = 0;
	uint texHeight = 0;
	tex.GetDimensions(texWidth, texHeight);

	// Get texture index and exit if out of range
	uint2 idx = dispatchThreadIdx.xy;
	if (idx.x >= texWidth || idx.y >= texHeight) return;

	// Read previous value
	float4 value = tex[idx];

	// Skip "white" rows
	if (value.x >= 0.99 && value.y >= 0.99 && value.z >= 0.99) return;

	// Modify value
	value.x += 0.01;
	if (value.x > 1.0) value.x = 0.0;

	// Write value
	tex[idx] = value;
}
