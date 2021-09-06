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

namespace sfz {

// Shader
// ------------------------------------------------------------------------------------------------

enum class ShaderType : u32 {
	RENDER = 0,
	COMPUTE = 1
};

struct SamplerItem final {
	u32 samplerRegister = ~0u;
	ZgSampler sampler = {};

	SamplerItem() = default;
	SamplerItem(u32 samplerRegisterIn, ZgSamplingMode sampling, ZgWrappingMode wrapping)
	{
		samplerRegister = samplerRegisterIn;
		sampler.samplingMode = sampling;
		sampler.wrappingModeU = wrapping;
		sampler.wrappingModeV = wrapping;
	}
};

enum class PipelineBlendMode  : u32 {
	NO_BLENDING = 0,
	ALPHA_BLENDING,
	ADDITIVE_BLENDING
};

struct VertexInputLayout final {
	bool standardVertexLayout = false;
	u32 vertexSizeBytes = 0;
	ArrayLocal<ZgVertexAttribute, ZG_MAX_NUM_VERTEX_ATTRIBUTES> attributes;
};

struct ShaderRender final {
	// The pipeline
	zg::PipelineRender pipeline;

	// Parsed information
	str32 vertexShaderEntry = "VSMain";
	str32 pixelShaderEntry = "PSMain";
	VertexInputLayout inputLayout;
	ArrayLocal<ZgFormat, ZG_MAX_NUM_RENDER_TARGETS> renderTargets;
	ZgComparisonFunc depthFunc = ZG_COMPARISON_FUNC_NONE;
	bool cullingEnabled = false;
	bool cullFrontFacing = false;
	bool frontFacingIsCounterClockwise = false;
	i32 depthBias = 0;
	f32 depthBiasSlopeScaled = 0.0f;
	f32 depthBiasClamp = 0.0f;
	bool wireframeRenderingEnabled = false;
	PipelineBlendMode blendMode = PipelineBlendMode::NO_BLENDING;
};

struct ShaderCompute final {
	// The pipeline
	zg::PipelineCompute pipeline;

	// Parsed information
	str32 computeShaderEntry = "CSMain";
};

struct Shader final {
	strID name;
	time_t lastModified = 0;
	ShaderType type = ShaderType::RENDER;
	str192 shaderPath;
	ShaderRender render;
	ShaderCompute compute;
	ArrayLocal<u32, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstRegisters;
	ArrayLocal<SamplerItem, ZG_MAX_NUM_SAMPLERS> samplers;

	bool build() noexcept;
};

// ShaderManager
// ------------------------------------------------------------------------------------------------

struct ShaderManagerState;

class ShaderManager final {
public:
	SFZ_DECLARE_DROP_TYPE(ShaderManager);
	void init(u32 maxNumShaders, SfzAllocator* allocator) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void update();
	void renderDebugUI();

	SfzHandle getShaderHandle(const char* name) const;
	SfzHandle getShaderHandle(strID name) const;
	Shader* getShader(SfzHandle handle);
	SfzHandle addShader(Shader&& shader);
	void removeShader(strID name);

private:
	ShaderManagerState* mState = nullptr;
};

} // namespace sfz
