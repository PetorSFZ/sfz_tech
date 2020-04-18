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
#include <skipifzero_arrays.hpp>

namespace sfz {

// ZeroG logger
// -----------------------------------------------------------------------------------------------

ZgLogger getPhantasyEngineZeroGLogger() noexcept;

// ZeroG sfz::Allocator wrapper
// -----------------------------------------------------------------------------------------------

ZgAllocator createZeroGAllocatorWrapper(sfz::Allocator* allocator) noexcept;

// Error handling helpers
// -----------------------------------------------------------------------------------------------

// Checks result (zg::ErrorCode) from ZeroG call and log if not success, returns result unmodified
#define CHECK_ZG (sfz::CheckZgImpl(__FILE__, __LINE__)) %

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
	zg::Context& zgCtx, SDL_Window* window, sfz::Allocator* allocator, bool debugMode, bool vsync) noexcept;

void* getNativeHandle(SDL_Window* window) noexcept;


// PerFrameData template
// -----------------------------------------------------------------------------------------------

constexpr uint64_t MAX_NUM_FRAME_LATENCY = 3;

// A template used to signify that a given set of resources are frame-specific.
//
// For resources that are updated every frame (constants buffers, streaming vertex data such as
// imgui, etc) there need to be multiple copies of the memory on the GPU. Otherwise we can't start
// uploading the next frame's data until the previous frame has finished rendering. This template
// signifies that resources are "per-frame".
//
// Typically we should have a latency of at least two so that we can upload to resource while
// rendering using the other.
template<typename T>
class PerFrameData {
public:
	PerFrameData() noexcept {}
	PerFrameData(const PerFrameData&) = delete;
	PerFrameData& operator= (const PerFrameData&) = delete;
	PerFrameData(PerFrameData&& other) = default;
	PerFrameData& operator= (PerFrameData&& other) = default;
	~PerFrameData() noexcept { this->destroy(); }

	template<typename InitFun>
	void init(uint32_t latency, InitFun initFun)
	{
		sfz_assert(mData.isEmpty());
		for (uint32_t i = 0; i < latency; i++) {
			initFun(mData.add());
		}
	}
	void init(uint32_t latency) { this->init(latency, [](T&) {}); }

	template<typename DeinitFun>
	void destroy(DeinitFun deinitFun)
	{
		for (uint32_t i = 0; i < mData.size(); i++) {
			deinitFun(mData[i]);
		}
		mData.clear();
	}
	void destroy() { this->destroy([](T&) {}); }

	T& data(uint64_t frameIdx) { return mData[frameIdx % mData.size()]; }
	const T& data(uint64_t frameIdx) const { return mData[frameIdx % mData.size()]; }

private:
	ArrayLocal<T, MAX_NUM_FRAME_LATENCY> mData;
};

} // namespace sfz
