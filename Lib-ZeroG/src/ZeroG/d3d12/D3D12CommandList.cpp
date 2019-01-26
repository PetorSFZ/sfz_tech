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

// State methods
// ------------------------------------------------------------------------------------------------

void D3D12CommandList::swap(D3D12CommandList& other) noexcept
{
	std::swap(this->commandAllocator, other.commandAllocator);
	std::swap(this->commandList, other.commandList);
	std::swap(this->fenceValue, other.fenceValue);
}

// D3D12CommandList: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode D3D12CommandList::experimentalCommands(
	IFramebuffer* framebufferIn,
	IBuffer* bufferIn,
	IPipelineRendering* pipelineIn) noexcept
{
	// Cast input to D3D12
	D3D12Framebuffer& framebuffer = *reinterpret_cast<D3D12Framebuffer*>(framebufferIn);
	D3D12Buffer& vertexBuffer = *reinterpret_cast<D3D12Buffer*>(bufferIn);
	D3D12PipelineRendering& pipeline = *reinterpret_cast<D3D12PipelineRendering*>(pipelineIn);


	// TODO: Bad hardcoded vertex buffer information
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = vertexBuffer.resource->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(float) * 6; // TODO: Don't hardcode
	vertexBufferView.SizeInBytes = vertexBufferView.StrideInBytes * 3; // TODO: Don't hardcode


	// Set viewport
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = float(framebuffer.width);
	viewport.Height = float(framebuffer.height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	// Set scissor rects
	D3D12_RECT scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = LONG_MAX;
	scissorRect.bottom = LONG_MAX;
	commandList->RSSetScissorRects(1, &scissorRect);


	// Set render target descriptor
	commandList->OMSetRenderTargets(1, &framebuffer.descriptor, FALSE, nullptr);

	// Set pipeline
	commandList->SetPipelineState(pipeline.pipelineState.Get());
	commandList->SetGraphicsRootSignature(pipeline.rootSignature.Get());

	// Set vertex buffer
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	// Draw
	commandList->DrawInstanced(3, 1, 0, 0);

	return ZG_SUCCESS;
}

} // namespace zg
