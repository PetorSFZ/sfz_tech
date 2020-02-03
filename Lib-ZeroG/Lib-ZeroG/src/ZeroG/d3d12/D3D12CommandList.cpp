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

#include "ZeroG/d3d12/D3D12Memory.hpp"
#include "ZeroG/d3d12/D3D12Profiler.hpp"
#include "ZeroG/util/ErrorReporting.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

static uint32_t numBytesPerPixelForFormat(ZgTextureFormat format) noexcept
{
	switch (format) {
	case ZG_TEXTURE_FORMAT_R_U8_UNORM: return 1 * sizeof(uint8_t);
	case ZG_TEXTURE_FORMAT_RG_U8_UNORM: return 2 * sizeof(uint8_t);
	case ZG_TEXTURE_FORMAT_RGBA_U8_UNORM: return 4 * sizeof(uint8_t);

	case ZG_TEXTURE_FORMAT_R_F16: return 1 * sizeof(uint16_t);
	case ZG_TEXTURE_FORMAT_RG_F16: return 2 * sizeof(uint16_t);
	case ZG_TEXTURE_FORMAT_RGBA_F16: return 4 * sizeof(uint16_t);

	case ZG_TEXTURE_FORMAT_R_F32: return 1 * sizeof(float);
	case ZG_TEXTURE_FORMAT_RG_F32: return 2 * sizeof(float);
	case ZG_TEXTURE_FORMAT_RGBA_F32: return 4 * sizeof(float);
	}
	sfz_assert(false);
	return 0;
}

// D3D12CommandList: State methods
// ------------------------------------------------------------------------------------------------

void D3D12CommandList::create(
	uint32_t maxNumBuffers,
	ComPtr<ID3D12Device3> device,
	D3DX12Residency::ResidencyManager* residencyManager,
	D3D12DescriptorRingBuffer* descriptorBuffer) noexcept
{
	mDevice = device;
	mDescriptorBuffer = descriptorBuffer;
	pendingBufferIdentifiers.init(maxNumBuffers, getAllocator(), sfz_dbg("ZeroG - D3D12CommandList - Internal"));
	pendingBufferStates.init(maxNumBuffers, getAllocator(), sfz_dbg("ZeroG - D3D12CommandList - Internal"));
	pendingTextureIdentifiers.init(maxNumBuffers, getAllocator(), sfz_dbg("ZeroG - D3D12CommandList - Internal"));
	pendingTextureStates.init(maxNumBuffers, getAllocator(), sfz_dbg("ZeroG - D3D12CommandList - Internal"));

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
	this->pendingTextureIdentifiers.swap(other.pendingTextureIdentifiers);
	this->pendingTextureStates.swap(other.pendingTextureStates);

	std::swap(this->mDevice, other.mDevice);
	std::swap(this->mResidencyManager, other.mResidencyManager);
	std::swap(this->mDescriptorBuffer, other.mDescriptorBuffer);
	std::swap(this->mPipelineSet, other.mPipelineSet);
	std::swap(this->mBoundPipelineRender, other.mBoundPipelineRender);
	std::swap(this->mBoundPipelineCompute, other.mBoundPipelineCompute);
	std::swap(this->mFramebufferSet, other.mFramebufferSet);
	std::swap(this->mFramebuffer, other.mFramebuffer);
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
	pendingTextureIdentifiers.destroy();
	pendingTextureStates.destroy();

	mDevice = nullptr;
	mResidencyManager = nullptr;
	mDescriptorBuffer = nullptr;
	mPipelineSet = false;
	mBoundPipelineRender = nullptr;
	mBoundPipelineCompute = nullptr;
	mFramebufferSet = false;
	mFramebuffer = nullptr;
}

// D3D12CommandList: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgResult D3D12CommandList::memcpyBufferToBuffer(
	ZgBuffer* dstBufferIn,
	uint64_t dstBufferOffsetBytes,
	ZgBuffer* srcBufferIn,
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
	if (srcBuffer.memoryHeap->memoryType == ZG_MEMORY_TYPE_UPLOAD) {
		srcTargetState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	// Set buffer resource states
	ZgResult res = setBufferState(dstBuffer, dstTargetState);
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
	residencySet->Insert(&srcBuffer.memoryHeap->managedObject);
	residencySet->Insert(&dstBuffer.memoryHeap->managedObject);

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

ZgResult D3D12CommandList::memcpyToTexture(
	ZgTexture2D* dstTextureIn,
	uint32_t dstTextureMipLevel,
	const ZgImageViewConstCpu& srcImageCpu,
	ZgBuffer* tempUploadBufferIn) noexcept
{
	// Cast input to D3D12
	D3D12Texture2D& dstTexture = *reinterpret_cast<D3D12Texture2D*>(dstTextureIn);
	D3D12Buffer& tmpBuffer = *reinterpret_cast<D3D12Buffer*>(tempUploadBufferIn);

	// Check that mip level is valid
	if (dstTextureMipLevel >= dstTexture.numMipmaps) return ZG_ERROR_INVALID_ARGUMENT;

	// Calculate width and height of this mip level
	uint32_t dstTexMipWidth = dstTexture.width;
	uint32_t dstTexMipHeight = dstTexture.height;
	for (uint32_t i = 0; i < dstTextureMipLevel; i++) {
		dstTexMipWidth /= 2;
		dstTexMipHeight /= 2;
	}

	// Check that CPU image has correct dimensions and format
	if (srcImageCpu.format != dstTexture.zgFormat) return ZG_ERROR_INVALID_ARGUMENT;
	if (srcImageCpu.width != dstTexMipWidth) return ZG_ERROR_INVALID_ARGUMENT;
	if (srcImageCpu.height != dstTexMipHeight) return ZG_ERROR_INVALID_ARGUMENT;
	
	// Check that temp buffer is upload
	if (tmpBuffer.memoryHeap->memoryType != ZG_MEMORY_TYPE_UPLOAD) return ZG_ERROR_INVALID_ARGUMENT;

	// Check that upload buffer is big enough
	uint32_t numBytesPerPixel = numBytesPerPixelForFormat(srcImageCpu.format);
	uint32_t numBytesPerRow = srcImageCpu.width * numBytesPerPixel;
	uint32_t tmpBufferPitch = ((numBytesPerRow + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) /
		D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
	uint32_t tmpBufferRequiredSize = tmpBufferPitch * srcImageCpu.height;
	if (tmpBuffer.sizeBytes < tmpBufferRequiredSize) {
		ZG_ERROR("Temporary buffer is too small, it is %u bytes, but %u bytes is required."
			" The pitch of the upload buffer is required to be %u byte aligned.",
			tmpBuffer.sizeBytes,
			tmpBufferRequiredSize,
			D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		return ZG_ERROR_INVALID_ARGUMENT;
	}

	// Not gonna read from temp buffer
	D3D12_RANGE readRange = {};
	readRange.Begin = 0;
	readRange.End = 0;

	// Map buffer
	void* mappedPtr = nullptr;
	if (D3D12_FAIL(tmpBuffer.resource->Map(0, &readRange, &mappedPtr))) {
		return ZG_ERROR_GENERIC;
	}

	// Memcpy cpu image to tmp buffer
	for (uint32_t y = 0; y < srcImageCpu.height; y++) {
		const uint8_t* rowPtr = ((const uint8_t*)srcImageCpu.data) + srcImageCpu.pitchInBytes * y;
		uint8_t* dstPtr = reinterpret_cast<uint8_t*>(mappedPtr) + tmpBufferPitch * y;
		memcpy(dstPtr, rowPtr, numBytesPerRow);
	}
	
	// Unmap buffer
	tmpBuffer.resource->Unmap(0, nullptr);

	// Set texture resource state
	ZgResult stateRes = setTextureState(dstTexture, dstTextureMipLevel, D3D12_RESOURCE_STATE_COPY_DEST);
	if (stateRes != ZG_SUCCESS) return stateRes;

	// Insert into residency set
	residencySet->Insert(&tmpBuffer.memoryHeap->managedObject);
	residencySet->Insert(&dstTexture.textureHeap->managedObject);

	// Issue copy command
	D3D12_TEXTURE_COPY_LOCATION tmpCopyLoc = {};
	tmpCopyLoc.pResource = tmpBuffer.resource.Get();
	tmpCopyLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	tmpCopyLoc.PlacedFootprint = dstTexture.subresourceFootprints[dstTextureMipLevel];
	
	// TODO: THIS IS A HACK
	// Essentially, in D3D12 you are meant to upload all of your subresources (i.e. mip levels)
	// at the same time. All of these mip levels will be in THE SAME temporary upload buffer. What
	// we instead have done here is said that each mip level will be in its own temporary upload
	// buffer, thus we need to modify the placed footprint so that it does not have an offset.
	tmpCopyLoc.PlacedFootprint.Offset = 0; 

	D3D12_TEXTURE_COPY_LOCATION dstCopyLoc = {};
	dstCopyLoc.pResource = dstTexture.resource.Get();
	dstCopyLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstCopyLoc.SubresourceIndex = dstTextureMipLevel;

	commandList->CopyTextureRegion(&dstCopyLoc, 0, 0, 0, &tmpCopyLoc, nullptr);

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::enableQueueTransitionBuffer(ZgBuffer* bufferIn) noexcept
{
	// Cast to D3D12
	D3D12Buffer& buffer = *reinterpret_cast<D3D12Buffer*>(bufferIn);

	// Check that it is a device buffer
	if (buffer.memoryHeap->memoryType == ZG_MEMORY_TYPE_UPLOAD ||
		buffer.memoryHeap->memoryType == ZG_MEMORY_TYPE_DOWNLOAD) {
		ZG_ERROR("enableQueueTransitionBuffer(): Can't transition upload and download buffers");
		return ZG_ERROR_INVALID_ARGUMENT;
	}

	// Set buffer resource state
	ZgResult res = setBufferState(buffer, D3D12_RESOURCE_STATE_COMMON);
	if (res != ZG_SUCCESS) return res;

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::enableQueueTransitionTexture(ZgTexture2D* textureIn) noexcept
{
	// Cast to D3D12
	D3D12Texture2D& texture = *reinterpret_cast<D3D12Texture2D*>(textureIn);

	// Set buffer resource state
	ZgResult res = setTextureStateAllMipLevels(texture, D3D12_RESOURCE_STATE_COMMON);
	if (res != ZG_SUCCESS) return res;

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setPushConstant(
	uint32_t shaderRegister,
	const void* dataPtr,
	uint32_t dataSizeInBytes) noexcept
{
	// Require that a pipeline has been set so we can query its parameters
	if (!mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Get root signature
	const D3D12RootSignature* rootSignaturePtr = nullptr;
	if (mBoundPipelineRender != nullptr) rootSignaturePtr = &mBoundPipelineRender->rootSignature;
	else if (mBoundPipelineCompute != nullptr) rootSignaturePtr = &mBoundPipelineCompute->rootSignature;
	else return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Linear search to find push constant maping
	const D3D12PushConstantMapping* mappingPtr =
		rootSignaturePtr->getPushConstantMapping(shaderRegister);
	if (mappingPtr == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	const D3D12PushConstantMapping& mapping = *mappingPtr;

	// Sanity check to attempt to see if user provided enough bytes to read
	if (mapping.sizeInBytes != dataSizeInBytes) {
		ZG_ERROR("Push constant at shader register %u is %u bytes, provided data is %u bytes",
			shaderRegister, mapping.sizeInBytes, dataSizeInBytes);
		return ZG_ERROR_INVALID_ARGUMENT;
	}

	// Set push constant
	if (mBoundPipelineRender != nullptr) {
		if (mapping.sizeInBytes == 4) {
			uint32_t data = *reinterpret_cast<const uint32_t*>(dataPtr);
			commandList->SetGraphicsRoot32BitConstant(mapping.parameterIndex, data, 0);
		}
		else {
			commandList->SetGraphicsRoot32BitConstants(
				mapping.parameterIndex, mapping.sizeInBytes / 4, dataPtr, 0);
		}
	}
	else {
		if (mapping.sizeInBytes == 4) {
			uint32_t data = *reinterpret_cast<const uint32_t*>(dataPtr);
			commandList->SetComputeRoot32BitConstant(mapping.parameterIndex, data, 0);
		}
		else {
			commandList->SetComputeRoot32BitConstants(
				mapping.parameterIndex, mapping.sizeInBytes / 4, dataPtr, 0);
		}
	}
	
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setPipelineBindings(
	const ZgPipelineBindings& bindings) noexcept
{
	// Require that a pipeline has been set so we can query its parameters
	if (!mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Get root signature
	const D3D12RootSignature* rootSignaturePtr = nullptr;
	if (mBoundPipelineRender != nullptr) rootSignaturePtr = &mBoundPipelineRender->rootSignature;
	else if (mBoundPipelineCompute != nullptr) rootSignaturePtr = &mBoundPipelineCompute->rootSignature;
	else return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	const uint32_t numConstantBuffers = rootSignaturePtr->constBuffers.size();
	const uint32_t numUnorderedBuffers = rootSignaturePtr->unorderedBuffers.size();
	const uint32_t numUnorderedTextures = rootSignaturePtr->unorderedTextures.size();
	const uint32_t numTextures = rootSignaturePtr->textures.size();

	// If no bindings specified, do nothing.
	if (bindings.numConstantBuffers == 0 &&
		bindings.numUnorderedBuffers == 0 &&
		bindings.numUnorderedTextures == 0 &&
		bindings.numTextures == 0) return ZG_SUCCESS;

	// Allocate descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE rangeStartCpu = {};
	D3D12_GPU_DESCRIPTOR_HANDLE rangeStartGpu = {};
	ZgResult allocRes = mDescriptorBuffer->allocateDescriptorRange(
		numConstantBuffers + numUnorderedBuffers + numUnorderedTextures + numTextures, rangeStartCpu, rangeStartGpu);
	if (allocRes != ZG_SUCCESS) return allocRes;

	// Create constant buffer views and fill (CPU) descriptors
	for (const D3D12ConstantBufferMapping& mapping : rootSignaturePtr->constBuffers) {

		// Get the CPU descriptor
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor;
		cpuDescriptor.ptr =
			rangeStartCpu.ptr + mDescriptorBuffer->descriptorSize * mapping.tableOffset;

		// Linear search to find matching argument among the bindings
		uint32_t bindingIdx = ~0u;
		for (uint32_t j = 0; j < bindings.numConstantBuffers; j++) {
			const ZgConstantBufferBinding& binding = bindings.constantBuffers[j];
			if (binding.bufferRegister == mapping.bufferRegister) {
				bindingIdx = j;
				break;
			}
		}

		// If we can't find argument we need to insert null descriptor
		if (bindingIdx == ~0u) {
			// TODO: Not sure if possible to implement?
			sfz_assert(false);
			return ZG_WARNING_UNIMPLEMENTED;
		}

		// Get buffer from binding and cast it to D3D12 buffer
		D3D12Buffer* buffer =
			reinterpret_cast<D3D12Buffer*>(bindings.constantBuffers[bindingIdx].buffer);

		// D3D12 requires that a Constant Buffer View is at least 256 bytes, and a multiple of 256.
		// Round up constant buffer size to nearest 256 alignment
		sfz_assert(mapping.sizeInBytes != 0);
		uint32_t bufferSize256Aligned = (mapping.sizeInBytes + 255) & 0xFFFFFF00u;

		// Check that buffer is large enough
		if (buffer->sizeBytes < bufferSize256Aligned) {
			ZG_ERROR("Constant buffer at shader register %u requires a buffer that is at"
				" least %u bytes, specified buffer is %u bytes.",
				mapping.bufferRegister, bufferSize256Aligned, buffer->sizeBytes);
			return ZG_ERROR_INVALID_ARGUMENT;
		}

		// Create constant buffer view
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = buffer->resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = bufferSize256Aligned;
		mDevice->CreateConstantBufferView(&cbvDesc, cpuDescriptor);

		// Set buffer resource state
		setBufferState(*buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		// Insert into residency set
		residencySet->Insert(&buffer->memoryHeap->managedObject);
	}

	// Create unordered resource views and fill (CPU) descriptors for unordered buffers
	for (const D3D12UnorderedBufferMapping& mapping : rootSignaturePtr->unorderedBuffers) {

		// Get the CPU descriptor
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor;
		cpuDescriptor.ptr =
			rangeStartCpu.ptr + mDescriptorBuffer->descriptorSize * mapping.tableOffset;

		// Linear search to find matching argument among the bindings
		uint32_t bindingIdx = ~0u;
		for (uint32_t j = 0; j < bindings.numUnorderedBuffers; j++) {
			const ZgUnorderedBufferBinding& binding = bindings.unorderedBuffers[j];
			if (binding.unorderedRegister == mapping.unorderedRegister) {
				bindingIdx = j;
				break;
			}
		}

		// If we can't find argument we need to insert null descriptor
		if (bindingIdx == ~0u) {
			// TODO: Is definitely possible
			sfz_assert(false);
			return ZG_WARNING_UNIMPLEMENTED;
		}

		// Get binding and buffer
		const ZgUnorderedBufferBinding& binding = bindings.unorderedBuffers[bindingIdx];
		D3D12Buffer* buffer =
			reinterpret_cast<D3D12Buffer*>(binding.buffer);

		// Create unordered access view
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN; // TODO: Unsure about this one
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = binding.firstElementIdx;
		uavDesc.Buffer.NumElements = binding.numElements;
		uavDesc.Buffer.StructureByteStride = binding.elementStrideBytes;
		uavDesc.Buffer.CounterOffsetInBytes = 0; // We don't have a counter
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // TODO: This need to be set if RWByteAddressBuffer
		mDevice->CreateUnorderedAccessView(buffer->resource.Get(), nullptr, &uavDesc, cpuDescriptor);

		// Set buffer resource state
		setBufferState(*buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Insert into residency set
		residencySet->Insert(&buffer->memoryHeap->managedObject);
	}

	// Create unordered access views and fill (CPU) descriptors for unordered textures
	for (const D3D12UnorderedTextureMapping& mapping : rootSignaturePtr->unorderedTextures) {

		// Get the CPU descriptor
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor;
		cpuDescriptor.ptr =
			rangeStartCpu.ptr + mDescriptorBuffer->descriptorSize * mapping.tableOffset;

		// Linear search to find matching argument among the bindings
		uint32_t bindingIdx = ~0u;
		for (uint32_t j = 0; j < bindings.numUnorderedTextures; j++) {
			const ZgUnorderedTextureBinding& binding = bindings.unorderedTextures[j];
			if (binding.unorderedRegister == mapping.unorderedRegister) {
				bindingIdx = j;
				break;
			}
		}

		// If we can't find argument we need to insert null descriptor
		if (bindingIdx == ~0u) {
			// TODO: Is definitely possible
			sfz_assert(false);
			return ZG_WARNING_UNIMPLEMENTED;
		}

		// Get binding and texture
		const ZgUnorderedTextureBinding& binding = bindings.unorderedTextures[bindingIdx];
		D3D12Texture2D* texture =
			reinterpret_cast<D3D12Texture2D*>(binding.texture);

		// Create unordered access view
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = texture->format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = binding.mipLevel;
		uavDesc.Texture2D.PlaneSlice = 0;
		mDevice->CreateUnorderedAccessView(texture->resource.Get(), nullptr, &uavDesc, cpuDescriptor);

		// Set texture resource state
		setTextureState(*texture, binding.mipLevel, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Insert into residency set
		residencySet->Insert(&texture->textureHeap->managedObject);
	}

	// Create shader resource views and fill (CPU) descriptors
	for (const D3D12TextureMapping& mapping : rootSignaturePtr->textures) {

		// Get the CPU descriptor;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor;
		cpuDescriptor.ptr =
			rangeStartCpu.ptr + mDescriptorBuffer->descriptorSize * mapping.tableOffset;

		// Linear search to find matching argument among the bindings
		uint32_t bindingIdx = ~0u;
		for (uint32_t j = 0; j < bindings.numTextures; j++) {
			const ZgTextureBinding& binding = bindings.textures[j];
			if (binding.textureRegister == mapping.textureRegister) {
				bindingIdx = j;
				break;
			}
		}

		// If binding found, get D3D12 texture and its resource and format. Otherwise set default
		// in order to create null descriptor
		D3D12Texture2D* texture = nullptr;
		ID3D12Resource* resource = nullptr;
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		if (bindingIdx != ~0u) {
			texture = reinterpret_cast<D3D12Texture2D*>(bindings.textures[bindingIdx].texture);
			resource = texture->resource.Get();
			format = texture->format;
		}

		// If depth format, convert to SRV compatible format
		if (format == DXGI_FORMAT_D32_FLOAT) {
			format = DXGI_FORMAT_R32_FLOAT;
		}

		// Create shader resource view
		// Will be null descriptor if no binding found
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = (uint32_t)-1; // All mip-levels from most detailed and downwards
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		mDevice->CreateShaderResourceView(resource, &srvDesc, cpuDescriptor);

		// Set texture resource state and insert into residency set if not null descriptor
		if (bindingIdx != ~0u) {

			// Set texture resource state
			setTextureStateAllMipLevels(
				*texture,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			// Insert into residency set
			residencySet->Insert(&texture->textureHeap->managedObject);
		}
	}

	// Set descriptor table to root signature
	if (mBoundPipelineRender != nullptr) {
		commandList->SetGraphicsRootDescriptorTable(
			rootSignaturePtr->dynamicBuffersParameterIndex, rangeStartGpu);
	}
	else {
		commandList->SetComputeRootDescriptorTable(
			rootSignaturePtr->dynamicBuffersParameterIndex, rangeStartGpu);
	}

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setPipelineCompute(
	ZgPipelineCompute* pipelineIn) noexcept
{
	D3D12PipelineCompute& pipeline = *static_cast<D3D12PipelineCompute*>(pipelineIn);

	// If a pipeline is already set for this command list, return error. We currently only allow a
	// single pipeline per command list.
	if (mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	mPipelineSet = true;
	mBoundPipelineCompute = &pipeline;

	// Set compute pipeline
	commandList->SetPipelineState(pipeline.pipelineState.Get());
	commandList->SetComputeRootSignature(pipeline.rootSignature.rootSignature.Get());

	// Set descriptor heap
	ID3D12DescriptorHeap* heaps[] = { mDescriptorBuffer->descriptorHeap.Get() };
	commandList->SetDescriptorHeaps(1, heaps);

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::unorderedBarrierBuffer(
	ZgBuffer* bufferIn) noexcept
{
	D3D12Buffer* buffer = static_cast<D3D12Buffer*>(bufferIn);
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = buffer->resource.Get();
	commandList->ResourceBarrier(1, &barrier);
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::unorderedBarrierTexture(
	ZgTexture2D* textureIn) noexcept
{
	D3D12Texture2D* texture = static_cast<D3D12Texture2D*>(textureIn);
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = texture->resource.Get();
	commandList->ResourceBarrier(1, &barrier);
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::unorderedBarrierAll() noexcept
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = nullptr;
	commandList->ResourceBarrier(1, &barrier);
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::dispatchCompute(
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ) noexcept
{
	if (!mPipelineSet || mBoundPipelineCompute == nullptr) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setPipelineRender(
	ZgPipelineRender* pipelineIn) noexcept
{
	D3D12PipelineRender& pipeline = *static_cast<D3D12PipelineRender*>(pipelineIn);
	
	// If a pipeline is already set for this command list, return error. We currently only allow a
	// single pipeline per command list.
	if (mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	mPipelineSet = true;
	mBoundPipelineRender = &pipeline;

	// Set render pipeline
	commandList->SetPipelineState(pipeline.pipelineState.Get());
	commandList->SetGraphicsRootSignature(pipeline.rootSignature.rootSignature.Get());

	// Set descriptor heap
	ID3D12DescriptorHeap* heaps[] = { mDescriptorBuffer->descriptorHeap.Get() };
	commandList->SetDescriptorHeaps(1, heaps);

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setFramebuffer(
	ZgFramebuffer* framebufferIn,
	const ZgFramebufferRect* optionalViewport,
	const ZgFramebufferRect* optionalScissor) noexcept
{
	// Cast input to D3D12
	D3D12Framebuffer& framebuffer = *static_cast<D3D12Framebuffer*>(framebufferIn);

	// Check arguments
	ZG_ARG_CHECK(!framebuffer.hasDepthBuffer && framebuffer.numRenderTargets == 0,
		"Can't set a framebuffer with no render targets or depth buffer");

	// If a framebuffer is already set for this command list, return error. We currently only allow
	// a single framebuffer per command list.
	if (mFramebufferSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	mFramebufferSet = true;
	mFramebuffer = &framebuffer;

	// If no viewport is requested, set one that covers entire screen
	D3D12_VIEWPORT viewport = {};
	if (optionalViewport == nullptr) {

		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = float(framebuffer.width);
		viewport.Height = float(framebuffer.height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
	}

	// Otherwise do what the user explicitly requested
	else {
		viewport.TopLeftX = float(optionalViewport->topLeftX);
		viewport.TopLeftY = float(optionalViewport->topLeftY);
		viewport.Width = float(optionalViewport->width);
		viewport.Height = float(optionalViewport->height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
	}

	// Set viewport
	commandList->RSSetViewports(1, &viewport);
	
	// If no scissor is requested, set one that covers entire screen
	D3D12_RECT scissorRect = {};
	if (optionalScissor == nullptr) {

		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = LONG_MAX;
		scissorRect.bottom = LONG_MAX;
	}

	// Otherwise do what user explicitly requested
	else {
		// TODO: Possibly off by one (i.e. topLeftX + width - 1)
		scissorRect.left = optionalScissor->topLeftX;
		scissorRect.top = optionalScissor->topLeftY;
		scissorRect.right = optionalScissor->topLeftX + optionalScissor->width;
		scissorRect.bottom = optionalScissor->topLeftY + optionalScissor->height;
	}

	// Set scissor rect
	commandList->RSSetScissorRects(1, &scissorRect);

	// If not swapchain framebuffer, set resource states and insert into residency sets
	if (!framebuffer.swapchainFramebuffer) {

		// Render targets
		for (uint32_t i = 0; i < framebuffer.numRenderTargets; i++) {
			D3D12Texture2D* renderTarget = framebuffer.renderTargets[i];

			// Set resource state
			sfz_assert(renderTarget->numMipmaps == 1);
			setTextureState(*renderTarget, 0, D3D12_RESOURCE_STATE_RENDER_TARGET);

			// Insert into residency set
			residencySet->Insert(&renderTarget->textureHeap->managedObject);
		}

		// Depth buffer
		if (framebuffer.hasDepthBuffer) {
			D3D12Texture2D* depthBuffer = framebuffer.depthBuffer;

			// Set resource state
			sfz_assert(depthBuffer->numMipmaps == 1);
			setTextureState(*depthBuffer, 0, D3D12_RESOURCE_STATE_DEPTH_WRITE);

			// Insert into residency set
			residencySet->Insert(&depthBuffer->textureHeap->managedObject);
		}
	}

	// Set framebuffer
	commandList->OMSetRenderTargets(
		framebuffer.numRenderTargets,
		framebuffer.numRenderTargets > 0 ? framebuffer.renderTargetDescriptors : nullptr,
		FALSE,
		framebuffer.hasDepthBuffer ? &framebuffer.depthBufferDescriptor : nullptr);

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setFramebufferViewport(
	const ZgFramebufferRect& viewportRect) noexcept
{
	// Return error if no framebuffer is set
	if (!mFramebufferSet) {
		ZG_ERROR("setFramebufferViewport(): Must set a framebuffer before you can change viewport");
		return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	}

	// Set viewport
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = float(viewportRect.topLeftX);
	viewport.TopLeftY = float(viewportRect.topLeftY);
	viewport.Width = float(viewportRect.width);
	viewport.Height = float(viewportRect.height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setFramebufferScissor(
	const ZgFramebufferRect& scissor) noexcept
{
	// Return error if no framebuffer is set
	if (!mFramebufferSet) {
		ZG_ERROR("setFramebufferScissor(): Must set a framebuffer before you can change scissor");
		return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	}

	// Set scissor rect
	// TODO: Possibly off by one (i.e. topLeftX + width - 1)
	D3D12_RECT scissorRect = {};
	scissorRect.left = scissor.topLeftX;
	scissorRect.top = scissor.topLeftY;
	scissorRect.right = scissor.topLeftX + scissor.width;
	scissorRect.bottom = scissor.topLeftY + scissor.height;

	// Bad scissor specified, just use whole viewport
	if (scissor.width == 0 && scissor.height == 0) {
		ZG_INFO("setFramebufferScissor(): Bad scissor specified, ignoring");
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = LONG_MAX;
		scissorRect.bottom = LONG_MAX;
	}

	commandList->RSSetScissorRects(1, &scissorRect);

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::clearFramebufferOptimal() noexcept
{
	// Return error if no framebuffer is set
	if (!mFramebufferSet) {
		ZG_ERROR("clearFramebufferOptimal(): Must set a framebuffer before you can clear it");
		return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	}

	constexpr float ZEROS[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	constexpr float ONES[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	// Clear render targets
	for (uint32_t i = 0; i < mFramebuffer->numRenderTargets; i++) {

		const float* clearColor = [&]() -> const float* {
			switch (mFramebuffer->renderTargetOptimalClearValues[i]) {
			case ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED: return ZEROS;
			case ZG_OPTIMAL_CLEAR_VALUE_ZERO: return ZEROS;
			case ZG_OPTIMAL_CLEAR_VALUE_ONE: return ONES;
			}
			sfz_assert(false);
			return nullptr;
		}();

		commandList->ClearRenderTargetView(
			mFramebuffer->renderTargetDescriptors[i], clearColor, 0, nullptr);
	}

	// Clear depth buffer
	if (mFramebuffer->hasDepthBuffer) {

		const float clearDepth = [&]() {
			switch (mFramebuffer->depthBufferOptimalClearValue) {
			case ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED: return 0.0f;
			case ZG_OPTIMAL_CLEAR_VALUE_ZERO: return 0.0f;
			case ZG_OPTIMAL_CLEAR_VALUE_ONE: return 1.0f;
			}
			sfz_assert(false);
			return 0.0f;
		}();

		commandList->ClearDepthStencilView(
			mFramebuffer->depthBufferDescriptor, D3D12_CLEAR_FLAG_DEPTH, clearDepth, 0, 0, nullptr);
	}

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::clearRenderTargets(
	float red,
	float green,
	float blue,
	float alpha) noexcept
{
	// Return error if no framebuffer is set
	if (!mFramebufferSet) {
		ZG_ERROR("clearRenderTargets(): Must set a framebuffer before you can clear its render targets");
		return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	}
	if (mFramebuffer->numRenderTargets == 0) return ZG_WARNING_GENERIC;

	// Clear render targets
	float clearColor[4] = { red, green, blue, alpha };
	for (uint32_t i = 0; i < mFramebuffer->numRenderTargets; i++) {
		commandList->ClearRenderTargetView(
			mFramebuffer->renderTargetDescriptors[i], clearColor, 0, nullptr);
	}

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::clearDepthBuffer(
	float depth) noexcept
{
	// Return error if no framebuffer is set
	if (!mFramebufferSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	if (!mFramebuffer->hasDepthBuffer) return ZG_WARNING_GENERIC;

	// Clear depth buffer
	commandList->ClearDepthStencilView(
		mFramebuffer->depthBufferDescriptor, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setIndexBuffer(
	ZgBuffer* indexBufferIn,
	ZgIndexBufferType type) noexcept
{
	// Cast input to D3D12
	D3D12Buffer& indexBuffer = *reinterpret_cast<D3D12Buffer*>(indexBufferIn);

	// Set buffer resource state
	ZgResult res;
	if (indexBuffer.memoryHeap->memoryType == ZG_MEMORY_TYPE_DEVICE) {
		res = setBufferState(indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	}
	else if (indexBuffer.memoryHeap->memoryType == ZG_MEMORY_TYPE_UPLOAD) {
		res = setBufferState(indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
	else {
		return ZG_ERROR_INVALID_ARGUMENT;
	}
	if (res != ZG_SUCCESS) return res;

	// Create index buffer view
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = indexBuffer.resource->GetGPUVirtualAddress();
	sfz_assert(indexBuffer.sizeBytes <= uint64_t(UINT32_MAX));
	indexBufferView.SizeInBytes = uint32_t(indexBuffer.sizeBytes);
	indexBufferView.Format = type == ZG_INDEX_BUFFER_TYPE_UINT32 ?
		DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

	// Set index buffer
	commandList->IASetIndexBuffer(&indexBufferView);

	// Insert into residency set
	residencySet->Insert(&indexBuffer.memoryHeap->managedObject);

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setVertexBuffer(
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBufferIn) noexcept
{
	// Cast input to D3D12
	D3D12Buffer& vertexBuffer = *reinterpret_cast<D3D12Buffer*>(vertexBufferIn);

	// Need to have a pipeline set to verify vertex buffer binding
	if (!mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

	// Check that the vertex buffer slot is not out of bounds for the bound pipeline
	const ZgPipelineRenderCreateInfo& pipelineInfo = mBoundPipelineRender->createInfo;
	if (pipelineInfo.numVertexBufferSlots <= vertexBufferSlot) {
		return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
	}

	// Set buffer resource state
	ZgResult res;
	if (vertexBuffer.memoryHeap->memoryType == ZG_MEMORY_TYPE_DEVICE) {
		res = setBufferState(vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}
	else if (vertexBuffer.memoryHeap->memoryType == ZG_MEMORY_TYPE_UPLOAD) {
		res = setBufferState(vertexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
	else {
		return ZG_ERROR_INVALID_ARGUMENT;
	}
	if (res != ZG_SUCCESS) return res;

	// Create vertex buffer view
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = vertexBuffer.resource->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = pipelineInfo.vertexBufferStridesBytes[vertexBufferSlot];
	vertexBufferView.SizeInBytes = uint32_t(vertexBuffer.sizeBytes);

	// Set vertex buffer
	commandList->IASetVertexBuffers(vertexBufferSlot, 1, &vertexBufferView);

	// Insert into residency set
	residencySet->Insert(&vertexBuffer.memoryHeap->managedObject);

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::drawTriangles(
	uint32_t startVertexIndex,
	uint32_t numVertices) noexcept
{	
	// Draw triangles
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(numVertices, 1, startVertexIndex, 0);
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::drawTrianglesIndexed(
	uint32_t startIndex,
	uint32_t numTriangles) noexcept
{
	// Draw triangles indexed
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(numTriangles * 3, 1, startIndex, 0, 0);
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::profileBegin(
	ZgProfiler* profilerIn,
	uint64_t& measurementIdOut) noexcept
{
	// TODO: This is necessary because we don't get timestamp frequency for other queue types.
	//       Besides, timestamp queries only work on present and compute queues in the first place.
	sfz_assert(commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT);

	// Access profilers state through its mutex
	D3D12Profiler* profiler = static_cast<D3D12Profiler*>(profilerIn);
	MutexAccessor<D3D12ProfilerState> profilerStateAccessor = profiler->state.access();
	D3D12ProfilerState& profilerState = profilerStateAccessor.data();

	// Get next measurement id and calculate query idx
	uint64_t measurementId = profilerState.nextMeasurementId;
	measurementIdOut = measurementId;
	profilerState.nextMeasurementId += 1;
	uint32_t queryIdx = measurementId % profilerState.maxNumMeasurements;
	uint32_t timestampIdx = queryIdx * 2;

	// Start timestamp query
	commandList->EndQuery(profilerState.queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, timestampIdx);
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::profileEnd(
	ZgProfiler* profilerIn,
	uint64_t measurementId) noexcept
{
	// TODO: This is necessary because we don't get timestamp frequency for other queue types.
	//       Besides, timestamp queries only work on present and compute queues in the first place.
	sfz_assert(commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT);

	// Access profilers state through its mutex
	D3D12Profiler* profiler = static_cast<D3D12Profiler*>(profilerIn);
	MutexAccessor<D3D12ProfilerState> profilerStateAccessor = profiler->state.access();
	D3D12ProfilerState& state = profilerStateAccessor.data();

	// Return invalid argument if measurement id is not valid
	bool validMeasurementId =
		measurementId < state.nextMeasurementId &&
		(measurementId + state.maxNumMeasurements) >= state.nextMeasurementId;
	if (!validMeasurementId) return ZG_ERROR_INVALID_ARGUMENT;

	// Get query idx
	uint32_t queryIdx = measurementId % state.maxNumMeasurements;
	uint32_t timestampBaseIdx = queryIdx * 2;
	uint32_t timestampIdx = timestampBaseIdx + 1;

	// End timestamp query
	commandList->EndQuery(state.queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, timestampIdx);
	
	// Resolve query
	uint64_t bufferOffset = timestampBaseIdx * sizeof(uint64_t);
	commandList->ResolveQueryData(
		state.queryHeap.Get(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		timestampBaseIdx,
		2,
		state.downloadBuffer->resource.Get(),
		bufferOffset);
	
	// Insert into residency set
	residencySet->Insert(&state.downloadHeap->managedObject);

	return ZG_SUCCESS;
}

// D3D12CommandList: Helper methods
// ------------------------------------------------------------------------------------------------

ZgResult D3D12CommandList::reset() noexcept
{
	if (D3D12_FAIL(commandAllocator->Reset())) {
		return ZG_ERROR_GENERIC;
	}
	if (D3D12_FAIL(commandList->Reset(commandAllocator.Get(), nullptr))) {
		return ZG_ERROR_GENERIC;
	}

	pendingBufferIdentifiers.clear();
	pendingBufferStates.clear();

	pendingTextureIdentifiers.clear();
	pendingTextureStates.clear();

	mPipelineSet = false;
	mBoundPipelineCompute = nullptr;
	mBoundPipelineRender = nullptr;
	mFramebufferSet = false;
	mFramebuffer = nullptr;
	return ZG_SUCCESS;
}

// D3D12CommandList: Private methods
// ------------------------------------------------------------------------------------------------

ZgResult D3D12CommandList::getPendingBufferStates(
	D3D12Buffer& buffer,
	D3D12_RESOURCE_STATES neededState,
	PendingBufferState*& pendingStatesOut) noexcept
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
		pendingBufferStates.add(PendingBufferState());

		// Set initial pending buffer state
		pendingBufferStates.last().buffer = &buffer;
		pendingBufferStates.last().neededInitialState = neededState;
		pendingBufferStates.last().currentState = neededState;
	}

	pendingStatesOut = &pendingBufferStates[bufferStateIdx];
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setBufferState(
	D3D12Buffer& buffer, D3D12_RESOURCE_STATES targetState) noexcept
{
	// Get pending states
	PendingBufferState* pendingState = nullptr;
	ZgResult pendingStateRes = getPendingBufferStates(
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

ZgResult D3D12CommandList::getPendingTextureStates(
	D3D12Texture2D& texture,
	uint32_t mipLevel,
	D3D12_RESOURCE_STATES neededState,
	PendingTextureState*& pendingStatesOut) noexcept
{
	// Try to find index of pending buffer states
	uint32_t textureStateIdx = ~0u;
	for (uint32_t i = 0; i < pendingTextureIdentifiers.size(); i++) {
		TextureMipIdentifier identifier = pendingTextureIdentifiers[i];
		if (identifier.identifier == texture.identifier && identifier.mipLevel == mipLevel) {
			textureStateIdx = i;
			break;
		}
	}

	// If texture does not have a pending state, create one
	if (textureStateIdx == ~0u) {

		// Check if we have enough space for another pending state
		if (pendingTextureStates.size() == pendingTextureStates.capacity()) {
			return ZG_ERROR_GENERIC;
		}

		// Create pending buffer state
		textureStateIdx = pendingTextureStates.size();
		TextureMipIdentifier identifier;
		identifier.identifier = texture.identifier;
		identifier.mipLevel = mipLevel;
		pendingTextureIdentifiers.add(identifier);
		pendingTextureStates.add(PendingTextureState());
		
		// Set initial pending buffer state
		pendingTextureStates.last().texture = &texture;
		pendingTextureStates.last().mipLevel = mipLevel;
		pendingTextureStates.last().neededInitialState = neededState;
		pendingTextureStates.last().currentState = neededState;
	}

	pendingStatesOut = &pendingTextureStates[textureStateIdx];
	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setTextureState(
	D3D12Texture2D& texture,
	uint32_t mipLevel,
	D3D12_RESOURCE_STATES targetState) noexcept
{
	// Get pending states
	PendingTextureState* pendingState = nullptr;
	ZgResult pendingStateRes = getPendingTextureStates(
		texture, mipLevel, targetState, pendingState);
	if (pendingStateRes != ZG_SUCCESS) return pendingStateRes;

	// Change state of texture if necessary
	if (pendingState->currentState != targetState) {
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			texture.resource.Get(),
			pendingState->currentState,
			targetState,
			mipLevel);
		commandList->ResourceBarrier(1, &barrier);
		pendingState->currentState = targetState;
	}

	return ZG_SUCCESS;
}

ZgResult D3D12CommandList::setTextureStateAllMipLevels(
	D3D12Texture2D& texture,
	D3D12_RESOURCE_STATES targetState) noexcept
{
	// Get pending states
	PendingTextureState* pendingStates[ZG_MAX_NUM_MIPMAPS] = {};
	for (uint32_t i = 0; i < texture.numMipmaps; i++) {
		ZgResult pendingStateRes = getPendingTextureStates(
			texture, i, targetState, pendingStates[i]);
		if (pendingStateRes != ZG_SUCCESS) return pendingStateRes;
	}

	// Create all necessary barriers
	CD3DX12_RESOURCE_BARRIER barriers[ZG_MAX_NUM_MIPMAPS] = {};
	uint32_t numBarriers = 0;
	for (uint32_t i = 0; i < texture.numMipmaps; i++) {
		if (pendingStates[i]->currentState != targetState) {
			barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(
				texture.resource.Get(),
				pendingStates[i]->currentState,
				targetState,
				i);
			numBarriers += 1;
			pendingStates[i]->currentState = targetState;
		}
	}

	// Submit barriers
	if (numBarriers != 0) {
		commandList->ResourceBarrier(numBarriers, barriers);
	}
	
	return ZG_SUCCESS;
}

} // namespace zg
