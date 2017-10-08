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

#include <sfz/math/ProjectionMatrices.hpp>
#include <sfz/memory/CAllocatorWrapper.hpp>
#include <sfz/memory/New.hpp>

#include <sfz/gl/Program.hpp>
#include <sfz/gl/FullscreenGeometry.hpp>
#include <sfz/gl/UniformSetters.hpp>

#include "ph/Model.hpp"

using namespace sfz;
using namespace ph;

// Cube model
// ------------------------------------------------------------------------------------------------

static const vec3 CUBE_POSITIONS[] = {
	// x, y, z
	// Left
	vec3(0.0f, 0.0f, 0.0f), // 0, left-bottom-back
	vec3(0.0f, 0.0f, 1.0f), // 1, left-bottom-front
	vec3(0.0f, 1.0f, 0.0f), // 2, left-top-back
	vec3(0.0f, 1.0f, 1.0f), // 3, left-top-front

	// Right
	vec3(1.0f, 0.0f, 0.0f), // 4, right-bottom-back
	vec3(1.0f, 0.0f, 1.0f), // 5, right-bottom-front
	vec3(1.0f, 1.0f, 0.0f), // 6, right-top-back
	vec3(1.0f, 1.0f, 1.0f), // 7, right-top-front

	// Bottom
	vec3(0.0f, 0.0f, 0.0f), // 8, left-bottom-back
	vec3(0.0f, 0.0f, 1.0f), // 9, left-bottom-front
	vec3(1.0f, 0.0f, 0.0f), // 10, right-bottom-back
	vec3(1.0f, 0.0f, 1.0f), // 11, right-bottom-front

	// Top
	vec3(0.0f, 1.0f, 0.0f), // 12, left-top-back
	vec3(0.0f, 1.0f, 1.0f), // 13, left-top-front
	vec3(1.0f, 1.0f, 0.0f), // 14, right-top-back
	vec3(1.0f, 1.0f, 1.0f), // 15, right-top-front

	// Back
	vec3(0.0f, 0.0f, 0.0f), // 16, left-bottom-back
	vec3(0.0f, 1.0f, 0.0f), // 17, left-top-back
	vec3(1.0f, 0.0f, 0.0f), // 18, right-bottom-back
	vec3(1.0f, 1.0f, 0.0f), // 19, right-top-back

	// Front
	vec3(0.0f, 0.0f, 1.0f), // 20, left-bottom-front
	vec3(0.0f, 1.0f, 1.0f), // 21, left-top-front
	vec3(1.0f, 0.0f, 1.0f), // 22, right-bottom-front
	vec3(1.0f, 1.0f, 1.0f)  // 23, right-top-front
};

static const vec3 CUBE_NORMALS[] = {
	// x, y, z
	// Left
	vec3(-1.0f, 0.0f, 0.0f), // 0, left-bottom-back
	vec3(-1.0f, 0.0f, 0.0f), // 1, left-bottom-front
	vec3(-1.0f, 0.0f, 0.0f), // 2, left-top-back
	vec3(-1.0f, 0.0f, 0.0f), // 3, left-top-front

	// Right
	vec3(1.0f, 0.0f, 0.0f), // 4, right-bottom-back
	vec3(1.0f, 0.0f, 0.0f), // 5, right-bottom-front
	vec3(1.0f, 0.0f, 0.0f), // 6, right-top-back
	vec3(1.0f, 0.0f, 0.0f), // 7, right-top-front

	// Bottom
	vec3(0.0f, -1.0f, 0.0f), // 8, left-bottom-back
	vec3(0.0f, -1.0f, 0.0f), // 9, left-bottom-front
	vec3(0.0f, -1.0f, 0.0f), // 10, right-bottom-back
	vec3(0.0f, -1.0f, 0.0f), // 11, right-bottom-front

	// Top
	vec3(0.0f, 1.0f, 0.0f), // 12, left-top-back
	vec3(0.0f, 1.0f, 0.0f), // 13, left-top-front
	vec3(0.0f, 1.0f, 0.0f), // 14, right-top-back
	vec3(0.0f, 1.0f, 0.0f), // 15, right-top-front

	// Back
	vec3(0.0f, 0.0f, -1.0f), // 16, left-bottom-back
	vec3(0.0f, 0.0f, -1.0f), // 17, left-top-back
	vec3(0.0f, 0.0f, -1.0f), // 18, right-bottom-back
	vec3(0.0f, 0.0f, -1.0f), // 19, right-top-back

	// Front
	vec3(0.0f, 0.0f, 1.0f), // 20, left-bottom-front
	vec3(0.0f, 0.0f, 1.0f), // 21, left-top-front
	vec3(0.0f, 0.0f, 1.0f), // 22, right-bottom-front
	vec3(0.0f, 0.0f, 1.0f)  // 23, right-top-front
};

static const vec2 CUBE_TEXCOORDS[] = {
	// u, v
	// Left
	vec2(0.0f, 0.0f), // 0, left-bottom-back
	vec2(1.0f, 0.0f), // 1, left-bottom-front
	vec2(0.0f, 1.0f), // 2, left-top-back
	vec2(1.0f, 1.0f), // 3, left-top-front

	// Right
	vec2(1.0f, 0.0f), // 4, right-bottom-back
	vec2(0.0f, 0.0f), // 5, right-bottom-front
	vec2(1.0f, 1.0f), // 6, right-top-back
	vec2(0.0f, 1.0f), // 7, right-top-front

	// Bottom
	vec2(0.0f, 0.0f), // 8, left-bottom-back
	vec2(0.0f, 1.0f), // 9, left-bottom-front
	vec2(1.0f, 0.0f), // 10, right-bottom-back
	vec2(1.0f, 1.0f), // 11, right-bottom-front

	// Top
	vec2(0.0f, 1.0f), // 12, left-top-back
	vec2(0.0f, 0.0f), // 13, left-top-front
	vec2(1.0f, 1.0f), // 14, right-top-back
	vec2(1.0f, 0.0f), // 15, right-top-front

	// Back
	vec2(1.0f, 0.0f), // 16, left-bottom-back
	vec2(1.0f, 1.0f), // 17, left-top-back
	vec2(0.0f, 0.0f), // 18, right-bottom-back
	vec2(0.0f, 1.0f), // 19, right-top-back

	// Front
	vec2(0.0f, 0.0f), // 20, left-bottom-front
	vec2(0.0f, 1.0f), // 21, left-top-front
	vec2(1.0f, 0.0f), // 22, right-bottom-front
	vec2(1.0f, 1.0f)  // 23, right-top-front
};

static const uint32_t CUBE_MATERIALS[] = {
	// Left
	0, 0, 0, 0,
	// Right
	0, 0, 0, 0,
	// Bottom
	0, 0, 0, 0,
	// Top
	0, 0, 0, 0,
	// Back
	0, 0, 0, 0,
	// Front
	0, 0, 0, 0
};

static const uint32_t CUBE_INDICES[] = {
	// Left
	0, 1, 2,
	3, 2, 1,

	// Right
	5, 4, 7,
	6, 7, 4,

	// Bottom
	8, 10, 9,
	11, 9, 10,

	// Top
	13, 15, 12,
	14, 12, 15,

	// Back
	18, 16, 19,
	17, 19, 16,

	// Front
	20, 22, 21,
	23, 21, 22
};

static const uint32_t CUBE_NUM_VERTICES = sizeof(CUBE_POSITIONS) / sizeof(vec3);
static const uint32_t CUBE_NUM_INDICES = sizeof(CUBE_INDICES) / sizeof(uint32_t);

static Model createCubeModel(Allocator* allocator) noexcept
{
	// Create mesh from hardcoded values
	ph::Mesh mesh;
	mesh.vertices.create(CUBE_NUM_VERTICES, allocator);
	mesh.vertices.addMany(CUBE_NUM_VERTICES);
	mesh.materialIndices.create(CUBE_NUM_VERTICES, allocator);
	mesh.materialIndices.addMany(CUBE_NUM_VERTICES);
	for (uint32_t i = 0; i < CUBE_NUM_VERTICES; i++) {
		mesh.vertices[i].pos = CUBE_POSITIONS[i];
		mesh.vertices[i].normal = CUBE_NORMALS[i];
		mesh.vertices[i].texcoord = CUBE_TEXCOORDS[i];
		mesh.materialIndices[i] = CUBE_MATERIALS[i];
	}
	mesh.indices.create(CUBE_NUM_INDICES, allocator);
	mesh.indices.add(CUBE_INDICES, CUBE_NUM_INDICES);

	// Create model from mesh
	Model tmpModel;
	tmpModel.create(mesh.cView(), allocator);

	// Return model
	return tmpModel;
}

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


	state.model = createCubeModel(&state.allocator);

	state.modelShader = gl::Program::fromSource(R"(
		// Input
		attribute vec3 inPos;
		attribute vec3 inNormal;
		attribute vec2 inTexcoord;

		// Output
		varying vec3 vsPos;
		varying vec3 vsNormal;
		varying vec2 texcoord;

		// Uniforms
		uniform mat4 uProjMatrix;
		uniform mat4 uViewMatrix;
		uniform mat4 uModelMatrix;
		uniform mat4 uNormalMatrix; // inverse(transpose(modelViewMatrix)) for non-uniform scaling

		void main()
		{
			vec4 vsPosTmp = uViewMatrix * uModelMatrix * vec4(inPos, 1.0);
			
			vsPos = vsPosTmp.xyz / vsPosTmp.w; // Unsure if division necessary.
			vsNormal = (uNormalMatrix * vec4(inNormal, 0.0)).xyz;
			texcoord = inTexcoord;

			gl_Position = uProjMatrix * vsPosTmp;
		}
	)", R"(
		precision mediump float;

		// Input
		varying vec3 vsPos;
		varying vec3 vsNormal;
		varying vec2 texcoord;

		// Uniforms
		uniform vec3 color;

		void main()
		{
			gl_FragColor = vec4(vsNormal, 1.0);
			//gl_FragColor = vec4(texcoord.x, texcoord.y, 0.0, 1.0);
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

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//state.shader.useProgram();
	//state.fullscreenGeom.render();

	state.modelShader.useProgram();

	float yFovDeg = 90.0f;
	float aspectRatio = 1.0f;
	float zNear = 0.01f;
	float zFar = 10.0f;
	mat4 projMatrix = perspectiveProjectionGL(yFovDeg, aspectRatio, zNear, zFar);
	mat4 viewMatrix = viewMatrixGL(vec3(3.0f, 3.0f, 3.0f), vec3(-1.0f, -0.25f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 modelMatrix = mat4::identity();
	mat4 normalMatrix = inverse(transpose(viewMatrix * modelMatrix));

	gl::setUniform(state.modelShader, "uProjMatrix", projMatrix);
	gl::setUniform(state.modelShader, "uViewMatrix", viewMatrix);
	gl::setUniform(state.modelShader, "uModelMatrix", modelMatrix);
	gl::setUniform(state.modelShader, "uNormalMatrix", normalMatrix);

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