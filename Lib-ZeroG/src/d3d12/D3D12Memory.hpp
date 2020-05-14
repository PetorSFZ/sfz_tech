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
	~ZgMemoryHeap() noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult bufferCreate(
		ZgBuffer** bufferOut,
		const ZgBufferCreateInfo& createInfo) noexcept;

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
	D3DX12Residency::ManagedObject managedObject;
};

// D3D12 Memory Heap functions
// ------------------------------------------------------------------------------------------------

ZgResult createMemoryHeap(
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	D3DX12Residency::ResidencyManager& residencyManager,
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
	~ZgBuffer() noexcept {}

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

	// A unique identifier for this buffer
	uint64_t identifier = 0;

	ZgMemoryHeap* memoryHeap = nullptr;
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
