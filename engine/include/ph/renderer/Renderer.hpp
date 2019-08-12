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

#include <sfz/math/Vector.hpp>
#include <sfz/memory/Allocator.hpp>
#include <sfz/strings/StringID.hpp>

#include "ph/rendering/Mesh.hpp"
#include "ph/rendering/ImageView.hpp"
#include "ph/rendering/ImguiRenderingData.hpp"

// Forward declarations
struct SDL_Window;
struct phContext;

namespace ph {

using sfz::StringID;
using sfz::vec2_s32;

// Helper structs
// ------------------------------------------------------------------------------------------------

struct MeshRegisters final {
	uint32_t materialIdxPushConstant = ~0u;
	uint32_t materialsArray = ~0u;
	uint32_t albedo = ~0u;
	uint32_t metallicRoughness = ~0u;
	uint32_t normal = ~0u;
	uint32_t occlusion = ~0u;
	uint32_t emissive = ~0u;
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
		const phConstImageView& fontTexture,
		sfz::Allocator* allocator) noexcept;
	bool loadConfiguration(const char* jsonConfigPath) noexcept;
	void swap(Renderer& other) noexcept;
	void destroy() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	vec2_s32 windowResolution() const noexcept;

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
	//
	// TODO: NEED OPTION TO SPECIFY IF MIPMAPS SHOULD BE GENERATED OR NOT
	bool uploadTextureBlocking(StringID id, const phConstImageView& image) noexcept;

	// Check if a texture is loaded or not
	bool textureLoaded(StringID id) const noexcept;

	// Uploads a mesh to the renderer, blocks until done.
	//
	// The "id" is a unique string identifier for this mesh. This should normally be, assuming the
	// mesh is read from file, the "global path" (i.e. the relative path from the game
	// executable) to the mesh. E.g. "res/path/to/model.gltf" if the mesh is in the "res"
	// directory in the same directory as the executable.
	//
	// Returns whether succesful or not.
	//
	// TODO: NEED TO ALSO SPECIFY HOW LOAD TEXTURES
	bool uploadMeshBlocking(StringID id, const Mesh& mesh) noexcept;

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
	void stageBeginInput(StringID stageName) noexcept;

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

	// Sets a constant buffer for the currently input active stage.
	//
	// You are only allowed to set a given constant buffer for a stage once per frame. This
	// limitation currently exists because multiple buffer are allocated for each constant buffer
	// internally in order to allow CPU->GPU uploading while rendering previous frames.
	void stageSetConstantBufferUntyped(
		uint32_t shaderRegister, const void* data, uint32_t numBytes) noexcept;

	template<typename T>
	void stageSetConstantBuffer(uint32_t shaderRegister, const T& data) noexcept
	{
		stageSetConstantBufferUntyped(
			shaderRegister, &data, sizeof(T));
	}

	// Draws a mesh in the currently input active stage
	//
	// The specified registers will get data if available
	void stageDrawMesh(StringID meshId, const MeshRegisters& registers) noexcept;

	// Ends user-input for the specified stage.
	void stageEndInput() noexcept;

	void renderImguiHack(
		const phImguiVertex* vertices,
		uint32_t numVertices,
		const uint32_t* indices,
		uint32_t numIndices,
		const phImguiCommand* commands,
		uint32_t numCommands) noexcept; // TODO: This should be baked into finishFrame()
	
	// Finished the frame, no additional stage methods may be called after this.
	void frameFinish() noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------
private:

	RendererState* mState = nullptr;
};

} // namespace ph
