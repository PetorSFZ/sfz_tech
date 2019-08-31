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

static DXGI_FORMAT createInfoToDxgiFormat(const ZgTexture2DCreateInfo& info) noexcept
{
	if (info.normalized == ZG_FALSE) {
		// TODO: This currently seems to be broken. Unormalized textures always return 0 when read
		//       in shader. Should investigate further.
		ZG_ASSERT(false);
		switch (info.format) {
		case ZG_TEXTURE_2D_FORMAT_R_U8: return DXGI_FORMAT_R8_UINT;
		case ZG_TEXTURE_2D_FORMAT_RG_U8: return DXGI_FORMAT_R8G8_UINT;
		case ZG_TEXTURE_2D_FORMAT_RGBA_U8: return DXGI_FORMAT_R8G8B8A8_UINT;
		default:
			break;
		}
	}
	else {
		switch (info.format) {
		case ZG_TEXTURE_2D_FORMAT_R_U8: return DXGI_FORMAT_R8_UNORM;
		case ZG_TEXTURE_2D_FORMAT_RG_U8: return DXGI_FORMAT_R8G8_UNORM;
		case ZG_TEXTURE_2D_FORMAT_RGBA_U8: return DXGI_FORMAT_R8G8B8A8_UNORM;
		default:
			break;
		}
	}

	ZG_ASSERT(false);
	return DXGI_FORMAT_UNKNOWN;
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
	desc.Format = createInfoToDxgiFormat(info);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = [&]() {
		switch (info.mode) {
		case ZG_TEXTURE_MODE_DEFAULT: return D3D12_RESOURCE_FLAG_NONE;
		case ZG_TEXTURE_MODE_RENDER_TARGET: return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		case ZG_TEXTURE_MODE_DEPTH_BUFFER: return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
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
	if (this->memoryType == ZG_MEMORY_TYPE_TEXTURE) return ZG_ERROR_INVALID_ARGUMENT;
	if (this->memoryType == ZG_MEMORY_TYPE_FRAMEBUFFER) return ZG_ERROR_INVALID_ARGUMENT;

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
	if (this->memoryType == ZG_MEMORY_TYPE_UPLOAD) return ZG_ERROR_INVALID_ARGUMENT;
	if (this->memoryType == ZG_MEMORY_TYPE_DOWNLOAD) return ZG_ERROR_INVALID_ARGUMENT;
	if (this->memoryType == ZG_MEMORY_TYPE_DEVICE) return ZG_ERROR_INVALID_ARGUMENT;
	if (this->memoryType == ZG_MEMORY_TYPE_TEXTURE) {
		if (createInfo.mode != ZG_TEXTURE_MODE_DEFAULT) return ZG_ERROR_INVALID_ARGUMENT;
	}
	if (this->memoryType == ZG_MEMORY_TYPE_FRAMEBUFFER) {
		if (createInfo.mode == ZG_TEXTURE_MODE_DEFAULT) return ZG_ERROR_INVALID_ARGUMENT;
	}

	// Get resource desc
	D3D12_RESOURCE_DESC desc = createInfoToResourceDesc(createInfo);

	// Get allocation info
	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo =
		device->GetResourceAllocationInfo(0, 1, &desc);

	// Create placed resource
	const D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON;
	ComPtr<ID3D12Resource> resource;
	if (D3D12_FAIL(device->CreatePlacedResource(
		heap.Get(),
		createInfo.offsetInBytes,
		&desc,
		initialResourceState,
		nullptr,
		IID_PPV_ARGS(&resource)))) {
		return ZG_ERROR_GPU_OUT_OF_MEMORY;
	}

	// Get the subresource footprint for the texture
	// TODO: One for each mipmap level?
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprints[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS] = {};
	uint32_t numRows[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS] = {};
	uint64_t rowSizesInBytes[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS] = {};
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
