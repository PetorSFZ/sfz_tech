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

#include "ZeroG.h"
#include "ZeroG/d3d12/D3D12Common.hpp"
#include "ZeroG/BackendInterface.hpp"

namespace zg {

// D3D12 Texture Heap
// ------------------------------------------------------------------------------------------------

class D3D12TextureHeap final : public ITextureHeap {
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

	ZgErrorCode texture2DGetAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept override final;

	ZgErrorCode texture2DCreate(
		ITexture2D** textureOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept override final;

	ZgErrorCode texture2DRelease(
		ITexture2D* texture) noexcept override final;

	// Members
	// --------------------------------------------------------------------------------------------

	ZgLogger logger = {};
	ZgAllocator allocator = {};
	ID3D12Device3* device = nullptr;

	uint64_t sizeBytes = 0;
	ComPtr<ID3D12Heap> heap;
	D3DX12Residency::ManagedObject managedObject;
};

// D3D12 Texture Heap functions
// ------------------------------------------------------------------------------------------------

ZgErrorCode createTextureHeap(
	ZgLogger& logger,
	ZgAllocator& allocator,
	ID3D12Device3& device,
	D3DX12Residency::ResidencyManager& residencyManager,
	D3D12TextureHeap** heapOut,
	const ZgTextureHeapCreateInfo& createInfo) noexcept;

// D3D12 Texture 2D
// ------------------------------------------------------------------------------------------------

class D3D12Texture2D final : public ITexture2D {
public:
	D3D12Texture2D() = default;
	D3D12Texture2D(const D3D12Texture2D&) = delete;
	D3D12Texture2D& operator= (const D3D12Texture2D&) = delete;
	D3D12Texture2D(D3D12Texture2D&&) = delete;
	D3D12Texture2D& operator= (D3D12Texture2D&&) = delete;
	~D3D12Texture2D() noexcept;

	D3D12TextureHeap* textureHeap = nullptr;
	ComPtr<ID3D12Resource> resource;
};


} // namespace zg
