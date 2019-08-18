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

// D3D12 PipelineRendering
// ------------------------------------------------------------------------------------------------

class D3D12PipelineRendering final : public ZgPipelineRendering {
public:

	D3D12PipelineRendering() noexcept = default;
	D3D12PipelineRendering(const D3D12PipelineRendering&) = delete;
	D3D12PipelineRendering& operator= (const D3D12PipelineRendering&) = delete;
	D3D12PipelineRendering(D3D12PipelineRendering&&) = delete;
	D3D12PipelineRendering& operator= (D3D12PipelineRendering&&) = delete;
	~D3D12PipelineRendering() noexcept;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	ZgPipelineRenderingSignature signature = {};
	uint32_t numPushConstants = 0;
	D3D12PushConstantMapping pushConstants[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	uint32_t numConstantBuffers = 0;
	D3D12ConstantBufferMapping constBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	uint32_t numTextures = 0;
	D3D12TextureMapping textures[ZG_MAX_NUM_TEXTURES] = {};
	uint32_t dynamicBuffersParameterIndex = ~0u;
	ZgPipelineRenderingCreateInfoCommon createInfo = {}; // The info used to create the pipeline 
};

// D3D12 PipelineRendering functions
// ------------------------------------------------------------------------------------------------

ZgErrorCode createPipelineRenderingFileSPIRV(
	D3D12PipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	ZgPipelineRenderingCreateInfoFileSPIRV createInfo,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	ID3D12Device3& device) noexcept;

ZgErrorCode createPipelineRenderingFileHLSL(
	D3D12PipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfoFileHLSL& createInfo,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	ID3D12Device3& device) noexcept;

ZgErrorCode createPipelineRenderingSourceHLSL(
	D3D12PipelineRendering** pipelineOut,
	ZgPipelineRenderingSignature* signatureOut,
	const ZgPipelineRenderingCreateInfoSourceHLSL& createInfo,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	ID3D12Device3& device) noexcept;

} // namespace zg
