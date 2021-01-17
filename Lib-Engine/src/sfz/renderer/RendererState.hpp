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
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

#include <ZeroG-ImGui.hpp>

#include "sfz/config/GlobalConfig.hpp"
#include "sfz/resources/MeshResource.hpp"
#include "sfz/renderer/RendererUI.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/shaders/ShaderManager.hpp"

struct SDL_Window;

namespace sfz {

// RendererState
// ------------------------------------------------------------------------------------------------

struct GroupProfilingID final {
	strID groupName;
	uint64_t id = ~0ull;
};

struct FrameProfilingIDs final {
	uint64_t frameId = ~0ull;
	uint64_t imguiId = ~0ull;
	Arr64<GroupProfilingID> groupIds;
};

struct RendererState final {

	// Members
	// --------------------------------------------------------------------------------------------

	sfz::Allocator* allocator = nullptr;
	SDL_Window* window = nullptr;

	// The current index of the frame, increments at every frameBegin()
	uint64_t currentFrameIdx = 0;

	// Synchronization primitives to make sure we have finished rendering using a given set of
	//" PerFrameData" resources so we can start uploading new data to them.
	uint32_t frameLatency = 2;
	PerFrameData<zg::Fence> frameFences;

	vec2_i32 windowRes = vec2_i32(0);
	zg::Framebuffer windowFramebuffer;
	zg::CommandQueue presentQueue;
	zg::CommandQueue copyQueue;

	// Profiler
	zg::Profiler profiler;
	PerFrameData<FrameProfilingIDs> frameMeasurementIds;
	float lastRetrievedFrameTimeMs = 0.0f;
	uint64_t lastRetrievedFrameTimeFrameIdx = ~0ull;

	// UI
	RendererUI ui;

	// Imgui renderer
	const Setting* imguiScaleSetting = nullptr;
	zg::ImGuiRenderState* imguiRenderState = nullptr;;

	// Settings
	const Setting* vsync = nullptr;
	const Setting* flushPresentQueueEachFrame = nullptr;
	const Setting* flushCopyQueueEachFrame = nullptr;
	const Setting* emitDebugEvents = nullptr;

	// Path to current configuration
	str320 configPath;
};

} // namespace sfz
