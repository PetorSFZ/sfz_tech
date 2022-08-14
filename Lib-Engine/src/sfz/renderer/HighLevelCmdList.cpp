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
	zg::Uploader* uploader,
	zg::Framebuffer* defaultFB,
	SfzStrIDs* ids,
	SfzResourceManager* resMan,
	SfzShaderManager* shaderMan)
{
	this->destroy();
	mName = sfzStrIDCreateRegister(ids, cmdListName);
	mCurrFrameIdx = currFrameIdx;
	mCmdList = sfz_move(cmdList);
	mUploader = uploader;
	mIds = ids;
	mResources = resMan;
	mShaders = shaderMan;
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

	if (mBoundShader->type == SfzShaderType::COMPUTE) {
		CHECK_ZG mCmdList.setPipeline(mBoundShader->computePipeline);
	}
	else {
		CHECK_ZG mCmdList.setPipeline(mBoundShader->renderPipeline);
	}
}

void HighLevelCmdList::setFramebuffer(SfzHandle handle)
{
	SfzFramebufferResource* fb = mResources->getFramebuffer(handle);
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

void HighLevelCmdList::setScissorDefault()
{
	ZgRect rect = {};
	rect.topLeftX = 0;
	rect.topLeftY = 0;
	rect.width = U16_MAX;
	rect.height = U16_MAX;
	CHECK_ZG mCmdList.setFramebufferScissor(rect);
}

void HighLevelCmdList::setScissor(ZgRect rect)
{
	CHECK_ZG mCmdList.setFramebufferScissor(rect);
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

		SfzBufferResource* bufferRes = nullptr;
		ZgBuffer* buffer = nullptr;
		if (binding.type == ZG_BINDING_TYPE_BUFFER_CONST ||
			binding.type == ZG_BINDING_TYPE_BUFFER_TYPED ||
			binding.type == ZG_BINDING_TYPE_BUFFER_STRUCTURED ||
			binding.type == ZG_BINDING_TYPE_BUFFER_STRUCTURED_UAV ||
			binding.type == ZG_BINDING_TYPE_BUFFER_BYTEADDRESS ||
			binding.type == ZG_BINDING_TYPE_BUFFER_BYTEADDRESS_UAV) {

			bufferRes = mResources->getBuffer(binding.handle);
			sfz_assert(bufferRes != nullptr);
			sfz_assert(binding.reg != ~0u);

			buffer = nullptr;
			if (bufferRes->type == SfzBufferResourceType::STATIC) {
				buffer = bufferRes->staticMem.buffer.handle;
			}
			else if (bufferRes->type == SfzBufferResourceType::STREAMING) {
				buffer = bufferRes->streamingMem.data(mCurrFrameIdx).buffer.handle;
			}
			else {
				sfz_assert_hard(false);
			}
		}

		if (binding.type == ZG_BINDING_TYPE_BUFFER_CONST) {
			zgBindings.addBufferConst(binding.reg, buffer);
		}

		else if (binding.type == ZG_BINDING_TYPE_BUFFER_TYPED) {
			zgBindings.addBufferTyped(binding.reg, buffer, binding.format, bufferRes->maxNumElements);
		}

		else if (binding.type == ZG_BINDING_TYPE_BUFFER_STRUCTURED) {
			zgBindings.addBufferStructured(binding.reg, buffer, bufferRes->elementSizeBytes, bufferRes->maxNumElements);
		}

		else if (binding.type == ZG_BINDING_TYPE_BUFFER_STRUCTURED_UAV) {
			zgBindings.addBufferStructuredUAV(binding.reg, buffer, bufferRes->elementSizeBytes, bufferRes->maxNumElements);
		}

		else if (binding.type == ZG_BINDING_TYPE_BUFFER_BYTEADDRESS) {
			zgBindings.addBufferByteAddress(binding.reg, buffer);
		}

		else if (binding.type == ZG_BINDING_TYPE_BUFFER_BYTEADDRESS_UAV) {
			zgBindings.addBufferByteAddressUAV(binding.reg, buffer);
		}

		else if (binding.type == ZG_BINDING_TYPE_TEXTURE) {
			SfzTextureResource* resource = mResources->getTexture(binding.handle);
			sfz_assert(resource != nullptr);
			sfz_assert(binding.reg != ~0u);
			zgBindings.addTexture(binding.reg, resource->texture.handle);
		}

		else if (binding.type == ZG_BINDING_TYPE_TEXTURE_UAV) {
			SfzTextureResource* resource = mResources->getTexture(binding.handle);
			sfz_assert(resource != nullptr);
			sfz_assert(binding.reg != ~0u);
			sfz_assert(binding.mipLevel < resource->numMipmaps);
			zgBindings.addTextureUAV(binding.reg, resource->texture.handle, binding.mipLevel);
		}
	}

	CHECK_ZG mCmdList.setPipelineBindings(zgBindings);
}

void HighLevelCmdList::uploadToStreamingBufferUntyped(
	SfzHandle handle, const void* data, u32 elementSize, u32 numElements)
{
	// Get streaming buffer
	SfzBufferResource* resource = mResources->getBuffer(handle);
	sfz_assert(resource != nullptr);
	sfz_assert(resource->type == SfzBufferResourceType::STREAMING);

	// Calculate number of bytes to copy to streaming buffer
	const u32 numBytes = elementSize * numElements;
	sfz_assert(numBytes != 0);
	sfz_assert(numBytes <= (resource->elementSizeBytes * resource->maxNumElements));
	sfz_assert(elementSize == resource->elementSizeBytes); // TODO: Might want to remove this assert

	// Grab this frame's memory
	SfzStreamingBufferMemory& memory = resource->streamingMem.data(mCurrFrameIdx);

	// Only allowed to upload to streaming buffer once per frame
	sfz_assert(memory.lastFrameIdxTouched < mCurrFrameIdx);
	memory.lastFrameIdxTouched = mCurrFrameIdx;
	
	// Upload to buffer
	CHECK_ZG mCmdList.uploadToBuffer(mUploader->handle, memory.buffer.handle, 0, data, numBytes);
}

void HighLevelCmdList::setIndexBuffer(SfzHandle handle, ZgIndexBufferType indexType)
{
	SfzBufferResource* resource = mResources->getBuffer(handle);
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == SfzBufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == SfzBufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mCurrFrameIdx).buffer;
	}
	else {
		sfz_assert_hard(false);
	}

	CHECK_ZG mCmdList.setIndexBuffer(*buffer, indexType);
}

void HighLevelCmdList::drawTriangles(u32 startVertex, u32 numVertices)
{
	sfz_assert(mBoundShader != nullptr);
	sfz_assert(mBoundShader->type == SfzShaderType::RENDER);
	CHECK_ZG mCmdList.drawTriangles(startVertex, numVertices);
}

void HighLevelCmdList::drawTrianglesIndexed(u32 firstIndex, u32 numIndices)
{
	sfz_assert(mBoundShader != nullptr);
	sfz_assert(mBoundShader->type == SfzShaderType::RENDER);
	CHECK_ZG mCmdList.drawTrianglesIndexed(firstIndex, numIndices);
}

i32x3 HighLevelCmdList::getComputeGroupDims() const
{
	sfz_assert(mBoundShader != nullptr);
	sfz_assert(mBoundShader->type == SfzShaderType::COMPUTE);
	u32 x = 0, y = 0, z = 0;
	mBoundShader->computePipeline.getGroupDims(x, y, z);
	return i32x3_init(x, y, z);
}

void HighLevelCmdList::dispatchCompute(i32 groupCountX, i32 groupCountY, i32 groupCountZ)
{
	sfz_assert(mBoundShader != nullptr);
	sfz_assert(mBoundShader->type == SfzShaderType::COMPUTE);
	sfz_assert(groupCountX > 0);
	sfz_assert(groupCountY > 0);
	sfz_assert(groupCountZ > 0);
	CHECK_ZG mCmdList.dispatchCompute(groupCountX, groupCountY, groupCountZ);
}

void HighLevelCmdList::uavBarrierAll()
{
	CHECK_ZG mCmdList.uavBarrier();
}

void HighLevelCmdList::uavBarrierBuffer(SfzHandle handle)
{
	SfzBufferResource* resource = mResources->getBuffer(handle);
	sfz_assert(resource != nullptr);

	zg::Buffer* buffer = nullptr;
	if (resource->type == SfzBufferResourceType::STATIC) {
		buffer = &resource->staticMem.buffer;
	}
	else if (resource->type == SfzBufferResourceType::STREAMING) {
		buffer = &resource->streamingMem.data(mCurrFrameIdx).buffer;
	}
	else {
		sfz_assert_hard(false);
	}

	CHECK_ZG mCmdList.uavBarrier(*buffer);
}

void HighLevelCmdList::uavBarrierTexture(SfzHandle handle)
{
	SfzTextureResource* resource = mResources->getTexture(handle);
	sfz_assert(resource != nullptr);
	CHECK_ZG mCmdList.uavBarrier(resource->texture);
}

} // namespace sfz
