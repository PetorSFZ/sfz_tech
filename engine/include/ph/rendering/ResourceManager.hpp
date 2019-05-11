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

#include <sfz/containers/DynArray.hpp>
#include <sfz/containers/HashMap.hpp>
#include <sfz/strings/StringID.hpp>

#include "ph/rendering/Mesh.hpp"
#include "ph/rendering/Renderer.hpp"

namespace ph {

using sfz::StringID;

// Helper structs
// ------------------------------------------------------------------------------------------------

struct ResourceMapping final {
	StringID globalPathId;
	uint32_t globalIdx = uint32_t(~0);

	static ResourceMapping create(StringID globalPathId, uint32_t globalIdx) noexcept
	{
		ResourceMapping mapping;
		mapping.globalPathId = globalPathId;
		mapping.globalIdx = globalIdx;
		return mapping;
	}
};

// ResourceManager class
// ------------------------------------------------------------------------------------------------

// Class responsible for keeping track of resources in a Renderer.
//
// If a ResourceManager is used all resources should be sent to the renderer through it, otherwise
// weird stuff might happen.
class ResourceManager final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ResourceManager() noexcept = default;
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator= (const ResourceManager&) = delete;
	ResourceManager(ResourceManager&& o) noexcept { this->swap(o); }
	ResourceManager& operator= (ResourceManager&& o) noexcept { this->swap(o); return *this; }
	~ResourceManager() noexcept { this->destroy(); }

	// Creates a ResourceManager and makes it track the given renderer
	static ResourceManager create(Renderer* renderer, Allocator* allocator) noexcept;

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(ResourceManager& other) noexcept;
	void destroy() noexcept;

	// Texture methods
	// --------------------------------------------------------------------------------------------

	// Registers a texture and returns its texture ID in the renderer
	//
	// If the texture is not in the renderer it is loaded from file and then uploaded. The ID is
	// then recorded by this ResourceManager and return by this function.
	//
	// If the texture is already available in the renderer its ID is returned.
	//
	// The parameter should be the "global path" to the texture. This is a path relative to the
	// game executable. I.e. normally on the form "res/path/to/texture.jpg" if the texture is in
	// the "res" directory in the same directory as the executable.
	//
	// Returns ~0 (UINT32_MAX) if texture is not available in renderer and can't be loaded
	uint32_t registerTexture(const char* globalPath) noexcept;

	// Checks if a given texture is available in the renderer or not without modifying global
	// any state.
	bool hasTexture(StringID globalPathId) const noexcept;

	const sfz::DynArray<ResourceMapping>& textures() const noexcept { return mTextures; }

	// Debug function that returns a string containing the global path for a specific index
	const char* debugTextureIndexToGlobalPath(uint32_t index) const noexcept;

	// Mesh methods
	// --------------------------------------------------------------------------------------------

	uint32_t registerMesh(const char* globalPath, const Mesh& mesh) noexcept;

	bool hasMesh(StringID globalPathId) const noexcept;

	bool hasMeshDependencies(StringID globalPathId) const noexcept;

	const sfz::DynArray<ResourceMapping>& meshes() const noexcept { return mMeshes; }

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	Allocator* mAllocator = nullptr;
	Renderer* mRenderer = nullptr;

	sfz::DynArray<ResourceMapping> mTextures;
	sfz::HashMap<StringID, uint32_t> mTextureMap;

	sfz::DynArray<ResourceMapping> mMeshes;
	sfz::HashMap<StringID, uint32_t> mMeshMap;
};


} // namespace ph
