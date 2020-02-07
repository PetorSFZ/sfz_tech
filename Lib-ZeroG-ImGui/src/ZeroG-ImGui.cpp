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


struct phImguiVertex {
	sfz::vec2 pos;
	sfz::vec2 texcoord;
	uint32_t color;
};
static_assert(sizeof(phImguiVertex) == sizeof(float) * 5, "phImguiVertex is padded");

struct phImguiCommand {
	uint32_t idxBufferOffset = 0;
	uint32_t numIndices = 0;
	uint32_t padding[2];
	sfz::vec4 clipRect = sfz::vec4(0.0f);
};
static_assert(sizeof(phImguiCommand) == sizeof(uint32_t) * 8, "phImguiCommand is padded");


namespace zg {

// Constants
// ------------------------------------------------------------------------------------------------

constexpr uint32_t IMGUI_MAX_NUM_VERTICES = 32768;
constexpr uint32_t IMGUI_MAX_NUM_INDICES = 32768;
constexpr uint64_t IMGUI_VERTEX_BUFFER_SIZE = IMGUI_MAX_NUM_VERTICES * sizeof(ImGuiVertex);
constexpr uint64_t IMGUI_INDEX_BUFFER_SIZE = IMGUI_MAX_NUM_INDICES * sizeof(uint32_t);

// ImGui Renderer
// ------------------------------------------------------------------------------------------------

zg::Result imguiInitRenderState(
	ImGuiRenderState& state,
	uint32_t frameLatency,
	sfz::Allocator* allocator,
	zg::CommandQueue& queue,
	const ZgImageViewConstCpu& fontTexture,
	float scale) noexcept
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
			//.addVertexShaderPath("VSMain", "res_ph/shaders/imgui.hlsl")
			//.addPixelShaderPath("PSMain", "res_ph/shaders/imgui.hlsl")
			//.buildFromFileHLSL(mPipeline, ZG_SHADER_MODEL_6_0);
			.addVertexShaderPath("VSMain", "res_ph/shaders/imgui_vs.spv")
			.addPixelShaderPath("PSMain", "res_ph/shaders/imgui_ps.spv")
			.buildFromFileSPIRV(state.pipeline);
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
	{
		zg::Result res = zg::Texture2D::getAllocationInfo(texAllocInfo, texCreateInfo);
		if (!zg::isSuccess(res)) return res;
	}
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
	state.fontTexture.setDebugName("ImGui_FontTexture");

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
		{
			zg::Result res = state.uploadHeap.bufferCreate(tmpUploadBuffer, 0, texAllocInfo.sizeInBytes);
			if (!zg::isSuccess(res)) return res;
		}

		// Copy to the texture
		zg::CommandList commandList;
		queue.beginCommandListRecording(commandList);
		commandList.memcpyToTexture(state.fontTexture, 0, fontTexture, tmpUploadBuffer);
		commandList.enableQueueTransition(state.fontTexture);
		queue.executeCommandList(commandList);
		queue.flush();
	}

	// Actually create the vertex and index buffers
	uint64_t uploadHeapOffset = 0;
	uint32_t frameStateIdx = 0;
	state.frameStates.init(frameLatency, allocator, sfz_dbg(""));
	for (uint32_t i = 0; i < frameLatency; i++) {
		state.frameStates.add({});
		ImGuiFrameState& frame = state.frameStates.last();

		frame.fence.create();

		/*memSuccess &= CHECK_ZG*/ state.uploadHeap.bufferCreate(
			frame.uploadVertexBuffer, uploadHeapOffset, IMGUI_VERTEX_BUFFER_SIZE);
		uploadHeapOffset += IMGUI_VERTEX_BUFFER_SIZE;
		frame.uploadVertexBuffer.setDebugName(sfz::str32("ImGui_VertexBuffer_%u", frameStateIdx));

		/*memSuccess &= CHECK_ZG */state.uploadHeap.bufferCreate(
			frame.uploadIndexBuffer, uploadHeapOffset, IMGUI_INDEX_BUFFER_SIZE);
		uploadHeapOffset += IMGUI_INDEX_BUFFER_SIZE;
		frame.uploadIndexBuffer.setDebugName(sfz::str32("ImGui_IndexBuffer_%u", frameStateIdx));

		frameStateIdx += 1;
	}
	sfz_assert(uploadHeapOffset == uploadHeapNumBytes);

	//if (!memSuccess) return false;

	return zg::Result::SUCCESS;
}

zg::Result imguiRender(
	ImGuiRenderState& state,
	uint64_t frameIdx,
	zg::CommandQueue& presentQueue,
	zg::Framebuffer& framebuffer,
	float scale,
	const phImguiVertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices,
	const phImguiCommand* commands,
	uint32_t numCommands) noexcept
{
	sfz_assert_hard(numVertices < IMGUI_MAX_NUM_VERTICES);
	sfz_assert_hard(numIndices < IMGUI_MAX_NUM_INDICES);

	// Get current frame's resources and then wait until they are available (i.e. the frame they
	// are part of has finished rendering).
	ImGuiFrameState& imguiFrame = state.getFrameState(frameIdx);
	/*CHECK_ZG */imguiFrame.fence.waitOnCpuBlocking();

	// Convert ImGui vertices because slightly different representation
	if (imguiFrame.convertedVertices.data() == nullptr) {
		imguiFrame.convertedVertices.init(IMGUI_MAX_NUM_VERTICES, state.allocator, sfz_dbg(""));
	}
	imguiFrame.convertedVertices.hackSetSize(numVertices);
	for (uint32_t i = 0; i < numVertices; i++) {
		imguiFrame.convertedVertices[i].pos = vertices[i].pos;
		imguiFrame.convertedVertices[i].texcoord = vertices[i].texcoord;
		imguiFrame.convertedVertices[i].color = [&]() {
			vec4 color;
			color.x = float(vertices[i].color & 0xFFu) * (1.0f / 255.0f);
			color.y = float((vertices[i].color >> 8u) & 0xFFu) * (1.0f / 255.0f);
			color.z = float((vertices[i].color >> 16u) & 0xFFu) * (1.0f / 255.0f);
			color.w = float((vertices[i].color >> 24u) & 0xFFu) * (1.0f / 255.0f);
			return color;
		}();
	}

	// Memcpy vertices and indices to imgui upload buffers
	/*CHECK_ZG */imguiFrame.uploadVertexBuffer.memcpyTo(
		0, imguiFrame.convertedVertices.data(), numVertices * sizeof(ImGuiVertex));
	/*CHECK_ZG */imguiFrame.uploadIndexBuffer.memcpyTo(
		0, indices, numIndices * sizeof(uint32_t));

	// Here we should normally signal that imguiFrame.uploadFinished() and then wait on it before
	// executing the imgui rendering commands. But because we only upload data to the UPLOAD heap
	// (which is synchronous) we don't actually need to do this. Therefore we skip it in this case.

	zg::CommandList commandList;
	/*CHECK_ZG */presentQueue.beginCommandListRecording(commandList);

	// Set framebuffer
	/*CHECK_ZG */commandList.setFramebuffer(framebuffer);

	// Set ImGui pipeline
	/*CHECK_ZG */commandList.setPipeline(state.pipeline);
	/*CHECK_ZG */commandList.setIndexBuffer(imguiFrame.uploadIndexBuffer, ZG_INDEX_BUFFER_TYPE_UINT32);
	/*CHECK_ZG */commandList.setVertexBuffer(0, imguiFrame.uploadVertexBuffer);

	// Bind pipeline parameters
	/*CHECK_ZG */commandList.setPipelineBindings(zg::PipelineBindings()
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
	/*CHECK_ZG */commandList.setPushConstant(0, &projMatrix, sizeof(float) * 16);

	// Render ImGui commands
	for (uint32_t i = 0; i < numCommands; i++) {
		const phImguiCommand& cmd = commands[i];
		sfz_assert((cmd.numIndices % 3) == 0);

		ZgFramebufferRect scissorRect = {};
		scissorRect.topLeftX = uint32_t(cmd.clipRect.x * imguiInvScaleFactor);
		scissorRect.topLeftY = uint32_t(cmd.clipRect.y * imguiInvScaleFactor);
		scissorRect.width = uint32_t((cmd.clipRect.z - cmd.clipRect.x) * imguiInvScaleFactor);
		scissorRect.height = uint32_t((cmd.clipRect.w - cmd.clipRect.y) * imguiInvScaleFactor);
		/*CHECK_ZG */commandList.setFramebufferScissor(scissorRect);

		/*CHECK_ZG */commandList.drawTrianglesIndexed(cmd.idxBufferOffset, cmd.numIndices / 3);
	}

	// Execute command list
	/*CHECK_ZG */presentQueue.executeCommandList(commandList);

	// Signal that we have finished rendering this ImGui frame
	/*CHECK_ZG */presentQueue.signalOnGpu(imguiFrame.fence);

	return zg::Result::SUCCESS;
}

} // namespace zg
