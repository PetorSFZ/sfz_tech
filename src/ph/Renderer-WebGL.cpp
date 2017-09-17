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

#include "ph/RendererInterface.h"

#include <SDL.h>
#include <SDL_opengles2.h>

#include <sfz/memory/CAllocatorWrapper.hpp>
#include <sfz/memory/New.hpp>

using namespace sfz;

// State struct
// ------------------------------------------------------------------------------------------------

struct RendererState final {
	CAllocatorWrapper allocator;
	SDL_Window* window = nullptr;
	phConfig config = {};
	phLogger logger = {};

	
	SDL_GLContext glContext = nullptr;
};

// Statics
// ------------------------------------------------------------------------------------------------

static RendererState* statePtr = nullptr;

// Interface: Init functions
// ------------------------------------------------------------------------------------------------

DLL_EXPORT uint32_t phRendererInterfaceVersion(void)
{
	return 1;
}

DLL_EXPORT uint32_t phRequiredSDL2WindowFlags(void)
{
	return SDL_WINDOW_OPENGL;
}

DLL_EXPORT uint32_t phInitRenderer(
	SDL_Window* window,
	sfzAllocator* cAllocator,
	phConfig* config,
	phLogger* logger)
{
	if (statePtr != nullptr) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_WARNING, "Renderer-WebGL", "Renderer already initialized, returning.");
		return 1;
	}

	// Create internal state
	{
		CAllocatorWrapper tmp;
		tmp.setCAllocator(cAllocator);
		statePtr = sfzNew<RendererState>(&tmp);
		if (statePtr == nullptr) {
			PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL", "Failed to allocate memory for internal state.");
			return 0;
		}
		statePtr->allocator.setCAllocator(cAllocator);
	}
	RendererState& state = *statePtr;

	// Store input parameters to state
	state.window = window;
	state.config = *config;
	state.logger = *logger;

	// OpenGL Context (OpenGL ES 2.0 == WebGL 1.0)
	bool failed = false;
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) < 0) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL", "Failed to set gl context major version: %s", SDL_GetError());
		failed = true;
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) < 0) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL", "Failed to set gl context profile: %s", SDL_GetError());
		failed = true;
	}
	bool debugContext = false;
	if (debugContext) {
		if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG) < 0) {
			PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL", "Failed to request debug context: %s", SDL_GetError());
			failed = true;
		}
	}

	// Bail out if we couldn't set settings
	if (failed) {
		sfzDelete(statePtr, &state.allocator);
		statePtr = nullptr;
		return 0;
	}

	// Create context (and bail out on failure)
	state.glContext = SDL_GL_CreateContext(state.window); 
	if (state.glContext == nullptr) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL", "Failed to create GL context: %s", SDL_GetError());
		sfzDelete(statePtr, &state.allocator);
		statePtr = nullptr;
		return 0;
	}

	// Print information
	PH_LOGGER_LOG(*logger, LOG_LEVEL_INFO, "Renderer-WebGL", "\nVendor: %s\nVersion: %s\nRenderer: %s",
	              glGetString(GL_VENDOR), glGetString(GL_VERSION), glGetString(GL_RENDERER));

	return 1;
}

DLL_EXPORT void phDeinitRenderer()
{
	if (statePtr == nullptr) return;
	RendererState& state = *statePtr;

	// Destroy GL context
	PH_LOGGER_LOG(state.logger, LOG_LEVEL_INFO, "Renderer-WebGL", "Destroy OpenGL context");
	SDL_GL_DeleteContext(state.glContext);

	// Deallocate state
	{
		CAllocatorWrapper tmp;
		tmp.setCAllocator(state.allocator.cAllocator());
		sfzDelete(statePtr, &tmp);
	}
	statePtr = nullptr;
}

// Interface: Render commands
// ------------------------------------------------------------------------------------------------

DLL_EXPORT void phBeginFrame(
	const phCameraData* camera,
	const phSphereLight* dynamicSphereLights,
	uint32_t numDynamicSphereLights)
{

}

DLL_EXPORT void phRender(const phRenderEntity* entities, uint32_t numEntities)
{
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	//glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

DLL_EXPORT void phFinishFrame(void)
{
	RendererState& state = *statePtr;
	SDL_GL_SwapWindow(state.window);
}