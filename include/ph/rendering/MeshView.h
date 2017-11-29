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
using std::uint32_t;
#else
#include <stdint.h>
#endif

#include "ph/ExternC.h"
#include "ph/rendering/Vertex.h"

// MeshView structs (C)
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	phVertex* vertices;
	uint32_t* materialIndices;
	uint32_t numVertices;
	uint32_t* indices;
	uint32_t numIndices;
} phMeshView;

PH_EXTERN_C
typedef struct {
	const phVertex* vertices;
	const uint32_t* materialIndices;
	uint32_t numVertices;
	const uint32_t* indices;
	uint32_t numIndices;
} phConstMeshView;

// MeshView structs (C++)
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace ph {

struct MeshView final {
	Vertex* vertices = nullptr;
	uint32_t* materialIndices = nullptr;
	uint32_t numVertices = 0;
	uint32_t* indices = nullptr;
	uint32_t numIndices = 0;

	// Implicit conversions
	MeshView() noexcept = default;
	inline MeshView(const phMeshView& view) noexcept;
	inline operator phMeshView() const noexcept;
	inline operator phConstMeshView() const noexcept;
};

struct ConstMeshView final {
	const Vertex* vertices = nullptr;
	const uint32_t* materialIndices = nullptr;
	uint32_t numVertices = 0;
	const uint32_t* indices = nullptr;
	uint32_t numIndices = 0;

	// Implicit conversions
	ConstMeshView() noexcept = default;
	inline ConstMeshView(const phMeshView& view) noexcept;
	inline ConstMeshView(const phConstMeshView& view) noexcept;
	inline ConstMeshView(const MeshView& view) noexcept;
	inline operator phConstMeshView() const noexcept;
};

inline MeshView::MeshView(const phMeshView& view) noexcept
:
	vertices(reinterpret_cast<Vertex*>(view.vertices)),
	materialIndices(view.materialIndices),
	numVertices(view.numVertices),
	indices(view.indices),
	numIndices(view.numIndices)
{ }

inline MeshView::operator phMeshView() const noexcept
{
	phMeshView tmp;
	tmp.vertices = reinterpret_cast<phVertex*>(this->vertices);
	tmp.materialIndices = this->materialIndices;
	tmp.numVertices = this->numVertices;
	tmp.indices = this->indices;
	tmp.numIndices = this->numIndices;
	return tmp;
}

inline MeshView::operator phConstMeshView() const noexcept
{
	phConstMeshView tmp;
	tmp.vertices = reinterpret_cast<const phVertex*>(this->vertices);
	tmp.materialIndices = this->materialIndices;
	tmp.numVertices = this->numVertices;
	tmp.indices = this->indices;
	tmp.numIndices = this->numIndices;
	return tmp;
}

inline ConstMeshView::ConstMeshView(const phMeshView& view) noexcept
:
	ConstMeshView(MeshView(view))
{ }

inline ConstMeshView::ConstMeshView(const phConstMeshView& view) noexcept
:
	vertices(reinterpret_cast<const Vertex*>(view.vertices)),
	materialIndices(view.materialIndices),
	numVertices(view.numVertices),
	indices(view.indices),
	numIndices(view.numIndices)
{ }

inline ConstMeshView::ConstMeshView(const MeshView& view) noexcept
:
	vertices(view.vertices),
	materialIndices(view.materialIndices),
	numVertices(view.numVertices),
	indices(view.indices),
	numIndices(view.numIndices)
{ }

inline ConstMeshView::operator phConstMeshView() const noexcept
{
	phConstMeshView tmp;
	tmp.vertices = reinterpret_cast<const phVertex*>(this->vertices);
	tmp.materialIndices = this->materialIndices;
	tmp.numVertices = this->numVertices;
	tmp.indices = this->indices;
	tmp.numIndices = this->numIndices;
	return tmp;
}

} // namespace ph

#endif
