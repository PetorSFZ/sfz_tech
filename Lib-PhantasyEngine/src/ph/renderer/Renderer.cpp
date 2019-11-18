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

#include "ph/renderer/Renderer.hpp"

#include <utility> // std::swap()

#include <SDL.h>

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>

#include <sfz/Logging.hpp>
#include <sfz/math/Matrix.hpp>

#include <ZeroG-cpp.hpp>

#include "ph/Context.hpp"
#include "ph/config/GlobalConfig.hpp"
#include "ph/renderer/GpuTextures.hpp"
#include "ph/renderer/ImGuiRenderer.hpp"
#include "ph/renderer/RendererConfigParser.hpp"
#include "ph/renderer/RendererState.hpp"
#include "ph/renderer/ZeroGUtils.hpp"

namespace ph {

using sfz::ArrayDynamic;
using sfz::mat44;
using sfz::vec2;
using sfz::vec3;
using sfz::vec4;

// Renderer: State methods
// ------------------------------------------------------------------------------------------------

bool Renderer::init(
	SDL_Window* window,
	const phConstImageView& fontTexture,
	sfz::Allocator* allocator) noexcept
{
	this->destroy();
	mState = allocator->newObject<RendererState>(sfz_dbg("RendererState"));
	mState->allocator = allocator;
	mState->window = window;

	// Settings
	GlobalConfig& cfg = getGlobalConfig();
	Setting* debugModeSetting =
		cfg.sanitizeBool("Renderer", "ZeroGDebugModeOnStartup", true, false);
	mState->flushPresentQueueEachFrame =
		cfg.sanitizeBool("Renderer", "flushPresentQueueEachFrame", false, false);
	mState->flushCopyQueueEachFrame =
		cfg.sanitizeBool("Renderer", "flushCopyQueueEachFrame", false, false);

	// Initializer ZeroG
	bool zgInitSuccess =
		initializeZeroG(mState->zgCtx, window, allocator, debugModeSetting->boolValue());
	if (!zgInitSuccess) {
		this->destroy();
		return false;
	}

	// Set window resolution to default value (512x512)
	mState->windowRes = vec2_i32(512, 512);

	// Get command queues
	if (!(CHECK_ZG zg::CommandQueue::getPresentQueue(mState->presentQueue))) {
		this->destroy();
		return false;
	}
	if (!(CHECK_ZG zg::CommandQueue::getCopyQueue(mState->copyQueue))) {
		this->destroy();
		return false;
	}

	// Initialize dynamic gpu allocator
	constexpr uint32_t PAGE_SIZE_UPLOAD = 32 * 1024 * 1024; // 32 MiB
	constexpr uint32_t PAGE_SIZE_DEVICE = 64 * 1024 * 1024; // 64 MiB
	constexpr uint32_t PAGE_SIZE_TEXTURE = 64 * 1024 * 1024; // 64 MiB
	constexpr uint32_t PAGE_SIZE_FRAMEBUFFER = 64 * 1024 * 1024; // 64 MiB
	mState->gpuAllocatorUpload.init(mState->allocator, ZG_MEMORY_TYPE_UPLOAD, PAGE_SIZE_UPLOAD);
	mState->gpuAllocatorDevice.init(mState->allocator, ZG_MEMORY_TYPE_DEVICE, PAGE_SIZE_DEVICE);
	mState->gpuAllocatorTexture.init(mState->allocator, ZG_MEMORY_TYPE_TEXTURE, PAGE_SIZE_TEXTURE);
	mState->gpuAllocatorFramebuffer.init(mState->allocator, ZG_MEMORY_TYPE_FRAMEBUFFER, PAGE_SIZE_FRAMEBUFFER);

	// Initialize hashmaps for resources
	mState->textures.create(512, mState->allocator);
	mState->meshes.create(512, mState->allocator);

	// Initialize ImGui rendering state
	bool imguiInitSuccess = mState->imguiRenderer.init(
		mState->allocator, mState->copyQueue, fontTexture);
	if (!imguiInitSuccess) {
		this->destroy();
		return false;
	}

	return true;
}

bool Renderer::loadConfiguration(const char* jsonConfigPath) noexcept
{
	if (!this->active()) {
		sfz_assert(false);
		return false;
	}

	// Parse config
	bool parseSuccess = parseRendererConfig(*mState, jsonConfigPath);
	if (!parseSuccess) {
		// TODO: Maybe this is a bit too extreme?
		this->destroy();
		return false;
	}

	return true;
}

void Renderer::swap(Renderer& other) noexcept
{
	std::swap(this->mState, other.mState);
}

void Renderer::destroy() noexcept
{
	if (mState != nullptr) {

		// Flush queues
		CHECK_ZG mState->presentQueue.flush();
		CHECK_ZG mState->copyQueue.flush();

		// Destroy all textures and meshes
		this->removeAllTexturesGpuBlocking();
		this->removeAllMeshesGpuBlocking();

		// Destroy framebuffers
		for (FramebufferItem& item : mState->configurable.framebuffers) {
			item.deallocate(mState->gpuAllocatorFramebuffer);
		}

		// Deallocate stage memory
		bool stageDeallocSuccess = deallocateStageMemory(*mState);
		sfz_assert(stageDeallocSuccess);

		// Deallocate rest of state
		sfz::Allocator* allocator = mState->allocator;
		allocator->deleteObject(mState);
	}
	mState = nullptr;
}

// Renderer: Getters
// ------------------------------------------------------------------------------------------------

vec2_i32 Renderer::windowResolution() const noexcept
{
	return mState->windowRes;
}

// Renderer: ImGui UI methods
// ------------------------------------------------------------------------------------------------

void Renderer::renderImguiUI() noexcept
{
	mState->ui.render(*mState);
}

// Renderer: Resource methods
// ------------------------------------------------------------------------------------------------

bool Renderer::uploadTextureBlocking(
	StringID id, const phConstImageView& image, bool generateMipmaps) noexcept
{
	// Error out and return false if texture already exists
	if (mState->textures.get(id) != nullptr) return false;

	uint32_t numMipmaps = 0;
	zg::Texture2D texture = textureAllocateAndUploadBlocking(
		image,
		mState->gpuAllocatorTexture,
		mState->gpuAllocatorUpload,
		mState->allocator,
		mState->copyQueue,
		generateMipmaps,
		numMipmaps);
	sfz_assert(texture.valid());

	// Fill texture item with info and store it
	TextureItem item;
	item.texture = std::move(texture);
	item.format = toZeroGImageFormat(image.type);
	item.width = image.width;
	item.height = image.height;
	item.numMipmaps = numMipmaps;
	mState->textures[id] = std::move(item);

	return true;
}

bool Renderer::textureLoaded(StringID id) const noexcept
{
	const ph::TextureItem* item = mState->textures.get(id);
	return item != nullptr;
}

void Renderer::removeTextureGpuBlocking(StringID id) noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	// Return if texture is not loaded in first place
	ph::TextureItem* item = mState->textures.get(id);
	if (item == nullptr) return;

	// Ensure all GPU operations in progress are finished
	mState->presentQueue.flush();
	mState->copyQueue.flush();

	// Destroy texture
	mState->gpuAllocatorTexture.deallocate(item->texture);
	mState->textures.remove(id);
}

void Renderer::removeAllTexturesGpuBlocking() noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	// Ensure all GPU operations in progress are finished
	mState->presentQueue.flush();
	mState->copyQueue.flush();

	// Destroy all textures
	for (auto pair : mState->textures) {
		mState->gpuAllocatorTexture.deallocate(pair.value.texture);
	}
	mState->textures.clear();
}

bool Renderer::uploadMeshBlocking(StringID id, const Mesh& mesh) noexcept
{
	// Error out and return false if mesh already exists
	sfz_assert(id != StringID::invalid());
	if (mState->meshes.get(id) != nullptr) return false;

	// Allocate memory for mesh
	GpuMesh gpuMesh = gpuMeshAllocate(mesh, mState->gpuAllocatorDevice, mState->allocator);

	// Upload memory to mesh
	gpuMeshUploadBlocking(
		gpuMesh, mesh, mState->gpuAllocatorUpload, mState->allocator, mState->copyQueue);

	// Store mesh
	mState->meshes[id] = std::move(gpuMesh);

	return true;
}

bool Renderer::meshLoaded(StringID id) const noexcept
{
	const ph::GpuMesh* mesh = mState->meshes.get(id);
	return mesh != nullptr;
}

void Renderer::removeMeshGpuBlocking(StringID id) noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	// Return if mesh is not loaded in first place
	ph::GpuMesh* mesh = mState->meshes.get(id);
	if (mesh == nullptr) return;

	// Ensure all GPU operations in progress are finished
	mState->presentQueue.flush();
	mState->copyQueue.flush();

	// Destroy mesh
	gpuMeshDeallocate(*mesh, mState->gpuAllocatorDevice);
	mState->meshes.remove(id);
}

void Renderer::removeAllMeshesGpuBlocking() noexcept
{
	// Ensure not between frameBegin() and frameFinish()
	sfz_assert(!mState->windowFramebuffer.valid());

	// Ensure all GPU operations in progress are finished
	mState->presentQueue.flush();
	mState->copyQueue.flush();

	// Destroy all meshes
	for (auto pair : mState->meshes) {
		gpuMeshDeallocate(pair.value, mState->gpuAllocatorDevice);
	}
	mState->meshes.clear();
}

// Renderer: Methods
// ------------------------------------------------------------------------------------------------

void Renderer::frameBegin() noexcept
{
	// Increment frame index
	mState->currentFrameIdx += 1;

	// Query drawable width and height from SDL
	int32_t newResX = 0;
	int32_t newResY = 0;
	SDL_GL_GetDrawableSize(mState->window, &newResX, &newResY);
	bool resolutionChanged = newResX != mState->windowRes.x || newResY != mState->windowRes.y;
	
	// Check if any framebuffer scale settings has changed, necessating a resolution change
	if (!resolutionChanged) {
		for (FramebufferItem& item : mState->configurable.framebuffers) {
			if (!item.resolutionIsFixed && item.resolutionScaleSetting != nullptr) {
				if (item.resolutionScale != item.resolutionScaleSetting->floatValue()) {
					resolutionChanged = true;
					break;
				}
			}
		}
	}

	// If resolution has changed, resize swapchain and framebuffers
	if (resolutionChanged) {

		SFZ_INFO("Renderer",
			"Resolution changed, new resolution: %i x %i. Updating framebuffers...",
			newResX, newResY);

		// Set new resolution
		mState->windowRes.x = newResX;
		mState->windowRes.y = newResY;

		// Stop present queue so its safe to reallocate framebuffers
		CHECK_ZG mState->presentQueue.flush();
		
		// Resize swapchain
		// Note: This is actually safe to call every frame and without first flushing present
		//       queue, but since we are also resizing other framebuffers created by us we might
		//       as well protect this call just the same.
		CHECK_ZG mState->zgCtx.swapchainResize(
			uint32_t(mState->windowRes.x), uint32_t(mState->windowRes.y));

		// Resize our framebuffers
		for (FramebufferItem& item : mState->configurable.framebuffers) {
			
			// Only resize if not fixed resolution
			if (!item.resolutionIsFixed) {
				item.deallocate(mState->gpuAllocatorFramebuffer);
				bool res = item.buildFramebuffer(mState->windowRes, mState->gpuAllocatorFramebuffer);
				sfz_assert(res);
			}
		}
	}

	// Begin ZeroG frame
	sfz_assert(!mState->windowFramebuffer.valid());
	CHECK_ZG mState->zgCtx.swapchainBeginFrame(mState->windowFramebuffer);

	// Clear all framebuffers
	// TODO: Should probably only clear using a specific clear framebuffer stage
	zg::CommandList commandList;
	CHECK_ZG mState->presentQueue.beginCommandListRecording(commandList);
	CHECK_ZG commandList.setFramebuffer(mState->windowFramebuffer);
	CHECK_ZG commandList.clearFramebufferOptimal();
	CHECK_ZG mState->presentQueue.executeCommandList(commandList);

	for (FramebufferItem& fbItem : mState->configurable.framebuffers) {
		CHECK_ZG mState->presentQueue.beginCommandListRecording(commandList);
		CHECK_ZG commandList.setFramebuffer(fbItem.framebuffer.framebuffer);
		CHECK_ZG commandList.clearFramebufferOptimal();
		CHECK_ZG mState->presentQueue.executeCommandList(commandList);
	}

	// Set current stage set index to first stage
	mState->currentStageSetIdx = 0;
	if (mState->configurable.presentQueueStages.size() > 0) {
		sfz_assert(
			mState->configurable.presentQueueStages.first().stageType != StageType::USER_STAGE_BARRIER);
	}
}

bool Renderer::inStageInputMode() const noexcept
{
	if (mState->currentInputEnabledStageIdx == ~0u) return false;
	if (mState->currentInputEnabledStage == nullptr) return false;
	if (mState->currentPipelineRender == nullptr) return false;
	if (!mState->currentCommandList.valid()) return false;
	return true;
}

void Renderer::stageBeginInput(StringID stageName) noexcept
{
	// Ensure no stage is currently set to accept input
	sfz_assert(!inStageInputMode());
	if (inStageInputMode()) return;

	// Find stage
	uint32_t stageIdx = mState->findActiveStageIdx(stageName);
	sfz_assert(stageIdx != ~0u);
	if (stageIdx == ~0u) return;
	sfz_assert(stageIdx < mState->configurable.presentQueueStages.size());
	Stage& stage = mState->configurable.presentQueueStages[stageIdx];
	sfz_assert(stage.stageType == StageType::USER_INPUT_RENDERING);

	// Find render pipeline
	uint32_t pipelineIdx = mState->findPipelineRenderIdx(stage.renderPipelineName);
	sfz_assert(pipelineIdx != ~0u);
	if (pipelineIdx == ~0u) return;
	sfz_assert(pipelineIdx < mState->configurable.renderPipelines.size());
	PipelineRenderItem& pipelineItem = mState->configurable.renderPipelines[pipelineIdx];
	sfz_assert(pipelineItem.pipeline.valid());
	if (!pipelineItem.pipeline.valid()) return;

	// Set currently active stage
	mState->currentInputEnabledStageIdx = stageIdx;
	mState->currentInputEnabledStage = &stage;
	mState->currentPipelineRender = &pipelineItem;

	// Get stage's framebuffer
	zg::Framebuffer* framebuffer =
		mState->configurable.getFramebuffer(mState->windowFramebuffer, stage.framebufferName);
	sfz_assert(framebuffer != nullptr);

	// In debug mode, validate that the pipeline's render targets matches the framebuffer
#ifndef NDEBUG
	if (framebuffer == &mState->windowFramebuffer) {
		sfz_assert(pipelineItem.numRenderTargets == 1);
		sfz_assert(pipelineItem.renderTargets[0] == ZG_TEXTURE_FORMAT_RGBA_U8_UNORM);
	}
	else {
		FramebufferItem* fbItem = mState->configurable.getFramebufferItem(stage.framebufferName);
		sfz_assert(fbItem != nullptr);
		sfz_assert(pipelineItem.numRenderTargets == fbItem->numRenderTargets);
		for (uint32_t i = 0; i < fbItem->numRenderTargets; i++) {
			sfz_assert(pipelineItem.renderTargets[i] == fbItem->renderTargetItems[i].format);
		}
	}
#endif

	// Begin recording command list and set pipeline and framebuffer
	CHECK_ZG mState->presentQueue.beginCommandListRecording(mState->currentCommandList);
	CHECK_ZG mState->currentCommandList.setFramebuffer(*framebuffer);
	CHECK_ZG mState->currentCommandList.setPipeline(pipelineItem.pipeline);
}

void Renderer::stageSetPushConstantUntyped(
	uint32_t shaderRegister, const void* data, uint32_t numBytes) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(data != nullptr);
	sfz_assert(numBytes > 0);
	sfz_assert(numBytes <= 128);

	// In debug mode, validate that the specified shader registers corresponds to a a suitable
	// push constant in the pipeline
#ifndef NDEBUG
	const ZgPipelineRenderSignature& signature =
		mState->currentPipelineRender->pipeline.signature;
	
	uint32_t bufferIdx = ~0u;
	for (uint32_t i = 0; i < signature.numConstantBuffers; i++) {
		if (shaderRegister == signature.constantBuffers[i].shaderRegister) {
			bufferIdx = i;
			break;
		}
	}
	
	sfz_assert(bufferIdx != ~0u);
	sfz_assert(signature.constantBuffers[bufferIdx].pushConstant == ZG_TRUE);
	sfz_assert(signature.constantBuffers[bufferIdx].sizeInBytes >= numBytes);
#endif

	CHECK_ZG mState->currentCommandList.setPushConstant(shaderRegister, data, numBytes);
}

void Renderer::stageSetConstantBufferUntyped(
	uint32_t shaderRegister, const void* data, uint32_t numBytes) noexcept
{
	sfz_assert(inStageInputMode());
	sfz_assert(data != nullptr);
	sfz_assert(numBytes > 0);

	// In debug mode, validate that the specified shader registers corresponds to a a suitable
	// constant buffer in the pipeline
#ifndef NDEBUG
	const ZgPipelineRenderSignature& signature =
		mState->currentPipelineRender->pipeline.signature;

	uint32_t bufferIdx = ~0u;
	for (uint32_t i = 0; i < signature.numConstantBuffers; i++) {
		if (shaderRegister == signature.constantBuffers[i].shaderRegister) {
			bufferIdx = i;
			break;
		}
	}

	sfz_assert(bufferIdx != ~0u);
	sfz_assert(signature.constantBuffers[bufferIdx].pushConstant == ZG_FALSE);
	sfz_assert(signature.constantBuffers[bufferIdx].sizeInBytes >= numBytes);
#endif

	// Find constant buffer
	PerFrame<ConstantBufferMemory>* frame =
		mState->findConstantBufferInCurrentInputStage(shaderRegister);
	sfz_assert(frame != nullptr);

	// Ensure that we can only set constant buffer once per frame
	sfz_assert(frame->state.lastFrameIdxTouched != mState->currentFrameIdx);
	frame->state.lastFrameIdxTouched = mState->currentFrameIdx;

	// Wait until frame specific memory is available
	CHECK_ZG frame->renderingFinished.waitOnCpuBlocking();

	// Copy data to upload buffer
	CHECK_ZG frame->state.uploadBuffer.memcpyTo(0, data, numBytes);

	// Issue upload to device buffer
	CHECK_ZG mState->currentCommandList.memcpyBufferToBuffer(
		frame->state.deviceBuffer, 0, frame->state.uploadBuffer, 0, numBytes);
	
	// NOTE: Here we don't need to signal frame.uploadFinished because we are uploading and then
	//       using the uploaded data in the same command list. Internal resource barriers set by
	//       ZeroG should cover this case.
}

void Renderer::stageDrawMesh(StringID meshId, const MeshRegisters& registers) noexcept
{
	sfz_assert(meshId != StringID::invalid());
	sfz_assert(inStageInputMode());

	// Find mesh
	GpuMesh* meshPtr = mState->meshes.get(meshId);
	sfz_assert(meshPtr != nullptr);
	if (meshPtr == nullptr) return;

	// Validate some stuff in debug mode
#ifndef NDEBUG
	// Validate pipeline vertex input for standard mesh rendering
	const ZgPipelineRenderSignature& signature =
		mState->currentPipelineRender->pipeline.signature;
	sfz_assert(signature.numVertexAttributes == 3);

	sfz_assert(signature.vertexAttributes[0].location == 0);
	sfz_assert(signature.vertexAttributes[0].vertexBufferSlot == 0);
	sfz_assert(signature.vertexAttributes[0].type == ZG_VERTEX_ATTRIBUTE_F32_3);
	sfz_assert(
		signature.vertexAttributes[0].offsetToFirstElementInBytes == offsetof(Vertex, pos));

	sfz_assert(signature.vertexAttributes[1].location == 1);
	sfz_assert(signature.vertexAttributes[1].vertexBufferSlot == 0);
	sfz_assert(signature.vertexAttributes[1].type == ZG_VERTEX_ATTRIBUTE_F32_3);
	sfz_assert(
		signature.vertexAttributes[1].offsetToFirstElementInBytes == offsetof(Vertex, normal));

	sfz_assert(signature.vertexAttributes[2].location == 2);
	sfz_assert(signature.vertexAttributes[2].vertexBufferSlot == 0);
	sfz_assert(signature.vertexAttributes[2].type == ZG_VERTEX_ATTRIBUTE_F32_2);
	sfz_assert(
		signature.vertexAttributes[2].offsetToFirstElementInBytes == offsetof(Vertex, texcoord));

	// Validate material index push constant
	if (registers.materialIdxPushConstant != ~0u) {
		bool found = false;
		for (uint32_t i = 0; i < signature.numConstantBuffers; i++) {
			const ZgConstantBufferDesc& desc = signature.constantBuffers[i];
			if (desc.shaderRegister == registers.materialIdxPushConstant) {
				found = true;
				sfz_assert(desc.pushConstant == ZG_TRUE);
				break;
			}
		}
		sfz_assert(found);
	}

	// Validate materials array
	if (registers.materialsArray != ~0u) {
		bool found = false;
		for (uint32_t i = 0; i < signature.numConstantBuffers; i++) {
			const ZgConstantBufferDesc& desc = signature.constantBuffers[i];
			if (desc.shaderRegister == registers.materialsArray) {
				found = true;
				sfz_assert(desc.pushConstant == ZG_FALSE);
				sfz_assert(desc.sizeInBytes >= meshPtr->numMaterials * sizeof(ShaderMaterial));
				sfz_assert(desc.sizeInBytes == sizeof(ForwardShaderMaterialsBuffer));
				break;
			}
		}
		sfz_assert(found);
	}

	// Validate texture bindings
	auto assertTextureRegister = [&](uint32_t texRegister) {
		if (texRegister == ~0u) return;
		bool found = false;
		for (uint32_t i = 0; i < signature.numTextures; i++) {
			const ZgTextureDesc& desc = signature.textures[i];
			if (desc.textureRegister == texRegister) {
				found = true;
				break;
			}
		}
		sfz_assert(found);
	};

	assertTextureRegister(registers.albedo);
	assertTextureRegister(registers.metallicRoughness);
	assertTextureRegister(registers.normal);
	assertTextureRegister(registers.occlusion);
	assertTextureRegister(registers.emissive);
#endif

	// Set vertex buffer
	sfz_assert(meshPtr->vertexBuffer.valid());
	CHECK_ZG mState->currentCommandList.setVertexBuffer(0, meshPtr->vertexBuffer);

	// Set index buffer
	sfz_assert(meshPtr->indexBuffer.valid());
	CHECK_ZG mState->currentCommandList.setIndexBuffer(
		meshPtr->indexBuffer, ZG_INDEX_BUFFER_TYPE_UINT32);

	// Set common pipeline bindings that are same for all components
	zg::PipelineBindings commonBindings;

	// Create materials array binding
	if (registers.materialsArray != ~0u) {
		sfz_assert(meshPtr->materialsBuffer.valid());
		commonBindings.addConstantBuffer(registers.materialsArray, meshPtr->materialsBuffer);
	}

	// User-specified constant buffers
	for (Framed<ConstantBufferMemory>& framed : mState->currentInputEnabledStage->constantBuffers) {
		PerFrame<ConstantBufferMemory>& frame = framed.getState(mState->currentFrameIdx);
		sfz_assert(frame.state.lastFrameIdxTouched == mState->currentFrameIdx);
		commonBindings.addConstantBuffer(frame.state.shaderRegister, frame.state.deviceBuffer);
	}

	// Bound render targets
	for (const BoundRenderTarget& target : mState->currentInputEnabledStage->boundRenderTargets) {
		
		FramebufferItem* item = mState->configurable.getFramebufferItem(target.framebuffer);
		sfz_assert(item != nullptr);
		if (target.depthBuffer) {
			sfz_assert(item->hasDepthBuffer);
			commonBindings.addTexture(
				target.textureRegister, item->framebuffer.depthBuffer);
		}
		else {
			sfz_assert(target.renderTargetIdx < item->framebuffer.numRenderTargets);
			commonBindings.addTexture(
				target.textureRegister, item->framebuffer.renderTargets[target.renderTargetIdx]);
		}
	}

	// Draw all mesh components
	for (MeshComponent& comp : meshPtr->components) {

		sfz_assert(comp.materialIdx < meshPtr->cpuMaterials.size());
		const Material& material = meshPtr->cpuMaterials[comp.materialIdx];

		// Set material index push constant
		if (registers.materialIdxPushConstant != ~0u) {
			sfz::vec4_u32 tmp = sfz::vec4_u32(0u);
			tmp.x = comp.materialIdx;
			CHECK_ZG mState->currentCommandList.setPushConstant(
				registers.materialIdxPushConstant, &tmp, sizeof(sfz::vec4_u32 ));
		}

		// Create texture bindings
		zg::PipelineBindings bindings = commonBindings;
		auto bindTexture = [&](uint32_t texRegister, StringID texID) {
			if (texRegister != ~0u && texID != StringID::invalid()) {

				// Find texture
				TextureItem* texItem = mState->textures.get(texID);
				sfz_assert(texItem != nullptr);

				// Bind texture
				bindings.addTexture(texRegister, texItem->texture);
			}
		};
		bindTexture(registers.albedo, material.albedoTex);
		bindTexture(registers.metallicRoughness, material.metallicRoughnessTex);
		bindTexture(registers.normal, material.normalTex);
		bindTexture(registers.occlusion, material.occlusionTex);
		bindTexture(registers.emissive, material.emissiveTex);

		// Set pipeline bindings
		CHECK_ZG mState->currentCommandList.setPipelineBindings(bindings);

		// Issue draw command
		sfz_assert(comp.numIndices != 0);
		sfz_assert((comp.numIndices % 3) == 0);
		CHECK_ZG mState->currentCommandList.drawTrianglesIndexed(
			comp.firstIndex, comp.numIndices / 3);
	}
}

void Renderer::stageEndInput() noexcept
{
	// Ensure a stage was set to accept input
	sfz_assert(inStageInputMode());
	if (!inStageInputMode()) return;

	// Execute command list
	CHECK_ZG mState->presentQueue.executeCommandList(mState->currentCommandList);

	// Signal all frame specific data
	for (Framed<ConstantBufferMemory>& framed : mState->currentInputEnabledStage->constantBuffers) {
		PerFrame<ConstantBufferMemory>& frame = framed.getState(mState->currentFrameIdx);
		CHECK_ZG mState->presentQueue.signalOnGpu(frame.renderingFinished);
	}
	
	// Clear currently active stage info
	mState->currentInputEnabledStageIdx = ~0u;
	mState->currentInputEnabledStage = nullptr;
	mState->currentPipelineRender = nullptr;
	mState->currentCommandList.release();
}

bool Renderer::stageBarrierProgressNext() noexcept
{
	sfz_assert(!inStageInputMode());

	// Find the next barrier stage
	uint32_t barrierStageIdx = mState->findNextBarrierIdx();
	if (barrierStageIdx == ~0u) return false;

	// Set current stage set index to the stage after the barrier
	mState->currentStageSetIdx = barrierStageIdx + 1;
	sfz_assert(mState->currentStageSetIdx < mState->configurable.presentQueueStages.size());

	return true;
}

void Renderer::renderImguiHack(
	const phImguiVertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices,
	const phImguiCommand* commands,
	uint32_t numCommands) noexcept
{
	mState->imguiRenderer.render(
		mState->currentFrameIdx,
		mState->presentQueue,
		mState->windowFramebuffer,
		mState->windowRes,
		vertices,
		numVertices,
		indices,
		numIndices,
		commands,
		numCommands);
}

void Renderer::frameFinish() noexcept
{
	// Finish ZeroG frame
	sfz_assert(mState->windowFramebuffer.valid());
	CHECK_ZG mState->zgCtx.swapchainFinishFrame();
	mState->windowFramebuffer.release();

	// Flush queues if requested
	if (mState->flushPresentQueueEachFrame->boolValue()) {
		CHECK_ZG mState->presentQueue.flush();
	}
	if (mState->flushCopyQueueEachFrame->boolValue()) {
		CHECK_ZG mState->copyQueue.flush();
	}
}

} // namespace ph
