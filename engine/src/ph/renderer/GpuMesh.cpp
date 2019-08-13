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
		ZG_MEMORY_TYPE_DEVICE, cpuMesh.vertices.size() * sizeof(phVertex));
	sfz_assert_debug(gpuMesh.vertexBuffer.valid());
	CHECK_ZG gpuMesh.vertexBuffer.setDebugName(sfz::str128("Vertex_Buffer_%i", counter++));

	// Allocate (CPU) memory for mesh component handles
	gpuMesh.components.create(cpuMesh.components.size(), cpuAllocator);

	// Allocate (GPU) memory for index buffers
	for (uint32_t i = 0; i < cpuMesh.components.size(); i++) {
		const MeshComponent& cpuComponent = cpuMesh.components[i];
		GpuMeshComponent component;
		component.indexBuffer = gpuAllocator.allocateBuffer(
			ZG_MEMORY_TYPE_DEVICE, cpuComponent.indices.size() * sizeof(uint32_t));
		sfz_assert_debug(component.indexBuffer.valid());
		CHECK_ZG component.indexBuffer.setDebugName(sfz::str128("Index_Buffer_%i", counter++));
		gpuMesh.components.add(std::move(component));
	}

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
	// Deallocate index buffers
	for (GpuMeshComponent& comp : gpuMesh.components) {
		sfz_assert_debug(comp.indexBuffer.valid());
		gpuAllocator.deallocate(comp.indexBuffer);
		sfz_assert_debug(!comp.indexBuffer.valid());
	}

	// Deallocate vertex buffer
	sfz_assert_debug(gpuMesh.vertexBuffer.valid());
	gpuAllocator.deallocate(gpuMesh.vertexBuffer);
	sfz_assert_debug(!gpuMesh.vertexBuffer.valid());

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
	for (const GpuMeshComponent& comp : gpuMesh.components) sfz_assert_debug(comp.indexBuffer.valid());
	sfz_assert_debug(gpuMesh.components.size() == cpuMesh.components.size());

	// Begin recording copy queue command list
	zg::CommandList commandList;
	CHECK_ZG copyQueue.beginCommandListRecording(commandList);

	// Allocate vertex upload buffer, memcpy data to it and queue upload command
	uint32_t vertexBufferSizeBytes = cpuMesh.vertices.size() * sizeof(phVertex);
	zg::Buffer vertexUploadBuffer =
		gpuAllocator.allocateBuffer(ZG_MEMORY_TYPE_UPLOAD, vertexBufferSizeBytes);
	CHECK_ZG vertexUploadBuffer.memcpyTo(0, cpuMesh.vertices.data(), vertexBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		gpuMesh.vertexBuffer, 0, vertexUploadBuffer, 0, vertexBufferSizeBytes);

	// Allocate (cpu) memory for temporary index upload buffers
	DynArray<zg::Buffer> indexUploadBuffers;
	indexUploadBuffers.create(cpuMesh.components.size(), cpuAllocator);

	// Allocate, memcpy and queue upload commands for all index buffers
	for (uint32_t i = 0; i < cpuMesh.components.size(); i++) {
		const MeshComponent& cpuComp = cpuMesh.components[i];
		GpuMeshComponent& gpuComp = gpuMesh.components[i];

		uint32_t indexBufferSizeBytes = cpuComp.indices.size() * sizeof(uint32_t);
		zg::Buffer indexUploadBuffer =
			gpuAllocator.allocateBuffer(ZG_MEMORY_TYPE_UPLOAD, indexBufferSizeBytes);
		CHECK_ZG indexUploadBuffer.memcpyTo(0, cpuComp.indices.data(), indexBufferSizeBytes);
		CHECK_ZG commandList.memcpyBufferToBuffer(
			gpuComp.indexBuffer, 0, indexUploadBuffer, 0, indexBufferSizeBytes);

		// Store index upload buffer in temporary list
		indexUploadBuffers.add(std::move(indexUploadBuffer));

		// Store number of indices in gpu component
		gpuComp.numIndices = cpuComp.indices.size();

		// Store material index in component
		sfz_assert_debug(cpuComp.materialIdx < cpuMesh.materials.size());
		gpuComp.cpuMaterial.materialIdx = cpuComp.materialIdx;

		// Store texture IDs in component
		const MaterialUnbound& material = cpuMesh.materials[cpuComp.materialIdx];
		gpuComp.cpuMaterial.albedoTex = material.albedoTex;
		gpuComp.cpuMaterial.metallicRoughnessTex = material.metallicRoughnessTex;
		gpuComp.cpuMaterial.normalTex = material.normalTex;
		gpuComp.cpuMaterial.occlusionTex = material.occlusionTex;
		gpuComp.cpuMaterial.emissiveTex = material.emissiveTex;
	}

	// Allocate (cpu) memory for temporary materials buffer and fill it
	DynArray<ShaderMaterial> gpuMaterials;
	gpuMaterials.create(cpuMesh.materials.size(), cpuAllocator);
	gpuMaterials.addMany(cpuMesh.materials.size());
	for (uint32_t i = 0; i < cpuMesh.materials.size(); i++) {
		ShaderMaterial& dst = gpuMaterials[i];
		const MaterialUnbound& src = cpuMesh.materials[i];
		
		dst.albedo = vec4(src.albedo) * (1.0f / 255.0f);
		dst.emissive.xyz = sfz::vec3(src.emissive) * (1.0f / 255.0f);
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

	// Enable resources to be used on other queues than copy queue
	for (GpuMeshComponent& comp : gpuMesh.components) {
		CHECK_ZG commandList.enableQueueTransition(comp.indexBuffer);
	}
	CHECK_ZG commandList.enableQueueTransition(gpuMesh.vertexBuffer);
	CHECK_ZG commandList.enableQueueTransition(gpuMesh.materialsBuffer);

	// Execute command list to upload all data
	CHECK_ZG copyQueue.executeCommandList(commandList);
	CHECK_ZG copyQueue.flush();

	// Deallocate temporary upload buffers
	gpuAllocator.deallocate(vertexUploadBuffer);
	for (zg::Buffer& buffer : indexUploadBuffers) {
		gpuAllocator.deallocate(buffer);
	}
	gpuAllocator.deallocate(materialsUploadBuffer);
}

} // namespace ph
