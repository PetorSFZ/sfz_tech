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

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_ring_buffers.hpp>

#include "ZeroG.h"
#include "d3d12/D3D12Common.hpp"
#include "d3d12/D3D12CommandList.hpp"
#include "d3d12/D3D12DescriptorRingBuffer.hpp"
#include "d3d12/D3D12ResourceTracking.hpp"

// Fence
// ------------------------------------------------------------------------------------------------

struct ZgFence final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ZgFence() noexcept = default;
	ZgFence(const ZgFence&) = delete;
	ZgFence& operator= (const ZgFence&) = delete;
	ZgFence(ZgFence&&) = delete;
	ZgFence& operator= (ZgFence&&) = delete;
	~ZgFence() noexcept {}

	// Members
	// --------------------------------------------------------------------------------------------

	u64 fenceValue = 0;
	ZgCommandQueue* commandQueue = nullptr;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult reset() noexcept
	{
		this->fenceValue = 0;
		this->commandQueue = nullptr;
		return ZG_SUCCESS;
	}

	ZgResult checkIfSignaled(bool& fenceSignaledOut) const noexcept;
	ZgResult waitOnCpuBlocking() const noexcept;
};

// ZgCommandQueue
// ------------------------------------------------------------------------------------------------

constexpr u32 MAX_NUM_COMMAND_LISTS = 1024;

struct ZgCommandQueue final {
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ZgCommandQueue() noexcept = default;
	ZgCommandQueue(const ZgCommandQueue&) = delete;
	ZgCommandQueue& operator= (const ZgCommandQueue&) = delete;
	ZgCommandQueue(ZgCommandQueue&&) = delete;
	ZgCommandQueue& operator= (ZgCommandQueue&&) = delete;
	~ZgCommandQueue() noexcept
	{
		// Flush queue
		this->flush();

		// Check that all command lists have been returned
		sfz_assert(mCommandListStorage.size() == mCommandListQueue.size());

		// Destroy fence event
		CloseHandle(mCommandQueueFenceEvent);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgResult create(
		D3D12_COMMAND_LIST_TYPE type,
		ComPtr<ID3D12Device3>& device,
		D3D12DescriptorRingBuffer* descriptorBuffer) noexcept
	{
		mType = type;
		mDevice = device;
		mDescriptorBuffer = descriptorBuffer;

		// Create command queue
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = type;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // TODO: D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
		desc.NodeMask = 0;

		if (D3D12_FAIL(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&mCommandQueue)))) {
			return ZG_ERROR_NO_SUITABLE_DEVICE;
		}

		// Create command queue fence
		if (D3D12_FAIL(device->CreateFence(
			mCommandQueueFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCommandQueueFence)))) {
			return ZG_ERROR_GENERIC;
		}

		// Create command queue fence event
		mCommandQueueFenceEvent = ::CreateEvent(NULL, false, false, NULL);

		// Allocate memory for command lists
		mCommandListStorage.init(
			MAX_NUM_COMMAND_LISTS, getAllocator(), sfz_dbg("ZeroG - D3D12CommandQueue - CommandListStorage"));
		mCommandListQueue.create(
			MAX_NUM_COMMAND_LISTS, getAllocator(), sfz_dbg("ZeroG - D3D12CommandQueue - CommandListQueue"));

		return ZG_SUCCESS;
	}

	// Members
	// --------------------------------------------------------------------------------------------

	D3D12_COMMAND_LIST_TYPE mType;
	ComPtr<ID3D12Device3> mDevice;
	D3D12DescriptorRingBuffer* mDescriptorBuffer = nullptr;

	ComPtr<ID3D12CommandQueue> mCommandQueue;

	ComPtr<ID3D12Fence> mCommandQueueFence;
	u64 mCommandQueueFenceValue = 0;
	HANDLE mCommandQueueFenceEvent = nullptr;

	sfz::Array<ZgCommandList> mCommandListStorage;
	sfz::RingBuffer<ZgCommandList*> mCommandListQueue;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult signalOnGpu(ZgFence& fenceToSignal) noexcept
	{
		fenceToSignal.commandQueue = this;
		fenceToSignal.fenceValue = signalOnGpuInternal();
		return ZG_SUCCESS;
	}

	ZgResult waitOnGpu(const ZgFence& fence) noexcept
	{
		if (fence.commandQueue == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
		CHECK_D3D12 this->mCommandQueue->Wait(
			fence.commandQueue->mCommandQueueFence.Get(), fence.fenceValue);
		return ZG_SUCCESS;
	}

	ZgResult flush() noexcept
	{
		u64 fenceValue = this->signalOnGpuInternal();
		this->waitOnCpuInternal(fenceValue);
		return ZG_SUCCESS;
	}

	ZgResult beginCommandListRecording(ZgCommandList** commandListOut) noexcept
	{
		ZgCommandList* commandList = nullptr;
		bool commandListFound = false;

		// If command lists available in queue, attempt to get one of them
		u64 queueSize = mCommandListQueue.size();
		if (queueSize != 0) {
			if (isFenceValueDone(mCommandListQueue.first()->fenceValue)) {
				mCommandListQueue.pop(commandList);
				commandListFound = true;
			}
		}

		// If no command list found, create new one
		if (!commandListFound) {
			ZgResult res = createCommandList(commandList);
			if (res != ZG_SUCCESS) return res;
			commandListFound = true;
		}

		// Reset command list and allocator
		ZgResult res = commandList->reset();
		if (res != ZG_SUCCESS) return res;

		// Return command list
		*commandListOut = commandList;
		return ZG_SUCCESS;
	}

	ZgResult executeCommandList(ZgCommandList* commandList, bool barrierList = false) noexcept
	{
		// Close command list
		if (D3D12_FAIL(commandList->commandList->Close())) {
			return ZG_ERROR_GENERIC;
		}

		auto execBarriers = [&](const CD3DX12_RESOURCE_BARRIER* barriers, u32 numBarriers) -> ZgResult {
			// Get command list to execute barriers in
			ZgCommandList* commandList = nullptr;
			ZgResult res =
				this->beginCommandListRecording(&commandList);
			if (res != ZG_SUCCESS) return res;

			// Insert barrier call
			commandList->commandList->ResourceBarrier(numBarriers, barriers);

			// Execute barriers
			return this->executeCommandList(commandList, true);
		};

		// Execute command list
		ID3D12CommandList* cmdLists[1] = {};
		ZgTrackerCommandListState* cmdListStates[1] = {};
		cmdLists[0] = commandList->commandList.Get();
		cmdListStates[0] = &commandList->tracking;
		executeCommandLists(*mCommandQueue.Get(), cmdLists, cmdListStates, 1, execBarriers, barrierList);

		// Signal
		u64 fenceValue = this->signalOnGpuInternal();
		commandList->fenceValue = fenceValue;

		// Add command list to queue
		mCommandListQueue.add(commandList);

		return ZG_SUCCESS;
	}

	// Synchronization methods
	// --------------------------------------------------------------------------------------------

	u64 signalOnGpuInternal() noexcept
	{
		CHECK_D3D12 mCommandQueue->Signal(mCommandQueueFence.Get(), mCommandQueueFenceValue);
		return mCommandQueueFenceValue++;
	}

	void waitOnCpuInternal(u64 fenceValue) noexcept
	{
		if (!isFenceValueDone(fenceValue)) {
			CHECK_D3D12 mCommandQueueFence->SetEventOnCompletion(
				fenceValue, mCommandQueueFenceEvent);
			// TODO: Don't wait forever
			::WaitForSingleObject(mCommandQueueFenceEvent, INFINITE);
		}
	}

	bool isFenceValueDone(u64 fenceValue) noexcept
	{
		return mCommandQueueFence->GetCompletedValue() >= fenceValue;
	}

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	ZgResult createCommandList(ZgCommandList*& commandListOut) noexcept
	{
		// Create a new command list in storage
		ZgCommandList& commandList = mCommandListStorage.add();
		sfz_assert_hard(mCommandListStorage.size() < MAX_NUM_COMMAND_LISTS);
		commandList.commandListType = this->mType;

		// Create command allocator
		if (D3D12_FAIL(mDevice->CreateCommandAllocator(
			mType, IID_PPV_ARGS(&commandList.commandAllocator)))) {
			mCommandListStorage.pop();
			return ZG_ERROR_GENERIC;
		}

		// Create command list
		if (D3D12_FAIL(mDevice->CreateCommandList(
			0,
			mType,
			commandList.commandAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&commandList.commandList)))) {
			mCommandListStorage.pop();
			return ZG_ERROR_GENERIC;
		}

		// Ensure command list is in closed state
		if (D3D12_FAIL(commandList.commandList->Close())) {
			mCommandListStorage.pop();
			return ZG_ERROR_GENERIC;
		}

		// Initialize command list
		commandList.create(
			mDevice,
			mDescriptorBuffer);

		commandListOut = &commandList;
		return ZG_SUCCESS;
	}
};

// Fence (continued
// ------------------------------------------------------------------------------------------------

inline ZgResult ZgFence::checkIfSignaled(bool& fenceSignaledOut) const noexcept
{
	if (this->commandQueue == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	fenceSignaledOut = this->commandQueue->isFenceValueDone(this->fenceValue);
	return ZG_SUCCESS;
}

inline ZgResult ZgFence::waitOnCpuBlocking() const noexcept
{
	if (this->commandQueue == nullptr) return ZG_SUCCESS;
	this->commandQueue->waitOnCpuInternal(this->fenceValue);
	return ZG_SUCCESS;
}
