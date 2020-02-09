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

#include <utility> // std::swap()

#include <skipifzero_strings.hpp>

#include <imgui.h>

namespace zg {

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
	ImGuiRenderState& state,
	uint32_t frameLatency,
	sfz::Allocator* allocator,
	zg::CommandQueue& copyQueue,
	const ZgImageViewConstCpu& fontTexture) noexcept
{
	sfz_assert(state.allocator == nullptr);

	// Init some general stuff
	state.allocator = allocator;

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
			.buildFromSourceHLSL(state.pipeline, ZG_SHADER_MODEL_6_0);
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
		zg::Result res = state.fontTextureHeap.create(texAllocInfo.sizeInBytes, ZG_MEMORY_TYPE_TEXTURE);
		if (!zg::isSuccess(res)) return res;
	}

	{
		zg::Result res = state.fontTextureHeap.texture2DCreate(state.fontTexture, texCreateInfo);
		if (!zg::isSuccess(res)) return res;
	}
	ASSERT_ZG state.fontTexture.setDebugName("ImGui_FontTexture");

	// Allocate memory for vertex and index buffer
	const uint64_t uploadHeapNumBytes = (IMGUI_VERTEX_BUFFER_SIZE + IMGUI_INDEX_BUFFER_SIZE) * frameLatency;
	{
		zg::Result res = state.uploadHeap.create(uploadHeapNumBytes, ZG_MEMORY_TYPE_UPLOAD);
		if (!zg::isSuccess(res)) return res;
	}

	// Upload font texture to GPU
	{
		// Utilize vertex and index buffer upload heap to upload font texture
		// Create temporary upload buffer
		zg::Buffer tmpUploadBuffer;
		ASSERT_ZG state.uploadHeap.bufferCreate(tmpUploadBuffer, 0, texAllocInfo.sizeInBytes);

		// Copy to the texture
		zg::CommandList commandList;
		ASSERT_ZG copyQueue.beginCommandListRecording(commandList);
		ASSERT_ZG commandList.memcpyToTexture(state.fontTexture, 0, fontTexture, tmpUploadBuffer);
		ASSERT_ZG commandList.enableQueueTransition(state.fontTexture);
		ASSERT_ZG copyQueue.executeCommandList(commandList);
		ASSERT_ZG copyQueue.flush();
	}

	// Actually create the vertex and index buffers
	uint64_t uploadHeapOffset = 0;
	uint32_t frameStateIdx = 0;
	state.frameStates.init(frameLatency, allocator, sfz_dbg(""));
	for (uint32_t i = 0; i < frameLatency; i++) {
		state.frameStates.add({});
		ImGuiFrameState& frame = state.frameStates.last();

		ASSERT_ZG frame.fence.create();

		ASSERT_ZG state.uploadHeap.bufferCreate(
			frame.uploadVertexBuffer, uploadHeapOffset, IMGUI_VERTEX_BUFFER_SIZE);
		uploadHeapOffset += IMGUI_VERTEX_BUFFER_SIZE;
		ASSERT_ZG frame.uploadVertexBuffer.setDebugName(sfz::str32("ImGui_VertexBuffer_%u", frameStateIdx));

		ASSERT_ZG state.uploadHeap.bufferCreate(
			frame.uploadIndexBuffer, uploadHeapOffset, IMGUI_INDEX_BUFFER_SIZE);
		uploadHeapOffset += IMGUI_INDEX_BUFFER_SIZE;
		ASSERT_ZG frame.uploadIndexBuffer.setDebugName(sfz::str32("ImGui_IndexBuffer_%u", frameStateIdx));

		frameStateIdx += 1;
	}
	sfz_assert(uploadHeapOffset == uploadHeapNumBytes);

	// TODO: Remove
	state.tmpVertices.init(IMGUI_MAX_NUM_VERTICES, allocator, sfz_dbg(""));
	state.tmpIndices.init(IMGUI_MAX_NUM_INDICES, allocator, sfz_dbg(""));
	state.tmpCommands.init(100, allocator, sfz_dbg(""));

	return zg::Result::SUCCESS;
}

void imguiRender(
	ImGuiRenderState& state,
	uint64_t frameIdx,
	zg::CommandQueue& presentQueue,
	zg::Framebuffer& framebuffer,
	float scale) noexcept
{
	// Generate ImGui draw lists and get the draw data
	ImGui::Render();
	ImDrawData& drawData = *ImGui::GetDrawData();

	// Clear old temp data
	state.tmpVertices.clear();
	state.tmpIndices.clear();
	state.tmpCommands.clear();

	// Convert draw data
	for (int i = 0; i < drawData.CmdListsCount; i++) {

		const ImDrawList& cmdList = *drawData.CmdLists[i];

		// indexOffset is the offset to offset all indices with
		const uint32_t indexOffset = state.tmpVertices.size();

		// indexBufferOffset is the offset to where the indices start
		uint32_t indexBufferOffset = state.tmpIndices.size();

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

			state.tmpVertices.add(convertedVertex);
		}

		// Fix indices and add to global list
		for (int j = 0; j < cmdList.IdxBuffer.size(); j++) {
			state.tmpIndices.add(cmdList.IdxBuffer[j] + indexOffset);
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

			state.tmpCommands.add(cmd);
		}
	}

	sfz_assert_hard(state.tmpVertices.size() < IMGUI_MAX_NUM_VERTICES);
	sfz_assert_hard(state.tmpIndices.size() < IMGUI_MAX_NUM_INDICES);

	// Get current frame's resources and then wait until they are available (i.e. the frame they
	// are part of has finished rendering).
	ImGuiFrameState& imguiFrame = state.getFrameState(frameIdx);
	ASSERT_ZG imguiFrame.fence.waitOnCpuBlocking();

	// Memcpy vertices and indices to imgui upload buffers
	ASSERT_ZG imguiFrame.uploadVertexBuffer.memcpyTo(
		0, state.tmpVertices.data(), state.tmpVertices.size() * sizeof(ImGuiVertex));
	ASSERT_ZG imguiFrame.uploadIndexBuffer.memcpyTo(
		0, state.tmpIndices.data(), state.tmpIndices.size() * sizeof(uint32_t));

	zg::CommandList commandList;
	ASSERT_ZG presentQueue.beginCommandListRecording(commandList);

	// Set framebuffer
	ASSERT_ZG commandList.setFramebuffer(framebuffer);

	// Set ImGui pipeline
	ASSERT_ZG commandList.setPipeline(state.pipeline);
	ASSERT_ZG commandList.setIndexBuffer(imguiFrame.uploadIndexBuffer, ZG_INDEX_BUFFER_TYPE_UINT32);
	ASSERT_ZG commandList.setVertexBuffer(0, imguiFrame.uploadVertexBuffer);

	// Bind pipeline parameters
	ASSERT_ZG commandList.setPipelineBindings(zg::PipelineBindings()
		.addTexture(0, state.fontTexture));

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
	for (uint32_t i = 0; i < state.tmpCommands.size(); i++) {
		const ImGuiCommand& cmd = state.tmpCommands[i];
		sfz_assert((cmd.numIndices % 3) == 0);

		ZgFramebufferRect scissorRect = {};
		scissorRect.topLeftX = uint32_t(cmd.clipRect.x * imguiInvScaleFactor);
		scissorRect.topLeftY = uint32_t(cmd.clipRect.y * imguiInvScaleFactor);
		scissorRect.width = uint32_t((cmd.clipRect.z - cmd.clipRect.x) * imguiInvScaleFactor);
		scissorRect.height = uint32_t((cmd.clipRect.w - cmd.clipRect.y) * imguiInvScaleFactor);
		ASSERT_ZG commandList.setFramebufferScissor(scissorRect);

		ASSERT_ZG commandList.drawTrianglesIndexed(cmd.idxBufferOffset, cmd.numIndices / 3);
	}

	// Execute command list
	ASSERT_ZG presentQueue.executeCommandList(commandList);

	// Signal that we have finished rendering this ImGui frame
	ASSERT_ZG presentQueue.signalOnGpu(imguiFrame.fence);
}

} // namespace zg
