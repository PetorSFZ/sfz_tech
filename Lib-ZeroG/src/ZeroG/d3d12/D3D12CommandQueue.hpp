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

#include "ZeroG/d3d12/D3D12Common.hpp"
#include "ZeroG/d3d12/D3D12CommandList.hpp"
#include "ZeroG/util/RingBuffer.hpp"
#include "ZeroG/util/Vector.hpp"
#include "ZeroG/BackendInterface.hpp"
#include "ZeroG/ZeroG-CApi.h"

namespace zg {

// D3D12CommandQueue
// ------------------------------------------------------------------------------------------------

class D3D12CommandQueue final : public ICommandQueue {
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
		ComPtr<ID3D12Device3>& device,
		uint32_t maxNumCommandLists,
		uint32_t maxNumBuffersPerCommandList,
		ZgAllocator allocator) noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode flush() noexcept override final;
	ZgErrorCode beginCommandListRecording(ICommandList** commandListOut) noexcept override final;
	ZgErrorCode executeCommandList(ICommandList* commandList) noexcept override final;

	// Synchronization methods
	// --------------------------------------------------------------------------------------------

	uint64_t signalOnGpu() noexcept;
	void waitOnCpu(uint64_t fenceValue) noexcept;
	bool isFenceValueDone(uint64_t fenceValue) noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	ID3D12CommandQueue* commandQueue() noexcept { return mCommandQueue.Get(); }

private:
	// Private  methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode beginCommandListRecordingUnmutexed(ICommandList** commandListOut) noexcept;
	ZgErrorCode executeCommandListUnmutexed(ICommandList* commandList) noexcept;
	uint64_t signalOnGpuUnmutexed() noexcept;

	ZgErrorCode createCommandList(D3D12CommandList*& commandListOut) noexcept;

	ZgErrorCode executePreCommandListBufferStateChanges(
		Vector<PendingState>& pendingStates) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	ZgAllocator mAllocator = {};

	std::mutex mQueueMutex;
	ComPtr<ID3D12Device3> mDevice;
	
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	
	ComPtr<ID3D12Fence> mCommandQueueFence;
	uint64_t mCommandQueueFenceValue = 0;
	HANDLE mCommandQueueFenceEvent = nullptr;

	uint32_t mMaxNumBuffersPerCommandList = 0;
	Vector<D3D12CommandList> mCommandListStorage;
	RingBuffer<D3D12CommandList*> mCommandListQueue;
};

} // namespace zg
