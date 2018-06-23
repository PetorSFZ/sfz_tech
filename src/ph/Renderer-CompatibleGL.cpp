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

#define PH_EXPORTING_RENDERER_FUNCTIONS
#include "ph/RendererInterface.h"

#include <algorithm> // std::swap()
#include <cstddef> // offsetof()

#include <SDL.h>

#include <sfz/Context.hpp>
#include <sfz/Logging.hpp>
#include <sfz/gl/IncludeOpenGL.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/math/ProjectionMatrices.hpp>
#include <sfz/memory/New.hpp>
#include <sfz/strings/StackString.hpp>

#include <sfz/gl/Program.hpp>
#include <sfz/gl/Framebuffer.hpp>
#include <sfz/gl/FullscreenGeometry.hpp>
#include <sfz/gl/UniformSetters.hpp>

#include <ph/rendering/CameraData.hpp>
#include <ph/rendering/ImageView.hpp>
#include <ph/rendering/ImguiRenderingData.hpp>
#include <ph/rendering/Material.hpp>
#include <ph/rendering/MeshView.hpp>
#include <ph/rendering/RenderEntity.hpp>
#include <ph/rendering/SphereLight.hpp>

#include "ph/ImguiRendering.hpp"
#include "ph/Model.hpp"
#include "ph/Shaders.hpp"
#include "ph/Texture.hpp"

using namespace sfz;
using namespace ph;

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
	sfz::Allocator* allocator;
	SDL_Window* window = nullptr;
	phConfig config = {};
	SDL_GLContext glContext = nullptr;

	// Resources
	gl::FullscreenGeometry fullscreenGeom;
	DynArray<Texture> textures;
	DynArray<phMaterial> materials;
	DynArray<Model> dynamicModels;

	// Window information
	int32_t windowWidth, windowHeight;
	int32_t fbWidth, fbHeight;
	float aspect;

	// Framebuffers
	gl::Framebuffer internalFB;

	// Shaders
	gl::Program modelShader, copyOutShader;

	// Camera matrices
	mat4 viewMatrix = mat4::identity();
	mat4 projMatrix = mat4::identity();

	// Scene
	DynArray<phSphereLight> dynamicSphereLights;

	// Imgui
	ImguiVertexData imguiGlCmdList;
	Texture imguiFontTexture;
	DynArray<phImguiCommand> imguiCommands;
	gl::Program imguiShader;
	const phSettingValue* imguiScaleSetting = nullptr;
	const phSettingValue* imguiFontLinearSetting = nullptr;
};

static RendererState* statePtr = nullptr;

// Statics
// ------------------------------------------------------------------------------------------------

#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)

static void checkGLError(const char* file, int line) noexcept
{
	if (statePtr == nullptr) return;

	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		switch (error) {
		case GL_INVALID_ENUM:
			SFZ_ERROR("Renderer-CompatibleGL", "%s:%i: GL_INVALID_ENUM", file, line);
			break;
		case GL_INVALID_VALUE:
			SFZ_ERROR("Renderer-CompatibleGL", "%s:%i: GL_INVALID_VALUE", file, line);
			break;
		case GL_INVALID_OPERATION:
			SFZ_ERROR("Renderer-CompatibleGL", "%s:%i: GL_INVALID_OPERATION", file, line);
			break;
		case GL_OUT_OF_MEMORY:
			SFZ_ERROR("Renderer-CompatibleGL", "%s:%i: GL_OUT_OF_MEMORY", file, line);
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			SFZ_ERROR("Renderer-CompatibleGL", "%s:%i: GL_INVALID_FRAMEBUFFER_OPERATION", file, line);
		}
	}
}

static void stupidSetSphereLightUniform(
	const gl::Program& program,
	const char* name,
	uint32_t index,
	const phSphereLight& sphereLight,
	const mat4& viewMatrix) noexcept
{
	gl::setUniform(program, str80("%s[%u].%s", name, index, "vsPos"),
		transformPoint(viewMatrix, sphereLight.pos));
	gl::setUniform(program, str80("%s[%u].%s", name, index, "radius"), sphereLight.radius);
	gl::setUniform(program, str80("%s[%u].%s", name, index, "range"), sphereLight.range);
	gl::setUniform(program, str80("%s[%u].%s", name, index, "strength"), sphereLight.strength);
}

static void stupidSetMaterialUniform(
	const gl::Program& program,
	const char* name,
	const phMaterial& m) noexcept
{
	auto toInt = [](uint16_t tex) -> int32_t {
		return (tex == uint16_t(~0)) ? 0 : 1;
	};

	gl::setUniform(program, str80("%s.albedo", name), vec4(m.albedo) * (1.0f / 255.0f));
	gl::setUniform(program, str80("%s.emissive", name), vec3(m.emissive) * (1.0f / 255.0f));
	gl::setUniform(program, str80("%s.roughness", name), float(m.roughness) * (1.0f / 255.0f));
	gl::setUniform(program, str80("%s.metallic", name), float(m.metallic) * (1.0f / 255.0f));

	gl::setUniform(program, str80("%s.hasAlbedoTexture", name), toInt(m.albedoTexIndex));
	gl::setUniform(program, str80("%s.hasMetallicRoughnessTexture", name),
		toInt(m.metallicRoughnessTexIndex));
	gl::setUniform(program, str80("%s.hasNormalTexture", name), toInt(m.normalTexIndex));
	gl::setUniform(program, str80("%s.hasOcclusionTexture", name), toInt(m.occlusionTexIndex));
	gl::setUniform(program, str80("%s.hasEmissiveTexture", name), toInt(m.emissiveTexIndex));
}

// Interface: Init functions
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
uint32_t phRendererInterfaceVersion(void)
{
	return 8;
}

extern "C" PH_DLL_EXPORT
uint32_t phRequiredSDL2WindowFlags(void)
{
	return SDL_WINDOW_OPENGL;
}

extern "C" PH_DLL_EXPORT
uint32_t phInitRenderer(
	void* sfzCoreContext,
	SDL_Window* window,
	void* allocator,
	phConfig* config)
{
	// Return if already initialized
	if (statePtr != nullptr) {
		SFZ_WARNING("Renderer-CompatibleGL", "Renderer already initialized, returning.");
		return 1;
	}

	// Set sfzCore context
	if (!sfz::setContext(reinterpret_cast<sfz::Context*>(sfzCoreContext))) {
		SFZ_INFO("Renderer-CompatibleGL",
			"sfzCore Context already set, expected if renderer is statically linked");
	}

	SFZ_INFO("Renderer-CompatibleGL", "Creating OpenGL context");
#ifdef __EMSCRIPTEN__
	// Create OpenGL Context (OpenGL ES 2.0 == WebGL 1.0)
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) < 0) {
		SFZ_ERROR("Renderer-CompatibleGL", "Failed to set GL context major version: %s",
			SDL_GetError());
		return 0;
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) < 0) {
		SFZ_ERROR("Renderer-CompatibleGL", "Failed to set GL context profile: %s", SDL_GetError());
		return 0;
	}
#else
	// Create OpenGL Context (OpenGL 3.3)
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) < 0) {
		SFZ_ERROR("Renderer-CompatibleGL", "Failed to set GL context major version: %s",
			SDL_GetError());
		return 0;
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) < 0) {
		SFZ_ERROR("Renderer-CompatibleGL", "Failed to set GL context minor version: %s",
			SDL_GetError());
		return 0;
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) < 0) {
		SFZ_ERROR("Renderer-CompatibleGL", "Failed to set GL context profile: %s", SDL_GetError());
		return 0;
	}
#endif

	SDL_GLContext tmpContext = SDL_GL_CreateContext(window);
	if (tmpContext == nullptr) {
		SFZ_ERROR("Renderer-CompatibleGL", "Failed to create GL context: %s", SDL_GetError());
		return 0;
	}

	// Load GLEW on not emscripten
#ifndef __EMSCRIPTEN__
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		SFZ_ERROR("Renderer-CompatibleGL", "GLEW init failure: %s", glewGetErrorString(glewError));
	}
#endif

	// Create internal state
	SFZ_INFO("Renderer-CompatibleGL", "Creating internal state");
	{
		Allocator* tmp = reinterpret_cast<Allocator*>(allocator);
		statePtr = sfzNew<RendererState>(tmp);
		if (statePtr == nullptr) {
			SFZ_ERROR("Renderer-CompatibleGL", "Failed to allocate memory for internal state.");
			SDL_GL_DeleteContext(tmpContext);
			return 0;
		}
		statePtr->allocator = tmp;
	}
	RendererState& state = *statePtr;

	// Store input parameters to state
	state.window = window;
	state.config = *config;
	state.glContext = tmpContext;

	// Print information
	SFZ_INFO("Renderer-CompatibleGL", "Vendor: %s\nVersion: %s\nRenderer: %s",
		glGetString(GL_VENDOR), glGetString(GL_VERSION), glGetString(GL_RENDERER));

	// Create FullscreenGeometry
	state.fullscreenGeom.create(gl::FullscreenGeometryType::OGL_CLIP_SPACE_RIGHT_HANDED_FRONT_FACE);

	// Init resources arrays
	state.textures.create(256, state.allocator);
	state.materials.create(256, state.allocator);
	state.dynamicModels.create(128, state.allocator);

	// Create Framebuffers
	int w, h;
	SDL_GL_GetDrawableSize(window, &w, &h);
	state.internalFB = gl::FramebufferBuilder(w, h)
		.addTexture(0, gl::FBTextureFormat::RGBA_U8, gl::FBTextureFiltering::LINEAR)
#ifdef __EMSCRIPTEN__
		.addDepthBuffer(gl::FBDepthFormat::F16)
#else
		.addDepthBuffer(gl::FBDepthFormat::F32)
#endif
		.build();

	// Compile shaders
	state.modelShader = gl::Program::fromSource(
		SHADER_HEADER_SRC,
		VERTEX_SHADER_SRC,
		FRAGMENT_SHADER_SRC,
		[](uint32_t shaderProgram) {
			glBindAttribLocation(shaderProgram, 0, "inPos");
			glBindAttribLocation(shaderProgram, 1, "inNormal");
			glBindAttribLocation(shaderProgram, 2, "inTexcoord");
		},
		state.allocator);

	state.copyOutShader = gl::Program::postProcessFromSource(
		SHADER_HEADER_SRC,
		COPY_OUT_SHADER_SRC,
		state.allocator);

	// Initialize array to hold dynamic sphere lights
	state.dynamicSphereLights.create(MAX_NUM_DYNAMIC_SPHERE_LIGHTS, state.allocator);

	CHECK_GL_ERROR();
	SFZ_INFO("Renderer-CompatibleGL", "Finished initializing renderer");
	return 1;
}

extern "C" PH_DLL_EXPORT
void phDeinitRenderer(void)
{
	if (statePtr == nullptr) return;
	RendererState& state = *statePtr;

	// Backups from state before destruction
	SDL_GLContext context = state.glContext;

	// Deallocate state
	SFZ_INFO("Renderer-CompatibleGL", "Destroying state");
	{
		Allocator* tmp = state.allocator;
		sfzDelete(statePtr, tmp);
	}
	statePtr = nullptr;

	// Destroy GL context
	SFZ_INFO("Renderer-CompatibleGL", "Destroying OpenGL context");
	SDL_GL_DeleteContext(context);
}

extern "C" PH_DLL_EXPORT
void phInitImgui(const phConstImageView* fontTexture)
{
	RendererState& state = *statePtr;

	// Init imgui settings
	state.imguiScaleSetting = state.config.sanitizeFloat("Imgui", "scale",
		Bool32(true), FloatBounds(2.0f, 1.0f, 3.0f).cPtr());
	state.imguiFontLinearSetting = state.config.sanitizeBool("Imgui", "bilinearFontSampling",
		Bool32(true), BoolBounds(false).cPtr());

	TextureFiltering fontFiltering = Bool32(state.imguiFontLinearSetting->b.value) ?
		TextureFiltering::BILINEAR : TextureFiltering::NEAREST;

	// Upload font texture to GL memory
	state.imguiFontTexture.create(*fontTexture, fontFiltering);

	// Initialize cpu temp memory for imgui commands
	state.imguiCommands.create(4096, state.allocator);

	// Creating OpenGL memory for vertices and indices
	state.imguiGlCmdList.create(4096, 4096);

	// Compile Imgui shader
	state.imguiShader = gl::Program::fromSource(
		SHADER_HEADER_SRC,
		IMGUI_VERTEX_SHADER_SRC,
		IMGUI_FRAGMENT_SHADER_SRC,
		[](uint32_t shaderProgram) {
			glBindAttribLocation(shaderProgram, 0, "inPos");
			glBindAttribLocation(shaderProgram, 1, "inTexcoord");
			glBindAttribLocation(shaderProgram, 2, "inColor");
		},
		state.allocator);

	// Always read font texture from location 0
	state.imguiShader.useProgram();
	gl::setUniform(state.imguiShader, "uTexture", 0);
}

// State query functions
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phImguiWindowDimensions(float* widthOut, float* heightOut)
{
	RendererState& state = *statePtr;

	// Retrieve scale factor from config
	float scaleFactor = 1.0f;
	if (state.imguiScaleSetting != nullptr) scaleFactor = 1.0f / state.imguiScaleSetting->f.value;

	int w, h;
	SDL_GL_GetDrawableSize(state.window, &w, &h);
	if (widthOut != nullptr) *widthOut = float(w) * scaleFactor;
	if (heightOut != nullptr) *heightOut = float(h) * scaleFactor;
}

// Resource management (textures)
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phSetTextures(const phConstImageView* textures, uint32_t numTextures)
{
	RendererState& state = *statePtr;

	// Remove any previous textures
	state.textures.clear();

	// Create textures from all images and add them to state
	for (uint32_t i = 0; i < numTextures; i++) {
		state.textures.add(Texture(textures[i]));
	}
}

extern "C" PH_DLL_EXPORT
uint32_t phAddTexture(const phConstImageView* texture)
{
	RendererState& state = *statePtr;

	uint32_t index = state.textures.size();
	state.textures.add(Texture(*texture));
	return index;
}

extern "C" PH_DLL_EXPORT
uint32_t phUpdateTexture(const phConstImageView* texture, uint32_t index)
{
	RendererState& state = *statePtr;

	// Check if texture exists
	if (state.textures.size() <= index) return 0;

	state.textures[index] = Texture(*texture);
	return 1;
}

// Resource management (materials)
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phSetMaterials(const phMaterial* materials, uint32_t numMaterials)
{
	RendererState& state = *statePtr;

	// Remove any previous materials
	state.materials.clear();

	// Add materials to state
	state.materials.add(materials, numMaterials);
}

extern "C" PH_DLL_EXPORT
uint32_t phAddMaterial(const phMaterial* material)
{
	RendererState& state = *statePtr;

	uint32_t index = state.materials.size();
	state.materials.add(*material);
	return index;
}

extern "C" PH_DLL_EXPORT
uint32_t phUpdateMaterial(const phMaterial* material, uint32_t index)
{
	RendererState& state = *statePtr;

	// Check if material exists
	if (state.materials.size() <= index) return 0;

	state.materials[index] = *material;
	return 1;
}

// Interface: Resource management (meshes)
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phSetDynamicMeshes(const phConstMeshView* meshes, uint32_t numMeshes)
{
	RendererState& state = *statePtr;

	// Remove any previous models
	state.dynamicModels.clear();

	// Create models from all meshes and add them to state
	for (uint32_t i = 0; i < numMeshes; i++) {
		state.dynamicModels.add(Model(meshes[i], state.allocator));
	}
}

extern "C" PH_DLL_EXPORT
uint32_t phAddDynamicMesh(const phConstMeshView* mesh)
{
	RendererState& state = *statePtr;

	uint32_t index = state.dynamicModels.size();
	state.dynamicModels.add(Model(*mesh, state.allocator));
	return index;
}

extern "C" PH_DLL_EXPORT
uint32_t phUpdateDynamicMesh(const phConstMeshView* mesh, uint32_t index)
{
	RendererState& state = *statePtr;

	// Check if model exists
	if (state.dynamicModels.size() <= index) return 0;

	state.dynamicModels[index] = Model(*mesh, state.allocator);
	return 1;
}

// Interface: Render commands
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phBeginFrame(
	const phCameraData* camera,
	const phSphereLight* dynamicSphereLights,
	uint32_t numDynamicSphereLights)
{
	RendererState& state = *statePtr;

	// Get size of default framebuffer and window
	SDL_GetWindowSize(state.window, &state.windowWidth, &state.windowHeight);
	SDL_GL_GetDrawableSize(state.window, &state.fbWidth, &state.fbHeight);
	state.aspect = float(state.fbWidth) / float(state.fbHeight);

	// Create camera matrices
	state.viewMatrix = viewMatrixGL(camera->pos, camera->dir, camera->up);
	state.projMatrix =
		perspectiveProjectionGL(camera->vertFovDeg, state.aspect, camera->near, camera->far);

	// Set dynamic sphere lights
	state.dynamicSphereLights.clear();
	state.dynamicSphereLights.insert(0, dynamicSphereLights,
		min(numDynamicSphereLights, MAX_NUM_DYNAMIC_SPHERE_LIGHTS));

	// Set some GL settings
	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_SCISSOR_TEST);

	// Upload dynamic sphere lights to shader
	state.modelShader.useProgram();
	gl::setUniform(state.modelShader, "uNumDynamicSphereLights", int(state.dynamicSphereLights.size()));
	for (uint32_t i = 0; i < state.dynamicSphereLights.size(); i++) {
		stupidSetSphereLightUniform(state.modelShader, "uDynamicSphereLights", i,
			state.dynamicSphereLights[i], state.viewMatrix);
	}

	// Prepare internal framebuffer for rendering
	state.internalFB.bindViewportClearColorDepth(vec4(0.0f));

	CHECK_GL_ERROR();
}

extern "C" PH_DLL_EXPORT
void phRenderImgui(
	const phImguiVertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices,
	const phImguiCommand* commands,
	uint32_t numCommands)
{
	RendererState& state = *statePtr;

	// Clear and copy commands
	state.imguiCommands.clear();
	state.imguiCommands.add(commands, numCommands);

	// Upload vertices and indices to GPU
	state.imguiGlCmdList.upload(vertices, numVertices, indices, numIndices);
}

extern "C" PH_DLL_EXPORT
void phRender(const phRenderEntity* entities, uint32_t numEntities)
{
	RendererState& state = *statePtr;

	state.modelShader.useProgram();

	const mat4& projMatrix = state.projMatrix;
	const mat4& viewMatrix = state.viewMatrix;

	gl::setUniform(state.modelShader, "uProjMatrix", projMatrix);
	gl::setUniform(state.modelShader, "uViewMatrix", viewMatrix);
	int modelMatrixLoc = glGetUniformLocation(state.modelShader.handle(), "uModelMatrix");
	int normalMatrixLoc = glGetUniformLocation(state.modelShader.handle(), "uNormalMatrix");

	gl::setUniform(state.modelShader, "uAlbedoTexture", 0);
	gl::setUniform(state.modelShader, "uMetallicRoughnessTexture", 1);
	gl::setUniform(state.modelShader, "uNormalTexture", 2);
	gl::setUniform(state.modelShader, "uOcclusionTexture", 3);
	gl::setUniform(state.modelShader, "uEmissiveTexture", 4);

	for (uint32_t i = 0; i < numEntities; i++) {
		const phRenderEntity& entity = entities[i];
		auto& model = state.dynamicModels[entity.meshIndex];

		// Set model and normal matrices
		gl::setUniform(modelMatrixLoc, mat4(entity.transform));
		mat4 normalMatrix = inverse(transpose(viewMatrix * mat4(entity.transform)));
		gl::setUniform(normalMatrixLoc, normalMatrix);

		model.bindVAO();
		auto& modelComponents = model.components();
		for (auto& component : modelComponents) {

			// Upload component's material to shader
			uint32_t materialIndex = component.materialIndex();
			const auto& material = state.materials[materialIndex];
			stupidSetMaterialUniform(state.modelShader, "uMaterial", material);

			// Bind materials textures
			if (material.albedoTexIndex != uint16_t(~0)) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, state.textures[material.albedoTexIndex].handle());
			}
			if (material.metallicRoughnessTexIndex != uint16_t(~0)) {
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D,
					state.textures[material.metallicRoughnessTexIndex].handle());
			}
			if (material.normalTexIndex != uint16_t(~0)) {
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, state.textures[material.normalTexIndex].handle());
			}
			if (material.occlusionTexIndex != uint16_t(~0)) {
				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, state.textures[material.occlusionTexIndex].handle());
			}
			if (material.emissiveTexIndex != uint16_t(~0)) {
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, state.textures[material.emissiveTexIndex].handle());
			}

			// Render component of mesh
			component.render();
		}
	}
}

extern "C" PH_DLL_EXPORT
void phFinishFrame(void)
{
	RendererState& state = *statePtr;

	// Bind and clear output framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, state.fbWidth, state.fbHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render out internal framebuffer to window using copyOutShader
	state.copyOutShader.useProgram();
	gl::setUniform(state.copyOutShader, "uTexture", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, state.internalFB.textures[0]);
	state.fullscreenGeom.render();


	// Imgui Rendering

	// Store some previous OpenGL state
	GLint lastScissorBox[4]; glGetIntegerv(GL_SCISSOR_BOX, lastScissorBox);

	// Set some OpenGL state
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);

	// Bind imgui shader and set some uniforms
	state.imguiShader.useProgram();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, state.imguiFontTexture.handle());

	// Update font filtering
	TextureFiltering imguiFontFiltering = Bool32(state.imguiFontLinearSetting->b.value) ?
		TextureFiltering::BILINEAR : TextureFiltering::NEAREST;
	state.imguiFontTexture.setFilteringFormat(imguiFontFiltering);

	// Retrieve imgui scale factor
	float imguiScaleFactor = 1.0f;
	if (state.imguiScaleSetting != nullptr) imguiScaleFactor /= state.imguiScaleSetting->f.value;
	float imguiInvScaleFactor = 1.0f / imguiScaleFactor;

	float imguiWidth = state.fbWidth * imguiScaleFactor;
	float imguiHeight = state.fbHeight * imguiScaleFactor;

	mat44 projMatrix;
	projMatrix.row0 = vec4(2.0f / imguiWidth, 0.0f, 0.0f, -1.0f);
	projMatrix.row1 = vec4(0.0f, 2.0f / -imguiHeight, 0.0f, 1.0f);
	projMatrix.row2 = vec4(0.0f, 0.0f, -1.0f, 0.0f);
	projMatrix.row3 = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	gl::setUniform(state.imguiShader, "uProjMatrix", projMatrix);

	// Bind gl command list
	state.imguiGlCmdList.bindVAO();

	// Render commands
	for (const phImguiCommand& cmd : state.imguiCommands) {

		glScissor(
			GLsizei(cmd.clipRect.x * imguiInvScaleFactor),
			GLsizei(state.fbHeight - (cmd.clipRect.w * imguiInvScaleFactor)),
			GLsizei((cmd.clipRect.z - cmd.clipRect.x) * imguiInvScaleFactor),
			GLsizei((cmd.clipRect.w - cmd.clipRect.y) * imguiInvScaleFactor));

		state.imguiGlCmdList.render(cmd.idxBufferOffset, cmd.numIndices);
		CHECK_GL_ERROR();
	}

	// Restore some previous OpenGL state
	glScissor(lastScissorBox[0], lastScissorBox[1], lastScissorBox[2], lastScissorBox[3]);
	glDisable(GL_SCISSOR_TEST);


	// Swap back and front buffers
	SDL_GL_SwapWindow(state.window);
	CHECK_GL_ERROR();
}
