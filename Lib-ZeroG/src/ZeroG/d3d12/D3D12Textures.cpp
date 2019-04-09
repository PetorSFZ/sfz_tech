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

#include "ZeroG/d3d12/D3D12Textures.hpp"

#include "ZeroG/util/Assert.hpp"
#include "ZeroG/util/CpuAllocation.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

static DXGI_FORMAT createInfoToDxgiFormat(const ZgTexture2DCreateInfo& info) noexcept
{
	if (info.normalized == ZG_TRUE) {
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

static D3D12_RESOURCE_DESC createInfoToResourceDesc(const ZgTexture2DCreateInfo& info) noexcept
{
	D3D12_RESOURCE_DESC desc = {};

	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = info.width;
	desc.Height = info.height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1; // TODO: Currently no mips I think. Expose this.
	desc.Format = createInfoToDxgiFormat(info);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	// TODO: Maybe expose flags:
	//      * D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	//      * D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS

	return desc;
}

// D3D12TextureHeap: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12TextureHeap::~D3D12TextureHeap() noexcept
{

}

// D3D12TextureHeap: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12TextureHeap::texture2DGetAllocationInfo(
	ZgTexture2DAllocationInfo& allocationInfoOut,
	const ZgTexture2DCreateInfo& createInfo) noexcept
{
	// Get resource desc
	D3D12_RESOURCE_DESC desc = createInfoToResourceDesc(createInfo);

	// Get allocation info
	D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);

	// Return allocation info
	allocationInfoOut.sizeInBytes = allocInfo.SizeInBytes;
	allocationInfoOut.alignmentInBytes = allocInfo.Alignment;
	return ZG_SUCCESS;
}

ZgErrorCode D3D12TextureHeap::texture2DCreate(
	ITexture2D** textureOut,
	const ZgTexture2DCreateInfo& createInfo) noexcept
{
	// Get resource desc
	D3D12_RESOURCE_DESC desc = createInfoToResourceDesc(createInfo);

	// Get allocation info
	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo =
		device->GetResourceAllocationInfo(0, 1, &desc);

	// Create placed resource
	const D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	ComPtr<ID3D12Resource> resource;
	if (D3D12_FAIL(logger, device->CreatePlacedResource(
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
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprint = {};
	uint32_t numRows = 0;
	uint64_t rowSizeInBytes = 0;
	uint64_t totalSizeInBytes = 0;

	device->GetCopyableFootprints(&desc, 0, 1, createInfo.offsetInBytes,
		&subresourceFootprint, &numRows, &rowSizeInBytes, &totalSizeInBytes);

	// Allocate texture
	D3D12Texture2D* texture =
		zgNew<D3D12Texture2D>(allocator, "ZeroG - D3D12Texture");

	// Copy stuff
	texture->identifier = std::atomic_fetch_add(resourceUniqueIdentifierCounter, 1);

	texture->textureHeap = this;
	texture->resource = resource;
	texture->zgFormat = createInfo.format;
	texture->format = desc.Format;
	texture->width = createInfo.width;
	texture->height = createInfo.height;

	texture->subresourceFootprint = subresourceFootprint;
	texture->numRows = numRows;
	texture->rowSizeInBytes = rowSizeInBytes;
	texture->totalSizeInBytes = totalSizeInBytes;

	texture->lastCommittedState = initialResourceState;

	// Return texture
	*textureOut = texture;
	return ZG_SUCCESS;
}

ZgErrorCode D3D12TextureHeap::texture2DRelease(
	ITexture2D* texture) noexcept
{
	zgDelete<ITexture2D>(allocator, texture);
	return ZG_SUCCESS;
}

// D3D12 Texture Heap functions
// ------------------------------------------------------------------------------------------------

ZgErrorCode createTextureHeap(
	ZgLogger& logger,
	ZgAllocator& allocator,
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	D3DX12Residency::ResidencyManager& residencyManager,
	D3D12TextureHeap** heapOut,
	const ZgTextureHeapCreateInfo& createInfo) noexcept
{
	// Create heap
	ComPtr<ID3D12Heap> heap;
	{
		D3D12_HEAP_DESC desc = {};
		desc.SizeInBytes = createInfo.sizeInBytes;
		desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		desc.Properties.CreationNodeMask = 0; // No multi-GPU support
		desc.Properties.VisibleNodeMask = 0; // No multi-GPU support
		desc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT; // 4 MiB alignment;
		desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;

		// Create heap
		if (D3D12_FAIL(logger, device.CreateHeap(&desc, IID_PPV_ARGS(&heap)))) {
			return ZG_ERROR_GPU_OUT_OF_MEMORY;
		}
	}

	// Allocate texture heap
	D3D12TextureHeap* textureHeap =
		zgNew<D3D12TextureHeap>(allocator, "ZeroG - D3D12TextureHeap");

	// Create residency manager object and begin tracking
	textureHeap->managedObject.Initialize(heap.Get(), createInfo.sizeInBytes);
	residencyManager.BeginTrackingObject(&textureHeap->managedObject);

	// Copy stuff
	textureHeap->logger = logger;
	textureHeap->allocator = allocator;
	textureHeap->device = &device;
	textureHeap->resourceUniqueIdentifierCounter = resourceUniqueIdentifierCounter;
	textureHeap->sizeBytes = createInfo.sizeInBytes;
	textureHeap->heap = heap;

	// Return heap
	*heapOut = textureHeap;
	return ZG_SUCCESS;
}

// D3D12 Texture 2D
// ------------------------------------------------------------------------------------------------

D3D12Texture2D::~D3D12Texture2D() noexcept
{

}

} // namespace zg
