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

#include "sfz/resources/VoxelResources.hpp"

#include "sfz/PushWarnings.hpp"
#define OGT_VOX_IMPLEMENTATION
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include "ogt_vox.h"
#include "ogt_voxel_meshify.h"
#include "sfz/PopWarnings.hpp"

#include "sfz/Logging.hpp"
#include "sfz/util/IO.hpp"

namespace sfz {

// Statics
// ------------------------------------------------------------------------------------------------

static Allocator* voxAllocator = nullptr;

static void* ogtVoxMallocWrapper(size_t size)
{
	return voxAllocator->allocate(sfz_dbg("opengametools"), uint64_t(size));
}

static void ogtVoxFreeWrapper(void* mem)
{
	voxAllocator->deallocate(mem);
}

// VoxelModel
// ------------------------------------------------------------------------------------------------

bool VoxelModelResource::build(Allocator* allocator)
{
	// Load file
	Array<uint8_t> file = readBinaryFile(this->path, allocator);
	if (file.size() == 0) {
		SFZ_ERROR("VoxelModelResource", "Failed to load file: \"%s\"", path.str());
		return false;
	}

	// Parse file
	const uint32_t readFlags = 0; // k_read_scene_flags_groups
	const ogt_vox_scene* scene = ogt_vox_read_scene_with_flags(file.data(), file.size(), readFlags);
	if (scene == nullptr) {
		SFZ_ERROR("VoxelModelResource", "Failed to parse file: \"%s\"", path.str());
		return false;
	}

	// Store last modified date
	this->lastModifiedDate = fileLastModifiedDate(path);

	// Some assumptions
	sfz_assert(scene->num_models == 1);
	sfz_assert(scene->num_instances == 1);
	//sfz_assert(scene->num_layers == 1);
	sfz_assert(scene->num_groups == 1);
	const ogt_vox_model& model = *scene->models[0];

	// Copy voxels to voxel model
	this->dims = vec3_u32(model.size_x, model.size_y, model.size_z);
	const uint32_t maxNumVoxels = model.size_x * model.size_y * model.size_z;
	this->voxels.init(maxNumVoxels, allocator, sfz_dbg(""));
	this->voxels.add(model.voxel_data, maxNumVoxels);

	// Find highest voxel value and number of non-empty voxels
	uint32_t highestVoxelVal = 0;
	this->numVoxels = 0;
	bool materialUsed[256] = {};
	for (uint8_t voxel : this->voxels) {
		materialUsed[voxel] = true;
		highestVoxelVal = sfz::max(highestVoxelVal, uint32_t(voxel));
		if (voxel != uint8_t(0)) this->numVoxels += 1;
	}

	// Copy palette to voxel model, remove materials which are not used by voxel model
	this->palette.clear();
	for (uint32_t i = 0; i <= highestVoxelVal; i++) {
		vec4_u8& dst = this->palette.add();
		if (materialUsed[i]) {
			ogt_vox_rgba color = scene->palette.color[i];
			dst.x = color.r;
			dst.y = color.g;
			dst.z = color.b;
			dst.w = color.a;
		}
		else {
			dst.x = 75;
			dst.y = 75;
			dst.z = 75;
			dst.w = 255; // Unsure about this one
		}
	}

	return true;
}

VoxelModelResource VoxelModelResource::load(const char* path, Allocator* allocator)
{
	VoxelModelResource resource;
	resource.name = strID(path);
	resource.path = path;

	bool success = resource.build(allocator);
	(void)success; // We do not need to look at error code here, errors are logged inside build().
	return resource;
}

// OpenGameTools allocator
// ------------------------------------------------------------------------------------------------

void setOpenGameToolsAllocator(Allocator* allocator)
{
	sfz_assert(voxAllocator == nullptr);
	voxAllocator = allocator;
	ogt_vox_set_memory_allocator(ogtVoxMallocWrapper, ogtVoxFreeWrapper);
}

} // namespace sfz
