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

// D3D12PipelineBindingsSignature
// ------------------------------------------------------------------------------------------------

struct D3D12PipelineBindingsSignature final {

	ArrayLocal<ZgConstantBufferBindingDesc, ZG_MAX_NUM_CONSTANT_BUFFERS> constBuffers;
	ArrayLocal<ZgUnorderedBufferBindingDesc, ZG_MAX_NUM_UNORDERED_BUFFERS> unorderedBuffers;
	ArrayLocal<ZgTextureBindingDesc, ZG_MAX_NUM_TEXTURES> textures;
	ArrayLocal<ZgUnorderedTextureBindingDesc, ZG_MAX_NUM_UNORDERED_TEXTURES> unorderedTextures;

	D3D12PipelineBindingsSignature() = default;
	D3D12PipelineBindingsSignature(const D3D12PipelineBindingsSignature&) = default;
	D3D12PipelineBindingsSignature& operator= (const D3D12PipelineBindingsSignature&) = default;

	explicit D3D12PipelineBindingsSignature(const ZgPipelineBindingsSignature& signature)
	{
		this->constBuffers.add(signature.constBuffers, signature.numConstBuffers);
		this->unorderedBuffers.add(signature.unorderedBuffers, signature.numUnorderedBuffers);
		this->textures.add(signature.textures, signature.numTextures);
		this->unorderedTextures.add(signature.unorderedTextures, signature.numUnorderedTextures);
	}

	ZgPipelineBindingsSignature toZgSignature() const
	{
		ZgPipelineBindingsSignature signature = {};
		
		for (uint32_t i = 0; i < constBuffers.size(); i++) {
			signature.constBuffers[i] = constBuffers[i];
		}
		signature.numConstBuffers = constBuffers.size();

		for (uint32_t i = 0; i < unorderedBuffers.size(); i++) {
			signature.unorderedBuffers[i] = unorderedBuffers[i];
		}
		signature.numUnorderedBuffers = unorderedBuffers.size();

		for (uint32_t i = 0; i < textures.size(); i++) {
			signature.textures[i] = textures[i];
		}
		signature.numTextures = textures.size();

		for (uint32_t i = 0; i < unorderedTextures.size(); i++) {
			signature.unorderedTextures[i] = unorderedTextures[i];
		}
		signature.numUnorderedTextures = unorderedTextures.size();
		
		return signature;
	}
};

// D3D12RootSignature
// ------------------------------------------------------------------------------------------------

// A D3D12 root signature can at most contain 64 32-bit parameters, these parameters can contain
// push constants directly, inline descriptors (currently unused in ZeroG) or descriptors to
// tables of descriptors.
//
// In ZeroG we currently place all the push constants at the top, then we have a parameter
// containing a descriptor pointing to a table with all SRVs, UAVs and CBVs.

struct D3D12PushConstantMapping final {
	uint32_t bufferRegister = ~0u;
	uint32_t parameterIndex = ~0u;
	uint32_t sizeInBytes = ~0u;
};

struct D3D12ConstantBufferMapping final {
	uint32_t bufferRegister = ~0u;
	uint32_t tableOffset = ~0u;
	uint32_t sizeInBytes = ~0u;
};

struct D3D12TextureMapping final {
	uint32_t textureRegister = ~0u;
	uint32_t tableOffset = ~0u;
};

struct D3D12UnorderedBufferMapping final {
	uint32_t unorderedRegister = ~0u;
	uint32_t tableOffset = ~0u;
};

struct D3D12UnorderedTextureMapping final {
	uint32_t unorderedRegister = ~0u;
	uint32_t tableOffset = ~0u;
};

struct D3D12RootSignature final {
	ComPtr<ID3D12RootSignature> rootSignature;
	uint32_t dynamicBuffersParameterIndex = ~0u;
	ArrayLocal<D3D12PushConstantMapping, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstants;
	ArrayLocal<D3D12ConstantBufferMapping, ZG_MAX_NUM_CONSTANT_BUFFERS> constBuffers;
	ArrayLocal<D3D12TextureMapping, ZG_MAX_NUM_TEXTURES> textures;
	ArrayLocal<D3D12UnorderedBufferMapping, ZG_MAX_NUM_UNORDERED_BUFFERS> unorderedBuffers;
	ArrayLocal<D3D12UnorderedTextureMapping, ZG_MAX_NUM_UNORDERED_TEXTURES> unorderedTextures;

	const D3D12PushConstantMapping* getPushConstantMapping(uint32_t bufferRegister) const noexcept;
	const D3D12ConstantBufferMapping* getConstBufferMapping(uint32_t bufferRegister) const noexcept;
	const D3D12TextureMapping* getTextureMapping(uint32_t textureRegister) const noexcept;
	const D3D12UnorderedBufferMapping* getUnorderedBufferMapping(uint32_t unorderedRegister) const noexcept;
	const D3D12UnorderedTextureMapping* getUnorderedTextureMapping(uint32_t unorderedRegister) const noexcept;
};

// D3D12PipelineCompute
// ------------------------------------------------------------------------------------------------

class D3D12PipelineCompute final : public ZgPipelineCompute {
public:

	D3D12PipelineCompute() noexcept {}
	D3D12PipelineCompute(const D3D12PipelineCompute&) = delete;
	D3D12PipelineCompute& operator= (const D3D12PipelineCompute&) = delete;
	D3D12PipelineCompute(D3D12PipelineCompute&&) = delete;
	D3D12PipelineCompute& operator= (D3D12PipelineCompute&&) = delete;
	~D3D12PipelineCompute() noexcept {}

	ComPtr<ID3D12PipelineState> pipelineState;
	D3D12RootSignature rootSignature;
	D3D12PipelineBindingsSignature bindingsSignature;
};

// D3D12PipelineRender
// ------------------------------------------------------------------------------------------------

class D3D12PipelineRender final : public ZgPipelineRender {
public:

	D3D12PipelineRender() noexcept {}
	D3D12PipelineRender(const D3D12PipelineRender&) = delete;
	D3D12PipelineRender& operator= (const D3D12PipelineRender&) = delete;
	D3D12PipelineRender(D3D12PipelineRender&&) = delete;
	D3D12PipelineRender& operator= (D3D12PipelineRender&&) = delete;
	~D3D12PipelineRender() noexcept {}

	ComPtr<ID3D12PipelineState> pipelineState;
	D3D12RootSignature rootSignature;
	D3D12PipelineBindingsSignature bindingsSignature;
	ZgPipelineRenderSignature renderSignature = {};
	ZgPipelineRenderCreateInfo createInfo = {}; // The info used to create the pipeline 
};

// D3D12PipelineCompute functions
// ------------------------------------------------------------------------------------------------

ZgResult createPipelineComputeFileHLSL(
	D3D12PipelineCompute** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
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
