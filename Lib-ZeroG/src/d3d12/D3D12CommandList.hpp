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

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>

#include "ZeroG.h"
#include "common/BackendInterface.hpp"
#include "d3d12/D3D12Common.hpp"
#include "d3d12/D3D12DescriptorRingBuffer.hpp"
#include "d3d12/D3D12Framebuffer.hpp"
#include "d3d12/D3D12Memory.hpp"
#include "d3d12/D3D12Pipelines.hpp"

namespace zg {

class D3D12CommandQueue;

// PendingState struct
// ------------------------------------------------------------------------------------------------

// Struct representing the pending state for a buffer in a command list
struct PendingBufferState final {

	// The associated D3D12Buffer
	D3D12Buffer* buffer = nullptr;

	// The state the resource need to be in before the command list is executed
	D3D12_RESOURCE_STATES neededInitialState = D3D12_RESOURCE_STATE_COMMON;

	// The state the resource is in after the command list is executed
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
};

struct PendingTextureState final {

	// The associated D3D12Texture
	D3D12Texture2D* texture = nullptr;

	// The mip level of the associated texture
	uint32_t mipLevel = ~0u;

	// The state the resource need to be in before the command list is executed
	D3D12_RESOURCE_STATES neededInitialState = D3D12_RESOURCE_STATE_COMMON;

	// The state the resource is in after the command list is executed
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
};

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
		D3D12CommandQueue* queue,
		uint32_t maxNumBuffers,
		ComPtr<ID3D12Device3> device,
		D3DX12Residency::ResidencyManager* residencyManager,
		D3D12DescriptorRingBuffer* descriptorBuffer) noexcept;
	void swap(D3D12CommandList& other) noexcept;
	void destroy() noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult memcpyBufferToBuffer(
		ZgBuffer* dstBuffer,
		uint64_t dstBufferOffsetBytes,
		ZgBuffer* srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept override final;

	ZgResult memcpyToTexture(
		ZgTexture2D* dstTexture,
		uint32_t dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		ZgBuffer* tempUploadBuffer) noexcept override final;

	ZgResult enableQueueTransitionBuffer(ZgBuffer* buffer) noexcept override final;

	ZgResult enableQueueTransitionTexture(ZgTexture2D* texture) noexcept override final;

	ZgResult setPushConstant(
		uint32_t shaderRegister,
		const void* data,
		uint32_t dataSizeInBytes) noexcept override final;

	ZgResult setPipelineBindings(
		const ZgPipelineBindings& bindings) noexcept override final;

	ZgResult setPipelineCompute(
		ZgPipelineCompute* pipeline) noexcept override final;

	ZgResult unorderedBarrierBuffer(
		ZgBuffer* buffer) noexcept override final;

	ZgResult unorderedBarrierTexture(
		ZgTexture2D* texture) noexcept override final;

	ZgResult unorderedBarrierAll() noexcept override final;

	ZgResult dispatchCompute(
		uint32_t groupCountX,
		uint32_t groupCountY,
		uint32_t groupCountZ) noexcept override final;

	ZgResult setPipelineRender(
		ZgPipelineRender* pipeline) noexcept override final;

	ZgResult setFramebuffer(
		ZgFramebuffer* framebuffer,
		const ZgFramebufferRect* optionalViewport,
		const ZgFramebufferRect* optionalScissor) noexcept override final;

	ZgResult setFramebufferViewport(
		const ZgFramebufferRect& viewport) noexcept override final;

	ZgResult setFramebufferScissor(
		const ZgFramebufferRect& scissor) noexcept override final;

	ZgResult clearFramebufferOptimal() noexcept override final;

	ZgResult clearRenderTargets(
		float red,
		float green,
		float blue,
		float alpha) noexcept override final;

	ZgResult clearDepthBuffer(
		float depth) noexcept override final;

	ZgResult setIndexBuffer(
		ZgBuffer* indexBuffer,
		ZgIndexBufferType type) noexcept override final;

	ZgResult setVertexBuffer(
		uint32_t vertexBufferSlot,
		ZgBuffer* vertexBuffer) noexcept override final;

	ZgResult drawTriangles(
		uint32_t startVertexIndex,
		uint32_t numVertices) noexcept override final;

	ZgResult drawTrianglesIndexed(
		uint32_t startIndex,
		uint32_t numTriangles) noexcept override final;

	ZgResult profileBegin(
		ZgProfiler* profilerIn,
		uint64_t& measurementIdOut) noexcept override final;

	ZgResult profileEnd(
		ZgProfiler* profilerIn,
		uint64_t measurementId) noexcept override final;

	// Helper methods
	// --------------------------------------------------------------------------------------------

	ZgResult reset() noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	D3D12CommandQueue* queue = nullptr;
	D3D12_COMMAND_LIST_TYPE commandListType;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	uint64_t fenceValue = 0;

	D3DX12Residency::ResidencySet* residencySet = nullptr;

	sfz::Array<uint64_t> pendingBufferIdentifiers;
	sfz::Array<PendingBufferState> pendingBufferStates;

	struct TextureMipIdentifier {
		uint64_t identifier = ~0u;
		uint32_t mipLevel = ~0u;
	};
	sfz::Array<TextureMipIdentifier> pendingTextureIdentifiers;
	sfz::Array<PendingTextureState> pendingTextureStates;

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	ZgResult getPendingBufferStates(
		D3D12Buffer& buffer,
		D3D12_RESOURCE_STATES neededState,
		PendingBufferState*& pendingStatesOut) noexcept;
	
	ZgResult setBufferState(D3D12Buffer& buffer, D3D12_RESOURCE_STATES targetState) noexcept;

	ZgResult getPendingTextureStates(
		D3D12Texture2D& texture,
		uint32_t mipLevel,
		D3D12_RESOURCE_STATES neededState,
		PendingTextureState*& pendingStatesOut) noexcept;

	ZgResult setTextureState(
		D3D12Texture2D& texture,
		uint32_t mipLevel,
		D3D12_RESOURCE_STATES targetState) noexcept;

	ZgResult setTextureStateAllMipLevels(
		D3D12Texture2D& texture,
		D3D12_RESOURCE_STATES targetState) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	ComPtr<ID3D12Device3> mDevice;
	D3DX12Residency::ResidencyManager* mResidencyManager = nullptr;
	D3D12DescriptorRingBuffer* mDescriptorBuffer = nullptr;
	bool mPipelineSet = false; // Only allow a single pipeline per command list
	D3D12PipelineRender* mBoundPipelineRender = nullptr;
	D3D12PipelineCompute* mBoundPipelineCompute = nullptr;
	bool mFramebufferSet = false; // Only allow a single framebuffer to be set.
	D3D12Framebuffer* mFramebuffer = nullptr;
};

} // namespace zg
