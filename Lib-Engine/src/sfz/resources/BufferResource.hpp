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

#pragma once

#include <skipifzero.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

#include "sfz/renderer/ZeroGUtils.hpp"

namespace sfz {

// BufferResource
// ------------------------------------------------------------------------------------------------

enum class BufferResourceType : uint32_t {
	STATIC = 0,
	STREAMING = 1
};

struct StaticBufferMemory final {
	zg::Buffer buffer;
};

struct StreamingBufferMemory final {
	uint64_t lastFrameIdxTouched = 0;
	zg::Buffer uploadBuffer;
	zg::Buffer deviceBuffer;
};

struct BufferResource final {
	strID name;
	uint32_t elementSizeBytes = 0;
	uint32_t maxNumElements = 0;
	BufferResourceType type = BufferResourceType::STATIC;
	StaticBufferMemory staticMem;
	PerFrameData<StreamingBufferMemory> streamingMem;
	
	static BufferResource createStatic(
		const char* name,
		uint32_t elementSize,
		uint32_t maxNumElements);

	static BufferResource createStreaming(
		const char* name,
		uint32_t elementSize,
		uint32_t maxNumElements,
		uint32_t frameLatency);
};

} // namespace sfz
