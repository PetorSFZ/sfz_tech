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

struct SfzBufferResource;
struct SfzFramebufferResource;
struct SfzTextureResource;

// SfzResourceManager
// ------------------------------------------------------------------------------------------------

struct SfzResourceManagerState;

struct SfzResourceManager final {
public:
	SFZ_DECLARE_DROP_TYPE(SfzResourceManager);
	void init(u32 maxNumResources, SfzAllocator* allocator, ZgUploader* uploader) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void renderDebugUI(SfzStrIDs* ids);

	// Updates all resources that depend on screen resolution
	void updateResolution(i32x2 screenRes, SfzStrIDs* ids);

	ZgUploader* getUploader();

	// Buffer methods
	// --------------------------------------------------------------------------------------------

	SfzHandle getBufferHandle(SfzStrIDs* ids, const char* name) const;
	SfzHandle getBufferHandle(SfzStrID name) const;
	SfzBufferResource* getBuffer(SfzHandle handle);
	SfzHandle addBuffer(SfzBufferResource&& resource, bool allowReplace = false);
	void removeBuffer(SfzStrID name);

	// Texture methods
	// --------------------------------------------------------------------------------------------

	SfzHandle getTextureHandle(SfzStrIDs* ids, const char* name) const;
	SfzHandle getTextureHandle(SfzStrID name) const;
	SfzTextureResource* getTexture(SfzHandle handle);
	SfzHandle addTexture(SfzTextureResource&& resource);
	void removeTexture(SfzStrID name);

	// Framebuffer methods
	// --------------------------------------------------------------------------------------------

	SfzHandle getFramebufferHandle(SfzStrIDs* ids, const char* name) const;
	SfzHandle getFramebufferHandle(SfzStrID name) const;
	SfzFramebufferResource* getFramebuffer(SfzHandle handle);
	SfzHandle addFramebuffer(SfzFramebufferResource&& resource);
	void removeFramebuffer(SfzStrID name);

private:
	SfzResourceManagerState* mState = nullptr;
};
