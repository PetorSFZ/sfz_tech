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

#include <atomic>

#include "ZeroG.h"
#include "ZeroG/d3d12/D3D12Buffer.hpp"
#include "ZeroG/d3d12/D3D12Common.hpp"
#include "ZeroG/d3d12/D3D12Textures.hpp"
#include "ZeroG/BackendInterface.hpp"

namespace zg {

// Helper functions
// ------------------------------------------------------------------------------------------------

D3D12_RESOURCE_DESC createInfoToResourceDesc(const ZgTexture2DCreateInfo& info) noexcept;

// D3D12 Memory Heap
// ------------------------------------------------------------------------------------------------

class D3D12MemoryHeap final : public ZgMemoryHeap {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12MemoryHeap() = default;
	D3D12MemoryHeap(const D3D12MemoryHeap&) = delete;
	D3D12MemoryHeap& operator= (const D3D12MemoryHeap&) = delete;
	D3D12MemoryHeap(D3D12MemoryHeap&&) = delete;
	D3D12MemoryHeap& operator= (D3D12MemoryHeap&&) = delete;
	~D3D12MemoryHeap() noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult bufferCreate(
		ZgBuffer** bufferOut,
		const ZgBufferCreateInfo& createInfo) noexcept override final;

	ZgResult texture2DCreate(
		ZgTexture2D** textureOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept override final;

	// Members
	// --------------------------------------------------------------------------------------------

	ID3D12Device3* device = nullptr;
	std::atomic_uint64_t* resourceUniqueIdentifierCounter = nullptr;

	ZgMemoryType memoryType = ZG_MEMORY_TYPE_UNDEFINED;
	uint64_t sizeBytes = 0;
	ComPtr<ID3D12Heap> heap;
	D3DX12Residency::ManagedObject managedObject;
};

// D3D12 Memory Heap functions
// ------------------------------------------------------------------------------------------------

ZgResult createMemoryHeap(
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	D3DX12Residency::ResidencyManager& residencyManager,
	D3D12MemoryHeap** heapOut,
	const ZgMemoryHeapCreateInfo& createInfo) noexcept;

} // namespace zg
