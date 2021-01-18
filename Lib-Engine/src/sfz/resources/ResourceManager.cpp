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

#include "sfz/resources/ResourceManager.hpp"

#include "sfz/Logging.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/resources/FramebufferResource.hpp"
#include "sfz/resources/ResourceManagerState.hpp"
#include "sfz/resources/ResourceManagerUI.hpp"
#include "sfz/resources/TextureResource.hpp"
#include "sfz/resources/VoxelResources.hpp"

namespace sfz {

// ResourceManager: State methods
// ------------------------------------------------------------------------------------------------

void ResourceManager::init(uint32_t maxNumResources, Allocator* allocator) noexcept
{
	sfz_assert(mState == nullptr);
	mState = allocator->newObject<ResourceManagerState>(sfz_dbg(""));
	mState->allocator = allocator;

	mState->bufferHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->buffers.init(maxNumResources, allocator, sfz_dbg(""));

	mState->textureHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->textures.init(maxNumResources, allocator, sfz_dbg(""));

	mState->framebufferHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->framebuffers.init(maxNumResources, allocator, sfz_dbg(""));

	mState->meshHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->meshes.init(maxNumResources, allocator, sfz_dbg(""));

	mState->voxelModelHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->voxelModels.init(maxNumResources, allocator, sfz_dbg(""));

	mState->voxelMaterialHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->voxelMaterialColors.init(maxNumResources, allocator, sfz_dbg(""));
	mState->voxelMaterials.init(maxNumResources, allocator, sfz_dbg(""));
	mState->voxelMaterialShaderBufferCpu.init(maxNumResources, allocator, sfz_dbg(""));

	// Sets allocator for opengametools
	// TODO: Might want to place somewhere else
	setOpenGameToolsAllocator(allocator);

	// Create voxel shader material buffer
	mState->voxelMaterialShaderBufferCpu.add(ShaderVoxelMaterial{}, maxNumResources);
	mState->voxelMaterialShaderBufferHandle = addBuffer(BufferResource::createStatic(
		"voxel_material_buffer", sizeof(ShaderVoxelMaterial), maxNumResources));
}

void ResourceManager::destroy() noexcept
{
	if (mState == nullptr) return;
	
	// Flush ZeroG queues to ensure no resources are still in-use
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();
	
	Allocator* allocator = mState->allocator;
	allocator->deleteObject(mState);
	mState = nullptr;
}

// ResourceManager: Methods
// ------------------------------------------------------------------------------------------------

void ResourceManager::renderDebugUI()
{
	resourceManagerUI(*this, *mState);
}

void ResourceManager::updateResolution(vec2_u32 screenRes)
{
	// Check if any textures need rebuilding
	bool anyTexNeedRebuild = false;
	for (HashMapPair<strID, PoolHandle> itemItr : mState->textureHandles) {
		const TextureResource& resource = mState->textures[itemItr.value];
		anyTexNeedRebuild = anyTexNeedRebuild || resource.needRebuild(screenRes);
	}

	// Update textures if they need rebuilding
	if (anyTexNeedRebuild) {
		SFZ_INFO("Resources", "Rebuilding textures, screenRes = %u x %u", screenRes.x, screenRes.y);

		// Flush present and copy queue to ensure the textures aren't in use
		zg::CommandQueue presentQueue = zg::CommandQueue::getPresentQueue();
		zg::CommandQueue copyQueue = zg::CommandQueue::getCopyQueue();
		CHECK_ZG presentQueue.flush();
		CHECK_ZG copyQueue.flush();

		// Rebuild textures
		for (HashMapPair<strID, PoolHandle> itemItr : mState->textureHandles) {
			TextureResource& resource = mState->textures[itemItr.value];
			if (resource.screenRelativeResolution) {
				CHECK_ZG resource.build(screenRes);
			}
		}

		// Rebuild framebuffers
		for (HashMapPair<strID, PoolHandle> itemItr : mState->framebufferHandles) {
			FramebufferResource& resource = mState->framebuffers[itemItr.value];
			if (resource.screenRelativeResolution) {
				CHECK_ZG resource.build(screenRes);
			}
		}
	}
}

// ResourceManager: Buffer methods
// ------------------------------------------------------------------------------------------------

PoolHandle ResourceManager::getBufferHandle(const char* name) const
{
	return this->getBufferHandle(strID(name));
}

PoolHandle ResourceManager::getBufferHandle(strID name) const
{
	const PoolHandle* handle = mState->bufferHandles.get(name);
	if (handle == nullptr) return NULL_HANDLE;
	return *handle;
}

BufferResource* ResourceManager::getBuffer(PoolHandle handle)
{
	return mState->buffers.get(handle);
}

PoolHandle ResourceManager::addBuffer(BufferResource&& resource)
{
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->bufferHandles.get(name) == nullptr);
	PoolHandle handle = mState->buffers.allocate(std::move(resource));
	mState->bufferHandles.put(name, handle);
	sfz_assert(mState->bufferHandles.size() == mState->buffers.numAllocated());
	return handle;
}

void ResourceManager::removeBuffer(strID name)
{
	// TODO: Currently blocking, can probably be made async.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	PoolHandle handle = this->getBufferHandle(name);
	if (handle == NULL_HANDLE) return;
	mState->bufferHandles.remove(name);
	mState->buffers.deallocate(handle);
}

// ResourceManager: Texture methods
// ------------------------------------------------------------------------------------------------

PoolHandle ResourceManager::getTextureHandle(const char* name) const
{
	return this->getTextureHandle(strID(name));
}

PoolHandle ResourceManager::getTextureHandle(strID name) const
{
	const PoolHandle* handle = mState->textureHandles.get(name);
	if (handle == nullptr) return NULL_HANDLE;
	return *handle;
}

TextureResource* ResourceManager::getTexture(PoolHandle handle)
{
	return mState->textures.get(handle);
}

PoolHandle ResourceManager::addTexture(TextureResource&& resource)
{
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->textureHandles.get(name) == nullptr);
	PoolHandle handle = mState->textures.allocate(std::move(resource));
	mState->textureHandles.put(name, handle);
	sfz_assert(mState->textureHandles.size() == mState->textures.numAllocated());
	return handle;
}

void ResourceManager::removeTexture(strID name)
{
	// TODO: Currently blocking, can probably be made async if we just add it to a list of textures
	//       to remove and then remove it in a frame or two.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	PoolHandle handle = this->getTextureHandle(name);
	if (handle == NULL_HANDLE) return;
	mState->textureHandles.remove(name);
	mState->textures.deallocate(handle);
}

// ResourceManager: Framebuffer methods
// ------------------------------------------------------------------------------------------------

PoolHandle ResourceManager::getFramebufferHandle(const char* name) const
{
	return this->getFramebufferHandle(strID(name));
}

PoolHandle ResourceManager::getFramebufferHandle(strID name) const
{
	const PoolHandle* handle = mState->framebufferHandles.get(name);
	if (handle == nullptr) return NULL_HANDLE;
	return *handle;
}

FramebufferResource* ResourceManager::getFramebuffer(PoolHandle handle)
{
	return mState->framebuffers.get(handle);
}

PoolHandle ResourceManager::addFramebuffer(FramebufferResource&& resource)
{
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->framebufferHandles.get(name) == nullptr);
	PoolHandle handle = mState->framebuffers.allocate(std::move(resource));
	mState->framebufferHandles.put(name, handle);
	sfz_assert(mState->framebufferHandles.size() == mState->framebuffers.numAllocated());
	return handle;
}

void ResourceManager::removeFramebuffer(strID name)
{
	// TODO: Currently blocking, can probably be made async.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	PoolHandle handle = this->getFramebufferHandle(name);
	if (handle == NULL_HANDLE) return;
	mState->framebufferHandles.remove(name);
	mState->framebuffers.deallocate(handle);
}

// ResourceManager: Mesh methods
// ------------------------------------------------------------------------------------------------

PoolHandle ResourceManager::getMeshHandle(const char* name) const
{
	return this->getMeshHandle(strID(name));
}

PoolHandle ResourceManager::getMeshHandle(strID name) const
{
	const PoolHandle* handle = mState->meshHandles.get(name);
	if (handle == nullptr) return NULL_HANDLE;
	return *handle;
}

MeshResource* ResourceManager::getMesh(PoolHandle handle)
{
	return mState->meshes.get(handle);
}

PoolHandle ResourceManager::addMesh(MeshResource&& resource)
{
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->meshHandles.get(name) == nullptr);
	PoolHandle handle = mState->meshes.allocate(std::move(resource));
	mState->meshHandles.put(name, handle);
	sfz_assert(mState->meshHandles.size() == mState->meshes.numAllocated());
	return handle;
}

void ResourceManager::removeMesh(strID name)
{
	// TODO: Currently blocking, can probably be made async if we just add it to a list of meshes
	//       to remove and then remove it in a frame or two.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	PoolHandle handle = this->getMeshHandle(name);
	if (handle == NULL_HANDLE) return;
	mState->meshHandles.remove(name);
	mState->meshes.deallocate(handle);
}

// ResourceManager: VoxelModel methods
// ------------------------------------------------------------------------------------------------

PoolHandle ResourceManager::getVoxelModelHandle(const char* name) const
{
	return this->getVoxelModelHandle(strID(name));
}

PoolHandle ResourceManager::getVoxelModelHandle(strID name) const
{
	const PoolHandle* handle = mState->voxelModelHandles.get(name);
	if (handle == nullptr) return NULL_HANDLE;
	return *handle;
}

VoxelModelResource* ResourceManager::getVoxelModel(PoolHandle handle)
{
	return mState->voxelModels.get(handle);
}

PoolHandle ResourceManager::addVoxelModel(VoxelModelResource&& resource)
{
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->voxelModelHandles.get(name) == nullptr);
	PoolHandle handle = mState->voxelModels.allocate(std::move(resource));
	mState->voxelModelHandles.put(name, handle);
	sfz_assert(mState->voxelModelHandles.size() == mState->voxelModels.numAllocated());
	return handle;
}

void ResourceManager::removeVoxelModel(strID name)
{
	PoolHandle handle = this->getVoxelModelHandle(name);
	if (handle == NULL_HANDLE) return;
	mState->voxelModelHandles.remove(name);
	mState->voxelModels.deallocate(handle);
}

// ResourceManager: VoxelMaterial methods
// ------------------------------------------------------------------------------------------------

PoolHandle ResourceManager::getVoxelMaterialHandle(const char* name) const
{
	return this->getVoxelMaterialHandle(strID(name));
}

PoolHandle ResourceManager::getVoxelMaterialHandle(strID name) const
{
	const PoolHandle* handle = mState->voxelMaterialHandles.get(name);
	if (handle == nullptr) return NULL_HANDLE;
	return *handle;
}

PoolHandle ResourceManager::getVoxelMaterialHandle(vec4_u8 color) const
{
	const PoolHandle* handle = mState->voxelMaterialColors.get(color);
	if (handle == nullptr) return NULL_HANDLE;
	return *handle;
}

VoxelMaterial* ResourceManager::getVoxelMaterial(PoolHandle handle)
{
	return mState->voxelMaterials.get(handle);
}

PoolHandle ResourceManager::addVoxelMaterial(VoxelMaterial&& resource)
{
	strID name = resource.name;
	vec4_u8 originalColor = resource.originalColor;
	sfz_assert(name.isValid());
	sfz_assert(mState->voxelMaterialHandles.get(name) == nullptr);
	sfz_assert(mState->voxelMaterialColors.get(originalColor) == nullptr);
	PoolHandle handle = mState->voxelMaterials.allocate(std::move(resource));
	mState->voxelMaterialHandles.put(name, handle);
	mState->voxelMaterialColors.put(originalColor, handle);
	sfz_assert(mState->voxelMaterialHandles.size() == mState->voxelMaterials.numAllocated());
	sfz_assert(mState->voxelMaterialColors.size() == mState->voxelMaterials.numAllocated());
	return handle;
}

void ResourceManager::removeVoxelMaterial(strID name)
{
	PoolHandle handle = this->getVoxelMaterialHandle(name);
	if (handle == NULL_HANDLE) return;
	VoxelMaterial material = mState->voxelMaterials[handle];
	sfz_assert(handle == this->getVoxelMaterialHandle(material.originalColor));
	mState->voxelMaterialHandles.remove(name);
	mState->voxelMaterialColors.remove(material.originalColor);
	mState->voxelMaterials.deallocate(handle);
}

void ResourceManager::syncVoxelMaterialsToGpuBlocking()
{
	// Flush present queue
	zg::CommandQueue presentQueue = zg::CommandQueue::getPresentQueue();
	CHECK_ZG presentQueue.flush();

	BufferResource* buffer = getBuffer(mState->voxelMaterialShaderBufferHandle);
	sfz_assert(buffer != nullptr);

	Array<ShaderVoxelMaterial>& cpu = mState->voxelMaterialShaderBufferCpu;
	Pool<VoxelMaterial>& pool = mState->voxelMaterials;
	sfz_assert(cpu.size() >= pool.arraySize());
	for (uint32_t i = 0; i < pool.arraySize(); i++) {
		const VoxelMaterial& src = pool.data()[i];
		ShaderVoxelMaterial& dst = cpu[i];
		dst.albedo = src.albedo;
		dst.roughness = src.roughness;
		dst.emissive = src.emissive;
		dst.metallic = src.metallic;
	}

	// Note: We are doing this using the present queue because the copy queue can't change the
	//       resource state of the buffer. Plus, the buffer may be in use on the present queue.
	buffer->uploadBlocking<ShaderVoxelMaterial>(cpu.data(), pool.arraySize(), presentQueue);
}

PoolHandle ResourceManager::getVoxelMaterialShaderBufferHandle() const
{
	return mState->voxelMaterialShaderBufferHandle;
}

} // namespace sfz
