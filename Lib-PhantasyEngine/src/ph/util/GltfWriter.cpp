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

#include "ph/util/GltfWriter.hpp"

#include <cfloat>

#include <skipifzero_strings.hpp>

#include <sfz/Logging.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/util/IO.hpp>

namespace sfz {

#if 0

using sfz::DynString;
using sfz::str96;
using sfz::str320;
using sfz::vec2;
using sfz::vec3;

// Statics (input processing)
// ------------------------------------------------------------------------------------------------

struct MeshComponent {
	Array<uint32_t> indices;
	uint32_t materialIdx;
};

// Sorts all triangles in a mesh into different components where each component uses only one
// material. If the entire mesh uses a single material only one component will be returned.
static Array<MeshComponent> componentsFromMesh(const phConstMeshView& mesh) noexcept
{
	sfz_assert((mesh.numIndices % 3) == 0);

	Array<MeshComponent> components;
	components.create(10);

	for (uint32_t i = 0; i < mesh.numIndices; i += 3) {
		uint32_t idx0 = mesh.indices[i + 0];
		uint32_t idx1 = mesh.indices[i + 1];
		uint32_t idx2 = mesh.indices[i + 2];

		// Require material to be same for entire triangle
		uint32_t m0 = mesh.materialIndices[idx0];
		uint32_t m1 = mesh.materialIndices[idx1];
		uint32_t m2 = mesh.materialIndices[idx2];
		sfz_assert(m0 == m1);
		sfz_assert(m1 == m2);

		// Try to find existing component with same material index
		Array<uint32_t>* indicesPtr = nullptr;
		for (MeshComponent& component : components) {
			if (component.materialIdx == m0) {
				indicesPtr = &component.indices;
			}
		}

		// If component did not exist, create it
		if (indicesPtr == nullptr) {
			components.add(MeshComponent());
			components.last().materialIdx = m0;
			indicesPtr = &components.last().indices;
			indicesPtr->create(mesh.numIndices);
		}

		// Add indicies to component
		indicesPtr->add(idx0);
		indicesPtr->add(idx1);
		indicesPtr->add(idx2);
	}

	return components;
}

struct SplitMesh {
	Array<phVertex> vertices;
	Array<MeshComponent> components;
	vec3 posMin = vec3(FLT_MAX);
	vec3 posMax = vec3(-FLT_MAX);
};

// Pick out all meshes from the assets and split them into components
static Array<SplitMesh> splitMeshes(
	const LevelAssets& assets,
	const Array<uint32_t>& meshIndices) noexcept
{
	Array<SplitMesh> splitMeshes;
	splitMeshes.create(meshIndices.size());

	// Go through all meshes, split into components
	for (uint32_t i = 0; i < meshIndices.size(); i++) {
		uint32_t meshIdx = meshIndices[i];

		// Print error message and skip if mesh does not exist
		if (meshIdx >= assets.meshes.size()) {
			SFZ_ERROR("glTF writer", "Trying to write mesh that does not exist: %u", meshIdx);
			continue;
		}

		const Mesh& mesh = assets.meshes[meshIdx];
		SplitMesh splitMesh;

		// Copy vertices
		splitMesh.vertices = mesh.vertices;

		// Split mesh into components
		splitMesh.components = componentsFromMesh(mesh);

		// Find min and max elements
		for (const phVertex& v : splitMesh.vertices) {
			splitMesh.posMax = sfz::max(splitMesh.posMax, v.pos);
			splitMesh.posMin = sfz::min(splitMesh.posMin, v.pos);
		}
		//splitMesh.posMax += vec3(0.1f);
		//splitMesh.posMin -= vec3(0.1f);

		splitMeshes.add(splitMesh);
	}

	return splitMeshes;
}

struct ProcessedAssets {
	Array<SplitMesh> splitMeshes;
	Array<phMaterial> materials;
	Array<uint32_t> textureIndices;
};

// Process assets that are prepared for writing, this includes:
//
// * Spliting meshes into components that only use on material each
// * Pick out the materials used by the meshes so they can be written
// * Update components material indices to point to this new material list
// * Pick out which textures to write to file
// * Update all indices in materials to point to the new textures list
static ProcessedAssets processAssets(
	const LevelAssets& assets, const Array<uint32_t>& meshIndices) noexcept
{
	// Split meshes
	ProcessedAssets processedAssets;
	processedAssets.splitMeshes = splitMeshes(assets, meshIndices);

	// List of material indices to write
	Array<uint32_t> materialIndices;

	// Add materials to write list and modify components to use the new indices
	for (SplitMesh& splitMesh : processedAssets.splitMeshes) {
		for (MeshComponent& component : splitMesh.components) {

			// Linear search through materials to write
			bool materialFound = false;
			for (uint32_t i = 0; i < materialIndices.size(); i++) {
				if (materialIndices[i] != component.materialIdx) continue;

				// If material found in list, just update components index and exit
				component.materialIdx = i;
				materialFound = true;
				break;
			}

			// Add material to write list if not found
			if (!materialFound) {
				materialIndices.add(component.materialIdx);
				processedAssets.materials.add(assets.materials[component.materialIdx]);
				component.materialIdx = materialIndices.size() - 1;
			}
		}
	}

	// Go through materials to write and find all textures to write, also update texture indices in
	// materials to reflect their new indices in the gltf file.
	processedAssets.textureIndices.create(100);
	for (phMaterial& m : processedAssets.materials) {

		// Albedo
		if (m.albedoTexIndex != uint16_t(~0)) {
			sfz_assert(m.albedoTexIndex < assets.textures.size());
			processedAssets.textureIndices.add(m.albedoTexIndex);
			m.albedoTexIndex = uint16_t(processedAssets.textureIndices.size()) - 1;
		}

		// MetallicRoughness
		if (m.metallicRoughnessTexIndex != uint16_t(~0)) {
			sfz_assert(m.metallicRoughnessTexIndex < assets.textures.size());
			processedAssets.textureIndices.add(m.metallicRoughnessTexIndex);
			m.metallicRoughnessTexIndex = uint16_t(processedAssets.textureIndices.size()) - 1;
		}

		// Normal
		if (m.normalTexIndex != uint16_t(~0)) {
			sfz_assert(m.normalTexIndex < assets.textures.size());
			processedAssets.textureIndices.add(m.normalTexIndex);
			m.normalTexIndex = uint16_t(processedAssets.textureIndices.size()) - 1;
		}

		// Occlusion
		if (m.occlusionTexIndex != uint16_t(~0)) {
			sfz_assert(m.occlusionTexIndex < assets.textures.size());
			processedAssets.textureIndices.add(m.occlusionTexIndex);
			m.occlusionTexIndex = uint16_t(processedAssets.textureIndices.size()) - 1;
		}

		// Emissive
		if (m.emissiveTexIndex != uint16_t(~0)) {
			sfz_assert(m.emissiveTexIndex < assets.textures.size());
			processedAssets.textureIndices.add(m.emissiveTexIndex);
			m.emissiveTexIndex = uint16_t(processedAssets.textureIndices.size()) - 1;
		}
	}

	return processedAssets;
}

struct MeshOffsets {
	uint32_t posOffset;
	uint32_t posNumBytes;
	uint32_t normalOffset;
	uint32_t normalNumBytes;
	uint32_t texcoordOffset;
	uint32_t texcoordNumBytes;

	Array<uint32_t> indicesOffsets;
	Array<uint32_t> indicesNumBytes;
};

struct BinaryData {
	Array<uint8_t> combinedBinaryData;
	Array<MeshOffsets> offsets;
};

// Creates a single binary data chunk to write to file. This chunk contains all vertex and index
// data from  the processed assets
static BinaryData createBinaryMeshData(const ProcessedAssets& processedAssets) noexcept
{
	BinaryData data;
	data.combinedBinaryData.create(32 * 1024 * 1024); // Assume 32 MiB will be enough
	data.offsets.create(processedAssets.splitMeshes.size());

	for (const SplitMesh& mesh : processedAssets.splitMeshes) {

		MeshOffsets offsets;

		// Positions
		offsets.posOffset = data.combinedBinaryData.size();
		for (const phVertex& v : mesh.vertices) {
			data.combinedBinaryData.add(reinterpret_cast<const uint8_t*>(&v.pos), sizeof(vec3));
		}
		offsets.posNumBytes = data.combinedBinaryData.size() - offsets.posOffset;

		// Normals
		offsets.normalOffset = data.combinedBinaryData.size();
		for (const phVertex& v : mesh.vertices) {
			data.combinedBinaryData.add(reinterpret_cast<const uint8_t*>(&v.normal), sizeof(vec3));
		}
		offsets.normalNumBytes = data.combinedBinaryData.size() - offsets.normalOffset;

		// Texcoord
		offsets.texcoordOffset = data.combinedBinaryData.size();
		for (const phVertex& v : mesh.vertices) {
			data.combinedBinaryData.add(reinterpret_cast<const uint8_t*>(&v.texcoord), sizeof(vec2));
		}
		offsets.texcoordNumBytes = data.combinedBinaryData.size() - offsets.texcoordOffset;

		// Add indices to combined binary data
		offsets.indicesOffsets.create(mesh.components.size());
		offsets.indicesNumBytes.create(mesh.components.size());
		for (const MeshComponent& component : mesh.components) {
			offsets.indicesOffsets.add(data.combinedBinaryData.size());
			offsets.indicesNumBytes.add(component.indices.size() * sizeof(uint32_t));
			data.combinedBinaryData.add(reinterpret_cast<const uint8_t*>(component.indices.data()),
				offsets.indicesNumBytes.last());
		}

		data.offsets.add(offsets);
	}

	return data;
}

// Statics (paths)
// ------------------------------------------------------------------------------------------------

static str320 calculateBasePath(const char* path) noexcept
{
	str320 str("%s", path);

	// Go through path until the path separator is found
	bool success = false;
	for (uint32_t i = str.size() - 1; i > 0; i--) {
		const char c = str.str[i - 1];
		if (c == '\\' || c == '/') {
			str.str[i] = '\0';
			success = true;
			break;
		}
	}

	// If no path separator is found, assume we have no base path
	if (!success) str.printf("");

	return str;
}

static str96 getFileName(const char* path) noexcept
{
	str96 tmp;
	int32_t len = int32_t(strlen(path));
	for (int32_t i = len; i > 0; i--) {
		char c = path[i-1];
		if (c == '\\' || c == '/') {
			tmp.printf("%s", path + i);
			break;
		}
	}
	return tmp;
}

static str96 stripFileEnding(const str96& fileName) noexcept
{
	str96 tmp = fileName;

	for (int32_t i = int32_t(fileName.size()) - 1; i >= 0; i--) {
		const char c = fileName.str[i];
		if (c == '.') {
			tmp.str[i] = '\0';
			break;
		}
	}

	return tmp;
}

// Statics (Writing to file)
// ------------------------------------------------------------------------------------------------

// Write gltf asset header (non-optional)
// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#asset
static void writeHeader(DynString& gltf) noexcept
{
	gltf.printfAppend("%s", "{\n");
	gltf.printfAppend("%s", "\t\"asset\": {\n");
	gltf.printfAppend("%s", "\t\t\"version\": \"2.0\",\n");
	gltf.printfAppend("%s", "\t\t\"generator\": \"Phantasy Engine Exporter v1.0\"\n");
	gltf.printfAppend("%s", "\t},\n");
}

static void writeMaterials(DynString& gltf, const Array<phMaterial>& materials) noexcept
{
	gltf.printfAppend("%s", "\t\"materials\": [\n");

	auto u8tof32 = [](uint8_t val) {
		return float(val) * (1.0f / 255.0f);
	};

	for (uint32_t i = 0; i < materials.size(); i++) {
		const phMaterial& m = materials[i];

		gltf.printfAppend("%s", "\t\t{\n");

		// Name
		gltf.printfAppend("\t\t\t\"name\": \"%s\",\n", "UnknownMaterialName");

		// PBR material
		gltf.printfAppend("%s", "\t\t\t\"pbrMetallicRoughness\": {\n");

		// Albedo
		gltf.printfAppend("\t\t\t\t\"baseColorFactor\": [%.4f, %.4f, %.4f, %.4f],\n",
			u8tof32(m.albedo.x),
			u8tof32(m.albedo.y),
			u8tof32(m.albedo.z),
			u8tof32(m.albedo.w));

		// Albedo texture
		if (m.albedoTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", "\t\t\t\t\"baseColorTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\t\"index\": %u\n", uint32_t(m.albedoTexIndex));
			gltf.printfAppend("%s", "\t\t\t\t},\n");
		}

		// Roughness
		gltf.printfAppend("\t\t\t\t\"roughnessFactor\": %.4f,\n", u8tof32(m.roughness));

		// Metallic
		gltf.printfAppend("\t\t\t\t\"metallicFactor\": %.4f", u8tof32(m.metallic));

		// Metallic roughness texture
		if (m.metallicRoughnessTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", ",\n");

			gltf.printfAppend("%s", "\t\t\t\t\"metallicRoughnessTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\t\"index\": %u\n", uint32_t(m.metallicRoughnessTexIndex));
			gltf.printfAppend("%s", "\t\t\t\t}\n");
		}
		else {
			gltf.printfAppend("%s", "\n");
		}

		// End PBR material
		gltf.printfAppend("%s", "\t\t\t},\n");

		// Normal texture
		if (m.normalTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", "\t\t\t\"normalTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\"index\": %u\n", uint32_t(m.normalTexIndex));
			gltf.printfAppend("%s", "\t\t\t},\n");
		}

		// Oclussion texture
		if (m.occlusionTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", "\t\t\t\"occlusionTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\"index\": %u\n", uint32_t(m.occlusionTexIndex));
			gltf.printfAppend("%s", "\t\t\t},\n");
		}

		// Emissive texture
		if (m.emissiveTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", "\t\t\t\"emissiveTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\"index\": %u\n", uint32_t(m.emissiveTexIndex));
			gltf.printfAppend("%s", "\t\t\t},\n");
		}

		// Emissive
		gltf.printfAppend("\t\t\t\"emissiveFactor\": [%.4f, %.4f, %.4f]\n",
			u8tof32(m.emissive.x),
			u8tof32(m.emissive.y),
			u8tof32(m.emissive.z));

		if ((i + 1) == materials.size()) gltf.printfAppend("%s", "\t\t}\n");
		else gltf.printfAppend("%s", "\t\t},\n");
	}

	gltf.printfAppend("%s", "\t],\n");
}

static void writeTextures(
	DynString& gltf,
	const char* basePath,
	const char* baseMainFileName,
	const LevelAssets& assets,
	const ProcessedAssets& processedAssets) noexcept
{
	sfz_assert(assets.textures.size() == assets.textureFileMappings.size());
	if (processedAssets.textureIndices.size() == 0) return;

	// Attempt to create directory for textures if necessary
	sfz::createDirectory(str320("%s%s", basePath, baseMainFileName));

	// Write "images" section
	gltf.printfAppend("%s", "\t\"images\": [\n");
	for (uint32_t i = 0; i < processedAssets.textureIndices.size(); i++) {
		uint32_t originalTexIndex = processedAssets.textureIndices[i];
		const FileMapping& mapping = assets.textureFileMappings[originalTexIndex];
		str96 fileNameWithoutEnding = stripFileEnding(mapping.fileName);

		// Write image to file
		str320 imageWritePath("%s/%s/%s.png", basePath, baseMainFileName, fileNameWithoutEnding.str);
		if (!saveImagePng(assets.textures[originalTexIndex], imageWritePath)) {
			SFZ_ERROR("glTF writer", "Failed to write image \"%s\" to path \"%s\"",
				fileNameWithoutEnding.str, imageWritePath.str);
		}

		// Write uri to gltf string
		gltf.printfAppend("%s", "\t\t{\n");
		gltf.printfAppend("\t\t\t\"uri\": \"%s/%s.png\"\n",
			baseMainFileName, fileNameWithoutEnding.str);
		if ((i + 1) == processedAssets.textureIndices.size()) gltf.printfAppend("%s", "\t\t}\n");
		else gltf.printfAppend("%s", "\t\t},\n");
	}
	gltf.printfAppend("%s", "\t],\n");

	// Write "textures" section
	gltf.printfAppend("%s", "\t\"textures\": [\n");
	for (uint32_t i = 0; i < processedAssets.textureIndices.size(); i++) {
		gltf.printfAppend("%s", "\t\t{\n");
		gltf.printfAppend("\t\t\t\"source\": %u\n", i);
		if ((i + 1) == processedAssets.textureIndices.size()) gltf.printfAppend("%s", "\t\t}\n");
		else gltf.printfAppend("%s", "\t\t},\n");
	}
	gltf.printfAppend("%s", "\t],\n");
}

static bool writeMeshes(
	DynString& gltf,
	const char* basePath,
	const char* baseMainFileName,
	const ProcessedAssets& processedAssets,
	const BinaryData& binaryData) noexcept
{
	// Write binary data to file
	bool binaryWriteSuccess = sfz::writeBinaryFile(
		str320("%s%s/%s.bin", basePath, baseMainFileName, baseMainFileName),
		binaryData.combinedBinaryData.data(),
		binaryData.combinedBinaryData.size());
	if (!binaryWriteSuccess) {
		SFZ_ERROR("glTF writer", "Failed to write binary data to file: \"%s\"",
			str320("%s%s.bin", basePath, baseMainFileName).str);
		return false;
	}

	// Write "buffers" section
	gltf.printfAppend("%s", "\t\"buffers\": [\n");
	gltf.printfAppend("%s", "\t\t{\n");
	gltf.printfAppend("\t\t\t\"uri\": \"%s/%s.bin\",\n", baseMainFileName, baseMainFileName);
	gltf.printfAppend("\t\t\t\"byteLength\": %u\n", binaryData.combinedBinaryData.size());
	gltf.printfAppend("%s", "\t\t}\n");
	gltf.printfAppend("%s", "\t],\n");

	// Write "bufferViews" section
	gltf.printfAppend("%s", "\t\"bufferViews\": [\n");
	for (uint32_t i = 0; i < binaryData.offsets.size(); i++) {
		const MeshOffsets& offsets = binaryData.offsets[i];

		// Positions
		gltf.printfAppend("%s", "\n\t\t{\n");
		gltf.printfAppend("%s", "\t\t\t\"buffer\": 0,\n");
		gltf.printfAppend("\t\t\t\"byteOffset\": %u,\n", offsets.posOffset);
		gltf.printfAppend("\t\t\t\"byteLength\": %u,\n", offsets.posNumBytes);
		gltf.printfAppend("\t\t\t\"byteStride\": %u,\n", uint32_t(sizeof(vec3)));
		gltf.printfAppend("%s", "\t\t\t\"target\": 34962\n"); // ARRAY_BUFFER
		gltf.printfAppend("%s", "\t\t},\n");

		// Normals
		gltf.printfAppend("%s", "\t\t{\n");
		gltf.printfAppend("%s", "\t\t\t\"buffer\": 0,\n");
		gltf.printfAppend("\t\t\t\"byteOffset\": %u,\n", offsets.normalOffset);
		gltf.printfAppend("\t\t\t\"byteLength\": %u,\n", offsets.normalNumBytes);
		gltf.printfAppend("\t\t\t\"byteStride\": %u,\n", uint32_t(sizeof(vec3)));
		gltf.printfAppend("%s", "\t\t\t\"target\": 34962\n"); // ARRAY_BUFFER
		gltf.printfAppend("%s", "\t\t},\n");

		// Texcoords
		gltf.printfAppend("%s", "\t\t{\n");
		gltf.printfAppend("%s", "\t\t\t\"buffer\": 0,\n");
		gltf.printfAppend("\t\t\t\"byteOffset\": %u,\n", offsets.texcoordOffset);
		gltf.printfAppend("\t\t\t\"byteLength\": %u,\n", offsets.texcoordNumBytes);
		gltf.printfAppend("\t\t\t\"byteStride\": %u,\n", uint32_t(sizeof(vec2)));
		gltf.printfAppend("%s", "\t\t\t\"target\": 34962\n"); // ARRAY_BUFFER
		gltf.printfAppend("%s", "\t\t},\n");

		// Indices
		for (uint32_t j = 0; j < offsets.indicesOffsets.size(); j++) {
			uint32_t indexOffset = offsets.indicesOffsets[j];
			uint32_t indexNumBytes = offsets.indicesNumBytes[j];

			gltf.printfAppend("%s", "\t\t{\n");
			gltf.printfAppend("%s", "\t\t\t\"buffer\": 0,\n");
			gltf.printfAppend("\t\t\t\"byteOffset\": %u,\n", indexOffset);
			gltf.printfAppend("\t\t\t\"byteLength\": %u,\n", indexNumBytes);
			gltf.printfAppend("%s", "\t\t\t\"target\": 34963\n"); // ELEMENT_ARRAY_BUFFER
			if ((i + 1) == binaryData.offsets.size() && (j + 1) == offsets.indicesOffsets.size()) {
				gltf.printfAppend("%s", "\t\t}\n");
			}
			else {
				gltf.printfAppend("%s", "\t\t},\n");
			}
		}
	}
	gltf.printfAppend("%s", "\t],\n");

	// Write "accessors" section
	gltf.printfAppend("%s", "\t\"accessors\": [\n");
	uint32_t bufferViewIdx = 0;
	for (uint32_t i = 0; i < binaryData.offsets.size(); i++) {
		const MeshOffsets& offsets = binaryData.offsets[i];
		const SplitMesh& splitMesh = processedAssets.splitMeshes[i];

		// Positions
		gltf.printfAppend("%s", "\n\t\t{\n");
		gltf.printfAppend("\t\t\t\"bufferView\": %u,\n", bufferViewIdx);
		gltf.printfAppend("%s", "\t\t\t\"byteOffset\": 0,\n");
		gltf.printfAppend("%s", "\t\t\t\"componentType\": 5126,\n"); // FLOAT
		gltf.printfAppend("%s", "\t\t\t\"type\": \"VEC3\",\n");
		gltf.printfAppend("\t\t\t\"count\": %u,\n", offsets.posNumBytes / sizeof(vec3));
		gltf.printfAppend("%s", "\t\t\t\"min\": [\n");
		gltf.printfAppend("\t\t\t\t%.18f,\n", splitMesh.posMin.x);
		gltf.printfAppend("\t\t\t\t%.18f,\n", splitMesh.posMin.y);
		gltf.printfAppend("\t\t\t\t%.18f\n", splitMesh.posMin.z);
		gltf.printfAppend("%s", "\t\t\t],\n");
		gltf.printfAppend("%s", "\t\t\t\"max\": [\n");
		gltf.printfAppend("\t\t\t\t%.18f,\n", splitMesh.posMax.x);
		gltf.printfAppend("\t\t\t\t%.18f,\n", splitMesh.posMax.y);
		gltf.printfAppend("\t\t\t\t%.18f\n", splitMesh.posMax.z);
		gltf.printfAppend("%s", "\t\t\t]\n");
		gltf.printfAppend("%s", "\t\t},\n");
		bufferViewIdx += 1;

		// Normals
		gltf.printfAppend("%s", "\t\t{\n");
		gltf.printfAppend("\t\t\t\"bufferView\": %u,\n", bufferViewIdx);
		gltf.printfAppend("%s", "\t\t\t\"byteOffset\": 0,\n");
		gltf.printfAppend("%s", "\t\t\t\"componentType\": 5126,\n"); // FLOAT
		gltf.printfAppend("%s", "\t\t\t\"type\": \"VEC3\",\n");
		gltf.printfAppend("\t\t\t\"count\": %u\n", offsets.normalNumBytes / sizeof(vec3));
		gltf.printfAppend("%s", "\t\t},\n");
		bufferViewIdx += 1;

		// Texcoords
		gltf.printfAppend("%s", "\t\t{\n");
		gltf.printfAppend("\t\t\t\"bufferView\": %u,\n", bufferViewIdx);
		gltf.printfAppend("%s", "\t\t\t\"byteOffset\": 0,\n");
		gltf.printfAppend("%s", "\t\t\t\"componentType\": 5126,\n"); // FLOAT
		gltf.printfAppend("%s", "\t\t\t\"type\": \"VEC2\",\n");
		gltf.printfAppend("\t\t\t\"count\": %u\n", offsets.texcoordNumBytes / sizeof(vec2));
		gltf.printfAppend("%s", "\t\t},\n");
		bufferViewIdx += 1;

		for (uint32_t j = 0; j < offsets.indicesOffsets.size(); j++) {
			uint32_t indexNumBytes = offsets.indicesNumBytes[j];

			gltf.printfAppend("%s", "\t\t{\n");
			gltf.printfAppend("\t\t\t\"bufferView\": %u,\n", bufferViewIdx);
			gltf.printfAppend("%s", "\t\t\t\"byteOffset\": 0,\n");
			gltf.printfAppend("%s", "\t\t\t\"componentType\": 5125,\n"); // UNSIGNED INT
			gltf.printfAppend("%s", "\t\t\t\"type\": \"SCALAR\",\n");
			gltf.printfAppend("\t\t\t\"count\": %u\n", indexNumBytes / sizeof(uint32_t));
			if ((i + 1) == binaryData.offsets.size() && (j + 1) == offsets.indicesOffsets.size()) {
				gltf.printfAppend("%s", "\t\t}\n");
			}
			else {
				gltf.printfAppend("%s", "\t\t},\n");
			}
			bufferViewIdx += 1;
		}
	}
	gltf.printfAppend("%s", "\t],\n");


	// Write "meshes" section
	gltf.printfAppend("%s", "\t\"meshes\": [\n");
	sfz_assert(processedAssets.splitMeshes.size() == binaryData.offsets.size());
	uint32_t accessorIdx = 0;
	for (uint32_t i = 0; i < processedAssets.splitMeshes.size(); i++) {
		const SplitMesh& splitMesh = processedAssets.splitMeshes[i];
		const MeshOffsets& offsets = binaryData.offsets[i];

		gltf.printfAppend("%s", "\t\t{\n");

		// Mesh name
		gltf.printfAppend("\t\t\t\"name\": \"%s\",\n", "UNKNOWN NAME");

		str320 meshPrimitiveCommon;
		meshPrimitiveCommon.printf("%s", "\t\t\t\t\t\"attributes\": {\n");
		meshPrimitiveCommon.printfAppend("\t\t\t\t\t\t\"POSITION\": %u,\n", accessorIdx);
		accessorIdx++;
		meshPrimitiveCommon.printfAppend("\t\t\t\t\t\t\"NORMAL\": %u,\n", accessorIdx);
		accessorIdx++;
		meshPrimitiveCommon.printfAppend("\t\t\t\t\t\t\"TEXCOORD_0\": %u\n", accessorIdx);
		accessorIdx++;
		meshPrimitiveCommon.printfAppend("%s", "\t\t\t\t\t},\n");

		// Primitives (components)
		gltf.printfAppend("%s", "\t\t\t\"primitives\": [\n");

		sfz_assert(splitMesh.components.size() == offsets.indicesOffsets.size());
		for (uint32_t j = 0; j < splitMesh.components.size(); j++) {
			const MeshComponent& component = splitMesh.components[j];

			gltf.printfAppend("%s", "\t\t\t\t{\n");
			gltf.printfAppend("%s", meshPrimitiveCommon.str);
			gltf.printfAppend("\t\t\t\t\t\"indices\": %u,\n", accessorIdx);
			accessorIdx++;
			gltf.printfAppend("\t\t\t\t\t\"material\": %u\n", component.materialIdx);

			if ((j + 1) == splitMesh.components.size()) {
				gltf.printfAppend("%s", "\t\t\t\t}\n");
			}
			else {
				gltf.printfAppend("%s", "\t\t\t\t},\n");
			}
		}

		gltf.printfAppend("\t\t\t]\n");

		if ((i + 1) == processedAssets.splitMeshes.size()) {
			gltf.printfAppend("%s", "\t\t}\n");
		}
		else {
			gltf.printfAppend("%s", "\t\t},\n");
		}

	}
	gltf.printfAppend("%s", "\t],\n");

	return true;
}

static void writeScenes(DynString& gltf, const ProcessedAssets& processedAssets) noexcept
{
	// Write nodes
	gltf.printfAppend("%s", "\t\"nodes\": [\n");
	for (uint32_t i = 0; i < processedAssets.splitMeshes.size(); i++) {
		gltf.printfAppend("%s", "\t\t{\n");

		gltf.printfAppend("\t\t\t\"mesh\": %u\n", i);

		if ((i + 1) == processedAssets.splitMeshes.size()) {
			gltf.printfAppend("%s", "\t\t}\n");
		}
		else {
			gltf.printfAppend("%s", "\t\t},\n");
		}
	}
	gltf.printfAppend("%s", "\t],\n");


	// Default scene
	gltf.printfAppend("%s", "\t\"scene\": 0,\n");

	// Scenes (only one)
	gltf.printfAppend("%s", "\t\"scenes\": [\n");
	gltf.printfAppend("%s", "\t\t{\n");
	gltf.printfAppend("%s", "\t\t\t\"nodes\": [\n");
	for (uint32_t i = 0; i < processedAssets.splitMeshes.size(); i++) {
		if((i + 1) == processedAssets.splitMeshes.size()) {
			gltf.printfAppend("\t\t\t\t%u\n", i);
		}
		else {
			gltf.printfAppend("\t\t\t\t%u,\n", i);
		}
	}
	gltf.printfAppend("%s", "\t\t\t]\n");
	gltf.printfAppend("%s", "\t\t}\n");
	gltf.printfAppend("%s", "\t]\n");
}

static void writeExit(DynString& gltf) noexcept
{
	gltf.printfAppend("%s", "}\n");
}

// Entry function
// ------------------------------------------------------------------------------------------------

bool writeAssetsToGltf(
	const char* writePath,
	const LevelAssets& assets,
	const Array<uint32_t>& meshIndices) noexcept
{
	// Try to create base directory if it does not exist
	str320 basePath = calculateBasePath(writePath);
	if (!sfz::directoryExists(basePath)) {
		bool success = sfz::createDirectory(basePath);
		if (!success) {
			SFZ_ERROR("glTF writer", "Failed to create directory \"%s\"", basePath.str);
			return false;
		}
	}

	// Get file name
	str96 fileNameWithoutEnding = stripFileEnding(getFileName(writePath));

	// Process assets to write
	ProcessedAssets processedAssets = processAssets(assets, meshIndices);

	// Create binary data from the processed assets
	BinaryData binaryData = createBinaryMeshData(processedAssets);

	// Create gltf string to fill in
	const uint32_t GLTF_MAX_CAPACITY = 16 * 1024 * 1024; // Assume max 16 MiB for the .gltf file
	sfz::DynString tempGltfString("", GLTF_MAX_CAPACITY);

	// Write header
	writeHeader(tempGltfString);

	// Write materials and textures
	writeMaterials(tempGltfString, processedAssets.materials);
	writeTextures(tempGltfString, basePath, fileNameWithoutEnding.str, assets, processedAssets);

	// Write meshes
	if (!writeMeshes(
		tempGltfString, basePath, fileNameWithoutEnding, processedAssets, binaryData)) {
		return false;
	}

	// Write scenes
	writeScenes(tempGltfString, processedAssets);

	// Write JSON ending
	writeExit(tempGltfString);

	// Write gltf json to file
	bool writeSuccess = sfz::writeTextFile(writePath, tempGltfString.str());
	if (!writeSuccess) {
		SFZ_ERROR("glTF writer", "Failed to write to \"%s\"", writePath);
		return false;
	}

	return true;
}

#endif // if 0

} // namespace sfz

