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

#include <skipifzero.hpp>
#include <skipifzero_strings.hpp>

#include "ph/renderer/ZeroGUtils.hpp"

namespace sfz {

// GpuMesh functions
// ------------------------------------------------------------------------------------------------

ShaderMaterial cpuMaterialToShaderMaterial(const Material& cpuMaterial) noexcept
{
	ShaderMaterial dst;
	dst.albedo = vec4(cpuMaterial.albedo) * (1.0f / 255.0f);
	dst.emissive.xyz = cpuMaterial.emissive;
	dst.roughness = float(cpuMaterial.roughness) * (1.0f / 255.0f);
	dst.metallic = float(cpuMaterial.metallic) * (1.0f / 255.0f);
	dst.hasAlbedoTex = cpuMaterial.albedoTex != StringID::invalid() ? 1 : 0;
	dst.hasMetallicRoughnessTex = cpuMaterial.metallicRoughnessTex != StringID::invalid() ? 1 : 0;
	dst.hasNormalTex = cpuMaterial.normalTex != StringID::invalid() ? 1 : 0;
	dst.hasOcclusionTex = cpuMaterial.occlusionTex != StringID::invalid() ? 1 : 0;
	dst.hasEmissiveTex = cpuMaterial.emissiveTex != StringID::invalid() ? 1 : 0;
	return dst;
}

GpuMesh gpuMeshAllocate(
	const Mesh& cpuMesh,
	DynamicGpuAllocator& gpuAllocatorDevice,
	sfz::Allocator* cpuAllocator) noexcept
{
	sfz_assert(gpuAllocatorDevice.queryMemoryType() == ZG_MEMORY_TYPE_DEVICE);
	static uint32_t counter = 0;
	GpuMesh gpuMesh;

	// Allocate (GPU) memory for vertices
	gpuMesh.vertexBuffer = gpuAllocatorDevice.allocateBuffer(
		cpuMesh.vertices.size() * sizeof(Vertex));
	sfz_assert(gpuMesh.vertexBuffer.valid());
	CHECK_ZG gpuMesh.vertexBuffer.setDebugName(sfz::str128("Vertex_Buffer_%i", counter++));

	// Allocate (GPU) memory for indices
	gpuMesh.indexBuffer = gpuAllocatorDevice.allocateBuffer(
		cpuMesh.indices.size() * sizeof(uint32_t));
	sfz_assert(gpuMesh.indexBuffer.valid());
	CHECK_ZG gpuMesh.indexBuffer.setDebugName(sfz::str128("Index_Buffer_%i", counter++));

	// Allocate (CPU) memory for mesh component handles
	gpuMesh.components.init(cpuMesh.components.size(), cpuAllocator, sfz_dbg("GpuMesh::components"));
	gpuMesh.components.add(MeshComponent(), cpuMesh.components.size());

	// Allocate (GPU) memory for materials
	sfz_assert(cpuMesh.materials.size() <= MAX_NUM_SHADER_MATERIALS);
	gpuMesh.materialsBuffer = gpuAllocatorDevice.allocateBuffer(
		cpuMesh.materials.size() * sizeof(ShaderMaterial));
	sfz_assert(gpuMesh.materialsBuffer.valid());
	gpuMesh.numMaterials = cpuMesh.materials.size();
	CHECK_ZG gpuMesh.materialsBuffer.setDebugName(sfz::str128("Material_Buffer_%i", counter++));

	// Allocate (CPU) memory for cpu materials
	gpuMesh.cpuMaterials.init(cpuMesh.materials.size(), cpuAllocator, sfz_dbg("GpuMesh::cpuMaterials"));
	gpuMesh.cpuMaterials.add(Material(), cpuMesh.materials.size());

	return gpuMesh;
}

void gpuMeshDeallocate(
	GpuMesh& gpuMesh,
	DynamicGpuAllocator& gpuAllocatorDevice) noexcept
{
	sfz_assert(gpuAllocatorDevice.queryMemoryType() == ZG_MEMORY_TYPE_DEVICE);

	// Deallocate vertex buffer
	sfz_assert(gpuMesh.vertexBuffer.valid());
	gpuAllocatorDevice.deallocate(gpuMesh.vertexBuffer);
	sfz_assert(!gpuMesh.vertexBuffer.valid());

	// Deallocate index buffer
	sfz_assert(gpuMesh.indexBuffer.valid());
	gpuAllocatorDevice.deallocate(gpuMesh.indexBuffer);
	sfz_assert(!gpuMesh.indexBuffer.valid());

	// Deallocate materials buffer
	sfz_assert(gpuMesh.materialsBuffer.valid());
	gpuAllocatorDevice.deallocate(gpuMesh.materialsBuffer);
	sfz_assert(!gpuMesh.materialsBuffer.valid());

	// Destroy remaining CPU memory
	gpuMesh.components.destroy();
	gpuMesh.cpuMaterials.destroy();
}

void gpuMeshUploadBlocking(
	GpuMesh& gpuMesh,
	const Mesh& cpuMesh,
	DynamicGpuAllocator& gpuAllocatorUpload,
	sfz::Allocator* cpuAllocator,
	zg::CommandQueue& copyQueue) noexcept
{
	sfz_assert(gpuAllocatorUpload.queryMemoryType() == ZG_MEMORY_TYPE_UPLOAD);
	sfz_assert(gpuMesh.vertexBuffer.valid());
	sfz_assert(gpuMesh.indexBuffer.valid());
	sfz_assert(gpuMesh.materialsBuffer.valid());
	sfz_assert(gpuMesh.components.size() == cpuMesh.components.size());

	// Begin recording copy queue command list
	zg::CommandList commandList;
	CHECK_ZG copyQueue.beginCommandListRecording(commandList);

	// Allocate vertex upload buffer, memcpy data to it and queue upload command
	uint32_t vertexBufferSizeBytes = cpuMesh.vertices.size() * sizeof(Vertex);
	zg::Buffer vertexUploadBuffer =
		gpuAllocatorUpload.allocateBuffer(vertexBufferSizeBytes);
	CHECK_ZG vertexUploadBuffer.memcpyTo(0, cpuMesh.vertices.data(), vertexBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		gpuMesh.vertexBuffer, 0, vertexUploadBuffer, 0, vertexBufferSizeBytes);

	// Allocate index upload buffer, memcpy data to it and queue upload command
	uint32_t indexBufferSizeBytes = cpuMesh.indices.size() * sizeof(uint32_t);
	zg::Buffer indexUploadBuffer =
		gpuAllocatorUpload.allocateBuffer(indexBufferSizeBytes);
	CHECK_ZG indexUploadBuffer.memcpyTo(0, cpuMesh.indices.data(), indexBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		gpuMesh.indexBuffer, 0, indexUploadBuffer, 0, indexBufferSizeBytes);

	// Allocate (cpu) memory for temporary materials buffer and fill it
	sfz_assert(gpuMesh.numMaterials == cpuMesh.materials.size());
	Array<ShaderMaterial> gpuMaterials;
	gpuMaterials.init(cpuMesh.materials.size(), cpuAllocator, sfz_dbg("gpuMaterials"));
	gpuMaterials.add(ShaderMaterial(), cpuMesh.materials.size());
	for (uint32_t i = 0; i < cpuMesh.materials.size(); i++) {
		gpuMaterials[i] = cpuMaterialToShaderMaterial(cpuMesh.materials[i]);
	}

	// Allocate temporary materials upload buffer, memcpy data to it and queue upload command
	uint32_t materialsBufferSizeBytes = cpuMesh.materials.size() * sizeof(ShaderMaterial);
	zg::Buffer materialsUploadBuffer =
		gpuAllocatorUpload.allocateBuffer(materialsBufferSizeBytes);
	CHECK_ZG materialsUploadBuffer.memcpyTo(0, gpuMaterials.data(), materialsBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		gpuMesh.materialsBuffer, 0, materialsUploadBuffer, 0, materialsBufferSizeBytes);

	// Copy components
	sfz_assert(cpuMesh.components.size() == gpuMesh.components.size());
	uint32_t totalNumIndices = 0;
	for (uint32_t i = 0; i < cpuMesh.components.size(); i++) {
		gpuMesh.components[i] = cpuMesh.components[i];
		totalNumIndices += gpuMesh.components[i].numIndices;
	}
	sfz_assert(totalNumIndices == cpuMesh.indices.size());

	// Copy cpu materials
	sfz_assert(gpuMesh.cpuMaterials.size() == cpuMesh.materials.size());
	for (uint32_t i = 0; i < gpuMesh.cpuMaterials.size(); i++) {
		gpuMesh.cpuMaterials[i] = cpuMesh.materials[i];
	}
	
	// Enable resources to be used on other queues than copy queue
	CHECK_ZG commandList.enableQueueTransition(gpuMesh.vertexBuffer);
	CHECK_ZG commandList.enableQueueTransition(gpuMesh.indexBuffer);
	CHECK_ZG commandList.enableQueueTransition(gpuMesh.materialsBuffer);

	// Execute command list to upload all data
	CHECK_ZG copyQueue.executeCommandList(commandList);
	CHECK_ZG copyQueue.flush();

	// Deallocate temporary upload buffers
	gpuAllocatorUpload.deallocate(vertexUploadBuffer);
	gpuAllocatorUpload.deallocate(indexUploadBuffer);
	gpuAllocatorUpload.deallocate(materialsUploadBuffer);
}

} // namespace sfz
