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

#include "ph/renderer/GpuMesh.hpp"

#include <sfz/Assert.hpp>
#include <sfz/strings/StackString.hpp>

#include "ph/renderer/ZeroGUtils.hpp"

namespace ph {

// GpuMesh functions
// ------------------------------------------------------------------------------------------------

GpuMesh gpuMeshAllocate(
	const Mesh& cpuMesh,
	DynamicGpuAllocator& gpuAllocator,
	sfz::Allocator* cpuAllocator) noexcept
{
	static uint32_t counter = 0;
	GpuMesh gpuMesh;

	// Allocate (GPU) memory for vertices
	gpuMesh.vertexBuffer = gpuAllocator.allocateBuffer(
		ZG_MEMORY_TYPE_DEVICE, cpuMesh.vertices.size() * sizeof(Vertex));
	sfz_assert_debug(gpuMesh.vertexBuffer.valid());
	CHECK_ZG gpuMesh.vertexBuffer.setDebugName(sfz::str128("Vertex_Buffer_%i", counter++));

	// Allocate (GPU) memory for indices
	gpuMesh.indexBuffer = gpuAllocator.allocateBuffer(
		ZG_MEMORY_TYPE_DEVICE, cpuMesh.indices.size() * sizeof(uint32_t));
	sfz_assert_debug(gpuMesh.indexBuffer.valid());
	CHECK_ZG gpuMesh.indexBuffer.setDebugName(sfz::str128("Index_Buffer_%i", counter++));

	// Allocate (CPU) memory for mesh component handles
	gpuMesh.components.create(cpuMesh.components.size(), cpuAllocator);
	gpuMesh.components.addMany(cpuMesh.components.size());

	// Allocate (GPU) memory for materials
	sfz_assert_debug(cpuMesh.materials.size() <= MAX_NUM_SHADER_MATERIALS);
	gpuMesh.materialsBuffer = gpuAllocator.allocateBuffer(
		ZG_MEMORY_TYPE_DEVICE, cpuMesh.materials.size() * sizeof(ShaderMaterial));
	sfz_assert_debug(gpuMesh.materialsBuffer.valid());
	gpuMesh.numMaterials = cpuMesh.materials.size();
	CHECK_ZG gpuMesh.materialsBuffer.setDebugName(sfz::str128("Material_Buffer_%i", counter++));

	return gpuMesh;
}

void gpuMeshDeallocate(
	GpuMesh& gpuMesh,
	DynamicGpuAllocator& gpuAllocator) noexcept
{
	// Deallocate vertex buffer
	sfz_assert_debug(gpuMesh.vertexBuffer.valid());
	gpuAllocator.deallocate(gpuMesh.vertexBuffer);
	sfz_assert_debug(!gpuMesh.vertexBuffer.valid());

	// Deallocate index buffer
	sfz_assert_debug(gpuMesh.indexBuffer.valid());
	gpuAllocator.deallocate(gpuMesh.indexBuffer);
	sfz_assert_debug(!gpuMesh.indexBuffer.valid());

	// Deallocate materials buffer
	sfz_assert_debug(gpuMesh.materialsBuffer.valid());
	gpuAllocator.deallocate(gpuMesh.materialsBuffer);
	sfz_assert_debug(!gpuMesh.materialsBuffer.valid());

	// Destroy remaining CPU memory
	gpuMesh.components.destroy();
}

void gpuMeshUploadBlocking(
	GpuMesh& gpuMesh,
	const Mesh& cpuMesh,
	DynamicGpuAllocator& gpuAllocator,
	sfz::Allocator* cpuAllocator,
	zg::CommandQueue& copyQueue) noexcept
{
	sfz_assert_debug(gpuMesh.vertexBuffer.valid());
	sfz_assert_debug(gpuMesh.indexBuffer.valid());
	sfz_assert_debug(gpuMesh.materialsBuffer.valid());
	sfz_assert_debug(gpuMesh.components.size() == cpuMesh.components.size());

	// Begin recording copy queue command list
	zg::CommandList commandList;
	CHECK_ZG copyQueue.beginCommandListRecording(commandList);

	// Allocate vertex upload buffer, memcpy data to it and queue upload command
	uint32_t vertexBufferSizeBytes = cpuMesh.vertices.size() * sizeof(Vertex);
	zg::Buffer vertexUploadBuffer =
		gpuAllocator.allocateBuffer(ZG_MEMORY_TYPE_UPLOAD, vertexBufferSizeBytes);
	CHECK_ZG vertexUploadBuffer.memcpyTo(0, cpuMesh.vertices.data(), vertexBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		gpuMesh.vertexBuffer, 0, vertexUploadBuffer, 0, vertexBufferSizeBytes);

	// Allocate index upload buffer, memcpy data to it and queue upload command
	uint32_t indexBufferSizeBytes = cpuMesh.indices.size() * sizeof(uint32_t);
	zg::Buffer indexUploadBuffer =
		gpuAllocator.allocateBuffer(ZG_MEMORY_TYPE_UPLOAD, indexBufferSizeBytes);
	CHECK_ZG indexUploadBuffer.memcpyTo(0, cpuMesh.indices.data(), indexBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		gpuMesh.indexBuffer, 0, indexUploadBuffer, 0, indexBufferSizeBytes);

	// Allocate (cpu) memory for temporary materials buffer and fill it
	sfz_assert_debug(gpuMesh.numMaterials == cpuMesh.materials.size());
	DynArray<ShaderMaterial> gpuMaterials;
	gpuMaterials.create(cpuMesh.materials.size(), cpuAllocator);
	gpuMaterials.addMany(cpuMesh.materials.size());
	for (uint32_t i = 0; i < cpuMesh.materials.size(); i++) {
		ShaderMaterial& dst = gpuMaterials[i];
		const Material& src = cpuMesh.materials[i];
		
		dst.albedo = vec4(src.albedo) * (1.0f / 255.0f);
		dst.emissive.xyz = src.emissive;
		dst.roughness = float(src.roughness) * (1.0f / 255.0f);
		dst.metallic = float(src.metallic) * (1.0f / 255.0f);
		dst.hasAlbedoTex = src.albedoTex != StringID::invalid() ? 1 : 0;
		dst.hasMetallicRoughnessTex = src.metallicRoughnessTex != StringID::invalid() ? 1 : 0;
		dst.hasNormalTex = src.normalTex != StringID::invalid() ? 1 : 0;
		dst.hasOcclusionTex = src.occlusionTex != StringID::invalid() ? 1 : 0;
		dst.hasEmissiveTex = src.emissiveTex != StringID::invalid() ? 1 : 0;
	}

	// Allocate temporary materials upload buffer, memcpy data to it and queue upload command
	uint32_t materialsBufferSizeBytes = cpuMesh.materials.size() * sizeof(ShaderMaterial);
	zg::Buffer materialsUploadBuffer =
		gpuAllocator.allocateBuffer(ZG_MEMORY_TYPE_UPLOAD, materialsBufferSizeBytes);
	CHECK_ZG materialsUploadBuffer.memcpyTo(0, gpuMaterials.data(), materialsBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		gpuMesh.materialsBuffer, 0, materialsUploadBuffer, 0, materialsBufferSizeBytes);

	// Fill components with required info
	uint32_t totalNumIndices = 0;
	for (uint32_t i = 0; i < cpuMesh.components.size(); i++) {
		const MeshComponent& cpuComp = cpuMesh.components[i];
		GpuMeshComponent& gpuComp = gpuMesh.components[i];

		// Store index buffer offsets
		gpuComp.firstIndex = cpuComp.firstIndex;
		gpuComp.numIndices = cpuComp.numIndices;
		totalNumIndices += gpuComp.numIndices;

		// Store material index in component
		sfz_assert_debug(cpuComp.materialIdx < cpuMesh.materials.size());
		gpuComp.materialInfo.materialIdx = cpuComp.materialIdx;

		// Store texture IDs in component
		const Material& material = cpuMesh.materials[cpuComp.materialIdx];
		gpuComp.materialInfo.albedoTex = material.albedoTex;
		gpuComp.materialInfo.metallicRoughnessTex = material.metallicRoughnessTex;
		gpuComp.materialInfo.normalTex = material.normalTex;
		gpuComp.materialInfo.occlusionTex = material.occlusionTex;
		gpuComp.materialInfo.emissiveTex = material.emissiveTex;
	}
	sfz_assert_debug(totalNumIndices == cpuMesh.indices.size());

	
	// Enable resources to be used on other queues than copy queue
	CHECK_ZG commandList.enableQueueTransition(gpuMesh.vertexBuffer);
	CHECK_ZG commandList.enableQueueTransition(gpuMesh.indexBuffer);
	CHECK_ZG commandList.enableQueueTransition(gpuMesh.materialsBuffer);

	// Execute command list to upload all data
	CHECK_ZG copyQueue.executeCommandList(commandList);
	CHECK_ZG copyQueue.flush();

	// Deallocate temporary upload buffers
	gpuAllocator.deallocate(vertexUploadBuffer);
	gpuAllocator.deallocate(indexUploadBuffer);
	gpuAllocator.deallocate(materialsUploadBuffer);
}

} // namespace ph
