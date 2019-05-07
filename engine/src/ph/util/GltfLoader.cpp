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

#include "ph/util/GltfLoader.hpp"

#include <sfz/Logging.hpp>
#include <sfz/strings/StackString.hpp>

#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#define TINYGLTF_IMPLEMENTATION

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include <sfz/PushWarnings.hpp>
#include "tiny_gltf.h"
#include <sfz/PopWarnings.hpp>

#include <sfz/Assert.hpp>

#include <sfz/strings/StringHashers.hpp>
#include <sfz/containers/HashMap.hpp>
#include <sfz/strings/StackString.hpp>
#include <sfz/strings/StringID.hpp>

#include <ph/Context.hpp>

namespace ph {

using sfz::HashMap;
using sfz::str320;
using sfz::vec2;
using sfz::vec3;
using sfz::vec3_u8;
using sfz::vec4;
using sfz::vec4_u8;

// Statics
// ------------------------------------------------------------------------------------------------

static bool dummyLoadImageDataFunction(
	tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*, int, void*)
{
	return true;
}

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

enum class ComponentType : uint32_t {
	INT8 = 5120,
	UINT8 = 5121,
	INT16 = 5122,
	UINT16 = 5123,
	UINT32 = 5125,
	FLOAT32 = 5126,
};

static uint32_t numBytes(ComponentType type)
{
	switch (type) {
	case ComponentType::INT8: return 1;
	case ComponentType::UINT8: return 1;
	case ComponentType::INT16: return 2;
	case ComponentType::UINT16: return 2;
	case ComponentType::UINT32: return 4;
	case ComponentType::FLOAT32: return 4;
	}
	return 0;
}

enum class ComponentDimensions : uint32_t {
	SCALAR = TINYGLTF_TYPE_SCALAR,
	VEC2 = TINYGLTF_TYPE_VEC2,
	VEC3 = TINYGLTF_TYPE_VEC3,
	VEC4 = TINYGLTF_TYPE_VEC4,
	MAT2 = TINYGLTF_TYPE_MAT2,
	MAT3 = TINYGLTF_TYPE_MAT3,
	MAT4 = TINYGLTF_TYPE_MAT4,
};

static uint32_t numDimensions(ComponentDimensions dims)
{
	switch (dims) {
	case ComponentDimensions::SCALAR: return 1;
	case ComponentDimensions::VEC2: return 2;
	case ComponentDimensions::VEC3: return 3;
	case ComponentDimensions::VEC4: return 4;
	case ComponentDimensions::MAT2: return 4;
	case ComponentDimensions::MAT3: return 9;
	case ComponentDimensions::MAT4: return 16;
	}
	return 0;
}

struct DataAccess final {
	const uint8_t* rawPtr = nullptr;
	uint32_t numElements = 0;
	ComponentType compType = ComponentType::UINT8;
	ComponentDimensions compDims = ComponentDimensions::SCALAR;

	template<typename T>
	const T& at(uint32_t index) const noexcept
	{
		return reinterpret_cast<const T*>(rawPtr)[index];
	}
};

static DataAccess accessData(
	const tinygltf::Model& model, int accessorIdx) noexcept
{
	// Access Accessor
	if (accessorIdx < 0) return DataAccess();
	if (accessorIdx >= int32_t(model.accessors.size())) return DataAccess();
	const tinygltf::Accessor& accessor = model.accessors[accessorIdx];

	// Access BufferView
	if (accessor.bufferView < 0) return DataAccess();
	if (accessor.bufferView >= int32_t(model.bufferViews.size())) return DataAccess();
	const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];

	// Access Buffer
	if (bufferView.buffer < 0) return DataAccess();
	if (bufferView.buffer >= int32_t(model.buffers.size())) return DataAccess();
	const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

	// Fill DataAccess struct
	DataAccess tmp;
	tmp.rawPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
	tmp.numElements = uint32_t(accessor.count);
	tmp.compType = ComponentType(accessor.componentType);
	tmp.compDims = ComponentDimensions(accessor.type);

	// For now we require that that there is no padding between elements in buffer
	sfz_assert_release(
		bufferView.byteStride == 0 ||
		bufferView.byteStride == size_t(numDimensions(tmp.compDims) * numBytes(tmp.compType)));

	return tmp;
}

static DataAccess accessData(
	const tinygltf::Model& model, const tinygltf::Primitive& primitive, const char* type) noexcept
{
	const auto& itr = primitive.attributes.find(type);
	if (itr == primitive.attributes.end()) return DataAccess();
	return accessData(model, itr->second);
}

static uint8_t toU8(float val) noexcept
{
	return uint8_t(std::roundf(val * 255.0f));
}

static vec4_u8 toSfz(const tinygltf::ColorValue& val) noexcept
{
	vec4_u8 tmp;
	tmp.x = toU8(float(val[0]));
	tmp.y = toU8(float(val[1]));
	tmp.z = toU8(float(val[2]));
	tmp.w = toU8(float(val[3]));
	return tmp;
}

static void getBasePathAndFileName(const str320& path, str256& dirPath, str96& fileName) noexcept
{
	dirPath.printf("%s", path.str);

	// Go through path until the path separator is found
	bool success = false;
	for (uint32_t i = path.size() - 1; i > 0; i--) {
		const char c = path.str[i - 1];
		if (c == '\\' || c == '/') {
			dirPath.str[i] = '\0';
			fileName.printf("%s", path.str + i);
			success = true;
			break;
		}
	}

	// If no path separator is found, assume we have no base path
	if (!success) {
		dirPath.printf("");
		fileName.printf("%s", path.str);
	}
}

static FileMapping createFileMapping(const char* path) noexcept
{
	FileMapping mapping;
	mapping.hasFileMapping = true;
	getBasePathAndFileName(str320("%s", path), mapping.dirPath, mapping.fileName);
	return mapping;
}

static bool extractAssets(
	const char* basePath,
	const tinygltf::Model& model,
	LevelAssets& assets,
	ResourceManager& resourceManager) noexcept
{
	// Load textures
	HashMap<str320, uint16_t> texMapping;
	DynArray<uint16_t> localToGlobalTexIndex(uint32_t(model.textures.size()));
	for (uint32_t i = 0; i < model.textures.size(); i++) {
		const tinygltf::Texture& tex = model.textures[i];
		if (tex.source < 0 || int(model.images.size()) <= tex.source) {
			SFZ_ERROR("tinygltf", "Bad texture source: %i", tex.source);
			continue;
		}
		const tinygltf::Image& img = model.images[tex.source];

		// Create global path (path relative to game executable)
		const str320 globalPath("%s%s", basePath, img.uri.c_str());

		uint16_t globalIdx = resourceManager.registerTexture(globalPath.str);

	
		// Check if texture has already been read
		const uint16_t* texMappingIndexPtr = texMapping.get(img.uri.c_str());
		if (texMappingIndexPtr != nullptr) {
			localToGlobalTexIndex.add(*texMappingIndexPtr);
			continue;
		}

		// Create image from path
		ph::Image phImage = loadImage(basePath, img.uri.c_str());
		if (phImage.rawData.data() == nullptr) {
			SFZ_ERROR("tinygltf", "Could not load texture: %s", img.uri.c_str());
			continue;
		}
		else {
			SFZ_INFO_NOISY("tinygltf", "Loaded texture: %s", img.uri.c_str());
		}

		// Add texture to assets and record its global index in texMapping
		uint16_t globalTexIndex = uint16_t(assets.textures.size());
		assets.textures.add(std::move(phImage));
		texMapping[img.uri.c_str()] = globalTexIndex;
		localToGlobalTexIndex.add(globalTexIndex);

		// Add file mapping
		assets.textureFileMappings.add(createFileMapping(img.uri.c_str()));

		// TODO: We need to store these two values somewhere. Likely in material (because it does
		//       not make perfect sense that everything should access the texture the same way)
		//const tinygltf::Sampler& sampler = model.samplers[tex.sampler];
		//int wrapS = sampler.wrapS; // ["CLAMP_TO_EDGE", "MIRRORED_REPEAT", "REPEAT"], default "REPEAT"
		//int wrapT = sampler.wrapT; // ["CLAMP_TO_EDGE", "MIRRORED_REPEAT", "REPEAT"], default "REPEAT"

	}

	// Load materials
	const uint32_t materialBaseIndex = assets.materials.size();
	for (uint32_t i = 0; i < model.materials.size(); i++) {
		const tinygltf::Material& material = model.materials[i];
		phMaterial phMat;

		// Lambda for checking if parameter exists
		auto hasParamValues = [&](const char* key) {
			return material.values.find(key) != material.values.end();
		};
		auto hasParamAdditionalValues = [&](const char* key) {
			return material.additionalValues.find(key) != material.additionalValues.end();
		};

		// Albedo value
		if (hasParamValues("baseColorFactor")) {
			const tinygltf::Parameter& param = material.values.find("baseColorFactor")->second;
			tinygltf::ColorValue color = param.ColorFactor();
			phMat.albedo = toSfz(color);
		}

		// Albedo texture
		if (hasParamValues("baseColorTexture")) {
			const tinygltf::Parameter& param = material.values.find("baseColorTexture")->second;
			int texIndex = param.TextureIndex();
			if (texIndex < 0 || int(model.textures.size()) <= texIndex) {
				SFZ_ERROR("tinygltf", "Bad texture index for material %u", i);
				continue;
			}
			phMat.albedoTexIndex = localToGlobalTexIndex[texIndex];
			// TODO: Store which texcoords to use
		}

		// Roughness Value
		if (hasParamValues("roughnessFactor")) {
			const tinygltf::Parameter& param = material.values.find("roughnessFactor")->second;
			phMat.roughness = toU8(float(param.Factor()));
		}

		// Metallic Value
		if (hasParamValues("metallicFactor")) {
			const tinygltf::Parameter& param = material.values.find("metallicFactor")->second;
			phMat.metallic = toU8(float(param.Factor()));
		}

		// Emissive value
		if (hasParamAdditionalValues("emissiveFactor")) {
			const tinygltf::Parameter& param = material.additionalValues.find("emissiveFactor")->second;
			phMat.emissive = toSfz(param.ColorFactor()).xyz;
		}

		// Roughness and Metallic texture
		if (hasParamValues("metallicRoughnessTexture")) {
			const tinygltf::Parameter& param = material.values.find("metallicRoughnessTexture")->second;
			int texIndex = param.TextureIndex();
			if (texIndex < 0 || int(model.textures.size()) <= texIndex) {
				SFZ_ERROR("tinygltf", "Bad texture index for material %u", i);
				continue;
			}
			phMat.metallicRoughnessTexIndex = localToGlobalTexIndex[texIndex];
			// TODO: Store which texcoords to use
		}

		// Normal texture
		if (hasParamAdditionalValues("normalTexture")) {
			const tinygltf::Parameter& param = material.additionalValues.find("normalTexture")->second;
			int texIndex = param.TextureIndex();
			if (texIndex < 0 || int(model.textures.size()) <= texIndex) {
				SFZ_ERROR("tinygltf", "Bad texture index for material %u", i);
				continue;
			}
			phMat.normalTexIndex = localToGlobalTexIndex[texIndex];
			// TODO: Store which texcoords to use
		}

		// Occlusion texture
		if (hasParamAdditionalValues("occlusionTexture")) {
			const tinygltf::Parameter& param = material.additionalValues.find("occlusionTexture")->second;
			int texIndex = param.TextureIndex();
			if (texIndex < 0 || int(model.textures.size()) <= texIndex) {
				SFZ_ERROR("tinygltf", "Bad texture index for material %u", i);
				continue;
			}
			phMat.occlusionTexIndex = localToGlobalTexIndex[texIndex];
			// TODO: Store which texcoords to use
		}

		// Emissive texture
		if (hasParamAdditionalValues("emissiveTexture")) {
			const tinygltf::Parameter& param = material.additionalValues.find("emissiveTexture")->second;
			int texIndex = param.TextureIndex();
			if (texIndex < 0 || int(model.textures.size()) <= texIndex) {
				SFZ_ERROR("tinygltf", "Bad texture index for material %u", i);
				continue;
			}
			phMat.emissiveTexIndex = localToGlobalTexIndex[texIndex];
			// TODO: Store which texcoords to use
		}

		// Remove default emissive factor if no emissive is specified
		if (phMat.emissiveTexIndex == uint16_t(~0) && !hasParamAdditionalValues("emissiveFactor")) {
			phMat.emissive = vec3_u8(uint8_t(0));
		}

		// Add material to assets
		assets.materials.add(phMat);
	}

	// Add meshes
	for (uint32_t i = 0; i < uint32_t(model.meshes.size()); i++) {
		const tinygltf::Mesh& mesh = model.meshes[i];
		Mesh phMesh;

		// TODO: For now, stupidly assume each mesh only have one primitive
		const tinygltf::Primitive& primitive = mesh.primitives[0];

		// Mode can be:
		// TINYGLTF_MODE_POINTS (0)
		// TINYGLTF_MODE_LINE (1)
		// TINYGLTF_MODE_LINE_LOOP (2)
		// TINYGLTF_MODE_TRIANGLES (4)
		// TINYGLTF_MODE_TRIANGLE_STRIP (5)
		// TINYGLTF_MODE_TRIANGLE_FAN (6)
		sfz_assert_release(primitive.mode == TINYGLTF_MODE_TRIANGLES);

		sfz_assert_release(
			primitive.indices >= 0 && primitive.indices < int(model.accessors.size()));

		// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#geometry
		//
		// Allowed attributes:
		// POSITION, NORMAL, TANGENT, TEXCOORD_0, TEXCOORD_1, COLOR_0, JOINTS_0, WEIGHTS_0
		//
		// Stupidly assume positions, normals, and texcoord_0 exists
		DataAccess posAccess = accessData(model, primitive, "POSITION");
		sfz_assert_release(posAccess.rawPtr != nullptr);
		sfz_assert_release(posAccess.compType == ComponentType::FLOAT32);
		sfz_assert_release(posAccess.compDims == ComponentDimensions::VEC3);

		DataAccess normalAccess = accessData(model, primitive, "NORMAL");
		sfz_assert_release(normalAccess.rawPtr != nullptr);
		sfz_assert_release(normalAccess.compType == ComponentType::FLOAT32);
		sfz_assert_release(normalAccess.compDims == ComponentDimensions::VEC3);

		DataAccess texcoord0Access = accessData(model, primitive, "TEXCOORD_0");
		sfz_assert_release(texcoord0Access.rawPtr != nullptr)
		sfz_assert_release(texcoord0Access.compType == ComponentType::FLOAT32);
		sfz_assert_release(texcoord0Access.compDims == ComponentDimensions::VEC2);

		// Assume texcoord_1 does NOT exist
		DataAccess texcoord1Access = accessData(model, primitive, "TEXCOORD_1");
		sfz_assert_release(texcoord1Access.rawPtr == nullptr);

		// Create vertices from positions and normals
		// TODO: Texcoords
		sfz_assert_release(posAccess.numElements == normalAccess.numElements);
		phMesh.vertices.create(posAccess.numElements);
		for (uint32_t j = 0; j < posAccess.numElements; j++) {
			phVertex vertex;
			vertex.pos = posAccess.at<vec3>(j);
			vertex.normal = normalAccess.at<vec3>(j);
			vertex.texcoord = texcoord0Access.at<vec2>(j);
			phMesh.vertices.add(vertex);
		}

		// Create indices
		DataAccess idxAccess = accessData(model, primitive.indices);
		sfz_assert_release(idxAccess.rawPtr != nullptr);
		sfz_assert_release(idxAccess.compDims == ComponentDimensions::SCALAR);
		if (idxAccess.compType == ComponentType::UINT32) {
			phMesh.indices.create(idxAccess.numElements);
			phMesh.indices.add(&idxAccess.at<uint32_t>(0), idxAccess.numElements);
		}
		else if (idxAccess.compType == ComponentType::UINT16) {
			phMesh.indices.create(idxAccess.numElements);
			for (uint32_t j = 0; j < idxAccess.numElements; j++) {
				phMesh.indices.add(uint32_t(idxAccess.at<uint16_t>(j)));
			}
		}
		else {
			sfz_assert_release(false);
		}

		// Create materialIndices
		phMesh.materialIndices.create(phMesh.vertices.size());
		phMesh.materialIndices.addMany(phMesh.vertices.size(),
			materialBaseIndex + primitive.material);

		// Add mesh to assets
		assets.meshes.add(std::move(phMesh));
	}

	return true;
}

// Function for loading from gltf
// ------------------------------------------------------------------------------------------------

bool loadAssetsFromGltf(
	const char* gltfPath,
	LevelAssets& assets,
	ResourceManager& resourceManager) noexcept
{
	str320 basePath = calculateBasePath(gltfPath);

	// Initializing loader with dummy image loader function
	tinygltf::TinyGLTF loader;
	loader.SetImageLoader(dummyLoadImageDataFunction, nullptr);

	// Read model from file
	tinygltf::Model model;
	std::string error;
	std::string warnings;
	bool result = loader.LoadASCIIFromFile(&model, &error, &warnings, gltfPath);

	// Check error string
	if (!warnings.empty()) {
		SFZ_WARNING("tinygltf", "Warnings loading \"%s\": %s", gltfPath, warnings.c_str());
	}
	if (!error.empty()) {
		SFZ_ERROR("tinygltf", "Error loading \"%s\": %s", gltfPath, error.c_str());
		return false;
	}

	// Check return code
	if (!result) {
		SFZ_ERROR("tinygltf", "Error loading \"%s\"", gltfPath);
		return false;
	}

	// Log that model was succesfully loaded
	SFZ_INFO_NOISY("tinygltf", "Model \"%s\" loaded succesfully", gltfPath);

	// Extract assets from results
	return extractAssets(basePath.str, model, assets, resourceManager);
}

} // namespace ph
