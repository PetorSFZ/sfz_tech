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
#include "common/BackendInterface.hpp"
#include "d3d12/D3D12Common.hpp"
#include "d3d12/D3D12Memory.hpp"

// D3D12Framebuffer
// ------------------------------------------------------------------------------------------------

struct SwapchainBacking {
	ComPtr<ID3D12Resource> renderTarget;
	ComPtr<ID3D12Resource> depthBuffer;
};

struct ZgFramebuffer final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ZgFramebuffer() noexcept = default;
	ZgFramebuffer(const ZgFramebuffer&) = delete;
	ZgFramebuffer& operator= (const ZgFramebuffer&) = delete;
	ZgFramebuffer(ZgFramebuffer&&) = delete;
	ZgFramebuffer& operator= (ZgFramebuffer&&) = delete;
	~ZgFramebuffer() noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	// Legacy framebuffer
	bool swapchainFramebuffer = false;
	SwapchainBacking swapchain;

	// Dimensions
	uint32_t width = 0;
	uint32_t height = 0;

	// Render targets
	uint32_t numRenderTargets = 0;
	D3D12Texture2D* renderTargets[ZG_MAX_NUM_RENDER_TARGETS] = {};
	ComPtr<ID3D12DescriptorHeap> descriptorHeapRTV;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDescriptors[ZG_MAX_NUM_RENDER_TARGETS] = {};
	ZgOptimalClearValue renderTargetOptimalClearValues[ZG_MAX_NUM_RENDER_TARGETS] = {};

	// Depth buffer
	bool hasDepthBuffer = false;
	D3D12Texture2D* depthBuffer = nullptr;
	ComPtr<ID3D12DescriptorHeap> descriptorHeapDSV;
	D3D12_CPU_DESCRIPTOR_HANDLE depthBufferDescriptor = {};
	ZgOptimalClearValue depthBufferOptimalClearValue = ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult getResolution(uint32_t& widthOut, uint32_t& heightOut) const noexcept;
};

// D3D12 Framebuffer functions
// ------------------------------------------------------------------------------------------------

ZgResult createFramebuffer(
	ID3D12Device3& device,
	ZgFramebuffer** framebufferOut,
	const ZgFramebufferCreateInfo& createInfo) noexcept;
