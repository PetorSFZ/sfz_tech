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

#include <sfz/containers/DynArray.hpp>

#include <ZeroG-cpp.hpp>

#include "ph/config/GlobalConfig.hpp"
#include "ph/renderer/ZeroGUtils.hpp"
#include "ph/rendering/ImageView.hpp"
#include "ph/rendering/ImguiRenderingData.hpp"

namespace ph {

using sfz::DynArray;
using sfz::vec2;
using sfz::vec2_i32;
using sfz::vec4;

// Vertex
// ------------------------------------------------------------------------------------------------

struct ImGuiVertex final {
	vec2 pos;
	vec2 texcoord;
	vec4 color;
};
static_assert(sizeof(ImGuiVertex) == 32, "ImGuiVertex is padded");

// ImGuiRenderer
// ------------------------------------------------------------------------------------------------

struct ImGuiFrameState final {
	DynArray<ImGuiVertex> convertedVertices;
	zg::Buffer uploadVertexBuffer;
	zg::Buffer uploadIndexBuffer;
};

class ImGuiRenderer final {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ImGuiRenderer() noexcept = default;
	ImGuiRenderer(const ImGuiRenderer&) = delete;
	ImGuiRenderer& operator= (const ImGuiRenderer&) = delete;
	ImGuiRenderer(ImGuiRenderer&& other) noexcept { this->swap(other); }
	ImGuiRenderer& operator= (ImGuiRenderer&& other) noexcept { this->swap(other); return *this; }
	~ImGuiRenderer() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool init(
		sfz::Allocator* allocator,
		zg::CommandQueue& copyQueue,
		const phConstImageView& fontTexture) noexcept;
	void swap(ImGuiRenderer& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void render(
		uint64_t frameIdx,
		zg::CommandQueue& presentQueue,
		zg::Framebuffer& framebuffer,
		vec2_i32 framebufferRes,
		const phImguiVertex* vertices,
		uint32_t numVertices,
		const uint32_t* indices,
		uint32_t numIndices,
		const phImguiCommand* commands,
		uint32_t numCommands) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	sfz::Allocator* mAllocator = nullptr;

	// Pipeline used to render imgui gui with
	zg::PipelineRender mPipeline;

	// Font texture
	zg::MemoryHeap mFontTextureHeap;
	zg::Texture2D mFontTexture;

	// Memory used to upload imgui vertices and indices for a given frame
	zg::MemoryHeap mUploadHeap;

	// Per frame state
	Framed<ImGuiFrameState> mFrameStates;

	// Settings
	const Setting* mScaleSetting = nullptr;
};

} // namespace ph
