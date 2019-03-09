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

#include "ZeroG/d3d12/D3D12CommandList.hpp"

#include <algorithm>

#include "ZeroG/d3d12/D3D12Framebuffer.hpp"
#include "ZeroG/util/Assert.hpp"

namespace zg {

// D3D12CommandList: State methods
// ------------------------------------------------------------------------------------------------

void D3D12CommandList::create(
	uint32_t maxNumBuffers,
	ZgLogger logger,
	ZgAllocator allocator,
	ComPtr<ID3D12Device3> device,
	D3DX12Residency::ResidencyManager* residencyManager,
	D3D12DescriptorRingBuffer* descriptorBuffer) noexcept
{
	mLog = logger;
	mDevice = device;
	mDescriptorBuffer = descriptorBuffer;
	pendingBufferIdentifiers.create(maxNumBuffers, allocator, "ZeroG - D3D12CommandList - Internal");
	pendingBufferStates.create(maxNumBuffers, allocator, "ZeroG - D3D12CommandList - Internal");

	residencySet = residencyManager->CreateResidencySet();
}

void D3D12CommandList::swap(D3D12CommandList& other) noexcept
{
	std::swap(this->commandAllocator, other.commandAllocator);
	std::swap(this->commandList, other.commandList);
	std::swap(this->fenceValue, other.fenceValue);

	std::swap(this->residencySet, other.residencySet);

	this->pendingBufferIdentifiers.swap(other.pendingBufferIdentifiers);
	this->pendingBufferStates.swap(other.pendingBufferStates);

	std::swap(this->mLog, other.mLog);
	std::swap(this->mDevice, other.mDevice);
	std::swap(this->mResidencyManager, other.mResidencyManager);
	std::swap(this->mDescriptorBuffer, other.mDescriptorBuffer);
	std::swap(this->mPipelineSet, other.mPipelineSet);
	std::swap(this->mBoundPipeline, other.mBoundPipeline);
	std::swap(this->mFramebufferSet, other.mFramebufferSet);
	std::swap(this->mFramebufferDescriptor, other.mFramebufferDescriptor);
}

void D3D12CommandList::destroy() noexcept
{
	commandAllocator = nullptr;
	commandList = nullptr;
	fenceValue = 0;

	if (residencySet != nullptr) {
		mResidencyManager->DestroyResidencySet(residencySet);
	}
	residencySet = nullptr;

	pendingBufferIdentifiers.destroy();
	pendingBufferStates.destroy();

	mLog = {};
	mDevice = nullptr;
	mResidencyManager = nullptr;
	mDescriptorBuffer = nullptr;
	mPipelineSet = false;
	mBoundPipeline = nullptr;
	mFramebufferSet = false;
	mFramebufferDescriptor = {};
}

// D3D12CommandList: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandList::memcpyBufferToBuffer(
	IBuffer* dstBufferIn,
	uint64_t dstBufferOffsetBytes,
	IBuffer* srcBufferIn,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes) noexcept
{
	// Cast input to D3D12
	D3D12Buffer& dstBuffer = *reinterpret_cast<D3D12Buffer*>(dstBufferIn);
	D3D12Buffer& srcBuffer = *reinterpret_cast<D3D12Buffer*>(srcBufferIn);

	// Current don't allow memcpy:ing to the same buffer.
	if (dstBuffer.identifier == srcBuffer.identifier) return ZG_ERROR_INVALID_ARGUMENT;

	// Wanted resource states
	D3D12_RESOURCE_STATES dstTargetState = D3D12_RESOURCE_STATE_COPY_DEST;
	D3D12_RESOURCE_STATES srcTargetState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	if (srcBuffer.memoryType == ZG_BUFFER_MEMORY_TYPE_UPLOAD) {
		srcTargetState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	// Set buffer resource states
	ZgErrorCode res = setBufferState(dstBuffer, dstTargetState);
	if (res != ZG_SUCCESS) return res;
	 res = setBufferState(srcBuffer, srcTargetState);
	if (res != ZG_SUCCESS) return res;

	// Check if we should copy entire buffer or just a region of it
	bool copyEntireBuffer =
		dstBuffer.sizeBytes == srcBuffer.sizeBytes &&
		dstBuffer.sizeBytes == numBytes &&
		dstBufferOffsetBytes == 0 &&
		srcBufferOffsetBytes == 0;

	// Add buffers to residency set
	residencySet->Insert(&srcBuffer.heapManagedObject);
	residencySet->Insert(&dstBuffer.heapManagedObject);

	// Copy entire buffer
	if (copyEntireBuffer) {
		commandList->CopyResource(dstBuffer.resource.Get(), srcBuffer.resource.Get());
	}

	// Copy region of buffer
	else {
		commandList->CopyBufferRegion(
			dstBuffer.resource.Get(),
			dstBufferOffsetBytes,
			srcBuffer.resource.Get(),
			srcBufferOffsetBytes,
			numBytes);
	}

	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandList::setPushConstant(
	uint32_t shaderRegister,
	const void* dataPtr,
	uint32_t dataSizeInBytes) noexcept
{
	// Require that a pipeline has been set so we can query its parameters
	if (!mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Linear search to find push constant mapping
	uint32_t mappingIdx = ~0u;
	for (uint32_t i = 0; i < mBoundPipeline->numPushConstants; i++) {
		if (mBoundPipeline->pushConstants[i].shaderRegister == shaderRegister) {
			mappingIdx = i;
			break;
		}
	}

	// Return invalid argument if there is no push constant associated with the given register
	if (mappingIdx == ~0u) return ZG_ERROR_INVALID_ARGUMENT;
	const D3D12PushConstantMapping& mapping = mBoundPipeline->pushConstants[mappingIdx];

	// Sanity check to attempt to see if user provided enough bytes to read
	if (mapping.sizeInBytes != dataSizeInBytes) return ZG_ERROR_INVALID_ARGUMENT;

	// Set push constant
	if (mapping.sizeInBytes == 4) {
		uint32_t data = *reinterpret_cast<const uint32_t*>(dataPtr);
		commandList->SetGraphicsRoot32BitConstant(mapping.parameterIndex, data, 0);
	}
	else {
		commandList->SetGraphicsRoot32BitConstants(
			mapping.parameterIndex, mapping.sizeInBytes / 4, dataPtr, 0);
	}
	
	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandList::bindConstantBuffers(
	const ZgConstantBufferBindings& bindings) noexcept
{
	// Require that a pipeline has been set so we can query its parameters
	if (!mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Require that all constant buffers be specified
	if (bindings.numBindings != mBoundPipeline->numConstantBuffers) {
		return ZG_ERROR_INVALID_ARGUMENT;
	}

	// Allocate descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE rangeStartCpu = {};
	D3D12_GPU_DESCRIPTOR_HANDLE rangeStartGpu = {};
	ZgErrorCode allocRes = mDescriptorBuffer->allocateDescriptorRange(
		bindings.numBindings, rangeStartCpu, rangeStartGpu);
	if (allocRes != ZG_SUCCESS) return allocRes;

	// Create constant buffer views and fill (CPU) descriptors
	for (uint32_t i = 0; i < bindings.numBindings; i++) {
		const ZgConstantBufferBinding& binding = bindings.bindings[i];
		
		// Linear search to find mapping
		uint32_t mappingIdx = ~0u;
		for (uint32_t j = 0; j < mBoundPipeline->numConstantBuffers; j++) {
			const D3D12ConstantBufferMapping& mapping = mBoundPipeline->constBuffers[j];
			if (binding.shaderRegister == mapping.shaderRegister) {
				mappingIdx = j;
				break;
			}
		}

		// Return invalid argument if there is no constant buffer associated with the given register
		if (mappingIdx == ~0u) return ZG_ERROR_INVALID_ARGUMENT;
		const D3D12ConstantBufferMapping& mapping = mBoundPipeline->constBuffers[mappingIdx];

		// Cast buffer to D3D12Buffer
		D3D12Buffer* buffer = reinterpret_cast<D3D12Buffer*>(binding.buffer);

		// D3D12 requires that a Constant Buffer View is at least 256 bytes, and a multiple of 256.
		// Round up constant buffer size to nearest 256 alignment
		ZG_ASSERT(mapping.sizeInBytes != 0);
		uint32_t bufferSize256Aligned = (mapping.sizeInBytes + 255) & 0xFFFFFF00u;

		// Check that buffer is large enough
		if (buffer->sizeBytes < bufferSize256Aligned) {
			ZG_ERROR(mLog, "Constant buffer at shader register %u requires a buffer that is at"
				" least %u bytes, specified buffer is %u bytes.",
				mapping.shaderRegister, bufferSize256Aligned, buffer->sizeBytes);
			return ZG_ERROR_INVALID_ARGUMENT;
		}

		/// Get the CPU descriptor
		ZG_ASSERT(mapping.tableOffset < bindings.numBindings);
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor;
		cpuDescriptor.ptr =
			rangeStartCpu.ptr + mDescriptorBuffer->descriptorSize * mapping.tableOffset;

		// Create constant buffer view
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = buffer->resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = bufferSize256Aligned;
		mDevice->CreateConstantBufferView(&cbvDesc, cpuDescriptor);

		// Set buffer resource state
		setBufferState(*buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		// Insert into residency set
		residencySet->Insert(&buffer->heapManagedObject);
	}

	// Set descriptor table to root signature
	commandList->SetGraphicsRootDescriptorTable(
		mBoundPipeline->constBuffersParameterIndex, rangeStartGpu);

	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandList::setPipelineRendering(
	IPipelineRendering* pipelineIn) noexcept
{
	D3D12PipelineRendering& pipeline = *reinterpret_cast<D3D12PipelineRendering*>(pipelineIn);
	
	// If a pipeline is already set for this command list, return error. We currently only allow a
	// single pipeline per command list.
	if (mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	mPipelineSet = true;
	mBoundPipeline = &pipeline;

	// Set pipeline
	commandList->SetPipelineState(pipeline.pipelineState.Get());
	commandList->SetGraphicsRootSignature(pipeline.rootSignature.Get());

	// Set descriptor heap
	ID3D12DescriptorHeap* heaps[] = { mDescriptorBuffer->descriptorHeap.Get() };
	commandList->SetDescriptorHeaps(1, heaps);

	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandList::setFramebuffer(
	const ZgCommandListSetFramebufferInfo& info) noexcept
{
	// Cast input to D3D12
	D3D12Framebuffer& framebuffer = *reinterpret_cast<D3D12Framebuffer*>(info.framebuffer);

	// If a framebuffer is already set for this command list, return error. We currently only allow
	// a single framebuffer per command list.
	if (mFramebufferSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	mFramebufferSet = true;
	mFramebufferDescriptor = framebuffer.descriptor;

	// If input viewport is zero, set one that covers entire screen
	D3D12_VIEWPORT viewport = {};
	if (info.viewport.topLeftX == 0 &&
		info.viewport.topLeftY == 0 &&
		info.viewport.width == 0 &&
		info.viewport.height == 0) {

		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = float(framebuffer.width);
		viewport.Height = float(framebuffer.height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
	}

	// Otherwise do what the user explicitly requested
	else {
		viewport.TopLeftX = float(info.viewport.topLeftX);
		viewport.TopLeftY = float(info.viewport.topLeftY);
		viewport.Width = float(info.viewport.width);
		viewport.Height = float(info.viewport.height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
	}

	// Set viewport
	commandList->RSSetViewports(1, &viewport);
	
	// If scissor is zero, set on that covers entire screen
	D3D12_RECT scissorRect = {};
	if (info.scissor.topLeftX == 0 &&
		info.scissor.topLeftY == 0 &&
		info.scissor.width == 0 &&
		info.scissor.height == 0) {

		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = LONG_MAX;
		scissorRect.bottom = LONG_MAX;
	}

	// Otherwise do what user explicitly requested
	else {
		scissorRect.left = info.scissor.topLeftX;
		scissorRect.top = info.scissor.topLeftY;
		scissorRect.right = info.scissor.topLeftX + info.scissor.width;
		scissorRect.bottom = info.scissor.topLeftY + info.scissor.height;
	}

	// Set scissor rect
	commandList->RSSetScissorRects(1, &scissorRect);

	// Set framebuffer
	commandList->OMSetRenderTargets(1, &framebuffer.descriptor, FALSE, nullptr);

	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandList::clearFramebuffer(
	float red,
	float green,
	float blue,
	float alpha) noexcept
{
	// Return error if no framebuffer is set
	if (!mFramebufferSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Clear framebuffer
	float clearColor[4] = { red, green, blue, alpha };
	commandList->ClearRenderTargetView(mFramebufferDescriptor, clearColor, 0, nullptr);

	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandList::setVertexBuffer(
	uint32_t vertexBufferSlot,
	IBuffer* vertexBufferIn) noexcept
{
	// Cast input to D3D12
	D3D12Buffer& vertexBuffer = *reinterpret_cast<D3D12Buffer*>(vertexBufferIn);

	// Need to have a pipeline set to verify vertex buffer binding
	if (!mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Check that the vertex buffer slot is not out of bounds for the bound pipeline
	const ZgPipelineRenderingCreateInfo& pipelineInfo = mBoundPipeline->createInfo;
	if (pipelineInfo.numVertexBufferSlots <= vertexBufferSlot) {
		return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	}

	// Set buffer resource state
	ZgErrorCode res = setBufferState(vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	if (res != ZG_SUCCESS) return res;

	// Create vertex buffer view
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = vertexBuffer.resource->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = pipelineInfo.vertexBufferStridesBytes[vertexBufferSlot];
	vertexBufferView.SizeInBytes = uint32_t(vertexBuffer.sizeBytes);

	// Set vertex buffer
	commandList->IASetVertexBuffers(vertexBufferSlot, 1, &vertexBufferView);

	// Insert into residency set
	residencySet->Insert(&vertexBuffer.heapManagedObject);

	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandList::drawTriangles(
	uint32_t startVertexIndex,
	uint32_t numVertices) noexcept
{	
	// Draw triangles
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(numVertices, 1, startVertexIndex, 0);
	return ZG_SUCCESS;
}

// D3D12CommandList: Helper methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandList::reset() noexcept
{
	if (D3D12_FAIL(mLog, commandAllocator->Reset())) {
		return ZG_ERROR_GENERIC;
	}
	if (D3D12_FAIL(mLog, commandList->Reset(commandAllocator.Get(), nullptr))) {
		return ZG_ERROR_GENERIC;
	}

	pendingBufferIdentifiers.clear();
	pendingBufferStates.clear();

	mPipelineSet = false;
	mBoundPipeline = nullptr;
	mFramebufferSet = false;
	mFramebufferDescriptor = {};
	return ZG_SUCCESS;
}

// D3D12CommandList: Private methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandList::getPendingBufferStates(
	D3D12Buffer& buffer,
	D3D12_RESOURCE_STATES neededState,
	PendingState*& pendingStatesOut) noexcept
{
	// Try to find index of pending buffer states
	uint32_t bufferStateIdx = ~0u;
	for (uint32_t i = 0; i < pendingBufferIdentifiers.size(); i++) {
		uint64_t identifier = pendingBufferIdentifiers[i];
		if (identifier == buffer.identifier) {
			bufferStateIdx = i;
			break;
		}
	}

	// If buffer does not have a pending state, create one
	if (bufferStateIdx == ~0u) {

		// Check if we have enough space for another pending state
		if (pendingBufferStates.size() == pendingBufferStates.capacity()) {
			return ZG_ERROR_GENERIC;
		}

		// Create pending buffer state
		bufferStateIdx = pendingBufferStates.size();
		pendingBufferIdentifiers.add(buffer.identifier);
		pendingBufferStates.add(PendingState());

		// Set initial pending buffer state
		pendingBufferStates.last().buffer = &buffer;
		pendingBufferStates.last().neededInitialState = neededState;
		pendingBufferStates.last().currentState = neededState;
	}

	pendingStatesOut = &pendingBufferStates[bufferStateIdx];
	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandList::setBufferState(
	D3D12Buffer& buffer, D3D12_RESOURCE_STATES targetState) noexcept
{
	// Get pending states
	PendingState* pendingState = nullptr;
	ZgErrorCode pendingStateRes = getPendingBufferStates(
		buffer, targetState, pendingState);
	if (pendingStateRes != ZG_SUCCESS) return pendingStateRes;

	// Change state of buffer if necessary
	if (pendingState->currentState != targetState) {
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			buffer.resource.Get(),
			pendingState->currentState,
			targetState);
		commandList->ResourceBarrier(1, &barrier);
		pendingState->currentState = targetState;
	}

	return ZG_SUCCESS;
}

} // namespace zg
