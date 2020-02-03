// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include "ZeroG/d3d12/D3D12Profiler.hpp"

#include "ZeroG/Context.hpp"

namespace zg {

// D3D12Profiler: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12Profiler::~D3D12Profiler() noexcept
{
	MutexAccessor<D3D12ProfilerState> accessor = state.access();
	D3D12ProfilerState& profilerState = accessor.data();

	// Deallocate download heap and buffer
	if (profilerState.downloadHeap != nullptr) {
		sfz_assert(profilerState.downloadBuffer != nullptr);

		// Deallocate buffer
		zg::getAllocator()->deleteObject(profilerState.downloadBuffer);
		profilerState.downloadBuffer = nullptr;

		// Deallocate heap
		zg::getAllocator()->deleteObject(profilerState.downloadHeap);
		profilerState.downloadHeap = nullptr;
	}
}

// D3D12Profiler: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgResult D3D12Profiler::getMeasurement(
	uint64_t measurementId,
	float& measurementMsOut) noexcept
{
	MutexAccessor<D3D12ProfilerState> accessor = state.access();
	D3D12ProfilerState& profilerState = accessor.data();

	// Return invalid argument if measurement id is not valid
	bool validMeasurementId =
		measurementId < profilerState.nextMeasurementId &&
		(measurementId + profilerState.maxNumMeasurements) >= profilerState.nextMeasurementId;
	if (!validMeasurementId) return ZG_ERROR_INVALID_ARGUMENT;

	// Get query idx
	uint32_t queryIdx = measurementId % profilerState.maxNumMeasurements;
	uint32_t timestampBaseIdx = queryIdx * 2;
	uint64_t bufferOffset = timestampBaseIdx * sizeof(uint64_t);

	// Download timestamps
	uint64_t timestamps[2] = {};
	ZgResult memcpyRes =
		profilerState.downloadBuffer->memcpyFrom(bufferOffset, timestamps, sizeof(uint64_t) * 2);
	if (memcpyRes != ZG_SUCCESS) return memcpyRes;

	// Calculate time between timestamps in ms
	float diffSeconds = float(timestamps[1] - timestamps[0]) / float(profilerState.timestampTicksPerSecond);
	measurementMsOut = diffSeconds * 1000.0f;

	return ZG_SUCCESS;
}

// D3D12Profiler functions
// ------------------------------------------------------------------------------------------------

ZgResult d3d12CreateProfiler(
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	D3DX12Residency::ResidencyManager& residencyManager,
	uint64_t timestampTicksPerSecond,
	D3D12Profiler** profilerOut,
	const ZgProfilerCreateInfo& createInfo) noexcept
{
	constexpr uint64_t TIMESTAMPS_PER_MEASUREMENT = 2;

	// Create query heap
	ComPtr<ID3D12QueryHeap> queryHeap;
	{
		D3D12_QUERY_HEAP_DESC desc = {};
		desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		desc.Count = createInfo.maxNumMeasurements * TIMESTAMPS_PER_MEASUREMENT;
		desc.NodeMask = 0;
		bool success = D3D12_SUCC(device.CreateQueryHeap(&desc, IID_PPV_ARGS(&queryHeap)));
		if (!success) return ZG_ERROR_GPU_OUT_OF_MEMORY;
	}

	// Create download buffer
	D3D12MemoryHeap* downloadHeap = nullptr;
	D3D12Buffer* downloadBuffer = nullptr;
	{
		// Create download heap
		ZgMemoryHeapCreateInfo heapInfo = {};
		heapInfo.sizeInBytes =
			sizeof(uint64_t) * TIMESTAMPS_PER_MEASUREMENT * createInfo.maxNumMeasurements;
		heapInfo.memoryType = ZG_MEMORY_TYPE_DOWNLOAD;
		ZgResult res = createMemoryHeap(
			device, resourceUniqueIdentifierCounter, residencyManager, &downloadHeap, heapInfo);
		if (res != ZG_SUCCESS) return res;

		// Create download buffer
		ZgBufferCreateInfo bufferInfo = {};
		bufferInfo.offsetInBytes = 0;
		bufferInfo.sizeInBytes = heapInfo.sizeInBytes;
		ZgBuffer* bufferTmp = nullptr;
		res = downloadHeap->bufferCreate(&bufferTmp, bufferInfo);
		if (res != ZG_SUCCESS) {
			zg::getAllocator()->deleteObject(downloadBuffer);
			return res;
		}
		downloadBuffer = static_cast<D3D12Buffer*>(bufferTmp);
	}
	
	// Allocate profiler
	D3D12Profiler* profiler = getAllocator()->newObject<D3D12Profiler>(sfz_dbg("D3D12Profiler"));

	// Set members
	{
		MutexAccessor<D3D12ProfilerState> accessor = profiler->state.access();
		D3D12ProfilerState& state = accessor.data();
		state.maxNumMeasurements = createInfo.maxNumMeasurements;
		
		state.timestampTicksPerSecond = timestampTicksPerSecond;

		state.queryHeap = queryHeap;

		state.downloadHeap = downloadHeap;
		state.downloadBuffer = downloadBuffer;
	}

	// Return profiler
	*profilerOut = profiler;
	return ZG_SUCCESS;
}

} // namespace zg
