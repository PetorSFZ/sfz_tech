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

#include <sfz/math/MathSupport.hpp>
#include <sfz/math/ProjectionMatrices.hpp>
#include <sfz/memory/CAllocatorWrapper.hpp>
#include <sfz/memory/New.hpp>
#include <sfz/strings/StackString.hpp>

#include <sfz/gl/Program.hpp>
#include <sfz/gl/FullscreenGeometry.hpp>
#include <sfz/gl/UniformSetters.hpp>

#include "ph/Model.hpp"

using namespace sfz;
using namespace ph;

// Constants
// ------------------------------------------------------------------------------------------------

static const uint32_t MAX_NUM_DYNAMIC_SPHERE_LIGHTS = 32;

// State
// ------------------------------------------------------------------------------------------------

struct RendererState final {

	// Disable copying & moving
	RendererState() noexcept = default;
	RendererState(const RendererState&) = delete;
	RendererState& operator= (const RendererState&) = delete;
	RendererState(RendererState&&) = delete;
	RendererState& operator= (RendererState&&) = delete;

	// Utilities
	CAllocatorWrapper allocator;
	SDL_Window* window = nullptr;
	phConfig config = {};
	phLogger logger = {};
	SDL_GLContext glContext = nullptr;

	// Resources
	gl::FullscreenGeometry fullscreenGeom;
	DynArray<Material> materials;
	DynArray<Model> dynamicModels;

	// Shaders
	uint32_t fbWidth, fbHeight;
	gl::Program shader;
	gl::Program modelShader;

	// Camera matrices
	mat4 viewMatrix = mat4::identity();
	mat4 projMatrix = mat4::identity();

	// Scene
	DynArray<ph::SphereLight> dynamicSphereLights;
};

static RendererState* statePtr = nullptr;

// Statics
// ------------------------------------------------------------------------------------------------

#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)

static void checkGLError(const char* file, int line) noexcept
{
	if (statePtr == nullptr) return;
	RendererState& state = *statePtr;

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

static void stupidSetSphereLightUniform(
	const gl::Program& program,
	const char* name,
	uint32_t index,
	const ph::SphereLight& sphereLight,
	const mat4& viewMatrix) noexcept
{
	StackString tmpStr;
	tmpStr.printf("%s[%u].%s", name, index, "vsPos");
	gl::setUniform(program, tmpStr.str, transformPoint(viewMatrix, sphereLight.pos));
	tmpStr.printf("%s[%u].%s", name, index, "radius");
	gl::setUniform(program, tmpStr.str, sphereLight.radius);
	tmpStr.printf("%s[%u].%s", name, index, "range");
	gl::setUniform(program, tmpStr.str, sphereLight.range);
	tmpStr.printf("%s[%u].%s", name, index, "strength");
	gl::setUniform(program, tmpStr.str, sphereLight.strength);
}

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

	// Create FullscreenGeometry
	state.fullscreenGeom.create(gl::FullscreenGeometryType::OGL_CLIP_SPACE_RIGHT_HANDED_FRONT_FACE);

	// Init resources arrays
	state.materials.create(256, &state.allocator);
	state.dynamicModels.create(128, &state.allocator);

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

		// SphereLight struct
		struct SphereLight {
			vec3 vsPos;
			float radius;
			float range;
			vec3 strength;
		};

		// Input
		varying vec3 vsPos;
		varying vec3 vsNormal;
		varying vec2 texcoord;

		// Uniforms
		const int MAX_NUM_DYNAMIC_SPHERE_LIGHTS = 32;
		uniform SphereLight uDynamicSphereLights[MAX_NUM_DYNAMIC_SPHERE_LIGHTS];
		uniform int uNumDynamicSphereLights;

		void main()
		{
			vec3 totalOutput = vec3(0.0);

			for (int i = 0; i < MAX_NUM_DYNAMIC_SPHERE_LIGHTS; i++) {
				SphereLight light = uDynamicSphereLights[i];

				vec3 toLight = light.vsPos - vsPos;
				float toLightDist = length(toLight);
				vec3 toLightDir = toLight * (1.0 / toLightDist);

				vec3 color = vec3(dot(toLightDir, vsNormal)) / toLightDist;

				if (i < uNumDynamicSphereLights) {
					totalOutput += clamp(color, vec3(0.0), vec3(1.0));
				}
			}

			gl_FragColor = vec4(totalOutput, 1.0);
		}
	)", [](uint32_t shaderProgram) {
		glBindAttribLocation(shaderProgram, 0, "inPos");
		glBindAttribLocation(shaderProgram, 1, "inNormal");
		glBindAttribLocation(shaderProgram, 2, "inTexcoord");
	});

	// Initialize array to hold dynamic sphere lights
	state.dynamicSphereLights.create(MAX_NUM_DYNAMIC_SPHERE_LIGHTS, &state.allocator);

	CHECK_GL_ERROR();
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

// Resource management (materials)
// ------------------------------------------------------------------------------------------------

DLL_EXPORT void phSetMaterials(const phMaterial* materials, uint32_t numMaterials)
{
	RendererState& state = *statePtr;

	// Remove any previous materials
	state.materials.clear();

	// Add materials to state
	state.materials.add(reinterpret_cast<const Material*>(materials), numMaterials);
}

DLL_EXPORT uint32_t phAddMaterial(const phMaterial* material)
{
	RendererState& state = *statePtr;

	uint32_t index = state.materials.size();
	state.materials.add(*reinterpret_cast<const Material*>(material));
	return index;
}

DLL_EXPORT uint32_t phUpdateMaterial(const phMaterial* material, uint32_t index)
{
	RendererState& state = *statePtr;

	// Check if material exists
	if (state.materials.size() <= index) return 0;

	state.materials[index] = *reinterpret_cast<const Material*>(material);
	return 1;
}

// Interface: Resource management (meshes)
// ------------------------------------------------------------------------------------------------

DLL_EXPORT void phSetDynamicMeshes(const phMesh* meshes, uint32_t numMeshes)
{
	RendererState& state = *statePtr;

	// Remove any previous models
	state.dynamicModels.clear();

	// Create models from all meshes and add them to state
	for (uint32_t i = 0; i < numMeshes; i++) {
		state.dynamicModels.add(Model(meshes[i], &state.allocator));
	}
}

DLL_EXPORT uint32_t phAddDynamicMesh(const phMesh* mesh)
{
	RendererState& state = *statePtr;

	uint32_t index = state.dynamicModels.size();
	state.dynamicModels.add(Model(*mesh, &state.allocator));
	return index;
}

DLL_EXPORT uint32_t phUpdateDynamicMesh(const phMesh* mesh, uint32_t index)
{
	RendererState& state = *statePtr;

	// Check if model exists
	if (state.dynamicModels.size() <= index) return 0;

	state.dynamicModels[index] = Model(*mesh, &state.allocator);
	return 1;
}

// Interface: Render commands
// ------------------------------------------------------------------------------------------------

DLL_EXPORT void phBeginFrame(
	const phCameraData* cameraPtr,
	const phSphereLight* dynamicSphereLights,
	uint32_t numDynamicSphereLights)
{
	RendererState& state = *statePtr;
	const CameraData& camera = *reinterpret_cast<const CameraData*>(cameraPtr);

	// Get size of default framebuffer
	int w = 0, h = 0;
	SDL_GL_GetDrawableSize(state.window, &w, &h);
	state.fbWidth = uint32_t(w);
	state.fbHeight = uint32_t(h);
	float aspect = float(w) / float(h);

	// Create camera matrices
	state.viewMatrix = viewMatrixGL(camera.pos, camera.dir, camera.up);
	state.projMatrix = perspectiveProjectionGL(camera.vertFovDeg, aspect, camera.near, camera.far);

	// Set dynamic sphere lights
	state.dynamicSphereLights.clear();
	state.dynamicSphereLights.insert(0,
		reinterpret_cast<const ph::SphereLight*>(dynamicSphereLights),
		min(numDynamicSphereLights, MAX_NUM_DYNAMIC_SPHERE_LIGHTS));

	// Set some GL settings
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Upload dynamic sphere lights to shader
	state.modelShader.useProgram();
	gl::setUniform(state.modelShader, "uNumDynamicSphereLights", int(state.dynamicSphereLights.size()));
	for (uint32_t i = 0; i < state.dynamicSphereLights.size(); i++) {
		stupidSetSphereLightUniform(state.modelShader, "uDynamicSphereLights", i,
			state.dynamicSphereLights[i], state.viewMatrix);
	}

	CHECK_GL_ERROR();
}

DLL_EXPORT void phRender(const phRenderEntity* entities, uint32_t numEntities)
{
	RendererState& state = *statePtr;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, state.fbWidth, state.fbHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//state.shader.useProgram();
	//state.fullscreenGeom.render();

	state.modelShader.useProgram();

	const mat4& projMatrix = state.projMatrix;
	const mat4& viewMatrix = state.viewMatrix;
	mat4 modelMatrix = mat4::identity();
	mat4 normalMatrix = inverse(transpose(viewMatrix * modelMatrix));

	gl::setUniform(state.modelShader, "uProjMatrix", projMatrix);
	gl::setUniform(state.modelShader, "uViewMatrix", viewMatrix);
	gl::setUniform(state.modelShader, "uModelMatrix", modelMatrix);
	gl::setUniform(state.modelShader, "uNormalMatrix", normalMatrix);

	for (uint32_t i = 0; i < numEntities; i++) {
		const auto& entity = reinterpret_cast<const ph::RenderEntity*>(entities)[i];
		auto& model = state.dynamicModels[entity.meshIndex];

		// TODO: Set model & normal matrix here

		model.bindVAO();
		auto& modelComponents = model.components();
		for (auto& component : modelComponents) {

			uint32_t materialIndex = component.materialIndex();
			// TODO: Set material here

			component.render();
		}
	}
}

DLL_EXPORT void phFinishFrame(void)
{
	RendererState& state = *statePtr;


	SDL_GL_SwapWindow(state.window);
	CHECK_GL_ERROR();
}

