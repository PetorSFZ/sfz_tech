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

BufferResource BufferResource::createStatic(
	const char* name,
	uint32_t elementSize,
	uint32_t maxNumElements)
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
	uint32_t elementSize,
	uint32_t maxNumElements,
	uint32_t frameLatency)
{
	sfz_assert(elementSize > 0);
	sfz_assert(maxNumElements > 0);
	BufferResource resource;
	resource.name = strID(name);
	resource.elementSizeBytes = elementSize;
	resource.maxNumElements = maxNumElements;
	resource.type = BufferResourceType::STREAMING;

	const uint64_t sizeBytes = elementSize * maxNumElements;
	const bool committedAllocation = false;
	uint32_t frameIdx = 0;
	resource.streamingMem.init(frameLatency, [&](StreamingBufferMemory& memory) {
		str256 uploadDebugName("%s_upload_%u", name, frameIdx);
		str256 deviceDebugName("%s_device_%u", name, frameIdx);
		frameIdx += 1;
		CHECK_ZG memory.uploadBuffer.create(
			sizeBytes, ZG_MEMORY_TYPE_UPLOAD, committedAllocation, uploadDebugName.str());
		CHECK_ZG memory.deviceBuffer.create(
			sizeBytes, ZG_MEMORY_TYPE_DEVICE, committedAllocation, deviceDebugName.str());
	});

	return resource;
}

} // namespace sfz
