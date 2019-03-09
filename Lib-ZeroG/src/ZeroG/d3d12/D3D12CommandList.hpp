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
#include "ZeroG/d3d12/D3D12Memory.hpp"
#include "ZeroG/d3d12/D3D12PipelineRendering.hpp"
#include "ZeroG/BackendInterface.hpp"
#include "ZeroG/util/Vector.hpp"

namespace zg {

// D3D12CommandList
// ------------------------------------------------------------------------------------------------

class D3D12CommandList final : public ICommandList {
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
		ZgLogger logger,
		ZgAllocator allocator,
		ComPtr<ID3D12Device3> device,
		D3DX12Residency::ResidencyManager* residencyManager,
		D3D12DescriptorRingBuffer* descriptorBuffer) noexcept;
	void swap(D3D12CommandList& other) noexcept;
	void destroy() noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode memcpyBufferToBuffer(
		IBuffer* dstBuffer,
		uint64_t dstBufferOffsetBytes,
		IBuffer* srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept override final;

	ZgErrorCode setPushConstant(
		uint32_t shaderRegister,
		const void* data,
		uint32_t dataSizeInBytes) noexcept override final;

	ZgErrorCode bindConstantBuffers(
		const ZgConstantBufferBindings& bindings) noexcept override final;

	ZgErrorCode setPipelineRendering(
		IPipelineRendering* pipeline) noexcept override final;

	ZgErrorCode setFramebuffer(
		const ZgCommandListSetFramebufferInfo& info) noexcept override final;

	ZgErrorCode clearFramebuffer(
		float red,
		float green,
		float blue,
		float alpha) noexcept override final;

	ZgErrorCode setVertexBuffer(
		uint32_t vertexBufferSlot,
		IBuffer* vertexBuffer) noexcept override final;

	ZgErrorCode drawTriangles(
		uint32_t startVertexIndex,
		uint32_t numVertices) noexcept override final;

	// Helper methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode reset() noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	uint64_t fenceValue = 0;

	D3DX12Residency::ResidencySet* residencySet = nullptr;

	Vector<uint64_t> pendingBufferIdentifiers;
	Vector<PendingState> pendingBufferStates;

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode getPendingBufferStates(
		D3D12Buffer& buffer,
		D3D12_RESOURCE_STATES neededState,
		PendingState& pendingStatesOut) noexcept;
	
	// Private members
	// --------------------------------------------------------------------------------------------

	ZgLogger mLog = {};
	ComPtr<ID3D12Device3> mDevice;
	D3DX12Residency::ResidencyManager* mResidencyManager = nullptr;
	D3D12DescriptorRingBuffer* mDescriptorBuffer = nullptr;
	bool mPipelineSet = false; // Only allow a single pipeline per command list
	D3D12PipelineRendering* mBoundPipeline = nullptr;
	bool mFramebufferSet = false; // Only allow a single framebuffer to be set.
	D3D12_CPU_DESCRIPTOR_HANDLE mFramebufferDescriptor = {}; // The descriptor for the set framebuffer
};

} // namespace zg
