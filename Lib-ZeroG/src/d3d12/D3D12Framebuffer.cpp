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

#include "d3d12/D3D12Framebuffer.hpp"

#include "common/ErrorReporting.hpp"
#include "d3d12/D3D12Memory.hpp"

namespace zg {

// D3D12Framebuffer: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12Framebuffer::~D3D12Framebuffer() noexcept
{

}

// D3D12Framebuffer: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgResult D3D12Framebuffer::getResolution(uint32_t& widthOut, uint32_t& heightOut) const noexcept
{
	widthOut = this->width;
	heightOut = this->height;
	return ZG_SUCCESS;
}

// D3D12 Framebuffer functions
// ------------------------------------------------------------------------------------------------

ZgResult createFramebuffer(
	ID3D12Device3& device,
	D3D12Framebuffer** framebufferOut,
	const ZgFramebufferCreateInfo& createInfo) noexcept
{
	// Get dimensions from first available texture
	uint32_t width = 0;
	uint32_t height = 0;
	if (createInfo.numRenderTargets > 0) {
		D3D12Texture2D* renderTarget = static_cast<D3D12Texture2D*>(createInfo.renderTargets[0]);
		width = renderTarget->width;
		height = renderTarget->height;
	}
	else if (createInfo.depthBuffer != nullptr) {
		D3D12Texture2D* renderTarget = static_cast<D3D12Texture2D*>(createInfo.depthBuffer);
		width = renderTarget->width;
		height = renderTarget->height;
	}
	else {
		sfz_assert(false);
		return ZG_ERROR_INVALID_ARGUMENT;
	}
	sfz_assert(width != 0);
	sfz_assert(height != 0);

	// Check inputs
	for (uint32_t i = 0; i < createInfo.numRenderTargets; i++) {
		ZG_ARG_CHECK(createInfo.renderTargets[i] == nullptr, "");
		D3D12Texture2D* renderTarget = static_cast<D3D12Texture2D*>(createInfo.renderTargets[i]);
		ZG_ARG_CHECK(renderTarget->usage != ZG_TEXTURE_USAGE_RENDER_TARGET,
			"Can only use textures created with the RENDER_TARGET usage flag as render targets");
		ZG_ARG_CHECK(width != renderTarget->width, "All render targets must be same size");
		ZG_ARG_CHECK(height != renderTarget->height, "All render targets must be same size");
		ZG_ARG_CHECK(renderTarget->numMipmaps != 1, "Render targets may not have mipmaps");
	}
	if (createInfo.depthBuffer != nullptr) {
		D3D12Texture2D* depthBuffer = static_cast<D3D12Texture2D*>(createInfo.depthBuffer);
		ZG_ARG_CHECK(depthBuffer->usage != ZG_TEXTURE_USAGE_DEPTH_BUFFER,
			"Can only use textures created with the DEPTH_BUFFER usage flag as depth buffers");
		ZG_ARG_CHECK(width != depthBuffer->width, "All depth buffers must be same size");
		ZG_ARG_CHECK(height != depthBuffer->height, "All depth buffers must be same size");
		ZG_ARG_CHECK(depthBuffer->numMipmaps != 1, "Depth buffers may not have mipmaps");
		ZG_ARG_CHECK(depthBuffer->zgFormat != ZG_TEXTURE_FORMAT_DEPTH_F32, "Depth buffer may only be ZG_TEXTURE_FORMAT_DEPTH_F32 format");
	}

	// Create render target descriptors
	ComPtr<ID3D12DescriptorHeap> descriptorHeapRTV;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorsRTV[ZG_MAX_NUM_RENDER_TARGETS] = {};
	if (createInfo.numRenderTargets > 0) {
		
		// Create descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
		rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvDesc.NumDescriptors = createInfo.numRenderTargets;
		rtvDesc.NodeMask = 0;
		if (D3D12_FAIL(device.CreateDescriptorHeap(
			&rtvDesc, IID_PPV_ARGS(&descriptorHeapRTV)))) {
			return ZG_ERROR_CPU_OUT_OF_MEMORY;
		}

		// Get size of descriptor
		uint32_t descriptorSizeRTV =
			device.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Get first descriptor in heap
		D3D12_CPU_DESCRIPTOR_HANDLE startOfRtvDescriptorHeap =
			descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart();

		// Create render target views (RTVs) for render targets
		for (uint32_t i = 0; i < createInfo.numRenderTargets; i++) {

			// Get texture
			D3D12Texture2D* texture = reinterpret_cast<D3D12Texture2D*>(createInfo.renderTargets[i]);

			// Create render target view description
			D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = texture->format;
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = 0; // TODO: Mipmap index

			// Get descriptor
			D3D12_CPU_DESCRIPTOR_HANDLE descriptor = {};
			descriptor.ptr = startOfRtvDescriptorHeap.ptr + descriptorSizeRTV * i;
			descriptorsRTV[i] = descriptor;

			// Create render target view for i:th render target
			device.CreateRenderTargetView(texture->resource.Get(), &viewDesc, descriptor);
		}
	}

	// Create depth buffer descriptors
	ComPtr<ID3D12DescriptorHeap> descriptorHeapDSV;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorDSV = {};
	if (createInfo.depthBuffer != nullptr) {

		D3D12Texture2D* texture = reinterpret_cast<D3D12Texture2D*>(createInfo.depthBuffer);
		sfz_assert(texture->zgFormat == ZG_TEXTURE_FORMAT_DEPTH_F32);
		sfz_assert(texture->format == DXGI_FORMAT_D32_FLOAT);

		// Create descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
		dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvDesc.NumDescriptors = 1;
		dsvDesc.NodeMask = 0;
		if (D3D12_FAIL(device.CreateDescriptorHeap(
			&dsvDesc, IID_PPV_ARGS(&descriptorHeapDSV)))) {
			return ZG_ERROR_CPU_OUT_OF_MEMORY;
		}

		// Get descriptor
		descriptorDSV = descriptorHeapDSV->GetCPUDescriptorHandleForHeapStart();

		// Create depth buffer view
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvViewDesc = {};
		dsvViewDesc.Format = texture->format;
		dsvViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvViewDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvViewDesc.Texture2D.MipSlice = 0; // TODO: Mipmax index

		device.CreateDepthStencilView(texture->resource.Get(), &dsvViewDesc, descriptorDSV);
	}

	// Allocate framebuffer and copy members
	D3D12Framebuffer* framebuffer =
		getAllocator()->newObject<D3D12Framebuffer>(sfz_dbg("D3D12Framebuffer"));

	framebuffer->width = width;
	framebuffer->height = height;

	framebuffer->numRenderTargets = createInfo.numRenderTargets;
	framebuffer->descriptorHeapRTV = descriptorHeapRTV;
	for (uint32_t i = 0; i < createInfo.numRenderTargets; i++) {
		framebuffer->renderTargets[i] =
			reinterpret_cast<D3D12Texture2D*>(createInfo.renderTargets[i]);
		framebuffer->renderTargetDescriptors[i] = descriptorsRTV[i];
		framebuffer->renderTargetOptimalClearValues[i] = framebuffer->renderTargets[i]->optimalClearValue;
	}

	framebuffer->hasDepthBuffer = createInfo.depthBuffer != nullptr;
	framebuffer->depthBuffer = reinterpret_cast<D3D12Texture2D*>(createInfo.depthBuffer);
	framebuffer->descriptorHeapDSV = descriptorHeapDSV;
	framebuffer->depthBufferDescriptor = descriptorDSV;
	if (framebuffer->depthBuffer != nullptr) {
		framebuffer->depthBufferOptimalClearValue = framebuffer->depthBuffer->optimalClearValue;
	}

	*framebufferOut = framebuffer;
	return ZG_SUCCESS;
}

} // namespace zg
