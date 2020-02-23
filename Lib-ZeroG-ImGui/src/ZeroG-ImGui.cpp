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

#include "ZeroG-ImGui.hpp"

#include <skipifzero_arrays.hpp>
#include <skipifzero_strings.hpp>

#include <imgui.h>

namespace zg {

using sfz::vec2;
using sfz::vec3;
using sfz::vec4;

// Helper structs
// ------------------------------------------------------------------------------------------------

// TODO: Remove
struct ImGuiVertex final {
	vec2 pos;
	vec2 texcoord;
	vec4 color;
};
static_assert(sizeof(ImGuiVertex) == 32, "ImGuiVertex is padded");

// TODO: Remove
struct ImGuiCommand {
	uint32_t idxBufferOffset = 0;
	uint32_t numIndices = 0;
	uint32_t padding[2];
	sfz::vec4 clipRect = sfz::vec4(0.0f);
};
static_assert(sizeof(ImGuiCommand) == sizeof(uint32_t) * 8, "ImguiCommand is padded");

// ImGui Renderer
// ------------------------------------------------------------------------------------------------

struct ImGuiFrameState final {
	zg::Fence fence;
	zg::Buffer uploadVertexBuffer;
	zg::Buffer uploadIndexBuffer;
};

struct ImGuiRenderState final {
	sfz::Allocator* allocator = nullptr;

	// Pipeline used to render ImGui
	zg::PipelineRender pipeline;

	// Font texture
	zg::MemoryHeap fontTextureHeap;
	zg::Texture2D fontTexture;

	// Memory used to upload ImgGi vertices and indices for a given frame
	zg::MemoryHeap uploadHeap;

	// Per frame state
	sfz::Array<ImGuiFrameState> frameStates;
	ImGuiFrameState& getFrameState(uint64_t idx) { return frameStates[idx % frameStates.size()]; }

	// Temp arrays
	// TODO: Remove
	sfz::Array<ImGuiVertex> tmpVertices;
	sfz::Array<uint32_t> tmpIndices;
	sfz::Array<ImGuiCommand> tmpCommands;
};

// Constants
// ------------------------------------------------------------------------------------------------

constexpr uint32_t IMGUI_MAX_NUM_VERTICES = 32768;
constexpr uint32_t IMGUI_MAX_NUM_INDICES = 32768;
constexpr uint64_t IMGUI_VERTEX_BUFFER_SIZE = IMGUI_MAX_NUM_VERTICES * sizeof(ImGuiVertex);
constexpr uint64_t IMGUI_INDEX_BUFFER_SIZE = IMGUI_MAX_NUM_INDICES * sizeof(uint32_t);

// Shader source
// ------------------------------------------------------------------------------------------------

static constexpr char IMGUI_SHADER_HLSL_SRC[] = R"(

cbuffer TransformsCB : register(b0) {
	row_major float4x4 projMatrix;
}

struct VSInput {
	float2 position : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
	float4 color : TEXCOORD2;
};

struct VSOutput {
	float2 texcoord : PARAM_0;
	float4 color : PARAM_1;
	float4 position : SV_Position;
};

struct PSInput {
	float2 texcoord : PARAM_0;
	float4 color : PARAM_1;
};

Texture2D fontTexture : register(t0);

SamplerState fontSampler : register(s0);

VSOutput VSMain(VSInput input)
{
	VSOutput output;

	output.texcoord = input.texcoord;
	output.color = input.color;

	output.position = mul(projMatrix, float4(input.position, 0.0f, 1.0f));

	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float fontAlpha = fontTexture.Sample(fontSampler, input.texcoord).r;
	return float4(input.color.rgb, input.color.a * fontAlpha);
}

)";

// Error handling helpers
// -----------------------------------------------------------------------------------------------

struct ZgAsserter final { void operator% (zg::Result res) { sfz_assert(zg::isSuccess(res)); }; };

#define ASSERT_ZG ZgAsserter() %

// ImGui Renderer
// ------------------------------------------------------------------------------------------------

zg::Result imguiInitRenderState(
	ImGuiRenderState*& stateOut,
	uint32_t frameLatency,
	sfz::Allocator* allocator,
	zg::CommandQueue& copyQueue,
	const ZgImageViewConstCpu& fontTexture) noexcept
{
	sfz_assert(stateOut == nullptr);

	// Allocate state
	stateOut = allocator->newObject<ImGuiRenderState>(sfz_dbg("ImGuiRenderState"));
	stateOut->allocator = allocator;

	// Build ImGui pipeline
	{
		zg::Result res = zg::PipelineRenderBuilder()
			.addVertexAttribute(0, 0, ZG_VERTEX_ATTRIBUTE_F32_2, offsetof(ImGuiVertex, pos))
			.addVertexAttribute(1, 0, ZG_VERTEX_ATTRIBUTE_F32_2, offsetof(ImGuiVertex, texcoord))
			.addVertexAttribute(2, 0, ZG_VERTEX_ATTRIBUTE_F32_4, offsetof(ImGuiVertex, color))
			.addVertexBufferInfo(0, sizeof(ImGuiVertex))
			.addPushConstant(0)
			.addSampler(0, ZG_SAMPLING_MODE_TRILINEAR)
			.addRenderTarget(ZG_TEXTURE_FORMAT_RGBA_U8_UNORM)
			.setCullingEnabled(false)
			.setBlendingEnabled(true)
			.setBlendFuncColor(ZG_BLEND_FUNC_ADD, ZG_BLEND_FACTOR_SRC_ALPHA, ZG_BLEND_FACTOR_SRC_INV_ALPHA)
			.setDepthTestEnabled(false)
			.addVertexShaderSource("VSMain", IMGUI_SHADER_HLSL_SRC)
			.addPixelShaderSource("PSMain", IMGUI_SHADER_HLSL_SRC)
			.buildFromSourceHLSL(stateOut->pipeline, ZG_SHADER_MODEL_6_0);
		if (!zg::isSuccess(res)) return res;
	}

	// Allocate memory for font texture
	sfz_assert_hard(fontTexture.format == ZG_TEXTURE_FORMAT_R_U8_UNORM);
	ZgTexture2DCreateInfo texCreateInfo = {};
	texCreateInfo.format = ZG_TEXTURE_FORMAT_R_U8_UNORM;
	texCreateInfo.width = fontTexture.width;
	texCreateInfo.height = fontTexture.height;
	texCreateInfo.numMipmaps = 1; // TODO: Mipmaps

	ZgTexture2DAllocationInfo texAllocInfo = {};
	ASSERT_ZG zg::Texture2D::getAllocationInfo(texAllocInfo, texCreateInfo);
	texCreateInfo.offsetInBytes = 0;
	texCreateInfo.sizeInBytes = texAllocInfo.sizeInBytes;

	{
		zg::Result res = stateOut->fontTextureHeap.create(
			texAllocInfo.sizeInBytes, ZG_MEMORY_TYPE_TEXTURE);
		if (!zg::isSuccess(res)) return res;
	}

	{
		zg::Result res = stateOut->fontTextureHeap.texture2DCreate(stateOut->fontTexture, texCreateInfo);
		if (!zg::isSuccess(res)) return res;
	}
	ASSERT_ZG stateOut->fontTexture.setDebugName("ImGui_FontTexture");

	// Allocate memory for vertex and index buffer
	const uint64_t uploadHeapNumBytes = (IMGUI_VERTEX_BUFFER_SIZE + IMGUI_INDEX_BUFFER_SIZE) * frameLatency;
	{
		zg::Result res = stateOut->uploadHeap.create(uploadHeapNumBytes, ZG_MEMORY_TYPE_UPLOAD);
		if (!zg::isSuccess(res)) return res;
	}

	// Upload font texture to GPU
	{
		// Utilize vertex and index buffer upload heap to upload font texture
		// Create temporary upload buffer
		zg::Buffer tmpUploadBuffer;
		ASSERT_ZG stateOut->uploadHeap.bufferCreate(tmpUploadBuffer, 0, texAllocInfo.sizeInBytes);

		// Copy to the texture
		zg::CommandList commandList;
		ASSERT_ZG copyQueue.beginCommandListRecording(commandList);
		ASSERT_ZG commandList.memcpyToTexture(stateOut->fontTexture, 0, fontTexture, tmpUploadBuffer);
		ASSERT_ZG commandList.enableQueueTransition(stateOut->fontTexture);
		ASSERT_ZG copyQueue.executeCommandList(commandList);
		ASSERT_ZG copyQueue.flush();
	}

	// Actually create the vertex and index buffers
	uint64_t uploadHeapOffset = 0;
	uint32_t frameStateIdx = 0;
	stateOut->frameStates.init(frameLatency, allocator, sfz_dbg(""));
	for (uint32_t i = 0; i < frameLatency; i++) {
		stateOut->frameStates.add({});
		ImGuiFrameState& frame = stateOut->frameStates.last();

		ASSERT_ZG frame.fence.create();

		ASSERT_ZG stateOut->uploadHeap.bufferCreate(
			frame.uploadVertexBuffer, uploadHeapOffset, IMGUI_VERTEX_BUFFER_SIZE);
		uploadHeapOffset += IMGUI_VERTEX_BUFFER_SIZE;
		ASSERT_ZG frame.uploadVertexBuffer.setDebugName(sfz::str32("ImGui_VertexBuffer_%u", frameStateIdx));

		ASSERT_ZG stateOut->uploadHeap.bufferCreate(
			frame.uploadIndexBuffer, uploadHeapOffset, IMGUI_INDEX_BUFFER_SIZE);
		uploadHeapOffset += IMGUI_INDEX_BUFFER_SIZE;
		ASSERT_ZG frame.uploadIndexBuffer.setDebugName(sfz::str32("ImGui_IndexBuffer_%u", frameStateIdx));

		frameStateIdx += 1;
	}
	sfz_assert(uploadHeapOffset == uploadHeapNumBytes);

	// TODO: Remove
	stateOut->tmpVertices.init(IMGUI_MAX_NUM_VERTICES, allocator, sfz_dbg(""));
	stateOut->tmpIndices.init(IMGUI_MAX_NUM_INDICES, allocator, sfz_dbg(""));
	stateOut->tmpCommands.init(100, allocator, sfz_dbg(""));

	return zg::Result::SUCCESS;
}

void imguiDestroyRenderState(ImGuiRenderState*& state) noexcept
{
	sfz_assert(state != nullptr);
	sfz_assert(state->allocator != nullptr);
	sfz::Allocator* allocator = state->allocator;
	allocator->deallocate(state);
	state = nullptr;
}

void imguiRender(
	ImGuiRenderState* state,
	uint64_t frameIdx,
	zg::CommandQueue& presentQueue,
	zg::Framebuffer& framebuffer,
	float scale,
	zg::Profiler* profiler,
	uint64_t* measurentIdOut) noexcept
{
	// Generate ImGui draw lists and get the draw data
	ImGui::Render();
	ImDrawData& drawData = *ImGui::GetDrawData();

	// Clear old temp data
	state->tmpVertices.clear();
	state->tmpIndices.clear();
	state->tmpCommands.clear();

	// Convert draw data
	for (int i = 0; i < drawData.CmdListsCount; i++) {

		const ImDrawList& cmdList = *drawData.CmdLists[i];

		// indexOffset is the offset to offset all indices with
		const uint32_t indexOffset = state->tmpVertices.size();

		// indexBufferOffset is the offset to where the indices start
		uint32_t indexBufferOffset = state->tmpIndices.size();

		// Convert vertices and add to global list
		for (int j = 0; j < cmdList.VtxBuffer.size(); j++) {
			const ImDrawVert& imguiVertex = cmdList.VtxBuffer[j];

			ImGuiVertex convertedVertex;
			convertedVertex.pos = vec2(imguiVertex.pos.x, imguiVertex.pos.y);
			convertedVertex.texcoord = vec2(imguiVertex.uv.x, imguiVertex.uv.y);
			convertedVertex.color = [&]() {
				vec4 color;
				color.x = float(imguiVertex.col & 0xFFu) * (1.0f / 255.0f);
				color.y = float((imguiVertex.col >> 8u) & 0xFFu)* (1.0f / 255.0f);
				color.z = float((imguiVertex.col >> 16u) & 0xFFu)* (1.0f / 255.0f);
				color.w = float((imguiVertex.col >> 24u) & 0xFFu)* (1.0f / 255.0f);
				return color;
			}();

			state->tmpVertices.add(convertedVertex);
		}

		// Fix indices and add to global list
		for (int j = 0; j < cmdList.IdxBuffer.size(); j++) {
			state->tmpIndices.add(cmdList.IdxBuffer[j] + indexOffset);
		}

		// Create new commands
		for (int j = 0; j < cmdList.CmdBuffer.Size; j++) {
			const ImDrawCmd& inCmd = cmdList.CmdBuffer[j];

			ImGuiCommand cmd;
			cmd.idxBufferOffset = indexBufferOffset;
			cmd.numIndices = inCmd.ElemCount;
			indexBufferOffset += inCmd.ElemCount;
			cmd.clipRect.x = inCmd.ClipRect.x;
			cmd.clipRect.y = inCmd.ClipRect.y;
			cmd.clipRect.z = inCmd.ClipRect.z;
			cmd.clipRect.w = inCmd.ClipRect.w;

			state->tmpCommands.add(cmd);
		}
	}

	sfz_assert_hard(state->tmpVertices.size() < IMGUI_MAX_NUM_VERTICES);
	sfz_assert_hard(state->tmpIndices.size() < IMGUI_MAX_NUM_INDICES);

	// Get current frame's resources and then wait until they are available (i.e. the frame they
	// are part of has finished rendering).
	ImGuiFrameState& imguiFrame = state->getFrameState(frameIdx);
	ASSERT_ZG imguiFrame.fence.waitOnCpuBlocking();

	// Memcpy vertices and indices to imgui upload buffers
	ASSERT_ZG imguiFrame.uploadVertexBuffer.memcpyTo(
		0, state->tmpVertices.data(), state->tmpVertices.size() * sizeof(ImGuiVertex));
	ASSERT_ZG imguiFrame.uploadIndexBuffer.memcpyTo(
		0, state->tmpIndices.data(), state->tmpIndices.size() * sizeof(uint32_t));

	zg::CommandList commandList;
	ASSERT_ZG presentQueue.beginCommandListRecording(commandList);

	// Start profiling if requested
	if (profiler != nullptr) {
		sfz_assert(measurentIdOut != nullptr);
		ASSERT_ZG commandList.profileBegin(*profiler, *measurentIdOut);
	}

	// Set framebuffer
	ASSERT_ZG commandList.setFramebuffer(framebuffer);

	// Set ImGui pipeline
	ASSERT_ZG commandList.setPipeline(state->pipeline);
	ASSERT_ZG commandList.setIndexBuffer(imguiFrame.uploadIndexBuffer, ZG_INDEX_BUFFER_TYPE_UINT32);
	ASSERT_ZG commandList.setVertexBuffer(0, imguiFrame.uploadVertexBuffer);

	// Bind pipeline parameters
	ASSERT_ZG commandList.setPipelineBindings(zg::PipelineBindings()
		.addTexture(0, state->fontTexture));

	// Retrieve imgui scale factor
	float imguiScaleFactor = 1.0f;
	imguiScaleFactor /= scale;
	float imguiInvScaleFactor = 1.0f / imguiScaleFactor;
	float imguiWidth = framebuffer.width * imguiScaleFactor;
	float imguiHeight = framebuffer.height * imguiScaleFactor;

	// Calculate and set ImGui projection matrix
	sfz::vec4 projMatrix[4] = {
		sfz::vec4(2.0f / imguiWidth, 0.0f, 0.0f, -1.0f),
		sfz::vec4(0.0f, 2.0f / -imguiHeight, 0.0f, 1.0f),
		sfz::vec4(0.0f, 0.0f, 0.5f, 0.5f),
		sfz::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	};
	ASSERT_ZG commandList.setPushConstant(0, &projMatrix, sizeof(float) * 16);

	// Render ImGui commands
	for (uint32_t i = 0; i < state->tmpCommands.size(); i++) {
		const ImGuiCommand& cmd = state->tmpCommands[i];
		sfz_assert((cmd.numIndices % 3) == 0);

		ZgFramebufferRect scissorRect = {};
		scissorRect.topLeftX = uint32_t(cmd.clipRect.x * imguiInvScaleFactor);
		scissorRect.topLeftY = uint32_t(cmd.clipRect.y * imguiInvScaleFactor);
		scissorRect.width = uint32_t((cmd.clipRect.z - cmd.clipRect.x) * imguiInvScaleFactor);
		scissorRect.height = uint32_t((cmd.clipRect.w - cmd.clipRect.y) * imguiInvScaleFactor);
		ASSERT_ZG commandList.setFramebufferScissor(scissorRect);

		ASSERT_ZG commandList.drawTrianglesIndexed(cmd.idxBufferOffset, cmd.numIndices / 3);
	}

	// End profiling if requested
	if (profiler != nullptr) {
		sfz_assert(measurentIdOut != nullptr);
		ASSERT_ZG commandList.profileEnd(*profiler, *measurentIdOut);
	}

	// Execute command list
	ASSERT_ZG presentQueue.executeCommandList(commandList);

	// Signal that we have finished rendering this ImGui frame
	ASSERT_ZG presentQueue.signalOnGpu(imguiFrame.fence);
}

} // namespace zg
