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

#include "d3d12/D3D12Pipelines.hpp"

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_new.hpp>
#include <skipifzero_strings.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>

#include "common/Strings.hpp"

using time_point = std::chrono::high_resolution_clock::time_point;

// Statics
// ------------------------------------------------------------------------------------------------

static f32 timeSinceLastTimestampMillis(const time_point& previousTime) noexcept
{
	time_point currentTime = std::chrono::high_resolution_clock::now();
	using FloatMS = std::chrono::duration<f32, std::milli>;
	return std::chrono::duration_cast<FloatMS>(currentTime - previousTime).count();
}

static sfz::Array<u8> readBinaryFile(const char* path) noexcept
{
	// Open file
	std::FILE* file = std::fopen(path, "rb");
	if (file == NULL) return sfz::Array<u8>();

	// Get size of file
	std::fseek(file, 0, SEEK_END);
	i64 size = std::ftell(file);
	if (size <= 0) {
		std::fclose(file);
		return sfz::Array<u8>();
	}
	std::fseek(file, 0, SEEK_SET);

	// Allocate memory for file
	sfz::Array<u8> data;
	data.init(u32(size), getAllocator(), sfz_dbg("binary file"));
	data.hackSetSize(u32(size));

	// Read file
	size_t bytesRead = std::fread(data.data(), 1, data.size(), file);
	if (bytesRead != size_t(size)) {
		std::fclose(file);
		return sfz::Array<u8>();
	}

	// Close file and return data
	std::fclose(file);
	return data;
}

static bool writeBinaryFile(const char* path, const void* data, size_t numBytes) noexcept
{
	// Open file
	if (path == nullptr) return false;
	std::FILE* file = std::fopen(path, "wb");
	if (file == NULL) return false;

	size_t numWritten = std::fwrite(data, 1, numBytes, file);
	std::fclose(file);
	return (numWritten == numBytes);
}

static time_t getFileLastModifiedDate(const char* path) noexcept
{
	static_assert(sizeof(time_t) == sizeof(__time64_t), "");
	// struct _stat64i32
	// {
	//	_dev_t         st_dev;
	//	_ino_t         st_ino;
	//	unsigned short st_mode;
	//	short          st_nlink;
	//	short          st_uid;
	//	short          st_gid;
	//	_dev_t         st_rdev;
	//	_off_t         st_size;
	//	__time64_t     st_atime;
	//	__time64_t     st_mtime;
	//	__time64_t     st_ctime;
	//};
	struct _stat64i32 buffer = {};
	int res = _stat(path, &buffer);
	if (res < 0) {
		return time_t(0);
	}
	sfz_assert(res == 0);
	return buffer.st_mtime;
}

static const char* filenameFromPath(const char* path) noexcept
{
	const char* strippedFile1 = std::strrchr(path, '\\');
	const char* strippedFile2 = std::strrchr(path, '/');
	if (strippedFile1 == nullptr && strippedFile2 == nullptr) {
		return path;
	}
	else if (strippedFile2 == nullptr) {
		return strippedFile1 + 1;
	}
	else {
		return strippedFile2 + 1;
	}
}

static bool relativeToAbsolute(char* pathOut, u32 pathOutSize, const char* pathIn) noexcept
{
	DWORD res = GetFullPathNameA(pathIn, pathOutSize, pathOut, NULL);
	return res > 0;
}

static bool fixPath(WCHAR* pathOut, u32 pathOutNumChars, const char* utf8In) noexcept
{
	char absolutePath[MAX_PATH] = { 0 };
	if (!relativeToAbsolute(absolutePath, MAX_PATH, utf8In)) return false;
	if (!utf8ToWide(pathOut, pathOutNumChars, absolutePath)) return false;
	return true;
}

// DFCC_DXIL enum constant from DxilContainer/DxilContainer.h in DirectXShaderCompiler
#define DXIL_FOURCC(ch0, ch1, ch2, ch3) ( \
	(u32)(u8)(ch0)        | (u32)(u8)(ch1) << 8  | \
	(u32)(u8)(ch2) << 16  | (u32)(u8)(ch3) << 24   \
)
static constexpr u32 DFCC_DXIL = DXIL_FOURCC('D', 'X', 'I', 'L');

static HRESULT getShaderReflection(
	ComPtr<IDxcBlob>& blob, ComPtr<ID3D12ShaderReflection>& reflectionOut) noexcept
{
	// Get and load the DxcContainerReflection
	ComPtr<IDxcContainerReflection> dxcReflection;
	HRESULT res = DxcCreateInstance(
		CLSID_DxcContainerReflection, IID_PPV_ARGS(&dxcReflection));
	if (!SUCCEEDED(res)) return res;
	res = dxcReflection->Load(blob.Get());
	if (!SUCCEEDED(res)) return res;

	// Attempt to wrangle out the ID3D12ShaderReflection from it
	u32 shaderIdx = 0;
	res = dxcReflection->FindFirstPartKind(DFCC_DXIL, &shaderIdx);
	if (!SUCCEEDED(res)) return res;
	res = dxcReflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&reflectionOut));
	if (!SUCCEEDED(res)) return res;

	// We succeded probably
	return S_OK;
}

static ZgResult dxcCreateHlslBlobFromFile(
	IDxcLibrary& dxcLibrary,
	const char* path,
	ComPtr<IDxcBlobEncoding>& blobOut) noexcept
{
	// Convert paths to absolute wide strings
	WCHAR shaderFilePathWide[MAX_PATH] = { 0 };
	if (!fixPath(shaderFilePathWide, MAX_PATH, path)) {
		return ZG_ERROR_GENERIC;
	}

	// Create an encoding blob from file
	u32 CODE_PAGE = CP_UTF8;
	if (D3D12_FAIL(dxcLibrary.CreateBlobFromFile(
		shaderFilePathWide, &CODE_PAGE, &blobOut))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	return ZG_SUCCESS;
}

static ZgResult dxcCreateHlslBlobFromSource(
	IDxcLibrary& dxcLibrary,
	const char* source,
	ComPtr<IDxcBlobEncoding>& blobOut) noexcept
{
	// Create an encoding blob from memory
	u32 CODE_PAGE = CP_UTF8;
	if (D3D12_FAIL(dxcLibrary.CreateBlobWithEncodingFromPinned(
		source, u32(std::strlen(source)), CODE_PAGE, &blobOut))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	return ZG_SUCCESS;
}

enum class ShaderType {
	VERTEX,
	PIXEL,
	COMPUTE
};

static ZgResult compileHlslShader(
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ComPtr<IDxcBlob>& blobOut,
	bool isSource,
	const char* pathOrSource,
	const char* shaderName,
	const char* entryName,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	ShaderType shaderType,
	const char* pipelineCacheDir,
	bool& wasCached) noexcept
{
	// Calculate path used to cache this shader blob
	str320 cachePath;
	if (!isSource && pipelineCacheDir != nullptr) {
		const time_t lastModified = getFileLastModifiedDate(pathOrSource);
		if (lastModified == 0) return ZG_ERROR_SHADER_COMPILE_ERROR;
		str320 hlslName = filenameFromPath(pathOrSource);
		hlslName.removeChars(5);
		cachePath.appendf("%s/%s_%s_%lli.dxil", pipelineCacheDir, hlslName.str(), entryName, lastModified);
		//ZG_INFO("%s", cachePath.str());
	}

	// Attempt to read binary from cache and exit if possible
	if (cachePath != "") {
		// Convert cache path to absolute wide string
		WCHAR cachePathWide[MAX_PATH] = { 0 };
		if (!fixPath(cachePathWide, MAX_PATH, cachePath.str())) {
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Load bin from file
		ComPtr<IDxcBlobEncoding> tmpBlob;
		if (SUCCEEDED(dxcLibrary.CreateBlobFromFile(cachePathWide, 0, &tmpBlob))) {
			blobOut = tmpBlob;
			wasCached = true;
			return ZG_SUCCESS;
		}
	}

	// Grab shader from file
	ComPtr<IDxcBlobEncoding> encodingBlob;
	if (isSource) {
		ZgResult res = dxcCreateHlslBlobFromSource(dxcLibrary, pathOrSource, encodingBlob);
		if (res != ZG_SUCCESS) return res;
	}
	else {
		ZgResult res = dxcCreateHlslBlobFromFile(dxcLibrary, pathOrSource, encodingBlob);
		if (res != ZG_SUCCESS) return res;
	}

	// Convert entry point to wide string
	WCHAR shaderEntryWide[256] = { 0 };
	if (!utf8ToWide(shaderEntryWide, 256, entryName)) {
		return ZG_ERROR_GENERIC;
	}

	// Select shader type target profile string
	LPCWSTR targetProfile = [&]() {
		switch (shaderType) {

		case ShaderType::VERTEX:
			switch (compileSettings.shaderModel) {
			case ZG_SHADER_MODEL_6_0: return L"vs_6_0";
			case ZG_SHADER_MODEL_6_1: return L"vs_6_1";
			case ZG_SHADER_MODEL_6_2: return L"vs_6_2";
			case ZG_SHADER_MODEL_6_3: return L"vs_6_3";
			case ZG_SHADER_MODEL_6_4: return L"vs_6_4";
			case ZG_SHADER_MODEL_6_5: return L"vs_6_5";
			case ZG_SHADER_MODEL_6_6: return L"vs_6_6";
			case ZG_SHADER_MODEL_UNDEFINED:
			default:
				sfz_assert_hard(false);
				return L"UNKNOWN";
			}

		case ShaderType::PIXEL:
			switch (compileSettings.shaderModel) {
			case ZG_SHADER_MODEL_6_0: return L"ps_6_0";
			case ZG_SHADER_MODEL_6_1: return L"ps_6_1";
			case ZG_SHADER_MODEL_6_2: return L"ps_6_2";
			case ZG_SHADER_MODEL_6_3: return L"ps_6_3";
			case ZG_SHADER_MODEL_6_4: return L"ps_6_4";
			case ZG_SHADER_MODEL_6_5: return L"ps_6_5";
			case ZG_SHADER_MODEL_6_6: return L"ps_6_6";
			case ZG_SHADER_MODEL_UNDEFINED:
			default:
				sfz_assert_hard(false);
				return L"UNKNOWN";
			}

		case ShaderType::COMPUTE:
			switch (compileSettings.shaderModel) {
			case ZG_SHADER_MODEL_6_0: return L"cs_6_0";
			case ZG_SHADER_MODEL_6_1: return L"cs_6_1";
			case ZG_SHADER_MODEL_6_2: return L"cs_6_2";
			case ZG_SHADER_MODEL_6_3: return L"cs_6_3";
			case ZG_SHADER_MODEL_6_4: return L"cs_6_4";
			case ZG_SHADER_MODEL_6_5: return L"cs_6_5";
			case ZG_SHADER_MODEL_6_6: return L"cs_6_6";
			case ZG_SHADER_MODEL_UNDEFINED:
			default:
				sfz_assert_hard(false);
				return L"UNKNOWN";
			}
		}
		sfz_assert_hard(false);
		return L"UNKNOWN";
	}();

	// Split and convert args to wide strings :(
	WCHAR argsContainer[ZG_MAX_NUM_DXC_COMPILER_FLAGS][32] = {};
	LPCWSTR args[ZG_MAX_NUM_DXC_COMPILER_FLAGS] = {};

	u32 numArgs = 0;
	for (u32 i = 0; i < ZG_MAX_NUM_DXC_COMPILER_FLAGS; i++) {
		if (compileSettings.dxcCompilerFlags[i] == nullptr) continue;
		utf8ToWide(argsContainer[numArgs], 32, compileSettings.dxcCompilerFlags[i]);
		args[numArgs] = argsContainer[numArgs];
		numArgs++;
	}

	// Compile shader
	ComPtr<IDxcOperationResult> result;
	if (D3D12_FAIL(dxcCompiler.Compile(
		encodingBlob.Get(),
		nullptr, // TODO: Filename
		shaderEntryWide,
		targetProfile,
		args,
		numArgs,
		nullptr,
		0,
		dxcIncludeHandler,
		&result))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	// Log compile errors/warnings
	ComPtr<IDxcBlobEncoding> errors;
	if (D3D12_FAIL(result->GetErrorBuffer(&errors))) {
		return ZG_ERROR_GENERIC;
	}
	if (errors->GetBufferSize() > 0) {
		ZG_ERROR("Shader \"%s\" compilation errors:\n%s\n",
			shaderName, (const char*)errors->GetBufferPointer());
	}

	// Check if compilation succeeded
	HRESULT compileResult = S_OK;
	result->GetStatus(&compileResult);
	if (D3D12_FAIL(compileResult)) return ZG_ERROR_SHADER_COMPILE_ERROR;

	// Pick out the compiled binary
	if (!SUCCEEDED(result->GetResult(&blobOut))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	// Attempt to write compiled binary to cache if possible
	if (cachePath != "") {
		writeBinaryFile(cachePath.str(), blobOut->GetBufferPointer(), blobOut->GetBufferSize());
	}

	return ZG_SUCCESS;
}

static D3D12_CULL_MODE toD3D12CullMode(
	const ZgRasterizerSettings& rasterizerSettings) noexcept
{
	if (rasterizerSettings.cullingEnabled == ZG_FALSE) return D3D12_CULL_MODE_NONE;
	if (rasterizerSettings.cullFrontFacing == ZG_FALSE) return D3D12_CULL_MODE_BACK;
	else return D3D12_CULL_MODE_FRONT;
}

static D3D12_COMPARISON_FUNC toD3D12ComparsionFunc(ZgComparisonFunc func) noexcept
{
	switch (func) {
	case ZG_COMPARISON_FUNC_NONE: return D3D12_COMPARISON_FUNC(0);
	case ZG_COMPARISON_FUNC_LESS: return D3D12_COMPARISON_FUNC_LESS;
	case ZG_COMPARISON_FUNC_LESS_EQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case ZG_COMPARISON_FUNC_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case ZG_COMPARISON_FUNC_NOT_EQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case ZG_COMPARISON_FUNC_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case ZG_COMPARISON_FUNC_GREATER_EQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case ZG_COMPARISON_FUNC_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	}
	sfz_assert(false);
	return D3D12_COMPARISON_FUNC(0);
}

static D3D12_BLEND_OP toD3D12BlendOp(ZgBlendFunc func) noexcept
{
	switch (func) {
	case ZG_BLEND_FUNC_ADD: return D3D12_BLEND_OP_ADD;
	case ZG_BLEND_FUNC_DST_SUB_SRC: return D3D12_BLEND_OP_SUBTRACT;
	case ZG_BLEND_FUNC_SRC_SUB_DST: return D3D12_BLEND_OP_REV_SUBTRACT;
	case ZG_BLEND_FUNC_MIN: return D3D12_BLEND_OP_MIN;
	case ZG_BLEND_FUNC_MAX: return D3D12_BLEND_OP_MAX;
	}
	sfz_assert(false);
	return D3D12_BLEND_OP_ADD;
}

static D3D12_BLEND toD3D12BlendFactor(ZgBlendFactor val) noexcept
{
	switch (val) {
	case ZG_BLEND_FACTOR_ZERO: return D3D12_BLEND_ZERO;
	case ZG_BLEND_FACTOR_ONE: return D3D12_BLEND_ONE;
	case ZG_BLEND_FACTOR_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
	case ZG_BLEND_FACTOR_SRC_INV_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
	case ZG_BLEND_FACTOR_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
	case ZG_BLEND_FACTOR_SRC_INV_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
	case ZG_BLEND_FACTOR_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
	case ZG_BLEND_FACTOR_DST_INV_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
	case ZG_BLEND_FACTOR_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
	case ZG_BLEND_FACTOR_DST_INV_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
	}
	sfz_assert(false);
	return D3D12_BLEND_ZERO;
}

static DXGI_FORMAT vertexAttributeTypeToFormat(ZgVertexAttributeType type) noexcept
{
	switch (type) {
	case ZG_VERTEX_ATTRIBUTE_F32: return DXGI_FORMAT_R32_FLOAT;
	case ZG_VERTEX_ATTRIBUTE_F32_2: return DXGI_FORMAT_R32G32_FLOAT;
	case ZG_VERTEX_ATTRIBUTE_F32_3: return DXGI_FORMAT_R32G32B32_FLOAT;
	case ZG_VERTEX_ATTRIBUTE_F32_4: return DXGI_FORMAT_R32G32B32A32_FLOAT;

	case ZG_VERTEX_ATTRIBUTE_S32: return DXGI_FORMAT_R32_SINT;
	case ZG_VERTEX_ATTRIBUTE_S32_2: return DXGI_FORMAT_R32G32_SINT;
	case ZG_VERTEX_ATTRIBUTE_S32_3: return DXGI_FORMAT_R32G32B32_SINT;
	case ZG_VERTEX_ATTRIBUTE_S32_4: return DXGI_FORMAT_R32G32B32A32_SINT;

	case ZG_VERTEX_ATTRIBUTE_U32: return DXGI_FORMAT_R32_UINT;
	case ZG_VERTEX_ATTRIBUTE_U32_2: return DXGI_FORMAT_R32G32_UINT;
	case ZG_VERTEX_ATTRIBUTE_U32_3: return DXGI_FORMAT_R32G32B32_UINT;
	case ZG_VERTEX_ATTRIBUTE_U32_4: return DXGI_FORMAT_R32G32B32A32_UINT;

	default: break;
	}
	sfz_assert(false);
	return DXGI_FORMAT_UNKNOWN;
}

static const char* vertexAttributeTypeToString(ZgVertexAttributeType type) noexcept
{
	switch (type) {
	case ZG_VERTEX_ATTRIBUTE_F32: return "ZG_VERTEX_ATTRIBUTE_F32";
	case ZG_VERTEX_ATTRIBUTE_F32_2: return "ZG_VERTEX_ATTRIBUTE_F32_2";
	case ZG_VERTEX_ATTRIBUTE_F32_3: return "ZG_VERTEX_ATTRIBUTE_F32_3";
	case ZG_VERTEX_ATTRIBUTE_F32_4: return "ZG_VERTEX_ATTRIBUTE_F32_4";

	case ZG_VERTEX_ATTRIBUTE_S32: return "ZG_VERTEX_ATTRIBUTE_S32";
	case ZG_VERTEX_ATTRIBUTE_S32_2: return "ZG_VERTEX_ATTRIBUTE_S32_2";
	case ZG_VERTEX_ATTRIBUTE_S32_3: return "ZG_VERTEX_ATTRIBUTE_S32_3";
	case ZG_VERTEX_ATTRIBUTE_S32_4: return "ZG_VERTEX_ATTRIBUTE_S32_4";

	case ZG_VERTEX_ATTRIBUTE_U32: return "ZG_VERTEX_ATTRIBUTE_U32";
	case ZG_VERTEX_ATTRIBUTE_U32_2: return "ZG_VERTEX_ATTRIBUTE_U32_2";
	case ZG_VERTEX_ATTRIBUTE_U32_3: return "ZG_VERTEX_ATTRIBUTE_U32_3";
	case ZG_VERTEX_ATTRIBUTE_U32_4: return "ZG_VERTEX_ATTRIBUTE_U32_4";

	default: break;
	}
	sfz_assert(false);
	return "";
}

static const char* bindingTypeToString(ZgBindingType type) noexcept
{
	switch (type) {
	case ZG_BINDING_TYPE_UNDEFINED: return "UNDEFINED";
	case ZG_BINDING_TYPE_BUFFER_CONST: return "BUFFER_CONST";
	case ZG_BINDING_TYPE_BUFFER_STRUCTURED: return "BUFFER_STRUCTURED";
	case ZG_BINDING_TYPE_BUFFER_STRUCTURED_UAV: return "BUFFER_STRUCTURED_UAV";
	case ZG_BINDING_TYPE_TEXTURE: return "TEXTURE";
	case ZG_BINDING_TYPE_TEXTURE_UAV: return "TEXTURE_UAV";
	default: break;
	}
	sfz_assert(false);
	return "";
}

static ZgVertexAttributeType vertexReflectionToAttribute(
	D3D_REGISTER_COMPONENT_TYPE compType, BYTE mask) noexcept
{
	sfz_assert(compType == D3D_REGISTER_COMPONENT_FLOAT32
		|| compType == D3D_REGISTER_COMPONENT_SINT32
		|| compType == D3D_REGISTER_COMPONENT_UINT32);
	sfz_assert(mask == 1 || mask == 3 || mask == 7 || mask == 15);

	if (compType == D3D_REGISTER_COMPONENT_FLOAT32) {
		if (mask == 1) return ZG_VERTEX_ATTRIBUTE_F32;
		if (mask == 3) return ZG_VERTEX_ATTRIBUTE_F32_2;
		if (mask == 7) return ZG_VERTEX_ATTRIBUTE_F32_3;
		if (mask == 15)return ZG_VERTEX_ATTRIBUTE_F32_4;
	}
	else if (compType == D3D_REGISTER_COMPONENT_SINT32) {
		if (mask == 1) return ZG_VERTEX_ATTRIBUTE_S32;
		if (mask == 3) return ZG_VERTEX_ATTRIBUTE_S32_2;
		if (mask == 7) return ZG_VERTEX_ATTRIBUTE_S32_3;
		if (mask == 15)return ZG_VERTEX_ATTRIBUTE_S32_4;
	}
	else if (compType == D3D_REGISTER_COMPONENT_UINT32) {
		if (mask == 1) return ZG_VERTEX_ATTRIBUTE_U32;
		if (mask == 3) return ZG_VERTEX_ATTRIBUTE_U32_2;
		if (mask == 7) return ZG_VERTEX_ATTRIBUTE_U32_3;
		if (mask == 15)return ZG_VERTEX_ATTRIBUTE_U32_4;
	}

	sfz_assert(false);
	return ZG_VERTEX_ATTRIBUTE_UNDEFINED;
}

static D3D12_FILTER samplingModeToD3D12(ZgSamplingMode samplingMode) noexcept
{
	switch (samplingMode) {
	case ZG_SAMPLING_MODE_NEAREST: return D3D12_FILTER_MIN_MAG_MIP_POINT;
	case ZG_SAMPLING_MODE_TRILINEAR: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	case ZG_SAMPLING_MODE_ANISOTROPIC: return D3D12_FILTER_ANISOTROPIC;
	}
	sfz_assert(false);
	return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

static D3D12_TEXTURE_ADDRESS_MODE wrappingModeToD3D12(ZgWrappingMode wrappingMode) noexcept
{
	switch (wrappingMode) {
	case ZG_WRAPPING_MODE_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case ZG_WRAPPING_MODE_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	sfz_assert(false);
	return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
}

static void logPipelineComputeInfo(
	const ZgPipelineComputeDesc& createInfo,
	const char* computeShaderName,
	const RootSignatureMapping& rootMapping,
	u32 groupDimX,
	u32 groupDimY,
	u32 groupDimZ,
	f32 compileTimeMs,
	f32 computeBlobCompileTime,
	bool computeBlobWasCached) noexcept
{
	sfz::str4096 tmpStr;

	// Print header
	tmpStr.appendf("Compiled ZgPipelineCompute with:\n");
	tmpStr.appendf(" - Compute shader: \"%s\" -- %s()\n\n",
		computeShaderName, createInfo.computeShaderEntry);

	// Print compile time
	tmpStr.appendf("Total compile time: %.2fms\n", compileTimeMs);
	tmpStr.appendf(" - Compute DXIL: %.2fms%s\n",
		computeBlobCompileTime, computeBlobWasCached ? " (cached)" : "");

	// Print group dim
	tmpStr.appendf("\nGroup dimensions: %u x %u x %u\n", groupDimX, groupDimY, groupDimZ);

	// Print push constants
	if (rootMapping.pushConsts.size() > 0) {
		tmpStr.appendf("\nPush constants (%u):\n", rootMapping.pushConsts.size());
		for (u32 i = 0; i < rootMapping.pushConsts.size(); i++) {
			const PushConstMapping& pushConst = rootMapping.pushConsts[i];
			tmpStr.appendf(" - Register: %u -- Size: %u bytes\n",
				pushConst.reg,
				pushConst.sizeBytes);
		}
	}

	// Print constant buffers
	if (rootMapping.CBVs.size() > 0) {
		tmpStr.appendf("\nConstant buffers (%u):\n", rootMapping.CBVs.size());
		for (u32 i = 0; i < rootMapping.CBVs.size(); i++) {
			const CBVMapping& cbuffer = rootMapping.CBVs[i];
			tmpStr.appendf(" - Register: %u -- Size: %u bytes\n",
				cbuffer.reg,
				cbuffer.sizeBytes);
		}
	}

	// Print SRVs
	if (rootMapping.SRVs.size() > 0) {
		tmpStr.appendf("\nSRVs (%u):\n", rootMapping.SRVs.size());
		for (u32 i = 0; i < rootMapping.SRVs.size(); i++) {
			const SRVMapping& srv = rootMapping.SRVs[i];
			tmpStr.appendf(" - Register: %u -- Type: %s\n", srv.reg, bindingTypeToString(srv.type));
		}
	}

	// Print UAVs
	if (rootMapping.UAVs.size() > 0) {
		tmpStr.appendf("\nUAVs (%u):\n", rootMapping.UAVs.size());
		for (u32 i = 0; i < rootMapping.UAVs.size(); i++) {
			const UAVMapping& uav = rootMapping.UAVs[i];
			tmpStr.appendf(" - Register: %u -- Type: %s\n", uav.reg, bindingTypeToString(uav.type));
		}
	}

	// Log
	ZG_NOISE("%s", tmpStr.str());
}

static void logPipelineRenderInfo(
	const ZgPipelineRenderDesc& createInfo,
	const char* vertexShaderName,
	const char* pixelShaderName,
	const RootSignatureMapping& rootMapping,
	const ZgPipelineRenderSignature& renderSignature,
	f32 compileTimeMs,
	f32 vertexBlobCompileTime,
	bool vertexBlobWasCached,
	f32 pixelBlobCompileTime,
	bool pixelBlobWasCached) noexcept
{
	sfz::str4096 tmpStr;

	// Print header
	tmpStr.appendf("Compiled ZgPipelineRendering with:\n");
	tmpStr.appendf(" - Vertex shader: \"%s\" -- %s()\n",
		vertexShaderName, createInfo.vertexShaderEntry);
	tmpStr.appendf(" - Pixel shader: \"%s\" -- %s()\n\n",
		pixelShaderName, createInfo.pixelShaderEntry);

	// Print compile time
	tmpStr.appendf("Total compile time: %.2fms\n", compileTimeMs);
	tmpStr.appendf(" - Vertex DXIL: %.2fms%s\n",
		vertexBlobCompileTime, vertexBlobWasCached ? " (cached)" : "");
	tmpStr.appendf(" - Pixel DXIL: %.2fms%s\n",
		pixelBlobCompileTime, pixelBlobWasCached ? " (cached)" : "");

	// Print vertex attributes
	if (renderSignature.numVertexAttributes > 0) {
		tmpStr.appendf("\nVertex attributes (%u):\n", renderSignature.numVertexAttributes);
		for (u32 i = 0; i < renderSignature.numVertexAttributes; i++) {
			const ZgVertexAttribute& attrib = renderSignature.vertexAttributes[i];
			tmpStr.appendf(" - Location: %u -- Type: %s\n",
				attrib.location,
				vertexAttributeTypeToString(attrib.type));
		}
	}
	
	// Print push constants
	if (rootMapping.pushConsts.size() > 0) {
		tmpStr.appendf("\nPush constants (%u):\n", rootMapping.pushConsts.size());
		for (u32 i = 0; i < rootMapping.pushConsts.size(); i++) {
			const PushConstMapping& pushConst = rootMapping.pushConsts[i];
			tmpStr.appendf(" - Register: %u -- Size: %u bytes\n",
				pushConst.reg,
				pushConst.sizeBytes);
		}
	}

	// Print constant buffers
	if (rootMapping.CBVs.size() > 0) {
		tmpStr.appendf("\nConstant buffers (%u):\n", rootMapping.CBVs.size());
		for (u32 i = 0; i < rootMapping.CBVs.size(); i++) {
			const CBVMapping& cbuffer = rootMapping.CBVs[i];
			tmpStr.appendf(" - Register: %u -- Size: %u bytes\n",
				cbuffer.reg,
				cbuffer.sizeBytes);
		}
	}

	// Print SRVs
	if (rootMapping.SRVs.size() > 0) {
		tmpStr.appendf("\nSRVs (%u):\n", rootMapping.SRVs.size());
		for (u32 i = 0; i < rootMapping.SRVs.size(); i++) {
			const SRVMapping& srv = rootMapping.SRVs[i];
			tmpStr.appendf(" - Register: %u -- Type: %s\n", srv.reg, bindingTypeToString(srv.type));
		}
	}

	// Print UAVs
	if (rootMapping.UAVs.size() > 0) {
		tmpStr.appendf("\nUAVs (%u):\n", rootMapping.UAVs.size());
		for (u32 i = 0; i < rootMapping.UAVs.size(); i++) {
			const UAVMapping& uav = rootMapping.UAVs[i];
			tmpStr.appendf(" - Register: %u -- Type: %s\n", uav.reg, bindingTypeToString(uav.type));
		}
	}

	// Log
	ZG_NOISE("%s", tmpStr.str());
}

static ZgResult rootSignatureMappingFromReflection(
	ID3D12ShaderReflection* refl1,
	ID3D12ShaderReflection* refl2,
	const sfz::ArrayLocal<u32, ZG_MAX_NUM_CONSTANT_BUFFERS>& pushConstRegs,
	RootSignatureMapping& mappingOut)
{
	// Get shader descriptions from reflections
	D3D12_SHADER_DESC shaderDesc1 = {};
	D3D12_SHADER_DESC shaderDesc2 = {};
	CHECK_D3D12 refl1->GetDesc(&shaderDesc1);
	if (refl2 != nullptr) CHECK_D3D12 refl2->GetDesc(&shaderDesc2);

	// Push constants and constant buffers
	{
		for (u32 i = 0; i < shaderDesc1.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
			CHECK_D3D12 refl1->GetResourceBindingDesc(i, &resDesc);
			if (resDesc.Type != D3D_SIT_CBUFFER) continue;

			sfz_assert(resDesc.Space == 0);
			sfz_assert(resDesc.BindCount == 1);
			if (resDesc.Space != 0) return ZG_ERROR_SHADER_COMPILE_ERROR;

			D3D12_SHADER_BUFFER_DESC cbufferDesc = {};
			CHECK_D3D12 refl1->GetConstantBufferByName(resDesc.Name)->GetDesc(&cbufferDesc);

			const bool isPushConst = pushConstRegs.findElement(resDesc.BindPoint) != nullptr;
			if (isPushConst) {
				sfz_assert(!mappingOut.pushConsts.isFull());
				if (mappingOut.pushConsts.isFull()) return ZG_ERROR_SHADER_COMPILE_ERROR;

				PushConstMapping& mapping = mappingOut.pushConsts.add();
				mapping.reg = resDesc.BindPoint;
				mapping.paramIdx = ~0u; // Deferred to later
				mapping.sizeBytes = cbufferDesc.Size;
			}
			else {
				sfz_assert(!mappingOut.CBVs.isFull());
				if (mappingOut.CBVs.isFull()) return ZG_ERROR_SHADER_COMPILE_ERROR;

				CBVMapping& mapping = mappingOut.CBVs.add();
				mapping.reg = resDesc.BindPoint;
				mapping.tableOffset = ~0u; // Deferred to later
				mapping.sizeBytes = cbufferDesc.Size;
			}
		}

		for (u32 i = 0; i < shaderDesc2.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
			CHECK_D3D12 refl2->GetResourceBindingDesc(i, &resDesc);
			if (resDesc.Type != D3D_SIT_CBUFFER) continue;

			sfz_assert(resDesc.Space == 0);
			sfz_assert(resDesc.BindCount == 1);
			if (resDesc.Space != 0) return ZG_ERROR_SHADER_COMPILE_ERROR;

			// Skip if already registered in previous reflection
			const bool pushConstAlreadyRegistered =  mappingOut.pushConsts
				.find([&](const PushConstMapping& m) { return m.reg == resDesc.BindPoint; }) != nullptr;
			const bool cbvAlreadyRegistered = mappingOut.CBVs
				.find([&](const CBVMapping& m) { return m.reg == resDesc.BindPoint; }) != nullptr;
			if (pushConstAlreadyRegistered || cbvAlreadyRegistered) continue;

			D3D12_SHADER_BUFFER_DESC cbufferDesc = {};
			CHECK_D3D12 refl2->GetConstantBufferByName(resDesc.Name)->GetDesc(&cbufferDesc);

			const bool isPushConst = pushConstRegs.findElement(resDesc.BindPoint) != nullptr;
			if (isPushConst) {
				sfz_assert(!mappingOut.pushConsts.isFull());
				if (mappingOut.pushConsts.isFull()) return ZG_ERROR_SHADER_COMPILE_ERROR;

				PushConstMapping& mapping = mappingOut.pushConsts.add();
				mapping.reg = resDesc.BindPoint;
				mapping.paramIdx = ~0u; // Deferred to later
				mapping.sizeBytes = cbufferDesc.Size;
			}
			else {
				sfz_assert(!mappingOut.CBVs.isFull());
				if (mappingOut.CBVs.isFull()) return ZG_ERROR_SHADER_COMPILE_ERROR;

				CBVMapping& mapping = mappingOut.CBVs.add();
				mapping.reg = resDesc.BindPoint;
				mapping.tableOffset = ~0u; // Deferred to later
				mapping.sizeBytes = cbufferDesc.Size;
			}
		}

		// Sort push constants and CBVs
		mappingOut.pushConsts.sort([](const PushConstMapping& lhs, const PushConstMapping& rhs) {
			return lhs.reg < rhs.reg;
		});
		mappingOut.CBVs.sort([](const CBVMapping& lhs, const CBVMapping& rhs) {
			return lhs.reg < rhs.reg;
		});
	}

	// SRVs
	{
		for (u32 i = 0; i < shaderDesc1.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
			CHECK_D3D12 refl1->GetResourceBindingDesc(i, &resDesc);
			if (resDesc.Type != D3D_SIT_TEXTURE &&
				resDesc.Type != D3D_SIT_STRUCTURED) continue;

			sfz_assert(resDesc.Space == 0);
			sfz_assert(resDesc.BindCount == 1);
			if (resDesc.Space != 0) return ZG_ERROR_SHADER_COMPILE_ERROR;

			sfz_assert(!mappingOut.SRVs.isFull());
			if (mappingOut.SRVs.isFull()) return ZG_ERROR_SHADER_COMPILE_ERROR;

			SRVMapping& mapping = mappingOut.SRVs.add();
			mapping.reg = resDesc.BindPoint;
			mapping.tableOffset = ~0u; // Deferred to later

			if (resDesc.Type == D3D_SIT_TEXTURE) {
				mapping.type = ZG_BINDING_TYPE_TEXTURE;
			}
			else if (resDesc.Type == D3D_SIT_STRUCTURED) {
				mapping.type = ZG_BINDING_TYPE_BUFFER_STRUCTURED;
			}
		}

		for (u32 i = 0; i < shaderDesc2.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
			CHECK_D3D12 refl2->GetResourceBindingDesc(i, &resDesc);
			if (resDesc.Type != D3D_SIT_TEXTURE &&
				resDesc.Type != D3D_SIT_STRUCTURED) continue;

			// Skip if already registered in previous reflection
			const bool srvAlreadyRegistered = mappingOut.SRVs
				.find([&](const SRVMapping& m) { return m.reg == resDesc.BindPoint; }) != nullptr;
			if (srvAlreadyRegistered) continue;

			sfz_assert(resDesc.Space == 0);
			sfz_assert(resDesc.BindCount == 1);
			if (resDesc.Space != 0) return ZG_ERROR_SHADER_COMPILE_ERROR;

			sfz_assert(!mappingOut.SRVs.isFull());
			if (mappingOut.SRVs.isFull()) return ZG_ERROR_SHADER_COMPILE_ERROR;

			SRVMapping& mapping = mappingOut.SRVs.add();
			mapping.reg = resDesc.BindPoint;
			mapping.tableOffset = ~0u;

			if (resDesc.Type == D3D_SIT_TEXTURE) {
				mapping.type = ZG_BINDING_TYPE_TEXTURE;
			}
			else if (resDesc.Type == D3D_SIT_STRUCTURED) {
				mapping.type = ZG_BINDING_TYPE_BUFFER_STRUCTURED;
			}
		}

		// Sort SRVs
		mappingOut.SRVs.sort([](const SRVMapping& lhs, const SRVMapping& rhs) {
			return lhs.reg < rhs.reg;
		});
	}

	// UAVs
	{
		for (u32 i = 0; i < shaderDesc1.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
			CHECK_D3D12 refl1->GetResourceBindingDesc(i, &resDesc);
			if (resDesc.Type != D3D_SIT_UAV_RWTYPED &&
				resDesc.Type != D3D11_SIT_UAV_RWSTRUCTURED) continue;

			sfz_assert(resDesc.Space == 0);
			sfz_assert(resDesc.BindCount == 1);
			if (resDesc.Space != 0) return ZG_ERROR_SHADER_COMPILE_ERROR;

			sfz_assert(!mappingOut.UAVs.isFull());
			if (mappingOut.UAVs.isFull()) return ZG_ERROR_SHADER_COMPILE_ERROR;

			UAVMapping& mapping = mappingOut.UAVs.add();
			mapping.reg = resDesc.BindPoint;
			mapping.tableOffset = ~0u; // Deferred to later

			if (resDesc.Type == D3D_SIT_UAV_RWTYPED) {
				mapping.type = ZG_BINDING_TYPE_TEXTURE_UAV;
			}
			else if (resDesc.Type == D3D11_SIT_UAV_RWSTRUCTURED) {
				mapping.type = ZG_BINDING_TYPE_BUFFER_STRUCTURED_UAV;
			}
		}

		for (u32 i = 0; i < shaderDesc2.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
			CHECK_D3D12 refl2->GetResourceBindingDesc(i, &resDesc);
			if (resDesc.Type != D3D_SIT_UAV_RWTYPED &&
				resDesc.Type != D3D11_SIT_UAV_RWSTRUCTURED) continue;

			// Skip if already registered in previous reflection
			const bool uavAlreadyRegistered = mappingOut.UAVs
				.find([&](const UAVMapping& m) { return m.reg == resDesc.BindPoint; }) != nullptr;
			if (uavAlreadyRegistered) continue;

			sfz_assert(resDesc.Space == 0);
			sfz_assert(resDesc.BindCount == 1);
			if (resDesc.Space != 0) return ZG_ERROR_SHADER_COMPILE_ERROR;

			sfz_assert(!mappingOut.UAVs.isFull());
			if (mappingOut.UAVs.isFull()) return ZG_ERROR_SHADER_COMPILE_ERROR;

			UAVMapping& mapping = mappingOut.UAVs.add();
			mapping.reg = resDesc.BindPoint;
			mapping.tableOffset = ~0u; // Deferred to later

			if (resDesc.Type == D3D_SIT_UAV_RWTYPED) {
				mapping.type = ZG_BINDING_TYPE_TEXTURE_UAV;
			}
			else if (resDesc.Type == D3D11_SIT_UAV_RWSTRUCTURED) {
				mapping.type = ZG_BINDING_TYPE_BUFFER_STRUCTURED_UAV;
			}
		}

		// Sort UAVs
		mappingOut.UAVs.sort([](const UAVMapping& lhs, const UAVMapping& rhs) {
			return lhs.reg < rhs.reg;
		});
	}

	return ZG_SUCCESS;
}

static ZgResult createRootSignature(
	RootSignatureMapping& mapping,
	ComPtr<ID3D12RootSignature>& rootSignatureOut,
	const ArrayLocal<ZgSampler, ZG_MAX_NUM_SAMPLERS>& zgSamplers,
	ID3D12Device3& device)
{
	// Root signature parameters
	// We know that we can't have more than 64 root parameters as maximum (i.e. 64 words)
	constexpr u32 MAX_NUM_ROOT_PARAMETERS = 64;
	ArrayLocal<CD3DX12_ROOT_PARAMETER1, MAX_NUM_ROOT_PARAMETERS> params;

	// Add push constants
	for (PushConstMapping& push : mapping.pushConsts) {
		push.paramIdx = params.size();

		CD3DX12_ROOT_PARAMETER1& param = params.add();
		sfz_assert((push.sizeBytes % 4) == 0);
		sfz_assert(push.sizeBytes <= 1024);
		param.InitAsConstants(push.sizeBytes / 4, push.reg, 0, D3D12_SHADER_VISIBILITY_ALL);
		sfz_assert(!params.isFull());
	}

	// The offset into the dynamic table
	constexpr u32 MAX_NUM_RANGES = 3; // CBVs, UAVs and SRVs
	ArrayLocal<CD3DX12_DESCRIPTOR_RANGE1, MAX_NUM_RANGES> ranges;
	u32 currentTableOffset = 0;

	// CBVs
	// TODO: We currently assume that the CBVs are in a continuous range, i.e. not intermixed with
	//       push constants.
	if (!mapping.CBVs.isEmpty()) {
		for (CBVMapping& cbv : mapping.CBVs) {
			cbv.tableOffset = currentTableOffset;
			currentTableOffset += 1;
		}
		CD3DX12_DESCRIPTOR_RANGE1& rangeCBV = ranges.add();
		rangeCBV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
			mapping.CBVs.size(), mapping.CBVs[0].reg);
	}

	// UAVs
	// TODO: Assuming all UAVs are in a continuous range.
	if (!mapping.UAVs.isEmpty()) {
		for (UAVMapping& uav : mapping.UAVs) {
			uav.tableOffset = currentTableOffset;
			currentTableOffset += 1;
		}
		CD3DX12_DESCRIPTOR_RANGE1& rangeUAV = ranges.add();
		rangeUAV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
			mapping.UAVs.size(), mapping.UAVs[0].reg);
	}

	// SRVs
	// TODO: Assuming all SRVs are in a continuous range.
	if (!mapping.SRVs.isEmpty()) {
		for (SRVMapping& srv : mapping.SRVs) {
			srv.tableOffset = currentTableOffset;
			currentTableOffset += 1;
		}
		CD3DX12_DESCRIPTOR_RANGE1& rangeSRV = ranges.add();
		rangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
			mapping.SRVs.size(), mapping.SRVs[0].reg);
	}

	mapping.dynamicTableSize = currentTableOffset;

	// Add dynamic table parameter if we need to
	if (!ranges.isEmpty()) {
		
		// Store parameter index of dynamic table
		sfz_assert(!params.isFull());
		mapping.dynamicParamIdx = params.size();

		// Create dynamic table parameter
		CD3DX12_ROOT_PARAMETER1& param = params.add();
		param.InitAsDescriptorTable(ranges.size(), ranges.data());
	}

	// Add static samplers
	ArrayLocal<D3D12_STATIC_SAMPLER_DESC, ZG_MAX_NUM_SAMPLERS> samplerDescs;
	for (const ZgSampler& zgSampler : zgSamplers) {
		D3D12_STATIC_SAMPLER_DESC& samplerDesc = samplerDescs.add();
		samplerDesc.Filter = samplingModeToD3D12(zgSampler.samplingMode);
		samplerDesc.AddressU = wrappingModeToD3D12(zgSampler.wrappingModeU);
		samplerDesc.AddressV = wrappingModeToD3D12(zgSampler.wrappingModeV);
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.MipLODBias = zgSampler.mipLodBias;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = toD3D12ComparsionFunc(zgSampler.comparisonFunc);
		if (samplerDesc.ComparisonFunc != 0) {
			if (zgSampler.samplingMode == ZG_SAMPLING_MODE_NEAREST) {
				samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
			}
			else if (zgSampler.samplingMode == ZG_SAMPLING_MODE_TRILINEAR) {
				samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
			}
			else {
				ZG_ERROR("Sampler has a comparison function set, but sampling mode is not nearest or trilinear.");
				return ZG_ERROR_SHADER_COMPILE_ERROR;
			}
		}
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.ShaderRegister = samplerDescs.size() - 1;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // TODO: Check this from reflection
	}

	// Allow root signature access from all shader stages, opt in to using an input layout
	D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc;
	rootDesc.Init_1_1(params.size(), params.data(), samplerDescs.size(), samplerDescs.data(), flags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> errorBlob;
	if (D3D12_FAIL(D3DX12SerializeVersionedRootSignature(
		&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &blob, &errorBlob))) {

		ZG_ERROR("D3DX12SerializeVersionedRootSignature() failed: %s\n",
			(const char*)errorBlob->GetBufferPointer());
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	// Create root signature
	if (D3D12_FAIL(device.CreateRootSignature(
		0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignatureOut)))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	return ZG_SUCCESS;
}

// D3D12PipelineCompute functions
// ------------------------------------------------------------------------------------------------

static ZgResult createPipelineComputeInternal(
	ZgPipelineCompute** pipelineOut,
	const ZgPipelineComputeDesc& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	bool isSource,
	const char* pathOrSource,
	const char* computeShaderName,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device,
	const char* pipelineCacheDir) noexcept
{
	// Start measuring compile-time
	time_point compileStartTime = std::chrono::high_resolution_clock::now();

	// Compile compute shader
	ComPtr<IDxcBlob> computeBlob;
	bool computeBlobWasCached = false;
	ZgResult computeShaderRes = compileHlslShader(
		dxcLibrary,
		dxcCompiler,
		dxcIncludeHandler,
		computeBlob,
		isSource,
		pathOrSource,
		computeShaderName,
		createInfo.computeShaderEntry,
		compileSettings,
		ShaderType::COMPUTE,
		pipelineCacheDir,
		computeBlobWasCached);
	if (computeShaderRes != ZG_SUCCESS) return computeShaderRes;
	const f32 computeBlobCompileTimeMs = timeSinceLastTimestampMillis(compileStartTime);

	// Attempt to get reflection data
	ComPtr<ID3D12ShaderReflection> computeReflection;
	if (D3D12_FAIL(getShaderReflection(computeBlob, computeReflection))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	// Get shader description froms reflection data
	D3D12_SHADER_DESC computeDesc = {};
	CHECK_D3D12 computeReflection->GetDesc(&computeDesc);

	// Get root signature mapping from reflection
	RootSignatureMapping mapping = {};
	{
		sfz::ArrayLocal<u32, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstRegs;
		pushConstRegs.add(createInfo.pushConstantRegisters, createInfo.numPushConstants);

		ZgResult res = rootSignatureMappingFromReflection(
			computeReflection.Get(),
			nullptr,
			pushConstRegs,
			mapping);
		if (res != ZG_SUCCESS) return res;
	}

	// Create root signature
	ComPtr<ID3D12RootSignature> rootSignature;
	{
		ArrayLocal<ZgSampler, ZG_MAX_NUM_SAMPLERS> samplers;
		samplers.add(createInfo.samplers, createInfo.numSamplers);
		ZgResult res = createRootSignature(mapping, rootSignature, samplers, device);
		if (res != ZG_SUCCESS) return res;
	}

	// Create Pipeline State Object (PSO)
	ComPtr<ID3D12PipelineState> pipelineState;
	{
		// Essentially tokens are sent to Device->CreatePipelineState(), it does not matter
		// what order the tokens are sent in. For this reason we create our own struct with
		// the tokens we care about.
		struct PipelineStateStream {
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS computeShader;
		};

		// Create our token stream and set root signature
		PipelineStateStream stream = {};
		stream.rootSignature = rootSignature.Get();

		// Set compute shader
		stream.computeShader = CD3DX12_SHADER_BYTECODE(
			computeBlob->GetBufferPointer(), computeBlob->GetBufferSize());

		// Create pipeline state
		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
		streamDesc.pPipelineStateSubobjectStream = &stream;
		streamDesc.SizeInBytes = sizeof(PipelineStateStream);
		if (D3D12_FAIL(device.CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState)))) {
			return ZG_ERROR_GENERIC;
		}
	}

	// Get thread group dimensions of the compute pipeline
	u32 groupDimX = 0;
	u32 groupDimY = 0;
	u32 groupDimZ = 0;
	computeReflection->GetThreadGroupSize(&groupDimX, &groupDimY, &groupDimZ);
	sfz_assert(groupDimX != 0);
	sfz_assert(groupDimY != 0);
	sfz_assert(groupDimZ != 0);

	// Log information about the pipeline
	f32 compileTimeMs = timeSinceLastTimestampMillis(compileStartTime);
	logPipelineComputeInfo(
		createInfo,
		computeShaderName,
		mapping,
		groupDimX,
		groupDimY,
		groupDimZ,
		compileTimeMs,
		computeBlobCompileTimeMs,
		computeBlobWasCached);

	// Allocate pipeline
	ZgPipelineCompute* pipeline = sfz_new<ZgPipelineCompute>(getAllocator(), sfz_dbg("ZgPipelineCompute"));

	// Store pipeline state
	pipeline->pipelineState = pipelineState;
	pipeline->rootSignature = rootSignature;
	pipeline->mapping = mapping;
	pipeline->groupDimX = groupDimX;
	pipeline->groupDimY = groupDimY;
	pipeline->groupDimZ = groupDimZ;

	// Return pipeline
	*pipelineOut = pipeline;
	return ZG_SUCCESS;
}

ZgResult createPipelineComputeFileHLSL(
	ZgPipelineCompute** pipelineOut,
	const ZgPipelineComputeDesc& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device,
	const char* pipelineCacheDir) noexcept
{
	return createPipelineComputeInternal(
		pipelineOut,
		createInfo,
		compileSettings,
		false,
		createInfo.computeShader,
		createInfo.computeShader,
		dxcLibrary,
		dxcCompiler,
		dxcIncludeHandler,
		device,
		pipelineCacheDir);
}

// D3D12PipelineRender functions
// ------------------------------------------------------------------------------------------------

static ZgResult createPipelineRenderInternal(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderDesc& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	bool isSource,
	const char* vertexPathOrSource,
	const char* pixelPathOrSource,
	const char* vertexShaderName,
	const char* pixelShaderName,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device,
	const char* pipelineCacheDir) noexcept
{
	// Start measuring compile-time
	const time_point compileStartTime = std::chrono::high_resolution_clock::now();

	// Compile vertex shader
	ComPtr<IDxcBlob> vertexBlob;
	bool vertexBlobWasCached = false;
	ZgResult vertexShaderRes = compileHlslShader(
		dxcLibrary,
		dxcCompiler,
		dxcIncludeHandler,
		vertexBlob,
		isSource,
		vertexPathOrSource,
		vertexShaderName,
		createInfo.vertexShaderEntry,
		compileSettings,
		ShaderType::VERTEX,
		pipelineCacheDir,
		vertexBlobWasCached);
	if (vertexShaderRes != ZG_SUCCESS) return vertexShaderRes;
	const f32 vertexBlobCompileTimeMs = timeSinceLastTimestampMillis(compileStartTime);

	// Vertex reflection
	ComPtr<ID3D12ShaderReflection> vertexReflection;
	if (D3D12_FAIL(getShaderReflection(vertexBlob, vertexReflection))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	// Compile pixel shader
	const time_point pixelCompileStartTime = std::chrono::high_resolution_clock::now();
	ComPtr<IDxcBlob> pixelBlob;
	bool pixelBlobWasCached = false;
	ZgResult pixelShaderRes = compileHlslShader(
		dxcLibrary,
		dxcCompiler,
		dxcIncludeHandler,
		pixelBlob,
		isSource,
		pixelPathOrSource,
		pixelShaderName,
		createInfo.pixelShaderEntry,
		compileSettings,
		ShaderType::PIXEL,
		pipelineCacheDir,
		pixelBlobWasCached);
	if (pixelShaderRes != ZG_SUCCESS) return pixelShaderRes;
	const f32 pixelBlobCompileTimeMs = timeSinceLastTimestampMillis(pixelCompileStartTime);

	// Pixel reflection
	ComPtr<ID3D12ShaderReflection> pixelReflection;
	if (D3D12_FAIL(getShaderReflection(pixelBlob, pixelReflection))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	// Get root signature mapping from reflection
	RootSignatureMapping mapping = {};
	{
		sfz::ArrayLocal<u32, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstRegs;
		pushConstRegs.add(createInfo.pushConstantRegisters, createInfo.numPushConstants);

		ZgResult res = rootSignatureMappingFromReflection(
			vertexReflection.Get(),
			pixelReflection.Get(),
			pushConstRegs,
			mapping);
		if (res != ZG_SUCCESS) return res;
	}

	// Get shader description froms reflection data
	D3D12_SHADER_DESC vertexDesc = {};
	CHECK_D3D12 vertexReflection->GetDesc(&vertexDesc);
	D3D12_SHADER_DESC pixelDesc = {};
	CHECK_D3D12 pixelReflection->GetDesc(&pixelDesc);

	ZgPipelineRenderSignature renderSignature = {};
	renderSignature.numVertexAttributes = createInfo.numVertexAttributes;

	// Validate vertex attributes
	for (u32 i = 0; i < createInfo.numVertexAttributes; i++) {

		const ZgVertexAttribute& attrib = createInfo.vertexAttributes[i];

		// Get signature for the i:th vertex attribute
		D3D12_SIGNATURE_PARAMETER_DESC sign = {};
		CHECK_D3D12 vertexReflection->GetInputParameterDesc(i, &sign);

		// Get the type found in the shader
		ZgVertexAttributeType reflectedType =
			vertexReflectionToAttribute(sign.ComponentType, sign.Mask);

		// Check that the reflected type is the same as the specified type
		if (reflectedType != attrib.type) {
			ZG_ERROR("Invalid ZgPipelineRenderingCreateInfo. It specifies that the %u:th"
				" vertex attribute is of type %s, shader reflection finds %s",
				i,
				vertexAttributeTypeToString(attrib.type),
				vertexAttributeTypeToString(reflectedType));
			return ZG_ERROR_INVALID_ARGUMENT;
		}

		// Check that the attribute location (semantic index) is the same
		if (sign.SemanticIndex != createInfo.vertexAttributes[i].location) {
			ZG_ERROR("Invalid ZgPipelineRenderingCreateInfo. It specifies that the %u:th"
				" vertex attribute has location %u, shader reflection finds %u",
				i,
				attrib.location,
				sign.SemanticIndex);
			return ZG_ERROR_INVALID_ARGUMENT;
		}

		// Set vertex attribute in signature
		renderSignature.vertexAttributes[i] = attrib;
	}

	// Check that the correct number of render targets is specified
	u32 numRenderTargets = pixelDesc.OutputParameters;
	if (numRenderTargets != createInfo.numRenderTargets) {

		bool hasDepthOutput = false;
		for (u32 i = 0; i < numRenderTargets; i++) {
			D3D12_SIGNATURE_PARAMETER_DESC outDesc = {};
			CHECK_D3D12 pixelReflection->GetOutputParameterDesc(i, &outDesc);
			sfz::str32 semanticName = sfz::str32("%s", outDesc.SemanticName);
			semanticName.toLower();
			if (semanticName == "sv_depth") {
				hasDepthOutput = true;
				break;
			}
		}

		if (hasDepthOutput) {
			numRenderTargets -= 1;
		}

		if (numRenderTargets != createInfo.numRenderTargets) {
			ZG_ERROR("%u render targets were specified, however %u is used by the pipeline",
				createInfo.numRenderTargets, numRenderTargets);
			return ZG_ERROR_INVALID_ARGUMENT;
		}
	}

	// Copy render target info to signature
	renderSignature.numRenderTargets = numRenderTargets;
	for (u32 i = 0; i < numRenderTargets; i++) {
		renderSignature.renderTargets[i] = createInfo.renderTargets[i];
	}

	// Convert ZgVertexAttribute's to D3D12_INPUT_ELEMENT_DESC
	// This is the "input layout"
	ArrayLocal<D3D12_INPUT_ELEMENT_DESC, ZG_MAX_NUM_VERTEX_ATTRIBUTES> attributes;
	for (u32 i = 0; i < createInfo.numVertexAttributes; i++) {

		const ZgVertexAttribute& attribute = createInfo.vertexAttributes[i];
		D3D12_INPUT_ELEMENT_DESC desc = {};
		desc.SemanticName = "TEXCOORD";
		desc.SemanticIndex = attribute.location;
		desc.Format = vertexAttributeTypeToFormat(attribute.type);
		desc.InputSlot = attribute.vertexBufferSlot;
		desc.AlignedByteOffset = attribute.offsetToFirstElementInBytes;
		desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		desc.InstanceDataStepRate = 0;
		attributes.add(desc);
	}

	// Create root signature
	ComPtr<ID3D12RootSignature> rootSignature;
	{
		ArrayLocal<ZgSampler, ZG_MAX_NUM_SAMPLERS> samplers;
		samplers.add(createInfo.samplers, createInfo.numSamplers);
		ZgResult res = createRootSignature(mapping, rootSignature, samplers, device);
		if (res != ZG_SUCCESS) return res;
	}

	// Create Pipeline State Object (PSO)
	ComPtr<ID3D12PipelineState> pipelineState;
	{
		// Essentially tokens are sent to Device->CreatePipelineState(), it does not matter
		// what order the tokens are sent in. For this reason we create our own struct with
		// the tokens we care about.
		struct PipelineStateStreamNoInputLayout {
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vertexShader;
			CD3DX12_PIPELINE_STATE_STREAM_PS pixelShader;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rasterizer;
			CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blending;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 depthStencil;
		};
		
		struct PipelineStateStream {
			PipelineStateStreamNoInputLayout stream;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
		};

		// Create our token stream and set root signature
		PipelineStateStreamNoInputLayout stream = {};
		stream.rootSignature = rootSignature.Get();

		// Set primitive topology
		// We only allow triangles for now
		stream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// Set vertex shader
		stream.vertexShader = CD3DX12_SHADER_BYTECODE(
			vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize());

		// Set pixel shader
		stream.pixelShader = CD3DX12_SHADER_BYTECODE(
			pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize());

		// Set render target formats
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = renderSignature.numRenderTargets;
		for (u32 i = 0; i < renderSignature.numRenderTargets; i++) {
			rtvFormats.RTFormats[i] = zgToDxgiTextureFormat(renderSignature.renderTargets[i]);
		}
		stream.rtvFormats = rtvFormats;

		// Set depth buffer formats
		// TODO: Allow other depth formats? Stencil buffers?
		stream.dsvFormat = DXGI_FORMAT_D32_FLOAT;

		// Set rasterizer state
		D3D12_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = (createInfo.rasterizer.wireframeMode == ZG_FALSE) ?
			D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;
		rasterizerDesc.CullMode = toD3D12CullMode(createInfo.rasterizer);
		rasterizerDesc.FrontCounterClockwise =
			(createInfo.rasterizer.frontFacingIsCounterClockwise == ZG_FALSE) ? FALSE : TRUE;
		rasterizerDesc.DepthBias = createInfo.rasterizer.depthBias;
		rasterizerDesc.DepthBiasClamp = createInfo.rasterizer.depthBiasClamp;
		rasterizerDesc.SlopeScaledDepthBias = createInfo.rasterizer.depthBiasSlopeScaled;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		rasterizerDesc.ForcedSampleCount = 0;
		rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		stream.rasterizer = CD3DX12_RASTERIZER_DESC(rasterizerDesc);

		// Set blending state
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable =
			(createInfo.blending.blendingEnabled == ZG_FALSE) ? FALSE : TRUE;
		blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
		blendDesc.RenderTarget[0].SrcBlend = toD3D12BlendFactor (createInfo.blending.srcValColor);
		blendDesc.RenderTarget[0].DestBlend = toD3D12BlendFactor(createInfo.blending.dstValColor);
		blendDesc.RenderTarget[0].BlendOp = toD3D12BlendOp(createInfo.blending.blendFuncColor);
		blendDesc.RenderTarget[0].SrcBlendAlpha = toD3D12BlendFactor (createInfo.blending.srcValAlpha);
		blendDesc.RenderTarget[0].DestBlendAlpha = toD3D12BlendFactor(createInfo.blending.dstValAlpha);
		blendDesc.RenderTarget[0].BlendOpAlpha = toD3D12BlendOp(createInfo.blending.blendFuncAlpha);
		blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		stream.blending = CD3DX12_BLEND_DESC(blendDesc);

		// Set depth and stencil state
		D3D12_DEPTH_STENCIL_DESC1 depthStencilDesc = {};
		depthStencilDesc.DepthEnable =
			(createInfo.depthFunc != ZG_COMPARISON_FUNC_NONE) ? TRUE : FALSE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = toD3D12ComparsionFunc(createInfo.depthFunc);
		depthStencilDesc.StencilEnable = FALSE;
		depthStencilDesc.DepthBoundsTestEnable = FALSE;
		stream.depthStencil = CD3DX12_DEPTH_STENCIL_DESC1(depthStencilDesc);

		// Create pipeline state, different paths depending on if there is an input layout or not.
		if (!attributes.isEmpty()) {
			PipelineStateStream inputLayoutStream = {};
			inputLayoutStream.stream = stream;

			D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
			inputLayoutDesc.pInputElementDescs = attributes.data();
			inputLayoutDesc.NumElements = attributes.size();
			inputLayoutStream.inputLayout = inputLayoutDesc;

			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
			streamDesc.pPipelineStateSubobjectStream = &inputLayoutStream;
			streamDesc.SizeInBytes = sizeof(PipelineStateStream);
			if (D3D12_FAIL(device.CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState)))) {
				return ZG_ERROR_GENERIC;
			}
		}
		else {
			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
			streamDesc.pPipelineStateSubobjectStream = &stream;
			streamDesc.SizeInBytes = sizeof(PipelineStateStreamNoInputLayout);
			if (D3D12_FAIL(device.CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState)))) {
				return ZG_ERROR_GENERIC;
			}
		}
	}

	// Log information about the pipeline
	f32 compileTimeMs = timeSinceLastTimestampMillis(compileStartTime);
	logPipelineRenderInfo(
		createInfo,
		vertexShaderName,
		pixelShaderName,
		mapping,
		renderSignature,
		compileTimeMs,
		vertexBlobCompileTimeMs,
		vertexBlobWasCached,
		pixelBlobCompileTimeMs,
		pixelBlobWasCached);

	// Allocate pipeline
	ZgPipelineRender* pipeline =
		sfz_new<ZgPipelineRender>(getAllocator(), sfz_dbg("ZgPipelineRender"));

	// Store pipeline state
	pipeline->pipelineState = pipelineState;
	pipeline->rootSignature = rootSignature;
	pipeline->mapping = mapping;
	pipeline->renderSignature = renderSignature;
	pipeline->createInfo = createInfo;

	// Return pipeline
	*pipelineOut = pipeline;
	return ZG_SUCCESS;
}

ZgResult createPipelineRenderFileHLSL(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderDesc& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device,
	const char* pipelineCacheDir) noexcept
{
	return createPipelineRenderInternal(
		pipelineOut,
		createInfo,
		compileSettings,
		false,
		createInfo.vertexShader,
		createInfo.pixelShader,
		createInfo.vertexShader,
		createInfo.pixelShader,
		dxcLibrary,
		dxcCompiler,
		dxcIncludeHandler,
		device,
		pipelineCacheDir);
}

ZgResult createPipelineRenderSourceHLSL(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderDesc& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device,
	const char* pipelineCacheDir) noexcept
{
	return createPipelineRenderInternal(
		pipelineOut,
		createInfo,
		compileSettings,
		true,
		createInfo.vertexShader,
		createInfo.pixelShader,
		"<From source, no vertex name>",
		"<From source, no pixel name>",
		dxcLibrary,
		dxcCompiler,
		dxcIncludeHandler,
		device,
		pipelineCacheDir);
}
