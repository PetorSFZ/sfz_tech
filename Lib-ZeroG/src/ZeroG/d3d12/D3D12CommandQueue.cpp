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

#include "ZeroG/d3d12/D3D12CommandQueue.hpp"

namespace zg {

// D3D12CommandQueue: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12CommandQueue::~D3D12CommandQueue() noexcept
{
	// Destroy fence event
	CloseHandle(mCommandQueueFenceEvent);
}

// D3D12CommandQueue: State methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandQueue::init(
	ComPtr<ID3D12Device3>& device,
	uint32_t maxNumCommandLists) noexcept
{
	// Create command queue
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // TODO: D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
	desc.NodeMask = 0;

	if (!CHECK_D3D12_SUCCEEDED(
		device->CreateCommandQueue(&desc, IID_PPV_ARGS(&mCommandQueue)))) {
		return ZG_ERROR_NO_SUITABLE_DEVICE;
	}

	// Create command queue fence
	if (!CHECK_D3D12_SUCCEEDED(device->CreateFence(
		mCommandQueueFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCommandQueueFence)))) {
		return ZG_ERROR_GENERIC;
	}

	// Create command queue fence event
	mCommandQueueFenceEvent = ::CreateEvent(NULL, false, false, NULL);

	return ZG_SUCCESS;
}

// D3D12CommandQueue: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandQueue::flush() noexcept
{
	uint64_t fenceValue = this->signalOnGpu();
	this->waitOnCpu(fenceValue);
	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandQueue::getCommandList(ICommandList** commandListOut) noexcept
{
	std::lock_guard<std::mutex> lock(mQueueMutex);

	return ZG_ERROR_UNIMPLEMENTED;
}

ZgErrorCode D3D12CommandQueue::executeCommandList(ICommandList* commandList) noexcept
{
	std::lock_guard<std::mutex> lock(mQueueMutex);

	return ZG_ERROR_UNIMPLEMENTED;
}

// D3D12CommandQueue: Synchronization methods
// ------------------------------------------------------------------------------------------------

uint64_t D3D12CommandQueue::signalOnGpu()
{
	std::lock_guard<std::mutex> lock(mQueueMutex);
	CHECK_D3D12 mCommandQueue->Signal(mCommandQueueFence.Get(), mCommandQueueFenceValue);
	return mCommandQueueFenceValue++;
}

void D3D12CommandQueue::waitOnCpu(uint64_t fenceValue)
{
	// TODO: Kind of bad to only have one event, must have mutex here because of that.
	std::lock_guard<std::mutex> lock(mQueueMutex);

	if (mCommandQueueFence->GetCompletedValue() < fenceValue) {
		CHECK_D3D12 mCommandQueueFence->SetEventOnCompletion(
			fenceValue, mCommandQueueFenceEvent);
		// TODO: Don't wait forever
		::WaitForSingleObject(mCommandQueueFenceEvent, INFINITE);
	}
}

} // namespace zg
