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

#pragma once

#include <mutex>

#include "ZeroG.h"
#include "ZeroG/d3d12/D3D12Common.hpp"
#include "ZeroG/BackendInterface.hpp"

namespace zg {

// Shader register mappings
// ------------------------------------------------------------------------------------------------

struct D3D12PushConstantMapping {
	uint32_t shaderRegister = ~0u;
	uint32_t parameterIndex = ~0u;
	uint32_t sizeInBytes = ~0u;
};

struct D3D12ConstantBufferMapping {
	uint32_t shaderRegister = ~0u;
	uint32_t tableOffset = ~0u;
	uint32_t sizeInBytes = ~0u;
};

struct D3D12TextureMapping {
	uint32_t textureRegister = ~0u;
	uint32_t tableOffset = ~0u;
};

// D3D12PipelineCompute
// ------------------------------------------------------------------------------------------------

class D3D12PipelineCompute final : public ZgPipelineCompute {
public:

	D3D12PipelineCompute() noexcept = default;
	D3D12PipelineCompute(const D3D12PipelineCompute&) = delete;
	D3D12PipelineCompute& operator= (const D3D12PipelineCompute&) = delete;
	D3D12PipelineCompute(D3D12PipelineCompute&&) = delete;
	D3D12PipelineCompute& operator= (D3D12PipelineCompute&&) = delete;
	~D3D12PipelineCompute() noexcept {}

	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
};

// D3D12PipelineRender
// ------------------------------------------------------------------------------------------------

class D3D12PipelineRender final : public ZgPipelineRender {
public:

	D3D12PipelineRender() noexcept = default;
	D3D12PipelineRender(const D3D12PipelineRender&) = delete;
	D3D12PipelineRender& operator= (const D3D12PipelineRender&) = delete;
	D3D12PipelineRender(D3D12PipelineRender&&) = delete;
	D3D12PipelineRender& operator= (D3D12PipelineRender&&) = delete;
	~D3D12PipelineRender() noexcept {}

	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
	ZgPipelineBindingsSignature bindingsSignature = {};
	ZgPipelineRenderSignature renderSignature = {};
	uint32_t numPushConstants = 0;
	D3D12PushConstantMapping pushConstants[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	uint32_t numConstantBuffers = 0;
	D3D12ConstantBufferMapping constBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	uint32_t numTextures = 0;
	D3D12TextureMapping textures[ZG_MAX_NUM_TEXTURES] = {};
	uint32_t dynamicBuffersParameterIndex = ~0u;
	ZgPipelineRenderCreateInfo createInfo = {}; // The info used to create the pipeline 
};

// D3D12PipelineCompute functions
// ------------------------------------------------------------------------------------------------

ZgResult createPipelineComputeFileHLSL(
	D3D12PipelineCompute** pipelineOut,
	const ZgPipelineComputeCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept;

// D3D12PipelineRender functions
// ------------------------------------------------------------------------------------------------

ZgResult createPipelineRenderFileSPIRV(
	D3D12PipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	ZgPipelineRenderCreateInfo createInfo,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept;

ZgResult createPipelineRenderFileHLSL(
	D3D12PipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept;

ZgResult createPipelineRenderSourceHLSL(
	D3D12PipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo& createInfo,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device) noexcept;

} // namespace zg
