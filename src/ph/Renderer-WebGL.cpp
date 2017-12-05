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

#include <sfz/gl/IncludeOpenGL.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/math/ProjectionMatrices.hpp>
#include <sfz/memory/CAllocatorWrapper.hpp>
#include <sfz/memory/New.hpp>
#include <sfz/strings/StackString.hpp>

#include <sfz/gl/Program.hpp>
#include <sfz/gl/FullscreenGeometry.hpp>
#include <sfz/gl/UniformSetters.hpp>

#include "ph/Model.hpp"
#include "ph/Texture.hpp"

using namespace sfz;
using namespace ph;

// Constants
// ------------------------------------------------------------------------------------------------

static const uint32_t MAX_NUM_DYNAMIC_SPHERE_LIGHTS = 32;

// Shaders
// ------------------------------------------------------------------------------------------------

const char* SHADER_HEADER_SRC =

#ifdef __EMSCRIPTEN__
R"(
precision mediump float;
#define PH_WEB_GL 1
#define PH_VERTEX_IN attribute
#define PH_VERTEX_OUT varying
#define PH_FRAGMENT_IN varying
#define PH_TEXREAD(sampler, coord) texture2D(sampler, coord)
)"
#else
R"(
#version 330
precision highp float;
#define PH_DESKTOP_GL 1
#define PH_VERTEX_IN in
#define PH_VERTEX_OUT out
#define PH_FRAGMENT_IN in
#define PH_TEXREAD(sampler, coord) texture(sampler, coord)
)"
#endif

R"(
// Structs
// ------------------------------------------------------------------------------------------------

// Material struct
struct Material {
	int hasAlbedoTexture;
	int hasRoughnessTexture;
	int hasMetallicTexture;
	vec4 albedo;
	float roughness;
	float metallic;
};

// SphereLight struct
struct SphereLight {
	vec3 vsPos;
	float radius;
	float range;
	vec3 strength;
};

)";

const char* VERTEX_SHADER_SRC = R"(

// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_VERTEX_IN vec3 inPos;
PH_VERTEX_IN vec3 inNormal;
PH_VERTEX_IN vec2 inTexcoord;

// Output
PH_VERTEX_OUT vec3 vsPos;
PH_VERTEX_OUT vec3 vsNormal;
PH_VERTEX_OUT vec2 texcoord;

// Uniforms
uniform mat4 uProjMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uModelMatrix;
uniform mat4 uNormalMatrix; // inverse(transpose(modelViewMatrix)) for non-uniform scaling

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	vec4 vsPosTmp = uViewMatrix * uModelMatrix * vec4(inPos, 1.0);

	vsPos = vsPosTmp.xyz / vsPosTmp.w; // Unsure if division necessary.
	vsNormal = (uNormalMatrix * vec4(inNormal, 0.0)).xyz;
	texcoord = inTexcoord;

	gl_Position = uProjMatrix * vsPosTmp;
}

)";

const char* FRAGMENT_SHADER_SRC = R"(

// Input, output and uniforms
// ------------------------------------------------------------------------------------------------

// Input
PH_FRAGMENT_IN vec3 vsPos;
PH_FRAGMENT_IN vec3 vsNormal;
PH_FRAGMENT_IN vec2 texcoord;

// Output
#ifdef PH_DESKTOP_GL
out vec4 fragOut;
#endif

// Uniforms (material)
uniform Material uMaterial;
uniform sampler2D uAlbedoTexture;
uniform sampler2D uRoughnessTexture;
uniform sampler2D uMetallicTexture;

// Uniforms (dynamic spherelights)
const int MAX_NUM_DYNAMIC_SPHERE_LIGHTS = 32;
uniform SphereLight uDynamicSphereLights[MAX_NUM_DYNAMIC_SPHERE_LIGHTS];
uniform int uNumDynamicSphereLights;

// Gamma Correction Functions
// ------------------------------------------------------------------------------------------------

const vec3 gamma = vec3(2.2);

vec3 linearize(vec3 rgbGamma)
{
	return pow(rgbGamma, gamma);
}

vec4 linearize(vec4 rgbaGamma)
{
	return vec4(linearize(rgbaGamma.rgb), rgbaGamma.a);
}

vec3 applyGammaCorrection(vec3 linearValue)
{
	return pow(linearValue, vec3(1.0 / gamma));
}

vec4 applyGammaCorrection(vec4 linearValue)
{
	return vec4(applyGammaCorrection(linearValue.rgb), linearValue.a);
}

// PBR shading functions
// ------------------------------------------------------------------------------------------------

const float PI = 3.14159265359;

// References used:
// https://de45xmedrsdbp.cloudfront.net/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// http://blog.selfshadow.com/publications/s2016-shading-course/
// http://www.codinglabs.net/article_physically_based_rendering_cook_torrance.aspx
// http://graphicrants.blogspot.se/2013/08/specular-brdf-reference.html

// Normal distribution function, GGX/Trowbridge-Reitz
// a = roughness^2, UE4 parameterization
// dot(n,h) term should be clamped to 0 if negative
float ggx(float nDotH, float a)
{
	float a2 = a * a;
	float div = PI * pow(nDotH * nDotH * (a2 - 1.0) + 1.0, 2.0);
	return a2 / div;
}

// Schlick's model adjusted to fit Smith's method
// k = a/2, where a = roughness^2, however, for analytical light sources (non image based)
// roughness is first remapped to roughness = (roughnessOrg + 1) / 2.
// Essentially, for analytical light sources:
// k = (roughness + 1)^2 / 8
// For image based lighting:
// k = roughness^2 / 2
float geometricSchlick(float nDotL, float nDotV, float k)
{
	float g1 = nDotL / (nDotL * (1.0 - k) + k);
	float g2 = nDotV / (nDotV * (1.0 - k) + k);
	return g1 * g2;
}

// Schlick's approximation. F0 should typically be 0.04 for dielectrics
vec3 fresnelSchlick(float nDotL, vec3 f0)
{
	return f0 + (vec3(1.0) - f0) * clamp(pow(1.0 - nDotL, 5.0), 0.0, 1.0);
}

// Main
// ------------------------------------------------------------------------------------------------

void main()
{
	// Albedo (Gamma space)
	vec3 albedo = uMaterial.albedo.rgb;
	float alpha = uMaterial.albedo.a;
	if (uMaterial.hasAlbedoTexture != 0) {
		vec4 tmp = PH_TEXREAD(uAlbedoTexture, texcoord);
		albedo = tmp.rgb;
		alpha = tmp.a;
	}
	albedo = linearize(albedo);

	// Skip fragment if it is transparent
	if (alpha < 0.1) discard;

	// Roughness (Linear space)
	float roughness = uMaterial.roughness;
	if (uMaterial.hasRoughnessTexture != 0) {
		roughness = PH_TEXREAD(uRoughnessTexture, texcoord).r;
	}

	// Metallic (Liner space)
	float metallic = uMaterial.metallic;
	if (uMaterial.hasMetallicTexture != 0) {
		metallic = PH_TEXREAD(uMetallicTexture, texcoord).r;
	}

	// Fragment's position and normal
	vec3 p = vsPos;
	vec3 n = normalize(vsNormal);

	vec3 v = normalize(-p); // to view
	float nDotV = dot(n, v);

	// Interpolation of normals sometimes makes them face away from the camera. Clamp
	// these to almost zero, to not break shading calculations.
	nDotV = max(0.001, nDotV);

	vec3 totalOutput = vec3(0.0);

	for (int i = 0; i < MAX_NUM_DYNAMIC_SPHERE_LIGHTS; i++) {

		// Skip if we are out of light sources
		if (i >= uNumDynamicSphereLights) break;

		// Retrieve light source
		SphereLight light = uDynamicSphereLights[i];

		// Shading parameters
		vec3 toLight = light.vsPos - p;
		float toLightDist = length(toLight);
		vec3 l = toLight * (1.0 / toLightDist); // to light
		vec3 h = normalize(l + v); // half vector (normal of microfacet)

		// If nDotL is <= 0 then the light source is not in the hemisphere of the surface, i.e.
		// no shading needs to be performed
		float nDotL = dot(n, l);
		if (nDotL <= 0.0) continue;

		// Lambert diffuse
		vec3 diffuse = albedo / PI;

		// Cook-Torrance specular
		// Normal distribution function
		float nDotH = max(dot(n, h), 0.0); // max() should be superfluous here
		float ctD = ggx(nDotH, roughness * roughness);

		// Geometric self-shadowing term
		float k = pow(roughness + 1.0, 2.0) / 8.0;
		float ctG = geometricSchlick(nDotL, nDotV, k);

		// Fresnel function
		// Assume all dielectrics have a f0 of 0.04, for metals we assume f0 == albedo
		vec3 f0 = mix(vec3(0.04), albedo, metallic);
		vec3 ctF = fresnelSchlick(nDotV, f0);

		// Calculate final Cook-Torrance specular value
		vec3 specular = ctD * ctF * ctG / (4.0 * nDotL * nDotV);

		// Calculates light strength
		float shadow = 1.0; // TODO: Shadow map
		float fallofNumerator = pow(clamp(1.0 - pow(toLightDist / light.range, 4.0), 0.0, 1.0), 2.0);
		float fallofDenominator = (toLightDist * toLightDist + 1.0);
		float falloff = fallofNumerator / fallofDenominator;
		vec3 lightContrib = falloff * light.strength * shadow;

		vec3 ks = ctF;
		vec3 kd = (1.0 - ks) * (1.0 - metallic);

		// "Solves" reflectance equation under the assumption that the light source is a point light
		// and that there is no global illumination.
		totalOutput += (kd * diffuse + specular) * lightContrib * nDotL;
	}

	vec4 outTmp = vec4(applyGammaCorrection(totalOutput), 1.0);
#ifdef PH_WEB_GL
	gl_FragColor = outTmp;
#else
	fragOut = outTmp;
#endif
}

)";

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

	// Shaders
	uint32_t fbWidth, fbHeight;
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

    PH_LOGGER_LOG(*logger, LOG_LEVEL_INFO, "Renderer-WebGL", "Creating OpenGL context");
#ifdef __EMSCRIPTEN__
	// Create OpenGL Context (OpenGL ES 2.0 == WebGL 1.0)
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
#else
    // Create OpenGL Context (OpenGL 3.3)
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) < 0) {
        PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-ModernGL",
                      "Failed to set GL context major version: %s", SDL_GetError());
        return 0;
    }
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) < 0) {
        PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-ModernGL",
                      "Failed to set GL context minor version: %s", SDL_GetError());
        return 0;
    }
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) < 0) {
        PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-ModernGL",
                      "Failed to set GL context profile: %s", SDL_GetError());
        return 0;
    }
#endif

	SDL_GLContext tmpContext = SDL_GL_CreateContext(window);
	if (tmpContext == nullptr) {
		PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
		    "Failed to create GL context: %s", SDL_GetError());
		return 0;
	}

    // Load GLEW on not emscripten
#ifndef __EMSCRIPTEN__
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        PH_LOGGER_LOG(*logger, LOG_LEVEL_ERROR, "Renderer-WebGL",
                      "GLEW init failure: %s", glewGetErrorString(glewError));
    }
#endif

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
	state.textures.create(256, &state.allocator);
	state.materials.create(256, &state.allocator);
	state.dynamicModels.create(128, &state.allocator);

	// Compile shader program
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


	SDL_GL_SwapWindow(state.window);
	CHECK_GL_ERROR();
}

