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

#include "ph/renderer/ImGuiRenderer.hpp"

#include <utility> // std::swap()

#include <sfz/math/Matrix.hpp>

#include "ph/Context.hpp"
#include "ph/renderer/ZeroGUtils.hpp"

namespace ph {

using sfz::mat44;

// Constants
// ------------------------------------------------------------------------------------------------

constexpr uint32_t IMGUI_MAX_NUM_VERTICES = 32768;
constexpr uint32_t IMGUI_MAX_NUM_INDICES = 32768;
constexpr uint64_t IMGUI_VERTEX_BUFFER_SIZE = IMGUI_MAX_NUM_VERTICES * sizeof(ImGuiVertex);
constexpr uint64_t IMGUI_INDEX_BUFFER_SIZE = IMGUI_MAX_NUM_INDICES * sizeof(uint32_t);

// ImGuiRenderer: State methods
// ------------------------------------------------------------------------------------------------

bool ImGuiRenderer::init(
	sfz::Allocator* allocator,
	zg::CommandQueue& copyQueue,
	const phConstImageView& fontTextureView) noexcept
{
	// Init some general stuff
	mAllocator = allocator;

	// Init imgui settings
	GlobalConfig& cfg = getGlobalConfig();
	mScaleSetting = cfg.sanitizeFloat("Imgui", "scale", true, FloatBounds(2.0f, 1.0f, 3.0f));

	// Build ImGui pipeline
	bool pipelineSuccess = CHECK_ZG zg::PipelineRenderingBuilder()
		.addVertexAttribute(0, 0, ZG_VERTEX_ATTRIBUTE_F32_2, offsetof(ImGuiVertex, pos))
		.addVertexAttribute(1, 0, ZG_VERTEX_ATTRIBUTE_F32_2, offsetof(ImGuiVertex, texcoord))
		.addVertexAttribute(2, 0, ZG_VERTEX_ATTRIBUTE_F32_4, offsetof(ImGuiVertex, color))
		.addVertexBufferInfo(0, sizeof(ImGuiVertex))
		.addPushConstant(0)
		.addSampler(0, ZG_SAMPLING_MODE_TRILINEAR)
		.setCullingEnabled(false)
		.setBlendingEnabled(true)
		.setBlendFuncColor(ZG_BLEND_FUNC_ADD, ZG_BLEND_FACTOR_SRC_ALPHA, ZG_BLEND_FACTOR_SRC_INV_ALPHA)
		.setDepthTestEnabled(false)
		//.addVertexShaderPath("VSMain", "res_ph/shaders/imgui.hlsl")
		//.addPixelShaderPath("PSMain", "res_ph/shaders/imgui.hlsl")
		//.buildFromFileHLSL(mPipeline, ZG_SHADER_MODEL_6_0);
		.addVertexShaderPath("VSMain", "res_ph/shaders/imgui_vs.spv")
		.addPixelShaderPath("PSMain", "res_ph/shaders/imgui_ps.spv")
		.buildFromFileSPIRV(mPipeline);
	if (!pipelineSuccess) return false;

	// Allocate memory for font texture
	sfz_assert_release(fontTextureView.type == ImageType::R_U8);
	ZgTexture2DCreateInfo texCreateInfo = {};
	texCreateInfo.format = ZG_TEXTURE_2D_FORMAT_R_U8;
	texCreateInfo.normalized = ZG_TRUE;
	texCreateInfo.width = fontTextureView.width;
	texCreateInfo.height = fontTextureView.height;
	texCreateInfo.numMipmaps = 1; // TODO: Mipmaps

	ZgTexture2DAllocationInfo texAllocInfo = {};
	CHECK_ZG zg::Texture2D::getAllocationInfo(texAllocInfo, texCreateInfo);
	texCreateInfo.offsetInBytes = 0;
	texCreateInfo.sizeInBytes = texAllocInfo.sizeInBytes;

	bool texMemSuccess = CHECK_ZG mFontTextureHeap.create(texAllocInfo.sizeInBytes);

	texMemSuccess &= CHECK_ZG mFontTextureHeap.texture2DCreate(
		mFontTexture, texCreateInfo);
	mFontTexture.setDebugName("ImGui_FontTexture");

	if (!texMemSuccess) return false;

	// Allocate memory for vertex and index buffer
	const uint64_t uploadHeapNumBytes = (IMGUI_VERTEX_BUFFER_SIZE + IMGUI_INDEX_BUFFER_SIZE) * MAX_NUM_FRAMES;
	bool memSuccess = CHECK_ZG mUploadHeap.create(uploadHeapNumBytes, ZG_MEMORY_TYPE_UPLOAD);

	// Upload font texture to GPU
	{
		// Utilize vertex and index buffer upload heap to upload font texture
		// Create temporary upload buffer
		zg::Buffer tmpUploadBuffer;
		CHECK_ZG mUploadHeap.bufferCreate(tmpUploadBuffer, 0, texAllocInfo.sizeInBytes);

		// Convert to ZgImageViewConstCpu
		ZgImageViewConstCpu imageView = {};
		imageView.format = ZG_TEXTURE_2D_FORMAT_R_U8;
		imageView.data = fontTextureView.rawData;
		imageView.width = fontTextureView.width;
		imageView.height = fontTextureView.height;
		imageView.pitchInBytes = fontTextureView.width * sizeof(uint8_t);

		// Copy to the texture
		zg::CommandList commandList;
		CHECK_ZG copyQueue.beginCommandListRecording(commandList);
		CHECK_ZG commandList.memcpyToTexture(mFontTexture, 0, imageView, tmpUploadBuffer);
		CHECK_ZG commandList.enableQueueTransition(mFontTexture);
		CHECK_ZG copyQueue.executeCommandList(commandList);
		CHECK_ZG copyQueue.flush();
	}

	// Actually create the vertex and index buffers
	uint64_t uploadHeapOffset = 0;
	uint32_t frameStateIdx = 0;
	mFrameStates.initAllStates([&](ImGuiFrameState& frame) {
		memSuccess &= CHECK_ZG mUploadHeap.bufferCreate(
			frame.uploadVertexBuffer, uploadHeapOffset, IMGUI_VERTEX_BUFFER_SIZE);
		uploadHeapOffset += IMGUI_VERTEX_BUFFER_SIZE;
		CHECK_ZG frame.uploadVertexBuffer.setDebugName(str32("ImGui_VertexBuffer_%u", frameStateIdx));

		memSuccess &= CHECK_ZG mUploadHeap.bufferCreate(
			frame.uploadIndexBuffer, uploadHeapOffset, IMGUI_INDEX_BUFFER_SIZE);
		uploadHeapOffset += IMGUI_INDEX_BUFFER_SIZE;
		CHECK_ZG frame.uploadIndexBuffer.setDebugName(str32("ImGui_IndexBuffer_%u", frameStateIdx));

		frameStateIdx += 1;
	});
	sfz_assert_debug(uploadHeapOffset == uploadHeapNumBytes);

	if (!memSuccess) return false;

	// Initialize fences for per frame state
	CHECK_ZG mFrameStates.initAllFences();

	return true;
}

void ImGuiRenderer::swap(ImGuiRenderer& other) noexcept
{
	std::swap(this->mAllocator, other.mAllocator);

	this->mPipeline.swap(other.mPipeline);

	this->mFontTextureHeap.swap(other.mFontTextureHeap);
	this->mFontTexture.swap(other.mFontTexture);

	this->mUploadHeap.swap(other.mUploadHeap);

	std::swap(this->mFrameStates, other.mFrameStates);

	std::swap(this->mScaleSetting, other.mScaleSetting);
}

void ImGuiRenderer::destroy() noexcept
{
	mAllocator = nullptr;

	mPipeline.release();

	mFontTextureHeap.release();
	mFontTexture.release();

	mUploadHeap.release();

	mFrameStates.deinitAllStates([](ImGuiFrameState& state) {
		state = ImGuiFrameState();
	});
	mFrameStates.releaseAllFences();

	mScaleSetting = nullptr;
}

// ImGuiRenderer: Methods
// ------------------------------------------------------------------------------------------------

void ImGuiRenderer::render(
	uint64_t frameIdx,
	zg::CommandQueue& presentQueue,
	ZgFramebuffer* framebuffer,
	vec2_s32 framebufferRes,
	const phImguiVertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices,
	const phImguiCommand* commands,
	uint32_t numCommands) noexcept
{
	sfz_assert_release(numVertices < IMGUI_MAX_NUM_VERTICES);
	sfz_assert_release(numIndices < IMGUI_MAX_NUM_INDICES);

	// Get current frame's resources and then wait until they are available (i.e. the frame they
	// are part of has finished rendering).
	PerFrame<ImGuiFrameState>& imguiFrame = mFrameStates.getState(frameIdx);
	CHECK_ZG imguiFrame.renderingFinished.waitOnCpuBlocking();

	// Convert ImGui vertices because slightly different representation
	if (imguiFrame.state.convertedVertices.data() == nullptr) {
		imguiFrame.state.convertedVertices.create(IMGUI_MAX_NUM_VERTICES, mAllocator);
	}
	imguiFrame.state.convertedVertices.setSize(numVertices);
	for (uint32_t i = 0; i < numVertices; i++) {
		imguiFrame.state.convertedVertices[i].pos = vertices[i].pos;
		imguiFrame.state.convertedVertices[i].texcoord = vertices[i].texcoord;
		imguiFrame.state.convertedVertices[i].color = [&]() {
			vec4 color;
			color.x = float(vertices[i].color & 0xFFu) * (1.0f / 255.0f);
			color.y = float((vertices[i].color >> 8u) & 0xFFu) * (1.0f / 255.0f);
			color.z = float((vertices[i].color >> 16u) & 0xFFu) * (1.0f / 255.0f);
			color.w = float((vertices[i].color >> 24u) & 0xFFu) * (1.0f / 255.0f);
			return color;
		}();
	}

	// Memcpy vertices and indices to imgui upload buffers
	CHECK_ZG imguiFrame.state.uploadVertexBuffer.memcpyTo(
		0, imguiFrame.state.convertedVertices.data(), numVertices * sizeof(ImGuiVertex));
	CHECK_ZG imguiFrame.state.uploadIndexBuffer.memcpyTo(
		0, indices, numIndices * sizeof(uint32_t));

	// Here we should normally signal that imguiFrame.uploadFinished() and then wait on it before
	// executing the imgui rendering commands. But because we only upload data to the UPLOAD heap
	// (which is synchronous) we don't actually need to do this. Therefore we skip it in this case.

	zg::CommandList commandList;
	CHECK_ZG presentQueue.beginCommandListRecording(commandList);

	// Set framebuffer
	CHECK_ZG commandList.setFramebuffer(framebuffer);

	// Set ImGui pipeline
	CHECK_ZG commandList.setPipeline(mPipeline);
	CHECK_ZG commandList.setIndexBuffer(imguiFrame.state.uploadIndexBuffer, ZG_INDEX_BUFFER_TYPE_UINT32);
	CHECK_ZG commandList.setVertexBuffer(0, imguiFrame.state.uploadVertexBuffer);

	// Bind pipeline parameters
	CHECK_ZG commandList.setPipelineBindings(zg::PipelineBindings()
		.addTexture(0, mFontTexture));

	// Retrieve imgui scale factor
	float imguiScaleFactor = 1.0f;
	if (mScaleSetting != nullptr) imguiScaleFactor /= mScaleSetting->floatValue();
	float imguiInvScaleFactor = 1.0f / imguiScaleFactor;
	float imguiWidth = framebufferRes.x * imguiScaleFactor;
	float imguiHeight = framebufferRes.y * imguiScaleFactor;

	// Calculate and set ImGui projection matrix
	mat44 projMatrix;
	projMatrix.row0 = vec4(2.0f / imguiWidth, 0.0f, 0.0f, -1.0f);
	projMatrix.row1 = vec4(0.0f, 2.0f / -imguiHeight, 0.0f, 1.0f);
	projMatrix.row2 = vec4(0.0f, 0.0f, 0.5f, 0.5f);
	projMatrix.row3 = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	CHECK_ZG commandList.setPushConstant(0, &projMatrix, sizeof(mat44));

	// Render ImGui commands
	for (uint32_t i = 0; i < numCommands; i++) {
		const phImguiCommand& cmd = commands[i];
		sfz_assert_debug((cmd.numIndices % 3) == 0);

		ZgFramebufferRect scissorRect = {};
		scissorRect.topLeftX = cmd.clipRect.x * imguiInvScaleFactor;
		scissorRect.topLeftY = cmd.clipRect.y * imguiInvScaleFactor;
		scissorRect.width = (cmd.clipRect.z - cmd.clipRect.x) * imguiInvScaleFactor;
		scissorRect.height = (cmd.clipRect.w - cmd.clipRect.y) * imguiInvScaleFactor;
		CHECK_ZG commandList.setFramebufferScissor(scissorRect);

		CHECK_ZG commandList.drawTrianglesIndexed(cmd.idxBufferOffset, cmd.numIndices / 3);
	}

	// Execute command list
	CHECK_ZG presentQueue.executeCommandList(commandList);

	// Signal that we have finished rendering this ImGui frame
	CHECK_ZG presentQueue.signalOnGpu(imguiFrame.renderingFinished);
}

} // namespace ph
