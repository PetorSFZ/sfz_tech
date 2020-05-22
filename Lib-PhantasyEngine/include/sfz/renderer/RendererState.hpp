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

#include <ZeroG.h>

#include <ZeroG-ImGui.hpp>

#include "sfz/config/GlobalConfig.hpp"
#include "sfz/renderer/GpuMesh.hpp"
#include "sfz/renderer/RendererUI.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/strings/StringID.hpp"

struct SDL_Window;

namespace sfz {

// Pipeline types
// ------------------------------------------------------------------------------------------------

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
	str256 vertexShaderPath;
	str256 pixelShaderPath;
	str128 vertexShaderEntry;
	str128 pixelShaderEntry;
	bool standardVertexAttributes = false;
	ArrayLocal<uint32_t, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstRegisters;
	ArrayLocal<uint32_t, ZG_MAX_NUM_CONSTANT_BUFFERS> nonUserSettableConstBuffers;
	ArrayLocal<SamplerItem, ZG_MAX_NUM_SAMPLERS> samplers;
	ArrayLocal<ZgTextureFormat, ZG_MAX_NUM_RENDER_TARGETS> renderTargets;
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

struct PipelineComputeItem final {

	// The pipeline
	zg::PipelineCompute pipeline;

	// Parsed information
	StringID name;
	str256 computeShaderPath;
	str128 computeShaderEntry;
	ArrayLocal<uint32_t, ZG_MAX_NUM_CONSTANT_BUFFERS> pushConstRegisters;
	ArrayLocal<uint32_t, ZG_MAX_NUM_CONSTANT_BUFFERS> nonUserSettableConstBuffers;
	ArrayLocal<SamplerItem, ZG_MAX_NUM_SAMPLERS> samplers;

	// Method for building pipeline given the parsed information
	bool buildPipeline() noexcept;
};

// Static resources
// ------------------------------------------------------------------------------------------------

struct StaticTextureItem final {
	
	// The texture
	zg::Texture2D texture;

	// Parsed information
	StringID name;
	ZgTextureFormat format = ZG_TEXTURE_FORMAT_UNDEFINED;
	float clearValue = 0.0f;
	bool resolutionIsFixed = false;
	float resolutionScale = 1.0f;
	Setting* resolutionScaleSetting = nullptr;
	vec2_i32 resolutionFixed = vec2_i32(0);

	// Allocation and deallocating the static texture using the parsed information
	void buildTexture(vec2_i32 windowRes) noexcept;
};

// Stage types
// ------------------------------------------------------------------------------------------------

// The type of stage
enum class StageType {

	// A rendering pass (i.e. rendering pipeline) where all the draw calls are provided by the user
	// through code.
	USER_INPUT_RENDERING,

	// A compute pass (i.e. compute pipeline) where all the dispatches are provided by the user
	// through code
	USER_INPUT_COMPUTE
};

struct ConstantBufferMemory final {
	uint64_t lastFrameIdxTouched = 0;
	uint32_t shaderRegister = ~0u;
	zg::Buffer uploadBuffer;
	zg::Buffer deviceBuffer;
};

struct BoundTexture final {
	uint32_t textureRegister = ~0u;
	StringID textureName = StringID::invalid();
};

struct Stage final {

	Array<PerFrameData<ConstantBufferMemory>> constantBuffers;

	// Parsed information
	StringID name;
	StageType type;
	struct {
		zg::Framebuffer framebuffer;
		StringID pipelineName;
		ArrayLocal<StringID, ZG_MAX_NUM_RENDER_TARGETS> renderTargetNames;
		StringID depthBufferName;
		bool defaultFramebuffer = false;
	} render;
	struct {
		StringID pipelineName;
	} compute;
	ArrayLocal<BoundTexture, ZG_MAX_NUM_TEXTURES> boundTextures;
	ArrayLocal<BoundTexture, ZG_MAX_NUM_UNORDERED_TEXTURES> boundUnorderedTextures;

	void rebuildFramebuffer(Array<StaticTextureItem>& staticTextures) noexcept;
};

struct StageGroup final {
	StringID groupName = StringID::invalid();
	Array<Stage> stages;
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

struct StageCommandList final {
	StringID stageName;
	zg::CommandList commandList;
};

struct GroupProfilingID final {
	StringID groupName = StringID::invalid();
	uint64_t id = ~0ull;
};

struct FrameProfilingIDs final {
	uint64_t frameId = ~0ull;
	uint64_t imguiId = ~0ull;
	ArrayLocal<GroupProfilingID, 64> groupIds;
};

struct RendererConfigurableState final {

	// Path to current configuration
	str320 configPath;

	// Pipelines
	Array<PipelineRenderItem> renderPipelines;
	Array<PipelineComputeItem> computePipelines;

	// Static resources
	Array<StaticTextureItem> staticTextures;

	// Present Queue
	Array<StageGroup> presentQueue;
};

struct RendererState final {

	// Members
	// --------------------------------------------------------------------------------------------

	sfz::Allocator* allocator = nullptr;
	SDL_Window* window = nullptr;

	// Whether the renderer is in "dummy" mode or not. Dummy mode is used when the renderer is
	// bypassed by the application so that it can render using ZeroG directly. The renderer still
	// owns "the ZeroG swapbuffer" and ImGui rendering.
	bool dummyMode = false;

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
	PerFrameData<FrameProfilingIDs> frameMeasurementIds;
	float lastRetrievedFrameTimeMs = 0.0f;
	uint64_t lastRetrievedFrameTimeFrameIdx = ~0ull;

	// GPU resources
	HashMap<StringID, TextureItem> textures;
	HashMap<StringID, GpuMesh> meshes;

	// UI
	RendererUI ui;

	// Imgui renderer
	const Setting* imguiScaleSetting = nullptr;
	zg::ImGuiRenderState* imguiRenderState = nullptr;;

	// Settings
	const Setting* vsync = nullptr;
	const Setting* flushPresentQueueEachFrame = nullptr;
	const Setting* flushCopyQueueEachFrame = nullptr;

	// Configurable state
	RendererConfigurableState configurable;

	// The currently active stage group
	uint32_t currentStageGroupIdx = 0;
	ArrayLocal<StageCommandList, 32> groupCommandLists;

	// The current input-enabled stage
	struct {
		bool inInputMode = false;
		uint32_t stageIdx = ~0u;
		Stage* stage = nullptr;
		PipelineRenderItem* pipelineRender = nullptr;
		PipelineComputeItem* pipelineCompute = nullptr;
		StageCommandList* commandList = nullptr;
	} inputEnabled;

	// Helper methods
	// --------------------------------------------------------------------------------------------

	// Finds the index of the specified stage among the current actives ones (i.e. the ones from the
	// current set index to the next stage barrier). Returns ~0u if stage is not among the current
	// active set.
	uint32_t findActiveStageIdx(StringID stageName) const noexcept;

	StageCommandList* getStageCommandList(StringID stageName) noexcept;
	
	zg::CommandList& inputEnabledCommandList() noexcept;

	// Finds the index of the specified pipeline. Returns ~0u if it does not exist.
	uint32_t findPipelineRenderIdx(StringID pipelineName) const noexcept;
	uint32_t findPipelineComputeIdx(StringID pipelineName) const noexcept;

	// Finds the current constant buffer's memory for the current input stage given its shader
	// register.
	//
	// Returns nullptr if not found
	ConstantBufferMemory* findConstantBufferInCurrentInputStage(uint32_t shaderRegister) noexcept;
};

} // namespace sfz
