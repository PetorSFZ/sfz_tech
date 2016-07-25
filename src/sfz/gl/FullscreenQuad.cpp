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

#include "sfz/gl/FullscreenQuad.hpp"

#include <algorithm>

#include "sfz/gl/IncludeOpenGL.hpp"

namespace sfz {

namespace gl {

using std::int32_t;

FullscreenQuad::FullscreenQuad() noexcept
{
	const float positions[] = {
		-1.0f, -1.0f, 0.0f, // bottom-left
		1.0f, -1.0f, 0.0f, // bottom-right
		-1.0f, 1.0f, 0.0f, // top-left
		1.0f, 1.0f, 0.0f // top-right
	};
	const float uvCoords[] = {
		// bottom-left UV
		0.0f, 0.0f,
		// bottom-right UV
		1.0f, 0.0f,
		// top-left UV
		0.0f, 1.0f,
		// top-right UV
		1.0f, 1.0f
	};
	const unsigned int indices[] = {
		0, 1, 2,
		1, 3, 2
	};

	// Buffer objects
	glGenBuffers(1, &mPosBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mPosBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

	glGenBuffers(1, &mUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uvCoords), uvCoords, GL_STATIC_DRAW);

	glGenBuffers(1, &mIndexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mIndexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Vertex Array Object
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);

	glBindBuffer(GL_ARRAY_BUFFER, mPosBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
}

FullscreenQuad::FullscreenQuad(FullscreenQuad&& other) noexcept
{
	std::swap(this->mVAO, other.mVAO);
	std::swap(this->mPosBuffer, other.mPosBuffer);
	std::swap(this->mUVBuffer, other.mUVBuffer);
	std::swap(this->mIndexBuffer, other.mIndexBuffer);
}

FullscreenQuad& FullscreenQuad::operator= (FullscreenQuad&& other) noexcept
{
	std::swap(this->mVAO, other.mVAO);
	std::swap(this->mPosBuffer, other.mPosBuffer);
	std::swap(this->mUVBuffer, other.mUVBuffer);
	std::swap(this->mIndexBuffer, other.mIndexBuffer);
	return *this;
}

FullscreenQuad::~FullscreenQuad() noexcept
{
	glDeleteBuffers(1, &mPosBuffer);
	glDeleteBuffers(1, &mUVBuffer);
	glDeleteBuffers(1, &mIndexBuffer);
	glDeleteVertexArrays(1, &mVAO);
}

void FullscreenQuad::render() noexcept
{
	glBindVertexArray(mVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

} // namespace gl
} // namespace sfz
