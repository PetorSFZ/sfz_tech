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

#include "ZeroG/d3d12/D3D12Common.hpp"
#include "ZeroG/BackendInterface.hpp"
#include "ZeroG/ZeroG-CApi.h"

namespace zg {

// D3D12 PipelineRendering
// ------------------------------------------------------------------------------------------------

class D3D12PipelineRendering final : public IPipelineRendering {
public:

	D3D12PipelineRendering() noexcept = default;
	D3D12PipelineRendering(const D3D12PipelineRendering&) = delete;
	D3D12PipelineRendering& operator= (const D3D12PipelineRendering&) = delete;
	D3D12PipelineRendering(D3D12PipelineRendering&&) = delete;
	D3D12PipelineRendering& operator= (D3D12PipelineRendering&&) = delete;
	~D3D12PipelineRendering() noexcept;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
};

// D3D12 PipelineRendering functions
// ------------------------------------------------------------------------------------------------

ZgErrorCode createPipelineRendering(
	D3D12PipelineRendering** pipelineOut,
	const ZgPipelineRenderingCreateInfo& createInfo,
	IDxcLibrary& dxcLibrary,
	IDxcCompiler& dxcCompiler,
	ZgAllocator& allocator,
	ID3D12Device5& device,
	std::mutex& contextMutex) noexcept;

} // namespace zg
