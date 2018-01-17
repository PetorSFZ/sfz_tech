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

// Shader sources
// ------------------------------------------------------------------------------------------------

const char* IMGUI_VERTEX_SHADER_SRC = R"(

// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_VERTEX_IN vec2 inPos;
PH_VERTEX_IN vec2 inTexcoord;
PH_VERTEX_IN vec4 inColor;

// Output
PH_VERTEX_OUT vec2 texcoord;
PH_VERTEX_OUT vec4 color;

// Uniforms
uniform mat4 uProjMatrix;

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	texcoord = inTexcoord;
	color = inColor;
	gl_Position = uProjMatrix * vec4(inPos, 0.0, 1.0);
}

)";

const char* IMGUI_FRAGMENT_SHADER_SRC = R"(

// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_FRAGMENT_IN vec2 texcoord;
PH_FRAGMENT_IN vec4 color;

// Output
#ifdef PH_DESKTOP_GL
out vec4 fragOut;
#endif

// Uniforms
uniform sampler2D uTexture;

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	vec4 outTmp = color * PH_TEXREAD(uTexture, texcoord).x;

#ifdef PH_WEB_GL
	gl_FragColor = outTmp;
#else
	fragOut = outTmp;
#endif
}

)";

// ImguiVertexData class
// ------------------------------------------------------------------------------------------------

void ImguiVertexData::create(uint32_t maxNumVertices, uint32_t maxNumIndices) noexcept
{
	mMaxNumVertices = maxNumVertices;
	mMaxNumIndices = maxNumIndices;

	// Create vertex array object
#ifdef __EMSCRIPTEN__
	glGenVertexArraysOES(1, &mVAO);
	glBindVertexArrayOES(mVAO);
#else
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);
#endif

	// Vertex buffer
	glGenBuffers(1, &mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, maxNumVertices * sizeof(ImguiVertex),
		nullptr, GL_DYNAMIC_DRAW);

	// Set location of vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImguiVertex),
		(void*)offsetof(ImguiVertex, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImguiVertex),
		(void*)offsetof(ImguiVertex, texcoord));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImguiVertex),
		(void*)offsetof(ImguiVertex, color));

	// Index buffer
	glGenBuffers(1, &mIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxNumIndices * sizeof(uint32_t),
		nullptr, GL_DYNAMIC_DRAW);

	// Cleanup
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
#ifdef __EMSCRIPTEN__
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
#ifdef __EMSCRIPTEN__
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
	const ImguiVertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices) noexcept
{
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

	// Allocate more vertex GPU memory if necessary
	if (numVertices > mMaxNumVertices) {
		mMaxNumVertices = numVertices;
		glBufferData(GL_ARRAY_BUFFER, mMaxNumVertices * sizeof(ImguiVertex),
			nullptr, GL_DYNAMIC_DRAW);
	}

	// Upload vertex data to GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * sizeof(ImguiVertex), vertices);

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
#ifdef __EMSCRIPTEN__
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
