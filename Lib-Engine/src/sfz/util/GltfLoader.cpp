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

#include "sfz/util/GltfLoader.hpp"

#include <skipifzero.hpp>
#include <skipifzero_strings.hpp>

#include "sfz/Context.hpp"
#include "sfz/Logging.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace sfz {

// Statics
// ------------------------------------------------------------------------------------------------

static str320 calculateBasePath(const char* path) noexcept
{
	str320 str("%s", path);

	// Go through path until the path separator is found
	bool success = false;
	for (uint32_t i = str.size() - 1; i > 0; i--) {
		const char c = str.str()[i - 1];
		if (c == '\\' || c == '/') {
			str.mRawStr[i] = '\0';
			success = true;
			break;
		}
	}

	// If no path separator is found, assume we have no base path
	if (!success) {
		str.clear();
		str.appendf("");
	}

	return str;
}

static uint8_t toU8(float val) noexcept
{
	return uint8_t(std::roundf(val * 255.0f));
}

static vec4_u8 toU8(vec4 val) noexcept
{
	vec4_u8 tmp;
	tmp.x = toU8(val.x);
	tmp.y = toU8(val.y);
	tmp.z = toU8(val.z);
	tmp.w = toU8(val.w);
	return tmp;
}

static void* cgltfAlloc(void* userPtr, cgltf_size size) noexcept
{
	SfzAllocator* allocator = static_cast<SfzAllocator*>(userPtr);
	return allocator->alloc(sfz_dbg("cgltf"), uint64_t(size));
}

static void cgltfFree(void* userPtr, void* ptr) noexcept
{
	SfzAllocator* allocator = static_cast<SfzAllocator*>(userPtr);
	allocator->dealloc(ptr);
}

static cgltf_memory_options sfzToCgltfAllocator(SfzAllocator* allocator) noexcept
{
	cgltf_memory_options memOptions = {};
	memOptions.alloc = cgltfAlloc;
	memOptions.free = cgltfFree;
	memOptions.user_data = allocator;
	return memOptions;
}

static const char* toString(cgltf_result res) noexcept
{
	switch (res) {
	case cgltf_result_success: return "success";
	case cgltf_result_data_too_short: return "data_too_short";
	case cgltf_result_unknown_format: return "unknown_format";
	case cgltf_result_invalid_json: return "invalid_json";
	case cgltf_result_invalid_gltf: return "invalid_gltf";
	case cgltf_result_invalid_options: return "invalid_options";
	case cgltf_result_file_not_found: return "file_not_found";
	case cgltf_result_io_error: return "io_error";
	case cgltf_result_out_of_memory: return "out_of_memory";
	case cgltf_result_legacy_gltf: return "legacy_gltf";
	}
	sfz_assert(false);
	return "";
}

static cgltf_attribute* findAttributeByName(cgltf_primitive& primitive, const char* nameIn)
{
	str64 name = str64("%s", nameIn);
	const uint32_t numAttributes = uint32_t(primitive.attributes_count);
	for (uint32_t i = 0; i < numAttributes; i++) {
		cgltf_attribute& attribute = primitive.attributes[i];
		if (name == attribute.name) return &attribute;
	}
	return nullptr;
}

static const uint8_t* getBufferStart(cgltf_accessor* accessor)
{
	// Start of buffer is offset both by the buffer_view's offset and the accessor's offset
	const uint32_t accessorOffset = uint32_t(accessor->offset);
	const uint32_t bufferViewOffset = uint32_t(accessor->buffer_view->offset);
	const uint8_t* buffer = static_cast<const uint8_t*>(accessor->buffer_view->buffer->data);
	return buffer + accessorOffset + bufferViewOffset;
}

static uint32_t getBufferStrideBytes(cgltf_accessor* accessor)
{
	const uint32_t accessorStrideBytes = uint32_t(accessor->stride);
	const uint32_t bufferViewStrideBytes = uint32_t(accessor->buffer_view->stride);
	if (bufferViewStrideBytes == 0) {
		// If buffer_view's stride is 0, then the stride is determined by the accessor
		sfz_assert(accessorStrideBytes != 0);
		return accessorStrideBytes;
	}
	// TODO: Unsure what's correct here
	sfz_assert(accessorStrideBytes == bufferViewStrideBytes);
	return accessorStrideBytes;// +bufferViewStrideBytes;
}

template<typename T>
static T access(const uint8_t* buffer, uint32_t strideBytes, uint32_t idx)
{
	return *reinterpret_cast<const T*>(buffer + strideBytes * idx);
}

// Function for loading from gltf
// ------------------------------------------------------------------------------------------------

bool loadAssetsFromGltf(
	const char* gltfPath,
	Mesh& meshOut,
	Array<ImageAndPath>& texturesOut,
	SfzAllocator* allocator,
	bool (*checkIfTextureIsLoaded)(strID id, void* userPtr),
	void* userPtr) noexcept
{
	cgltf_options cgltfOptions = {};
	cgltfOptions.memory = sfzToCgltfAllocator(allocator);

	// Attempt to read the gltf file and parse it
	cgltf_data* data = nullptr;
	{
		cgltf_result res = cgltf_parse_file(&cgltfOptions, gltfPath, &data);
		if (res != cgltf_result_success) {
			SFZ_ERROR("cgltf", "Failed to load glTF from \"%s\", result: %s",
				gltfPath, toString(res));
			return false;
		}
	}

	// Attempt to load buffers
	{
		cgltf_result res = cgltf_load_buffers(&cgltfOptions, data, gltfPath);
		if (res != cgltf_result_success) {
			SFZ_ERROR("cgltf", "Failed to load buffers from \"%s\", result: %s",
				gltfPath, toString(res));
			cgltf_free(data);
			return false;
		}
	}

	str320 basePath = calculateBasePath(gltfPath);

	// Load textures
	{
		const uint32_t numTextures = uint32_t(data->textures_count);
		texturesOut.init(numTextures, allocator, sfz_dbg(""));
		for (uint32_t texIdx = 0; texIdx < numTextures; texIdx++) {
			cgltf_texture& texture = data->textures[texIdx];
			cgltf_image& image = *texture.image;

			// Create global path (path relative to game executable)
			const str320 globalPath("%s%s", basePath, image.uri);
			strID globalPathId = strID(globalPath);

			// Check if texture is already loaded, skip it if it is
			if (checkIfTextureIsLoaded != nullptr) {
				if (checkIfTextureIsLoaded(globalPathId, userPtr)) continue;
			}

			// Load and store image
			ImageAndPath pack;
			pack.globalPathId = globalPathId;
			pack.image = loadImage("", globalPath);
			if (pack.image.rawData.data() == nullptr) {
				SFZ_ERROR("cgltf", "Could not load texture: \"%s\"", globalPath.str());
				return false;
			}
			texturesOut.add(std::move(pack));
		}
	}

	// Add materials
	{
		const uint32_t numMaterials = uint32_t(data->materials_count);
		meshOut.materials.init(numMaterials, allocator, sfz_dbg(""));
		for (uint32_t matIdx = 0; matIdx < numMaterials; matIdx++) {
			cgltf_material& material = data->materials[matIdx];

			sfz_assert(material.has_pbr_metallic_roughness);
			cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;

			Material& phMat = meshOut.materials.add();
			phMat.albedo = toU8(vec4(pbr.base_color_factor));
			phMat.roughness = toU8(pbr.roughness_factor);
			phMat.metallic = toU8(pbr.metallic_factor);
			phMat.emissive = vec3(material.emissive_factor);
				
			auto lookupTexture = [&](cgltf_texture_view& view) -> strID {
				if (view.texture == nullptr) return strID();
				sfz_assert(!view.has_transform);

				// Create global path (path relative to game executable)
				const str320 globalPath("%s%s", basePath, view.texture->image->uri);
				return strID(globalPath);
			};

			phMat.albedoTex = lookupTexture(pbr.base_color_texture);
			phMat.metallicRoughnessTex = lookupTexture(pbr.metallic_roughness_texture);
			phMat.normalTex = lookupTexture(material.normal_texture);
			phMat.occlusionTex = lookupTexture(material.occlusion_texture);
			phMat.emissiveTex = lookupTexture(material.emissive_texture);
		}
	}

	// Add single default material if no materials
	if (meshOut.materials.size() == 0) {
		sfz::Material& defaultMaterial = meshOut.materials.add();
		defaultMaterial.emissive = vec3(1.0, 0.0, 0.0);
	}

	// Load all meshes inside file and store them in a single mesh
	{
		const uint32_t numMeshes = uint32_t(data->meshes_count);
		const uint32_t numVertexGuess = numMeshes * 256;
		meshOut.vertices.init(numVertexGuess, allocator, sfz_dbg(""));
		meshOut.indices.init(numVertexGuess * 2, allocator, sfz_dbg(""));
		meshOut.components.init(numMeshes, allocator, sfz_dbg(""));
		for (uint32_t meshIdx = 0; meshIdx < numMeshes; meshIdx++) {
			cgltf_mesh& mesh = data->meshes[meshIdx];

			// TODO: For now, stupidly assume each mesh only have one triangle primitive
			sfz_assert_hard(mesh.primitives_count == 1);
			cgltf_primitive& primitive = mesh.primitives[0];
			sfz_assert_hard(primitive.type == cgltf_primitive_type_triangles);

			// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#geometry
			//
			// Allowed attributes:
			// POSITION, NORMAL, TANGENT, TEXCOORD_0, TEXCOORD_1, COLOR_0, JOINTS_0, WEIGHTS_0
			//
			// Stupidly assume positions, normals, and texcoord_0 exists
			cgltf_attribute* posAttrib = findAttributeByName(primitive, "POSITION");
			cgltf_attribute* normalAttrib = findAttributeByName(primitive, "NORMAL");
			cgltf_attribute* texcoordAttrib = findAttributeByName(primitive, "TEXCOORD_0");
			sfz_assert(findAttributeByName(primitive, "TEXCOORD_1") == nullptr);
			sfz_assert_hard(posAttrib != nullptr);
			sfz_assert_hard(normalAttrib != nullptr);
			sfz_assert_hard(texcoordAttrib != nullptr);
			sfz_assert_hard(posAttrib->data->component_type == cgltf_component_type_r_32f);
			sfz_assert_hard(posAttrib->data->type == cgltf_type_vec3);
			sfz_assert_hard(normalAttrib->data->component_type == cgltf_component_type_r_32f);
			sfz_assert_hard(normalAttrib->data->type == cgltf_type_vec3);
			sfz_assert_hard(texcoordAttrib->data->component_type == cgltf_component_type_r_32f);
			sfz_assert_hard(texcoordAttrib->data->type == cgltf_type_vec2);
			sfz_assert(posAttrib->data->count == normalAttrib->data->count);
			sfz_assert(posAttrib->data->count == texcoordAttrib->data->count);
			const uint32_t numVertices = uint32_t(posAttrib->data->count);

			// Grab data pointers and strides
			const uint8_t* posBuffer = getBufferStart(posAttrib->data);
			const uint32_t posStride = getBufferStrideBytes(posAttrib->data);
			const uint8_t* normalBuffer = getBufferStart(normalAttrib->data);
			const uint32_t normalStride = getBufferStrideBytes(normalAttrib->data);
			const uint8_t* texcoordBuffer = getBufferStart(texcoordAttrib->data);
			const uint32_t texcoordStride = getBufferStrideBytes(texcoordAttrib->data);

			// Add vertices to list of vertices
			const uint32_t offsetToThisComp = meshOut.vertices.size();
			for (uint32_t i = 0; i < numVertices; i++) {
				Vertex& v = meshOut.vertices.add();
				v.pos = access<vec3>(posBuffer, posStride, i);
				v.normal = access<vec3>(normalBuffer, normalStride, i);
				v.texcoord = access<vec2>(texcoordBuffer, texcoordStride, i);
			}

			// Grab index buffer
			const bool index16 = primitive.indices->component_type == cgltf_component_type_r_16u;
			const bool index32 = primitive.indices->component_type == cgltf_component_type_r_32u;
			sfz_assert_hard(index16 || index32);
			sfz_assert_hard(primitive.indices->type == cgltf_type_scalar);
			const uint32_t numIndices = uint32_t(primitive.indices->count);
			const uint8_t* indexBuffer = getBufferStart(primitive.indices);
			const uint32_t indexStride = getBufferStrideBytes(primitive.indices);
			
			// Add indices to list of indices
			MeshComponent comp;
			comp.firstIndex = meshOut.indices.size();
			comp.numIndices = numIndices;
			if (index16) {
				for (uint32_t i = 0; i < numIndices; i++) {
					const uint32_t offsetIndex =
						offsetToThisComp + access<uint16_t>(indexBuffer, indexStride, i);
					meshOut.indices.add(offsetIndex);
				}
			}
			else if (index32) {
				for (uint32_t i = 0; i < numIndices; i++) {
					const uint32_t offsetIndex =
						offsetToThisComp + access<uint32_t>(indexBuffer, indexStride, i);
					meshOut.indices.add(offsetIndex);
				}
			}
			else {
				sfz_assert_hard(false);
			}

			// Material
			const bool hasMaterial = primitive.material != nullptr;
			comp.materialIdx = 0;
			if (hasMaterial) comp.materialIdx = uint32_t(primitive.material - data->materials);
			sfz_assert_hard(comp.materialIdx < meshOut.materials.size());

			// Add component to mesh
			meshOut.components.add(comp);
		}
	}

	// Free cgltf data and return
	cgltf_free(data);
	return true;
}

} // namespace sfz
