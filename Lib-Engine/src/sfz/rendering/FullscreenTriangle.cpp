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

#include "sfz/rendering/FullscreenTriangle.hpp"

#include <skipifzero.hpp>

namespace sfz {

using sfz::Vertex;
using sfz::vec2;
using sfz::vec3;

// Statics
// ------------------------------------------------------------------------------------------------

static constexpr Vertex TRIANGLE_VERTICES[] = {
	Vertex(vec3(-1.0f, -1.0f, 0.0f), vec3(0.0f), vec2(0.0f, 1.0f)), // Bottom left
	Vertex(vec3(3.0f, -1.0f, 0.0f), vec3(0.0f), vec2(2.0f, 1.0f)), // Bottom right
	Vertex(vec3(-1.0f, 3.0f, 0.0f), vec3(0.0f), vec2(0.0f, -1.0f)), // Top lef
};
static constexpr uint32_t NUM_TRIANGLE_VERTICES = 3;

static constexpr uint32_t TRIANGLE_INDICES[] = {
	0, 1, 2
};
static constexpr uint32_t NUM_TRIANGLE_INDICES = 3;

// Function that returns a mesh containing a "fullscreen" triangle
// ------------------------------------------------------------------------------------------------

sfz::Mesh createFullscreenTriangle(SfzAllocator* allocator, float clipSpaceDepth)
{
	sfz::Mesh mesh;

	// Vertices
	mesh.vertices.init(NUM_TRIANGLE_VERTICES, allocator, sfz_dbg(""));
	mesh.vertices.add(TRIANGLE_VERTICES, NUM_TRIANGLE_VERTICES);

	// Set clip space depth
	for (Vertex& v : mesh.vertices) {
		v.pos.z = clipSpaceDepth;
	}

	// Indices
	mesh.indices.init(NUM_TRIANGLE_INDICES, allocator, sfz_dbg(""));
	mesh.indices.add(TRIANGLE_INDICES, NUM_TRIANGLE_INDICES);

	// Components
	sfz::MeshComponent comp;
	comp.materialIdx = 0;
	comp.firstIndex = 0;
	comp.numIndices = NUM_TRIANGLE_INDICES;
	mesh.components.init(1, allocator, sfz_dbg(""));
	mesh.components.add(std::move(comp));

	// Material
	mesh.materials.init(1, allocator, sfz_dbg(""));
	mesh.materials.add(sfz::Material());

	return mesh;
}

} // namespace sfz
