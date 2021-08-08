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
#include "sfz/renderer/HighLevelCmdList.hpp"
#include "sfz/rendering/Mesh.hpp"

// Forward declarations
struct SDL_Window;
struct phContext;

namespace sfz {

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
		SfzAllocator* allocator) noexcept;
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

	// Render methods
	// --------------------------------------------------------------------------------------------

	// Begins the frame, must be called before any other stage methods are called for a given frame.
	void frameBegin();

	// Command list methods
	HighLevelCmdList beginCommandList(const char* cmdListName);
	void executeCommandList(HighLevelCmdList cmdList);

	// Finished the frame, no additional stage methods may be called after this.
	void frameFinish();

private:
	RendererState* mState = nullptr;
};

} // namespace sfz
