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

#include "ZeroG/d3d12/D3D12PipelineRendering.hpp"

#include <algorithm>

#include "ZeroG/util/Assert.hpp"
#include "ZeroG/util/CpuAllocation.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

static bool relativeToAbsolute(char* pathOut, uint32_t pathOutSize, const char* pathIn) noexcept
{
	DWORD res = GetFullPathNameA(pathIn, pathOutSize, pathOut, NULL);
	return res > 0;
}

static bool utf8ToWide(WCHAR* wideOut, uint32_t wideSizeBytes, const char* utf8In) noexcept
{
	int res = MultiByteToWideChar(CP_UTF8, 0, utf8In, -1, wideOut, wideSizeBytes);
	return res != 0;
}

static bool fixPath(WCHAR* pathOut, uint32_t pathOutSizeBytes, const char* utf8In) noexcept
{
	char absolutePath[MAX_PATH] = { 0 };
	if (!relativeToAbsolute(absolutePath, MAX_PATH, utf8In)) return false;
	if (!utf8ToWide(pathOut, pathOutSizeBytes, absolutePath)) return false;
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

enum class HlslShaderType {
	VERTEX_SHADER_5_1,
	VERTEX_SHADER_6_0,
	VERTEX_SHADER_6_1,
	VERTEX_SHADER_6_2,
	VERTEX_SHADER_6_3,

	PIXEL_SHADER_5_1,
	PIXEL_SHADER_6_0,
	PIXEL_SHADER_6_1,
	PIXEL_SHADER_6_2,
	PIXEL_SHADER_6_3,
};

static ZgErrorCode compileHlslShader(
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	ZgLogger& logger,
	ComPtr<IDxcBlob>& blobOut,
	ComPtr<ID3D12ShaderReflection>& reflectionOut,
	const char* path,
	const char* entryName,
	const char* const * compilerFlags,
	HlslShaderType shaderType) noexcept
{
	// Convert paths to absolute wide strings
	WCHAR shaderFilePathWide[MAX_PATH] = { 0 };
	const uint32_t shaderFilePathWideSizeBytes = sizeof(shaderFilePathWide);
	if (!fixPath(shaderFilePathWide, shaderFilePathWideSizeBytes, path)) {
		return ZG_ERROR_GENERIC;
	}

	// Convert entry point to wide string
	WCHAR shaderEntryWide[256] = { 0 };
	const uint32_t shaderEntryWideSizeBytes = sizeof(shaderEntryWide);
	if (!utf8ToWide(shaderEntryWide, shaderEntryWideSizeBytes, entryName)) {
		return ZG_ERROR_GENERIC;
	}

	// Select shader type target profile string
	LPCWSTR targetProfile = [&]() {
		switch (shaderType) {
		case HlslShaderType::VERTEX_SHADER_5_1: return L"vs_5_1";
		case HlslShaderType::VERTEX_SHADER_6_0: return L"vs_6_0";
		case HlslShaderType::VERTEX_SHADER_6_1: return L"vs_6_1";
		case HlslShaderType::VERTEX_SHADER_6_2: return L"vs_6_2";
		case HlslShaderType::VERTEX_SHADER_6_3: return L"vs_6_3";

		case HlslShaderType::PIXEL_SHADER_5_1: return L"ps_5_1";
		case HlslShaderType::PIXEL_SHADER_6_0: return L"ps_6_0";
		case HlslShaderType::PIXEL_SHADER_6_1: return L"ps_6_1";
		case HlslShaderType::PIXEL_SHADER_6_2: return L"ps_6_2";
		case HlslShaderType::PIXEL_SHADER_6_3: return L"ps_6_3";
		}
		return L"UNKNOWN";
	}();

	// Create an encoding blob from file
	ComPtr<IDxcBlobEncoding> blob;
	uint32_t CODE_PAGE = CP_UTF8;
	if (D3D12_FAIL(logger, dxcLibrary.CreateBlobFromFile(
		shaderFilePathWide, &CODE_PAGE, &blob))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	// Split and convert args to wide strings :(
	WCHAR argsContainer[ZG_MAX_NUM_DXC_COMPILER_FLAGS][32] = {};
	LPCWSTR args[ZG_MAX_NUM_DXC_COMPILER_FLAGS] = {};

	uint32_t numArgs = 0;
	for (uint32_t i = 0; i < ZG_MAX_NUM_DXC_COMPILER_FLAGS; i++) {
		if (compilerFlags[i] == nullptr) continue;
		utf8ToWide(argsContainer[numArgs], 32 * sizeof(WCHAR), compilerFlags[i]);
		args[numArgs] = argsContainer[numArgs];
		numArgs++;
	}

	// Compile shader
	ComPtr<IDxcOperationResult> result;
	if (D3D12_FAIL(logger, dxcCompiler.Compile(
		blob.Get(),
		nullptr, // TODO: Filename
		shaderEntryWide,
		targetProfile,
		args,
		numArgs,
		nullptr,
		0,
		nullptr, // TODO: include handler
		&result))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	// Log compile errors/warnings
	ComPtr<IDxcBlobEncoding> errors;
	if (D3D12_FAIL(logger, result->GetErrorBuffer(&errors))) {
		return ZG_ERROR_GENERIC;
	}
	if (errors->GetBufferSize() > 0) {
		ZG_ERROR(logger, "Shader \"%s\" compilation errors:\n%s\n",
			path, (const char*)errors->GetBufferPointer());
	}

	// Check if compilation succeeded
	HRESULT compileResult = S_OK;
	result->GetStatus(&compileResult);
	if (D3D12_FAIL(logger, compileResult)) return ZG_ERROR_SHADER_COMPILE_ERROR;

	// Pick out the compiled binary
	if (!SUCCEEDED(result->GetResult(&blobOut))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}
	
	// Attempt to get reflection data
	if (D3D12_FAIL(logger, getShaderReflection(blobOut, reflectionOut))) {
		return ZG_ERROR_SHADER_COMPILE_ERROR;
	}

	return ZG_SUCCESS;
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
	ZG_ASSERT(false);
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
	ZG_ASSERT(false);
	return "";
}

static ZgVertexAttributeType vertexReflectionToAttribute(
	D3D_REGISTER_COMPONENT_TYPE compType, BYTE mask) noexcept
{
	ZG_ASSERT(compType == D3D_REGISTER_COMPONENT_FLOAT32
		|| compType == D3D_REGISTER_COMPONENT_SINT32
		|| compType == D3D_REGISTER_COMPONENT_UINT32);
	ZG_ASSERT(mask == 1 || mask == 3 || mask == 7 || mask == 15);

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

	ZG_ASSERT(false);
	return ZG_VERTEX_ATTRIBUTE_UNDEFINED;
}

static D3D12_FILTER samplingModeToD3D12(ZgSamplingMode samplingMode) noexcept
{
	switch (samplingMode) {
	case ZG_SAMPLING_MODE_NEAREST: return D3D12_FILTER_MIN_MAG_MIP_POINT;
	case ZG_SAMPLING_MODE_TRILINEAR: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	case ZG_SAMPLING_MODE_ANISOTROPIC: return D3D12_FILTER_ANISOTROPIC;
	}
	ZG_ASSERT(false);
	return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

static D3D12_TEXTURE_ADDRESS_MODE wrappingModeToD3D12(ZgWrappingMode wrappingMode) noexcept
{
	switch (wrappingMode) {
	case ZG_WRAPPING_MODE_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case ZG_WRAPPING_MODE_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	ZG_ASSERT(false);
	return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
}

static void printfAppend(char*& str, uint32_t& bytesLeft, const char* format, ...) noexcept
{
	va_list args;
	va_start(args, format);
	int res = std::vsnprintf(str, bytesLeft, format, args);
	va_end(args);

	ZG_ASSERT(res >= 0);
	ZG_ASSERT(res < int(bytesLeft));
	bytesLeft -= res;
	str += res;
}

static void logPipelineInfo(
	ZgAllocator& allocator,
	ZgLogger& logger,
	const ZgPipelineRenderingCreateInfo& createInfo,
	const ZgPipelineRenderingSignature& signature,
	const ComPtr<ID3D12ShaderReflection>& vertexReflection,
	const ComPtr<ID3D12ShaderReflection>& pixelReflection) noexcept
{
	// Allocate temp string to log
	const uint32_t STRING_MAX_SIZE = 4096;
	char* const tmpStrOriginal = reinterpret_cast<char*>(allocator.allocate(
		allocator.userPtr, STRING_MAX_SIZE, "Pipeline log temp string"));
	char* tmpStr = tmpStrOriginal;
	tmpStr[0] = '\0';
	uint32_t bytesLeft = STRING_MAX_SIZE;

	// Print header
	printfAppend(tmpStr, bytesLeft, "Compiled ZgPipelineRendering with:\n");
	printfAppend(tmpStr, bytesLeft, " - Vertex shader: \"%s\" -- %s()\n",
		createInfo.vertexShaderPath, createInfo.vertexShaderEntry);
	printfAppend(tmpStr, bytesLeft, " - Pixel shader: \"%s\" -- %s()\n\n",
		createInfo.pixelShaderPath, createInfo.pixelShaderEntry);

	// Print vertex attributes
	printfAppend(tmpStr, bytesLeft, "Vertex attributes (%u):\n", signature.numVertexAttributes);
	for (uint32_t i = 0; i < signature.numVertexAttributes; i++) {
		const ZgVertexAttribute& attrib = signature.vertexAttributes[i];
		printfAppend(tmpStr, bytesLeft, " - Location: %u -- Type: %s\n",
			attrib.location,
			vertexAttributeTypeToString(attrib.type));
	}

	// Print constant buffers
	printfAppend(tmpStr, bytesLeft, "\nConstant buffers (%u):\n", signature.numConstantBuffers);
	for (uint32_t i = 0; i < signature.numConstantBuffers; i++) {
		const ZgConstantBufferDesc& cbuffer = signature.constantBuffers[i];
		printfAppend(tmpStr, bytesLeft,
			" - Register: %u -- Size: %u bytes -- Push constant: %s -- Vertex shader: %s -- Pixel shader: %s\n",
			cbuffer.shaderRegister,
			cbuffer.sizeInBytes,
			cbuffer.pushConstant ? "YES" : "NO",
			cbuffer.vertexAccess == ZG_TRUE ? "YES" : "NO",
			cbuffer.pixelAccess == ZG_TRUE ? "YES" : "NO");
	}

	// Print textures
	printfAppend(tmpStr, bytesLeft, "\nTextures (%u):\n", signature.numTextures);
	for (uint32_t i = 0; i < signature.numTextures; i++) {
		const ZgTextureDesc& texture = signature.textures[i];
		printfAppend(tmpStr, bytesLeft,
			" - Register: %u -- Vertex shader: %s -- Pixel shader: %s\n",
			texture.textureRegister,
			texture.vertexAccess == ZG_TRUE ? "YES" : "NO",
			texture.pixelAccess == ZG_TRUE ? "YES" : "NO");
	}


	// Log
	ZG_INFO(logger, "%s", tmpStrOriginal);

	// Deallocate temp string
	allocator.deallocate(allocator.userPtr, tmpStrOriginal);
}

// D3D12 PipelineRendering
// ------------------------------------------------------------------------------------------------

D3D12PipelineRendering::~D3D12PipelineRendering() noexcept
{
	// Do nothing
}

// D3D12 PipelineRendering functions
// ------------------------------------------------------------------------------------------------

ZgErrorCode createPipelineRendering(
	D3D12PipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfo& createInfo,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	ZgLogger& logger,
	ZgAllocator& allocator,
	ID3D12Device3& device,
	std::mutex& contextMutex) noexcept
{
	// Pick out which vertex and pixel shader type to compile with
	HlslShaderType vertexShaderType = HlslShaderType::VERTEX_SHADER_5_1;
	HlslShaderType pixelShaderType = HlslShaderType::PIXEL_SHADER_5_1;
	switch (createInfo.shaderVersion) {
	case ZG_SHADER_MODEL_5_1:
		vertexShaderType = HlslShaderType::VERTEX_SHADER_5_1;
		pixelShaderType = HlslShaderType::PIXEL_SHADER_5_1;
		break;
	case ZG_SHADER_MODEL_6_0:
		vertexShaderType = HlslShaderType::VERTEX_SHADER_6_0;
		pixelShaderType = HlslShaderType::PIXEL_SHADER_6_0;
		break;
	case ZG_SHADER_MODEL_6_1:
		vertexShaderType = HlslShaderType::VERTEX_SHADER_6_1;
		pixelShaderType = HlslShaderType::PIXEL_SHADER_6_1;
		break;
	case ZG_SHADER_MODEL_6_2:
		vertexShaderType = HlslShaderType::VERTEX_SHADER_6_2;
		pixelShaderType = HlslShaderType::PIXEL_SHADER_6_2;
		break;
	case ZG_SHADER_MODEL_6_3:
		vertexShaderType = HlslShaderType::VERTEX_SHADER_6_3;
		pixelShaderType = HlslShaderType::PIXEL_SHADER_6_3;
		break;
	}

	// Compile vertex shader
	ComPtr<IDxcBlob> vertexBlob;
	ComPtr<ID3D12ShaderReflection> vertexReflection;
	ZgErrorCode vertexShaderRes = compileHlslShader(
		dxcLibrary,
		dxcCompiler,
		logger,
		vertexBlob,
		vertexReflection,
		createInfo.vertexShaderPath,
		createInfo.vertexShaderEntry,
		createInfo.dxcCompilerFlags,
		vertexShaderType);
	if (vertexShaderRes != ZG_SUCCESS) return vertexShaderRes;

	// Compile pixel shader
	ComPtr<IDxcBlob> pixelBlob;
	ComPtr<ID3D12ShaderReflection> pixelReflection;
	ZgErrorCode pixelShaderRes = compileHlslShader(
		dxcLibrary,
		dxcCompiler,
		logger,
		pixelBlob,
		pixelReflection,
		createInfo.pixelShaderPath,
		createInfo.pixelShaderEntry,
		createInfo.dxcCompilerFlags,
		pixelShaderType);
	if (pixelShaderRes != ZG_SUCCESS) return pixelShaderRes;

	// Get shader description froms reflection data
	D3D12_SHADER_DESC vertexDesc = {};
	CHECK_D3D12(logger) vertexReflection->GetDesc(&vertexDesc);
	D3D12_SHADER_DESC pixelDesc = {};
	CHECK_D3D12(logger) pixelReflection->GetDesc(&pixelDesc);

	// Validate that the user has specified correct number of vertex attributes
	if (createInfo.numVertexAttributes != vertexDesc.InputParameters) {
		ZG_ERROR(logger, "Invalid ZgPipelineRenderingCreateInfo. It specifies %u vertex"
			" attributes, shader reflection finds %u",
			createInfo.numVertexAttributes, vertexDesc.InputParameters);
		return ZG_ERROR_INVALID_ARGUMENT;
	}
	signatureOut->numVertexAttributes = createInfo.numVertexAttributes;

	// Validate vertex attributes
	for (uint32_t i = 0; i < createInfo.numVertexAttributes; i++) {
		
		const ZgVertexAttribute& attrib = createInfo.vertexAttributes[i];

		// Get signature for the i:th vertex attribute
		D3D12_SIGNATURE_PARAMETER_DESC sign = {};
		CHECK_D3D12(logger) vertexReflection->GetInputParameterDesc(i, &sign);

		// Get the type found in the shader
		ZgVertexAttributeType reflectedType =
			vertexReflectionToAttribute(sign.ComponentType, sign.Mask);

		// Check that the reflected type is the same as the specified type
		if (reflectedType != attrib.type) {
			ZG_ERROR(logger, "Invalid ZgPipelineRenderingCreateInfo. It specifies that the %u:th"
				" vertex attribute is of type %s, shader reflection finds %s",
				i,
				vertexAttributeTypeToString(attrib.type),
				vertexAttributeTypeToString(reflectedType));
			return ZG_ERROR_INVALID_ARGUMENT;
		}

		// Check that the attribute location (semantic index) is the same
		if (sign.SemanticIndex != createInfo.vertexAttributes[i].location) {
			ZG_ERROR(logger, "Invalid ZgPipelineRenderingCreateInfo. It specifies that the %u:th"
				" vertex attribute has location %u, shader reflection finds %u",
				i,
				attrib.location,
				sign.SemanticIndex);
			return ZG_ERROR_INVALID_ARGUMENT;
		}

		// Set vertex attribute in signature
		signatureOut->vertexAttributes[i] = attrib;
	}

	// Build up list of all constant buffers
	ZgConstantBufferDesc constBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	uint32_t numConstBuffers = 0;

	// First add all constant buffers from vertex shader
	for (uint32_t i = 0; i < vertexDesc.BoundResources; i++) {
		D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
		CHECK_D3D12(logger) vertexReflection->GetResourceBindingDesc(i, &resDesc);

		// Continue if not a constant buffer
		if (resDesc.Type != D3D_SIT_CBUFFER) continue;

		// Error out if buffers uses more than one register
		// TODO: This should probably be relaxed
		if (resDesc.BindCount != 1) {
			ZG_ERROR(logger, "Multiple registers for a single resource not allowed");
			return ZG_ERROR_UNIMPLEMENTED;
		}

		// Error out if we have too many constant buffers
		if (numConstBuffers >= ZG_MAX_NUM_CONSTANT_BUFFERS) {
			ZG_ERROR(logger, "Too many constant buffers, only %u allowed",
				ZG_MAX_NUM_CONSTANT_BUFFERS);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Error out if another register space than 0 is used
		if (resDesc.Space != 0) {
			ZG_ERROR(logger,
				"Vertex shader resource %s (register = %u) uses register space %u, only 0 is allowed",
				resDesc.Name, resDesc.BindPoint, resDesc.Space);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Get constant buffer reflection
		ID3D12ShaderReflectionConstantBuffer* cbufferReflection =
			vertexReflection->GetConstantBufferByName(resDesc.Name);
		D3D12_SHADER_BUFFER_DESC cbufferDesc = {};
		CHECK_D3D12(logger) cbufferReflection->GetDesc(&cbufferDesc);

		// Add slot for buffer in array
		ZgConstantBufferDesc& cbuffer = constBuffers[numConstBuffers];
		numConstBuffers += 1;

		// Set constant buffer members
		cbuffer.shaderRegister = resDesc.BindPoint;
		cbuffer.sizeInBytes = cbufferDesc.Size;
		cbuffer.vertexAccess = ZG_TRUE;
	}

	// Then add constant buffers from pixel shader
	for (uint32_t i = 0; i < pixelDesc.BoundResources; i++) {
		D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
		CHECK_D3D12(logger) pixelReflection->GetResourceBindingDesc(i, &resDesc);

		// Continue if not a constant buffer
		if (resDesc.Type != D3D_SIT_CBUFFER) continue;

		// See if buffer was already found/used by vertex shader
		uint32_t vertexCBufferIdx = ~0u;
		for (uint32_t j = 0; j < numConstBuffers; j++) {
			if (constBuffers[j].shaderRegister == resDesc.BindPoint) {
				vertexCBufferIdx = j;
				break;
			}
		}

		// If buffer was already found, mark it as accessed by pixel shader and continue to next
		// iteration
		if (vertexCBufferIdx != ~0u) {
			constBuffers[vertexCBufferIdx].pixelAccess = ZG_TRUE;
			continue;
		}

		// Error out if buffers uses more than one register
		// TODO: This should probably be relaxed
		if (resDesc.BindCount != 1) {
			ZG_ERROR(logger, "Multiple registers for a single resource not allowed");
			return ZG_ERROR_UNIMPLEMENTED;
		}

		// Error out if we have too many constant buffers
		if (numConstBuffers >= ZG_MAX_NUM_CONSTANT_BUFFERS) {
			ZG_ERROR(logger, "Too many constant buffers, only %u allowed",
				ZG_MAX_NUM_CONSTANT_BUFFERS);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Error out if another register space than 0 is used
		if (resDesc.Space != 0) {
			ZG_ERROR(logger,
				"Pixel shader resource %s (register = %u) uses register space %u, only 0 is allowed",
				resDesc.Name, resDesc.BindPoint, resDesc.Space);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Get constant buffer reflection
		ID3D12ShaderReflectionConstantBuffer* cbufferReflection =
			pixelReflection->GetConstantBufferByName(resDesc.Name);
		D3D12_SHADER_BUFFER_DESC cbufferDesc = {};
		CHECK_D3D12(logger) cbufferReflection->GetDesc(&cbufferDesc);

		// Add slot for buffer in array
		ZgConstantBufferDesc& cbuffer = constBuffers[numConstBuffers];
		numConstBuffers += 1;

		// Set constant buffer members
		cbuffer.shaderRegister = resDesc.BindPoint;
		cbuffer.sizeInBytes = cbufferDesc.Size;
		cbuffer.pixelAccess = ZG_TRUE;
	}

	// Sort buffers by register
	std::sort(constBuffers, constBuffers + numConstBuffers,
		[](const ZgConstantBufferDesc& lhs, const ZgConstantBufferDesc& rhs) {
		return lhs.shaderRegister < rhs.shaderRegister;
	});

	// Go through buffers and check if any of them are marked as push constants
	bool pushConstantRegisterUsed[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	for (uint32_t i = 0; i < numConstBuffers; i++) {
		ZgConstantBufferDesc& cbuffer = constBuffers[i];
		for (uint32_t j = 0; j < createInfo.numPushConstants; j++) {
			if (cbuffer.shaderRegister == createInfo.pushConstantRegisters[j]) {
				if (pushConstantRegisterUsed[j]) {
					ZG_ASSERT(pushConstantRegisterUsed[j]);
					return ZG_ERROR_INVALID_ARGUMENT;
				}
				cbuffer.pushConstant = ZG_TRUE;
				pushConstantRegisterUsed[j] = true;
				break;
			}
		}
	}

	// Check that all push constant registers specified was actually used
	for (uint32_t i = 0; i < createInfo.numPushConstants; i++) {
		if (!pushConstantRegisterUsed[i]) {
			ZG_ERROR(logger,
				"Shader register %u was registered as a push constant, but never used in the shader",
				createInfo.pushConstantRegisters[i]);
			return ZG_ERROR_INVALID_ARGUMENT;
		}
	}

	// Copy constant buffer information to signature
	signatureOut->numConstantBuffers = numConstBuffers;
	for (uint32_t i = 0; i < numConstBuffers; i++) {
		signatureOut->constantBuffers[i] = constBuffers[i];
	}


	// Gather all textures
	ZgTextureDesc textureDescs[ZG_MAX_NUM_TEXTURES] = {};
	uint32_t numTextures = 0;

	// First add all textures from vertex shader
	for (uint32_t i = 0; i < vertexDesc.BoundResources; i++) {
		D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
		CHECK_D3D12(logger) vertexReflection->GetResourceBindingDesc(i, &resDesc);

		// Continue if not a texture
		if (resDesc.Type != D3D_SIT_TEXTURE) continue;

		// Error out if texture uses more than one register
		// TODO: This should probably be relaxed
		if (resDesc.BindCount != 1) {
			ZG_ERROR(logger, "Multiple registers for a single resource not allowed");
			return ZG_ERROR_UNIMPLEMENTED;
		}

		// Error out if we have too many textures
		if (numTextures >= ZG_MAX_NUM_TEXTURES) {
			ZG_ERROR(logger, "Too many textures, only %u allowed", ZG_MAX_NUM_TEXTURES);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Error out if another register space than 0 is used
		if (resDesc.Space != 0) {
			ZG_ERROR(logger,
				"Vertex shader resource %s (register = %u) uses register space %u, only 0 is allowed",
				resDesc.Name, resDesc.BindPoint, resDesc.Space);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Add slot for texture in array
		ZgTextureDesc& texDesc = textureDescs[numTextures];
		numTextures += 1;

		// Set texture desc members
		texDesc.textureRegister = resDesc.BindPoint;
		texDesc.vertexAccess = ZG_TRUE;
	}

	// Then add textures from pixel shader
	for (uint32_t i = 0; i < pixelDesc.BoundResources; i++) {
		D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
		CHECK_D3D12(logger) pixelReflection->GetResourceBindingDesc(i, &resDesc);

		// Continue if not a texture
		if (resDesc.Type != D3D_SIT_TEXTURE) continue;

		// See if texture was already found/used by vertex shader
		uint32_t vertexTextureIdx = ~0u;
		for (uint32_t j = 0; j < numTextures; j++) {
			if (textureDescs[j].textureRegister == resDesc.BindPoint) {
				vertexTextureIdx = j;
				break;
			}
		}

		// If texture was already found, mark it as accessed by pixel shader and continue to next
		// iteration
		if (vertexTextureIdx != ~0u) {
			textureDescs[vertexTextureIdx].pixelAccess = ZG_TRUE;
			continue;
		}

		// Error out if texture uses more than one register
		// TODO: This should probably be relaxed
		if (resDesc.BindCount != 1) {
			ZG_ERROR(logger, "Multiple registers for a single resource not allowed");
			return ZG_ERROR_UNIMPLEMENTED;
		}

		// Error out if we have too many textures
		if (numTextures >= ZG_MAX_NUM_TEXTURES) {
			ZG_ERROR(logger, "Too many textures, only %u allowed", ZG_MAX_NUM_TEXTURES);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Error out if another register space than 0 is used
		if (resDesc.Space != 0) {
			ZG_ERROR(logger,
				"Vertex shader resource %s (register = %u) uses register space %u, only 0 is allowed",
				resDesc.Name, resDesc.BindPoint, resDesc.Space);
			return ZG_ERROR_SHADER_COMPILE_ERROR;
		}

		// Add slot for texture in array
		ZgTextureDesc& texDesc = textureDescs[numTextures];
		numTextures += 1;

		// Set texture desc members
		texDesc.textureRegister = resDesc.BindPoint;
		texDesc.pixelAccess = ZG_TRUE;
	}

	// Sort texture descs by register
	std::sort(textureDescs, textureDescs + numTextures,
		[](const ZgTextureDesc& lhs, const ZgTextureDesc& rhs) {
		return lhs.textureRegister < rhs.textureRegister;
	});

	// Copy texture information to signature
	signatureOut->numTextures = numTextures;
	for (uint32_t i = 0; i < numTextures; i++) {
		signatureOut->textures[i] = textureDescs[i];
	}


	// Check that all necessary sampler data is available
	bool samplerSet[ZG_MAX_NUM_SAMPLERS] = {};
	for (uint32_t i = 0; i < vertexDesc.BoundResources; i++) {
		D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
		CHECK_D3D12(logger) vertexReflection->GetResourceBindingDesc(i, &resDesc);

		// Continue if not a sampler
		if (resDesc.Type != D3D_SIT_SAMPLER) continue;

		// Error out if sampler has invalid register
		if (resDesc.BindPoint >= createInfo.numSamplers) {
			ZG_ERROR(logger, "Sampler %s is bound to register %u, num specified samplers is %u",
				resDesc.Name, resDesc.BindPoint, createInfo.numSamplers);
			return ZG_ERROR_INVALID_ARGUMENT;
		
		}
		ZG_ASSERT(resDesc.BindCount == 1);

		// Mark sampler as found
		samplerSet[resDesc.BindPoint] = true;
	}
	for (uint32_t i = 0; i < pixelDesc.BoundResources; i++) {
		D3D12_SHADER_INPUT_BIND_DESC resDesc = {};
		CHECK_D3D12(logger) pixelReflection->GetResourceBindingDesc(i, &resDesc);

		// Continue if not a sampler
		if (resDesc.Type != D3D_SIT_SAMPLER) continue;

		// Error out if sampler has invalid register
		if (resDesc.BindPoint >= createInfo.numSamplers) {
			ZG_ERROR(logger, "Sampler %s is bound to register %u, num specified samplers is %u",
				resDesc.Name, resDesc.BindPoint, createInfo.numSamplers);
			return ZG_ERROR_INVALID_ARGUMENT;

		}
		ZG_ASSERT(resDesc.BindCount == 1);

		// Mark sampler as found
		samplerSet[resDesc.BindPoint] = true;
	}
	for (uint32_t i = 0; i < createInfo.numSamplers; i++) {
		if (!samplerSet[i]) {
			ZG_ERROR(logger,
				"%u samplers were specified, however sampler %u is not used by the pipeline",
				createInfo.numSamplers, i);
			return ZG_ERROR_INVALID_ARGUMENT;
		}
	}


	// Convert ZgVertexAttribute's to D3D12_INPUT_ELEMENT_DESC
	// This is the "input layout"
	D3D12_INPUT_ELEMENT_DESC attributes[ZG_MAX_NUM_VERTEX_ATTRIBUTES] = {};
	for (uint32_t i = 0; i < createInfo.numVertexAttributes; i++) {

		const ZgVertexAttribute& attribute = createInfo.vertexAttributes[i];
		D3D12_INPUT_ELEMENT_DESC desc = {};
		desc.SemanticName = "ATTRIBUTE_LOCATION_";
		desc.SemanticIndex = attribute.location;
		desc.Format = vertexAttributeTypeToFormat(attribute.type);
		desc.InputSlot = attribute.vertexBufferSlot;
		desc.AlignedByteOffset = attribute.offsetToFirstElementInBytes;
		desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		desc.InstanceDataStepRate = 0;
		attributes[i] = desc;
	}

	// List of push constant mappings to be filled in when creating root signature
	D3D12PushConstantMapping pushConstantMappings[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	uint32_t numPushConstantsMappings = 0;

	// List of constant buffer mappings to be filled in when creating root signature
	D3D12ConstantBufferMapping constBufferMappings[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	uint32_t numConstBufferMappings = 0;
	
	// List of texture mappings to be filled in when creating root signature
	D3D12TextureMapping texMappings[ZG_MAX_NUM_TEXTURES] = {};
	uint32_t numTexMappings = 0;
	
	uint32_t dynamicBuffersParameterIndex = ~0u;

	// Create root signature
	ComPtr<ID3D12RootSignature> rootSignature;
	{
		// Allow root signature access from all shader stages, opt in to using an input layout
		D3D12_ROOT_SIGNATURE_FLAGS flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Root signature parameters
		// We know that we can't have more than 64 root parameters as maximum (i.e. 64 words)
		constexpr uint32_t MAX_NUM_ROOT_PARAMETERS = 64;
		CD3DX12_ROOT_PARAMETER1 parameters[MAX_NUM_ROOT_PARAMETERS];
		uint32_t numParameters = 0;

		// Add push constants
		for (uint32_t i = 0; i < signatureOut->numConstantBuffers; i++) {
			const ZgConstantBufferDesc& cbuffer = signatureOut->constantBuffers[i];
			if (cbuffer.pushConstant == ZG_FALSE) continue;

			// Get parameter index for the push constant
			uint32_t parameterIndex = numParameters;
			numParameters += 1;
			ZG_ASSERT(numParameters <= MAX_NUM_ROOT_PARAMETERS);

			// Calculate the correct shader visibility for the constant
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
			if (cbuffer.vertexAccess == ZG_TRUE && cbuffer.pixelAccess == ZG_FALSE) {
				visibility = D3D12_SHADER_VISIBILITY_VERTEX;
			}
			else if (cbuffer.vertexAccess == ZG_FALSE && cbuffer.pixelAccess == ZG_TRUE) {
				visibility = D3D12_SHADER_VISIBILITY_PIXEL;
			}

			ZG_ASSERT((cbuffer.sizeInBytes % 4) == 0);
			ZG_ASSERT(cbuffer.sizeInBytes <= 1024);
			parameters[parameterIndex].InitAsConstants(
				cbuffer.sizeInBytes / 4, cbuffer.shaderRegister, 0, visibility);

			// Add to push constants mappings
			pushConstantMappings[numPushConstantsMappings].shaderRegister = cbuffer.shaderRegister;
			pushConstantMappings[numPushConstantsMappings].parameterIndex = parameterIndex;
			pushConstantMappings[numPushConstantsMappings].sizeInBytes = cbuffer.sizeInBytes;
			numPushConstantsMappings += 1;
		}

		// Add dynamic constant buffers (non-push constants)
		uint32_t dynamicConstBuffersFirstRegister = ~0u; // TODO: THIS IS PROBABLY BAD
		for (uint32_t i = 0; i < signatureOut->numConstantBuffers; i++) {
			const ZgConstantBufferDesc& cbuffer = signatureOut->constantBuffers[i];
			if (cbuffer.pushConstant == ZG_TRUE) continue;

			if (dynamicConstBuffersFirstRegister == ~0u) {
				dynamicConstBuffersFirstRegister = cbuffer.shaderRegister;
			}
			
			// Add to constant buffer mappings
			uint32_t mappingIdx = numConstBufferMappings;
			numConstBufferMappings += 1;
			constBufferMappings[mappingIdx].shaderRegister = cbuffer.shaderRegister;
			constBufferMappings[mappingIdx].tableOffset = mappingIdx;
			constBufferMappings[mappingIdx].sizeInBytes = cbuffer.sizeInBytes;
		}

		// Add texture mappings
		uint32_t dynamicTexturesFirstRegister = ~0u; // TODO: THIS IS PROBABLY BAD
		for (uint32_t i = 0; i < signatureOut->numTextures; i++) {
			const ZgTextureDesc& texDesc = signatureOut->textures[i];
			
			if (dynamicTexturesFirstRegister == ~0u) {
				dynamicTexturesFirstRegister = texDesc.textureRegister;
			}

			// Add to texture mappings
			uint32_t mappingIdx = numTexMappings;
			numTexMappings += 1;
			texMappings[mappingIdx].textureRegister = texDesc.textureRegister;
			texMappings[mappingIdx].tableOffset = mappingIdx + numConstBufferMappings;
		}

		// Index of the parameter containing the dynamic table
		dynamicBuffersParameterIndex = numParameters;
		ZG_ASSERT(numParameters < MAX_NUM_ROOT_PARAMETERS);
		if ((numConstBufferMappings + numTexMappings) != 0) {
			numParameters += 1; // No dynamic table if no dynamic parameters
		}
		
		// TODO: Currently using the assumption that the shader register range is continuous,
		//       which is probably not at all reasonable in practice
		CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {};
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, numConstBufferMappings,
			dynamicConstBuffersFirstRegister);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numTexMappings,
			dynamicTexturesFirstRegister);
		parameters[dynamicBuffersParameterIndex].InitAsDescriptorTable(2, ranges);

		// Add static samplers
		D3D12_STATIC_SAMPLER_DESC samplers[ZG_MAX_NUM_SAMPLERS] = {};
		for (uint32_t i = 0; i < createInfo.numSamplers; i++) {

			const ZgSampler& zgSampler = createInfo.samplers[i];
			samplers[i].Filter = samplingModeToD3D12(zgSampler.samplingMode);
			samplers[i].AddressU = wrappingModeToD3D12(zgSampler.wrappingModeU);
			samplers[i].AddressV = wrappingModeToD3D12(zgSampler.wrappingModeV);
			samplers[i].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplers[i].MipLODBias = zgSampler.mipLodBias;
			samplers[i].MaxAnisotropy = 16;
			//samplers[i].ComparisonFunc;
			samplers[i].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			samplers[i].MinLOD = 0.0f;
			samplers[i].MaxLOD = D3D12_FLOAT32_MAX;
			samplers[i].ShaderRegister = i;
			samplers[i].RegisterSpace = 0;
			samplers[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // TODO: Check this from reflection
		}

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
		desc.Init_1_1(numParameters, parameters, createInfo.numSamplers, samplers, flags);

		// Serialize the root signature.
		ComPtr<ID3DBlob> blob;
		ComPtr<ID3DBlob> errorBlob;
		if (D3D12_FAIL(logger, D3DX12SerializeVersionedRootSignature(
			&desc, D3D_ROOT_SIGNATURE_VERSION_1_1, &blob, &errorBlob))) {

			ZG_ERROR(logger, "D3DX12SerializeVersionedRootSignature() failed: %s\n",
				(const char*)errorBlob->GetBufferPointer());
			return ZG_ERROR_GENERIC;
		}

		// Create root signature
		{
			std::lock_guard<std::mutex> lock(contextMutex);
			if (D3D12_FAIL(logger, device.CreateRootSignature(
				0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))) {
				return ZG_ERROR_GENERIC;
			}
		}
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
		};

		// Create our token stream and set root signature
		PipelineStateStream stream = {};
		stream.rootSignature = rootSignature.Get();

		// Set input layout
		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
		inputLayoutDesc.pInputElementDescs = attributes;
		inputLayoutDesc.NumElements = createInfo.numVertexAttributes;
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
		// TODO: Probably here Multiple Render Targets (MRT) is specified?
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Same as in our swapchain
		stream.rtvFormats = rtvFormats;

		// Set depth buffer formats
		// TODO: Allow other depth formats? Stencil buffers?
		stream.dsvFormat = DXGI_FORMAT_D32_FLOAT;

		// Create pipeline state
		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
		streamDesc.pPipelineStateSubobjectStream = &stream;
		streamDesc.SizeInBytes = sizeof(PipelineStateStream);
		{
			std::lock_guard<std::mutex> lock(contextMutex);
			if (D3D12_FAIL(logger,
				device.CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState)))) {
				return ZG_ERROR_GENERIC;
			}
		}
	}

	// Log information about the pipeline
	logPipelineInfo(
		allocator, logger, createInfo, *signatureOut, vertexReflection, pixelReflection);

	// Allocate pipeline
	D3D12PipelineRendering* pipeline =
		zgNew<D3D12PipelineRendering>(allocator, "ZeroG - D3D12PipelineRendering");

	// Store pipeline state
	pipeline->pipelineState = pipelineState;
	pipeline->rootSignature = rootSignature;
	pipeline->signature = *signatureOut;
	pipeline->numPushConstants = numPushConstantsMappings;
	pipeline->numConstantBuffers = numConstBufferMappings;
	for (uint32_t i = 0; i < ZG_MAX_NUM_CONSTANT_BUFFERS; i++) {
		pipeline->pushConstants[i] = pushConstantMappings[i];
		pipeline->constBuffers[i] = constBufferMappings[i];
	}
	pipeline->numTextures = numTexMappings;
	for (uint32_t i = 0; i < ZG_MAX_NUM_TEXTURES; i++) {
		pipeline->textures[i] = texMappings[i];
	}
	pipeline->dynamicBuffersParameterIndex = dynamicBuffersParameterIndex;
	pipeline->createInfo = createInfo;

	// Return pipeline
	*pipelineOut = pipeline;
	return ZG_SUCCESS;
}

} // namespace zg
