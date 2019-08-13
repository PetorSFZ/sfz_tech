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

#include "ZeroG/util/Assert.hpp"

namespace zg {

// D3D12Fence: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12Fence::~D3D12Fence() noexcept
{

}

// D3D12Fence: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12Fence::reset() noexcept
{
	this->fenceValue = 0;
	this->commandQueue = nullptr;
	return ZG_SUCCESS;
}

ZgErrorCode D3D12Fence::checkIfSignaled(bool& fenceSignaledOut) const noexcept
{
	if (this->commandQueue == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	fenceSignaledOut = this->commandQueue->isFenceValueDone(this->fenceValue);
	return ZG_SUCCESS;
}

ZgErrorCode D3D12Fence::waitOnCpuBlocking() const noexcept
{
	if (this->commandQueue == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	this->commandQueue->waitOnCpuInternal(this->fenceValue);
	return ZG_SUCCESS;
}


// D3D12CommandQueue: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12CommandQueue::~D3D12CommandQueue() noexcept
{
	// Flush queue
	this->flush();

	// Check that all command lists have been returned
	ZG_ASSERT(mCommandListStorage.size() == mCommandListQueue.size());

	// Destroy fence event
	CloseHandle(mCommandQueueFenceEvent);
}

// D3D12CommandQueue: State methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandQueue::create(
	D3D12_COMMAND_LIST_TYPE type,
	ComPtr<ID3D12Device3>& device,
	D3DX12Residency::ResidencyManager* residencyManager,
	D3D12DescriptorRingBuffer* descriptorBuffer,
	uint32_t maxNumCommandLists,
	uint32_t maxNumBuffersPerCommandList,
	ZgLogger logger,
	ZgAllocator allocator) noexcept
{
	mType = type;
	mDevice = device;
	mResidencyManager = residencyManager;
	mDescriptorBuffer = descriptorBuffer;
	mLog = logger;
	mAllocator = allocator;

	// Create command queue
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // TODO: D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
	desc.NodeMask = 0;

	if (D3D12_FAIL(mLog, device->CreateCommandQueue(&desc, IID_PPV_ARGS(&mCommandQueue)))) {
		return ZG_ERROR_NO_SUITABLE_DEVICE;
	}

	// Create command queue fence
	if (D3D12_FAIL(mLog, device->CreateFence(
		mCommandQueueFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCommandQueueFence)))) {
		return ZG_ERROR_GENERIC;
	}

	// Create command queue fence event
	mCommandQueueFenceEvent = ::CreateEvent(NULL, false, false, NULL);

	// Allocate memory for command lists
	mMaxNumBuffersPerCommandList = maxNumBuffersPerCommandList;
	mCommandListStorage.create(
		maxNumCommandLists, allocator, "ZeroG - D3D12CommandQueue - CommandListStorage");
	mCommandListQueue.create(
		maxNumCommandLists, allocator, "ZeroG - D3D12CommandQueue - CommandListQueue");

	return ZG_SUCCESS;
}

// D3D12CommandQueue: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandQueue::signalOnGpu(ZgFence& fenceToSignalIn) noexcept
{
	D3D12Fence& fenceToSignal = *static_cast<D3D12Fence*>(&fenceToSignalIn);
	fenceToSignal.commandQueue = this;
	fenceToSignal.fenceValue = signalOnGpuUnmutexed();
	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandQueue::waitOnGpu(const ZgFence& fenceIn) noexcept
{
	const D3D12Fence& fence = *static_cast<const D3D12Fence*>(&fenceIn);
	if (fence.commandQueue == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	CHECK_D3D12(mLog) this->mCommandQueue->Wait(
		fence.commandQueue->mCommandQueueFence.Get(), fence.fenceValue);
	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandQueue::flush() noexcept
{
	uint64_t fenceValue = this->signalOnGpuInternal();
	this->waitOnCpuInternal(fenceValue);
	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandQueue::beginCommandListRecording(ZgCommandList** commandListOut) noexcept
{
	std::lock_guard<std::mutex> lock(mQueueMutex);
	return this->beginCommandListRecordingUnmutexed(commandListOut);
}

ZgErrorCode D3D12CommandQueue::executeCommandList(ZgCommandList* commandListIn) noexcept
{
	std::lock_guard<std::mutex> lock(mQueueMutex);
	return this->executeCommandListUnmutexed(commandListIn);
}

// D3D12CommandQueue: Synchronization methods
// ------------------------------------------------------------------------------------------------

uint64_t D3D12CommandQueue::signalOnGpuInternal() noexcept
{
	std::lock_guard<std::mutex> lock(mQueueMutex);
	return signalOnGpuUnmutexed();
}

void D3D12CommandQueue::waitOnCpuInternal(uint64_t fenceValue) noexcept
{
	// TODO: Kind of bad to only have one event, must have mutex here because of that.
	std::lock_guard<std::mutex> lock(mQueueMutex);

	if (!isFenceValueDone(fenceValue)) {
		CHECK_D3D12(mLog) mCommandQueueFence->SetEventOnCompletion(
			fenceValue, mCommandQueueFenceEvent);
		// TODO: Don't wait forever
		::WaitForSingleObject(mCommandQueueFenceEvent, INFINITE);
	}
}

bool D3D12CommandQueue::isFenceValueDone(uint64_t fenceValue) noexcept
{
	return mCommandQueueFence->GetCompletedValue() >= fenceValue;
}

// D3D12CommandQueue: Private  methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandQueue::beginCommandListRecordingUnmutexed(
	ZgCommandList** commandListOut) noexcept
{
	D3D12CommandList* commandList = nullptr;
	bool commandListFound = false;

	// If command lists available in queue, attempt to get one of them
	uint64_t queueSize = mCommandListQueue.size();
	if (queueSize != 0) {
		if (isFenceValueDone(mCommandListQueue.first()->fenceValue)) {
			mCommandListQueue.pop(commandList);
			commandListFound = true;
		}
	}

	// If no command list found, create new one
	if (!commandListFound) {
		ZgErrorCode res = createCommandList(commandList);
		if (res != ZG_SUCCESS) return res;
		commandListFound = true;
	}

	// Reset command list and allocator
	ZgErrorCode res = commandList->reset();
	if (res != ZG_SUCCESS) return res;

	// Open command lists residency set
	CHECK_D3D12(mLog) commandList->residencySet->Open();

	// Return command list
	*commandListOut = commandList;
	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandQueue::executeCommandListUnmutexed(ZgCommandList* commandListIn) noexcept
{
	// Cast to D3D12
	D3D12CommandList& commandList = *static_cast<D3D12CommandList*>(commandListIn);

	// Close command list
	if (D3D12_FAIL(mLog, commandList.commandList->Close())) {
		return ZG_ERROR_GENERIC;
	}

	// Close residency set
	if (D3D12_FAIL(mLog, commandList.residencySet->Close())) {
		return ZG_ERROR_GENERIC;
	}

	// Create and execute a quick command list to insert barriers and commit pending states
	ZgErrorCode res = this->executePreCommandListStateChanges(
		commandList.pendingBufferStates,
		commandList.pendingTextureStates);
	if (res != ZG_SUCCESS) return res;

	// Execute command list
	ID3D12CommandList* commandListPtr = commandList.commandList.Get();
	HRESULT executeCommandListRes = mResidencyManager->ExecuteCommandLists(
		mCommandQueue.Get(), &commandListPtr, &commandList.residencySet, 1);

	// Signal
	commandList.fenceValue = this->signalOnGpuUnmutexed();

	// Add command list to queue
	mCommandListQueue.add(&commandList);

	if (D3D12_FAIL(mLog, executeCommandListRes)) return ZG_ERROR_GENERIC;
	return ZG_SUCCESS;
}

uint64_t D3D12CommandQueue::signalOnGpuUnmutexed() noexcept
{
	CHECK_D3D12(mLog) mCommandQueue->Signal(mCommandQueueFence.Get(), mCommandQueueFenceValue);
	return mCommandQueueFenceValue++;
}

ZgErrorCode D3D12CommandQueue::createCommandList(D3D12CommandList*& commandListOut) noexcept
{
	// Create a new command list in storage, return error if full
	bool addSuccesful = mCommandListStorage.add(D3D12CommandList());
	if (!addSuccesful) return ZG_ERROR_OUT_OF_COMMAND_LISTS;

	D3D12CommandList& commandList = mCommandListStorage.last();
	commandList.commandListType = this->mType;

	// Create command allocator
	if (D3D12_FAIL(mLog, mDevice->CreateCommandAllocator(
		mType, IID_PPV_ARGS(&commandList.commandAllocator)))) {
		mCommandListStorage.pop();
		return ZG_ERROR_GENERIC;
	}

	// Create command list
	if (D3D12_FAIL(mLog, mDevice->CreateCommandList(
		0,
		mType,
		commandList.commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&commandList.commandList)))) {
		mCommandListStorage.pop();
		return ZG_ERROR_GENERIC;
	}

	// Ensure command list is in closed state
	if (D3D12_FAIL(mLog, commandList.commandList->Close())) {
		return ZG_ERROR_GENERIC;
	}

	// Initialize command list
	commandList.create(mMaxNumBuffersPerCommandList, mLog, mAllocator, mDevice, mResidencyManager,
		mDescriptorBuffer);

	commandListOut = &commandList;
	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandQueue::executePreCommandListStateChanges(
	Vector<PendingBufferState>& pendingBufferStates,
	Vector<PendingTextureState>& pendingTextureStates) noexcept
{
	// Temporary storage array for the barriers to insert
	uint32_t numBarriers = 0;
	constexpr uint32_t MAX_NUM_BARRIERS = 256;
	CD3DX12_RESOURCE_BARRIER barriers[MAX_NUM_BARRIERS] = {};
	constexpr uint32_t MAX_NUM_RESIDENCY_OBJECTS = 512;
	D3DX12Residency::ManagedObject* residencyObjects[MAX_NUM_RESIDENCY_OBJECTS] = {};

	// Gather buffer barriers
	for (uint32_t i = 0; i < pendingBufferStates.size(); i++) {
		const PendingBufferState& state = pendingBufferStates[i];

		// Don't insert barrier if resource already is in correct state
		if (state.buffer->lastCommittedState == state.neededInitialState) {
			continue;
		}

		// Error out if we don't have enough space in our temp array
		if (numBarriers >= MAX_NUM_BARRIERS) {
			ZG_ERROR(mLog, "Internal error, need to insert too many barriers. Fixable, please contact ZeroG devs.");
			return ZG_ERROR_GENERIC;
		}

		// Create barrier
		barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(
			state.buffer->resource.Get(),
			state.buffer->lastCommittedState,
			state.neededInitialState);

		// Store residency set
		residencyObjects[numBarriers] = &state.buffer->memoryHeap->managedObject;

		numBarriers += 1;
	}

	// Gather texture barriers
	for (uint32_t i = 0; i < pendingTextureStates.size(); i++) {
		const PendingTextureState& state = pendingTextureStates[i];

		// Don't insert barrier if resource already is in correct state
		if (state.texture->lastCommittedStates[state.mipLevel] == state.neededInitialState) {
			continue;
		}

		// Error out if we don't have enough space in our temp array
		if (numBarriers >= MAX_NUM_BARRIERS) {
			ZG_ERROR(mLog, "Internal error, need to insert too many barriers. Fixable, please contact ZeroG devs.");
			return ZG_ERROR_GENERIC;
		}

		// Create barrier
		barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(
			state.texture->resource.Get(),
			state.texture->lastCommittedStates[state.mipLevel],
			state.neededInitialState,
			state.mipLevel);

		// Store residency set
		residencyObjects[numBarriers] = &state.texture->textureHeap->managedObject;

		numBarriers += 1;
	}

	// Exit if we do not need to insert any barriers
	if (numBarriers == 0) return ZG_SUCCESS;

	// Get command list to execute barriers in
	D3D12CommandList* commandList = nullptr;
	ZgErrorCode res =
		this->beginCommandListRecordingUnmutexed(reinterpret_cast<ZgCommandList**>(&commandList));
	if (res != ZG_SUCCESS) return res;

	// Insert barrier call
	commandList->commandList->ResourceBarrier(numBarriers, barriers);

	// Add all managed objects to residency set
	for (uint32_t i = 0; i < numBarriers; i++) {
		commandList->residencySet->Insert(residencyObjects[i]);
	}

	// Execute barriers
	res = this->executeCommandListUnmutexed(commandList);
	if (res != ZG_SUCCESS) return res;

	// Commit state changes
#pragma message("WARNING, probably serious race condition")
	// TODO: This is problematic and we probably need to something smarter. TL;DR, this comitted
	//       state is shared between all queues. Maybe it is enough to just put a mutex around it,
	//       but it is not obvious to me that that would be enough.
	for (uint32_t i = 0; i < pendingBufferStates.size(); i++) {
		const PendingBufferState& state = pendingBufferStates[i];
		state.buffer->lastCommittedState = state.currentState;
	}
	for (uint32_t i = 0; i < pendingTextureStates.size(); i++) {
		const PendingTextureState& state = pendingTextureStates[i];
		state.texture->lastCommittedStates[state.mipLevel] = state.currentState;
	}

	return ZG_SUCCESS;
}

} // namespace zg
