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

namespace zg {

// D3D12CommandList: State methods
// ------------------------------------------------------------------------------------------------

void D3D12CommandList::create(uint32_t maxNumBuffers, ZgAllocator allocator) noexcept
{
	pendingBufferIdentifiers.create(maxNumBuffers, allocator, "ZeroG - D3D12CommandList - Internal");
	pendingBufferStates.create(maxNumBuffers, allocator, "ZeroG - D3D12CommandList - Internal");
}

void D3D12CommandList::swap(D3D12CommandList& other) noexcept
{
	std::swap(this->commandAllocator, other.commandAllocator);
	std::swap(this->commandList, other.commandList);
	std::swap(this->fenceValue, other.fenceValue);

	this->pendingBufferIdentifiers.swap(other.pendingBufferIdentifiers);
	this->pendingBufferStates.swap(other.pendingBufferStates);

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

	pendingBufferIdentifiers.destroy();
	pendingBufferStates.destroy();

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

	// Get pending states
	PendingState dstPendingState;
	ZgErrorCode dstPendingStateRes =
		getPendingBufferStates(dstBuffer, dstTargetState, dstPendingState);
	if (dstPendingStateRes != ZG_SUCCESS) return dstPendingStateRes;

	PendingState srcPendingState;
	ZgErrorCode srcPendingStateRes =
		getPendingBufferStates(srcBuffer, srcTargetState, srcPendingState);
	if (srcPendingStateRes != ZG_SUCCESS) return srcPendingStateRes;

	// Change state of destination buffer to COPY_DEST if necessary
	if (dstPendingState.currentState != dstTargetState) {
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			dstBuffer.resource.Get(),
			dstPendingState.currentState,
			dstTargetState);
		commandList->ResourceBarrier(1, &barrier);
	}

	// Change state of source buffer to COPY_SOURCE if necessary
	if (srcPendingState.currentState != srcTargetState) {
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			srcBuffer.resource.Get(),
			srcPendingState.currentState,
			srcTargetState);
		commandList->ResourceBarrier(1, &barrier);
	}

	// Check if we should copy entire buffer or just a region of it
	bool copyEntireBuffer =
		dstBuffer.sizeBytes == srcBuffer.sizeBytes &&
		dstBuffer.sizeBytes == numBytes &&
		dstBufferOffsetBytes == 0 &&
		srcBufferOffsetBytes == 0;

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
	uint32_t parameterIndex,
	const void* dataPtr) noexcept
{
	// Require that a pipeline has been set so we can query its parameters
	if (!mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Return invalid argument if parameter index is out of bounds
	const ZgPipelineRenderingCreateInfo& pipelineInfo = mBoundPipeline->createInfo;
	if (parameterIndex >= pipelineInfo.numParameters) return ZG_ERROR_INVALID_ARGUMENT;

	const ZgPipeplineParameterPushConstant& constInfo =
		pipelineInfo.parameters[parameterIndex].pushConstant;
	if (constInfo.sizeInWords == 1) {
		uint32_t data = *reinterpret_cast<const uint32_t*>(dataPtr);
		commandList->SetGraphicsRoot32BitConstant(parameterIndex, data, 0);
	}
	else {
		commandList->SetGraphicsRoot32BitConstants(parameterIndex, constInfo.sizeInWords, dataPtr, 0);
	}

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

	// Need to have a command list set to verify vertex buffer binding
	if (!mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Check that the vertex buffer slot is not out of bounds for the bound pipeline
	const ZgPipelineRenderingCreateInfo& pipelineInfo = mBoundPipeline->createInfo;
	if (pipelineInfo.numVertexBufferSlots <= vertexBufferSlot) {
		return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	}

	// Create vertex buffer view
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = vertexBuffer.resource->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = pipelineInfo.vertexBufferStridesBytes[vertexBufferSlot];
	vertexBufferView.SizeInBytes = uint32_t(vertexBuffer.sizeBytes);

	// Set vertex buffer
	commandList->IASetVertexBuffers(vertexBufferSlot, 1, &vertexBufferView);

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
	if (!CHECK_D3D12_SUCCEEDED(commandAllocator->Reset())) {
		return ZG_ERROR_GENERIC;
	}
	if (!CHECK_D3D12_SUCCEEDED(
		commandList->Reset(commandAllocator.Get(), nullptr))) {
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
	PendingState& pendingStatesOut) noexcept
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

	pendingStatesOut = pendingBufferStates[bufferStateIdx];
	return ZG_SUCCESS;
}

} // namespace zg
