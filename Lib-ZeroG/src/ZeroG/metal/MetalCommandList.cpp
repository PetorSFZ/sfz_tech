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

#include "ZeroG/metal/MetalCommandList.hpp"

namespace zg {

// MetalCommandList: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgResult MetalCommandList::memcpyBufferToBuffer(
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes) noexcept
{
	(void)dstBuffer;
	(void)dstBufferOffsetBytes;
	(void)srcBuffer;
	(void)srcBufferOffsetBytes;
	(void)numBytes;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::memcpyToTexture(
	ZgTexture2D* dstTexture,
	uint32_t dstTextureMipLevel,
	const ZgImageViewConstCpu& srcImageCpu,
	ZgBuffer* tempUploadBuffer) noexcept
{
	(void)dstTexture;
	(void)dstTextureMipLevel;
	(void)srcImageCpu;
	(void)tempUploadBuffer;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::enableQueueTransitionBuffer(ZgBuffer* buffer) noexcept
{
	(void)buffer;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::enableQueueTransitionTexture(ZgTexture2D* texture) noexcept
{
	(void)texture;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::setPushConstant(
	uint32_t shaderRegister,
	const void* data,
	uint32_t dataSizeInBytes) noexcept
{
	(void)shaderRegister;
	(void)data;
	(void)dataSizeInBytes;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::setPipelineBindings(
	const ZgPipelineBindings& bindings) noexcept
{
	(void)bindings;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::setPipelineRender(
	ZgPipelineRender* pipeline) noexcept
{
	(void)pipeline;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::setFramebuffer(
	ZgFramebuffer* framebuffer,
	const ZgFramebufferRect* optionalViewport,
	const ZgFramebufferRect* optionalScissor) noexcept
{
	(void)framebuffer;
	(void)optionalViewport;
	(void)optionalScissor;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::setFramebufferViewport(
	const ZgFramebufferRect& viewport) noexcept
{
	(void)viewport;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::setFramebufferScissor(
	const ZgFramebufferRect& scissor) noexcept
{
	(void)scissor;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::clearFramebufferOptimal() noexcept
{
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::clearRenderTargets(
	float red,
	float green,
	float blue,
	float alpha) noexcept
{
	(void)red;
	(void)green;
	(void)blue;
	(void)alpha;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::clearDepthBuffer(
	float depth) noexcept
{
	(void)depth;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::setIndexBuffer(
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type) noexcept
{
	(void)indexBuffer;
	(void)type;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::setVertexBuffer(
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBuffer) noexcept
{
	(void)vertexBufferSlot;
	(void)vertexBuffer;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::drawTriangles(
	uint32_t startVertexIndex,
	uint32_t numVertices) noexcept
{
	(void)startVertexIndex;
	(void)numVertices;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult MetalCommandList::drawTrianglesIndexed(
	uint32_t startIndex,
	uint32_t numTriangles) noexcept
{
	(void)startIndex;
	(void)numTriangles;
	return ZG_WARNING_UNIMPLEMENTED;
}

} // namespace zg
