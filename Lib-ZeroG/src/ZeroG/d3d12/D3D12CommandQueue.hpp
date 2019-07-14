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

#include <mutex>

#include "ZeroG.h"
#include "ZeroG/d3d12/D3D12Common.hpp"
#include "ZeroG/d3d12/D3D12CommandList.hpp"
#include "ZeroG/d3d12/D3D12DescriptorRingBuffer.hpp"
#include "ZeroG/d3d12/D3D12Textures.hpp"
#include "ZeroG/util/RingBuffer.hpp"
#include "ZeroG/util/Vector.hpp"
#include "ZeroG/BackendInterface.hpp"

namespace zg {

// D3D12Fence
// ------------------------------------------------------------------------------------------------

class D3D12CommandQueue;

class D3D12Fence final : public ZgFence {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12Fence() noexcept = default;
	D3D12Fence(const D3D12Fence&) = delete;
	D3D12Fence& operator= (const D3D12Fence&) = delete;
	D3D12Fence(D3D12Fence&&) = delete;
	D3D12Fence& operator= (D3D12Fence&&) = delete;
	~D3D12Fence() noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	uint64_t fenceValue = 0;
	D3D12CommandQueue* commandQueue = nullptr;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode reset() noexcept override final;
	ZgErrorCode checkIfSignaled(bool& fenceSignaledOut) const noexcept override final;
	ZgErrorCode waitOnCpuBlocking() const noexcept override final;
};

// D3D12CommandQueue
// ------------------------------------------------------------------------------------------------

class D3D12CommandQueue final : public ZgCommandQueue {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12CommandQueue() noexcept = default;
	D3D12CommandQueue(const D3D12CommandQueue&) = delete;
	D3D12CommandQueue& operator= (const D3D12CommandQueue&) = delete;
	D3D12CommandQueue(D3D12CommandQueue&&) = delete;
	D3D12CommandQueue& operator= (D3D12CommandQueue&&) = delete;
	~D3D12CommandQueue() noexcept;

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode create(
		D3D12_COMMAND_LIST_TYPE type,
		ComPtr<ID3D12Device3>& device,
		D3DX12Residency::ResidencyManager* residencyManager,
		D3D12DescriptorRingBuffer* descriptorBuffer,
		uint32_t maxNumCommandLists,
		uint32_t maxNumBuffersPerCommandList,
		ZgLogger logger,
		ZgAllocator allocator) noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode signalOnGpu(ZgFence& fenceToSignal) noexcept override final;
	ZgErrorCode waitOnGpu(const ZgFence& fence) noexcept override final;
	ZgErrorCode flush() noexcept override final;
	ZgErrorCode beginCommandListRecording(ZgCommandList** commandListOut) noexcept override final;
	ZgErrorCode executeCommandList(ZgCommandList* commandList) noexcept override final;

	// Synchronization methods
	// --------------------------------------------------------------------------------------------

	uint64_t signalOnGpuInternal() noexcept;
	void waitOnCpuInternal(uint64_t fenceValue) noexcept;
	bool isFenceValueDone(uint64_t fenceValue) noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	D3D12_COMMAND_LIST_TYPE type() const noexcept { return mType; }
	ID3D12CommandQueue* commandQueue() noexcept { return mCommandQueue.Get(); }

private:
	// Private  methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode beginCommandListRecordingUnmutexed(ZgCommandList** commandListOut) noexcept;
	ZgErrorCode executeCommandListUnmutexed(ZgCommandList* commandList) noexcept;
	uint64_t signalOnGpuUnmutexed() noexcept;

	ZgErrorCode createCommandList(D3D12CommandList*& commandListOut) noexcept;

	ZgErrorCode executePreCommandListStateChanges(
		Vector<PendingBufferState>& pendingBufferStates,
		Vector<PendingTextureState>& pendingTextureStates) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	ZgLogger mLog = {};
	ZgAllocator mAllocator = {};

	std::mutex mQueueMutex;
	D3D12_COMMAND_LIST_TYPE mType;
	ComPtr<ID3D12Device3> mDevice;
	D3DX12Residency::ResidencyManager* mResidencyManager = nullptr;
	D3D12DescriptorRingBuffer* mDescriptorBuffer = nullptr;
	
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	
	ComPtr<ID3D12Fence> mCommandQueueFence;
	uint64_t mCommandQueueFenceValue = 0;
	HANDLE mCommandQueueFenceEvent = nullptr;

	uint32_t mMaxNumBuffersPerCommandList = 0;
	Vector<D3D12CommandList> mCommandListStorage;
	RingBuffer<D3D12CommandList*> mCommandListQueue;
};

} // namespace zg
