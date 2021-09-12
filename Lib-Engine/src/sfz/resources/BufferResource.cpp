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

#include "sfz/resources/BufferResource.hpp"

namespace sfz {

// BufferResource
// ------------------------------------------------------------------------------------------------

void BufferResource::uploadBlockingUntyped(
	const void* data,
	u32 elementSizeIn,
	u32 numElements,
	ZgUploader* uploader,
	zg::CommandQueue& copyQueue)
{
	const u64 bufferSizeBytes = this->maxNumElements * this->elementSizeBytes;
	const u64 numBytes = elementSizeIn * numElements;
	sfz_assert(numBytes <= bufferSizeBytes);
	sfz_assert(elementSizeIn == this->elementSizeBytes);
	sfz_assert_hard(this->type == BufferResourceType::STATIC);

	// Copy data to GPU
	zg::CommandList cmdList;
	CHECK_ZG copyQueue.beginCommandListRecording(cmdList);
	CHECK_ZG cmdList.uploadToBuffer(uploader, staticMem.buffer.handle, 0, data, numBytes);
	CHECK_ZG cmdList.enableQueueTransition(staticMem.buffer);
	CHECK_ZG copyQueue.executeCommandList(cmdList);
	CHECK_ZG copyQueue.flush();
}

BufferResource BufferResource::createStatic(
	const char* name,
	u32 elementSize,
	u32 maxNumElements)
{
	sfz_assert(elementSize > 0);
	sfz_assert(maxNumElements > 0);
	BufferResource resource;
	resource.name = strID(name);
	resource.elementSizeBytes = elementSize;
	resource.maxNumElements = maxNumElements;
	resource.type = BufferResourceType::STATIC;
	CHECK_ZG resource.staticMem.buffer.create(
		elementSize * maxNumElements, ZG_MEMORY_TYPE_DEVICE, false, name);
	return resource;
}

BufferResource BufferResource::createStreaming(
	const char* name,
	u32 elementSize,
	u32 maxNumElements,
	u32 frameLatency)
{
	sfz_assert(elementSize > 0);
	sfz_assert(maxNumElements > 0);
	BufferResource resource;
	resource.name = strID(name);
	resource.elementSizeBytes = elementSize;
	resource.maxNumElements = maxNumElements;
	resource.type = BufferResourceType::STREAMING;

	const u64 sizeBytes = elementSize * maxNumElements;
	const bool committedAllocation = false;
	u32 frameIdx = 0;
	resource.streamingMem.init(frameLatency, [&](StreamingBufferMemory& memory) {
		str256 deviceDebugName("%s_%u", name, frameIdx);
		frameIdx += 1;
		CHECK_ZG memory.buffer.create(
			sizeBytes, ZG_MEMORY_TYPE_DEVICE, committedAllocation, deviceDebugName.str());
	});

	return resource;
}

} // namespace sfz
