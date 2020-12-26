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
#include <skipifzero_image_view.hpp>
#include <skipifzero_pool.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

#include "sfz/resources/MeshItem.hpp"
#include "sfz/resources/TextureItem.hpp"

namespace sfz {

// ResourceManager
// ------------------------------------------------------------------------------------------------

struct ResourceManagerState;

class ResourceManager final {
public:
	SFZ_DECLARE_DROP_TYPE(ResourceManager);
	void init(uint32_t maxNumResources, Allocator* allocator);
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void renderDebugUI();

	// Texture methods
	// --------------------------------------------------------------------------------------------

	PoolHandle getTextureHandle(const char* name) const;
	PoolHandle getTextureHandle(strID name) const;
	TextureItem* getTexture(PoolHandle handle);
	PoolHandle addTexture(strID name, TextureItem&& item);
	void removeTexture(strID name);

	// Mesh methods
	// --------------------------------------------------------------------------------------------

	PoolHandle getMeshHandle(const char* name) const;
	PoolHandle getMeshHandle(strID name) const;
	MeshItem* getMesh(PoolHandle handle);
	PoolHandle addMesh(strID name, MeshItem&& item);
	void removeMesh(strID name);

private:
	ResourceManagerState* mState = nullptr;
};

} // namespace sfz
