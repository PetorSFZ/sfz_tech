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

	ZgErrorCode memcpyBufferToBuffer(
		ZgBuffer* dstBuffer,
		uint64_t dstBufferOffsetBytes,
		ZgBuffer* srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept override final;

	ZgErrorCode memcpyToTexture(
		ZgTexture2D* dstTexture,
		uint32_t dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		ZgBuffer* tempUploadBuffer) noexcept override final;

	ZgErrorCode enableQueueTransitionBuffer(ZgBuffer* buffer) noexcept override final;

	ZgErrorCode enableQueueTransitionTexture(ZgTexture2D* texture) noexcept override final;

	ZgErrorCode setPushConstant(
		uint32_t shaderRegister,
		const void* data,
		uint32_t dataSizeInBytes) noexcept override final;

	ZgErrorCode setPipelineBindings(
		const ZgPipelineBindings& bindings) noexcept override final;

	ZgErrorCode setPipelineRender(
		ZgPipelineRender* pipeline) noexcept override final;

	ZgErrorCode setFramebuffer(
		ZgFramebuffer* framebuffer,
		const ZgFramebufferRect* optionalViewport,
		const ZgFramebufferRect* optionalScissor) noexcept override final;

	ZgErrorCode setFramebufferViewport(
		const ZgFramebufferRect& viewport) noexcept override final;

	ZgErrorCode setFramebufferScissor(
		const ZgFramebufferRect& scissor) noexcept override final;

	ZgErrorCode clearFramebufferOptimal() noexcept override final;

	ZgErrorCode clearRenderTargets(
		float red,
		float green,
		float blue,
		float alpha) noexcept override final;

	ZgErrorCode clearDepthBuffer(
		float depth) noexcept override final;

	ZgErrorCode setIndexBuffer(
		ZgBuffer* indexBuffer,
		ZgIndexBufferType type) noexcept override final;

	ZgErrorCode setVertexBuffer(
		uint32_t vertexBufferSlot,
		ZgBuffer* vertexBuffer) noexcept override final;

	ZgErrorCode drawTriangles(
		uint32_t startVertexIndex,
		uint32_t numVertices) noexcept override final;

	ZgErrorCode drawTrianglesIndexed(
		uint32_t startIndex,
		uint32_t numTriangles) noexcept override final;

	// Members
	// --------------------------------------------------------------------------------------------

	mtlpp::CommandBuffer cmdBuffer;
};

} // namespace zg
