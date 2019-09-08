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
#include "ZeroG/d3d12/D3D12DescriptorRingBuffer.hpp"
#include "ZeroG/d3d12/D3D12Framebuffer.hpp"
#include "ZeroG/d3d12/D3D12Buffer.hpp"
#include "ZeroG/d3d12/D3D12PipelineRender.hpp"
#include "ZeroG/BackendInterface.hpp"
#include "ZeroG/util/Vector.hpp"

namespace zg {

// D3D12CommandList
// ------------------------------------------------------------------------------------------------

class D3D12CommandList final : public ZgCommandList {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12CommandList() = default;
	D3D12CommandList(const D3D12CommandList&) = delete;
	D3D12CommandList& operator= (const D3D12CommandList&) = delete;
	D3D12CommandList(D3D12CommandList&& other) noexcept { swap(other); }
	D3D12CommandList& operator= (D3D12CommandList&& other) noexcept { swap(other); return *this; }
	~D3D12CommandList() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void create(
		uint32_t maxNumBuffers,
		ComPtr<ID3D12Device3> device,
		D3DX12Residency::ResidencyManager* residencyManager,
		D3D12DescriptorRingBuffer* descriptorBuffer) noexcept;
	void swap(D3D12CommandList& other) noexcept;
	void destroy() noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode memcpyBufferToBuffer(
		ZgBuffer* dstBuffer,
		uint64_t dstBufferOffsetBytes,
		ZgBuffer* srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept override final;

	ZgErrorCode memcpyToTexture(
		ZgTexture2D* dstTexture,
		uint32_t dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		ZgBuffer* tempUploadBuffer) noexcept override final;

	ZgErrorCode enableQueueTransitionBuffer(ZgBuffer* buffer) noexcept override final;

	ZgErrorCode enableQueueTransitionTexture(ZgTexture2D* texture) noexcept override final;

	ZgErrorCode setPushConstant(
		uint32_t shaderRegister,
		const void* data,
		uint32_t dataSizeInBytes) noexcept override final;

	ZgErrorCode setPipelineBindings(
		const ZgPipelineBindings& bindings) noexcept override final;

	ZgErrorCode setPipelineRender(
		ZgPipelineRender* pipeline) noexcept override final;

	ZgErrorCode setFramebuffer(
		ZgFramebuffer* framebuffer,
		const ZgFramebufferRect* optionalViewport,
		const ZgFramebufferRect* optionalScissor) noexcept override final;

	ZgErrorCode setFramebufferViewport(
		const ZgFramebufferRect& viewport) noexcept override final;

	ZgErrorCode setFramebufferScissor(
		const ZgFramebufferRect& scissor) noexcept override final;

	ZgErrorCode clearFramebufferOptimal() noexcept override final;

	ZgErrorCode clearRenderTargets(
		float red,
		float green,
		float blue,
		float alpha) noexcept override final;

	ZgErrorCode clearDepthBuffer(
		float depth) noexcept override final;

	ZgErrorCode setIndexBuffer(
		ZgBuffer* indexBuffer,
		ZgIndexBufferType type) noexcept override final;

	ZgErrorCode setVertexBuffer(
		uint32_t vertexBufferSlot,
		ZgBuffer* vertexBuffer) noexcept override final;

	ZgErrorCode drawTriangles(
		uint32_t startVertexIndex,
		uint32_t numVertices) noexcept override final;

	ZgErrorCode drawTrianglesIndexed(
		uint32_t startIndex,
		uint32_t numTriangles) noexcept override final;

	// Helper methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode reset() noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	D3D12_COMMAND_LIST_TYPE commandListType;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	uint64_t fenceValue = 0;

	D3DX12Residency::ResidencySet* residencySet = nullptr;

	Vector<uint64_t> pendingBufferIdentifiers;
	Vector<PendingBufferState> pendingBufferStates;

	struct TextureMipIdentifier {
		uint64_t identifier = ~0u;
		uint32_t mipLevel = ~0u;
	};
	Vector<TextureMipIdentifier> pendingTextureIdentifiers;
	Vector<PendingTextureState> pendingTextureStates;

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode getPendingBufferStates(
		D3D12Buffer& buffer,
		D3D12_RESOURCE_STATES neededState,
		PendingBufferState*& pendingStatesOut) noexcept;
	
	ZgErrorCode setBufferState(D3D12Buffer& buffer, D3D12_RESOURCE_STATES targetState) noexcept;

	ZgErrorCode getPendingTextureStates(
		D3D12Texture2D& texture,
		uint32_t mipLevel,
		D3D12_RESOURCE_STATES neededState,
		PendingTextureState*& pendingStatesOut) noexcept;

	ZgErrorCode setTextureState(
		D3D12Texture2D& texture,
		uint32_t mipLevel,
		D3D12_RESOURCE_STATES targetState) noexcept;

	ZgErrorCode setTextureStateAllMipLevels(
		D3D12Texture2D& texture,
		D3D12_RESOURCE_STATES targetState) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	ComPtr<ID3D12Device3> mDevice;
	D3DX12Residency::ResidencyManager* mResidencyManager = nullptr;
	D3D12DescriptorRingBuffer* mDescriptorBuffer = nullptr;
	bool mPipelineSet = false; // Only allow a single pipeline per command list
	D3D12PipelineRender* mBoundPipeline = nullptr;
	bool mFramebufferSet = false; // Only allow a single framebuffer to be set.
	D3D12Framebuffer* mFramebuffer = nullptr;
};

} // namespace zg
