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

#include "ZeroG/d3d12/D3D12MemoryHeap.hpp"

#include "ZeroG/util/Assert.hpp"
#include "ZeroG/util/CpuAllocation.hpp"
#include "ZeroG/util/ErrorReporting.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

static D3D12_HEAP_TYPE bufferMemoryTypeToD3D12HeapType(ZgMemoryType type) noexcept
{
	switch (type) {
	case ZG_MEMORY_TYPE_UPLOAD: return D3D12_HEAP_TYPE_UPLOAD;
	case ZG_MEMORY_TYPE_DOWNLOAD: return D3D12_HEAP_TYPE_READBACK;
	case ZG_MEMORY_TYPE_DEVICE: return D3D12_HEAP_TYPE_DEFAULT;
	case ZG_MEMORY_TYPE_TEXTURE: return D3D12_HEAP_TYPE_DEFAULT;
	case ZG_MEMORY_TYPE_FRAMEBUFFER: return D3D12_HEAP_TYPE_DEFAULT;
	}
	ZG_ASSERT(false);
	return D3D12_HEAP_TYPE_DEFAULT;
}

static const char* memoryTypeToString(ZgMemoryType type) noexcept
{
	switch (type) {
	case ZG_MEMORY_TYPE_UPLOAD: return "UPLOAD";
	case ZG_MEMORY_TYPE_DOWNLOAD: return "DOWNLOAD";
	case ZG_MEMORY_TYPE_DEVICE: return "DEVICE";
	case ZG_MEMORY_TYPE_TEXTURE: return "TEXTURE";
	case ZG_MEMORY_TYPE_FRAMEBUFFER: return "FRAMEBUFFER";
	}
	ZG_ASSERT(false);
	return "<UNKNOWN>";
}

// Helper functions
// ------------------------------------------------------------------------------------------------

D3D12_RESOURCE_DESC createInfoToResourceDesc(const ZgTexture2DCreateInfo& info) noexcept
{
	D3D12_RESOURCE_DESC desc = {};

	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = info.width;
	desc.Height = info.height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = (uint16_t)info.numMipmaps;
	desc.Format = zgToDxgiTextureFormat(info.format);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = [&]() {
		switch (info.usage) {
		case ZG_TEXTURE_USAGE_DEFAULT: return D3D12_RESOURCE_FLAG_NONE;
		case ZG_TEXTURE_USAGE_RENDER_TARGET: return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		case ZG_TEXTURE_USAGE_DEPTH_BUFFER: return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		ZG_ASSERT(false);
		return D3D12_RESOURCE_FLAG_NONE;
	}();
	// TODO: Maybe expose flags:
	//      * D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	//      * D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS

	return desc;
}

// D3D12MemoryHeap: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12MemoryHeap::~D3D12MemoryHeap() noexcept
{

}

// D3D12MemoryHeap: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12MemoryHeap::bufferCreate(
	ZgBuffer** bufferOut,
	const ZgBufferCreateInfo& createInfo) noexcept
{
	ZG_ARG_CHECK(this->memoryType == ZG_MEMORY_TYPE_TEXTURE, "Can't allocate buffers from TEXTURE heap");
	ZG_ARG_CHECK(this->memoryType == ZG_MEMORY_TYPE_FRAMEBUFFER, "Can't allocate buffers from FRAMEBUFFER heap");

	// Create placed resource
	ComPtr<ID3D12Resource> resource;
	D3D12_RESOURCE_STATES initialResourceState = [&]() {
		switch (memoryType) {
		case ZG_MEMORY_TYPE_UPLOAD: return D3D12_RESOURCE_STATE_GENERIC_READ;
		case ZG_MEMORY_TYPE_DOWNLOAD: return D3D12_RESOURCE_STATE_COPY_DEST;
		case ZG_MEMORY_TYPE_DEVICE: return D3D12_RESOURCE_STATE_COMMON;
		}
		ZG_ASSERT(false);
		return D3D12_RESOURCE_STATE_COMMON;
	}();
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
		if (D3D12_FAIL(device->CreatePlacedResource(
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
	D3D12Buffer* buffer = zgNew<D3D12Buffer>("ZeroG - D3D12Buffer");

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

ZgErrorCode D3D12MemoryHeap::texture2DCreate(
	ZgTexture2D** textureOut,
	const ZgTexture2DCreateInfo& createInfo) noexcept
{
	ZG_ARG_CHECK(this->memoryType == ZG_MEMORY_TYPE_UPLOAD, "Can't allocate textures from UPLOAD heap");
	ZG_ARG_CHECK(this->memoryType == ZG_MEMORY_TYPE_DOWNLOAD, "Can't allocate textures from DOWNLOAD heap");
	ZG_ARG_CHECK(this->memoryType == ZG_MEMORY_TYPE_DEVICE, "Can't allocate textures from DEVICE heap");
	if (this->memoryType == ZG_MEMORY_TYPE_TEXTURE) {
		ZG_ARG_CHECK(createInfo.usage != ZG_TEXTURE_USAGE_DEFAULT,
			"Can only allocate textures with DEFAULT usage from TEXTURE heap");
	}
	if (this->memoryType == ZG_MEMORY_TYPE_FRAMEBUFFER) {
		ZG_ARG_CHECK(createInfo.usage == ZG_TEXTURE_USAGE_DEFAULT,
			"Can't allocate textures with DEFAULT usage from FRAMEBUFFER heap");
	}
	if (createInfo.usage == ZG_TEXTURE_USAGE_DEPTH_BUFFER) {
		ZG_ARG_CHECK(createInfo.format != ZG_TEXTURE_FORMAT_DEPTH_F32,
			"Can only use DEPTH formats for DEPTH_BUFFERs");
	}

	// Get resource desc
	D3D12_RESOURCE_DESC desc = createInfoToResourceDesc(createInfo);

	// Get allocation info
	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo =
		device->GetResourceAllocationInfo(0, 1, &desc);

	// Optimal clear value
	D3D12_CLEAR_VALUE clearValue = {};
	D3D12_CLEAR_VALUE* optimalClearValue = nullptr;
	if (createInfo.optimalClearValue != ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED) {
		float value = (createInfo.optimalClearValue == ZG_OPTIMAL_CLEAR_VALUE_ZERO) ? 0.0f : 1.0f;
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

	// Create placed resource
	const D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON;
	ComPtr<ID3D12Resource> resource;
	if (D3D12_FAIL(device->CreatePlacedResource(
		heap.Get(),
		createInfo.offsetInBytes,
		&desc,
		initialResourceState,
		optimalClearValue,
		IID_PPV_ARGS(&resource)))) {
		return ZG_ERROR_GPU_OUT_OF_MEMORY;
	}

	// Get the subresource footprint for the texture
	// TODO: One for each mipmap level?
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprints[ZG_MAX_NUM_MIPMAPS] = {};
	uint32_t numRows[ZG_MAX_NUM_MIPMAPS] = {};
	uint64_t rowSizesInBytes[ZG_MAX_NUM_MIPMAPS] = {};
	uint64_t totalSizeInBytes = 0;

	device->GetCopyableFootprints(&desc, 0, createInfo.numMipmaps, createInfo.offsetInBytes,
		subresourceFootprints, numRows, rowSizesInBytes, &totalSizeInBytes);

	// Allocate texture
	D3D12Texture2D* texture = zgNew<D3D12Texture2D>("ZeroG - D3D12Texture");

	// Copy stuff
	texture->identifier = std::atomic_fetch_add(resourceUniqueIdentifierCounter, 1);

	texture->textureHeap = this;
	texture->resource = resource;
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
		texture->lastCommittedStates[i] = initialResourceState;
	}

	// Return texture
	*textureOut = texture;
	return ZG_SUCCESS;
}

// D3D12 Memory Heap functions
// ------------------------------------------------------------------------------------------------

ZgErrorCode createMemoryHeap(
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	D3DX12Residency::ResidencyManager& residencyManager,
	D3D12MemoryHeap** heapOut,
	const ZgMemoryHeapCreateInfo& createInfo) noexcept
{
	// Create heap
	ComPtr<ID3D12Heap> heap;
	{
		D3D12_HEAP_FLAGS flags = [&]() {
			switch (createInfo.memoryType) {
			case ZG_MEMORY_TYPE_UPLOAD: return D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			case ZG_MEMORY_TYPE_DOWNLOAD: return D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			case ZG_MEMORY_TYPE_DEVICE:
				return D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS | D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS;
			case ZG_MEMORY_TYPE_TEXTURE: return D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
			case ZG_MEMORY_TYPE_FRAMEBUFFER: return D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			default: break;
			}
			ZG_ASSERT(false);
			return D3D12_HEAP_FLAGS(0);
		}();

		D3D12_HEAP_DESC desc = {};
		desc.SizeInBytes = createInfo.sizeInBytes;
		desc.Properties.Type = bufferMemoryTypeToD3D12HeapType(createInfo.memoryType);
		desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		desc.Properties.CreationNodeMask = 0; // No multi-GPU support
		desc.Properties.VisibleNodeMask = 0; // No multi-GPU support
		desc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT; // 4 MiB alignment
		desc.Flags = flags;

		// Create heap
		if (D3D12_FAIL(device.CreateHeap(&desc, IID_PPV_ARGS(&heap)))) {
			return ZG_ERROR_GPU_OUT_OF_MEMORY;
		}
	}

	// Allocate memory heap
	D3D12MemoryHeap* memoryHeap = zgNew<D3D12MemoryHeap>("ZeroG - D3D12MemoryHeap");

	// Create residency manager object and begin tracking
	memoryHeap->managedObject.Initialize(heap.Get(), createInfo.sizeInBytes);
	residencyManager.BeginTrackingObject(&memoryHeap->managedObject);

	// Copy stuff
	memoryHeap->device = &device;
	memoryHeap->resourceUniqueIdentifierCounter = resourceUniqueIdentifierCounter;
	memoryHeap->memoryType = createInfo.memoryType;
	memoryHeap->sizeBytes = createInfo.sizeInBytes;
	memoryHeap->heap = heap;

	// Log that we created a memory heap
	if (createInfo.sizeInBytes < 1024) {
		ZG_INFO("Allocated memory heap (%s) of size: %u bytes",
			memoryTypeToString(createInfo.memoryType), createInfo.sizeInBytes);
	}
	else if (createInfo.sizeInBytes < (1024 * 1024)) {
		ZG_INFO("Allocated memory heap (%s) of size: %.2f KiB",
			memoryTypeToString(createInfo.memoryType), createInfo.sizeInBytes / (1024.0f));
	}
	else {
		ZG_INFO("Allocated memory heap (%s) of size: %.2f MiB",
			memoryTypeToString(createInfo.memoryType), createInfo.sizeInBytes / (1024.0f * 1024.0f));
	}

	// Return heap
	*heapOut = memoryHeap;
	return ZG_SUCCESS;
}

} // namespace zg
