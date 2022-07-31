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

#include <sfz.h>
#include <sfz_image_view.h>

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

#include "sfz/renderer/HighLevelCmdList.hpp"

// Forward declarations
struct SDL_Window;

struct SfzConfig;
struct SfzProfilingStats;
struct SfzResourceManager;
struct SfzShaderManager;

// Renderer
// ------------------------------------------------------------------------------------------------

struct SfzRendererState;

struct SfzRenderer final {
public:
	SFZ_DECLARE_DROP_TYPE(SfzRenderer);

	bool active() const { return mState != nullptr; }
	bool init(
		SDL_Window* window,
		const SfzImageViewConst& fontTexture,
		SfzAllocator* allocator,
		SfzConfig* cfg,
		SfzProfilingStats* profStats,
		zg::Uploader&& uploader);
	void destroy();

	// Getters
	// --------------------------------------------------------------------------------------------

	SfzRendererState& directAccessInternalState() { return *mState; }

	u64 currentFrameIdx() const; // Incremented each frameBegin()
	i32x2 windowResolution() const;

	// Returns the latest frame time retrieved and which frame idx it was related to.
	void frameTimeMs(u64& frameIdxOut, f32& frameTimeMsOut) const;

	ZgUploader* getUploader();

	// ImGui UI methods
	// --------------------------------------------------------------------------------------------

	void renderImguiUI();

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
		SfzStrID id, const SfzImageViewConst& image, bool generateMipmaps, SfzStrIDs* ids, SfzResourceManager* resMan);

	// Check if a texture is loaded or not
	bool textureLoaded(SfzStrID id, const SfzResourceManager* resMan) const;

	// Removes a texture from the renderer, will flush rendering.
	//
	// This operation flushes the rendering so we can guarantee no operation in progress is using
	// the texture to be removed. This of course means that this is a slow operation that will
	// cause frame stutter.
	//
	// WARNING: This must NOT be called between frameBegin() and frameFinish().
	void removeTextureGpuBlocking(SfzStrID id, SfzResourceManager* resMan);

	// Render methods
	// --------------------------------------------------------------------------------------------

	// Begins the frame, must be called before any other stage methods are called for a given frame.
	void frameBegin(
		SfzStrIDs* ids, SfzShaderManager* shaderMan, SfzResourceManager* resMan, SfzProfilingStats* profStats);

	// Command list methods
	sfz::HighLevelCmdList beginCommandList(
		const char* cmdListName,
		SfzStrIDs* ids,
		SfzProfilingStats* profStats,
		SfzShaderManager* shaderMan,
		SfzResourceManager* resMan);
	void executeCommandList(sfz::HighLevelCmdList cmdList);

	// Finished the frame, no additional stage methods may be called after this.
	void frameFinish();

private:
	SfzRendererState* mState = nullptr;
};
