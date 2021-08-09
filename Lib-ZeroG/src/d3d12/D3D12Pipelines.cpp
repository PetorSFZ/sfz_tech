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

static f32 calculateDeltaMillis(time_point& previousTime) noexcept
{
	time_point currentTime = std::chrono::high_resolution_clock::now();

	using FloatMS = std::chrono::duration<f32, std::milli>;
	f32 delta = std::chrono::duration_cast<FloatMS>(currentTime - previousTime).count();

	previousTime = currentTime;
	return delta;
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
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ComPtr<IDxcBlob>& blobOut,
	ComPtr<ID3D12ShaderReflection>& reflectionOut,
	const ComPtr<IDxcBlobEncoding>& encodingBlob,
	const char* shaderName,
	const char* entryName,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	ShaderType shaderType) noexcept
{
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

	// Attempt to get reflection data
	if (D3D12_FAIL(getShaderReflection(blobOut, reflectionOut))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
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
	const ZgPipelineComputeCreateInfo& createInfo,
	const char* computeShaderName,
	const ZgPipelineBindingsSignature& bindingsSignature,
	u32 groupDimX,
	u32 groupDimY,
	u32 groupDimZ,
	f32 compileTimeMs) noexcept
{
	sfz::str4096 tmpStr;

	// Print header
	tmpStr.appendf("Compiled ZgPipelineCompute with:\n");
	tmpStr.appendf(" - Compute shader: \"%s\" -- %s()\n\n",
		computeShaderName, createInfo.computeShaderEntry);

	// Print compile time
	tmpStr.appendf("Compile time: %.2fms\n", compileTimeMs);

	// Print group dim
	tmpStr.appendf("\nGroup dimensions: %u x %u x %u\n", groupDimX, groupDimY, groupDimZ);

	// Print constant buffers
	if (bindingsSignature.numConstBuffers > 0) {
		tmpStr.appendf("\nConstant buffers (%u):\n", bindingsSignature.numConstBuffers);
		for (u32 i = 0; i < bindingsSignature.numConstBuffers; i++) {
			const ZgConstantBufferBindingDesc& cbuffer = bindingsSignature.constBuffers[i];
			tmpStr.appendf(" - Register: %u -- Size: %u bytes -- Push constant: %s\n",
				cbuffer.bufferRegister,
				cbuffer.sizeInBytes,
				cbuffer.pushConstant ? "YES" : "NO");
		}
	}

	// Print unordered buffers
	if (bindingsSignature.numUnorderedBuffers > 0) {
		tmpStr.appendf("\nUnordered buffers (%u):\n", bindingsSignature.numUnorderedBuffers);
		for (u32 i = 0; i < bindingsSignature.numUnorderedBuffers; i++) {
			const ZgUnorderedBufferBindingDesc& buffer = bindingsSignature.unorderedBuffers[i];
			tmpStr.appendf(" - Register: %u\n", buffer.unorderedRegister);
		}
	}

	// Print textures
	if (bindingsSignature.numTextures > 0) {
		tmpStr.appendf("\nTextures (%u):\n", bindingsSignature.numTextures);
		for (u32 i = 0; i < bindingsSignature.numTextures; i++) {
			const ZgTextureBindingDesc& texture = bindingsSignature.textures[i];
			tmpStr.appendf(" - Register: %u\n", texture.textureRegister);
		}
	}

	// Print unordered textures
	if (bindingsSignature.numUnorderedTextures > 0) {
		tmpStr.appendf("\nUnordered textures (%u):\n", bindingsSignature.numUnorderedTextures);
		for (u32 i = 0; i < bindingsSignature.numUnorderedTextures; i++) {
			const ZgUnorderedTextureBindingDesc& texture = bindingsSignature.unorderedTextures[i];
			tmpStr.appendf(" - Register: %u\n", texture.unorderedRegister);
		}
	}

	// Log
	ZG_NOISE("%s", tmpStr.str());
}

static void logPipelineRenderInfo(
	const ZgPipelineRenderCreateInfo& createInfo,
	const char* vertexShaderName,
	const char* pixelShaderName,
	const ZgPipelineBindingsSignature& bindingsSignature,
	const ZgPipelineRenderSignature& renderSignature,
	f32 compileTimeMs) noexcept
{
	sfz::str4096 tmpStr;

	// Print header
	tmpStr.appendf("Compiled ZgPipelineRendering with:\n");
	tmpStr.appendf(" - Vertex shader: \"%s\" -- %s()\n",
		vertexShaderName, createInfo.vertexShaderEntry);
	tmpStr.appendf(" - Pixel shader: \"%s\" -- %s()\n\n",
		pixelShaderName, createInfo.pixelShaderEntry);

	// Print compile time
	tmpStr.appendf("Compile time: %.2fms\n", compileTimeMs);

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
	
	// Print constant buffers
	if (bindingsSignature.numConstBuffers > 0) {
		tmpStr.appendf("\nConstant buffers (%u):\n", bindingsSignature.numConstBuffers);
		for (u32 i = 0; i < bindingsSignature.numConstBuffers; i++) {
			const ZgConstantBufferBindingDesc& cbuffer = bindingsSignature.constBuffers[i];
			tmpStr.appendf(" - Register: %u -- Size: %u bytes -- Push constant: %s\n",
				cbuffer.bufferRegister,
				cbuffer.sizeInBytes,
				cbuffer.pushConstant ? "YES" : "NO");
		}
	}
	
	// Print unordered buffers
	if (bindingsSignature.numUnorderedBuffers > 0) {
		tmpStr.appendf("\nUnordered buffers (%u):\n", bindingsSignature.numUnorderedBuffers);
		for (u32 i = 0; i < bindingsSignature.numUnorderedBuffers; i++) {
			const ZgUnorderedBufferBindingDesc& buffer = bindingsSignature.unorderedBuffers[i];
			tmpStr.appendf(" - Register: %u\n", buffer.unorderedRegister);
		}
	}

	// Print textures
	if (bindingsSignature.numTextures > 0) {
		tmpStr.appendf("\nTextures (%u):\n", bindingsSignature.numTextures);
		for (u32 i = 0; i < bindingsSignature.numTextures; i++) {
			const ZgTextureBindingDesc& texture = bindingsSignature.textures[i];
			tmpStr.appendf(" - Register: %u\n", texture.textureRegister);
		}
	}

	// Print unordered textures
	if (bindingsSignature.numUnorderedTextures > 0) {
		tmpStr.appendf("\nUnordered textures (%u):\n", bindingsSignature.numUnorderedTextures);
		for (u32 i = 0; i < bindingsSignature.numUnorderedTextures; i++) {
			const ZgUnorderedTextureBindingDesc& texture = bindingsSignature.unorderedTextures[i];
			tmpStr.appendf(" - Register: %u\n", texture.unorderedRegister);
		}
	}

	// Log
	ZG_NOISE("%s", tmpStr.str());
}

static ZgResult bindingsFromReflection(
	const ComPtr<ID3D12ShaderReflection>& reflection,
	const sfz::ArrayLocal<u32, ZG_MAX_NUM_CONSTANT_BUFFERS>& pushConstantRegisters,
	D3D12PipelineBindingsSignature& bindingsOut) noexcept
{
	// Get shader description from reflection
	D3D12_SHADER_DESC shaderDesc = {};
	CHECK_D3D12 reflection->GetDesc(&shaderDesc);

	// Go through all bound resources
	for (u32 i = 0; i < shaderDesc.BoundResources; i++) {

		// Get resource desc
		D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
		CHECK_D3D12 reflection->GetResourceBindingDesc(i, &resDesc);
		
		// Error out if another register space than 0 is used
		if (resDesc.Space != 0) {
			ZG_ERROR("Shader resource %s (register = %u) uses register space %u, only 0 is allowed",
				resDesc.Name, resDesc.BindPoint, resDesc.Space);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Error out if resource uses more than 1 register
		// TODO: This should probably be relaxed
		if (resDesc.BindCount != 1) {
			ZG_ERROR("Multiple registers for a single resource not allowed");
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Constant buffer
		if (resDesc.Type == D3D_SIT_CBUFFER) {

			// Error out if we have too many constant buffers
			if (bindingsOut.constBuffers.isFull()) {
				ZG_ERROR("Too many constant buffers, only %u allowed",
					bindingsOut.constBuffers.capacity());
				return ZG_ERROR_SHADER_COMPILE_ERROR;
			}

			// Get constant buffer reflection
			ID3D12ShaderReflectionConstantBuffer* cbufferReflection =
				reflection->GetConstantBufferByName(resDesc.Name);
			D3D12_SHADER_BUFFER_DESC cbufferDesc = {};
			CHECK_D3D12 cbufferReflection->GetDesc(&cbufferDesc);

			// Check if push constant
			bool isPushConstant = pushConstantRegisters.findElement(resDesc.BindPoint) != nullptr;

			// Add constant buffer binding
			ZgConstantBufferBindingDesc binding = {};
			binding.bufferRegister = resDesc.BindPoint;
			binding.sizeInBytes = cbufferDesc.Size;
			binding.pushConstant = isPushConstant ? ZG_TRUE : ZG_FALSE;
			bindingsOut.constBuffers.add(binding);
		}

		// Unordered buffer
		else if (resDesc.Type == D3D_SIT_UAV_RWSTRUCTURED) {

			// Error out if we have too many unordered buffers
			if (bindingsOut.unorderedBuffers.isFull()) {
				ZG_ERROR("Too many unordered buffers, only %u allowed",
					bindingsOut.unorderedBuffers.capacity());
				return ZG_ERROR_SHADER_COMPILE_ERROR;
			}

			// Add unordered buffer binding
			ZgUnorderedBufferBindingDesc binding = {};
			binding.unorderedRegister = resDesc.BindPoint;
			bindingsOut.unorderedBuffers.add(binding);
		}

		// Texture
		else if (resDesc.Type == D3D_SIT_TEXTURE) {

			// Error out if we have too many textures
			if (bindingsOut.textures.isFull()) {
				ZG_ERROR("Too many textures, only %u allowed", bindingsOut.textures.capacity());
				return ZG_ERROR_SHADER_COMPILE_ERROR;
			}

			// Add texture binding
			ZgTextureBindingDesc binding = {};
			binding.textureRegister = resDesc.BindPoint;
			bindingsOut.textures.add(binding);
		}
		
		// Unordered texture
		else if (resDesc.Type == D3D_SIT_UAV_RWTYPED) {

			// Error out if we have too many unordered textures
			if (bindingsOut.textures.isFull()) {
				ZG_ERROR("Too many unordered textures, only %u allowed",
					bindingsOut.unorderedTextures.capacity());
				return ZG_ERROR_SHADER_COMPILE_ERROR;
			}

			// Add unordered texture binding
			ZgUnorderedTextureBindingDesc binding = {};
			binding.unorderedRegister = resDesc.BindPoint;
			bindingsOut.unorderedTextures.add(binding);
		}

		// Samplers
		else if (resDesc.Type == D3D_SIT_SAMPLER) {
			// Note: We don't expose dynamic samplers currently
		}

		// Unsupported resource type
		else {
			ZG_ERROR("Shader resource %s (register = %u) is of unsupported resource type",
				resDesc.Name, resDesc.BindPoint);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}
	}

	// Sort constant buffers by register
	bindingsOut.constBuffers.sort(
		[](const ZgConstantBufferBindingDesc& lhs, const ZgConstantBufferBindingDesc& rhs) {
			return lhs.bufferRegister < rhs.bufferRegister;
		});

	// Sort unordered buffers by register
	bindingsOut.unorderedBuffers.sort(
		[](const ZgUnorderedBufferBindingDesc& lhs, const ZgUnorderedBufferBindingDesc& rhs) {
			return lhs.unorderedRegister < rhs.unorderedRegister;
		});

	// Sort textures by register
	bindingsOut.textures.sort(
		[](const ZgTextureBindingDesc& lhs, const ZgTextureBindingDesc& rhs) {
			return lhs.textureRegister < rhs.textureRegister;
		});

	return ZG_SUCCESS;
}

static ZgResult createRootSignature(
	D3D12RootSignature& rootSignatureOut,
	const D3D12PipelineBindingsSignature& bindings,
	const ArrayLocal<ZgSampler, ZG_MAX_NUM_SAMPLERS>& zgSamplers,
	ID3D12Device3& device) noexcept
{
	// Allow root signature access from all shader stages, opt in to using an input layout
	D3D12_ROOT_SIGNATURE_FLAGS flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Root signature parameters
	// We know that we can't have more than 64 root parameters as maximum (i.e. 64 words)
	constexpr u32 MAX_NUM_ROOT_PARAMETERS = 64;
	ArrayLocal<CD3DX12_ROOT_PARAMETER1, MAX_NUM_ROOT_PARAMETERS> parameters;

	// Add push constants
	for (u32 i = 0; i < bindings.constBuffers.size(); i++) {
		const ZgConstantBufferBindingDesc& cbuffer = bindings.constBuffers[i];
		if (cbuffer.pushConstant == ZG_FALSE) continue;

		// Get parameter index for the push constant
		sfz_assert(!parameters.isFull());
		u32 parameterIndex = parameters.size();

		// Create root parameter
		CD3DX12_ROOT_PARAMETER1 parameter;
		sfz_assert((cbuffer.sizeInBytes % 4) == 0);
		sfz_assert(cbuffer.sizeInBytes <= 1024);
		parameter.InitAsConstants(
			cbuffer.sizeInBytes / 4, cbuffer.bufferRegister, 0, D3D12_SHADER_VISIBILITY_ALL);
		parameters.add(std::move(parameter));
		sfz_assert(!parameters.isFull());

		// Add to push constants mappings
		D3D12PushConstantMapping mapping;
		mapping.bufferRegister = cbuffer.bufferRegister;
		mapping.parameterIndex = parameterIndex;
		mapping.sizeInBytes = cbuffer.sizeInBytes;
		rootSignatureOut.pushConstants.add(mapping);
	}

	// The offset into the dynamic table
	u32 currentTableOffset = 0;

	// Add dynamic constant buffers(non-push constants) mappings
	u32 dynamicConstBuffersFirstRegister = ~0u; // TODO: THIS IS PROBABLY BAD
	for (const ZgConstantBufferBindingDesc& cbuffer : bindings.constBuffers) {
		if (cbuffer.pushConstant == ZG_TRUE) continue;

		if (dynamicConstBuffersFirstRegister == ~0u) {
			dynamicConstBuffersFirstRegister = cbuffer.bufferRegister;
		}

		// Add to constant buffer mappings
		D3D12ConstantBufferMapping mapping;
		mapping.bufferRegister = cbuffer.bufferRegister;
		mapping.tableOffset = currentTableOffset;
		mapping.sizeInBytes = cbuffer.sizeInBytes;
		rootSignatureOut.constBuffers.add(mapping);

		// Increment table offset
		currentTableOffset += 1;
	}

	// Note: Both unordered textures and buffers uses the same unordered register range. This means
	//       that we need to combine them so that we get the registers in the right order in the range.
	struct UnorderedBindingDesc {
		bool isBuffer = false;
		u32 unorderedRegister = 0;
	};

	constexpr u32 MAX_NUM_UNORDERED_RESOURCES =
		ZG_MAX_NUM_UNORDERED_BUFFERS + ZG_MAX_NUM_UNORDERED_TEXTURES;
	ArrayLocal<UnorderedBindingDesc, MAX_NUM_UNORDERED_RESOURCES> unorderedResources;

	for (const ZgUnorderedBufferBindingDesc& buffer : bindings.unorderedBuffers) {
		UnorderedBindingDesc desc;
		desc.isBuffer = true;
		desc.unorderedRegister = buffer.unorderedRegister;
		unorderedResources.add(desc);
	}
	for (const ZgUnorderedTextureBindingDesc& texture : bindings.unorderedTextures) {
		UnorderedBindingDesc desc;
		desc.isBuffer = false;
		desc.unorderedRegister = texture.unorderedRegister;
		unorderedResources.add(desc);
	}

	// Ensure all unordered resources are sorted by the registers
	unorderedResources.sort([](const auto& lhs, const auto& rhs) {
		return lhs.unorderedRegister < rhs.unorderedRegister;
	});


	// The first and last unordered register used
	u32 firstDynamicUnorderedRegister = U32_MAX;
	u32 lastDynamicUnorderedRegister = 0;

	// Add unordered resources
	for (const UnorderedBindingDesc& desc : unorderedResources) {

		firstDynamicUnorderedRegister = sfz::min(firstDynamicUnorderedRegister, desc.unorderedRegister);
		lastDynamicUnorderedRegister = sfz::max(lastDynamicUnorderedRegister, desc.unorderedRegister);

		// Add to unordered buffer/texture mappings
		if (desc.isBuffer) {
			D3D12UnorderedBufferMapping mapping;
			mapping.unorderedRegister = desc.unorderedRegister;
			mapping.tableOffset = currentTableOffset;
			rootSignatureOut.unorderedBuffers.add(mapping);
		}
		else {
			// Add to unordered texture mappings
			D3D12UnorderedTextureMapping mapping;
			mapping.unorderedRegister = desc.unorderedRegister;
			mapping.tableOffset = currentTableOffset;
			rootSignatureOut.unorderedTextures.add(mapping);
		}

		// Increment table offset
		currentTableOffset += 1;
	}

	// Sort of assume we have a coherent range
	// TODO: Can probably be removed, but make sure that's fine first or if we need to make some logic changes
	const u32 numUnorderedRegisters = lastDynamicUnorderedRegister - firstDynamicUnorderedRegister + 1;
	if (!rootSignatureOut.unorderedBuffers.isEmpty() || !rootSignatureOut.unorderedTextures.isEmpty()) {
		sfz_assert(numUnorderedRegisters == rootSignatureOut.unorderedBuffers.size() + rootSignatureOut.unorderedTextures.size());
	}

	// Add texture mappings
	u32 dynamicTexturesFirstRegister = ~0u; // TODO: THIS IS PROBABLY BAD
	for (const ZgTextureBindingDesc& texDesc : bindings.textures) {
		if (dynamicTexturesFirstRegister == ~0u) {
			dynamicTexturesFirstRegister = texDesc.textureRegister;
		}

		// Add to texture mappings
		D3D12TextureMapping mapping;
		mapping.textureRegister = texDesc.textureRegister;
		mapping.tableOffset = currentTableOffset;
		rootSignatureOut.textures.add(mapping);

		// Increment table offset
		currentTableOffset += 1;
	}

	// Add dynamic table parameter if we need to
	if (!rootSignatureOut.constBuffers.isEmpty() || 
		!rootSignatureOut.unorderedBuffers.isEmpty() ||
		!rootSignatureOut.textures.isEmpty() ||
		!rootSignatureOut.unorderedTextures.isEmpty()) {

		// Store parameter index of dynamic table
		sfz_assert(!parameters.isFull());
		rootSignatureOut.dynamicBuffersParameterIndex = parameters.size();

		// TODO: Currently using the assumption that the shader register range is continuous,
		//       which is probably not at all reasonable in practice
		constexpr u32 MAX_NUM_RANGES = 3; // CBVs, UAVs and SRVs
		ArrayLocal<CD3DX12_DESCRIPTOR_RANGE1, MAX_NUM_RANGES> ranges;
		if (!rootSignatureOut.constBuffers.isEmpty()) {
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
				rootSignatureOut.constBuffers.size(), dynamicConstBuffersFirstRegister);
			ranges.add(std::move(range));
		}
		if (!rootSignatureOut.unorderedBuffers.isEmpty() || !rootSignatureOut.unorderedTextures.isEmpty()) {
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
				numUnorderedRegisters,
				firstDynamicUnorderedRegister);
			ranges.add(std::move(range));
		}
		if (!rootSignatureOut.textures.isEmpty()) {
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				rootSignatureOut.textures.size(), dynamicTexturesFirstRegister);
			ranges.add(std::move(range));
		}

		// Create dynamic table parameter
		CD3DX12_ROOT_PARAMETER1 parameter;
		parameter.InitAsDescriptorTable(ranges.size(), ranges.data());
		parameters.add(std::move(parameter));
	}

	// Add static samplers
	ArrayLocal<D3D12_STATIC_SAMPLER_DESC, ZG_MAX_NUM_SAMPLERS> samplers;
	for (const ZgSampler& zgSampler : zgSamplers) {
		
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
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
		samplerDesc.ShaderRegister = samplers.size();
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // TODO: Check this from reflection
		samplers.add(samplerDesc);
	}

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
	desc.Init_1_1(parameters.size(), parameters.data(), samplers.size(), samplers.data(), flags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> errorBlob;
	if (D3D12_FAIL(D3DX12SerializeVersionedRootSignature(
		&desc, D3D_ROOT_SIGNATURE_VERSION_1_1, &blob, &errorBlob))) {

		ZG_ERROR("D3DX12SerializeVersionedRootSignature() failed: %s\n",
			(const char*)errorBlob->GetBufferPointer());
		return ZG_ERROR_GENERIC;
	}

	// Create root signature
	if (D3D12_FAIL(device.CreateRootSignature(
		0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignatureOut.rootSignature)))) {
		return ZG_ERROR_GENERIC;
	}

	return ZG_SUCCESS;
}

// D3D12RootSignature
// ------------------------------------------------------------------------------------------------

const D3D12PushConstantMapping* D3D12RootSignature::getPushConstantMapping(u32 bufferRegister) const noexcept
{
	return pushConstants.find([&](const auto& e) { return e.bufferRegister == bufferRegister; });
}

const D3D12ConstantBufferMapping* D3D12RootSignature::getConstBufferMapping(u32 bufferRegister) const noexcept
{
	return constBuffers.find([&](const auto& e) { return e.bufferRegister == bufferRegister; });
}

const D3D12TextureMapping* D3D12RootSignature::getTextureMapping(u32 textureRegister) const noexcept
{
	return textures.find([&](const auto& e) { return e.textureRegister == textureRegister; });
}

const D3D12UnorderedBufferMapping* D3D12RootSignature::getUnorderedBufferMapping(u32 unorderedRegister) const noexcept
{
	return unorderedBuffers.find([&](const auto& e) { return e.unorderedRegister == unorderedRegister; });
}

const D3D12UnorderedTextureMapping* D3D12RootSignature::getUnorderedTextureMapping(u32 unorderedRegister) const noexcept
{
	return unorderedTextures.find([&](const auto& e) { return e.unorderedRegister == unorderedRegister; });
}

// D3D12PipelineCompute functions
// ------------------------------------------------------------------------------------------------

static ZgResult createPipelineComputeInternal(
	ZgPipelineCompute** pipelineOut,
	const ZgPipelineComputeCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	time_point compileStartTime,
	const ComPtr<IDxcBlobEncoding>& encodingBlob,
	const char* computeShaderName,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept
{
	// Compile compute shader
	ComPtr<IDxcBlob> computeBlob;
	ComPtr<ID3D12ShaderReflection> computeReflection;
	ZgResult computeShaderRes = compileHlslShader(
		dxcCompiler,
		dxcIncludeHandler,
		computeBlob,
		computeReflection,
		encodingBlob,
		computeShaderName,
		createInfo.computeShaderEntry,
		compileSettings,
		ShaderType::COMPUTE);
	if (computeShaderRes != ZG_SUCCESS) return computeShaderRes;

	// Get shader description froms reflection data
	D3D12_SHADER_DESC computeDesc = {};
	CHECK_D3D12 computeReflection->GetDesc(&computeDesc);

	// Get pipeline bindings signature
	D3D12PipelineBindingsSignature bindings = {};
	{
		sfz::ArrayLocal<u32, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstantRegisters;
		pushConstantRegisters.add(createInfo.pushConstantRegisters, createInfo.numPushConstants);

		ZgResult bindingsRes = bindingsFromReflection(
			computeReflection, pushConstantRegisters, bindings);
		if (bindingsRes != ZG_SUCCESS) return bindingsRes;
	}

	// Create root signature
	D3D12RootSignature rootSignature;
	{
		ArrayLocal<ZgSampler, ZG_MAX_NUM_SAMPLERS> samplers;
		samplers.add(createInfo.samplers, createInfo.numSamplers);
		ZgResult res = createRootSignature(rootSignature, bindings, samplers, device);
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
		stream.rootSignature = rootSignature.rootSignature.Get();

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
	f32 compileTimeMs = calculateDeltaMillis(compileStartTime);
	logPipelineComputeInfo(
		createInfo,
		computeShaderName,
		bindings.toZgSignature(),
		groupDimX,
		groupDimY,
		groupDimZ,
		compileTimeMs);

	// Allocate pipeline
	ZgPipelineCompute* pipeline = sfz_new<ZgPipelineCompute>(getAllocator(), sfz_dbg("ZgPipelineCompute"));

	// Store pipeline state
	pipeline->pipelineState = pipelineState;
	pipeline->rootSignature = rootSignature;
	pipeline->bindingsSignature = bindings;
	pipeline->groupDimX = groupDimX;
	pipeline->groupDimY = groupDimY;
	pipeline->groupDimZ = groupDimZ;

	// Return pipeline
	*pipelineOut = pipeline;
	return ZG_SUCCESS;
}

ZgResult createPipelineComputeFileHLSL(
	ZgPipelineCompute** pipelineOut,
	const ZgPipelineComputeCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept
{
	// Start measuring compile-time
	time_point compileStartTime;
	calculateDeltaMillis(compileStartTime);

	// Read compute shader from file
	ComPtr<IDxcBlobEncoding> encodingBlob;
	ZgResult blobReadRes =
		dxcCreateHlslBlobFromFile(dxcLibrary, createInfo.computeShader, encodingBlob);
	if (blobReadRes != ZG_SUCCESS) return blobReadRes;

	return createPipelineComputeInternal(
		pipelineOut,
		createInfo,
		compileSettings,
		compileStartTime,
		encodingBlob,
		createInfo.computeShader,
		dxcCompiler,
		dxcIncludeHandler,
		device);
}

// D3D12PipelineRender functions
// ------------------------------------------------------------------------------------------------

static ZgResult createPipelineRenderInternal(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	time_point compileStartTime,
	const ComPtr<IDxcBlobEncoding>& vertexEncodingBlob,
	const ComPtr<IDxcBlobEncoding> pixelEncodingBlob,
	const char* vertexShaderName,
	const char* pixelShaderName,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept
{
	// Compile vertex shader
	ComPtr<IDxcBlob> vertexBlob;
	ComPtr<ID3D12ShaderReflection> vertexReflection;
	ZgResult vertexShaderRes = compileHlslShader(
		dxcCompiler,
		dxcIncludeHandler,
		vertexBlob,
		vertexReflection,
		vertexEncodingBlob,
		vertexShaderName,
		createInfo.vertexShaderEntry,
		compileSettings,
		ShaderType::VERTEX);
	if (vertexShaderRes != ZG_SUCCESS) return vertexShaderRes;

	// Compile pixel shader
	ComPtr<IDxcBlob> pixelBlob;
	ComPtr<ID3D12ShaderReflection> pixelReflection;
	ZgResult pixelShaderRes = compileHlslShader(
		dxcCompiler,
		dxcIncludeHandler,
		pixelBlob,
		pixelReflection,
		pixelEncodingBlob,
		pixelShaderName,
		createInfo.pixelShaderEntry,
		compileSettings,
		ShaderType::PIXEL);
	if (pixelShaderRes != ZG_SUCCESS) return pixelShaderRes;

	// Get shader description froms reflection data
	D3D12_SHADER_DESC vertexDesc = {};
	CHECK_D3D12 vertexReflection->GetDesc(&vertexDesc);
	D3D12_SHADER_DESC pixelDesc = {};
	CHECK_D3D12 pixelReflection->GetDesc(&pixelDesc);

	// Validate that the user has specified correct number of vertex attributes
	// TODO: This doesn't seem to work. InputParameter also counts compiler generated inputs, such
	//       as SV_VertexID.
	/*if (createInfo.numVertexAttributes != vertexDesc.InputParameters) {
		ZG_ERROR("Invalid ZgPipelineRenderingCreateInfo. It specifies %u vertex"
			" attributes, shader reflection finds %u",
			createInfo.numVertexAttributes, vertexDesc.InputParameters);
		return ZG_ERROR_INVALID_ARGUMENT;
	}*/
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

	// Get pipeline bindings signature
	D3D12PipelineBindingsSignature bindings = {};
	{
		sfz::ArrayLocal<u32, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstantRegisters;
		pushConstantRegisters.add(createInfo.pushConstantRegisters, createInfo.numPushConstants);

		ZgResult vertexBindingsRes = bindingsFromReflection(
			vertexReflection, pushConstantRegisters, bindings);
		if (vertexBindingsRes != ZG_SUCCESS) return vertexBindingsRes;

		D3D12PipelineBindingsSignature pixelBindings = {};
		ZgResult pixelBindingsRes = bindingsFromReflection(
			pixelReflection, pushConstantRegisters, pixelBindings);
		if (pixelBindingsRes != ZG_SUCCESS) return pixelBindingsRes;

		// Merge constant buffers
		for (const ZgConstantBufferBindingDesc& binding : pixelBindings.constBuffers) {

			// Add binding if we don't already have it
			bool haveBinding = bindings.constBuffers.find(
				[&](const auto& e) { return e.bufferRegister == binding.bufferRegister; }) != nullptr;
			if (!haveBinding) {

				// Ensure we don't have too many constants buffers
				if (bindings.constBuffers.isFull()) {
					ZG_ERROR("Too many constant buffers, only %u allowed",
						bindings.constBuffers.capacity());
					return ZG_ERROR_SHADER_COMPILE_ERROR;
				}
				
				// Add it
				bindings.constBuffers.add(binding);
			}
		}

		// Merge unordered buffers
		for (const ZgUnorderedBufferBindingDesc& binding : pixelBindings.unorderedBuffers) {

			// Add binding if we don't already have it
			bool haveBinding = bindings.unorderedBuffers.find(
				[&](const auto& e) { return e.unorderedRegister == binding.unorderedRegister; }) != nullptr;
			if (!haveBinding) {

				// Ensure we don't have too many constants buffers
				if (bindings.unorderedBuffers.isFull()) {
					ZG_ERROR("Too many unordered buffers, only %u allowed",
						bindings.unorderedBuffers.capacity());
					return ZG_ERROR_SHADER_COMPILE_ERROR;
				}

				// Add it
				bindings.unorderedBuffers.add(binding);
			}
		}

		// Merge textures
		for (const ZgTextureBindingDesc& binding : pixelBindings.textures) {

			// Add binding if we don't already have it
			bool haveBinding = bindings.textures.find(
				[&](const auto& e) { return e.textureRegister == binding.textureRegister; }) != nullptr;
			if (!haveBinding) {

				// Ensure we don't have too many textures
				if (bindings.textures.isFull()) {
					ZG_ERROR("Too many textures, only %u allowed", bindings.textures.capacity());
					return ZG_ERROR_SHADER_COMPILE_ERROR;
				}

				// Add it
				bindings.textures.add(binding);
			}
		}

		// Merge unordered textures
		for (const ZgUnorderedTextureBindingDesc& binding : pixelBindings.unorderedTextures) {

			// Add binding if we don't already have it
			bool haveBinding = bindings.unorderedTextures.find(
				[&](const auto& e) { return e.unorderedRegister == binding.unorderedRegister; }) != nullptr;
			if (!haveBinding) {

				// Ensure we don't have too many textures
				if (bindings.textures.isFull()) {
					ZG_ERROR("Too many unordered textures, only %u allowed",
						bindings.unorderedTextures.capacity());
					return ZG_ERROR_SHADER_COMPILE_ERROR;
				}

				// Add it
				bindings.unorderedTextures.add(binding);
			}
		}

		// Sort buffers by register
		bindings.constBuffers.sort(
			[](const ZgConstantBufferBindingDesc& lhs, const ZgConstantBufferBindingDesc& rhs) {
				return lhs.bufferRegister < rhs.bufferRegister;
			});

		// Sort unordered buffers by register
		bindings.unorderedBuffers.sort(
			[](const ZgUnorderedBufferBindingDesc& lhs, const ZgUnorderedBufferBindingDesc& rhs) {
				return lhs.unorderedRegister < rhs.unorderedRegister;
			});

		// Sort textures by register
		bindings.textures.sort(
			[](const ZgTextureBindingDesc& lhs, const ZgTextureBindingDesc& rhs) {
				return lhs.textureRegister < rhs.textureRegister;
			});

		// Sort unordered textures by register
		bindings.unorderedTextures.sort(
			[](const ZgUnorderedTextureBindingDesc& lhs, const ZgUnorderedTextureBindingDesc& rhs) {
				return lhs.unorderedRegister < rhs.unorderedRegister;
			});
	}

	// Check that all necessary sampler data is available
	ArrayLocal<bool, ZG_MAX_NUM_SAMPLERS> samplerSet;
	samplerSet.add(false, ZG_MAX_NUM_SAMPLERS);
	for (u32 i = 0; i < vertexDesc.BoundResources; i++) {
		D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
		CHECK_D3D12 vertexReflection->GetResourceBindingDesc(i, &resDesc);

		// Continue if not a sampler
		if (resDesc.Type != D3D_SIT_SAMPLER) continue;

		// Error out if sampler has invalid register
		if (resDesc.BindPoint >= createInfo.numSamplers) {
			ZG_ERROR("Sampler %s is bound to register %u, num specified samplers is %u",
				resDesc.Name, resDesc.BindPoint, createInfo.numSamplers);
			return ZG_ERROR_INVALID_ARGUMENT;

		}
		sfz_assert(resDesc.BindCount == 1);

		// Mark sampler as found
		samplerSet[resDesc.BindPoint] = true;
	}
	for (u32 i = 0; i < pixelDesc.BoundResources; i++) {
		D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
		CHECK_D3D12 pixelReflection->GetResourceBindingDesc(i, &resDesc);

		// Continue if not a sampler
		if (resDesc.Type != D3D_SIT_SAMPLER) continue;

		// Error out if sampler has invalid register
		if (resDesc.BindPoint >= createInfo.numSamplers) {
			ZG_ERROR("Sampler %s is bound to register %u, num specified samplers is %u",
				resDesc.Name, resDesc.BindPoint, createInfo.numSamplers);
			return ZG_ERROR_INVALID_ARGUMENT;

		}
		sfz_assert(resDesc.BindCount == 1);

		// Mark sampler as found
		samplerSet[resDesc.BindPoint] = true;
	}
	for (u32 i = 0; i < createInfo.numSamplers; i++) {
		if (!samplerSet[i]) {
			ZG_ERROR(
				"%u samplers were specified, however sampler %u is not used by the pipeline",
				createInfo.numSamplers, i);
			return ZG_ERROR_INVALID_ARGUMENT;
		}
	}

	// Check that the correct number of render targets is specified
	const u32 numRenderTargets = pixelDesc.OutputParameters;
	if (numRenderTargets != createInfo.numRenderTargets) {
		ZG_ERROR("%u render targets were specified, however %u is used by the pipeline",
			createInfo.numRenderTargets, numRenderTargets);
		return ZG_ERROR_INVALID_ARGUMENT;
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
	D3D12RootSignature rootSignature;
	{
		ArrayLocal<ZgSampler, ZG_MAX_NUM_SAMPLERS> samplers;
		samplers.add(createInfo.samplers, createInfo.numSamplers);
		ZgResult res = createRootSignature(rootSignature, bindings, samplers, device);
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
		stream.rootSignature = rootSignature.rootSignature.Get();

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
	f32 compileTimeMs = calculateDeltaMillis(compileStartTime);
	logPipelineRenderInfo(
		createInfo,
		vertexShaderName,
		pixelShaderName,
		bindings.toZgSignature(),
		renderSignature,
		compileTimeMs);

	// Allocate pipeline
	ZgPipelineRender* pipeline =
		sfz_new<ZgPipelineRender>(getAllocator(), sfz_dbg("ZgPipelineRender"));

	// Store pipeline state
	pipeline->pipelineState = pipelineState;
	pipeline->rootSignature = rootSignature;
	pipeline->bindingsSignature = bindings;
	pipeline->renderSignature = renderSignature;
	pipeline->renderSignature.bindings = bindings.toZgSignature();
	pipeline->createInfo = createInfo;

	// Return pipeline
	*pipelineOut = pipeline;
	return ZG_SUCCESS;
}

ZgResult createPipelineRenderFileHLSL(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept
{
	// Start measuring compile-time
	time_point compileStartTime;
	calculateDeltaMillis(compileStartTime);

	// Read vertex shader from file
	ComPtr<IDxcBlobEncoding> vertexEncodingBlob;
	ZgResult vertexBlobReadRes =
		dxcCreateHlslBlobFromFile(dxcLibrary, createInfo.vertexShader, vertexEncodingBlob);
	if (vertexBlobReadRes != ZG_SUCCESS) return vertexBlobReadRes;

	// Read pixel shader from file
	ComPtr<IDxcBlobEncoding> pixelEncodingBlob;
	bool vertexAndPixelSameEncodingBlob =
		std::strcmp(createInfo.vertexShader, createInfo.pixelShader) == 0;
	if (vertexAndPixelSameEncodingBlob) {
		pixelEncodingBlob = vertexEncodingBlob;
	}
	else {
		ZgResult pixelBlobReadRes =
			dxcCreateHlslBlobFromFile(dxcLibrary, createInfo.pixelShader, pixelEncodingBlob);
		if (pixelBlobReadRes != ZG_SUCCESS) return pixelBlobReadRes;
	}

	return createPipelineRenderInternal(
		pipelineOut,
		createInfo,
		compileSettings,
		compileStartTime,
		vertexEncodingBlob,
		pixelEncodingBlob,
		createInfo.vertexShader,
		createInfo.pixelShader,
		dxcCompiler,
		dxcIncludeHandler,
		device);
}

ZgResult createPipelineRenderSourceHLSL(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept
{
	// Start measuring compile-time
	time_point compileStartTime;
	calculateDeltaMillis(compileStartTime);

	// Create encoding blob from source
	ComPtr<IDxcBlobEncoding> vertexEncodingBlob;
	ZgResult vertexBlobReadRes =
		dxcCreateHlslBlobFromSource(dxcLibrary, createInfo.vertexShader, vertexEncodingBlob);
	if (vertexBlobReadRes != ZG_SUCCESS) return vertexBlobReadRes;

	// Create encoding blob from source
	ComPtr<IDxcBlobEncoding> pixelEncodingBlob;
	ZgResult pixelBlobReadRes =
		dxcCreateHlslBlobFromSource(dxcLibrary, createInfo.pixelShader, pixelEncodingBlob);
	if (pixelBlobReadRes != ZG_SUCCESS) return pixelBlobReadRes;

	return createPipelineRenderInternal(
		pipelineOut,
		createInfo,
		compileSettings,
		compileStartTime,
		vertexEncodingBlob,
		pixelEncodingBlob,
		"<From source, no vertex name>",
		"<From source, no pixel name>",
		dxcCompiler,
		dxcIncludeHandler,
		device);
}
