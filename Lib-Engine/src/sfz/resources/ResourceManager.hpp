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

#pragma once

#include <skipifzero.hpp>
#include <skipifzero_pool.hpp>
#include <skipifzero_strings.hpp>

struct ZgUploader;

namespace sfz {

struct BufferResource;
struct FramebufferResource;
struct MeshResource;
struct TextureResource;

// ResourceManager
// ------------------------------------------------------------------------------------------------

struct ResourceManagerState;

class ResourceManager final {
public:
	SFZ_DECLARE_DROP_TYPE(ResourceManager);
	void init(u32 maxNumResources, SfzAllocator* allocator, ZgUploader* uploader) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void renderDebugUI();

	// Updates all resources that depend on screen resolution
	void updateResolution(i32x2 screenRes);

	ZgUploader* getUploader();

	// Buffer methods
	// --------------------------------------------------------------------------------------------

	SfzHandle getBufferHandle(const char* name) const;
	SfzHandle getBufferHandle(SfzStrID name) const;
	BufferResource* getBuffer(SfzHandle handle);
	SfzHandle addBuffer(BufferResource&& resource);
	void removeBuffer(SfzStrID name);

	// Texture methods
	// --------------------------------------------------------------------------------------------

	SfzHandle getTextureHandle(const char* name) const;
	SfzHandle getTextureHandle(SfzStrID name) const;
	TextureResource* getTexture(SfzHandle handle);
	SfzHandle addTexture(TextureResource&& resource);
	void removeTexture(SfzStrID name);

	// Framebuffer methods
	// --------------------------------------------------------------------------------------------

	SfzHandle getFramebufferHandle(const char* name) const;
	SfzHandle getFramebufferHandle(SfzStrID name) const;
	FramebufferResource* getFramebuffer(SfzHandle handle);
	SfzHandle addFramebuffer(FramebufferResource&& resource);
	void removeFramebuffer(SfzStrID name);

	// Mesh methods
	// --------------------------------------------------------------------------------------------

	SfzHandle getMeshHandle(const char* name) const;
	SfzHandle getMeshHandle(SfzStrID name) const;
	MeshResource* getMesh(SfzHandle handle);
	SfzHandle addMesh(MeshResource&& resource);
	void removeMesh(SfzStrID name);

private:
	ResourceManagerState* mState = nullptr;
};

} // namespace sfz
