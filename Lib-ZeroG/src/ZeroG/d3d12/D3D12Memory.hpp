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
#include "ZeroG/d3d12/D3D12Common.hpp"
#include "ZeroG/BackendInterface.hpp"

namespace zg {

// D3D12 Memory Heap
// ------------------------------------------------------------------------------------------------

class D3D12MemoryHeap final : public IMemoryHeap {
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

	ZgErrorCode bufferCreate(
		IBuffer** bufferOut,
		const ZgBufferCreateInfo& createInfo) noexcept override final;

	ZgErrorCode bufferRelease(
		IBuffer* buffer) noexcept override final;

	// Members
	// --------------------------------------------------------------------------------------------

	ZgLogger logger = {};
	ZgAllocator allocator = {};
	ID3D12Device3* device = nullptr;
	std::atomic_uint64_t* resourceUniqueIdentifierCounter = nullptr;

	ZgMemoryType memoryType = ZG_MEMORY_TYPE_UNDEFINED;
	uint64_t sizeBytes = 0;
	ComPtr<ID3D12Heap> heap;
	D3DX12Residency::ManagedObject managedObject;
};

// D3D12 Memory Heap functions
// ------------------------------------------------------------------------------------------------

ZgErrorCode createMemoryHeap(
	ZgLogger& logger,
	ZgAllocator& allocator,
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	D3DX12Residency::ResidencyManager& residencyManager,
	D3D12MemoryHeap** heapOut,
	const ZgMemoryHeapCreateInfo& createInfo) noexcept;

// D3D12 Buffer
// ------------------------------------------------------------------------------------------------

class D3D12Buffer final : public IBuffer {
public:
	D3D12Buffer() = default;
	D3D12Buffer(const D3D12Buffer&) = delete;
	D3D12Buffer& operator= (const D3D12Buffer&) = delete;
	D3D12Buffer(D3D12Buffer&&) = delete;
	D3D12Buffer& operator= (D3D12Buffer&&) = delete;
	~D3D12Buffer() noexcept;

	// A unique identifier for this buffer
	uint64_t identifier = 0;

	D3D12MemoryHeap* memoryHeap = nullptr;
	uint64_t sizeBytes = 0;
	ComPtr<ID3D12Resource> resource;

	// The current resource state of the buffer. Committed because the state has been committed
	// in a command list which has been executed on a queue. There may be pending state changes
	// in command lists not yet executed.
	// TODO: Mutex protecting this? How handle changes submitted on different queues simulatenously?
	D3D12_RESOURCE_STATES lastCommittedState = D3D12_RESOURCE_STATE_COMMON;
};

} // namespace zg
