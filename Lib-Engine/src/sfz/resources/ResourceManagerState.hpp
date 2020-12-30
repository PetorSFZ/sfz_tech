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
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_pool.hpp>
#include <skipifzero_strings.hpp>

#include "sfz/resources/FramebufferResource.hpp"
#include "sfz/resources/ResourceManager.hpp"
#include "sfz/resources/TextureResource.hpp"

namespace sfz {

// ResourceManagerState
// ------------------------------------------------------------------------------------------------

// This is the internal state of the Resource Manager. If you are just using the ResourceManager
// you probably shouldn't be accessing this directly.
struct ResourceManagerState final {
	Allocator* allocator = nullptr;

	HashMap<strID, PoolHandle> textureHandles;
	Pool<TextureResource> textures;

	HashMap<strID, PoolHandle> framebufferHandles;
	Pool<FramebufferResource> framebuffers;

	HashMap<strID, PoolHandle> meshHandles;
	Pool<MeshItem> meshes;
};

} // namespace sfz
