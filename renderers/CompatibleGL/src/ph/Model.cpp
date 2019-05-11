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

#include "ph/Model.hpp"

#include <algorithm>

#include <sfz/Assert.hpp>
#include <sfz/gl/IncludeOpenGL.hpp>

namespace ph {

// ModelComponent: State methods
// ------------------------------------------------------------------------------------------------

void ModelComponent::create(const phConstMeshComponentView& view, uint32_t numMaterials) noexcept
{
	if (view.materialIdx != ~0u) sfz_assert_debug(view.materialIdx < numMaterials);
	this->destroy();

	// Index buffer
	glGenBuffers(1, &mIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(uint32_t) * view.numIndices, view.indices, GL_STATIC_DRAW);

	// Set members
	mNumIndices = view.numIndices;
	mMaterialIndex = view.materialIdx;
}

void ModelComponent::swap(ModelComponent& other) noexcept
{
	std::swap(this->mIndexBuffer, other.mIndexBuffer);
	std::swap(this->mNumIndices, other.mNumIndices);
	std::swap(this->mMaterialIndex, other.mMaterialIndex);
}

void ModelComponent::destroy() noexcept
{
	glDeleteBuffers(1, &mIndexBuffer);

	// Reset variables
	mIndexBuffer = 0;
	mNumIndices = 0;
	mMaterialIndex = 0;
}

// ModelComponent: Methods
// ------------------------------------------------------------------------------------------------

void ModelComponent::render() noexcept
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glDrawElements(GL_TRIANGLES, mNumIndices, GL_UNSIGNED_INT, 0);
}

// Model: State methods
// ------------------------------------------------------------------------------------------------

void Model::create(const phConstMeshView& mesh, Allocator* allocator) noexcept
{
	this->destroy();

	// Vertex array object
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(phVertex) * mesh.numVertices,
		mesh.vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(phVertex),
		(void*)offsetof(phVertex, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(phVertex),
		(void*)offsetof(phVertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(phVertex),
		(void*)offsetof(phVertex, texcoord));

	// Create components
	mComponents.create(mesh.numComponents, allocator);
	for (uint32_t compIdx = 0; compIdx < mesh.numComponents; compIdx++) {
		mComponents.add(ModelComponent(mesh.components[compIdx], mesh.numMaterials));
	}

	// Materials
	mMaterials.create(mesh.numMaterials, allocator);
	mMaterials.add(mesh.materials, mesh.numMaterials);

	// Cleanup
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glBindVertexArrayOES(0);
#else
	glBindVertexArray(0);
#endif
}

void Model::swap(Model& other) noexcept
{
	std::swap(this->mVAO, other.mVAO);
	std::swap(this->mVertexBuffer, other.mVertexBuffer);
	this->mComponents.swap(other.mComponents);
	this->mMaterials.swap(other.mMaterials);
}

void Model::destroy() noexcept
{
	// Delete buffers
	glDeleteBuffers(1, &mVertexBuffer);
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glDeleteVertexArraysOES(1, &mVAO);
#else
	glDeleteVertexArrays(1, &mVAO);
#endif

	// Reset members
	mVAO = 0;
	mVertexBuffer = 0;
	mComponents.destroy();
	mMaterials.destroy();
}

// Model: Methods
// ------------------------------------------------------------------------------------------------

void Model::bindVAO() noexcept
{
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glBindVertexArrayOES(mVAO);
#else
	glBindVertexArray(mVAO);
#endif
}

} // namespace ph
