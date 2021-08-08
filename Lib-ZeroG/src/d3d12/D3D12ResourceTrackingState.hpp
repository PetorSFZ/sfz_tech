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
#include <skipifzero_hash_maps.hpp>

#include "ZeroG.h"
#include "d3d12/D3D12Common.hpp"

struct ZgBuffer;
struct ZgTexture;

// Resource state
// ------------------------------------------------------------------------------------------------

struct ZgTrackerResourceState final {

	// The current resource state of the resource. Committed because the state has been committed
	// in a command list which has been executed on a queue. There may be pending state changes
	// in command lists not yet executed.
	// TODO: Mutex protecting this? How handle changes submitted on different queues simulatenously?
	D3D12_RESOURCE_STATES lastCommittedState = D3D12_RESOURCE_STATE_COMMON;
};

// CommandList state
// ------------------------------------------------------------------------------------------------

struct PendingBufferState final {

	// The associated D3D12Buffer
	ZgBuffer* buffer = nullptr;

	// The state the resource need to be in before the command list is executed
	D3D12_RESOURCE_STATES neededInitialState = D3D12_RESOURCE_STATE_COMMON;

	// The state the resource is in after the command list is executed
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
};

struct PendingTextureState final {

	// The associated D3D12Texture
	ZgTexture* texture = nullptr;

	// The mip level of the associated texture
	uint32_t mipLevel = ~0u;

	// The state the resource need to be in before the command list is executed
	D3D12_RESOURCE_STATES neededInitialState = D3D12_RESOURCE_STATE_COMMON;

	// The state the resource is in after the command list is executed
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
};

struct TextureMip {
	ZgTexture* tex = nullptr;
	uint32_t mipLevel = ~0u;

	TextureMip() = default;
	TextureMip(const TextureMip&) = default;
	TextureMip& operator= (const TextureMip&) = default;

	TextureMip(ZgTexture* tex, uint32_t mipLevel) : tex(tex), mipLevel(mipLevel) {}

	bool operator== (TextureMip other) const { return this->tex == other.tex && this->mipLevel == other.mipLevel; }
	bool operator!= (TextureMip other) const { return !(*this == other); }
};

namespace sfz {
	inline uint64_t hash(const TextureMip& val)
	{
		// hash_combine algorithm from boost
		uint64_t hash = 0;
		hash ^= sfz::hash(val.tex) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= sfz::hash(val.mipLevel) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		return hash;
	}
}

struct ZgTrackerCommandListState final {
	SFZ_DECLARE_DROP_TYPE(ZgTrackerCommandListState);

	void init(SfzAllocator* allocator)
	{
		pendingBuffers.init(64, allocator, sfz_dbg("ZgTrackerCommandListState"));
		pendingTextureMips.init(64, allocator, sfz_dbg("ZgTrackerCommandListState"));
	}

	void destroy()
	{
		this->pendingBuffers.destroy();
		this->pendingTextureMips.destroy();
	}

	sfz::HashMap<ZgBuffer*, PendingBufferState> pendingBuffers;
	sfz::HashMap<TextureMip, PendingTextureState> pendingTextureMips;
};
