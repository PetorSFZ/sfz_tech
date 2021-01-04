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

	// Sets allocator for opengametools
	// TODO: Might want to place somewhere else
	setOpenGameToolsAllocator(allocator);
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
	resourceManagerUI(*mState);
}

void ResourceManager::updateResolution(vec2_u32 screenRes)
{
	// Update screen relative textures
	bool texturesRebuilt = false;
	for (HashMapPair<strID, PoolHandle> itemItr : mState->textureHandles) {
		TextureResource& resource = mState->textures[itemItr.value];
		if (resource.screenRelativeResolution) {
			bool texRebuilt = false;
			CHECK_ZG resource.build(screenRes, &texRebuilt);
			texturesRebuilt = texturesRebuilt || texRebuilt;
		}
	}

	// Update screen relative framebuffers
	if (texturesRebuilt) {
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

} // namespace sfz
