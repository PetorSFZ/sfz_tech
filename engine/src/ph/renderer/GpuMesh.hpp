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

#include <sfz/containers/DynArray.hpp>

#include <ZeroG-cpp.hpp>

#include "ph/renderer/BuiltInShaderTypes.hpp"
#include "ph/renderer/DynamicGpuAllocator.hpp"
#include "ph/rendering/Mesh.hpp"

namespace ph {

using sfz::vec4;

// GpuMaterial
// ------------------------------------------------------------------------------------------------

struct CpuMaterial final {
	uint32_t materialIdx = ~0u;
	StringID albedoTex = StringID::invalid();
	StringID metallicRoughnessTex = StringID::invalid();
	StringID normalTex = StringID::invalid();
	StringID occlusionTex = StringID::invalid();
	StringID emissiveTex = StringID::invalid();
};

// GpuMesh
// ------------------------------------------------------------------------------------------------

struct GpuMeshComponent final {
	// TODO: This is kinda wasteful. Would be better to store all indices in the same buffer.
	zg::Buffer indexBuffer;
	uint32_t numIndices = 0;
	CpuMaterial cpuMaterial;
};

struct GpuMesh final {
	zg::Buffer vertexBuffer;
	DynArray<GpuMeshComponent> components;
	zg::Buffer materialsBuffer;
	uint32_t numMaterials = 0;
};

// GpuMesh functions
// ------------------------------------------------------------------------------------------------

GpuMesh gpuMeshAllocate(
	const Mesh& cpuMesh,
	DynamicGpuAllocator& gpuAllocator,
	sfz::Allocator* cpuAllocator) noexcept;

void gpuMeshDeallocate(
	GpuMesh& gpuMesh,
	DynamicGpuAllocator& gpuAllocator) noexcept;

void gpuMeshUploadBlocking(
	GpuMesh& gpuMesh,
	const Mesh& cpuMesh,
	DynamicGpuAllocator& gpuAllocator,
	sfz::Allocator* cpuAllocator,
	zg::CommandQueue& copyQueue) noexcept;

} // namespace ph
