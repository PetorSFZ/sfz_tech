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
	// Try to find index of pending buffer states
	uint32_t bufferStateIdx = ~0u;
	for (uint32_t i = 0; i < cmdListState.pendingBufferIdentifiers.size(); i++) {
		uint64_t identifier = cmdListState.pendingBufferIdentifiers[i];
		if (identifier == buffer->identifier) {
			bufferStateIdx = i;
			break;
		}
	}

	// If buffer does not have a pending state, create one
	if (bufferStateIdx == ~0u) {
		// Create pending buffer state
		bufferStateIdx = cmdListState.pendingBufferStates.size();
		cmdListState.pendingBufferIdentifiers.add(buffer->identifier);
		cmdListState.pendingBufferStates.add(PendingBufferState());

		// Set initial pending buffer state
		cmdListState.pendingBufferStates.last().buffer = buffer;
		cmdListState.pendingBufferStates.last().neededInitialState = requiredState;
		cmdListState.pendingBufferStates.last().currentState = requiredState;
	}

	PendingBufferState& pendingState = cmdListState.pendingBufferStates[bufferStateIdx];

	// Change state of buffer if necessary
	if (pendingState.currentState != requiredState) {
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			buffer->resource.resource,
			pendingState.currentState,
			requiredState);
		cmdList.ResourceBarrier(1, &barrier);
		pendingState.currentState = requiredState;
	}
}

inline void requireResourceStateTextureMip(
	ID3D12GraphicsCommandList& cmdList,
	ZgTrackerCommandListState& cmdListState,
	ZgTexture* texture,
	uint32_t mipLevel,
	D3D12_RESOURCE_STATES requiredState)
{
	// Try to find index of pending buffer states
	uint32_t textureStateIdx = ~0u;
	for (uint32_t i = 0; i < cmdListState.pendingTextureIdentifiers.size(); i++) {
		ZgTrackerCommandListState::TextureMipIdentifier identifier = cmdListState.pendingTextureIdentifiers[i];
		if (identifier.identifier == texture->identifier && identifier.mipLevel == mipLevel) {
			textureStateIdx = i;
			break;
		}
	}

	// If texture does not have a pending state, create one
	if (textureStateIdx == ~0u) {

		// Create pending buffer state
		textureStateIdx = cmdListState.pendingTextureStates.size();
		ZgTrackerCommandListState::TextureMipIdentifier identifier;
		identifier.identifier = texture->identifier;
		identifier.mipLevel = mipLevel;
		cmdListState.pendingTextureIdentifiers.add(identifier);
		cmdListState.pendingTextureStates.add(PendingTextureState());

		// Set initial pending buffer state
		cmdListState.pendingTextureStates.last().texture = texture;
		cmdListState.pendingTextureStates.last().mipLevel = mipLevel;
		cmdListState.pendingTextureStates.last().neededInitialState = requiredState;
		cmdListState.pendingTextureStates.last().currentState = requiredState;
	}

	PendingTextureState& pendingState = cmdListState.pendingTextureStates[textureStateIdx];

	// Change state of texture if necessary
	if (pendingState.currentState != requiredState) {
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			texture->resource.resource,
			pendingState.currentState,
			requiredState,
			mipLevel);
		cmdList.ResourceBarrier(1, &barrier);
		pendingState.currentState = requiredState;
	}
}

inline void requireResourceStateTextureAllMips(
	ID3D12GraphicsCommandList& cmdList,
	ZgTrackerCommandListState& cmdListState,
	ZgTexture* texture,
	D3D12_RESOURCE_STATES requiredState)
{
	// Get pending states
	uint32_t pendingStatesIndices[ZG_MAX_NUM_MIPMAPS] = {};
	for (uint32_t mipLevel = 0; mipLevel < texture->numMipmaps; mipLevel++) {

		// Try to find index of pending buffer states
		uint32_t textureStateIdx = ~0u;
		for (uint32_t i = 0; i < cmdListState.pendingTextureIdentifiers.size(); i++) {
			ZgTrackerCommandListState::TextureMipIdentifier identifier = cmdListState.pendingTextureIdentifiers[i];
			if (identifier.identifier == texture->identifier && identifier.mipLevel == mipLevel) {
				textureStateIdx = i;
				break;
			}
		}

		// If texture does not have a pending state, create one
		if (textureStateIdx == ~0u) {

			// Create pending buffer state
			textureStateIdx = cmdListState.pendingTextureStates.size();
			ZgTrackerCommandListState::TextureMipIdentifier identifier;
			identifier.identifier = texture->identifier;
			identifier.mipLevel = mipLevel;
			cmdListState.pendingTextureIdentifiers.add(identifier);
			cmdListState.pendingTextureStates.add(PendingTextureState());

			// Set initial pending buffer state
			cmdListState.pendingTextureStates.last().texture = texture;
			cmdListState.pendingTextureStates.last().mipLevel = mipLevel;
			cmdListState.pendingTextureStates.last().neededInitialState = requiredState;
			cmdListState.pendingTextureStates.last().currentState = requiredState;
		}

		// We can NOT store pointers here, array might increase size and invalidate pointers
		pendingStatesIndices[mipLevel] = textureStateIdx;
	}

	// Grab pointers to pending states
	PendingTextureState* pendingStates[ZG_MAX_NUM_MIPMAPS] = {};
	for (uint32_t i = 0; i < texture->numMipmaps; i++) {
		pendingStates[i] = &cmdListState.pendingTextureStates[pendingStatesIndices[i]];
	}

	// Create all necessary barriers
	CD3DX12_RESOURCE_BARRIER barriers[ZG_MAX_NUM_MIPMAPS] = {};
	uint32_t numBarriers = 0;
	for (uint32_t i = 0; i < texture->numMipmaps; i++) {
		if (pendingStates[i]->currentState != requiredState) {
			barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(
				texture->resource.resource,
				pendingStates[i]->currentState,
				requiredState,
				i);
			numBarriers += 1;
			pendingStates[i]->currentState = requiredState;
		}
	}

	// Submit barriers
	if (numBarriers != 0) {
		cmdList.ResourceBarrier(numBarriers, barriers);
	}
}

template<typename ExecBarriersFunc>
ZgResult executePreCommandListStateChanges(
	sfz::Array<PendingBufferState>& pendingBufferStates,
	sfz::Array<PendingTextureState>& pendingTextureStates,
	ExecBarriersFunc execBarriers) noexcept
{
	// Temporary storage array for the barriers to insert
	uint32_t numBarriers = 0;
	constexpr uint32_t MAX_NUM_BARRIERS = 512;
	CD3DX12_RESOURCE_BARRIER barriers[MAX_NUM_BARRIERS] = {};

	// Gather buffer barriers
	for (uint32_t i = 0; i < pendingBufferStates.size(); i++) {
		const PendingBufferState& state = pendingBufferStates[i];

		// Don't insert barrier if resource already is in correct state
		if (state.buffer->tracking.lastCommittedState == state.neededInitialState) {
			continue;
		}

		// Error out if we don't have enough space in our temp array
		if (numBarriers >= MAX_NUM_BARRIERS) {
			ZG_ERROR("Internal error, need to insert too many barriers. Fixable, please contact ZeroG devs.");
			return ZG_ERROR_GENERIC;
		}

		// Create barrier
		barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(
			state.buffer->resource.resource,
			state.buffer->tracking.lastCommittedState,
			state.neededInitialState);

		numBarriers += 1;
	}

	// Gather texture barriers
	for (uint32_t i = 0; i < pendingTextureStates.size(); i++) {
		const PendingTextureState& state = pendingTextureStates[i];

		// Don't insert barrier if resource already is in correct state
		if (state.texture->mipTrackings[state.mipLevel].lastCommittedState == state.neededInitialState){
			continue;
		}

		// Error out if we don't have enough space in our temp array
		if (numBarriers >= MAX_NUM_BARRIERS) {
			ZG_ERROR("Internal error, need to insert too many barriers. Fixable, please contact ZeroG devs.");
			return ZG_ERROR_GENERIC;
		}

		// Create barrier
		barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(
			state.texture->resource.resource,
			state.texture->mipTrackings[state.mipLevel].lastCommittedState,
			state.neededInitialState,
			state.mipLevel);

		numBarriers += 1;
	}

	// Exit if we do not need to insert any barriers
	if (numBarriers == 0) return ZG_SUCCESS;

	// Create small command list and execute barriers in it
	execBarriers(barriers, numBarriers);

	// Commit state changes
#pragma message("WARNING, probably serious race condition")
	// TODO: This is problematic and we probably need to something smarter. TL;DR, this comitted
	//       state is shared between all queues. Maybe it is enough to just put a mutex around it,
	//       but it is not obvious to me that that would be enough.
	for (uint32_t i = 0; i < pendingBufferStates.size(); i++) {
		const PendingBufferState& state = pendingBufferStates[i];
		state.buffer->tracking.lastCommittedState = state.currentState;
	}
	for (uint32_t i = 0; i < pendingTextureStates.size(); i++) {
		const PendingTextureState& state = pendingTextureStates[i];
		state.texture->mipTrackings[state.mipLevel].lastCommittedState = state.currentState;
	}

	return ZG_SUCCESS;
}
