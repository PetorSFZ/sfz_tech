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

#include "ph/ImguiRendering.hpp"

#include <algorithm>

#include <sfz/gl/IncludeOpenGL.hpp>

namespace ph {

// Imgui rendering shader
// ------------------------------------------------------------------------------------------------

Program compileImguiShader(Allocator* allocator) noexcept
{
	return Program::fromFile(
		"res_compgl/shaders/",
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
		"header_emscripten.glsl",
#else
		"header_desktop.glsl",
#endif
		"imgui.vert",
		"imgui.frag",
		[](uint32_t shaderProgram) {
			glBindAttribLocation(shaderProgram, 0, "inPos");
			glBindAttribLocation(shaderProgram, 1, "inTexcoord");
			glBindAttribLocation(shaderProgram, 2, "inColor");
		},
		allocator);
}

// ImguiVertexData class
// ------------------------------------------------------------------------------------------------

void ImguiVertexData::create(uint32_t maxNumVertices, uint32_t maxNumIndices) noexcept
{
	mMaxNumVertices = maxNumVertices;
	mMaxNumIndices = maxNumIndices;

	// Create vertex array object
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glGenVertexArraysOES(1, &mVAO);
	glBindVertexArrayOES(mVAO);
#else
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);
#endif

	// Vertex buffer
	glGenBuffers(1, &mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, maxNumVertices * sizeof(phImguiVertex),
		nullptr, GL_DYNAMIC_DRAW);

	// Set location of vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(phImguiVertex),
		(void*)offsetof(phImguiVertex, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(phImguiVertex),
		(void*)offsetof(phImguiVertex, texcoord));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(phImguiVertex),
		(void*)offsetof(phImguiVertex, color));

	// Index buffer
	glGenBuffers(1, &mIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxNumIndices * sizeof(uint32_t),
		nullptr, GL_DYNAMIC_DRAW);

	// Cleanup
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glBindVertexArrayOES(0);
#else
	glBindVertexArray(0);
#endif
}

void ImguiVertexData::swap(ImguiVertexData& other) noexcept
{
	std::swap(this->mVAO, other.mVAO);

	std::swap(this->mVertexBuffer, other.mVertexBuffer);
	std::swap(this->mMaxNumVertices, other.mMaxNumVertices);

	std::swap(this->mIndexBuffer, other.mIndexBuffer);
	std::swap(this->mMaxNumIndices, other.mMaxNumIndices);
}

void ImguiVertexData::destroy() noexcept
{
	glDeleteBuffers(1, &mVertexBuffer);
	glDeleteBuffers(1, &mIndexBuffer);
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glDeleteVertexArraysOES(1, &mVAO);
#else
	glDeleteVertexArrays(1, &mVAO);
#endif

	// Reset members
	mVAO = 0;

	mVertexBuffer = 0;
	mMaxNumVertices = 0;

	mIndexBuffer = 0;
	mMaxNumIndices = 0;
}

void ImguiVertexData::upload(
	const phImguiVertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices) noexcept
{
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

	// Allocate more vertex GPU memory if necessary
	if (numVertices > mMaxNumVertices) {
		mMaxNumVertices = numVertices;
		glBufferData(GL_ARRAY_BUFFER, mMaxNumVertices * sizeof(phImguiVertex),
			nullptr, GL_DYNAMIC_DRAW);
	}

	// Upload vertex data to GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * sizeof(phImguiVertex), vertices);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

	// Allocate more index GPU memory if necessary
	if (numIndices > mMaxNumIndices) {
		mMaxNumIndices = numIndices;
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mMaxNumIndices * sizeof(uint32_t),
			nullptr, GL_DYNAMIC_DRAW);
	}

	// Upload index data to GPU
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, numIndices * sizeof(uint32_t), indices);
}

void ImguiVertexData::bindVAO() noexcept
{
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glBindVertexArrayOES(mVAO);
#else
	glBindVertexArray(mVAO);
#endif
}

void ImguiVertexData::render(uint32_t indexOffset, uint32_t numIndices) noexcept
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT,
		(void*)(indexOffset * sizeof(uint32_t)));
}

} // namespace ph
