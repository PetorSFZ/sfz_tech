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

#include <SDL.h>

#include <ZeroG.h>
#include <ZeroG-cpp.hpp>

#include <skipifzero.hpp>

namespace ph {

// ZeroG logger
// -----------------------------------------------------------------------------------------------

ZgLogger getPhantasyEngineZeroGLogger() noexcept;

// ZeroG sfz::Allocator wrapper
// -----------------------------------------------------------------------------------------------

ZgAllocator createZeroGAllocatorWrapper(sfz::Allocator* allocator) noexcept;

// Error handling helpers
// -----------------------------------------------------------------------------------------------

// Checks result (zg::ErrorCode) from ZeroG call and log if not success, returns result unmodified
#define CHECK_ZG (ph::CheckZgImpl(__FILE__, __LINE__)) %

// Implementation of CHECK_ZG
struct CheckZgImpl final {
	const char* file;
	int line;

	CheckZgImpl() = delete;
	CheckZgImpl(const char* file, int line) noexcept : file(file), line(line) {}

	bool operator% (zg::Result result) noexcept;
};

// Initialization helpers
// -----------------------------------------------------------------------------------------------

bool initializeZeroG(
	zg::Context& zgCtx, SDL_Window* window, sfz::Allocator* allocator, bool debugMode) noexcept;

void* getNativeHandle(SDL_Window* window) noexcept;

// PerFrame template
// ------------------------------------------------------------------------------------------------

// A template used to signify that a given set of resources are frame-specific.
//
// For resources that are updated every frame (constants buffers, streaming vertex data such as
// imgui, etc) there need to be multiple copies of the memory on the GPU. Otherwise we can't start
// uploading the next frame's data until the previous frame has finished rendering. This template
// signifies that resources are "per-frame", and it also contains the necessary synchronization
// primitives to sync that.
//
// Typically we should have at least two copies of each "PerFrame" state so we can upload to one
// while we render using the other.
template<typename T>
struct PerFrame {

	// A chunk of state (i.e. resources) for a specific frame.
	T state;

	// Fence that should be signaled (from GPU or CPU depending on type of resources and type of
	// upload) when resources have finished uploading from CPU. This fence should then be waited on
	// (on GPU) before the frame starts rendering using the resources.
	zg::Fence uploadFinished;

	// Fence that should be signaled (from GPU) when frame has finished rendering using the
	// resources. Typically the CPU should (blockingly) wait on this fence before starting to
	// upload the next frame's resources.
	zg::Fence renderingFinished;

	// Simple helper method that initializes both fences
	zg::Result initFences() noexcept
	{
		zg::Result res1 = uploadFinished.create();
		if (res1 != zg::Result::SUCCESS) return res1;
		zg::Result res2 = renderingFinished.create();
		if (res2 != zg::Result::SUCCESS) return res2;
		return zg::Result::SUCCESS;
	}

	// Simple helper methods that releases both fences
	void releaseFences() noexcept
	{
		uploadFinished.release();
		renderingFinished.release();
	}
};

// Framed template
// ------------------------------------------------------------------------------------------------

// The maximum number of frames that can be rendered simulatenously
constexpr uint64_t MAX_NUM_FRAMES = 2;

// A simple wrapper around the PerFrame template which makes it a bit easier to handle in a similar
// way in the codebase.
template<typename T>
struct Framed {

	PerFrame<T> states[MAX_NUM_FRAMES];

	PerFrame<T>& getState(uint64_t frameIdx) noexcept { return states[frameIdx % MAX_NUM_FRAMES]; }
	const PerFrame<T>& getState(uint64_t frameIdx) const noexcept { return states[frameIdx % MAX_NUM_FRAMES]; }

	template<typename InitFun>
	void initAllStates(InitFun initFun) noexcept
	{
		for (uint32_t i = 0; i < MAX_NUM_FRAMES; i++) {
			T& state = states[i].state;
			initFun(state);
		}
	}

	template<typename DeinitFun>
	void deinitAllStates(DeinitFun deinitFun) noexcept
	{
		for (uint32_t i = 0; i < MAX_NUM_FRAMES; i++) {
			T& state = states[i].state;
			deinitFun(state);
		}
	}

	zg::Result initAllFences() noexcept
	{
		for (uint32_t i = 0; i < MAX_NUM_FRAMES; i++) {
			zg::Result res = states[i].initFences();
			if (res != zg::Result::SUCCESS) return res;
		}
		return zg::Result::SUCCESS;
	}

	void releaseAllFences() noexcept
	{
		for (uint32_t i = 0; i < MAX_NUM_FRAMES; i++) {
			states[i].releaseFences();
		}
	}
};

} // namespace ph
