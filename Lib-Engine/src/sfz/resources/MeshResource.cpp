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

#include "sfz/resources/MeshResource.hpp"

#include <skipifzero.hpp>
#include <skipifzero_strings.hpp>

#include "sfz/Context.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/resources/BufferResource.hpp"
#include "sfz/resources/ResourceManager.hpp"

namespace sfz {

// GpuMesh functions
// ------------------------------------------------------------------------------------------------

ShaderMaterial cpuMaterialToShaderMaterial(const Material& cpuMaterial) noexcept
{
	ShaderMaterial dst;
	dst.albedo = f32x4(cpuMaterial.albedo) * (1.0f / 255.0f);
	dst.emissive.xyz() = cpuMaterial.emissive;
	dst.roughness = f32(cpuMaterial.roughness) * (1.0f / 255.0f);
	dst.metallic = f32(cpuMaterial.metallic) * (1.0f / 255.0f);
	dst.hasAlbedoTex = cpuMaterial.albedoTex.isValid() ? 1 : 0;
	dst.hasMetallicRoughnessTex = cpuMaterial.metallicRoughnessTex.isValid() ? 1 : 0;
	dst.hasNormalTex = cpuMaterial.normalTex.isValid() ? 1 : 0;
	dst.hasOcclusionTex = cpuMaterial.occlusionTex.isValid() ? 1 : 0;
	dst.hasEmissiveTex = cpuMaterial.emissiveTex.isValid() ? 1 : 0;
	return dst;
}

MeshResource meshResourceAllocate(
	const char* meshName,
	const Mesh& cpuMesh,
	SfzAllocator* cpuAllocator) noexcept
{
	ResourceManager& resources = getResourceManager();

	MeshResource gpuMesh;

	gpuMesh.name = strID(meshName);

	// Allocate GPU buffers
	gpuMesh.vertexBuffer = resources.addBuffer(BufferResource::createStatic(
		str320("%s__Vertex_Buffer", meshName).str(), sizeof(Vertex), cpuMesh.vertices.size()));
	gpuMesh.indexBuffer = resources.addBuffer(BufferResource::createStatic(
		str320("%s__Index_Buffer", meshName).str(), sizeof(u32), cpuMesh.indices.size()));
	gpuMesh.materialsBuffer = resources.addBuffer(BufferResource::createStatic(
		str320("%s__Materials_Buffer", meshName).str(), sizeof(ShaderMaterial), MAX_NUM_SHADER_MATERIALS));

	// Allocate (CPU) memory for mesh component handles
	gpuMesh.components.init(cpuMesh.components.size(), cpuAllocator, sfz_dbg("GpuMesh::components"));
	gpuMesh.components.add(MeshComponent(), cpuMesh.components.size());

	// Allocate (CPU) memory for cpu materials
	sfz_assert(cpuMesh.materials.size() <= MAX_NUM_SHADER_MATERIALS);
	gpuMesh.numMaterials = cpuMesh.materials.size();
	gpuMesh.cpuMaterials.init(cpuMesh.materials.size(), cpuAllocator, sfz_dbg("GpuMesh::cpuMaterials"));
	gpuMesh.cpuMaterials.add(Material(), cpuMesh.materials.size());

	return gpuMesh;
}

void meshResourceUploadBlocking(
	MeshResource& gpuMesh,
	const Mesh& cpuMesh,
	SfzAllocator* cpuAllocator,
	zg::CommandQueue& copyQueue) noexcept
{
	ResourceManager& resources = getResourceManager();
	sfz_assert(gpuMesh.vertexBuffer != SFZ_NULL_HANDLE);
	sfz_assert(gpuMesh.indexBuffer != SFZ_NULL_HANDLE);
	sfz_assert(gpuMesh.materialsBuffer != SFZ_NULL_HANDLE);
	sfz_assert(gpuMesh.components.size() == cpuMesh.components.size());
	
	// Grab gpu buffers
	BufferResource* vertexBufferResource = resources.getBuffer(gpuMesh.vertexBuffer);
	sfz_assert(vertexBufferResource->type == BufferResourceType::STATIC);
	zg::Buffer& vertexBuffer = vertexBufferResource->staticMem.buffer;

	BufferResource* indexBufferResource = resources.getBuffer(gpuMesh.indexBuffer);
	sfz_assert(indexBufferResource->type == BufferResourceType::STATIC);
	zg::Buffer& indexBuffer = indexBufferResource->staticMem.buffer;

	BufferResource* materialsBufferResource = resources.getBuffer(gpuMesh.materialsBuffer);
	sfz_assert(materialsBufferResource->type == BufferResourceType::STATIC);
	zg::Buffer& materialsBuffer = materialsBufferResource->staticMem.buffer;

	// Begin recording copy queue command list
	zg::CommandList commandList;
	CHECK_ZG copyQueue.beginCommandListRecording(commandList);

	// Allocate vertex upload buffer, memcpy data to it and queue upload command
	u32 vertexBufferSizeBytes = cpuMesh.vertices.size() * sizeof(Vertex);
	zg::Buffer vertexUploadBuffer;
	CHECK_ZG vertexUploadBuffer.create(vertexBufferSizeBytes, ZG_MEMORY_TYPE_UPLOAD);
	CHECK_ZG vertexUploadBuffer.memcpyUpload(0, cpuMesh.vertices.data(), vertexBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		vertexBuffer, 0, vertexUploadBuffer, 0, vertexBufferSizeBytes);

	// Allocate index upload buffer, memcpy data to it and queue upload command
	u32 indexBufferSizeBytes = cpuMesh.indices.size() * sizeof(u32);
	zg::Buffer indexUploadBuffer;
	CHECK_ZG indexUploadBuffer.create(indexBufferSizeBytes, ZG_MEMORY_TYPE_UPLOAD);
	CHECK_ZG indexUploadBuffer.memcpyUpload(0, cpuMesh.indices.data(), indexBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		indexBuffer, 0, indexUploadBuffer, 0, indexBufferSizeBytes);

	// Allocate (cpu) memory for temporary materials buffer and fill it
	sfz_assert(gpuMesh.numMaterials == cpuMesh.materials.size());
	Array<ShaderMaterial> gpuMaterials;
	gpuMaterials.init(cpuMesh.materials.size(), cpuAllocator, sfz_dbg("gpuMaterials"));
	gpuMaterials.add(ShaderMaterial(), cpuMesh.materials.size());
	for (u32 i = 0; i < cpuMesh.materials.size(); i++) {
		gpuMaterials[i] = cpuMaterialToShaderMaterial(cpuMesh.materials[i]);
	}

	// Allocate temporary materials upload buffer, memcpy data to it and queue upload command
	u32 materialsBufferSizeBytes = cpuMesh.materials.size() * sizeof(ShaderMaterial);
	zg::Buffer materialsUploadBuffer;
	CHECK_ZG materialsUploadBuffer.create(materialsBufferSizeBytes, ZG_MEMORY_TYPE_UPLOAD);
	CHECK_ZG materialsUploadBuffer.memcpyUpload(0, gpuMaterials.data(), materialsBufferSizeBytes);
	CHECK_ZG commandList.memcpyBufferToBuffer(
		materialsBuffer, 0, materialsUploadBuffer, 0, materialsBufferSizeBytes);

	// Copy components
	sfz_assert(cpuMesh.components.size() == gpuMesh.components.size());
	u32 totalNumIndices = 0;
	for (u32 i = 0; i < cpuMesh.components.size(); i++) {
		gpuMesh.components[i] = cpuMesh.components[i];
		totalNumIndices += gpuMesh.components[i].numIndices;
	}
	sfz_assert(totalNumIndices == cpuMesh.indices.size());

	// Copy cpu materials
	sfz_assert(gpuMesh.cpuMaterials.size() == cpuMesh.materials.size());
	for (u32 i = 0; i < gpuMesh.cpuMaterials.size(); i++) {
		gpuMesh.cpuMaterials[i] = cpuMesh.materials[i];
	}
	
	// Enable resources to be used on other queues than copy queue
	CHECK_ZG commandList.enableQueueTransition(vertexBuffer);
	CHECK_ZG commandList.enableQueueTransition(indexBuffer);
	CHECK_ZG commandList.enableQueueTransition(materialsBuffer);

	// Execute command list to upload all data
	CHECK_ZG copyQueue.executeCommandList(commandList);
	CHECK_ZG copyQueue.flush();
}

} // namespace sfz
