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

#include <imgui.h>

#include <sfz/gl/IncludeOpenGL.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/math/ProjectionMatrices.hpp>
#include <sfz/memory/CAllocatorWrapper.hpp>
#include <sfz/memory/New.hpp>
#include <sfz/strings/StackString.hpp>

#include <sfz/gl/Program.hpp>
#include <sfz/gl/Framebuffer.hpp>
#include <sfz/gl/FullscreenGeometry.hpp>
#include <sfz/gl/UniformSetters.hpp>

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
	CAllocatorWrapper allocator;
	SDL_Window* window = nullptr;
	phConfig config = {};
	phLogger logger = {};
	SDL_GLContext glContext = nullptr;

	// Resources
	gl::FullscreenGeometry fullscreenGeom;
	DynArray<Texture> textures;
	DynArray<Material> materials;
	DynArray<Model> dynamicModels;

	// Window information
	int32_t windowWidth, windowHeight;
	int32_t fbWidth, fbHeight;
	float aspect;

	// Framebuffers
	gl::Framebuffer internalFB;

	// Shaders
	gl::Program modelShader, copyOutShader, imguiShader;

	// Camera matrices
	mat4 viewMatrix = mat4::identity();
	mat4 projMatrix = mat4::identity();

	// Scene
	DynArray<ph::SphereLight> dynamicSphereLights;

	// Imgui
	ImguiVertexData imguiGlCmdList;
	Texture imguiFontTexture;
};

static RendererState* statePtr = nullptr;

// Statics
// ------------------------------------------------------------------------------------------------

#define LOG_INFO_F(format, ...) \
	PH_LOGGER_LOG(statePtr->logger, LOG_LEVEL_INFO, "Renderer-CompatibleGL", \
		(format), ##__VA_ARGS__)

#define LOG_WARNING_F(format, ...) \
	PH_LOGGER_LOG(statePtr->logger, LOG_LEVEL_WARNING, "Renderer-CompatibleGL", \
		(format), ##__VA_ARGS__)

#define LOG_ERROR_F(format, ...) \
	PH_LOGGER_LOG(statePtr->logger, LOG_LEVEL_ERROR, "Renderer-CompatibleGL", \
		(format), ##__VA_ARGS__)

#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)

static void checkGLError(const char* file, int line) noexcept
{
	if (statePtr == nullptr) return;

	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		switch (error) {
		case GL_INVALID_ENUM:
			LOG_ERROR_F("%s:%i: GL_INVALID_ENUM", file, line);
			break;
		case GL_INVALID_VALUE:
			LOG_ERROR_F("%s:%i: GL_INVALID_VALUE", file, line);
			break;
		case GL_INVALID_OPERATION:
			LOG_ERROR_F("%s:%i: GL_INVALID_OPERATION", file, line);
			break;
		case GL_OUT_OF_MEMORY:
			LOG_ERROR_F("%s:%i: GL_OUT_OF_MEMORY", file, line);
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			LOG_ERROR_F("%s:%i: GL_INVALID_FRAMEBUFFER_OPERATION", file, line);
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

static void stupidSetMaterialUniform(
	const gl::Program& program,
	const char* name,
	const ph::Material& m) noexcept
{
	StackString tmpStr;
	tmpStr.printf("%s.hasAlbedoTexture", name);
	gl::setUniform(program, tmpStr.str, (m.albedoTexIndex == -1) ? 0 : 1);
	tmpStr.printf("%s.hasRoughnessTexture", name);
	gl::setUniform(program, tmpStr.str, (m.roughnessTexIndex == -1) ? 0 : 1);
	tmpStr.printf("%s.hasMetallicTexture", name);
	gl::setUniform(program, tmpStr.str, (m.metallicTexIndex == -1) ? 0 : 1);
	tmpStr.printf("%s.albedo", name);
	gl::setUniform(program, tmpStr.str, m.albedo);
	tmpStr.printf("%s.roughness", name);
	gl::setUniform(program, tmpStr.str, m.roughness);
	tmpStr.printf("%s.metallic", name);
	gl::setUniform(program, tmpStr.str, m.metallic);
}

static void renderImgui(ImDrawData* drawDataIn) noexcept
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawData& drawData = *drawDataIn;
	RendererState& state = *statePtr;

	if (!drawData.Valid) {
		return;
	}

	// Store some previous OpenGL state
	GLint lastScissorBox[4]; glGetIntegerv(GL_SCISSOR_BOX, lastScissorBox);

	// Set some OpenGL state
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	//glEnable(GL_SCISSOR_TEST);

	// Bind imgui shader and set some uniforms
	state.imguiShader.useProgram();

	gl::setUniform(state.imguiShader, "uTexture", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, state.imguiFontTexture.handle());

	mat44 projMatrix;
	projMatrix.row0 = vec4(2.0f / io.DisplaySize.x, 0.0f, 0.0f, -1.0f);
	projMatrix.row1 = vec4(0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 1.0f);
	projMatrix.row2 = vec4(0.0f, 0.0f, -1.0f, 0.0f);
	projMatrix.row3 = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	gl::setUniform(state.imguiShader, "uProjMatrix", projMatrix);

	// Bind gl command list
	state.imguiGlCmdList.bindVAO();

	struct ImguiCommand {
		uint32_t idxBufferOffset;
		uint32_t numIndices;
		vec4 clipRect;
	};
	
	DynArray<ImguiVertex> vertices;
	vertices.create(0, &state.allocator);

	static_assert(sizeof(ImDrawIdx) == sizeof(uint32_t), "Invalid type of ImDrawIdx");
	DynArray<uint32_t> indices;
	indices.create(0, &state.allocator);

	DynArray<ImguiCommand> commands;
	commands.create(0, &state.allocator);

	for (int i = 0; i < drawData.CmdListsCount; i++) {

		const ImDrawList& cmdList = *drawData.CmdLists[i];

		// indexOffset is the offset to offset all indices with
		const uint32_t indexOffset = vertices.size();

		// indexBufferOffset is the offset to where the indices start
		uint32_t indexBufferOffset = indices.size();

		// Convert vertices and add to global list
		for (int j = 0; j < cmdList.VtxBuffer.size(); j++) {
			const ImDrawVert& imguiVertex = cmdList.VtxBuffer[j];

			ImguiVertex convertedVertex;
			convertedVertex.pos = vec2(imguiVertex.pos.x, imguiVertex.pos.y);
			convertedVertex.texcoord = vec2(imguiVertex.uv.x, imguiVertex.uv.y);
			convertedVertex.color = imguiVertex.col;

			vertices.add(convertedVertex);
		}

		// Fix indices and add to global list
		for (int j = 0; j < cmdList.IdxBuffer.size(); j++) {
			indices.add(cmdList.IdxBuffer[j] + indexOffset);
		}

		// Create new commands
		for (int j = 0; j < cmdList.CmdBuffer.Size; j++) {
			const ImDrawCmd& inCmd = cmdList.CmdBuffer[j];

			ImguiCommand cmd;
			cmd.idxBufferOffset = indexBufferOffset;
			cmd.numIndices = inCmd.ElemCount;
			indexBufferOffset += inCmd.ElemCount;
			cmd.clipRect.x = inCmd.ClipRect.x;
			cmd.clipRect.y = inCmd.ClipRect.y;
			cmd.clipRect.z = inCmd.ClipRect.z;
			cmd.clipRect.w = inCmd.ClipRect.w;

			commands.add(cmd);
		}
	}

	// Upload vertices and indices to GPU
	state.imguiGlCmdList.upload(
		vertices.data(), vertices.size(), indices.data(), indices.size());

	printf("commands.size() == %u\n", commands.size());
	printf("commands[0].numIndices == %u, .indexOffset == %u\n", commands[0].numIndices, commands[0].idxBufferOffset);
	printf("commands[1].numIndices == %u, .indexOffset == %u\n", commands[1].numIndices, commands[1].idxBufferOffset);
	printf("commands[2].numIndices == %u, .indexOffset == %u\n", commands[2].numIndices, commands[2].idxBufferOffset);
	printf("commands[3].numIndices == %u, .indexOffset == %u\n", commands[3].numIndices, commands[3].idxBufferOffset);
	printf("commands[4].numIndices == %u, .indexOffset == %u\n", commands[4].numIndices, commands[4].idxBufferOffset);
	printf("commands[5].numIndices == %u, .indexOffset == %u\n", commands[5].numIndices, commands[5].idxBufferOffset);


	static int counter = 0;
	counter++;
	counter %= 100;

	if (counter < 50) {

	// Render commands
	for (const ImguiCommand& cmd : commands) {

		/*glScissor(
			cmd.clipRect.x,
			state.fbHeight - cmd.clipRect.w,
			cmd.clipRect.z - cmd.clipRect.x,
			cmd.clipRect.w - cmd.clipRect.y);*/

		state.imguiGlCmdList.render(cmd.idxBufferOffset, cmd.numIndices);
	}

	}
	else {

	for (int i = 0; i < drawData.CmdListsCount; i++) {

		// Upload command list to GPU
		const ImDrawList& cmdList = *drawData.CmdLists[i];

		vertices.clear();
		const ImVector<ImDrawVert>& vtxBuffer = cmdList.VtxBuffer;
		for (int j = 0; j < vtxBuffer.size(); j++) {
			const ImDrawVert& imguiVertex = vtxBuffer[j];
			
			ImguiVertex convertedVertex;
			convertedVertex.pos.x = imguiVertex.pos.x;
			convertedVertex.pos.y = imguiVertex.pos.y;
			convertedVertex.texcoord.x = imguiVertex.uv.x;
			convertedVertex.texcoord.y = imguiVertex.uv.y;
			convertedVertex.color = imguiVertex.col;

			vertices.add(convertedVertex);
		}

		indices.clear();
		indices.add(cmdList.IdxBuffer.Data, cmdList.IdxBuffer.Size);

		state.imguiGlCmdList.upload(
			vertices.data(), vertices.size(), indices.data(), indices.size());

		//
		//const ImVector<ImDrawIdx>& idxBuffer = cmdList.IdxBuffer;

		uint32_t idxBufferOffset = 0;
		
		// Render commands
		for (int j = 0; j < cmdList.CmdBuffer.Size; j++) {
			const ImDrawCmd& cmd = cmdList.CmdBuffer[j];

			// If a user callback is available call it instead of rendering command
			if (cmd.UserCallback != nullptr) {
				cmd.UserCallback(&cmdList, &cmd);
			}

			// Render command
			else {
				//glScissor(
				//	cmd.ClipRect.x,
				//	state.fbHeight - cmd.ClipRect.w,
				//	cmd.ClipRect.z - cmd.ClipRect.x,
				//	cmd.ClipRect.w - cmd.ClipRect.y);

				state.imguiGlCmdList.render(idxBufferOffset, cmd.ElemCount);
			}

			idxBufferOffset += cmd.ElemCount;
		}
	}

	}

	// Restore some previous OpenGL state
	glScissor(lastScissorBox[0], lastScissorBox[1], lastScissorBox[2], lastScissorBox[3]);
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
		PH_LOGGER_LOG(*logger, LOG_LEVEL_WARNING, "Renderer-CompatibleGL",
		    "Renderer already initialized, returning.");
		return 1;
	}

    PH_LOGGER_LOG(*logger, LOG_LEVEL_INFO, "Renderer-CompatibleGL", "Creating OpenGL context");
#ifdef __EMSCRIPTEN__
	// Create OpenGL Context (OpenGL ES 2.0 == WebGL 1.0)
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) < 0) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-CompatibleGL",
		    "Failed to set GL context major version: %s", SDL_GetError());
		return 0;
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) < 0) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-CompatibleGL",
		    "Failed to set GL context profile: %s", SDL_GetError());
		return 0;
	}
#else
    // Create OpenGL Context (OpenGL 3.3)
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) < 0) {
        PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-CompatibleGL",
                      "Failed to set GL context major version: %s", SDL_GetError());
        return 0;
    }
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) < 0) {
        PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-CompatibleGL",
                      "Failed to set GL context minor version: %s", SDL_GetError());
        return 0;
    }
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) < 0) {
        PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-CompatibleGL",
                      "Failed to set GL context profile: %s", SDL_GetError());
        return 0;
    }
#endif

	SDL_GLContext tmpContext = SDL_GL_CreateContext(window);
	if (tmpContext == nullptr) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-CompatibleGL",
		    "Failed to create GL context: %s", SDL_GetError());
		return 0;
	}

    // Load GLEW on not emscripten
#ifndef __EMSCRIPTEN__
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-CompatibleGL",
                      "GLEW init failure: %s", glewGetErrorString(glewError));
    }
#endif

	// Create internal state
	PH_LOGGER_LOG(*logger, LOG_LEVEL_INFO, "Renderer-CompatibleGL", "Creating internal state");
	{
		CAllocatorWrapper tmp;
		tmp.setCAllocator(cAllocator);
		statePtr = sfzNew<RendererState>(&tmp);
		if (statePtr == nullptr) {
			PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-CompatibleGL",
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
	LOG_INFO_F("\nVendor: %s\nVersion: %s\nRenderer: %s",
		glGetString(GL_VENDOR), glGetString(GL_VERSION), glGetString(GL_RENDERER));

	// Create FullscreenGeometry
	state.fullscreenGeom.create(gl::FullscreenGeometryType::OGL_CLIP_SPACE_RIGHT_HANDED_FRONT_FACE);

	// Init resources arrays
	state.textures.create(256, &state.allocator);
	state.materials.create(256, &state.allocator);
	state.dynamicModels.create(128, &state.allocator);

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
		&state.allocator);

	state.copyOutShader = gl::Program::postProcessFromSource(
		SHADER_HEADER_SRC,
		COPY_OUT_SHADER_SRC,
		&state.allocator);

	state.imguiShader = gl::Program::fromSource(
		SHADER_HEADER_SRC,
		IMGUI_VERTEX_SHADER_SRC,
		IMGUI_FRAGMENT_SHADER_SRC,
		[](uint32_t shaderProgram) {
			glBindAttribLocation(shaderProgram, 0, "inPos");
			glBindAttribLocation(shaderProgram, 1, "inTexcoord");
			glBindAttribLocation(shaderProgram, 2, "inColor");
		},
		&state.allocator);

	// Initialize array to hold dynamic sphere lights
	state.dynamicSphereLights.create(MAX_NUM_DYNAMIC_SPHERE_LIGHTS, &state.allocator);


	// Initialize imgui (TODO: Move out of renderer)
	LOG_INFO_F("Initializing imgui");
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = float(w);
	io.DisplaySize.y = float(h);
	io.RenderDrawListsFn = renderImgui;

	// Load texture atlas
	ImageView fontTexView;
	io.Fonts->GetTexDataAsAlpha8(&fontTexView.rawData, &fontTexView.width, &fontTexView.height);
	fontTexView.bytesPerPixel = 1;
	fontTexView.type = ImageType::GRAY_U8;

	// Upload texture to GL
	state.imguiFontTexture.create(fontTexView, TextureFiltering::NEAREST);

	// TODO: Store your texture pointer/identifier (whatever your engine uses) in 'io.Fonts->TexID'.
	//m This will be passed back to your via the renderer.
	//io.Fonts->TexID = (void*)texture;

	state.imguiGlCmdList.create(1000, 1000);

	CHECK_GL_ERROR();
	LOG_INFO_F("Finished initializing renderer");
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
	PH_LOGGER_LOG(logger, LOG_LEVEL_INFO, "Renderer-CompatibleGL", "Destroying state");
	{
		CAllocatorWrapper tmp;
		tmp.setCAllocator(state.allocator.cAllocator());
		sfzDelete(statePtr, &tmp);
	}
	statePtr = nullptr;

	// Destroy GL context
	PH_LOGGER_LOG(logger, LOG_LEVEL_INFO, "Renderer-CompatibleGL", "Destroying OpenGL context");
	SDL_GL_DeleteContext(context);
}

// Resource management (textures)
// ------------------------------------------------------------------------------------------------

DLL_EXPORT void phSetTextures(const phConstImageView* textures, uint32_t numTextures)
{
	RendererState& state = *statePtr;

	// Remove any previous textures
	state.textures.clear();

	// Create textures from all images and add them to state
	for (uint32_t i = 0; i < numTextures; i++) {
		state.textures.add(Texture(textures[i]));
	}
}

DLL_EXPORT uint32_t phAddTexture(const phConstImageView* texture)
{
	RendererState& state = *statePtr;

	uint32_t index = state.textures.size();
	state.textures.add(Texture(*texture));
	return index;
}

DLL_EXPORT uint32_t phUpdateTexture(const phConstImageView* texture, uint32_t index)
{
	RendererState& state = *statePtr;

	// Check if texture exists
	if (state.textures.size() <= index) return 0;

	state.textures[index] = Texture(*texture);
	return 1;
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

DLL_EXPORT void phSetDynamicMeshes(const phConstMeshView* meshes, uint32_t numMeshes)
{
	RendererState& state = *statePtr;

	// Remove any previous models
	state.dynamicModels.clear();

	// Create models from all meshes and add them to state
	for (uint32_t i = 0; i < numMeshes; i++) {
		state.dynamicModels.add(Model(meshes[i], &state.allocator));
	}
}

DLL_EXPORT uint32_t phAddDynamicMesh(const phConstMeshView* mesh)
{
	RendererState& state = *statePtr;

	uint32_t index = state.dynamicModels.size();
	state.dynamicModels.add(Model(*mesh, &state.allocator));
	return index;
}

DLL_EXPORT uint32_t phUpdateDynamicMesh(const phConstMeshView* mesh, uint32_t index)
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

	// Get size of default framebuffer and window
	SDL_GetWindowSize(state.window, &state.windowWidth, &state.windowHeight);
	SDL_GL_GetDrawableSize(state.window, &state.fbWidth, &state.fbHeight);
	state.aspect = float(state.fbWidth) / float(state.fbHeight);

	// Create camera matrices
	state.viewMatrix = viewMatrixGL(camera.pos, camera.dir, camera.up);
	state.projMatrix =
		perspectiveProjectionGL(camera.vertFovDeg, state.aspect, camera.near, camera.far);

	// Set dynamic sphere lights
	state.dynamicSphereLights.clear();
	state.dynamicSphereLights.insert(0,
		reinterpret_cast<const ph::SphereLight*>(dynamicSphereLights),
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

	// Set some Imgui stuff
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = float(state.windowWidth);
	io.DisplaySize.y = float(state.windowHeight);
	io.DisplayFramebufferScale.x = float(state.fbWidth) / float(state.windowWidth);
	io.DisplayFramebufferScale.x = float(state.fbHeight) / float(state.windowHeight);

	// Indicate to Imgui that we have started rendering a new frame
	ImGui::NewFrame();

	CHECK_GL_ERROR();
}

DLL_EXPORT void phRender(const phRenderEntity* entities, uint32_t numEntities)
{
	RendererState& state = *statePtr;

	state.modelShader.useProgram();

	const mat4& projMatrix = state.projMatrix;
	const mat4& viewMatrix = state.viewMatrix;
	mat4 modelMatrix = mat4::identity();
	mat4 normalMatrix = inverse(transpose(viewMatrix * modelMatrix));

	gl::setUniform(state.modelShader, "uProjMatrix", projMatrix);
	gl::setUniform(state.modelShader, "uViewMatrix", viewMatrix);
	gl::setUniform(state.modelShader, "uModelMatrix", modelMatrix);
	gl::setUniform(state.modelShader, "uNormalMatrix", normalMatrix);

	gl::setUniform(state.modelShader, "uAlbedoTexture", 0);
	gl::setUniform(state.modelShader, "uRoughnessTexture", 1);
	gl::setUniform(state.modelShader, "uMetallicTexture", 2);

	for (uint32_t i = 0; i < numEntities; i++) {
		const auto& entity = reinterpret_cast<const ph::RenderEntity*>(entities)[i];
		auto& model = state.dynamicModels[entity.meshIndex];

		// TODO: Set model & normal matrix here

		model.bindVAO();
		auto& modelComponents = model.components();
		for (auto& component : modelComponents) {

			// Upload component's material to shader
			uint32_t materialIndex = component.materialIndex();
			const auto& material = state.materials[materialIndex];
			stupidSetMaterialUniform(state.modelShader, "uMaterial", material);

			// Bind materials textures
			if (material.albedoTexIndex != -1) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, state.textures[material.albedoTexIndex].handle());
			}
			if (material.roughnessTexIndex != -1) {
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, state.textures[material.roughnessTexIndex].handle());
			}
			if (material.metallicTexIndex != -1) {
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, state.textures[material.metallicTexIndex].handle());
			}

			// Render component of mesh
			component.render();
		}
	}
}

DLL_EXPORT void phFinishFrame(void)
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

	ImGui::ShowTestWindow();

	ImGui::Begin("Very long time testing window");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::Button("Button");
	ImGui::End();

	// Render imgui UI
	ImGui::Render();

	SDL_GL_SwapWindow(state.window);
	CHECK_GL_ERROR();
}
