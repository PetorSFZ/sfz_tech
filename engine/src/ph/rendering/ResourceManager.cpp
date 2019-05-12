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

#include "ph/rendering/ResourceManager.hpp"

#include <algorithm>

#include <sfz/Logging.hpp>

#include <ph/Context.hpp>

#include "ph/rendering/Image.hpp"

namespace ph {

// Statics
// ------------------------------------------------------------------------------------------------

static phConstMeshComponentView toMeshComponentView(const MeshComponent& component) noexcept
{
	phConstMeshComponentView view;
	view.indices = component.indices.data();
	view.numIndices = component.indices.size();
	view.materialIdx = component.materialIdx;
	return view;
}

struct MeshViewContainer final {
	DynArray<phConstMeshComponentView> componentViews;
	phConstMeshView view;
};

static MeshViewContainer toMeshView(
	const Mesh& mesh,
	const sfz::DynArray<phMaterial>& boundMaterials,
	sfz::Allocator* allocator) noexcept
{
	MeshViewContainer viewCon;

	// Create mesh component views
	viewCon.componentViews.create(mesh.components.size(), allocator);
	for (const MeshComponent& component : mesh.components) {
		viewCon.componentViews.add(toMeshComponentView(component));
	}

	// Fill in rest of mesh view
	viewCon.view.vertices = mesh.vertices.data();
	viewCon.view.numVertices = mesh.vertices.size();
	viewCon.view.components = viewCon.componentViews.data();
	viewCon.view.materials = boundMaterials.data();
	viewCon.view.numComponents = mesh.components.size();
	viewCon.view.numMaterials = boundMaterials.size();

	return viewCon;
}

// ResourceManager: Constructors & destructors
// ------------------------------------------------------------------------------------------------

ResourceManager ResourceManager::create(Renderer* renderer, Allocator* allocator) noexcept
{
	ResourceManager manager;
	manager.mAllocator = allocator;
	manager.mRenderer = renderer;

	// Ensure renderer has not associated textures already, will break things
	sfz_assert_debug(renderer->numTextures() == 0);

	return manager;
}

// ResourceManager: State methods
// ------------------------------------------------------------------------------------------------

void ResourceManager::swap(ResourceManager& other) noexcept
{
	std::swap(this->mAllocator, other.mAllocator);
	std::swap(this->mRenderer, other.mRenderer);
	this->mTextureMap.swap(other.mTextureMap);
}

void ResourceManager::destroy() noexcept
{
	mAllocator = nullptr;
	mRenderer = nullptr;
	mTextureMap.destroy();
}

// ResourceManager: Texture methods
// ------------------------------------------------------------------------------------------------

uint32_t ResourceManager::registerTexture(const char* globalPath) noexcept
{
	// Convert global path to StringID
	StringCollection& resourceStrings = getResourceStrings();
	const StringID globalPathId = resourceStrings.getStringID(globalPath);

	// Check if texture is available in renderer, return index if it is
	uint32_t* globalIdxPtr = mTextureMap.get(globalPathId);
	if (globalIdxPtr != nullptr) return *globalIdxPtr;

	// Create image from path
	ph::Image image = loadImage("", globalPath);
	if (image.rawData.data() == nullptr) {
		SFZ_ERROR("ResourceManager", "Could not load texture: \"%s\"", globalPath);
		return uint16_t(~0);
	}

	// Upload image to renderer
	phConstImageView imageView = image;
	uint16_t globalIdx = mRenderer->addTexture(imageView);

	// Record entry
	mTextures.add(ResourceMapping::create(globalPathId, globalIdx));
	mTextureMap.put(globalPathId, globalIdx);

	SFZ_INFO_NOISY("ResourceManager", "Loaded texture: \"%s\", global index -> %u",
		globalPath, globalIdx);

	return globalIdx;
}

uint32_t ResourceManager::registerTexture(const char* globalPath, const Image& texture) noexcept
{
	// Convert global path to StringID
	StringCollection& resourceStrings = getResourceStrings();
	const StringID globalPathId = resourceStrings.getStringID(globalPath);

	// Check if texture is available in renderer, return index if it is
	uint32_t* globalIdxPtr = mTextureMap.get(globalPathId);
	if (globalIdxPtr != nullptr) return *globalIdxPtr;

	// Check if image is empty
	if (texture.rawData.data() == nullptr) {
		SFZ_ERROR("ResourceManager", "Image is empty: \"%s\"", globalPath);
		return uint16_t(~0);
	}

	// Upload image to renderer
	phConstImageView imageView = texture;
	uint16_t globalIdx = mRenderer->addTexture(imageView);

	// Record entry
	mTextures.add(ResourceMapping::create(globalPathId, globalIdx));
	mTextureMap.put(globalPathId, globalIdx);

	SFZ_INFO_NOISY("ResourceManager", "Loaded texture: \"%s\", global index -> %u",
		globalPath, globalIdx);

	return globalIdx;
}

uint32_t ResourceManager::getTextureIndex(StringID globalPathId) const noexcept
{
	const uint32_t* idxPtr = mTextureMap.get(globalPathId);
	if (idxPtr == nullptr) return ~0u;
	return *idxPtr;
}

bool ResourceManager::hasTexture(StringID globalPathId) const noexcept
{
	return mTextureMap.get(globalPathId) != nullptr;
}

const char* ResourceManager::debugTextureIndexToGlobalPath(uint32_t index) const noexcept
{
	const StringCollection& resourceStrings = getResourceStrings();
	for (const ResourceMapping& mapping : mTextures) {
		if (mapping.globalIdx == index) return resourceStrings.getString(mapping.globalPathId);
	}
	return "NO TEXTURE";
}

// ResourceManager: Mesh methods
// ------------------------------------------------------------------------------------------------

uint32_t ResourceManager::registerMesh(
	const char* globalPath, const Mesh& mesh, const DynArray<ImageAndPath>& textures) noexcept
{
	// Convert global path to StringID
	StringCollection& resourceStrings = getResourceStrings();
	const StringID globalPathId = resourceStrings.getStringID(globalPath);

	// Check if mesh is available in renderer, return index if it is
	uint32_t* globalIdxPtr = mMeshMap.get(globalPathId);
	if (globalIdxPtr != nullptr) return *globalIdxPtr;

	// Upload textures to renderer
	for (const ImageAndPath& texture : textures) {
		this->registerTexture(resourceStrings.getString(texture.globalPathId), texture.image);
	}

	// Bind materials
	sfz::DynArray<phMaterial> boundMaterials(mesh.materials.size(), mAllocator);
	for (const MaterialUnbound& unbound : mesh.materials) {
		phMaterial bound;
		bound.albedo = unbound.albedo;
		bound.emissive = unbound.emissive;
		bound.roughness = unbound.roughness;
		bound.metallic = unbound.metallic;

		auto getIdx = [&](StringID globalPathId) {
			
			if (globalPathId == StringID::invalid()) return uint16_t(~0);
			
			uint32_t texIndex = this->getTextureIndex(globalPathId);
			if (texIndex == ~0u) {
				const char* texPath = resourceStrings.getString(globalPathId);
				SFZ_ERROR("ResourceManager",
					"Attempted to bind texture \"%s\", but it was not available in Renderer",
					texPath);
				return uint16_t(~0);
			}

			return uint16_t(texIndex);
		};

		bound.albedoTexIndex = getIdx(unbound.albedoTex);
		bound.metallicRoughnessTexIndex = getIdx(unbound.metallicRoughnessTex);
		bound.normalTexIndex = getIdx(unbound.normalTex);
		bound.occlusionTexIndex = getIdx(unbound.occlusionTex);
		bound.emissiveTexIndex = getIdx(unbound.emissiveTex);

		boundMaterials.add(bound);
	}

	// Upload mesh to renderer
	MeshViewContainer meshViewContainer = toMeshView(mesh, boundMaterials, mAllocator);
	uint32_t globalIdx = mRenderer->addMesh(meshViewContainer.view);

	// Record entry
	MeshDescriptor descriptor;
	descriptor.globalPathId = globalPathId;
	descriptor.globalIdx = globalIdx;
	descriptor.componentDescriptors.create(mesh.components.size(), mAllocator);
	for (const MeshComponent& comp : mesh.components) {
		descriptor.componentDescriptors.add({ comp.materialIdx });
	}
	descriptor.materials = std::move(boundMaterials);
	mMeshDescriptors.add(std::move(descriptor));
	mMeshMap.put(globalPathId, globalIdx);

	SFZ_INFO_NOISY("ResourceManager", "Loaded mesh: \"%s\", global index -> %u",
		globalPath, globalIdx);

	return globalIdx;
}

bool ResourceManager::hasMesh(StringID globalPathId) const noexcept
{
	return mMeshMap.get(globalPathId) != nullptr;
}

} // namespace ph
