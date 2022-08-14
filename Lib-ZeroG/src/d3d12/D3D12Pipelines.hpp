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
#include "d3d12/D3D12Common.hpp"

// D3D12RootSignature
// ------------------------------------------------------------------------------------------------

// A D3D12 root signature can at most contain 64 32-bit parameters, these parameters can contain
// push constants directly, inline descriptors (currently unused in ZeroG) or descriptors to
// tables of descriptors.
//
// In ZeroG we currently place all the push constants at the top, then we have a parameter
// containing a descriptor pointing to a table with all SRVs, UAVs and CBVs.

struct PushConstMapping {
	u32 reg;
	u32 paramIdx; // The parameter in the root signature
	u32 sizeBytes; // Size of the push constant in bytes
};

struct CBVMapping {
	u32 reg;
	u32 tableOffset;
	u32 sizeBytes; // Size of the constant buffer in bytes
};

struct SRVMapping {
	u32 reg;
	u32 tableOffset;
	ZgBindingType type = ZgBindingType::ZG_BINDING_TYPE_UNDEFINED;
};

struct UAVMapping {
	u32 reg;
	u32 tableOffset;
	ZgBindingType type = ZgBindingType::ZG_BINDING_TYPE_UNDEFINED;
};

struct RootSignatureMapping {
	u32 dynamicParamIdx = ~0u;
	u32 dynamicTableSize = 0;
	SfzArrayLocal<PushConstMapping, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConsts;
	SfzArrayLocal<CBVMapping, ZG_MAX_NUM_CONSTANT_BUFFERS> CBVs;
	SfzArrayLocal<SRVMapping, ZG_MAX_NUM_TEXTURES> SRVs;
	SfzArrayLocal<UAVMapping, ZG_MAX_NUM_UNORDERED_TEXTURES> UAVs;
};

// D3D12PipelineCompute
// ------------------------------------------------------------------------------------------------

struct ZgPipelineCompute final {
	ZgPipelineCompute() = default;
	ZgPipelineCompute(const ZgPipelineCompute&) = delete;
	ZgPipelineCompute& operator= (const ZgPipelineCompute&) = delete;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	RootSignatureMapping mapping;
	u32 groupDimX = 0;
	u32 groupDimY = 0;
	u32 groupDimZ = 0;
};

// D3D12PipelineRender
// ------------------------------------------------------------------------------------------------

struct ZgPipelineRender final {
	ZgPipelineRender() = default;
	ZgPipelineRender(const ZgPipelineRender&) = delete;
	ZgPipelineRender& operator= (const ZgPipelineRender&) = delete;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	RootSignatureMapping mapping;
	ZgPipelineRenderSignature renderSignature = {};
	ZgPipelineRenderDesc createInfo = {}; // The info used to create the pipeline 
};

// D3D12PipelineCompute functions
// ------------------------------------------------------------------------------------------------

ZgResult createPipelineComputeFileHLSL(
	ZgPipelineCompute** pipelineOut,
	const ZgPipelineComputeDesc& desc,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device,
	const char* pipelineCacheDir) noexcept;

// D3D12PipelineRender functions
// ------------------------------------------------------------------------------------------------

ZgResult createPipelineRenderFileHLSL(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderDesc& desc,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device,
	const char* pipelineCacheDir) noexcept;

ZgResult createPipelineRenderSourceHLSL(
	ZgPipelineRender** pipelineOut,
	const char* src,
	const ZgPipelineRenderDesc& desc,
	const ZgPipelineCompileSettingsHLSL& compileSettings,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	IDxcIncludeHandler* dxcIncludeHandler,
	ID3D12Device3& device,
	const char* pipelineCacheDir) noexcept;
