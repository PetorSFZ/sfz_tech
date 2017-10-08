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

#include <algorithm> // std::swap()
#include <cstddef> // offsetof()

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES // This seems to enable extension use.
#include <SDL_opengles2.h>

#include <sfz/memory/CAllocatorWrapper.hpp>
#include <sfz/memory/New.hpp>

#include <sfz/gl/Program.hpp>
#include <sfz/gl/FullscreenGeometry.hpp>

#include "ph/Model.hpp"

using namespace sfz;
using namespace ph;

// State struct
// ------------------------------------------------------------------------------------------------

struct RendererState final {
	CAllocatorWrapper allocator;
	SDL_Window* window = nullptr;
	phConfig config = {};
	phLogger logger = {};
	SDL_GLContext glContext = nullptr;

	gl::FullscreenGeometry fullscreenGeom;
	gl::Program shader;

	Model model;
	gl::Program modelShader;
};

// Statics
// ------------------------------------------------------------------------------------------------

#define CHECK_GL_ERROR(state) checkGLError(state, __FILE__, __LINE__)

static void checkGLError(const RendererState& state, const char* file, int line) noexcept
{
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {

		switch (error) {
		case GL_INVALID_ENUM:
			PH_LOGGER_LOG(state.logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
				"%s:%i: GL_INVALID_ENUM", file, line);
			break;
		case GL_INVALID_VALUE:
			PH_LOGGER_LOG(state.logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
				"%s:%i: GL_INVALID_VALUE", file, line);
			break;
		case GL_INVALID_OPERATION:
			PH_LOGGER_LOG(state.logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
				"%s:%i: GL_INVALID_OPERATION", file, line);
			break;
		case GL_OUT_OF_MEMORY:
			PH_LOGGER_LOG(state.logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
				"%s:%i: GL_OUT_OF_MEMORY", file, line);
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			PH_LOGGER_LOG(state.logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
				"%s:%i: GL_INVALID_FRAMEBUFFER_OPERATION", file, line);
		}
	}
}

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
		PH_LOGGER_LOG(*logger, LOG_LEVEL_WARNING, "Renderer-WebGL",
		    "Renderer already initialized, returning.");
		return 1;
	}

	// Create OpenGL Context (OpenGL ES 2.0 == WebGL 1.0)
	PH_LOGGER_LOG(*logger, LOG_LEVEL_INFO, "Renderer-WebGL", "Creating OpenGL context");
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) < 0) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
		    "Failed to set GL context major version: %s", SDL_GetError());
		return 0;
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) < 0) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
		    "Failed to set GL context profile: %s", SDL_GetError());
		return 0;
	}
	SDL_GLContext tmpContext = SDL_GL_CreateContext(window); 
	if (tmpContext == nullptr) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
		    "Failed to create GL context: %s", SDL_GetError());
		return 0;
	}

	// Create internal state
	PH_LOGGER_LOG(*logger, LOG_LEVEL_INFO, "Renderer-WebGL", "Creating internal state");
	{
		CAllocatorWrapper tmp;
		tmp.setCAllocator(cAllocator);
		statePtr = sfzNew<RendererState>(&tmp);
		if (statePtr == nullptr) {
			PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
			    "Failed to allocate memory for internal state.");
			SDL_GL_DeleteContext(tmpContext);
			return 0;
		}
		statePtr->allocator.setCAllocator(cAllocator);
	}
	RendererState& state = *statePtr;

	// Store input parameters to state
	state.window = window;
	state.config = *config;
	state.logger = *logger;
	state.glContext = tmpContext;

	// Print information
	PH_LOGGER_LOG(*logger, LOG_LEVEL_INFO, "Renderer-WebGL",
	     "\nVendor: %s\nVersion: %s\nRenderer: %s",
	     glGetString(GL_VENDOR), glGetString(GL_VERSION), glGetString(GL_RENDERER));
	//PH_LOGGER_LOG(*logger, LOG_LEVEL_INFO, "Renderer-WebGL", "Extensions: %s",
	//    glGetString(GL_EXTENSIONS));

	// Create FullscreenVAO
	state.fullscreenGeom.create(gl::FullscreenGeometryType::OGL_CLIP_SPACE_RIGHT_HANDED_FRONT_FACE);

	// Compile shader program
	state.shader = gl::Program::postProcessFromSource(R"(
		precision mediump float;

		// Input
		varying vec2 texcoord;

		void main()
		{
			gl_FragColor = vec4(texcoord.x, texcoord.y, 0.0, 1.0);
			//gl_FragColor = vec4(0.0, 1.0, 1.0, 1.0);
		}
	)");


	ph::Mesh mesh;
	mesh.vertices.create(3, &state.allocator);
	mesh.vertices.addMany(3);

	// Bottom left corner
	mesh.vertices[0].pos = vec3(-1.0f, -1.0f, 0.0f);
	mesh.vertices[0].texcoord = vec2(0.0f, 0.0f);

	// Bottom right corner
	mesh.vertices[1].pos = vec3(1.0f, -1.0f, 0.0f);
	mesh.vertices[1].texcoord = vec2(2.0f, 0.0f);

	// Top left corner
	mesh.vertices[2].pos = vec3(-1.0f, 1.0f, 0.0f);
	mesh.vertices[2].texcoord = vec2(0.0f, 2.0f);

	mesh.materialIndices.create(3, &state.allocator);
	mesh.materialIndices.addMany(3);

	mesh.indices.create(3, &state.allocator);
	mesh.indices.addMany(3);

	mesh.indices[0] = 0;
	mesh.indices[1] = 1;
	mesh.indices[2] = 2;

	state.model.create(mesh.cView(), &state.allocator);

	state.modelShader = gl::Program::fromSource(R"(
		// Input
		attribute vec3 inPos;
		attribute vec3 inNormal;
		attribute vec2 inTexcoord;

		// Output
		varying vec2 texcoord;

		void main()
		{
			gl_Position = vec4(inPos, 1.0);
			texcoord = inTexcoord;
		}
	)", R"(
		precision mediump float;

		// Input
		varying vec2 texcoord;

		void main()
		{
			gl_FragColor = vec4(texcoord.x, texcoord.y, 0.0, 1.0);
			//gl_FragColor = vec4(0.0, 1.0, 1.0, 1.0);
		}
	)", [](uint32_t shaderProgram) {
		glBindAttribLocation(shaderProgram, 0, "inPos");
		glBindAttribLocation(shaderProgram, 1, "inNormal");
		glBindAttribLocation(shaderProgram, 2, "inTexcoord");
	});

	CHECK_GL_ERROR(state);
	PH_LOGGER_LOG(*logger, LOG_LEVEL_INFO, "Renderer-WebGL", "Finished initializing renderer");
	return 1;
}

DLL_EXPORT void phDeinitRenderer()
{
	if (statePtr == nullptr) return;
	RendererState& state = *statePtr;

	// Backups from state before destruction
	phLogger logger = state.logger;
	SDL_GLContext context = state.glContext;

	// Deallocate state
	PH_LOGGER_LOG(logger, LOG_LEVEL_INFO, "Renderer-WebGL", "Destroying state");
	{
		CAllocatorWrapper tmp;
		tmp.setCAllocator(state.allocator.cAllocator());
		sfzDelete(statePtr, &tmp);
	}
	statePtr = nullptr;

	// Destroy GL context
	PH_LOGGER_LOG(logger, LOG_LEVEL_INFO, "Renderer-WebGL", "Destroying OpenGL context");
	SDL_GL_DeleteContext(context);
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
	
}

DLL_EXPORT void phFinishFrame(void)
{
	RendererState& state = *statePtr;

	// Get size of default framebuffer
	int w = 0, h = 0;
	SDL_GL_GetDrawableSize(state.window, &w, &h);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//state.shader.useProgram();
	//state.fullscreenGeom.render();

	state.modelShader.useProgram();
	state.model.bindVAO();
	auto& modelComponents = state.model.components();
	for (auto& component : modelComponents) {

		// TODO: Set material uniforms here
		uint32_t materialIndex = component.materialIndex();

		component.render();
	}

	SDL_GL_SwapWindow(state.window);
	CHECK_GL_ERROR(state);
}