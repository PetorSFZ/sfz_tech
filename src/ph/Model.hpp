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
#include <sfz/memory/Allocator.hpp>

#include <ph/rendering/MeshView.h>

namespace ph {

using sfz::Allocator;
using sfz::DynArray;
using std::uint32_t;

// ModelComponent class
// ------------------------------------------------------------------------------------------------

class ModelComponent final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ModelComponent() noexcept = default;
	ModelComponent(const ModelComponent&) = delete;
	ModelComponent& operator= (const ModelComponent&) = delete;

	ModelComponent(const uint32_t* indices, uint32_t numIndices, uint32_t materialIndex) noexcept
	{
		this->create(indices, numIndices, materialIndex);
	}
	ModelComponent(ModelComponent&& other) noexcept { this->swap(other); }
	ModelComponent& operator= (ModelComponent&& other) noexcept { this->swap(other); return *this; }
	~ModelComponent() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void create(const uint32_t* indices, uint32_t numIndices, uint32_t materialIndex) noexcept;
	void swap(ModelComponent& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	uint32_t materialIndex() const noexcept { return mMaterialIndex; }

	void render() noexcept;

private:
	uint32_t mIndexBuffer = 0;
	uint32_t mNumIndices = 0;
	uint32_t mMaterialIndex = 0;
};

// Model class
// ------------------------------------------------------------------------------------------------

class Model final {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Model() noexcept = default;
	Model(const Model&) = delete;
	Model& operator= (const Model&) = delete;

	Model(const phConstMeshView& mesh, Allocator* allocator) noexcept { this->create(mesh, allocator); }
	Model(Model&& other) noexcept { this->swap(other); }
	Model& operator= (Model&& other) noexcept { this->swap(other); return *this; }
	~Model() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void create(const phConstMeshView& mesh, Allocator* allocator) noexcept;
	void swap(Model& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	bool isValid() const noexcept { return mVAO != 0; }

	void bindVAO() noexcept;

	DynArray<ModelComponent>& components() noexcept { return mComponents; }
	const DynArray<ModelComponent>& components() const noexcept { return mComponents; }

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	uint32_t mVAO = 0;
	uint32_t mVertexBuffer = 0;
	DynArray<ModelComponent> mComponents;
};

} // namespace ph
