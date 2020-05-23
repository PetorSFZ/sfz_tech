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
#include "common/ErrorReporting.hpp"
#include "d3d12/D3D12Common.hpp"


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

	ZgResult memcpyUpload(
		uint64_t dstBufferOffsetBytes,
		const void* srcMemory,
		uint64_t numBytes) noexcept
	{
		ZgBuffer& dstBuffer = *this;
		if (dstBuffer.memoryType != ZG_MEMORY_TYPE_UPLOAD) return ZG_ERROR_INVALID_ARGUMENT;

		// Not gonna read from buffer
		D3D12_RANGE readRange = {};
		readRange.Begin = 0;
		readRange.End = 0;

		// Map buffer
		void* mappedPtr = nullptr;
		if (D3D12_FAIL(dstBuffer.resource->Map(0, &readRange, &mappedPtr))) {
			return ZG_ERROR_GENERIC;
		}

		// Memcpy to buffer
		memcpy(reinterpret_cast<uint8_t*>(mappedPtr) + dstBufferOffsetBytes, srcMemory, numBytes);

		// The range we memcpy'd to
		D3D12_RANGE writeRange = {};
		writeRange.Begin = dstBufferOffsetBytes;
		writeRange.End = writeRange.Begin + numBytes;

		// Unmap buffer
		dstBuffer.resource->Unmap(0, &writeRange);

		return ZG_SUCCESS;
	}

	ZgResult memcpyDownload(
		uint64_t srcBufferOffsetBytes,
		void* dstMemory,
		uint64_t numBytes) noexcept
	{
		ZgBuffer& srcBuffer = *this;
		if (srcBuffer.memoryType != ZG_MEMORY_TYPE_DOWNLOAD) return ZG_ERROR_INVALID_ARGUMENT;

		// Specify range which we are going to read from in buffer
		D3D12_RANGE readRange = {};
		readRange.Begin = srcBufferOffsetBytes;
		readRange.End = srcBufferOffsetBytes + numBytes;

		// Map buffer
		void* mappedPtr = nullptr;
		if (D3D12_FAIL(srcBuffer.resource->Map(0, &readRange, &mappedPtr))) {
			return ZG_ERROR_GENERIC;
		}

		// Memcpy to buffer
		memcpy(dstMemory, reinterpret_cast<const uint8_t*>(mappedPtr) + srcBufferOffsetBytes, numBytes);

		// The didn't write anything
		D3D12_RANGE writeRange = {};
		writeRange.Begin = 0;
		writeRange.End = 0;

		// Unmap buffer
		srcBuffer.resource->Unmap(0, &writeRange);

		return ZG_SUCCESS;
	}

	// Members
	// --------------------------------------------------------------------------------------------

	ZgMemoryType memoryType = ZG_MEMORY_TYPE_DEVICE;
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

	ZgResult setDebugName(const char* name) noexcept
	{
		::setDebugName(this->resource, name);
		return ZG_SUCCESS;
	}
};

// Buffer functions
// ------------------------------------------------------------------------------------------------

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
	// TODO: ALLOCATION_FLAG_WITHIN_BUDGET maybe?
	allocationDesc.Flags = createInfo.committedAllocation ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_NONE;
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
	~ZgTexture2D() noexcept
	{
		if (allocation != nullptr) {
			allocation->Release();
			allocation = nullptr;
		}
	}

	// Members
	// --------------------------------------------------------------------------------------------

	D3D12MA::Allocation* allocation = nullptr;
	ComPtr<ID3D12Resource> resource;

	// A unique identifier for this texture
	uint64_t identifier = 0;

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

	ZgResult setDebugName(const char* name) noexcept
	{
		::setDebugName(this->resource, name);
		return ZG_SUCCESS;
	}
};

// Texture functions
// ------------------------------------------------------------------------------------------------

inline ZgResult createTexture(
	ZgTexture2D*& textureOut,
	const ZgTexture2DCreateInfo& createInfo,
	ID3D12Device3& device,
	D3D12MA::Allocator* d3d12Allocator,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter) noexcept
{
	if (createInfo.usage == ZG_TEXTURE_USAGE_DEPTH_BUFFER) {
		ZG_ARG_CHECK(createInfo.format != ZG_TEXTURE_FORMAT_DEPTH_F32,
			"Can only use DEPTH formats for DEPTH_BUFFERs");
	}

	// Create resource desc
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = createInfo.width;
	desc.Height = createInfo.height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = (uint16_t)createInfo.numMipmaps;
	desc.Format = zgToDxgiTextureFormat(createInfo.format);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = [&]() {
		switch (createInfo.usage) {
		case ZG_TEXTURE_USAGE_DEFAULT: return D3D12_RESOURCE_FLAG_NONE;
		case ZG_TEXTURE_USAGE_RENDER_TARGET: return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ;
		case ZG_TEXTURE_USAGE_DEPTH_BUFFER: return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		sfz_assert(false);
		return D3D12_RESOURCE_FLAG_NONE;
	}() | (createInfo.allowUnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);
	// TODO: Maybe expose flags:
	//      * D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS

	// Fill allocation desc
	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags = createInfo.committedAllocation ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_NONE;
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE; // Can be ignored
	allocationDesc.CustomPool = nullptr;

	ComPtr<ID3D12Resource> resource;
	D3D12MA::Allocation* allocation = nullptr;
	HRESULT res = d3d12Allocator->CreateResource(
		&allocationDesc,
		&desc,
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		&allocation,
		IID_PPV_ARGS(&resource));
	if (D3D12_FAIL(res)) {
		return ZG_ERROR_GENERIC;
	}

	// Get the subresource footprint for the texture
	// TODO: One for each mipmap level?
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprints[ZG_MAX_NUM_MIPMAPS] = {};
	uint32_t numRows[ZG_MAX_NUM_MIPMAPS] = {};
	uint64_t rowSizesInBytes[ZG_MAX_NUM_MIPMAPS] = {};
	uint64_t totalSizeInBytes = 0;

	device.GetCopyableFootprints(&desc, 0, createInfo.numMipmaps, allocation->GetOffset(),
		subresourceFootprints, numRows, rowSizesInBytes, &totalSizeInBytes);

	// Allocate texture
	ZgTexture2D* texture = getAllocator()->newObject<ZgTexture2D>(sfz_dbg("ZgTexture2D"));

	// Copy stuff
	texture->allocation = allocation;
	texture->resource = resource;
	
	texture->identifier = std::atomic_fetch_add(resourceUniqueIdentifierCounter, 1);

	texture->zgFormat = createInfo.format;
	texture->usage = createInfo.usage;
	texture->optimalClearValue = createInfo.optimalClearValue;
	texture->format = desc.Format;
	texture->width = createInfo.width;
	texture->height = createInfo.height;
	texture->numMipmaps = createInfo.numMipmaps;

	for (uint32_t i = 0; i < createInfo.numMipmaps; i++) {
		texture->subresourceFootprints[i] = subresourceFootprints[i];
		texture->numRows[i] = numRows[i];
		texture->rowSizesInBytes[i] = rowSizesInBytes[i];
	}
	texture->totalSizeInBytes = totalSizeInBytes;

	for (uint32_t i = 0; i < createInfo.numMipmaps; i++) {
		texture->lastCommittedStates[i] = D3D12_RESOURCE_STATE_COMMON;
	}

	// Return texture
	textureOut = texture;
	return ZG_SUCCESS;
}
