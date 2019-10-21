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
#include "ZeroG/BackendInterface.hpp"

#include <mtlpp.hpp>

namespace zg {

// MetalCommandList
// ------------------------------------------------------------------------------------------------

class MetalCommandList final : public ZgCommandList {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	MetalCommandList() = default;
	MetalCommandList(const MetalCommandList&) = delete;
	MetalCommandList& operator= (const MetalCommandList&) = delete;
	MetalCommandList(MetalCommandList&&) = delete;
	MetalCommandList& operator= (MetalCommandList&&) = delete;
	~MetalCommandList() noexcept = default;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult memcpyBufferToBuffer(
		ZgBuffer* dstBuffer,
		uint64_t dstBufferOffsetBytes,
		ZgBuffer* srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept override final;

	ZgResult memcpyToTexture(
		ZgTexture2D* dstTexture,
		uint32_t dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		ZgBuffer* tempUploadBuffer) noexcept override final;

	ZgResult enableQueueTransitionBuffer(ZgBuffer* buffer) noexcept override final;

	ZgResult enableQueueTransitionTexture(ZgTexture2D* texture) noexcept override final;

	ZgResult setPushConstant(
		uint32_t shaderRegister,
		const void* data,
		uint32_t dataSizeInBytes) noexcept override final;

	ZgResult setPipelineBindings(
		const ZgPipelineBindings& bindings) noexcept override final;

	ZgResult setPipelineRender(
		ZgPipelineRender* pipeline) noexcept override final;

	ZgResult setFramebuffer(
		ZgFramebuffer* framebuffer,
		const ZgFramebufferRect* optionalViewport,
		const ZgFramebufferRect* optionalScissor) noexcept override final;

	ZgResult setFramebufferViewport(
		const ZgFramebufferRect& viewport) noexcept override final;

	ZgResult setFramebufferScissor(
		const ZgFramebufferRect& scissor) noexcept override final;

	ZgResult clearFramebufferOptimal() noexcept override final;

	ZgResult clearRenderTargets(
		float red,
		float green,
		float blue,
		float alpha) noexcept override final;

	ZgResult clearDepthBuffer(
		float depth) noexcept override final;

	ZgResult setIndexBuffer(
		ZgBuffer* indexBuffer,
		ZgIndexBufferType type) noexcept override final;

	ZgResult setVertexBuffer(
		uint32_t vertexBufferSlot,
		ZgBuffer* vertexBuffer) noexcept override final;

	ZgResult drawTriangles(
		uint32_t startVertexIndex,
		uint32_t numVertices) noexcept override final;

	ZgResult drawTrianglesIndexed(
		uint32_t startIndex,
		uint32_t numTriangles) noexcept override final;

	// Members
	// --------------------------------------------------------------------------------------------

	mtlpp::CommandBuffer cmdBuffer;
};

} // namespace zg
