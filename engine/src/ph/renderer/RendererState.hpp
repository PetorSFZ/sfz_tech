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

#include <sfz/containers/DynArray.hpp>
#include <sfz/containers/HashMap.hpp>
#include <sfz/math/Vector.hpp>
#include <sfz/memory/Allocator.hpp>
#include <sfz/strings/StackString.hpp>
#include <sfz/strings/StringID.hpp>

#include <ZeroG-cpp.hpp>

#include "ph/config/GlobalConfig.hpp"
#include "ph/renderer/DynamicGpuAllocator.hpp"
#include "ph/renderer/GpuMesh.hpp"
#include "ph/renderer/ImGuiRenderer.hpp"
#include "ph/renderer/RendererUI.hpp"

struct SDL_Window;

namespace ph {

using sfz::DynArray;
using sfz::HashMap;
using sfz::str128;
using sfz::str256;
using sfz::StringID;
using sfz::vec2_s32;


// Pipeline types
// ------------------------------------------------------------------------------------------------

enum class PipelineSourceType {
	SPIRV = 0,
	HLSL
};

struct SamplerItem final {
	uint32_t samplerRegister = ~0u;
	ZgSampler sampler = {};
};

struct PipelineRenderingItem final {

	zg::PipelineRendering pipeline;

	StringID name;
	PipelineSourceType sourceType = PipelineSourceType::SPIRV;
	str256 vertexShaderPath;
	str256 pixelShaderPath;
	str128 vertexShaderEntry;
	str128 pixelShaderEntry;
	bool standardVertexAttributes = false;
	uint32_t numPushConstants = 0;
	uint32_t pushConstantRegisters[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	uint32_t numNonUserSettableConstantBuffers = 0;
	uint32_t nonUserSettableConstantBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS] = {};
	uint32_t numSamplers = 0;
	SamplerItem samplers[ZG_MAX_NUM_SAMPLERS];
	bool depthTest = false;
	ZgDepthFunc depthFunc = ZG_DEPTH_FUNC_LESS;
	bool cullingEnabled = false;
	bool cullFrontFacing = false;
	bool frontFacingIsCounterClockwise = false;
	bool wireframeRenderingEnabled = false;
};

// Stage types
// ------------------------------------------------------------------------------------------------

// The type of stage
enum class StageType {

	// A rendering pass (i.e. rendering pipeline) where all the draw calls are provided by the user
	// through code.
	USER_INPUT_RENDERING,

	// A barrier that ensures the stages before has finished executing before the stages afterward
	// starts. The user must manually (through code) check this barrier before it can be passed.
	USER_STAGE_BARRIER
};

struct ConstantBufferMemory {
	uint64_t lastFrameIdxTouched = 0;
	uint32_t shaderRegister = ~0u;
	zg::Buffer uploadBuffer;
	zg::Buffer deviceBuffer;
};

struct Stage final {
	StringID stageName;
	StageType stageType;
	StringID renderingPipelineName;
	DynArray<Framed<ConstantBufferMemory>> constantBuffers;
};

// Texture plus info
// ------------------------------------------------------------------------------------------------

struct TextureItem final {
	zg::Texture2D texture;
	ZgTexture2DFormat format = ZG_TEXTURE_2D_FORMAT_UNDEFINED;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t numMipmaps = 0;
};

// RendererState
// ------------------------------------------------------------------------------------------------

struct RendererConfigurableState final {

	// Pipelines
	DynArray<PipelineRenderingItem> renderingPipelines;

	// Present Queue Stages
	DynArray<Stage> presentQueueStages;
};

struct RendererState final {

	// Members
	// --------------------------------------------------------------------------------------------

	sfz::Allocator* allocator = nullptr;
	zg::Context zgCtx;
	SDL_Window* window = nullptr;

	// The current index of the frame, increments at every frameBegin()
	uint64_t currentFrameIdx = 0;

	vec2_s32 windowRes = vec2_s32(0);
	zg::Framebuffer windowFramebuffer;
	zg::CommandQueue presentQueue;
	zg::CommandQueue copyQueue;

	// Dynamic memory allocator
	DynamicGpuAllocator dynamicAllocator;
	HashMap<StringID, TextureItem> textures;
	HashMap<StringID, GpuMesh> meshes;

	// UI
	RendererUI ui;

	// Imgui renderer
	ImGuiRenderer imguiRenderer;

	// Settings
	const Setting* flushPresentQueueEachFrame = nullptr;
	const Setting* flushCopyQueueEachFrame = nullptr;

	// Configurable state
	RendererConfigurableState configurable;

	// The current stage set index
	// Note that all stages until the next stage barrier is active simulatenously.
	uint32_t currentStageSetIdx = 0;

	// The current input-enabled stage
	// Note: The current input-enabled stage must be part of the current stage set
	uint32_t currentInputEnabledStageIdx = ~0u;
	Stage* currentInputEnabledStage = nullptr;
	PipelineRenderingItem* currentPipelineRendering = nullptr;
	zg::CommandList currentCommandList;

	// Helper methods
	// --------------------------------------------------------------------------------------------

	// Gets the index of the next barrier stage, starting from the current stage set index
	// Returns ~0u if no barrier stage is found
	uint32_t findNextBarrierIdx() const noexcept;

	// Finds the index of the specified stage among the current actives ones (i.e. the ones from the
	// current set index to the next stage barrier). Returns ~0u if stage is not among the current
	// active set.
	uint32_t findActiveStageIdx(StringID stageName) const noexcept;

	// Finds the index of the specified rendering pipeline. Returns ~0u if it does not exist.
	uint32_t findPipelineRenderingIdx(StringID pipelineName) const noexcept;

	// Finds the current constant buffer's memory for the current input stage given its shader
	// register.
	//
	// Returns nullptr if not found
	PerFrame<ConstantBufferMemory>* findConstantBufferInCurrentInputStage(
		uint32_t shaderRegister) noexcept;
};

} // namespace ph
