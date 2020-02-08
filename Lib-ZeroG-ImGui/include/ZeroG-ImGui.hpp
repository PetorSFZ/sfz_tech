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

#include <ZeroG-cpp.hpp>

struct phImguiVertex;
struct phImguiCommand;

namespace zg {

using sfz::vec2;
using sfz::vec3;
using sfz::vec4;

// Helper structs
// ------------------------------------------------------------------------------------------------

struct ImGuiVertex final {
	vec2 pos;
	vec2 texcoord;
	vec4 color;
};
static_assert(sizeof(ImGuiVertex) == 32, "ImGuiVertex is padded");

// ImGui Renderer
// ------------------------------------------------------------------------------------------------

struct ImGuiFrameState final {
	zg::Fence fence;
	sfz::Array<ImGuiVertex> convertedVertices;
	zg::Buffer uploadVertexBuffer;
	zg::Buffer uploadIndexBuffer;
};

struct ImGuiRenderState final {
	sfz::Allocator* allocator = nullptr;

	// Pipeline used to render ImGui
	zg::PipelineRender pipeline;

	// Font texture
	zg::MemoryHeap fontTextureHeap;
	zg::Texture2D fontTexture;

	// Memory used to upload ImgGi vertices and indices for a given frame
	zg::MemoryHeap uploadHeap;

	// Per frame state
	sfz::Array<ImGuiFrameState> frameStates;
	ImGuiFrameState& getFrameState(uint64_t idx) { return frameStates[idx % frameStates.size()]; }
};

zg::Result imguiInitRenderState(
	ImGuiRenderState& state,
	uint32_t frameLatency,
	sfz::Allocator* allocator,
	zg::CommandQueue& copyQueue,
	const ZgImageViewConstCpu& fontTexture) noexcept;

void imguiRender(
	ImGuiRenderState& state,
	uint64_t frameIdx,
	zg::CommandQueue& presentQueue,
	zg::Framebuffer& framebuffer,
	float scale,
	const phImguiVertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices,
	const phImguiCommand* commands,
	uint32_t numCommands) noexcept;

} // namespace zg
