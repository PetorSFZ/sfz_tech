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

#include "ZeroG/d3d12/D3D12Memory.hpp"

#include "ZeroG/util/Assert.hpp"
#include "ZeroG/util/CpuAllocation.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

static D3D12_HEAP_TYPE bufferMemoryTypeToD3D12HeapType(ZgMemoryType type) noexcept
{
	switch (type) {
	case ZG_MEMORY_TYPE_UPLOAD: return D3D12_HEAP_TYPE_UPLOAD;
	case ZG_MEMORY_TYPE_DOWNLOAD: return D3D12_HEAP_TYPE_READBACK;
	case ZG_MEMORY_TYPE_DEVICE: return D3D12_HEAP_TYPE_DEFAULT;
	}
	ZG_ASSERT(false);
	return D3D12_HEAP_TYPE_DEFAULT;
}

// D3D12MemoryHeap: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12MemoryHeap::~D3D12MemoryHeap() noexcept
{

}

// D3D12MemoryHeap: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12MemoryHeap::bufferCreate(
	IBuffer** bufferOut,
	const ZgBufferCreateInfo& createInfo) noexcept
{
	// Create placed resource
	ComPtr<ID3D12Resource> resource;
	const D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	{
		bool allowUav = memoryType == ZG_MEMORY_TYPE_DEVICE;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		desc.Width = createInfo.sizeInBytes;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags =
			allowUav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAGS(0);

		// Create placed resource
		if (D3D12_FAIL(logger, device->CreatePlacedResource(
			heap.Get(),
			createInfo.offsetInBytes,
			&desc,
			initialResourceState,
			nullptr,
			IID_PPV_ARGS(&resource)))) {
			return ZG_ERROR_GPU_OUT_OF_MEMORY;
		}
	}

	// Allocate buffer
	D3D12Buffer* buffer =
		zgNew<D3D12Buffer>(allocator, "ZeroG - D3D12Buffer");

	// Copy stuff
	buffer->identifier = std::atomic_fetch_add(resourceUniqueIdentifierCounter, 1);
	buffer->memoryHeap = this;
	buffer->sizeBytes = createInfo.sizeInBytes;
	buffer->resource = resource;
	buffer->lastCommittedState = initialResourceState;

	// Return buffer
	*bufferOut = buffer;
	return ZG_SUCCESS;
}

// D3D12 Memory Heap functions
// ------------------------------------------------------------------------------------------------

ZgErrorCode createMemoryHeap(
	ZgLogger& logger,
	ZgAllocator& allocator,
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	D3DX12Residency::ResidencyManager& residencyManager,
	D3D12MemoryHeap** heapOut,
	const ZgMemoryHeapCreateInfo& createInfo) noexcept
{
	// Create heap
	ComPtr<ID3D12Heap> heap;
	{
		bool allowAtomics = createInfo.memoryType == ZG_MEMORY_TYPE_DEVICE;

		D3D12_HEAP_DESC desc = {};
		desc.SizeInBytes = createInfo.sizeInBytes;
		desc.Properties.Type = bufferMemoryTypeToD3D12HeapType(createInfo.memoryType);
		desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		desc.Properties.CreationNodeMask = 0; // No multi-GPU support
		desc.Properties.VisibleNodeMask = 0; // No multi-GPU support
		desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS |
			(allowAtomics ? D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS(0));

		// Create heap
		if (D3D12_FAIL(logger, device.CreateHeap(&desc, IID_PPV_ARGS(&heap)))) {
			return ZG_ERROR_GPU_OUT_OF_MEMORY;
		}
	}

	// Allocate memory heap
	D3D12MemoryHeap* memoryHeap =
		zgNew<D3D12MemoryHeap>(allocator, "ZeroG - D3D12MemoryHeap");

	// Create residency manager object and begin tracking
	memoryHeap->managedObject.Initialize(heap.Get(), createInfo.sizeInBytes);
	residencyManager.BeginTrackingObject(&memoryHeap->managedObject);

	// Copy stuff
	memoryHeap->logger = logger;
	memoryHeap->allocator = allocator;
	memoryHeap->device = &device;
	memoryHeap->resourceUniqueIdentifierCounter = resourceUniqueIdentifierCounter;
	memoryHeap->memoryType = createInfo.memoryType;
	memoryHeap->sizeBytes = createInfo.sizeInBytes;
	memoryHeap->heap = heap;

	// Return heap
	*heapOut = memoryHeap;
	return ZG_SUCCESS;
}

// D3D12Buffer: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12Buffer::~D3D12Buffer() noexcept
{

}

} // namespace zg
