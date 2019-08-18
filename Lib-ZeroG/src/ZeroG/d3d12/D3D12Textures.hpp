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

// Helper functions
// ------------------------------------------------------------------------------------------------

D3D12_RESOURCE_DESC createInfoToResourceDesc(const ZgTexture2DCreateInfo& info) noexcept;

// D3D12 Texture Heap
// ------------------------------------------------------------------------------------------------

class D3D12TextureHeap final : public ZgTextureHeap {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12TextureHeap() = default;
	D3D12TextureHeap(const D3D12TextureHeap&) = delete;
	D3D12TextureHeap& operator= (const D3D12TextureHeap&) = delete;
	D3D12TextureHeap(D3D12TextureHeap&&) = delete;
	D3D12TextureHeap& operator= (D3D12TextureHeap&&) = delete;
	~D3D12TextureHeap() noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode texture2DCreate(
		ZgTexture2D** textureOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept override final;

	// Members
	// --------------------------------------------------------------------------------------------

	ZgLogger logger = {};
	ID3D12Device3* device = nullptr;
	std::atomic_uint64_t* resourceUniqueIdentifierCounter = nullptr;

	uint64_t sizeBytes = 0;
	ComPtr<ID3D12Heap> heap;
	D3DX12Residency::ManagedObject managedObject;
};

// D3D12 Texture Heap functions
// ------------------------------------------------------------------------------------------------

ZgErrorCode createTextureHeap(
	ZgLogger& logger,
	ID3D12Device3& device,
	std::atomic_uint64_t* resourceUniqueIdentifierCounter,
	D3DX12Residency::ResidencyManager& residencyManager,
	D3D12TextureHeap** heapOut,
	const ZgTextureHeapCreateInfo& createInfo) noexcept;

// D3D12 Texture 2D
// ------------------------------------------------------------------------------------------------

class D3D12Texture2D final : public ZgTexture2D {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12Texture2D() = default;
	D3D12Texture2D(const D3D12Texture2D&) = delete;
	D3D12Texture2D& operator= (const D3D12Texture2D&) = delete;
	D3D12Texture2D(D3D12Texture2D&&) = delete;
	D3D12Texture2D& operator= (D3D12Texture2D&&) = delete;
	~D3D12Texture2D() noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	// A unique identifier for this texture
	uint64_t identifier = 0;

	D3D12TextureHeap* textureHeap = nullptr;
	ComPtr<ID3D12Resource> resource;
	ZgTexture2DFormat zgFormat = ZG_TEXTURE_2D_FORMAT_UNDEFINED;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t numMipmaps = 0;

	// Information from ID3D12Device::GetCopyableFootprints()
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprints[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS] = {};
	uint32_t numRows[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS] = {};
	uint64_t rowSizesInBytes[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS] = {};
	uint64_t totalSizeInBytes = 0;

	// The current resource state of the texture. Committed because the state has been committed
	// in a command list which has been executed on a queue. There may be pending state changes
	// in command lists not yet executed.
	// TODO: Mutex protecting this? How handle changes submitted on different queues simulatenously?
	D3D12_RESOURCE_STATES lastCommittedStates[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS] = {};

	// Methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode setDebugName(const char* name) noexcept override final;
};

} // namespace zg
