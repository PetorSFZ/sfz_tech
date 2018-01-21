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

#include <cstdint>

#include "ph/rendering/ImguiRenderingData.h"

namespace ph {

// Shader sources
// ------------------------------------------------------------------------------------------------

extern const char* IMGUI_VERTEX_SHADER_SRC;

extern const char* IMGUI_FRAGMENT_SHADER_SRC;

// ImguiVertexData class
// ------------------------------------------------------------------------------------------------

class ImguiVertexData final {
public:
	ImguiVertexData() noexcept = default;
	ImguiVertexData(const ImguiVertexData&) = delete;
	ImguiVertexData& operator= (const ImguiVertexData&) = delete;
	ImguiVertexData(ImguiVertexData&& other) noexcept { swap(other); }
	ImguiVertexData& operator= (ImguiVertexData&& other) noexcept { swap(other); return *this; }
	~ImguiVertexData() noexcept { destroy(); }

	void create(uint32_t maxNumVertices, uint32_t maxNumIndices) noexcept;
	void swap(ImguiVertexData& other) noexcept;
	void destroy() noexcept;

	void upload(
		const phImguiVertex* vertices,
		uint32_t numVertices,
		const uint32_t* indices,
		uint32_t numIndices) noexcept;
	void bindVAO() noexcept;
	void render(uint32_t indexOffset, uint32_t numIndices) noexcept;

private:
	uint32_t mVAO = 0;

	uint32_t mVertexBuffer = 0;
	uint32_t mMaxNumVertices = 0;

	uint32_t mIndexBuffer = 0;
	uint32_t mMaxNumIndices = 0;
};

} // namespace ph
