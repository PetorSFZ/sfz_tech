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

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG-cpp.hpp>

#include <ZeroG-ImGui.hpp>

#include "sfz/config/GlobalConfig.hpp"
#include "sfz/renderer/DynamicGpuAllocator.hpp"
#include "sfz/renderer/GpuMesh.hpp"
#include "sfz/renderer/RendererUI.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/strings/StringID.hpp"

struct SDL_Window;

namespace sfz {

// Framebuffer types
// ------------------------------------------------------------------------------------------------

struct FramebufferBacked final {
	zg::Framebuffer framebuffer;
	uint32_t numRenderTargets = 0;
	zg::Texture2D renderTargets[ZG_MAX_NUM_RENDER_TARGETS];
	zg::Texture2D depthBuffer;
};

struct RenderTargetItem final {
	ZgTextureFormat format = ZG_TEXTURE_FORMAT_UNDEFINED;
	float clearValue = 0.0f;
};

struct FramebufferItem final {
	
	// The framebuffer
	FramebufferBacked framebuffer;

	// Parsed information
	StringID name;
	bool resolutionIsFixed = false;
	float resolutionScale = 1.0f;
	Setting* resolutionScaleSetting = nullptr;
	vec2_i32 resolutionFixed = vec2_i32(0);
	uint32_t numRenderTargets = 0;
	RenderTargetItem renderTargetItems[ZG_MAX_NUM_RENDER_TARGETS];
	bool hasDepthBuffer = false;
	ZgTextureFormat depthBufferFormat = ZG_TEXTURE_FORMAT_R_F32;
	float depthBufferClearValue = 0.0f;

	// Method for deallocating previous framebuffer
	void deallocate(DynamicGpuAllocator& gpuAllocatorFramebuffer) noexcept;

	// Method for building the framebuffer given the parsed information
	bool buildFramebuffer(vec2_i32 windowRes, DynamicGpuAllocator& gpuAllocatorFramebuffer) noexcept;
};

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

enum class PipelineBlendMode {
	NO_BLENDING = 0,
	ALPHA_BLENDING,
	ADDITIVE_BLENDING
};

struct PipelineRenderItem final {

	// The pipeline
	zg::PipelineRender pipeline;

	// Parsed information
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
	uint32_t numRenderTargets = 0;
	ZgTextureFormat renderTargets[ZG_MAX_NUM_RENDER_TARGETS] = {};
	bool depthTest = false;
	ZgDepthFunc depthFunc = ZG_DEPTH_FUNC_LESS;
	bool cullingEnabled = false;
	bool cullFrontFacing = false;
	bool frontFacingIsCounterClockwise = false;
	int32_t depthBias = 0;
	float depthBiasSlopeScaled = 0.0f;
	float depthBiasClamp = 0.0f;
	bool wireframeRenderingEnabled = false;
	PipelineBlendMode blendMode = PipelineBlendMode::NO_BLENDING;

	// Method for building pipeline given the parsed information
	bool buildPipeline() noexcept;
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

struct ConstantBufferMemory final {
	uint64_t lastFrameIdxTouched = 0;
	uint32_t shaderRegister = ~0u;
	zg::Buffer uploadBuffer;
	zg::Buffer deviceBuffer;
};

struct BoundRenderTarget final {
	uint32_t textureRegister = ~0u;
	StringID framebuffer = StringID::invalid();
	bool depthBuffer = false;
	uint32_t renderTargetIdx = ~0u;
};

struct Stage final {
	StringID stageName = StringID::invalid();
	StageType stageType;
	StringID renderPipelineName = StringID::invalid();
	Array<PerFrameData<ConstantBufferMemory>> constantBuffers;
	StringID framebufferName = StringID::invalid();
	Array<BoundRenderTarget> boundRenderTargets;
};

// Texture plus info
// ------------------------------------------------------------------------------------------------

struct TextureItem final {
	zg::Texture2D texture;
	ZgTextureFormat format = ZG_TEXTURE_FORMAT_UNDEFINED;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t numMipmaps = 0;
};

// RendererState
// ------------------------------------------------------------------------------------------------

struct RendererConfigurableState final {

	// Path to current configuration
	str320 configPath;

	// Framebuffers
	Array<FramebufferItem> framebuffers;

	// Pipelines
	Array<PipelineRenderItem> renderPipelines;

	// Present Queue Stages
	Array<Stage> presentQueueStages;

	// Helper method to get a framebuffer given a StringID, returns nullptr on failure
	zg::Framebuffer* getFramebuffer(zg::Framebuffer& defaultFramebuffer, StringID id) noexcept;
	FramebufferItem* getFramebufferItem(StringID id) noexcept;
};

struct RendererState final {

	// Members
	// --------------------------------------------------------------------------------------------

	sfz::Allocator* allocator = nullptr;
	zg::Context zgCtx;
	SDL_Window* window = nullptr;

	// The current index of the frame, increments at every frameBegin()
	uint64_t currentFrameIdx = 0;

	// Synchronization primitives to make sure we have finished rendering using a given set of
	//" PerFrameData" resources so we can start uploading new data to them.
	uint32_t frameLatency = 2;
	PerFrameData<zg::Fence> frameFences;

	vec2_i32 windowRes = vec2_i32(0);
	zg::Framebuffer windowFramebuffer;
	zg::CommandQueue presentQueue;
	zg::CommandQueue copyQueue;

	// Profiler
	zg::Profiler profiler;
	PerFrameData<uint64_t> frameMeasurementIds;
	float lastRetrievedFrameTimeMs = 0.0f;
	uint64_t lastRetrievedFrameTimeFrameIdx = ~0ull;

	// Dynamic memory allocator
	DynamicGpuAllocator gpuAllocatorUpload;
	DynamicGpuAllocator gpuAllocatorDevice;
	DynamicGpuAllocator gpuAllocatorTexture;
	DynamicGpuAllocator gpuAllocatorFramebuffer;

	// GPU resources
	HashMap<StringID, TextureItem> textures;
	HashMap<StringID, GpuMesh> meshes;

	// UI
	RendererUI ui;

	// Imgui renderer
	const Setting* imguiScaleSetting = nullptr;
	zg::ImGuiRenderState* imguiRenderState = nullptr;;

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
	PipelineRenderItem* currentPipelineRender = nullptr;
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

	// Finds the index of the specified render pipeline. Returns ~0u if it does not exist.
	uint32_t findPipelineRenderIdx(StringID pipelineName) const noexcept;

	// Finds the current constant buffer's memory for the current input stage given its shader
	// register.
	//
	// Returns nullptr if not found
	ConstantBufferMemory* findConstantBufferInCurrentInputStage(uint32_t shaderRegister) noexcept;
};

} // namespace sfz
