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

#include <skipifzero.hpp>
#include <skipifzero_new.hpp>

#include "ZeroG.h"
#include "common/ErrorReporting.hpp"
#include "d3d12/D3D12Common.hpp"
#include "d3d12/D3D12ResourceTrackingState.hpp"

// General D3D12 Resource
// ------------------------------------------------------------------------------------------------

struct ZgResource final {
	SFZ_DECLARE_DROP_TYPE(ZgResource);
	
	void destroy() {
		if (allocation == nullptr) return;
		
		allocation->Release();
		resource->Release();
		
		allocation = nullptr;
		resource = nullptr;
	}
	
	D3D12MA::Allocation* allocation = nullptr;
	ID3D12Resource* resource = nullptr;
};

// Buffer
// ------------------------------------------------------------------------------------------------

struct ZgBuffer final {
	ZgResource resource;
	ZgTrackerResourceState tracking;

	ZgMemoryType memoryType = ZG_MEMORY_TYPE_DEVICE;
	u64 sizeBytes = 0;

	// A unique identifier for this buffer
	u64 identifier = 0;
};

// Texture
// ------------------------------------------------------------------------------------------------

struct ZgTexture final {
	ZgResource resource;
	sfz::ArrayLocal<ZgTrackerResourceState, ZG_MAX_NUM_MIPMAPS> mipTrackings;

	// A unique identifier for this texture
	u64 identifier = 0;

	ZgTextureFormat zgFormat = ZG_TEXTURE_FORMAT_UNDEFINED;
	ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT;
	ZgOptimalClearValue optimalClearValue = ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	u32 width = 0;
	u32 height = 0;
	u32 numMipmaps = 0;

	// Information from ID3D12Device::GetCopyableFootprints()
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprints[ZG_MAX_NUM_MIPMAPS] = {};
	u32 numRows[ZG_MAX_NUM_MIPMAPS] = {};
	u64 rowSizesInBytes[ZG_MAX_NUM_MIPMAPS] = {};
	u64 totalSizeInBytes = 0;
};

// Buffer functions
// ------------------------------------------------------------------------------------------------

inline ZgResult createBuffer(
	ZgBuffer*& bufferOut,
	const ZgBufferDesc& createInfo,
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

	ID3D12Resource* resource = nullptr;
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

	// Set debug name if available
	if (createInfo.debugName != nullptr) {
		::setDebugName(resource, createInfo.debugName);
	}

	// Allocate buffer
	ZgBuffer* buffer = sfz_new<ZgBuffer>(getAllocator(), sfz_dbg("ZgBuffer"));

	// Copy stuff
	buffer->resource.resource = resource;
	buffer->resource.allocation = allocation;
	buffer->tracking.lastCommittedState = initialResourceState;
	buffer->memoryType = createInfo.memoryType;
	buffer->sizeBytes = createInfo.sizeInBytes;
	buffer->identifier = std::atomic_fetch_add(resourceUniqueIdentifierCounter, 1);

	// Return buffer
	bufferOut = buffer;
	return ZG_SUCCESS;
}

inline ZgResult bufferMemcpyUpload(
	ZgBuffer& dstBuffer,
	u64 dstBufferOffsetBytes,
	const void* srcMemory,
	u64 numBytes) noexcept
{
	if (dstBuffer.memoryType != ZG_MEMORY_TYPE_UPLOAD) return ZG_ERROR_INVALID_ARGUMENT;

	// Not gonna read from buffer
	D3D12_RANGE readRange = {};
	readRange.Begin = 0;
	readRange.End = 0;

	// Map buffer
	void* mappedPtr = nullptr;
	if (D3D12_FAIL(dstBuffer.resource.resource->Map(0, &readRange, &mappedPtr))) {
		return ZG_ERROR_GENERIC;
	}

	// Memcpy to buffer
	memcpy(reinterpret_cast<u8*>(mappedPtr) + dstBufferOffsetBytes, srcMemory, numBytes);

	// The range we memcpy'd to
	D3D12_RANGE writeRange = {};
	writeRange.Begin = dstBufferOffsetBytes;
	writeRange.End = writeRange.Begin + numBytes;

	// Unmap buffer
	dstBuffer.resource.resource->Unmap(0, &writeRange);

	return ZG_SUCCESS;
}

inline ZgResult bufferMemcpyDownload(
	ZgBuffer& srcBuffer,
	u64 srcBufferOffsetBytes,
	void* dstMemory,
	u64 numBytes) noexcept
{
	if (srcBuffer.memoryType != ZG_MEMORY_TYPE_DOWNLOAD) return ZG_ERROR_INVALID_ARGUMENT;

	// Specify range which we are going to read from in buffer
	D3D12_RANGE readRange = {};
	readRange.Begin = srcBufferOffsetBytes;
	readRange.End = srcBufferOffsetBytes + numBytes;

	// Map buffer
	void* mappedPtr = nullptr;
	if (D3D12_FAIL(srcBuffer.resource.resource->Map(0, &readRange, &mappedPtr))) {
		return ZG_ERROR_GENERIC;
	}

	// Memcpy to buffer
	memcpy(dstMemory, reinterpret_cast<const u8*>(mappedPtr) + srcBufferOffsetBytes, numBytes);

	// The didn't write anything
	D3D12_RANGE writeRange = {};
	writeRange.Begin = 0;
	writeRange.End = 0;

	// Unmap buffer
	srcBuffer.resource.resource->Unmap(0, &writeRange);

	return ZG_SUCCESS;
}

// Texture functions
// ------------------------------------------------------------------------------------------------

inline ZgResult createTexture(
	ZgTexture*& textureOut,
	const ZgTextureDesc& createInfo,
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
	desc.MipLevels = (u16)createInfo.numMipmaps;
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

	// Optimal clear value
	D3D12_CLEAR_VALUE clearValue = {};
	D3D12_CLEAR_VALUE* optimalClearValue = nullptr;
	if (createInfo.optimalClearValue != ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED) {
		f32 value = (createInfo.optimalClearValue == ZG_OPTIMAL_CLEAR_VALUE_ZERO) ? 0.0f : 1.0f;
		clearValue.Format = desc.Format;
		if (createInfo.usage == ZG_TEXTURE_USAGE_RENDER_TARGET) {
			clearValue.Color[0] = value;
			clearValue.Color[1] = value;
			clearValue.Color[2] = value;
			clearValue.Color[3] = value;
		}
		else if (createInfo.usage == ZG_TEXTURE_USAGE_DEPTH_BUFFER) {
			clearValue.DepthStencil.Depth = value;
			clearValue.DepthStencil.Stencil = 0;
		}
		optimalClearValue = &clearValue;
	}

	// Fill allocation desc
	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags = createInfo.committedAllocation ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_NONE;
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE; // Can be ignored
	allocationDesc.CustomPool = nullptr;

	ID3D12Resource* resource = nullptr;
	D3D12MA::Allocation* allocation = nullptr;
	HRESULT res = d3d12Allocator->CreateResource(
		&allocationDesc,
		&desc,
		D3D12_RESOURCE_STATE_COMMON,
		optimalClearValue,
		&allocation,
		IID_PPV_ARGS(&resource));
	if (D3D12_FAIL(res)) {
		return ZG_ERROR_GENERIC;
	}

	// Get the subresource footprint for the texture
	// TODO: One for each mipmap level?
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprints[ZG_MAX_NUM_MIPMAPS] = {};
	u32 numRows[ZG_MAX_NUM_MIPMAPS] = {};
	u64 rowSizesInBytes[ZG_MAX_NUM_MIPMAPS] = {};
	u64 totalSizeInBytes = 0;

	device.GetCopyableFootprints(&desc, 0, createInfo.numMipmaps, allocation->GetOffset(),
		subresourceFootprints, numRows, rowSizesInBytes, &totalSizeInBytes);

	// Set debug name if available
	if (createInfo.debugName != nullptr) {
		::setDebugName(resource, createInfo.debugName);
	}

	// Allocate texture
	ZgTexture* texture = sfz_new<ZgTexture>(getAllocator(), sfz_dbg("ZgTexture"));

	// Copy stuff
	texture->resource.allocation = allocation;
	texture->resource.resource = resource;

	for (u32 i = 0; i < createInfo.numMipmaps; i++) {
		texture->mipTrackings.add().lastCommittedState = D3D12_RESOURCE_STATE_COMMON;
	}
	
	texture->identifier = std::atomic_fetch_add(resourceUniqueIdentifierCounter, 1);

	texture->zgFormat = createInfo.format;
	texture->usage = createInfo.usage;
	texture->optimalClearValue = createInfo.optimalClearValue;
	texture->format = desc.Format;
	texture->width = createInfo.width;
	texture->height = createInfo.height;
	texture->numMipmaps = createInfo.numMipmaps;

	for (u32 i = 0; i < createInfo.numMipmaps; i++) {
		texture->subresourceFootprints[i] = subresourceFootprints[i];
		texture->numRows[i] = numRows[i];
		texture->rowSizesInBytes[i] = rowSizesInBytes[i];
	}
	texture->totalSizeInBytes = totalSizeInBytes;

	// Return texture
	textureOut = texture;
	return ZG_SUCCESS;
}
