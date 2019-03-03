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
	case ZG_VERTEX_ATTRIBUTE_FLOAT: return DXGI_FORMAT_R32_FLOAT;
	case ZG_VERTEX_ATTRIBUTE_FLOAT2: return DXGI_FORMAT_R32G32_FLOAT;
	case ZG_VERTEX_ATTRIBUTE_FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT;
	case ZG_VERTEX_ATTRIBUTE_FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	default: break;
	}
	return DXGI_FORMAT_UNKNOWN;
}

static constexpr char SHADER_REFLECTION_LOG_FORMAT[] =
R"(Compiled %s shader from file: "%s"
Entry: %s()
ConstantBuffers: %u
BoundResources: %u
InputParameters: %u
OutputParameters: %u
InstructionCount: %u
TempRegisterCount: %u
TempArrayCount: %u
TextureNormalInstructions: %u
TextureLoadInstructions: %u
FloatInstructionCount: %u
IntInstructionCount: %u
UintInstructionCount: %u
StaticFlowControlCount: %u
DynamicFlowControlCount: %u
ArrayInstructionCount: %u
cBarrierInstructions: %u
cInterlockedInstructions: %u
cTextureStoreInstructions: %u)";

static void logReflection(
	ZgLogger& logger,
	const char* shaderType,
	const char* shaderPath,
	const char* shaderEntry,
	const D3D12_SHADER_DESC& desc) noexcept
{
	ZG_INFO(logger, SHADER_REFLECTION_LOG_FORMAT,
		shaderType,
		shaderPath,
		shaderEntry,
		desc.ConstantBuffers,
		desc.BoundResources,
		desc.InputParameters,
		desc.OutputParameters,
		desc.InstructionCount,
		desc.TempRegisterCount,
		desc.TempArrayCount,
		desc.TextureNormalInstructions,
		desc.TextureLoadInstructions,
		desc.FloatInstructionCount,
		desc.IntInstructionCount,
		desc.UintInstructionCount,
		desc.StaticFlowControlCount,
		desc.DynamicFlowControlCount,
		desc.ArrayInstructionCount,
		desc.cBarrierInstructions,
		desc.cInterlockedInstructions,
		desc.cTextureStoreInstructions);
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
	ComPtr<IDxcBlob> vertexShaderBlob;
	ComPtr<ID3D12ShaderReflection> vertexShaderReflection;
	ZgErrorCode vertexShaderRes = compileHlslShader(
		dxcLibrary,
		dxcCompiler,
		logger,
		vertexShaderBlob,
		vertexShaderReflection,
		createInfo.vertexShaderPath,
		createInfo.vertexShaderEntry,
		createInfo.dxcCompilerFlags,
		vertexShaderType);
	if (vertexShaderRes != ZG_SUCCESS) return vertexShaderRes;

	// Compile pixel shader
	ComPtr<IDxcBlob> pixelShaderBlob;
	ComPtr<ID3D12ShaderReflection> pixelShaderReflection;
	ZgErrorCode pixelShaderRes = compileHlslShader(
		dxcLibrary,
		dxcCompiler,
		logger,
		pixelShaderBlob,
		pixelShaderReflection,
		createInfo.pixelShaderPath,
		createInfo.pixelShaderEntry,
		createInfo.dxcCompilerFlags,
		pixelShaderType);
	if (pixelShaderRes != ZG_SUCCESS) return pixelShaderRes;

	// Get shader description froms reflection data
	D3D12_SHADER_DESC vertexDesc = {};
	CHECK_D3D12(logger) vertexShaderReflection->GetDesc(&vertexDesc);
	D3D12_SHADER_DESC pixelDesc = {};
	CHECK_D3D12(logger) pixelShaderReflection->GetDesc(&pixelDesc);

	// Log some info!
	logReflection(
		logger, "vertex", createInfo.vertexShaderPath, createInfo.vertexShaderEntry, vertexDesc);
	logReflection(
		logger, "pixel", createInfo.pixelShaderPath, createInfo.pixelShaderEntry, pixelDesc);

	// Convert ZgVertexAttribute's to D3D12_INPUT_ELEMENT_DESC
	// This is the "input layout"
	D3D12_INPUT_ELEMENT_DESC attributes[ZG_MAX_NUM_VERTEX_ATTRIBUTES] = {};
	for (uint32_t i = 0; i < createInfo.numVertexAttributes; i++) {

		const ZgVertexAttribute& attribute = createInfo.vertexAttributes[i];
		D3D12_INPUT_ELEMENT_DESC desc = {};
		desc.SemanticName = "ATTRIBUTE_LOCATION_";
		desc.SemanticIndex = attribute.attributeLocation;
		desc.Format = vertexAttributeTypeToFormat(attribute.type);
		desc.InputSlot = attribute.vertexBufferSlot;
		desc.AlignedByteOffset = attribute.offsetToFirstElementInBytes;
		desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		desc.InstanceDataStepRate = 0;
		attributes[i] = desc;
	}

	// Create root signature
	ComPtr<ID3D12RootSignature> rootSignature;
	{
		// Allow root signature access from all shader stages, opt in to using an input layout
		D3D12_ROOT_SIGNATURE_FLAGS flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Root signature parameters
		// TODO: Currently using temporary hardcoded parameters
		// TODO: Set dynamically with user provided settings
		CD3DX12_ROOT_PARAMETER1 parameters[ZG_MAX_NUM_PIPELINE_PARAMETERS];
		for (uint32_t i = 0; i < createInfo.numParameters; i++) {
			const ZgPipelineParameter& paramInfo = createInfo.parameters[i];
			if (paramInfo.bindingType == ZG_PIPELINE_PARAMETER_BINDING_TYPE_PUSH_CONSTANT) {
				const ZgPipeplineParameterPushConstant& constInfo = paramInfo.pushConstant;
				parameters[i].InitAsConstants(
					constInfo.sizeInWords, constInfo.shaderRegister, 0, D3D12_SHADER_VISIBILITY_ALL);
			}
			else {
				ZG_ASSERT(false);
			}
		}
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
		desc.Init_1_1(createInfo.numParameters, parameters, 0, nullptr, flags);

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
			//CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
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
			vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());

		// Set pixel shader
		stream.pixelShader = CD3DX12_SHADER_BYTECODE(
			pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());

		// Set render target formats
		// TODO: Probably here Multiple Render Targets (MRT) is specified?
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Same as in our swapchain
		stream.rtvFormats = rtvFormats;

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

	// Allocate pipeline
	D3D12PipelineRendering* pipeline =
		zgNew<D3D12PipelineRendering>(allocator, "ZeroG - D3D12PipelineRendering");

	// Store pipeline state
	pipeline->pipelineState = pipelineState;
	pipeline->rootSignature = rootSignature;
	pipeline->createInfo = createInfo;

	// Return pipeline
	*pipelineOut = pipeline;
	return ZG_SUCCESS;
}

} // namespace zg
