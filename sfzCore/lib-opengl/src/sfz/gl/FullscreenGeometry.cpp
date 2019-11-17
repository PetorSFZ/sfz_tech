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

#include "sfz/gl/FullscreenGeometry.hpp"

#include <algorithm>

#include <sfz/math/Vector.hpp>

#include "sfz/gl/IncludeOpenGL.hpp"

namespace sfz {

namespace gl {

// Helper vertex class
// ------------------------------------------------------------------------------------------------

struct Vertex final {
	vec3 pos;
	vec2 texcoord;
};
static_assert(sizeof(Vertex) == sizeof(float) * 5, "Vertex is padded");

// FullscreenGeometry: State methods
// ------------------------------------------------------------------------------------------------

void FullscreenGeometry::create(FullscreenGeometryType type) noexcept
{
	if (type != FullscreenGeometryType::OGL_CLIP_SPACE_RIGHT_HANDED_FRONT_FACE) {
		return;
	}

	Vertex vertices[3];

	// Bottom left corner
	vertices[0].pos = vec3(-1.0f, -1.0f, 0.0f);
	vertices[0].texcoord = vec2(0.0f, 0.0f);

	// Bottom right corner
	vertices[1].pos = vec3(3.0f, -1.0f, 0.0f);
	vertices[1].texcoord = vec2(2.0f, 0.0f);

	// Top left corner
	vertices[2].pos = vec3(-1.0f, 3.0f, 0.0f);
	vertices[2].texcoord = vec2(0.0f, 2.0f);

	const uint32_t indices[3] = {0, 1, 2};

	// Vertex array object
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glGenVertexArraysOES(1, &mVAO);
	glBindVertexArrayOES(mVAO);
#else
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);
#endif

	// Buffer objects
	glGenBuffers(1, &mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 3, vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &mIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, texcoord));

}

void FullscreenGeometry::swap(FullscreenGeometry& other) noexcept
{
	std::swap(this->mVAO, other.mVAO);
	std::swap(this->mVertexBuffer, other.mVertexBuffer);
	std::swap(this->mIndexBuffer, other.mIndexBuffer);
}

void FullscreenGeometry::destroy() noexcept
{
	// Delete buffers
	glDeleteBuffers(1, &mVertexBuffer);
	glDeleteBuffers(1, &mIndexBuffer);
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glDeleteVertexArraysOES(1, &mVAO);
#else
	glDeleteVertexArrays(1, &mVAO);
#endif

	// Reset variables
	mVAO = 0;
	mVertexBuffer = 0;
	mIndexBuffer = 0;
}

// FullscreenGeometry: Methods
// ------------------------------------------------------------------------------------------------

void FullscreenGeometry::render() noexcept
{
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glBindVertexArrayOES(mVAO);
#else
	glBindVertexArray(mVAO);
#endif
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
}

} // namespace gl
} // namespace sfz
