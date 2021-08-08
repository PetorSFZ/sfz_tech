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

#pragma once

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_new.hpp>

#include "common/Mutex.hpp"
#include "d3d12/D3D12Common.hpp"
#include "d3d12/D3D12Memory.hpp"

// ZgProfiler
// ------------------------------------------------------------------------------------------------

struct D3D12ProfilerState final {

	uint64_t nextMeasurementId = 0;
	uint32_t maxNumMeasurements = 0;

	sfz::Array<uint64_t> ticksPerSecond;

	ComPtr<ID3D12QueryHeap> queryHeap;

	ZgBuffer* downloadBuffer = nullptr;
};

struct ZgProfiler final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ZgProfiler() noexcept = default;
	ZgProfiler(const ZgProfiler&) = delete;
	ZgProfiler& operator= (const ZgProfiler&) = delete;
	ZgProfiler(ZgProfiler&&) = delete;
	ZgProfiler& operator= (ZgProfiler&&) = delete;
	~ZgProfiler() noexcept
	{
		MutexAccessor<D3D12ProfilerState> accessor = state.access();
		D3D12ProfilerState& profilerState = accessor.data();

		// Deallocate download buffer
		if (profilerState.downloadBuffer != nullptr) {
			// Deallocate buffer
			sfz_delete(getAllocator(), profilerState.downloadBuffer);
			profilerState.downloadBuffer = nullptr;
		}
	}

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult getMeasurement(
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
			bufferMemcpyDownload(*profilerState.downloadBuffer, bufferOffset, timestamps, sizeof(uint64_t) * 2);
		if (memcpyRes != ZG_SUCCESS) return memcpyRes;

		// Get number of ticks per second when this query was issued
		uint64_t ticksPerSecond = profilerState.ticksPerSecond[queryIdx];

		// Calculate time between timestamps in ms
		float diffSeconds = float(timestamps[1] - timestamps[0]) / float(ticksPerSecond);
		measurementMsOut = diffSeconds * 1000.0f;

		return ZG_SUCCESS;
	}

	// Members
	// --------------------------------------------------------------------------------------------

	Mutex<D3D12ProfilerState> state;
};

// ZgProfiler functions
// ------------------------------------------------------------------------------------------------

inline ZgResult d3d12CreateProfiler(
	ID3D12Device3& device,
	D3D12MA::Allocator* d3d12allocator,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	ZgProfiler** profilerOut,
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
	ZgBuffer* downloadBuffer = nullptr;
	{
		// Create download buffer
		ZgBufferCreateInfo bufferInfo = {};
		bufferInfo.memoryType = ZG_MEMORY_TYPE_DOWNLOAD;
		bufferInfo.sizeInBytes = sizeof(uint64_t) * TIMESTAMPS_PER_MEASUREMENT * createInfo.maxNumMeasurements;
		ZgBuffer* bufferTmp = nullptr;
		ZgResult res = createBuffer(bufferTmp, bufferInfo, d3d12allocator, resourceUniqueIdentifierCounter);
		if (res != ZG_SUCCESS) {
			sfz_delete(getAllocator(), downloadBuffer);
			return res;
		}
		downloadBuffer = bufferTmp;
	}

	// Allocate profiler
	ZgProfiler* profiler = sfz_new<ZgProfiler>(getAllocator(), sfz_dbg("ZgProfiler"));

	// Set members
	{
		MutexAccessor<D3D12ProfilerState> accessor = profiler->state.access();
		D3D12ProfilerState& state = accessor.data();
		state.maxNumMeasurements = createInfo.maxNumMeasurements;

		state.ticksPerSecond.init(
			createInfo.maxNumMeasurements, getAllocator(), sfz_dbg("ZgProfiler::ticksPerSecond"));
		state.ticksPerSecond.add(0ull, createInfo.maxNumMeasurements);

		state.queryHeap = queryHeap;

		state.downloadBuffer = downloadBuffer;
	}

	// Return profiler
	*profilerOut = profiler;
	return ZG_SUCCESS;
}

