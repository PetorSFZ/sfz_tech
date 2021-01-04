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

#pragma once

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_strings.hpp>

namespace sfz {

// VoxelMaterial
// ------------------------------------------------------------------------------------------------

struct VoxelMaterial final {
	strID name;

	vec4_u8 albedo = vec4_u8(uint8_t(0));
};

// VoxelModelResource
// ------------------------------------------------------------------------------------------------

// A simple dense voxel model.
//
// Stores 1 byte (uint8_t) per voxel. See accessVoxel() for an example of how to access a specific
// voxel. The value 0 is reserved for unused voxels. Other values are used to index into the
// color palette.
struct VoxelModelResource final {
	strID name;

	vec3_u32 dims = vec3_u32(0u);
	Array<uint8_t> voxels;
	Arr256<vec4_u8> palette;

	uint8_t& accessVoxel(vec3_u32 coord)
	{
		sfz_assert(coord.x < dims.x);
		sfz_assert(coord.y < dims.y);
		sfz_assert(coord.z < dims.z);
		uint32_t idx = coord.x + (coord.y * dims.x) + (coord.z * dims.x * dims.y);
		sfz_assert(idx < voxels.size());
		return voxels[idx];
	}

	uint8_t accessVoxel(vec3_u32 coord) const
	{
		return const_cast<VoxelModelResource*>(this)->accessVoxel(coord);
	}

	static VoxelModelResource load(const char* path, Allocator* allocator);
};

// OpenGameTools allocator
// ------------------------------------------------------------------------------------------------

void setOpenGameToolsAllocator(Allocator* allocator);

} // namespace sfz
