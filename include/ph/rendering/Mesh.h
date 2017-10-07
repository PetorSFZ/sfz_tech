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

#ifdef __cplusplus
#include <cstdint>
#include <sfz/math/Vector.hpp>
#include <sfz/containers/DynArray.hpp>
using std::uint32_t;
#else
#include <stdint.h>
#endif

#include "ph/rendering/Vertex.h"

// C Mesh struct
// ------------------------------------------------------------------------------------------------

extern "C"
typedef struct {
	phVertex* vertices;
	uint32_t* materialIndices;
	uint32_t numVertices;
	uint32_t* indices;
	uint32_t numIndices;
} phMesh;

// C++ Mesh struct
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace ph {

using sfz::DynArray;

struct Mesh final {
	DynArray<Vertex> vertices;
	DynArray<uint32_t> materialIndices;
	DynArray<uint32_t> indices;

	// Returns a phMesh that shows the contents of this Mesh. The phMesh does not own the memory
	// and will only be valid as long as the Mesh instance is not modified.
	inline phMesh cView() noexcept
	{
		phMesh tmp;
		tmp.vertices = reinterpret_cast<phVertex*>(this->vertices.data());
		tmp.materialIndices = this->materialIndices.data();
		tmp.numVertices = this->vertices.size();
		tmp.indices = this->indices.data();
		tmp.numIndices = this->indices.size();
		return tmp;
	}
};

} // namespace ph

#endif
