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

#include <ZeroG.h>

namespace zg {

// ImGui Renderer
// ------------------------------------------------------------------------------------------------

struct ImGuiRenderState;

ZgResult imguiInitRenderState(
	ImGuiRenderState*& stateOut,
	u32 frameLatency,
	SfzAllocator* allocator,
	zg::CommandQueue& copyQueue,
	const ZgImageViewConstCpu& fontTexture) noexcept;

void imguiDestroyRenderState(ImGuiRenderState*& state) noexcept;

void imguiRender(
	ImGuiRenderState* state,
	u64 frameIdx,
	zg::CommandList& cmdList,
	u32 fbWidth,
	u32 fbHeight,
	f32 scale,
	zg::Profiler* profiler = nullptr,
	u64* measurmentIdOut = nullptr) noexcept;

} // namespace zg
