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

struct ZgTrackerCommandListState final {
	SFZ_DECLARE_DROP_TYPE(ZgTrackerCommandListState);

	void init(sfz::Allocator* allocator)
	{
		pendingBufferIdentifiers.init(64, allocator, sfz_dbg("ZgTrackerCommandListState"));
		pendingBufferStates.init(64, allocator, sfz_dbg("ZgTrackerCommandListState"));
		pendingTextureIdentifiers.init(64, allocator, sfz_dbg("ZgTrackerCommandListState"));
		pendingTextureStates.init(64, allocator, sfz_dbg("ZgTrackerCommandListState"));
	}

	void destroy()
	{
		this->pendingBufferIdentifiers.destroy();
		this->pendingBufferStates.destroy();
		this->pendingTextureIdentifiers.destroy();
		this->pendingTextureStates.destroy();
	}

	sfz::Array<uint64_t> pendingBufferIdentifiers;
	sfz::Array<PendingBufferState> pendingBufferStates;

	struct TextureMipIdentifier {
		uint64_t identifier = ~0u;
		uint32_t mipLevel = ~0u;
	};
	sfz::Array<TextureMipIdentifier> pendingTextureIdentifiers;
	sfz::Array<PendingTextureState> pendingTextureStates;
};
