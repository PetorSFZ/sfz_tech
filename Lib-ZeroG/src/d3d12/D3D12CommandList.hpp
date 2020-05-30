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

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>

#include "ZeroG.h"
#include "common/Mutex.hpp"
#include "d3d12/D3D12Common.hpp"
#include "d3d12/D3D12DescriptorRingBuffer.hpp"
#include "d3d12/D3D12Framebuffer.hpp"
#include "d3d12/D3D12Memory.hpp"
#include "d3d12/D3D12Pipelines.hpp"
#include "d3d12/D3D12Profiler.hpp"
#include "d3d12/D3D12ResourceTracking.hpp"

// Helpers
// ------------------------------------------------------------------------------------------------

inline uint32_t numBytesPerPixelForFormat(ZgTextureFormat format) noexcept
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

// ZgCommandList
// ------------------------------------------------------------------------------------------------

struct ZgCommandList final {
	SFZ_DECLARE_DROP_TYPE(ZgCommandList);

	void create(
		ComPtr<ID3D12Device3> device,
		D3D12DescriptorRingBuffer* descriptorBuffer) noexcept
	{
		mDevice = device.Get();
		mDescriptorBuffer = descriptorBuffer;

		tracking.init(getAllocator());
	}

	void destroy() noexcept
	{
		commandAllocator = nullptr;
		commandList = nullptr;
		fenceValue = 0;

		tracking.destroy();

		mDevice = nullptr;
		mDescriptorBuffer = nullptr;
		mPipelineSet = false;
		mBoundPipelineRender = nullptr;
		mBoundPipelineCompute = nullptr;
		mFramebufferSet = false;
		mFramebuffer = nullptr;
	}

	ZgResult reset() noexcept
	{
		if (D3D12_FAIL(commandAllocator->Reset())) {
			return ZG_ERROR_GENERIC;
		}
		if (D3D12_FAIL(commandList->Reset(commandAllocator.Get(), nullptr))) {
			return ZG_ERROR_GENERIC;
		}

		tracking.pendingBufferIdentifiers.clear();
		tracking.pendingBufferStates.clear();

		tracking.pendingTextureIdentifiers.clear();
		tracking.pendingTextureStates.clear();

		mPipelineSet = false;
		mBoundPipelineCompute = nullptr;
		mBoundPipelineRender = nullptr;
		mFramebufferSet = false;
		mFramebuffer = nullptr;
		return ZG_SUCCESS;
	}

	// Members
	// --------------------------------------------------------------------------------------------

	D3D12_COMMAND_LIST_TYPE commandListType;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	uint64_t fenceValue = 0;

	ZgTrackerCommandListState tracking;

	ID3D12Device3* mDevice = nullptr;
	D3D12DescriptorRingBuffer* mDescriptorBuffer = nullptr;
	bool mPipelineSet = false; // Only allow a single pipeline per command list
	ZgPipelineRender* mBoundPipelineRender = nullptr;
	ZgPipelineCompute* mBoundPipelineCompute = nullptr;
	bool mFramebufferSet = false; // Only allow a single framebuffer to be set.
	ZgFramebuffer* mFramebuffer = nullptr;

	// Methods
	// --------------------------------------------------------------------------------------------

	ZgResult beginEvent(const char* name, const float* optionalRgbaColor)
	{
		// D3D12_EVENT_METADATA defintion
		constexpr UINT WINPIX_EVENT_PIX3BLOB_VERSION = 2;
		constexpr UINT D3D12_EVENT_METADATA = WINPIX_EVENT_PIX3BLOB_VERSION;

		// Buffer
		constexpr UINT64 PIXEventsGraphicsRecordSpaceQwords = 64;
		UINT64 buffer[PIXEventsGraphicsRecordSpaceQwords] = {};
		UINT64* destination = buffer;

		// Encode event info (timestamp = 0, PIXEvent_BeginEvent_NoArgs)
		constexpr uint64_t ENCODE_EVENT_INFO_CONSTANT = uint64_t(2048);
		*destination++ = ENCODE_EVENT_INFO_CONSTANT;

		// Parse and encode color from optionalRgbaColor
		sfz_assert(optionalRgbaColor == nullptr);
		float r = optionalRgbaColor != nullptr ? optionalRgbaColor[0] : 0.0f;
		float g = optionalRgbaColor != nullptr ? optionalRgbaColor[1] : 0.0f;
		float b = optionalRgbaColor != nullptr ? optionalRgbaColor[2] : 0.0f;
		UINT64 color = [](float r, float g, float b) -> UINT64 {
			BYTE rb = BYTE((r / 255.0f) + 0.5f);
			BYTE gb = BYTE((g / 255.0f) + 0.5f);
			BYTE bb = BYTE((b / 255.0f) + 0.5f);
			return UINT64(0xff000000u | (rb << 16) | (gb << 8) | bb);
		}(r, g, b);
		*destination++ = color;

		// Encode string info (alignment = 0, copyChunkSize = 8, isAnsi=true, isShortcut=false)
		constexpr uint64_t STRING_INFO_CONSTANT = uint64_t(306244774661193728);
		*destination++ = STRING_INFO_CONSTANT;

		// Copy string
		constexpr uint32_t STRING_MAX_LEN = 20 * 8;
		const uint32_t nameLen = uint32_t(strnlen(name, STRING_MAX_LEN));
		memcpy(destination, name, nameLen);
		destination += ((nameLen / 8) + 1);

		// Call BeginEvent with our hacked together binary blob
		UINT sizeBytes = UINT(reinterpret_cast<BYTE*>(destination) - reinterpret_cast<BYTE*>(buffer));
		commandList->BeginEvent(D3D12_EVENT_METADATA, buffer, sizeBytes);
		
		return ZG_SUCCESS;
	}

	ZgResult endEvent()
	{
		commandList->EndEvent();
		return ZG_SUCCESS;
	}

	ZgResult memcpyBufferToBuffer(
		ZgBuffer* dstBuffer,
		uint64_t dstBufferOffsetBytes,
		ZgBuffer* srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept
	{
		// Current don't allow memcpy:ing to the same buffer.
		if (dstBuffer->identifier == srcBuffer->identifier) return ZG_ERROR_INVALID_ARGUMENT;

		// Wanted resource states
		D3D12_RESOURCE_STATES dstTargetState = D3D12_RESOURCE_STATE_COPY_DEST;
		D3D12_RESOURCE_STATES srcTargetState = D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (srcBuffer->memoryType == ZG_MEMORY_TYPE_UPLOAD) {
			srcTargetState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}

		// Set buffer resource states
		requireResourceStateBuffer(*this->commandList.Get(), this->tracking, dstBuffer, dstTargetState);
		requireResourceStateBuffer(*this->commandList.Get(), this->tracking, srcBuffer, srcTargetState);

		commandList->CopyBufferRegion(
			dstBuffer->resource.resource,
			dstBufferOffsetBytes,
			srcBuffer->resource.resource,
			srcBufferOffsetBytes,
			numBytes);

		return ZG_SUCCESS;
	}

	ZgResult memcpyToTexture(
		ZgTexture* dstTextureIn,
		uint32_t dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		ZgBuffer* tempUploadBufferIn) noexcept
	{
		ZgTexture& dstTexture = *dstTextureIn;
		ZgBuffer& tmpBuffer = *tempUploadBufferIn;

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
		if (tmpBuffer.memoryType != ZG_MEMORY_TYPE_UPLOAD) return ZG_ERROR_INVALID_ARGUMENT;

		// Check that upload buffer is big enough
		uint32_t numBytesPerPixel = numBytesPerPixelForFormat(srcImageCpu.format);
		uint32_t numBytesPerRow = srcImageCpu.width * numBytesPerPixel;
		uint32_t tmpBufferPitch = ((numBytesPerRow + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) /
			D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
		uint32_t tmpBufferRequiredSize = tmpBufferPitch * srcImageCpu.height;
		if (tmpBuffer.resource.allocation->GetSize() < tmpBufferRequiredSize) {
			ZG_ERROR("Temporary buffer is too small, it is %u bytes, but %u bytes is required."
				" The pitch of the upload buffer is required to be %u byte aligned.",
				uint32_t(tmpBuffer.resource.allocation->GetSize()),
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
		if (D3D12_FAIL(tmpBuffer.resource.resource->Map(0, &readRange, &mappedPtr))) {
			return ZG_ERROR_GENERIC;
		}

		// Memcpy cpu image to tmp buffer
		for (uint32_t y = 0; y < srcImageCpu.height; y++) {
			const uint8_t* rowPtr = ((const uint8_t*)srcImageCpu.data) + srcImageCpu.pitchInBytes * y;
			uint8_t* dstPtr = reinterpret_cast<uint8_t*>(mappedPtr) + tmpBufferPitch * y;
			memcpy(dstPtr, rowPtr, numBytesPerRow);
		}

		// Unmap buffer
		tmpBuffer.resource.resource->Unmap(0, nullptr);

		// Set texture resource state
		requireResourceStateTextureMip(
			*this->commandList.Get(), this->tracking, &dstTexture, dstTextureMipLevel, D3D12_RESOURCE_STATE_COPY_DEST);

		// Issue copy command
		D3D12_TEXTURE_COPY_LOCATION tmpCopyLoc = {};
		tmpCopyLoc.pResource = tmpBuffer.resource.resource;
		tmpCopyLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		tmpCopyLoc.PlacedFootprint = dstTexture.subresourceFootprints[dstTextureMipLevel];

		// TODO: THIS IS A HACK
		// Essentially, in D3D12 you are meant to upload all of your subresources (i.e. mip levels)
		// at the same time. All of these mip levels will be in THE SAME temporary upload buffer. What
		// we instead have done here is said that each mip level will be in its own temporary upload
		// buffer, thus we need to modify the placed footprint so that it does not have an offset.
		tmpCopyLoc.PlacedFootprint.Offset = 0;

		D3D12_TEXTURE_COPY_LOCATION dstCopyLoc = {};
		dstCopyLoc.pResource = dstTexture.resource.resource;
		dstCopyLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstCopyLoc.SubresourceIndex = dstTextureMipLevel;

		commandList->CopyTextureRegion(&dstCopyLoc, 0, 0, 0, &tmpCopyLoc, nullptr);

		return ZG_SUCCESS;
	}

	ZgResult enableQueueTransitionBuffer(ZgBuffer* buffer) noexcept
	{
		// Check that it is a device buffer
		if (buffer->memoryType == ZG_MEMORY_TYPE_UPLOAD ||
			buffer->memoryType == ZG_MEMORY_TYPE_DOWNLOAD) {
			ZG_ERROR("enableQueueTransitionBuffer(): Can't transition upload and download buffers");
			return ZG_ERROR_INVALID_ARGUMENT;
		}

		// Set buffer resource state
		requireResourceStateBuffer(*this->commandList.Get(), this->tracking, buffer, D3D12_RESOURCE_STATE_COMMON);

		return ZG_SUCCESS;
	}

	ZgResult enableQueueTransitionTexture(ZgTexture* texture) noexcept
	{
		// Set buffer resource state
		requireResourceStateTextureAllMips(
			*this->commandList.Get(), this->tracking, texture, D3D12_RESOURCE_STATE_COMMON);
		return ZG_SUCCESS;
	}

	ZgResult setPushConstant(
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

	ZgResult setPipelineBindings(
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

			// Get buffer from binding
			ZgBuffer* buffer = bindings.constantBuffers[bindingIdx].buffer;

			// D3D12 requires that a Constant Buffer View is at least 256 bytes, and a multiple of 256.
			// Round up constant buffer size to nearest 256 alignment
			sfz_assert(mapping.sizeInBytes != 0);
			uint32_t bufferSize256Aligned = (mapping.sizeInBytes + 255) & 0xFFFFFF00u;

			// Check that buffer is large enough
			if (buffer->resource.allocation->GetSize() < bufferSize256Aligned) {
				ZG_ERROR("Constant buffer at shader register %u requires a buffer that is at"
					" least %u bytes, specified buffer is %u bytes.",
					mapping.bufferRegister, bufferSize256Aligned, uint32_t(buffer->resource.allocation->GetSize()));
				return ZG_ERROR_INVALID_ARGUMENT;
			}

			// Create constant buffer view
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = buffer->resource.resource->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = bufferSize256Aligned;
			mDevice->CreateConstantBufferView(&cbvDesc, cpuDescriptor);

			// Set buffer resource state
			requireResourceStateBuffer(
				*this->commandList.Get(), this->tracking, buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
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
			ZgBuffer* buffer = binding.buffer;

			// Create unordered access view
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = DXGI_FORMAT_UNKNOWN; // TODO: Unsure about this one
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = binding.firstElementIdx;
			uavDesc.Buffer.NumElements = binding.numElements;
			uavDesc.Buffer.StructureByteStride = binding.elementStrideBytes;
			uavDesc.Buffer.CounterOffsetInBytes = 0; // We don't have a counter
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // TODO: This need to be set if RWByteAddressBuffer
			mDevice->CreateUnorderedAccessView(buffer->resource.resource, nullptr, &uavDesc, cpuDescriptor);

			// Set buffer resource state
			requireResourceStateBuffer(
				*this->commandList.Get(), this->tracking, buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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
			ZgTexture* texture = binding.texture;

			// Create unordered access view
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = texture->format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = binding.mipLevel;
			uavDesc.Texture2D.PlaneSlice = 0;
			mDevice->CreateUnorderedAccessView(texture->resource.resource, nullptr, &uavDesc, cpuDescriptor);

			// Set texture resource state
			requireResourceStateTextureMip(
				*this->commandList.Get(), this->tracking, texture, binding.mipLevel, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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
			ZgTexture* texture = nullptr;
			ID3D12Resource* resource = nullptr;
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
			if (bindingIdx != ~0u) {
				texture = bindings.textures[bindingIdx].texture;
				resource = texture->resource.resource;
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
				requireResourceStateTextureAllMips(
					*this->commandList.Get(), this->tracking, texture,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
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

	ZgResult setPipelineCompute(
		ZgPipelineCompute* pipeline) noexcept
	{
		// If a pipeline is already set for this command list, return error. We currently only allow a
		// single pipeline per command list.
		if (mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
		mPipelineSet = true;
		mBoundPipelineCompute = pipeline;

		// Set compute pipeline
		commandList->SetPipelineState(pipeline->pipelineState.Get());
		commandList->SetComputeRootSignature(pipeline->rootSignature.rootSignature.Get());

		// Set descriptor heap
		ID3D12DescriptorHeap* heaps[] = { mDescriptorBuffer->descriptorHeap.Get() };
		commandList->SetDescriptorHeaps(1, heaps);

		return ZG_SUCCESS;
	}

	ZgResult unorderedBarrierBuffer(
		ZgBuffer* buffer) noexcept
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = buffer->resource.resource;
		commandList->ResourceBarrier(1, &barrier);
		return ZG_SUCCESS;
	}

	ZgResult unorderedBarrierTexture(
		ZgTexture* texture) noexcept
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = texture->resource.resource;
		commandList->ResourceBarrier(1, &barrier);
		return ZG_SUCCESS;
	}

	ZgResult unorderedBarrierAll() noexcept
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = nullptr;
		commandList->ResourceBarrier(1, &barrier);
		return ZG_SUCCESS;
	}

	ZgResult dispatchCompute(
		uint32_t groupCountX,
		uint32_t groupCountY,
		uint32_t groupCountZ) noexcept
	{
		if (!mPipelineSet || mBoundPipelineCompute == nullptr) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
		commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
		return ZG_SUCCESS;
	}

	ZgResult setPipelineRender(
		ZgPipelineRender* pipeline) noexcept
	{
		// If a pipeline is already set for this command list, return error. We currently only allow a
		// single pipeline per command list.
		if (mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
		mPipelineSet = true;
		mBoundPipelineRender = pipeline;

		// Set render pipeline
		commandList->SetPipelineState(pipeline->pipelineState.Get());
		commandList->SetGraphicsRootSignature(pipeline->rootSignature.rootSignature.Get());

		// Set descriptor heap
		ID3D12DescriptorHeap* heaps[] = { mDescriptorBuffer->descriptorHeap.Get() };
		commandList->SetDescriptorHeaps(1, heaps);

		return ZG_SUCCESS;
	}

	ZgResult setFramebuffer(
		ZgFramebuffer* framebuffer,
		const ZgRect* optionalViewport,
		const ZgRect* optionalScissor) noexcept
	{
		// Check arguments
		ZG_ARG_CHECK(!framebuffer->hasDepthBuffer && framebuffer->numRenderTargets == 0,
			"Can't set a framebuffer with no render targets or depth buffer");

		// If a framebuffer is already set for this command list, return error. We currently only allow
		// a single framebuffer per command list.
		if (mFramebufferSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
		mFramebufferSet = true;
		mFramebuffer = framebuffer;

		// If no viewport is requested, set one that covers entire screen
		D3D12_VIEWPORT viewport = {};
		if (optionalViewport == nullptr) {

			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = float(framebuffer->width);
			viewport.Height = float(framebuffer->height);
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
		if (!framebuffer->swapchainFramebuffer) {

			// Render targets
			for (uint32_t i = 0; i < framebuffer->numRenderTargets; i++) {
				ZgTexture* renderTarget = framebuffer->renderTargets[i];

				// Set resource state
				sfz_assert(renderTarget->numMipmaps == 1);
				requireResourceStateTextureMip(
					*this->commandList.Get(), this->tracking, renderTarget, 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			// Depth buffer
			if (framebuffer->hasDepthBuffer) {
				ZgTexture* depthBuffer = framebuffer->depthBuffer;

				// Set resource state
				sfz_assert(depthBuffer->numMipmaps == 1);
				requireResourceStateTextureMip(
					*this->commandList.Get(), this->tracking, depthBuffer, 0, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			}
		}

		// Set framebuffer
		commandList->OMSetRenderTargets(
			framebuffer->numRenderTargets,
			framebuffer->numRenderTargets > 0 ? framebuffer->renderTargetDescriptors : nullptr,
			FALSE,
			framebuffer->hasDepthBuffer ? &framebuffer->depthBufferDescriptor : nullptr);

		return ZG_SUCCESS;
	}

	ZgResult setFramebufferViewport(
		const ZgRect& viewportRect) noexcept
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

	ZgResult setFramebufferScissor(
		const ZgRect& scissor) noexcept
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

	ZgResult clearRenderTargetOptimal(uint32_t renderTargetIdx) noexcept
	{
		if (!mFramebufferSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
		if (mFramebuffer->numRenderTargets <= renderTargetIdx) return ZG_WARNING_GENERIC;

		constexpr float ZEROS[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		constexpr float ONES[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		const float* clearColor = [&]() -> const float* {
			switch (mFramebuffer->renderTargetOptimalClearValues[renderTargetIdx]) {
			case ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED: return ZEROS;
			case ZG_OPTIMAL_CLEAR_VALUE_ZERO: return ZEROS;
			case ZG_OPTIMAL_CLEAR_VALUE_ONE: return ONES;
			}
			sfz_assert(false);
			return nullptr;
		}();

		commandList->ClearRenderTargetView(
			mFramebuffer->renderTargetDescriptors[renderTargetIdx], clearColor, 0, nullptr);

		return ZG_SUCCESS;
	}

	ZgResult clearRenderTargets(
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

	ZgResult clearRenderTargetsOptimal() noexcept
	{
		// Return error if no framebuffer is set
		if (!mFramebufferSet) {
			ZG_ERROR("clearRenderTargetsOptimal(): Must set a framebuffer before you can clear it");
			return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
		}

		for (uint32_t i = 0; i < mFramebuffer->numRenderTargets; i++) {
			ZgResult res = this->clearRenderTargetOptimal(i);
			if (res != ZG_SUCCESS) return res;
		}
		
		return ZG_SUCCESS;
	}

	ZgResult clearDepthBuffer(
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

	ZgResult clearDepthBufferOptimal() noexcept
	{
		// Return error if no framebuffer is set
		if (!mFramebufferSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
		if (!mFramebuffer->hasDepthBuffer) return ZG_WARNING_GENERIC;

		const float clearDepth = [&]() {
			switch (mFramebuffer->depthBufferOptimalClearValue) {
			case ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED: return 0.0f;
			case ZG_OPTIMAL_CLEAR_VALUE_ZERO: return 0.0f;
			case ZG_OPTIMAL_CLEAR_VALUE_ONE: return 1.0f;
			}
			sfz_assert(false);
			return 0.0f;
		}();

		// Clear depth buffer
		commandList->ClearDepthStencilView(
			mFramebuffer->depthBufferDescriptor, D3D12_CLEAR_FLAG_DEPTH, clearDepth, 0, 0, nullptr);

		return ZG_SUCCESS;
	}

	ZgResult setIndexBuffer(
		ZgBuffer* indexBuffer,
		ZgIndexBufferType type) noexcept
	{
		// Set buffer resource state
		if (indexBuffer->memoryType == ZG_MEMORY_TYPE_DEVICE) {
			requireResourceStateBuffer(
				*this->commandList.Get(), this->tracking, indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		}
		else if (indexBuffer->memoryType == ZG_MEMORY_TYPE_UPLOAD) {
			requireResourceStateBuffer(
				*this->commandList.Get(), this->tracking, indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
		}
		else {
			return ZG_ERROR_INVALID_ARGUMENT;
		}

		// Create index buffer view
		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = indexBuffer->resource.resource->GetGPUVirtualAddress();
		sfz_assert(indexBuffer->resource.allocation->GetSize() <= uint64_t(UINT32_MAX));
		indexBufferView.SizeInBytes = uint32_t(indexBuffer->resource.allocation->GetSize());
		indexBufferView.Format = type == ZG_INDEX_BUFFER_TYPE_UINT32 ?
			DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

		// Set index buffer
		commandList->IASetIndexBuffer(&indexBufferView);

		return ZG_SUCCESS;
	}

	ZgResult setVertexBuffer(
		uint32_t vertexBufferSlot,
		ZgBuffer* vertexBuffer) noexcept
	{
		// Need to have a pipeline set to verify vertex buffer binding
		if (!mPipelineSet) return ZG_ERROR_INVALID_COMMAND_LIST_STATE;

		// Check that the vertex buffer slot is not out of bounds for the bound pipeline
		const ZgPipelineRenderCreateInfo& pipelineInfo = mBoundPipelineRender->createInfo;
		if (pipelineInfo.numVertexBufferSlots <= vertexBufferSlot) {
			return ZG_ERROR_INVALID_COMMAND_LIST_STATE;
		}

		// Set buffer resource state
		if (vertexBuffer->memoryType == ZG_MEMORY_TYPE_DEVICE) {
			requireResourceStateBuffer(
				*this->commandList.Get(), this->tracking, vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}
		else if (vertexBuffer->memoryType == ZG_MEMORY_TYPE_UPLOAD) {
			requireResourceStateBuffer(
				*this->commandList.Get(), this->tracking, vertexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
		}
		else {
			return ZG_ERROR_INVALID_ARGUMENT;
		}

		// Create vertex buffer view
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = vertexBuffer->resource.resource->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = pipelineInfo.vertexBufferStridesBytes[vertexBufferSlot];
		vertexBufferView.SizeInBytes = uint32_t(vertexBuffer->resource.allocation->GetSize());

		// Set vertex buffer
		commandList->IASetVertexBuffers(vertexBufferSlot, 1, &vertexBufferView);

		return ZG_SUCCESS;
	}

	ZgResult drawTriangles(
		uint32_t startVertexIndex,
		uint32_t numVertices) noexcept
	{
		// Draw triangles
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(numVertices, 1, startVertexIndex, 0);
		return ZG_SUCCESS;
	}

	ZgResult drawTrianglesIndexed(
		uint32_t startIndex,
		uint32_t numTriangles) noexcept
	{
		// Draw triangles indexed
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawIndexedInstanced(numTriangles * 3, 1, startIndex, 0, 0);
		return ZG_SUCCESS;
	}

	ZgResult profileBegin(
		ZgProfiler* profiler,
		uint64_t& measurementIdOut) noexcept
	{
		// TODO: This is necessary because we don't get timestamp frequency for other queue types.
		//       Besides, timestamp queries only work on present and compute queues in the first place.
		sfz_assert(commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT);

		// Access profilers state through its mutex
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

	ZgResult profileEnd(
		ZgProfiler* profiler,
		uint64_t measurementId,
		uint64_t timestampTicksPerSecond) noexcept
	{
		// TODO: This is necessary because we don't get timestamp frequency for other queue types.
		//       Besides, timestamp queries only work on present and compute queues in the first place.
		sfz_assert(commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT);

		// Access profilers state through its mutex
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
			state.downloadBuffer->resource.resource,
			bufferOffset);

		// Store ticks per second
		state.ticksPerSecond[queryIdx] = timestampTicksPerSecond;

		return ZG_SUCCESS;
	}
};
