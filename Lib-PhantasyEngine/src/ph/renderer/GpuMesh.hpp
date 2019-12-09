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

#include <skipifzero_arrays.hpp>

#include <ZeroG-cpp.hpp>

#include "ph/renderer/BuiltInShaderTypes.hpp"
#include "ph/renderer/DynamicGpuAllocator.hpp"
#include "ph/rendering/Mesh.hpp"

namespace sfz {

using sfz::vec4;

// GpuMesh
// ------------------------------------------------------------------------------------------------

struct GpuMesh final {
	zg::Buffer vertexBuffer;
	zg::Buffer indexBuffer;
	zg::Buffer materialsBuffer;
	uint32_t numMaterials = 0;
	Array<MeshComponent> components;
	Array<Material> cpuMaterials;
};

// GpuMesh functions
// ------------------------------------------------------------------------------------------------

ShaderMaterial cpuMaterialToShaderMaterial(const Material& cpuMaterial) noexcept;

GpuMesh gpuMeshAllocate(
	const Mesh& cpuMesh,
	DynamicGpuAllocator& gpuAllocatorDevice,
	sfz::Allocator* cpuAllocator) noexcept;

void gpuMeshDeallocate(
	GpuMesh& gpuMesh,
	DynamicGpuAllocator& gpuAllocatorDevice) noexcept;

void gpuMeshUploadBlocking(
	GpuMesh& gpuMesh,
	const Mesh& cpuMesh,
	DynamicGpuAllocator& gpuAllocatorUpload,
	sfz::Allocator* cpuAllocator,
	zg::CommandQueue& copyQueue) noexcept;

} // namespace sfz
