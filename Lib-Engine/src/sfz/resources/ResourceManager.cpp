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
#include "sfz/resources/VoxelResources.hpp"
#include "sfz/util/IO.hpp"

namespace sfz {

// ResourceManager: State methods
// ------------------------------------------------------------------------------------------------

void ResourceManager::init(u32 maxNumResources, SfzAllocator* allocator) noexcept
{
	sfz_assert(mState == nullptr);
	mState = sfz_new<ResourceManagerState>(allocator, sfz_dbg(""));
	mState->allocator = allocator;

	mState->bufferHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->buffers.init(maxNumResources, allocator, sfz_dbg(""));

	mState->textureHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->textures.init(maxNumResources, allocator, sfz_dbg(""));

	mState->framebufferHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->framebuffers.init(maxNumResources, allocator, sfz_dbg(""));

	mState->meshHandles.init(maxNumResources, allocator, sfz_dbg(""));
	mState->meshes.init(maxNumResources, allocator, sfz_dbg(""));

	GlobalConfig& cfg = getGlobalConfig();
	mState->voxelModelFileWatch = cfg.sanitizeBool("Resources", "voxelModelFileWatch", true, false);
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
	
	SfzAllocator* allocator = mState->allocator;
	sfz_delete(allocator, mState);
	mState = nullptr;
}

// ResourceManager: Methods
// ------------------------------------------------------------------------------------------------

void ResourceManager::renderDebugUI()
{
	resourceManagerUI(*this, *mState);
}

void ResourceManager::updateResolution(i32x2 screenRes)
{
	// Check if any textures need rebuilding
	bool anyTexNeedRebuild = false;
	for (HashMapPair<strID, SfzHandle> itemItr : mState->textureHandles) {
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
		for (HashMapPair<strID, SfzHandle> itemItr : mState->textureHandles) {
			TextureResource& resource = mState->textures[itemItr.value];
			if (resource.screenRelativeResolution || resource.settingControlledRes) {
				CHECK_ZG resource.build(screenRes);
			}
		}

		// Rebuild framebuffers
		for (HashMapPair<strID, SfzHandle> itemItr : mState->framebufferHandles) {
			FramebufferResource& resource = mState->framebuffers[itemItr.value];
			if (resource.screenRelativeResolution || resource.controlledResSetting) {
				CHECK_ZG resource.build(screenRes);
			}
		}
	}
}

bool ResourceManager::updateVoxelModels()
{
	bool updated = false;
	if (mState->voxelModelFileWatch->boolValue()) {
		for (HashMapPair<strID, SfzHandle> itemItr : mState->voxelModelHandles) {
			VoxelModelResource& resource = mState->voxelModels[itemItr.value];
			const time_t newLastModifiedDate = sfz::fileLastModifiedDate(resource.path.str());
			if (resource.lastModifiedDate < newLastModifiedDate) {
				resource.build(mState->allocator);
				updated = true;
			}
		}
	}
	return updated;
}

// ResourceManager: Buffer methods
// ------------------------------------------------------------------------------------------------

SfzHandle ResourceManager::getBufferHandle(const char* name) const
{
	return this->getBufferHandle(strID(name));
}

SfzHandle ResourceManager::getBufferHandle(strID name) const
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
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->bufferHandles.get(name) == nullptr);
	SfzHandle handle = mState->buffers.allocate(sfz_move(resource));
	mState->bufferHandles.put(name, handle);
	sfz_assert(mState->bufferHandles.size() == mState->buffers.numAllocated());
	return handle;
}

void ResourceManager::removeBuffer(strID name)
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
	return this->getTextureHandle(strID(name));
}

SfzHandle ResourceManager::getTextureHandle(strID name) const
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
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->textureHandles.get(name) == nullptr);
	SfzHandle handle = mState->textures.allocate(sfz_move(resource));
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

	SfzHandle handle = this->getTextureHandle(name);
	if (handle == SFZ_NULL_HANDLE) return;
	mState->textureHandles.remove(name);
	mState->textures.deallocate(handle);
}

// ResourceManager: Framebuffer methods
// ------------------------------------------------------------------------------------------------

SfzHandle ResourceManager::getFramebufferHandle(const char* name) const
{
	return this->getFramebufferHandle(strID(name));
}

SfzHandle ResourceManager::getFramebufferHandle(strID name) const
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
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->framebufferHandles.get(name) == nullptr);
	SfzHandle handle = mState->framebuffers.allocate(sfz_move(resource));
	mState->framebufferHandles.put(name, handle);
	sfz_assert(mState->framebufferHandles.size() == mState->framebuffers.numAllocated());
	return handle;
}

void ResourceManager::removeFramebuffer(strID name)
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
	return this->getMeshHandle(strID(name));
}

SfzHandle ResourceManager::getMeshHandle(strID name) const
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
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->meshHandles.get(name) == nullptr);
	SfzHandle handle = mState->meshes.allocate(sfz_move(resource));
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

	SfzHandle handle = this->getMeshHandle(name);
	if (handle == SFZ_NULL_HANDLE) return;
	mState->meshHandles.remove(name);
	mState->meshes.deallocate(handle);
}

// ResourceManager: VoxelModel methods
// ------------------------------------------------------------------------------------------------

SfzHandle ResourceManager::getVoxelModelHandle(const char* name) const
{
	return this->getVoxelModelHandle(strID(name));
}

SfzHandle ResourceManager::getVoxelModelHandle(strID name) const
{
	const SfzHandle* handle = mState->voxelModelHandles.get(name);
	if (handle == nullptr) return SFZ_NULL_HANDLE;
	return *handle;
}

VoxelModelResource* ResourceManager::getVoxelModel(SfzHandle handle)
{
	return mState->voxelModels.get(handle);
}

SfzHandle ResourceManager::addVoxelModel(VoxelModelResource&& resource)
{
	strID name = resource.name;
	sfz_assert(name.isValid());
	sfz_assert(mState->voxelModelHandles.get(name) == nullptr);
	SfzHandle handle = mState->voxelModels.allocate(sfz_move(resource));
	mState->voxelModelHandles.put(name, handle);
	sfz_assert(mState->voxelModelHandles.size() == mState->voxelModels.numAllocated());
	return handle;
}

void ResourceManager::removeVoxelModel(strID name)
{
	SfzHandle handle = this->getVoxelModelHandle(name);
	if (handle == SFZ_NULL_HANDLE) return;
	mState->voxelModelHandles.remove(name);
	mState->voxelModels.deallocate(handle);
}

// ResourceManager: VoxelMaterial methods
// ------------------------------------------------------------------------------------------------

SfzHandle ResourceManager::getVoxelMaterialHandle(const char* name) const
{
	return this->getVoxelMaterialHandle(strID(name));
}

SfzHandle ResourceManager::getVoxelMaterialHandle(strID name) const
{
	const SfzHandle* handle = mState->voxelMaterialHandles.get(name);
	if (handle == nullptr) return SFZ_NULL_HANDLE;
	return *handle;
}

SfzHandle ResourceManager::getVoxelMaterialHandle(u8x4 color) const
{
	const SfzHandle* handle = mState->voxelMaterialColors.get(color);
	if (handle == nullptr) return SFZ_NULL_HANDLE;
	return *handle;
}

VoxelMaterial* ResourceManager::getVoxelMaterial(SfzHandle handle)
{
	return mState->voxelMaterials.get(handle);
}

SfzHandle ResourceManager::addVoxelMaterial(VoxelMaterial&& resource)
{
	strID name = resource.name;
	u8x4 originalColor = resource.originalColor;
	sfz_assert(name.isValid());
	sfz_assert(mState->voxelMaterialHandles.get(name) == nullptr);
	sfz_assert(mState->voxelMaterialColors.get(originalColor) == nullptr);
	SfzHandle handle = mState->voxelMaterials.allocate(sfz_move(resource));
	mState->voxelMaterialHandles.put(name, handle);
	mState->voxelMaterialColors.put(originalColor, handle);
	sfz_assert(mState->voxelMaterialHandles.size() == mState->voxelMaterials.numAllocated());
	sfz_assert(mState->voxelMaterialColors.size() == mState->voxelMaterials.numAllocated());
	return handle;
}

void ResourceManager::removeVoxelMaterial(strID name)
{
	SfzHandle handle = this->getVoxelMaterialHandle(name);
	if (handle == SFZ_NULL_HANDLE) return;
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
	for (u32 i = 0; i < pool.arraySize(); i++) {
		const VoxelMaterial& src = pool.data()[i];
		ShaderVoxelMaterial& dst = cpu[i];
		dst.albedo = src.albedo;
		dst.roughness = src.roughness;
		dst.metallic = src.metallic;

		const f32x3 emissiveColorLinear = f32x3(
			powf(src.emissiveColor.x, 2.2f),
			powf(src.emissiveColor.y, 2.2f),
			powf(src.emissiveColor.z, 2.2f));
		dst.emissive = emissiveColorLinear * src.emissiveStrength;
	}

	// Note: We are doing this using the present queue because the copy queue can't change the
	//       resource state of the buffer. Plus, the buffer may be in use on the present queue.
	buffer->uploadBlocking<ShaderVoxelMaterial>(cpu.data(), pool.arraySize(), presentQueue);
}

SfzHandle ResourceManager::getVoxelMaterialShaderBufferHandle() const
{
	return mState->voxelMaterialShaderBufferHandle;
}

} // namespace sfz
