// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include "sfz/renderer/HighLevelCmdList.hpp"

#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/resources/BufferResource.hpp"
#include "sfz/resources/FramebufferResource.hpp"
#include "sfz/resources/TextureResource.hpp"

namespace sfz {

// HighLevelCmdList
// ------------------------------------------------------------------------------------------------

void HighLevelCmdList::init(
	const char* cmdListName,
	u64 currFrameIdx,
	zg::CommandList cmdList,
	zg::Framebuffer* defaultFB)
{
	this->destroy();
	mName = strID(cmdListName);
	mCurrFrameIdx = currFrameIdx;
	mCmdList = sfz_move(cmdList);
	mResources = &sfz::getResourceManager();
	mShaders = &sfz::getShaderManager();
	mDefaultFB = defaultFB;
}

void HighLevelCmdList::destroy() noexcept
{
	mName = {};
	mCurrFrameIdx = 0;
	mCmdList.destroy();
	mResources = nullptr;
	mShaders = nullptr;
	mBoundShader = nullptr;
	mDefaultFB = nullptr;
}

// HighLevelCmdList: Methods
// ------------------------------------------------------------------------------------------------

void HighLevelCmdList::setShader(SfzHandle handle)
{
	mBoundShader = mShaders->getShader(handle);
	sfz_assert(mBoundShader != nullptr);

	if (mBoundShader->type == ShaderType::COMPUTE) {
		CHECK_ZG mCmdList.setPipeline(mBoundShader->compute.pipeline);
	}
	else {
		CHECK_ZG mCmdList.setPipeline(mBoundShader->render.pipeline);
	}
}

void HighLevelCmdList::setFramebuffer(SfzHandle handle)
{
	FramebufferResource* fb = mResources->getFramebuffer(handle);
	sfz_assert(fb != nullptr);
	CHECK_ZG mCmdList.setFramebuffer(fb->framebuffer);
}

void HighLevelCmdList::setFramebufferDefault()
{
	CHECK_ZG mCmdList.setFramebuffer(*mDefaultFB);
}

void HighLevelCmdList::clearRenderTargetsOptimal()
{
	CHECK_ZG mCmdList.clearRenderTargetsOptimal();
}

void HighLevelCmdList::clearDepthBufferOptimal()
{
	CHECK_ZG mCmdList.clearDepthBufferOptimal();
}

void HighLevelCmdList::setPushConstantUntyped(u32 reg, const void* data, u32 numBytes)
{
	sfz_assert(numBytes > 0);
	sfz_assert(numBytes <= 128);
	CHECK_ZG mCmdList.setPushConstant(reg, data, numBytes);
}

void HighLevelCmdList::setBindings(const Bindings& bindings)
{
	ZgPipelineBindings zgBindings = {};

	for (const BindingHL& binding : bindings.bindings) {

		if (binding.type == ZG_BINDING_TYPE_CONST_BUFFER) {
			BufferResource* resource = mResources->getBuffer(binding.handle);
			sfz_assert(resource != nullptr);
			sfz_assert(binding.reg != ~0u);

			ZgBuffer* buffer = nullptr;
			if (resource->type == BufferResourceType::STATIC) {
				buffer = resource->staticMem.buffer.handle;
			}
			else if (resource->type == BufferResourceType::STREAMING) {
				buffer = resource->streamingMem.data(mCurrFrameIdx).deviceBuffer.handle;
			}
			else {
				sfz_assert_hard(false);
			}

			zgBindings.addConstBuffer(binding.reg, buffer);
		}

		else if (binding.type == ZG_BINDING_TYPE_UNORDERED_BUFFER) {
			BufferResource* resource = mResources->getBuffer(binding.handle);
			sfz_assert(resource != nullptr);
			sfz_assert(binding.reg != ~0u);

			ZgBuffer* buffer = nullptr;
			if (resource->type == BufferResourceType::STATIC) {
				buffer = resource->staticMem.buffer.handle;
			}
			else if (resource->type == BufferResourceType::STREAMING) {
				buffer = resource->streamingMem.data(mCurrFrameIdx).deviceBuffer.handle;
			}
			else {
				sfz_assert_hard(false);
			}

			zgBindings.addUnorderedBuffer(binding.reg, buffer, resource->elementSizeBytes, resource->maxNumElements);
		}

		else if (binding.type == ZG_BINDING_TYPE_TEXTURE) {
			TextureResource* resource = mResources->getTexture(binding.handle);
			sfz_assert(resource != nullptr);
			sfz_assert(binding.reg != ~0u);
			zgBindings.addTexture(binding.reg, resource->texture.handle);
		}

		else if (binding.type == ZG_BINDING_TYPE_UNORDERED_TEXTURE) {
			TextureResource* resource = mResources->getTexture(binding.handle);
			sfz_assert(resource != nullptr);
			sfz_assert(binding.reg != ~0u);
			sfz_assert(binding.mipLevel < resource->numMipmaps);
			zgBindings.addUnorderedTexture(binding.reg, resource->texture.handle, binding.mipLevel);
		}
	}

	CHECK_ZG mCmdList.setPipelineBindings(zgBindings);
}

void HighLevelCmdList::uploadToStreamingBufferUntyped(
	SfzHandle handle, const void* data, u32 elementSize, u32 numElements)
{
	// Get streaming buffer
	BufferResource* resource = mResources->getBuffer(handle);
	sfz_assert(resource != nullptr);
	sfz_assert(resource->type == BufferResourceType::STREAMING);

	// Calculate number of bytes to copy to streaming buffer
	const u32 numBytes = elementSize * numElements;
	sfz_assert(numBytes != 0);
	sfz_assert(numBytes <= (resource->elementSizeBytes * resource->maxNumElements));
	sfz_assert(elementSize == resource->elementSizeBytes); // TODO: Might want to remove this assert

	// Grab this frame's memory
	StreamingBufferMemory& memory = resource->streamingMem.data(mCurrFrameIdx);

	// Only allowed to upload to streaming buffer once per frame
	sfz_assert(memory.lastFrameIdxTouched < mCurrFrameIdx);
	memory.lastFrameIdxTouched = mCurrFrameIdx;

	// Memcpy to upload buffer
	CHECK_ZG memory.uploadBuffer.memcpyUpload(0, data, numBytes);

	// Schedule memcpy from upload buffer to device buffer
	CHECK_ZG mCmdList.memcpyBufferToBuffer(memory.deviceBuffer, 0, memory.uploadBuffer, 0, numBytes);
}

void HighLevelCmdList::setVertexBuffer(u32 slot, SfzHandle handle)
{
	BufferResource* resource = mResources->getBuffer(handle);
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == BufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == BufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mCurrFrameIdx).deviceBuffer;
	}
	else {
		sfz_assert_hard(false);
	}

	CHECK_ZG mCmdList.setVertexBuffer(slot, *buffer);
}

void HighLevelCmdList::setIndexBuffer(SfzHandle handle, ZgIndexBufferType indexType)
{
	BufferResource* resource = mResources->getBuffer(handle);
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == BufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == BufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mCurrFrameIdx).deviceBuffer;
	}
	else {
		sfz_assert_hard(false);
	}

	CHECK_ZG mCmdList.setIndexBuffer(*buffer, indexType);
}

void HighLevelCmdList::drawTriangles(u32 startVertex, u32 numVertices)
{
	sfz_assert(mBoundShader != nullptr);
	sfz_assert(mBoundShader->type == ShaderType::RENDER);
	CHECK_ZG mCmdList.drawTriangles(startVertex, numVertices);
}

void HighLevelCmdList::drawTrianglesIndexed(u32 firstIndex, u32 numIndices)
{
	sfz_assert(mBoundShader != nullptr);
	sfz_assert(mBoundShader->type == ShaderType::RENDER);
	CHECK_ZG mCmdList.drawTrianglesIndexed(firstIndex, numIndices);
}

i32x3 HighLevelCmdList::getComputeGroupDims() const
{
	sfz_assert(mBoundShader != nullptr);
	sfz_assert(mBoundShader->type == ShaderType::COMPUTE);
	u32 x = 0, y = 0, z = 0;
	mBoundShader->compute.pipeline.getGroupDims(x, y, z);
	return i32x3(x, y, z);
}

void HighLevelCmdList::dispatchCompute(i32 groupCountX, i32 groupCountY, i32 groupCountZ)
{
	sfz_assert(mBoundShader != nullptr);
	sfz_assert(mBoundShader->type == ShaderType::COMPUTE);
	sfz_assert(groupCountX > 0);
	sfz_assert(groupCountY > 0);
	sfz_assert(groupCountZ > 0);
	CHECK_ZG mCmdList.dispatchCompute(groupCountX, groupCountY, groupCountZ);
}

void HighLevelCmdList::unorderedBarrierAll()
{
	CHECK_ZG mCmdList.unorderedBarrier();
}

void HighLevelCmdList::unorderedBarrierBuffer(SfzHandle handle)
{
	BufferResource* resource = mResources->getBuffer(handle);
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == BufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == BufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mCurrFrameIdx).deviceBuffer;
	}
	else {
		sfz_assert_hard(false);
	}

	CHECK_ZG mCmdList.unorderedBarrier(*buffer);
}

void HighLevelCmdList::unorderedBarrierTexture(SfzHandle handle)
{
	TextureResource* resource = mResources->getTexture(handle);
	sfz_assert(resource != nullptr);
	CHECK_ZG mCmdList.unorderedBarrier(resource->texture);
}

} // namespace sfz
