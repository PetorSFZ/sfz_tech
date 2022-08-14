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
#include "d3d12/D3D12ResourceTrackingState.hpp"

// Tracking functions
// ------------------------------------------------------------------------------------------------

inline void requireResourceStateBuffer(
	ID3D12GraphicsCommandList& cmdList,
	ZgTrackerCommandListState& cmdListState,
	ZgBuffer* buffer,
	D3D12_RESOURCE_STATES requiredState)
{
	// Try to get pending buffer state, create it if it does not exist
	PendingBufferState* pendingState = cmdListState.pendingBuffers.get(buffer);
	if (pendingState == nullptr) {
		pendingState = &cmdListState.pendingBuffers.put(buffer, PendingBufferState());
		pendingState->buffer = buffer;
		pendingState->neededInitialState = requiredState;
		pendingState->currentState = requiredState;
	}

	// Change state of buffer if necessary
	if (pendingState->currentState != requiredState) {
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			buffer->resource.resource,
			pendingState->currentState,
			requiredState);
		cmdList.ResourceBarrier(1, &barrier);
		pendingState->currentState = requiredState;
	}
}

inline void requireResourceStateTextureMip(
	ID3D12GraphicsCommandList& cmdList,
	ZgTrackerCommandListState& cmdListState,
	ZgTexture* texture,
	u32 mipLevel,
	D3D12_RESOURCE_STATES requiredState)
{
	// Try to get pending texture mip state, create it if it does not exist
	PendingTextureState* pendingState =
		cmdListState.pendingTextureMips.get(TextureMip(texture, mipLevel));
	if (pendingState == nullptr) {
		pendingState = &cmdListState.pendingTextureMips.put(
			TextureMip(texture, mipLevel), PendingTextureState());
		pendingState->texture = texture;
		pendingState->mipLevel = mipLevel;
		pendingState->neededInitialState = requiredState;
		pendingState->currentState = requiredState;
	}

	// Change state of texture if necessary
	if (pendingState->currentState != requiredState) {
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			texture->resource.resource,
			pendingState->currentState,
			requiredState,
			mipLevel);
		cmdList.ResourceBarrier(1, &barrier);
		pendingState->currentState = requiredState;
	}
}

inline void requireResourceStateTextureAllMips(
	ID3D12GraphicsCommandList& cmdList,
	ZgTrackerCommandListState& cmdListState,
	ZgTexture* texture,
	D3D12_RESOURCE_STATES requiredState)
{
	// Create pending states if they do not exist
	// Note: Can NOT store pointers here, hash map can be resized
	for (u32 i = 0; i < texture->numMipmaps; i++) {
		if (cmdListState.pendingTextureMips.get(TextureMip(texture, i)) == nullptr) {
			PendingTextureState& pendingState = cmdListState.pendingTextureMips.put(
				TextureMip(texture, i), PendingTextureState());
			pendingState.texture = texture;
			pendingState.mipLevel = i;
			pendingState.neededInitialState = requiredState;
			pendingState.currentState = requiredState;
		}
	}

	// Get pointers to pending states
	SfzArrayLocal<PendingTextureState*, ZG_MAX_NUM_MIPMAPS> pendingStates;
	for (u32 i = 0; i < texture->numMipmaps; i++) {
		PendingTextureState* pendingState =
			cmdListState.pendingTextureMips.get(TextureMip(texture, i));
		sfz_assert_hard(pendingState != nullptr);
		pendingStates.add(pendingState);
	}

	// Create all necessary barriers
	SfzArrayLocal<CD3DX12_RESOURCE_BARRIER, ZG_MAX_NUM_MIPMAPS> barriers;
	for (u32 i = 0; i < texture->numMipmaps; i++) {
		if (pendingStates[i]->currentState != requiredState) {
			barriers.add(CD3DX12_RESOURCE_BARRIER::Transition(
				texture->resource.resource,
				pendingStates[i]->currentState,
				requiredState,
				i));
			pendingStates[i]->currentState = requiredState;
		}
	}

	// Submit barriers
	if (!barriers.isEmpty()) {
		cmdList.ResourceBarrier(barriers.size(), barriers.data());
	}
}

template<typename ExecBarriersFunc>
inline void executeCommandLists(
	ID3D12CommandQueue& queue,
	ID3D12CommandList* const* cmdLists,
	ZgTrackerCommandListState* const* cmdListStates,
	u32 numCmdLists,
	ExecBarriersFunc execBarriers,
	bool isBarrierList) noexcept
{
	sfz_assert_hard(numCmdLists == 1);
	ZgTrackerCommandListState& tracking = *cmdListStates[0];

	// No need for state tracking if this is a barrier only command list
	if (!isBarrierList) {

		// Temporary storage array for the barriers to insert
		u32 numBarriers = 0;
		constexpr u32 MAX_NUM_BARRIERS = 512;
		CD3DX12_RESOURCE_BARRIER barriers[MAX_NUM_BARRIERS] = {};

		// Gather buffer barriers
		const PendingBufferState* pendingBuffers = tracking.pendingBuffers.values();
		for (u32 i = 0; i < tracking.pendingBuffers.size(); i++) {
			const PendingBufferState& state = pendingBuffers[i];

			// Don't insert barrier if resource already is in correct state
			if (state.buffer->tracking.lastCommittedState == state.neededInitialState) {
				continue;
			}

			// Create barrier
			sfz_assert_hard(numBarriers < MAX_NUM_BARRIERS);
			barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(
				state.buffer->resource.resource,
				state.buffer->tracking.lastCommittedState,
				state.neededInitialState);

			numBarriers += 1;
		}

		// Gather texture barriers
		const PendingTextureState* pendingTextures = tracking.pendingTextureMips.values();
		for (u32 i = 0; i < tracking.pendingTextureMips.size(); i++) {
			const PendingTextureState& state = pendingTextures[i];

			// Don't insert barrier if resource already is in correct state
			if (state.texture->mipTrackings[state.mipLevel].lastCommittedState == state.neededInitialState) {
				continue;
			}

			// Create barrier
			sfz_assert_hard(numBarriers < MAX_NUM_BARRIERS);
			barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(
				state.texture->resource.resource,
				state.texture->mipTrackings[state.mipLevel].lastCommittedState,
				state.neededInitialState,
				state.mipLevel);

			numBarriers += 1;
		}

		// Create small command list and execute barriers in it
		if (numBarriers != 0) {
			[[maybe_unused]] ZgResult ignore = execBarriers(barriers, numBarriers);
		}

		// Commit state changes
#pragma message("WARNING, probably serious race condition")
	// TODO: This is problematic and we probably need to something smarter. TL;DR, this comitted
	//       state is shared between all queues. Maybe it is enough to just put a mutex around it,
	//       but it is not obvious to me that that would be enough.
		for (u32 i = 0; i < tracking.pendingBuffers.size(); i++) {
			const PendingBufferState& state = pendingBuffers[i];
			state.buffer->tracking.lastCommittedState = state.currentState;
		}
		for (u32 i = 0; i < tracking.pendingTextureMips.size(); i++) {
			const PendingTextureState& state = pendingTextures[i];
			state.texture->mipTrackings[state.mipLevel].lastCommittedState = state.currentState;
		}
	}

	else {
		// Can only be one command list if we are just executing barriers
		sfz_assert_hard(numCmdLists == 1);
	}

	// Execute command lists
	queue.ExecuteCommandLists(numCmdLists, cmdLists);

	// Clear tracking state
	tracking.pendingBuffers.clear();
	tracking.pendingTextureMips.clear();
}
