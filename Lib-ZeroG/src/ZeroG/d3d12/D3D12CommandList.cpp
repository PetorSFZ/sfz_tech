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
#include "ZeroG/d3d12/D3D12Memory.hpp"
#include "ZeroG/d3d12/D3D12PipelineRendering.hpp"

namespace zg {

// D3D12CommandList: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12CommandList::~D3D12CommandList() noexcept
{

}

// D3D12CommandList: State methods
// ------------------------------------------------------------------------------------------------

void D3D12CommandList::swap(D3D12CommandList& other) noexcept
{
	std::swap(this->commandAllocator, other.commandAllocator);
	std::swap(this->commandList, other.commandList);
	std::swap(this->fenceValue, other.fenceValue);
}

// D3D12CommandList: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandList::setPipelineRendering(
	IPipelineRendering* pipelineIn) noexcept
{
	D3D12PipelineRendering& pipeline = *reinterpret_cast<D3D12PipelineRendering*>(pipelineIn);
	
	// If a pipeline is already set for this command list, return error. We currently only allow a
	// single pipeline per command list.
	if (mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	mPipelineSet = true;

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

	// Set render target descriptor
	commandList->OMSetRenderTargets(1, &framebuffer.descriptor, FALSE, nullptr);

	return ZG_SUCCESS;
}

ZgErrorCode D3D12CommandList::experimentalCommands(
	IFramebuffer* framebufferIn,
	IBuffer* bufferIn,
	IPipelineRendering* pipelineIn) noexcept
{
	// Cast input to D3D12
	D3D12Buffer& vertexBuffer = *reinterpret_cast<D3D12Buffer*>(bufferIn);


	// TODO: Bad hardcoded vertex buffer information
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = vertexBuffer.resource->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(float) * 6; // TODO: Don't hardcode
	vertexBufferView.SizeInBytes = vertexBufferView.StrideInBytes * 3; // TODO: Don't hardcode



	// Set vertex buffer
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	// Draw
	commandList->DrawInstanced(3, 1, 0, 0);

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

	mPipelineSet = false;
	mFramebufferSet = false;
	return ZG_SUCCESS;
}

} // namespace zg
