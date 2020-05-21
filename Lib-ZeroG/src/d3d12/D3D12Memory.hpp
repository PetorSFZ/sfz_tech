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

#include <D3D12MemAlloc.h>

#include "ZeroG.h"
#include "d3d12/D3D12Common.hpp"


// Helper functions
// ------------------------------------------------------------------------------------------------

D3D12_RESOURCE_DESC createInfoToResourceDesc(const ZgTexture2DCreateInfo& info) noexcept;

// ZgMemoryHeap
// ------------------------------------------------------------------------------------------------

struct ZgMemoryHeap final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ZgMemoryHeap() = default;
	ZgMemoryHeap(const ZgMemoryHeap&) = delete;
	ZgMemoryHeap& operator= (const ZgMemoryHeap&) = delete;
	ZgMemoryHeap(ZgMemoryHeap&&) = delete;
	ZgMemoryHeap& operator= (ZgMemoryHeap&&) = delete;
	~ZgMemoryHeap() noexcept
	{

	}

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult texture2DCreate(
		ZgTexture2D** textureOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	ID3D12Device3* device = nullptr;
	std::atomic_uint64_t* resourceUniqueIdentifierCounter = nullptr;

	ZgMemoryType memoryType = ZG_MEMORY_TYPE_UNDEFINED;
	uint64_t sizeBytes = 0;
	ComPtr<ID3D12Heap> heap;
};

// D3D12 Memory Heap functions
// ------------------------------------------------------------------------------------------------

ZgResult createMemoryHeap(
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	ZgMemoryHeap** heapOut,
	const ZgMemoryHeapCreateInfo& createInfo) noexcept;

// D3D12 Buffer
// ------------------------------------------------------------------------------------------------

struct ZgBuffer final {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ZgBuffer() = default;
	ZgBuffer(const ZgBuffer&) = delete;
	ZgBuffer& operator= (const ZgBuffer&) = delete;
	ZgBuffer(ZgBuffer&&) = delete;
	ZgBuffer& operator= (ZgBuffer&&) = delete;
	~ZgBuffer() noexcept
	{
		if (allocation != nullptr) {
			allocation->Release();
			allocation = nullptr;
		}
	}

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult memcpyTo(
		uint64_t dstBufferOffsetBytes,
		const void* srcMemory,
		uint64_t numBytes) noexcept;

	ZgResult memcpyFrom(
		uint64_t srcBufferOffsetBytes,
		void* dstMemory,
		uint64_t numBytes) noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	ZgMemoryType memoryType = ZG_MEMORY_TYPE_UNDEFINED;
	D3D12MA::Allocation* allocation = nullptr;

	// A unique identifier for this buffer
	uint64_t identifier = 0;

	uint64_t sizeBytes = 0;
	ComPtr<ID3D12Resource> resource;

	// The current resource state of the buffer. Committed because the state has been committed
	// in a command list which has been executed on a queue. There may be pending state changes
	// in command lists not yet executed.
	// TODO: Mutex protecting this? How handle changes submitted on different queues simulatenously?
	D3D12_RESOURCE_STATES lastCommittedState = D3D12_RESOURCE_STATE_COMMON;

	// Methods
	// --------------------------------------------------------------------------------------------

	ZgResult setDebugName(const char* name) noexcept;
};

inline ZgResult createBuffer(
	ZgBuffer*& bufferOut,
	const ZgBufferCreateInfo& createInfo,
	D3D12MA::Allocator* d3d12Allocator,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter) noexcept
{
	D3D12_RESOURCE_STATES initialResourceState = [&]() {
		switch (createInfo.memoryType) {
		case ZG_MEMORY_TYPE_UPLOAD: return D3D12_RESOURCE_STATE_GENERIC_READ;
		case ZG_MEMORY_TYPE_DOWNLOAD: return D3D12_RESOURCE_STATE_COPY_DEST;
		case ZG_MEMORY_TYPE_DEVICE: return D3D12_RESOURCE_STATE_COMMON;
		}
		sfz_assert(false);
		return D3D12_RESOURCE_STATE_COMMON;
	}();

	bool allowUav = createInfo.memoryType == ZG_MEMORY_TYPE_DEVICE;

	// Fill resource desc
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = sfz::roundUpAligned(createInfo.sizeInBytes, 256);
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags =
		allowUav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAGS(0);

	// Fill allocation desc
	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;// D3D12MA::ALLOCATION_FLAG_COMMITTED; // ALLOCATION_FLAG_WITHIN_BUDGET
	allocationDesc.HeapType = [&]() {
		switch (createInfo.memoryType) {
		case ZG_MEMORY_TYPE_UPLOAD: return D3D12_HEAP_TYPE_UPLOAD;
		case ZG_MEMORY_TYPE_DOWNLOAD: return D3D12_HEAP_TYPE_READBACK;
		case ZG_MEMORY_TYPE_DEVICE: return D3D12_HEAP_TYPE_DEFAULT;
		}
		sfz_assert_hard(false);
	}();
	allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE; // Can be ignored
	allocationDesc.CustomPool = nullptr;

	ComPtr<ID3D12Resource> resource;
	D3D12MA::Allocation* allocation = nullptr;
	HRESULT res = d3d12Allocator->CreateResource(
		&allocationDesc,
		&desc,
		initialResourceState,
		NULL,
		&allocation,
		IID_PPV_ARGS(&resource));
	if (D3D12_FAIL(res)) {
		return  ZG_ERROR_GENERIC;
	}

	// Allocate buffer
	ZgBuffer* buffer = getAllocator()->newObject<ZgBuffer>(sfz_dbg("ZgBuffer"));

	// Copy stuff
	buffer->memoryType = createInfo.memoryType;
	buffer->allocation = allocation;
	buffer->identifier = std::atomic_fetch_add(resourceUniqueIdentifierCounter, 1);
	buffer->sizeBytes = sfz::roundUpAligned(createInfo.sizeInBytes, 256);
	buffer->resource = resource;
	buffer->lastCommittedState = initialResourceState;

	// Return buffer
	bufferOut = buffer;
	return ZG_SUCCESS;
}

// ZgTexture2D
// ------------------------------------------------------------------------------------------------

struct ZgTexture2D final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ZgTexture2D() = default;
	ZgTexture2D(const ZgTexture2D&) = delete;
	ZgTexture2D& operator= (const ZgTexture2D&) = delete;
	ZgTexture2D(ZgTexture2D&&) = delete;
	ZgTexture2D& operator= (ZgTexture2D&&) = delete;
	~ZgTexture2D() noexcept {}

	// Members
	// --------------------------------------------------------------------------------------------

	// A unique identifier for this texture
	uint64_t identifier = 0;

	ZgMemoryHeap* textureHeap = nullptr;
	ComPtr<ID3D12Resource> resource;
	ZgTextureFormat zgFormat = ZG_TEXTURE_FORMAT_UNDEFINED;
	ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT;
	ZgOptimalClearValue optimalClearValue = ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t numMipmaps = 0;

	// Information from ID3D12Device::GetCopyableFootprints()
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprints[ZG_MAX_NUM_MIPMAPS] = {};
	uint32_t numRows[ZG_MAX_NUM_MIPMAPS] = {};
	uint64_t rowSizesInBytes[ZG_MAX_NUM_MIPMAPS] = {};
	uint64_t totalSizeInBytes = 0;

	// The current resource state of the texture. Committed because the state has been committed
	// in a command list which has been executed on a queue. There may be pending state changes
	// in command lists not yet executed.
	// TODO: Mutex protecting this? How handle changes submitted on different queues simulatenously?
	D3D12_RESOURCE_STATES lastCommittedStates[ZG_MAX_NUM_MIPMAPS] = {};

	// Methods
	// --------------------------------------------------------------------------------------------

	ZgResult setDebugName(const char* name) noexcept;
};
