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

#include "common/BackendInterface.hpp"
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

	D3D12MemoryHeap* downloadHeap = nullptr;
	D3D12Buffer* downloadBuffer = nullptr;
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
	~ZgProfiler() noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult getMeasurement(
		uint64_t measurementId,
		float& measurementMsOut) noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	Mutex<D3D12ProfilerState> state;
};

// ZgProfiler functions
// ------------------------------------------------------------------------------------------------

ZgResult d3d12CreateProfiler(
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	D3DX12Residency::ResidencyManager& residencyManager,
	ZgProfiler** profilerOut,
	const ZgProfilerCreateInfo& createInfo) noexcept;
