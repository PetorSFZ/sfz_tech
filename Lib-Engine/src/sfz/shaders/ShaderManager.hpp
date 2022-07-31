// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include <ctime>

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_pool.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

struct SfzConfig;

// Shader
// ------------------------------------------------------------------------------------------------

enum class SfzShaderType : u32 {
	RENDER = 0,
	COMPUTE = 1
};

struct SfzShader final {
	SfzStrID name = SFZ_STR_ID_NULL;
	i64 lastModified = 0;
	SfzShaderType type = SfzShaderType::RENDER;
	ZgPipelineCompileSettingsHLSL compileSettings = {};

	ZgPipelineRenderDesc renderDesc = {};
	zg::PipelineRender renderPipeline;

	ZgPipelineComputeDesc computeDesc = {};
	zg::PipelineCompute computePipeline;

	bool build() noexcept;
};

// SfzShaderManager
// ------------------------------------------------------------------------------------------------

struct SfzShaderManagerState;

struct SfzShaderManager final {
public:
	SFZ_DECLARE_DROP_TYPE(SfzShaderManager);
	void init(u32 maxNumShaders, SfzConfig* cfg, SfzAllocator* allocator) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void update();
	void renderDebugUI(SfzStrIDs* ids);

	SfzHandle getShaderHandle(SfzStrIDs* ids, const char* name) const;
	SfzHandle getShaderHandle(SfzStrID name) const;
	SfzShader* getShader(SfzHandle handle);
	SfzHandle addShaderRender(
		const ZgPipelineRenderDesc& desc,
		const ZgPipelineCompileSettingsHLSL& settings,
		SfzStrIDs* ids);
	SfzHandle addShaderCompute(
		const ZgPipelineComputeDesc& desc,
		const ZgPipelineCompileSettingsHLSL& settings,
		SfzStrIDs* ids);
	void removeShader(SfzStrID name);

private:
	SfzShaderManagerState* mState = nullptr;
};
