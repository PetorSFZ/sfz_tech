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
#include "sfz/resources/ResourceManagerState.hpp"
#include "sfz/resources/ResourceManagerUI.hpp"

namespace sfz {

// ResourceManager: State methods
// ------------------------------------------------------------------------------------------------

void ResourceManager::init(uint32_t maxNumResources, Allocator* allocator)
{
	sfz_assert(mState == nullptr);
	mState = allocator->newObject<ResourceManagerState>(sfz_dbg(""));
	mState->allocator = allocator;

	mState->textureHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->textures.init(maxNumResources, allocator, sfz_dbg(""));
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

TextureItem* ResourceManager::getTexture(PoolHandle handle)
{
	return mState->textures.get(handle);
}

PoolHandle ResourceManager::addTexture(strID name, TextureItem&& item)
{
	sfz_assert(mState->textureHandles.get(name) == nullptr);
	PoolHandle handle = mState->textures.allocate(std::move(item));
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

} // namespace sfz
