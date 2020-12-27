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
#include <skipifzero_image_view.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

#include "sfz/Context.hpp"
#include "sfz/rendering/Mesh.hpp"

// Forward declarations
struct SDL_Window;
struct phContext;

namespace sfz {

// Helper structs
// ------------------------------------------------------------------------------------------------

constexpr uint32_t RENDERER_MAX_NUM_CONST_BUFFERS = 16;
constexpr uint32_t RENDERER_MAX_NUM_UNORDERED_BUFFERS = 16;
constexpr uint32_t RENDERER_MAX_NUM_TEXTURES = 16;
constexpr uint32_t RENDERER_MAX_NUM_UNORDERED_TEXTURES = 16;

enum class BindingResourceType {
	ID = 0,
	RAW_BUFFER = 1,
	RAW_TEXTURE = 2
};

struct Binding final {
	BindingResourceType type = BindingResourceType::ID;
	union {
		strID resourceID;
		ZgBuffer* rawBuffer;
		ZgTexture* rawTexture;
	} resource;
	uint32_t shaderRegister = ~0u;
	uint32_t mipLevel = 0; // Only used for unordered textures
};

struct PipelineBindings final {
	ArrayLocal<Binding, RENDERER_MAX_NUM_CONST_BUFFERS> constBuffers;
	ArrayLocal<Binding, RENDERER_MAX_NUM_UNORDERED_BUFFERS> unorderedBuffers;
	ArrayLocal<Binding, RENDERER_MAX_NUM_TEXTURES> textures;
	ArrayLocal<Binding, RENDERER_MAX_NUM_UNORDERED_TEXTURES> unorderedTextures;

	PipelineBindings& addConstBuffer(const char* name, uint32_t shaderRegister)
	{
		return addConstBuffer(strID(name), shaderRegister);
	}
	PipelineBindings& addConstBuffer(strID id, uint32_t shaderRegister)
	{
		Binding& binding = constBuffers.add();
		binding.resource.resourceID = id;
		binding.shaderRegister = shaderRegister;
		return *this;
	}
	PipelineBindings& addConstBuffer(zg::Buffer& buffer, uint32_t shaderRegister)
	{
		Binding& binding = constBuffers.add();
		binding.type = BindingResourceType::RAW_BUFFER;
		binding.resource.rawBuffer = buffer.handle;
		binding.shaderRegister = shaderRegister;
		return *this;
	}

	PipelineBindings& addUnorderedBuffer(const char* name, uint32_t shaderRegister)
	{
		return addUnorderedBuffer(strID(name), shaderRegister);
	}
	PipelineBindings& addUnorderedBuffer(strID id, uint32_t shaderRegister)
	{
		Binding& binding = unorderedBuffers.add();
		binding.resource.resourceID = id;
		binding.shaderRegister = shaderRegister;
		return *this;
	}

	PipelineBindings& addTexture(const char* name, uint32_t shaderRegister)
	{
		return addTexture(strID(name), shaderRegister);
	}
	PipelineBindings& addTexture(strID id, uint32_t shaderRegister)
	{
		Binding& binding = textures.add();
		binding.resource.resourceID = id;
		binding.shaderRegister = shaderRegister;
		return *this;
	}

	PipelineBindings& addUnorderedTexture(const char* name, uint32_t shaderRegister, uint32_t mipLevel)
	{
		return addUnorderedTexture(strID(name), shaderRegister, mipLevel);
	}
	PipelineBindings& addUnorderedTexture(strID id, uint32_t shaderRegister, uint32_t mipLevel)
	{
		Binding& binding = unorderedTextures.add();
		binding.resource.resourceID = id;
		binding.shaderRegister = shaderRegister;
		binding.mipLevel = mipLevel;
		return *this;
	}
};

// Renderer
// ------------------------------------------------------------------------------------------------

struct RendererState;

class Renderer final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Renderer() noexcept = default;
	Renderer(const Renderer&) = delete;
	Renderer& operator= (const Renderer&) = delete;
	Renderer(Renderer&& o) noexcept { this->swap(o); }
	Renderer& operator= (Renderer&& o) noexcept { this->swap(o); return *this; }
	~Renderer() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool active() const noexcept { return mState != nullptr; }
	bool init(
		SDL_Window* window,
		const ImageViewConst& fontTexture,
		sfz::Allocator* allocator) noexcept;
	bool loadConfiguration(const char* jsonConfigPath) noexcept;
	void swap(Renderer& other) noexcept;
	void destroy() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	RendererState& directAccessInternalState() noexcept { return *mState; }

	uint64_t currentFrameIdx() const noexcept; // Incremented each frameBegin()
	vec2_i32 windowResolution() const noexcept;

	// Returns the latest frame time retrieved and which frame idx it was related to.
	void frameTimeMs(uint64_t& frameIdxOut, float& frameTimeMsOut) const noexcept;

	// ImGui UI methods
	// --------------------------------------------------------------------------------------------

	void renderImguiUI() noexcept;

	// Resource methods
	// --------------------------------------------------------------------------------------------

	// Uploads a texture to the renderer, blocks until done.
	//
	// The "id" is a unique identifier for this texture. This should normally be, assuming the
	// texture is read from file, the "global path" (i.e. the relative from the game executable)
	// to the texture. E.g. "res/path/to/texture.png" if the texture is in the "res" directory in
	// the same directory as the executable.
	//
	// Returns whether succesful or not
	bool uploadTextureBlocking(
		strID id, const ImageViewConst& image, bool generateMipmaps) noexcept;

	// Check if a texture is loaded or not
	bool textureLoaded(strID id) const noexcept;

	// Removes a texture from the renderer, will flush rendering.
	//
	// This operation flushes the rendering so we can guarantee no operation in progress is using
	// the texture to be removed. This of course means that this is a slow operation that will
	// cause frame stutter.
	//
	// WARNING: This must NOT be called between frameBegin() and frameFinish().
	void removeTextureGpuBlocking(strID id) noexcept;

	// Uploads a mesh to the renderer, blocks until done.
	//
	// The "id" is a unique string identifier for this mesh. This should normally be, assuming the
	// mesh is read from file, the "global path" (i.e. the relative path from the game
	// executable) to the mesh. E.g. "res/path/to/model.gltf" if the mesh is in the "res"
	// directory in the same directory as the executable.
	//
	// Returns whether succesful or not.
	bool uploadMeshBlocking(strID id, const Mesh& mesh) noexcept;

	// Check if a mesh is loaded or not
	bool meshLoaded(strID id) const noexcept;

	// Removes a mesh from the renderer, will flush rendering.
	//
	// This operation flushes the rendering so we can guarantee no operation in progress is using
	// the texture to be removed. This of course means that this is a slow operation that will
	// cause frame stutter.
	//
	// WARNING: This must NOT be called between frameBegin() and frameFinish().
	void removeMeshGpuBlocking(strID id) noexcept;

	// Stage methods
	// --------------------------------------------------------------------------------------------

	// Begins the frame, must be called before any other stage methods are called for a given frame.
	void frameBegin() noexcept;

	// Returns whether in stage input mode (stageBeginInput(), stageEndInput()) or not. Mainly
	// used to internally validate state, but might be useful for users of renderer in some
	// contexts.
	bool inStageInputMode() const noexcept;

	// Enables the specified stage for input through the renderer's interface.
	//
	// Note that this does not mean that stages are executing sequentially (they might be executing
	// simulatenously if there are no stage barriers between them), it just means that the renderer
	// only accepts input for the specified stage until endStageInput() is called.
	void stageBeginInput(const char* stageName) noexcept;

	vec2_u32 stageGetFramebufferDims() const noexcept;

	// Uploads data to a streaming buffer
	void stageUploadToStreamingBufferUntyped(
		const char* bufferName, const void* data, uint32_t elementSize, uint32_t numElements) noexcept;

	template<typename T>
	void stageUploadToStreamingBuffer(
		const char* bufferName, const T* data, uint32_t numElements) noexcept
	{
		stageUploadToStreamingBufferUntyped(bufferName, data, sizeof(T), numElements);
	}

	void stageClearRenderTargetsOptimal() noexcept;

	void stageClearDepthBufferOptimal() noexcept;

	// Sets a push constant for the currently input active stage
	void stageSetPushConstantUntyped(
		uint32_t shaderRegister, const void* data, uint32_t numBytes) noexcept;

	template<typename T>
	void stageSetPushConstant(uint32_t shaderRegister, const T& data) noexcept
	{
		static_assert(sizeof(T) <= 128);
		stageSetPushConstantUntyped(
			shaderRegister, &data, sizeof(T));
	}

	void stageSetBindings(const PipelineBindings& bindings) noexcept;

	void stageSetVertexBuffer(const char* streamingBufferName) noexcept;
	void stageSetIndexBuffer(const char* streamingBufferName, bool u32Buffer) noexcept;

	void stageSetIndexBuffer(zg::Buffer& buffer, ZgIndexBufferType indexBufferType) noexcept;
	void stageSetVertexBuffer(uint32_t slot, zg::Buffer& buffer) noexcept;

	void stageDrawTriangles(uint32_t startVertex, uint32_t numVertices) noexcept;
	void stageDrawTrianglesIndexed(uint32_t firstIndex, uint32_t numIndices) noexcept;

	// Inserts an unordered barrier. Basically says that all writes to an unordered resource (or
	// all of them) must be finished before the resource is read from again.
	void stageUnorderedBarrierAll() noexcept;
	void stageUnorderedBarrierStaticBuffer(const char* staticBufferName) noexcept;
	void stageUnorderedBarrierStaticTexture(const char* staticBufferName) noexcept;

	// Gets the group dimensions of the compute pipeline associated with the currently active stage.
	vec3_i32 stageGetComputeGroupDims() noexcept;

	// Runs a compute pipeline with the specified number of groups.
	void stageDispatchComputeNoAutoBindings(
		uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) noexcept;

	// Ends user-input for the specified stage.
	void stageEndInput() noexcept;

	// Progress to the next stage group
	bool frameProgressNextStageGroup() noexcept;

	// Finished the frame, no additional stage methods may be called after this.
	void frameFinish() noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------
private:

	RendererState* mState = nullptr;
};

} // namespace sfz
