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
#include <skipifzero_strings.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>

#include "spirv_cross_c.h"

#include "common/Strings.hpp"

namespace zg {

using time_point = std::chrono::high_resolution_clock::time_point;

// Statics
// ------------------------------------------------------------------------------------------------

static float calculateDeltaMillis(time_point& previousTime) noexcept
{
	time_point currentTime = std::chrono::high_resolution_clock::now();

	using FloatMS = std::chrono::duration<float, std::milli>;
	float delta = std::chrono::duration_cast<FloatMS>(currentTime - previousTime).count();

	previousTime = currentTime;
	return delta;
}

static sfz::Array<uint8_t> readBinaryFile(const char* path) noexcept
{
	// Open file
	std::FILE* file = std::fopen(path, "rb");
	if (file == NULL) return sfz::Array<uint8_t>();

	// Get size of file
	std::fseek(file, 0, SEEK_END);
	int64_t size = std::ftell(file);
	if (size <= 0) {
		std::fclose(file);
		return sfz::Array<uint8_t>();
	}
	std::fseek(file, 0, SEEK_SET);

	// Allocate memory for file
	sfz::Array<uint8_t> data;
	data.init(uint32_t(size), getAllocator(), sfz_dbg("binary file"));
	data.hackSetSize(uint32_t(size));

	// Read file
	size_t bytesRead = std::fread(data.data(), 1, data.size(), file);
	if (bytesRead != size_t(size)) {
		std::fclose(file);
		return sfz::Array<uint8_t>();
	}

	// Close file and return data
	std::fclose(file);
	return data;
}

#define CHECK_SPIRV_CROSS(context) (zg::CheckSpirvCrossImpl(context, __FILE__, __LINE__)) %

struct CheckSpirvCrossImpl final {
	spvc_context ctx = nullptr;
	const char* file;
	int line;

	CheckSpirvCrossImpl() = delete;
	CheckSpirvCrossImpl(spvc_context ctx, const char* file, int line) noexcept
	:
		ctx(ctx), file(file), line(line)
	{ }

	spvc_result operator% (spvc_result result) noexcept
	{
		if (result == SPVC_SUCCESS) return result;

		// Get error string if context was specified
		const char* errorStr = "<NO ERROR MESSAGE>";
		if (ctx != nullptr) errorStr = spvc_context_get_last_error_string(ctx);

		// Log error message
		logWrapper(file, line, ZG_LOG_LEVEL_ERROR, "SPIRV-Cross error: %s\n", errorStr);

		sfz_assert(false);

		return result;
	}
};

static sfz::Array<char> crossCompileSpirvToHLSL(
	spvc_context context,
	const sfz::Array<uint8_t>& spirvData) noexcept
{
	// Parse SPIR-V
	spvc_parsed_ir parsedIr = nullptr;
	CHECK_SPIRV_CROSS(context) spvc_context_parse_spirv(
		context, reinterpret_cast<const SpvId*>(spirvData.data()), spirvData.size() / 4, &parsedIr);

	// Create compiler
	spvc_compiler compiler = nullptr;
	CHECK_SPIRV_CROSS(context) spvc_context_create_compiler(
		context, SPVC_BACKEND_HLSL, parsedIr, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);

	// Reflection resources
	// TODO: Attempt to fix stuff through reflection, does not seem to work properly when outputting
	//       HLSL?
	/*	spvc_resources resources = nullptr;
	CHECK_SPIRV_CROSS(logger, context) spvc_compiler_create_shader_resources(compiler, &resources);

	// Attempt to fix vertex input attribute
	const spvc_reflected_resource* vertexInputs = nullptr;
	size_t numVertexInputs = 0;
	CHECK_SPIRV_CROSS(logger, context) spvc_resources_get_resource_list_for_type(
		resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &vertexInputs, &numVertexInputs);

	for (size_t i = 0; i < numVertexInputs; i++) {
		spvc_reflected_resource vertexInput = vertexInputs[i];


	const spvc_reflected_resource* constantBuffers = nullptr;
	size_t numConstantBuffers = 0;
	CHECK_SPIRV_CROSS(logger, context) spvc_resources_get_resource_list_for_type(
		resources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &constantBuffers, &numConstantBuffers);


	// Fix constant buffers
	// TODO: This is bad, should fix per type, not per instance
	for (size_t i = 0; i < numConstantBuffers; i++) {

		const spvc_reflected_resource& cb = constantBuffers[i];
		spvc_type type = spvc_compiler_get_type_handle(compiler, cb.base_type_id);
		uint32_t numMembers = spvc_type_get_num_member_types(type);

		printf("Constant buffer %u: %s, numMembers: %u\n", i, constantBuffers[i].name, numMembers);

		//uint32_t numMembers = spvc_type_get_num_member_types(cb.type_id);

		// Remove "type_" from type name of constant buffer
		spvc_compiler_set_name(compiler, constantBuffers[i].id, constantBuffers[i].name + 5);
	}
	*/

	// Set some compiler options
	spvc_compiler_options options = nullptr;
	CHECK_SPIRV_CROSS(context) spvc_compiler_create_compiler_options(compiler, &options);

	// Set which version of HLSL to target
	// For now target shader model 6.0, which is the lowest ZeroG supports
	// TODO: Expose this?
	CHECK_SPIRV_CROSS(context) spvc_compiler_options_set_uint(
		options, SPVC_COMPILER_OPTION_HLSL_SHADER_MODEL, 60);

	// Apply compiler options
	CHECK_SPIRV_CROSS(context) spvc_compiler_install_compiler_options(compiler, options);

	// Attempt to fix entry points
	/*const spvc_entry_point* entryPoints = nullptr;
	size_t numEntryPoints = 0;
	CHECK_SPIRV_CROSS(logger, context) spvc_compiler_get_entry_points(
		compiler, &entryPoints, &numEntryPoints);

	for (size_t i = 0; i < numEntryPoints; i++) {
		switch (entryPoints[i].execution_model) {
		case SpvExecutionModelVertex:
			CHECK_SPIRV_CROSS(logger, context) spvc_compiler_rename_entry_point(
				compiler, entryPoints[i].name, vertexEntryPoint, SpvExecutionModelVertex);
			break;

		case SpvExecutionModelFragment:
			CHECK_SPIRV_CROSS(logger, context) spvc_compiler_rename_entry_point(
				compiler, entryPoints[i].name, pixelEntryPoint, SpvExecutionModelFragment);
			break;
		}
	}*/

	// Compile to HLSL
	const char* hlslSource = nullptr;
	CHECK_SPIRV_CROSS(context) spvc_compiler_compile(compiler, &hlslSource);

	// Allocate memory and copy HLSL source to Vector<char> and return it
	uint32_t hlslSrcLen = uint32_t(std::strlen(hlslSource));
	sfz::Array<char> hlslSourceTmp;
	hlslSourceTmp.init(hlslSrcLen + 1, getAllocator(), sfz_dbg("HLSL Source"));
	hlslSourceTmp.hackSetSize(hlslSrcLen + 1);
	std::memcpy(hlslSourceTmp.data(), hlslSource, hlslSrcLen);
	hlslSourceTmp[hlslSrcLen] = '\0';
	return hlslSourceTmp;
}

static bool relativeToAbsolute(char* pathOut, uint32_t pathOutSize, const char* pathIn) noexcept
{
	DWORD res = GetFullPathNameA(pathIn, pathOutSize, pathOut, NULL);
	return res > 0;
}

static bool fixPath(WCHAR* pathOut, uint32_t pathOutNumChars, const char* utf8In) noexcept
{
	char absolutePath[MAX_PATH] = { 0 };
	if (!relativeToAbsolute(absolutePath, MAX_PATH, utf8In)) return false;
	if (!utf8ToWide(pathOut, pathOutNumChars, absolutePath)) return false;
	return true;
}

// DFCC_DXIL enum constant from DxilContainer/DxilContainer.h in DirectXShaderCompiler
#define DXIL_FOURCC(ch0, ch1, ch2, ch3) ( \
	(uint32_t)(uint8_t)(ch0)        | (uint32_t)(uint8_t)(ch1) << 8  | \
	(uint32_t)(uint8_t)(ch2) << 16  | (uint32_t)(uint8_t)(ch3) << 24   \
)
static constexpr uint32_t DFCC_DXIL = DXIL_FOURCC('D', 'X', 'I', 'L');

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
	uint32_t shaderIdx = 0;
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
	uint32_t CODE_PAGE = CP_UTF8;
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
	uint32_t CODE_PAGE = CP_UTF8;
	if (D3D12_FAIL(dxcLibrary.CreateBlobWithEncodingFromPinned(
		source, uint32_t(std::strlen(source)), CODE_PAGE, &blobOut))) {
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

	uint32_t numArgs = 0;
	for (uint32_t i = 0; i < ZG_MAX_NUM_DXC_COMPILER_FLAGS; i++) {
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

static D3D12_COMPARISON_FUNC toD3D12ComparsionFunc(ZgDepthFunc func) noexcept
{
	switch (func) {
	case ZG_DEPTH_FUNC_LESS: return D3D12_COMPARISON_FUNC_LESS;
	case ZG_DEPTH_FUNC_LESS_EQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case ZG_DEPTH_FUNC_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case ZG_DEPTH_FUNC_NOT_EQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case ZG_DEPTH_FUNC_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case ZG_DEPTH_FUNC_GREATER_EQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	}
	sfz_assert(false);
	return D3D12_COMPARISON_FUNC_LESS;
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
	const ZgPipelineComputeSignature& computeSignature,
	float compileTimeMs) noexcept
{
	sfz::str4096 tmpStr;

	// Print header
	tmpStr.appendf("Compiled ZgPipelineCompute with:\n");
	tmpStr.appendf(" - Compute shader: \"%s\" -- %s()\n\n",
		computeShaderName, createInfo.computeShaderEntry);

	// Print compile time
	tmpStr.appendf("Compile time: %.2fms\n", compileTimeMs);

	// Print group dim
	tmpStr.appendf("\nGroup dimensions: %u x %u x %u\n",
		computeSignature.groupDimX, computeSignature.groupDimY, computeSignature.groupDimZ);

	// Print constant buffers
	if (bindingsSignature.numConstBuffers > 0) {
		tmpStr.appendf("\nConstant buffers (%u):\n", bindingsSignature.numConstBuffers);
		for (uint32_t i = 0; i < bindingsSignature.numConstBuffers; i++) {
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
		for (uint32_t i = 0; i < bindingsSignature.numUnorderedBuffers; i++) {
			const ZgUnorderedBufferBindingDesc& buffer = bindingsSignature.unorderedBuffers[i];
			tmpStr.appendf(" - Register: %u\n", buffer.unorderedRegister);
		}
	}

	// Print textures
	if (bindingsSignature.numTextures > 0) {
		tmpStr.appendf("\nTextures (%u):\n", bindingsSignature.numTextures);
		for (uint32_t i = 0; i < bindingsSignature.numTextures; i++) {
			const ZgTextureBindingDesc& texture = bindingsSignature.textures[i];
			tmpStr.appendf(" - Register: %u\n", texture.textureRegister);
		}
	}

	// Print unordered textures
	if (bindingsSignature.numUnorderedTextures > 0) {
		tmpStr.appendf("\nUnordered textures (%u):\n", bindingsSignature.numUnorderedTextures);
		for (uint32_t i = 0; i < bindingsSignature.numUnorderedTextures; i++) {
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
	float compileTimeMs) noexcept
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
		for (uint32_t i = 0; i < renderSignature.numVertexAttributes; i++) {
			const ZgVertexAttribute& attrib = renderSignature.vertexAttributes[i];
			tmpStr.appendf(" - Location: %u -- Type: %s\n",
				attrib.location,
				vertexAttributeTypeToString(attrib.type));
		}
	}
	
	// Print constant buffers
	if (bindingsSignature.numConstBuffers > 0) {
		tmpStr.appendf("\nConstant buffers (%u):\n", bindingsSignature.numConstBuffers);
		for (uint32_t i = 0; i < bindingsSignature.numConstBuffers; i++) {
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
		for (uint32_t i = 0; i < bindingsSignature.numUnorderedBuffers; i++) {
			const ZgUnorderedBufferBindingDesc& buffer = bindingsSignature.unorderedBuffers[i];
			tmpStr.appendf(" - Register: %u\n", buffer.unorderedRegister);
		}
	}

	// Print textures
	if (bindingsSignature.numTextures > 0) {
		tmpStr.appendf("\nTextures (%u):\n", bindingsSignature.numTextures);
		for (uint32_t i = 0; i < bindingsSignature.numTextures; i++) {
			const ZgTextureBindingDesc& texture = bindingsSignature.textures[i];
			tmpStr.appendf(" - Register: %u\n", texture.textureRegister);
		}
	}

	// Print unordered textures
	if (bindingsSignature.numUnorderedTextures > 0) {
		tmpStr.appendf("\nUnordered textures (%u):\n", bindingsSignature.numUnorderedTextures);
		for (uint32_t i = 0; i < bindingsSignature.numUnorderedTextures; i++) {
			const ZgUnorderedTextureBindingDesc& texture = bindingsSignature.unorderedTextures[i];
			tmpStr.appendf(" - Register: %u\n", texture.unorderedRegister);
		}
	}

	// Log
	ZG_NOISE("%s", tmpStr.str());
}

static ZgResult bindingsFromReflection(
	const ComPtr<ID3D12ShaderReflection>& reflection,
	const sfz::ArrayLocal<uint32_t, ZG_MAX_NUM_CONSTANT_BUFFERS>& pushConstantRegisters,
	D3D12PipelineBindingsSignature& bindingsOut) noexcept
{
	// Get shader description from reflection
	D3D12_SHADER_DESC shaderDesc = {};
	CHECK_D3D12 reflection->GetDesc(&shaderDesc);

	// Go through all bound resources
	for (uint32_t i = 0; i < shaderDesc.BoundResources; i++) {

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
	constexpr uint32_t MAX_NUM_ROOT_PARAMETERS = 64;
	ArrayLocal<CD3DX12_ROOT_PARAMETER1, MAX_NUM_ROOT_PARAMETERS> parameters;

	// Add push constants
	for (uint32_t i = 0; i < bindings.constBuffers.size(); i++) {
		const ZgConstantBufferBindingDesc& cbuffer = bindings.constBuffers[i];
		if (cbuffer.pushConstant == ZG_FALSE) continue;

		// Get parameter index for the push constant
		sfz_assert(!parameters.isFull());
		uint32_t parameterIndex = parameters.size();

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

	// Add dynamic constant buffers(non-push constants) mappings
	uint32_t dynamicConstBuffersFirstRegister = ~0u; // TODO: THIS IS PROBABLY BAD
	for (const ZgConstantBufferBindingDesc& cbuffer : bindings.constBuffers) {
		if (cbuffer.pushConstant == ZG_TRUE) continue;

		if (dynamicConstBuffersFirstRegister == ~0u) {
			dynamicConstBuffersFirstRegister = cbuffer.bufferRegister;
		}

		// Add to constant buffer mappings
		D3D12ConstantBufferMapping mapping;
		mapping.bufferRegister = cbuffer.bufferRegister;
		mapping.tableOffset = rootSignatureOut.constBuffers.size();
		mapping.sizeInBytes = cbuffer.sizeInBytes;
		rootSignatureOut.constBuffers.add(mapping);
	}

	// Add unordered buffer mappings
	uint32_t dynamicUnorderedBuffersFirstRegister = ~0u; // TODO: THIS IS PROBABLY BAD
	for (const ZgUnorderedBufferBindingDesc& buffer : bindings.unorderedBuffers) {

		if (dynamicUnorderedBuffersFirstRegister == ~0u) {
			dynamicUnorderedBuffersFirstRegister = buffer.unorderedRegister;
		}

		// Add to unordered buffer mappings
		D3D12UnorderedBufferMapping mapping;
		mapping.unorderedRegister = buffer.unorderedRegister;
		mapping.tableOffset =
			rootSignatureOut.constBuffers.size() +
			rootSignatureOut.unorderedBuffers.size();
		rootSignatureOut.unorderedBuffers.add(mapping);
	}

	// Add unordered texture mappings
	// Actually sort of an unordered buffer, so lets pretend it is
	for (const ZgUnorderedTextureBindingDesc& texture : bindings.unorderedTextures) {

		if (dynamicUnorderedBuffersFirstRegister == ~0u) {
			dynamicUnorderedBuffersFirstRegister = texture.unorderedRegister;
		}

		// Add to unordered texture mappings
		D3D12UnorderedTextureMapping mapping;
		mapping.unorderedRegister = texture.unorderedRegister;
		mapping.tableOffset =
			rootSignatureOut.constBuffers.size() +
			rootSignatureOut.unorderedBuffers.size() +
			rootSignatureOut.unorderedTextures.size();
		rootSignatureOut.unorderedTextures.add(mapping);
	}

	// Add texture mappings
	uint32_t dynamicTexturesFirstRegister = ~0u; // TODO: THIS IS PROBABLY BAD
	for (const ZgTextureBindingDesc& texDesc : bindings.textures) {
		if (dynamicTexturesFirstRegister == ~0u) {
			dynamicTexturesFirstRegister = texDesc.textureRegister;
		}

		// Add to texture mappings
		D3D12TextureMapping mapping;
		mapping.textureRegister = texDesc.textureRegister;
		mapping.tableOffset =
			rootSignatureOut.constBuffers.size() +
			rootSignatureOut.unorderedBuffers.size() +
			rootSignatureOut.unorderedTextures.size() +
			rootSignatureOut.textures.size();
		rootSignatureOut.textures.add(mapping);
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
		constexpr uint32_t MAX_NUM_RANGES = 3; // CBVs, UAVs and SRVs
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
				rootSignatureOut.unorderedBuffers.size() + rootSignatureOut.unorderedTextures.size(),
				dynamicUnorderedBuffersFirstRegister);
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
		//samplerDesc.ComparisonFunc;
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

const D3D12PushConstantMapping* D3D12RootSignature::getPushConstantMapping(uint32_t bufferRegister) const noexcept
{
	return pushConstants.find([&](const auto& e) { return e.bufferRegister == bufferRegister; });
}

const D3D12ConstantBufferMapping* D3D12RootSignature::getConstBufferMapping(uint32_t bufferRegister) const noexcept
{
	return constBuffers.find([&](const auto& e) { return e.bufferRegister == bufferRegister; });
}

const D3D12TextureMapping* D3D12RootSignature::getTextureMapping(uint32_t textureRegister) const noexcept
{
	return textures.find([&](const auto& e) { return e.textureRegister == textureRegister; });
}

const D3D12UnorderedBufferMapping* D3D12RootSignature::getUnorderedBufferMapping(uint32_t unorderedRegister) const noexcept
{
	return unorderedBuffers.find([&](const auto& e) { return e.unorderedRegister == unorderedRegister; });
}

const D3D12UnorderedTextureMapping* D3D12RootSignature::getUnorderedTextureMapping(uint32_t unorderedRegister) const noexcept
{
	return unorderedTextures.find([&](const auto& e) { return e.unorderedRegister == unorderedRegister; });
}

// D3D12PipelineCompute functions
// ------------------------------------------------------------------------------------------------

static ZgResult createPipelineComputeInternal(
	D3D12PipelineCompute** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineComputeSignature* computeSignatureOut,
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
		sfz::ArrayLocal<uint32_t, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstantRegisters;
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
	computeSignatureOut->groupDimX = 0;
	computeSignatureOut->groupDimY = 0;
	computeSignatureOut->groupDimZ = 0;
	computeReflection->GetThreadGroupSize(
		&computeSignatureOut->groupDimX, &computeSignatureOut->groupDimY, &computeSignatureOut->groupDimZ);
	sfz_assert(computeSignatureOut->groupDimX != 0);
	sfz_assert(computeSignatureOut->groupDimY != 0);
	sfz_assert(computeSignatureOut->groupDimZ != 0);

	// Log information about the pipeline
	float compileTimeMs = calculateDeltaMillis(compileStartTime);
	logPipelineComputeInfo(
		createInfo,
		computeShaderName,
		bindings.toZgSignature(),
		*computeSignatureOut,
		compileTimeMs);

	// Allocate pipeline
	D3D12PipelineCompute* pipeline =
		getAllocator()->newObject<D3D12PipelineCompute>(sfz_dbg("D3D12PipelineCompute"));

	// Store pipeline state
	pipeline->pipelineState = pipelineState;
	pipeline->rootSignature = rootSignature;
	pipeline->bindingsSignature = bindings;

	// Return pipeline
	*pipelineOut = pipeline;
	*bindingsSignatureOut = bindings.toZgSignature();
	return ZG_SUCCESS;
}

ZgResult createPipelineComputeFileHLSL(
	D3D12PipelineCompute** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineComputeSignature* computeSignatureOut,
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
		bindingsSignatureOut,
		computeSignatureOut,
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
	D3D12PipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
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
	if (createInfo.numVertexAttributes != vertexDesc.InputParameters) {
		ZG_ERROR("Invalid ZgPipelineRenderingCreateInfo. It specifies %u vertex"
			" attributes, shader reflection finds %u",
			createInfo.numVertexAttributes, vertexDesc.InputParameters);
		return ZG_ERROR_INVALID_ARGUMENT;
	}
	renderSignatureOut->numVertexAttributes = createInfo.numVertexAttributes;

	// Validate vertex attributes
	for (uint32_t i = 0; i < createInfo.numVertexAttributes; i++) {

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
		renderSignatureOut->vertexAttributes[i] = attrib;
	}

	// Get pipeline bindings signature
	D3D12PipelineBindingsSignature bindings = {};
	{
		sfz::ArrayLocal<uint32_t, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstantRegisters;
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
	for (uint32_t i = 0; i < vertexDesc.BoundResources; i++) {
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
	for (uint32_t i = 0; i < pixelDesc.BoundResources; i++) {
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
	for (uint32_t i = 0; i < createInfo.numSamplers; i++) {
		if (!samplerSet[i]) {
			ZG_ERROR(
				"%u samplers were specified, however sampler %u is not used by the pipeline",
				createInfo.numSamplers, i);
			return ZG_ERROR_INVALID_ARGUMENT;
		}
	}

	// Check that the correct number of render targets is specified
	const uint32_t numRenderTargets = pixelDesc.OutputParameters;
	if (numRenderTargets != createInfo.numRenderTargets) {
		ZG_ERROR("%u render targets were specified, however %u is used by the pipeline",
			createInfo.numRenderTargets, numRenderTargets);
		return ZG_ERROR_INVALID_ARGUMENT;
	}

	// Copy render target info to signature
	renderSignatureOut->numRenderTargets = numRenderTargets;
	for (uint32_t i = 0; i < numRenderTargets; i++) {
		renderSignatureOut->renderTargets[i] = createInfo.renderTargets[i];
	}

	// Convert ZgVertexAttribute's to D3D12_INPUT_ELEMENT_DESC
	// This is the "input layout"
	ArrayLocal<D3D12_INPUT_ELEMENT_DESC, ZG_MAX_NUM_VERTEX_ATTRIBUTES> attributes;
	for (uint32_t i = 0; i < createInfo.numVertexAttributes; i++) {

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
		struct PipelineStateStream {
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vertexShader;
			CD3DX12_PIPELINE_STATE_STREAM_PS pixelShader;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rasterizer;
			CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blending;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 depthStencil;
		};

		// Create our token stream and set root signature
		PipelineStateStream stream = {};
		stream.rootSignature = rootSignature.rootSignature.Get();

		// Set input layout
		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
		inputLayoutDesc.pInputElementDescs = attributes.data();
		inputLayoutDesc.NumElements = attributes.size();
		stream.inputLayout = inputLayoutDesc;

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
		rtvFormats.NumRenderTargets = renderSignatureOut->numRenderTargets;
		for (uint32_t i = 0; i < renderSignatureOut->numRenderTargets; i++) {
			rtvFormats.RTFormats[i] = zgToDxgiTextureFormat(renderSignatureOut->renderTargets[i]);
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
			(createInfo.depthTest.depthTestEnabled == ZG_FALSE) ? FALSE : TRUE; FALSE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = toD3D12ComparsionFunc(createInfo.depthTest.depthFunc);
		depthStencilDesc.StencilEnable = FALSE;
		depthStencilDesc.DepthBoundsTestEnable = FALSE;
		stream.depthStencil = CD3DX12_DEPTH_STENCIL_DESC1(depthStencilDesc);

		// Create pipeline state
		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
		streamDesc.pPipelineStateSubobjectStream = &stream;
		streamDesc.SizeInBytes = sizeof(PipelineStateStream);
		if (D3D12_FAIL(device.CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState)))) {
			return ZG_ERROR_GENERIC;
		}
	}

	// Log information about the pipeline
	float compileTimeMs = calculateDeltaMillis(compileStartTime);
	logPipelineRenderInfo(
		createInfo,
		vertexShaderName,
		pixelShaderName,
		bindings.toZgSignature(),
		*renderSignatureOut,
		compileTimeMs);

	// Allocate pipeline
	D3D12PipelineRender* pipeline =
		getAllocator()->newObject<D3D12PipelineRender>(sfz_dbg("D3D12PipelineRender"));

	// Store pipeline state
	pipeline->pipelineState = pipelineState;
	pipeline->rootSignature = rootSignature;
	pipeline->bindingsSignature = bindings;
	pipeline->renderSignature = *renderSignatureOut;
	pipeline->createInfo = createInfo;

	// Return pipeline
	*pipelineOut = pipeline;
	*bindingsSignatureOut = bindings.toZgSignature();
	return ZG_SUCCESS;
}

ZgResult createPipelineRenderFileSPIRV(
	D3D12PipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	ZgPipelineRenderCreateInfo createInfo,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept
{
	// Start measuring compile-time
	time_point compileStartTime;
	calculateDeltaMillis(compileStartTime);

	// Initialize SPIRV-Cross
	spvc_context spvcContext = nullptr;
	spvc_result res = CHECK_SPIRV_CROSS(nullptr) spvc_context_create(&spvcContext);
	if (res != SPVC_SUCCESS) return ZG_ERROR_GENERIC;

	// Read vertex SPIRV binary and cross-compile to HLSL
	sfz::Array<uint8_t> vertexData = readBinaryFile(createInfo.vertexShader);
	if (vertexData.size() == 0) return ZG_ERROR_INVALID_ARGUMENT;
	sfz::Array<char> vertexHlslSrc = crossCompileSpirvToHLSL(spvcContext, vertexData);
	if (vertexHlslSrc.size() == 0) return ZG_ERROR_SHADER_COMPILE_ERROR;

	// Read pixel SPIRV binary and cross-compile to HLSL
	sfz::Array<uint8_t> pixelData = readBinaryFile(createInfo.pixelShader);
	if (pixelData.size() == 0) return ZG_ERROR_INVALID_ARGUMENT;
	sfz::Array<char> pixelHlslSrc = crossCompileSpirvToHLSL(spvcContext, pixelData);
	if (pixelHlslSrc.size() == 0) return ZG_ERROR_SHADER_COMPILE_ERROR;

	// Log the modified source code
	ZG_NOISE("SPIRV-Cross compiled vertex HLSL source:\n\n%s", vertexHlslSrc.data());
	ZG_NOISE("SPIRV-Cross compiled pixel HLSL source:\n\n%s", pixelHlslSrc.data());

	// Deinitialize SPIRV-Cross
	spvc_context_destroy(spvcContext);

	// Create encoding blob from source
	ComPtr<IDxcBlobEncoding> vertexEncodingBlob;
	ZgResult vertexBlobReadRes =
		dxcCreateHlslBlobFromSource(dxcLibrary, vertexHlslSrc.data(), vertexEncodingBlob);
	if (vertexBlobReadRes != ZG_SUCCESS) return vertexBlobReadRes;

	// Create encoding blob from source
	ComPtr<IDxcBlobEncoding> pixelEncodingBlob;
	ZgResult pixelBlobReadRes =
		dxcCreateHlslBlobFromSource(dxcLibrary, pixelHlslSrc.data(), pixelEncodingBlob);
	if (pixelBlobReadRes != ZG_SUCCESS) return pixelBlobReadRes;

	// Fake some compiler settings
	ZgPipelineCompileSettingsHLSL compileSettings = {};
	compileSettings.shaderModel = ZG_SHADER_MODEL_6_0;
	compileSettings.dxcCompilerFlags[0] = "-Zi";
	compileSettings.dxcCompilerFlags[1] = "-O3";

	// Modify entry points in create info to always be "main", because that seems to be what
	// SPIRV-Cross generates
	createInfo.vertexShaderEntry = "main";
	createInfo.pixelShaderEntry = "main";

	return createPipelineRenderInternal(
		pipelineOut,
		bindingsSignatureOut,
		renderSignatureOut,
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

ZgResult createPipelineRenderFileHLSL(
	D3D12PipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
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
		bindingsSignatureOut,
		renderSignatureOut,
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
	D3D12PipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
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
		bindingsSignatureOut,
		renderSignatureOut,
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

} // namespace zg
