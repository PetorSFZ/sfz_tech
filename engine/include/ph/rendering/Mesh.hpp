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

#include <sfz/containers/DynArray.hpp>

#include <ph/rendering/MeshView.hpp>

namespace ph {

using sfz::DynArray;

// Mesh component
// ------------------------------------------------------------------------------------------------

struct MeshComponent final {
	DynArray<uint32_t> indices;
	uint32_t materialIdx = ~0u;

	phConstMeshComponentView toMeshComponentView() const noexcept;
};

inline phConstMeshComponentView MeshComponent::toMeshComponentView() const noexcept
{
	phConstMeshComponentView view;
	view.indices = this->indices.data();
	view.numIndices = this->indices.size();
	view.materialIdx = this->materialIdx;
	return view;
}

// Mesh
// ------------------------------------------------------------------------------------------------

struct MeshViewContainer final {
	DynArray<phConstMeshComponentView> componentViews;
	phConstMeshView view;
};

struct Mesh final {
	DynArray<phVertex> vertices;
	DynArray<MeshComponent> components;
	DynArray<phMaterial> materials;

	MeshViewContainer toMeshView(sfz::Allocator* allocator) const noexcept;
};

inline MeshViewContainer Mesh::toMeshView(sfz::Allocator* allocator) const noexcept
{
	MeshViewContainer viewCon;

	// Create mesh component views
	viewCon.componentViews.create(this->components.size(), allocator);
	for (const MeshComponent& component : components) {
		viewCon.componentViews.add(component.toMeshComponentView());
	}

	// Fill in rest of mesh view
	viewCon.view.vertices = this->vertices.data();
	viewCon.view.numVertices = this->vertices.size();
	viewCon.view.components = viewCon.componentViews.data();
	viewCon.view.materials = this->materials.data();
	viewCon.view.numComponents = this->components.size();
	viewCon.view.numMaterials = this->materials.size();

	return viewCon;
}

} // namespace ph
