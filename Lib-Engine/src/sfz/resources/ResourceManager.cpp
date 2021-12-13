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

#include <skipifzero_new.hpp>

#include "sfz/Logging.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/resources/FramebufferResource.hpp"
#include "sfz/resources/ResourceManagerState.hpp"
#include "sfz/resources/ResourceManagerUI.hpp"
#include "sfz/resources/TextureResource.hpp"
#include "sfz/util/IO.hpp"

namespace sfz {

// ResourceManager: State methods
// ------------------------------------------------------------------------------------------------

void ResourceManager::init(u32 maxNumResources, SfzAllocator* allocator, ZgUploader* uploader) noexcept
{
	sfz_assert(mState == nullptr);
	mState = sfz_new<ResourceManagerState>(allocator, sfz_dbg(""));
	mState->allocator = allocator;
	mState->uploader = uploader;

	mState->bufferHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->buffers.init(maxNumResources, allocator, sfz_dbg(""));

	mState->textureHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->textures.init(maxNumResources, allocator, sfz_dbg(""));

	mState->framebufferHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->framebuffers.init(maxNumResources, allocator, sfz_dbg(""));

	mState->meshHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->meshes.init(maxNumResources, allocator, sfz_dbg(""));
}

void ResourceManager::destroy() noexcept
{
	if (mState == nullptr) return;
	
	// Flush ZeroG queues to ensure no resources are still in-use
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();
	
	SfzAllocator* allocator = mState->allocator;
	sfz_delete(allocator, mState);
	mState = nullptr;
}

// ResourceManager: Methods
// ------------------------------------------------------------------------------------------------

void ResourceManager::renderDebugUI()
{
	resourceManagerUI(*mState);
}

void ResourceManager::updateResolution(i32x2 screenRes)
{
	// Check if any textures need rebuilding
	bool anyTexNeedRebuild = false;
	for (HashMapPair<SfzStrID, SfzHandle> itemItr : mState->textureHandles) {
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
		for (HashMapPair<SfzStrID, SfzHandle> itemItr : mState->textureHandles) {
			TextureResource& resource = mState->textures[itemItr.value];
			if (resource.screenRelativeRes || resource.settingControlledRes) {
				CHECK_ZG resource.build(screenRes);
			}
		}

		// Rebuild framebuffers
		for (HashMapPair<SfzStrID, SfzHandle> itemItr : mState->framebufferHandles) {
			FramebufferResource& resource = mState->framebuffers[itemItr.value];
			if (resource.screenRelativeRes || resource.controlledResSetting) {
				CHECK_ZG resource.build(screenRes);
			}
		}
	}
}

ZgUploader* ResourceManager::getUploader()
{
	return mState->uploader;
}

// ResourceManager: Buffer methods
// ------------------------------------------------------------------------------------------------

SfzHandle ResourceManager::getBufferHandle(const char* name) const
{
	return this->getBufferHandle(sfzStrIDCreate(name));
}

SfzHandle ResourceManager::getBufferHandle(SfzStrID name) const
{
	const SfzHandle* handle = mState->bufferHandles.get(name);
	if (handle == nullptr) return SFZ_NULL_HANDLE;
	return *handle;
}

BufferResource* ResourceManager::getBuffer(SfzHandle handle)
{
	return mState->buffers.get(handle);
}

SfzHandle ResourceManager::addBuffer(BufferResource&& resource)
{
	SfzStrID name = resource.name;
	sfz_assert(name != SFZ_STR_ID_NULL);
	sfz_assert(mState->bufferHandles.get(name) == nullptr);
	SfzHandle handle = mState->buffers.allocate(sfz_move(resource));
	mState->bufferHandles.put(name, handle);
	sfz_assert(mState->bufferHandles.size() == mState->buffers.numAllocated());
	return handle;
}

void ResourceManager::removeBuffer(SfzStrID name)
{
	// TODO: Currently blocking, can probably be made async.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	SfzHandle handle = this->getBufferHandle(name);
	if (handle == SFZ_NULL_HANDLE) return;
	mState->bufferHandles.remove(name);
	mState->buffers.deallocate(handle);
}

// ResourceManager: Texture methods
// ------------------------------------------------------------------------------------------------

SfzHandle ResourceManager::getTextureHandle(const char* name) const
{
	return this->getTextureHandle(sfzStrIDCreate(name));
}

SfzHandle ResourceManager::getTextureHandle(SfzStrID name) const
{
	const SfzHandle* handle = mState->textureHandles.get(name);
	if (handle == nullptr) return SFZ_NULL_HANDLE;
	return *handle;
}

TextureResource* ResourceManager::getTexture(SfzHandle handle)
{
	return mState->textures.get(handle);
}

SfzHandle ResourceManager::addTexture(TextureResource&& resource)
{
	SfzStrID name = resource.name;
	sfz_assert(name != SFZ_STR_ID_NULL);
	sfz_assert(mState->textureHandles.get(name) == nullptr);
	SfzHandle handle = mState->textures.allocate(sfz_move(resource));
	mState->textureHandles.put(name, handle);
	sfz_assert(mState->textureHandles.size() == mState->textures.numAllocated());
	return handle;
}

void ResourceManager::removeTexture(SfzStrID name)
{
	// TODO: Currently blocking, can probably be made async if we just add it to a list of textures
	//       to remove and then remove it in a frame or two.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	SfzHandle handle = this->getTextureHandle(name);
	if (handle == SFZ_NULL_HANDLE) return;
	mState->textureHandles.remove(name);
	mState->textures.deallocate(handle);
}

// ResourceManager: Framebuffer methods
// ------------------------------------------------------------------------------------------------

SfzHandle ResourceManager::getFramebufferHandle(const char* name) const
{
	return this->getFramebufferHandle(sfzStrIDCreate(name));
}

SfzHandle ResourceManager::getFramebufferHandle(SfzStrID name) const
{
	const SfzHandle* handle = mState->framebufferHandles.get(name);
	if (handle == nullptr) return SFZ_NULL_HANDLE;
	return *handle;
}

FramebufferResource* ResourceManager::getFramebuffer(SfzHandle handle)
{
	return mState->framebuffers.get(handle);
}

SfzHandle ResourceManager::addFramebuffer(FramebufferResource&& resource)
{
	SfzStrID name = resource.name;
	sfz_assert(name != SFZ_STR_ID_NULL);
	sfz_assert(mState->framebufferHandles.get(name) == nullptr);
	SfzHandle handle = mState->framebuffers.allocate(sfz_move(resource));
	mState->framebufferHandles.put(name, handle);
	sfz_assert(mState->framebufferHandles.size() == mState->framebuffers.numAllocated());
	return handle;
}

void ResourceManager::removeFramebuffer(SfzStrID name)
{
	// TODO: Currently blocking, can probably be made async.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	SfzHandle handle = this->getFramebufferHandle(name);
	if (handle == SFZ_NULL_HANDLE) return;
	mState->framebufferHandles.remove(name);
	mState->framebuffers.deallocate(handle);
}

// ResourceManager: Mesh methods
// ------------------------------------------------------------------------------------------------

SfzHandle ResourceManager::getMeshHandle(const char* name) const
{
	return this->getMeshHandle(sfzStrIDCreate(name));
}

SfzHandle ResourceManager::getMeshHandle(SfzStrID name) const
{
	const SfzHandle* handle = mState->meshHandles.get(name);
	if (handle == nullptr) return SFZ_NULL_HANDLE;
	return *handle;
}

MeshResource* ResourceManager::getMesh(SfzHandle handle)
{
	return mState->meshes.get(handle);
}

SfzHandle ResourceManager::addMesh(MeshResource&& resource)
{
	SfzStrID name = resource.name;
	sfz_assert(name != SFZ_STR_ID_NULL);
	sfz_assert(mState->meshHandles.get(name) == nullptr);
	SfzHandle handle = mState->meshes.allocate(sfz_move(resource));
	mState->meshHandles.put(name, handle);
	sfz_assert(mState->meshHandles.size() == mState->meshes.numAllocated());
	return handle;
}

void ResourceManager::removeMesh(SfzStrID name)
{
	// TODO: Currently blocking, can probably be made async if we just add it to a list of meshes
	//       to remove and then remove it in a frame or two.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	SfzHandle handle = this->getMeshHandle(name);
	if (handle == SFZ_NULL_HANDLE) return;
	mState->meshHandles.remove(name);
	mState->meshes.deallocate(handle);
}

} // namespace sfz
