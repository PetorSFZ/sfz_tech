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

#include <sfz.h>

#include "ZeroG.h"
#include "d3d12/D3D12Common.hpp"

// Uploader
// ------------------------------------------------------------------------------------------------

struct UploaderRange final {
	u64 idx = 0;
	u64 numBytes = 0;
};

struct ZgUploader final {
	SFZ_DECLARE_DROP_TYPE(ZgUploader);

	void destroy()
	{
		if (allocation == nullptr) return;

		allocation->Release();
		resource->Release();

		allocation = nullptr;
		resource = nullptr;
		sizeBytes = 0;
		headIdx = 0;
	}

	UploaderRange allocRange(u64 numBytes)
	{
		// Only allocate 256 aligned ranges
		numBytes = sfzRoundUpAlignedU64(numBytes, 256);

		// Can only allocate at most half of the uploader's backing buffer at once
		// TODO: WHY? This is stupid, disabling for now.
		if (numBytes == 0) return {};
		//if (numBytes >= (sizeBytes / 2)) return {};

		// Try to allocate a range in the buffer. If range is too big and stretches "both ends" of
		// the buffer try again.
		u64 beginIdxInf = headIdx.fetch_add(numBytes);
		u64 beginIdxMapped = beginIdxInf % sizeBytes;
		if ((beginIdxMapped + numBytes) > sizeBytes) {
			beginIdxInf = headIdx.fetch_add(numBytes);
			beginIdxMapped = beginIdxInf % sizeBytes;
			if ((beginIdxMapped + numBytes) > sizeBytes) return {};
		}

		// Check if we have allocated too much memory from uploader
		const u64 safeCompareOffset = (beginIdxInf + numBytes) - sizeBytes;
		if (safeCompareOffset >= safeOffset) {
			const u32 tooManyBytes = u32(safeCompareOffset - safeOffset);
			ZG_ERROR("Allocated too much memory from uploader (off by: %u bytes [%.2f MiB])",
				tooManyBytes, f32(tooManyBytes) / (1024.0f * 1024.0f));
			return {};
		}

		// Return range
		UploaderRange range = {};
		range.idx = beginIdxMapped;
		range.numBytes = numBytes;
		return range;
	}

	ZgResult memcpy(const UploaderRange& dstRange, const void* src, u64 numBytes)
	{
		// Memcpy to buffer
		::memcpy(mappedPtr + dstRange.idx, src, numBytes);
		return ZG_SUCCESS;
	}

	D3D12MA::Allocation* allocation = nullptr;
	ID3D12Resource* resource = nullptr;
	u64 sizeBytes = 0;
	u8* mappedPtr = nullptr;
	std::atomic_uint64_t headIdx = 0;
	u64 safeOffset = 0;
};


inline ZgResult createUploader(
	ZgUploader*& uploaderOut,
	const ZgUploaderDesc& uploaderDesc,
	D3D12MA::Allocator* d3d12Allocator)
{
	// Allocate buffer
	ID3D12Resource* resource = nullptr;
	D3D12MA::Allocation* allocation = nullptr;
	{
		// Fill resource desc
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Alignment = 0;
		resDesc.Width = sfzRoundUpAlignedU64(uploaderDesc.sizeBytes, 256);
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resDesc.Flags = D3D12_RESOURCE_FLAGS(0);

		// Fill allocation desc
		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
		allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE; // Can be ignored
		allocDesc.CustomPool = nullptr;

		// Allocate buffer
		HRESULT res = d3d12Allocator->CreateResource(
			&allocDesc,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			NULL,
			&allocation,
			IID_PPV_ARGS(&resource));
		if (D3D12_FAIL(res)) {
			return  ZG_ERROR_GENERIC;
		}
	}

	// Set debug name if available
	if (uploaderDesc.debugName != nullptr) {
		::setDebugName(resource, uploaderDesc.debugName);
	}

	// Map buffer
	D3D12_RANGE readRange = {};
	readRange.Begin = 0;
	readRange.End = 0;
	void* mappedPtr = nullptr;
	if (D3D12_FAIL(resource->Map(0, &readRange, &mappedPtr))) {
		allocation->Release();
		resource->Release();
		return ZG_ERROR_GENERIC;
	}

	// Allocate uploader and return
	ZgUploader* uploader = sfz_new<ZgUploader>(getAllocator(), sfz_dbg("ZgUploader"));
	uploader->resource = resource;
	uploader->allocation = allocation;
	uploader->mappedPtr = reinterpret_cast<u8*>(mappedPtr);
	uploader->sizeBytes = sfzRoundUpAlignedU64(uploaderDesc.sizeBytes, 256);
	uploader->headIdx = uploader->sizeBytes * 2;
	uploader->safeOffset = uploader->headIdx;
	uploaderOut = uploader;
	return ZG_SUCCESS;
}
