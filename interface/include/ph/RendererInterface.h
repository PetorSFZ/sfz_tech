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

#include <cstdint>

#include "ph/Bool32.h"

// Forward declarations
// ------------------------------------------------------------------------------------------------

struct SDL_Window;
struct phContext;
struct phCameraData;
struct phImageView;
struct phConstImageView;
struct phImguiVertex;
struct phImguiCommand;
struct phMaterial;
struct phConstMeshView;
struct phRenderEntity;
struct phSphereLight;
struct phStaticSceneView;

// Macros
// ------------------------------------------------------------------------------------------------

#ifdef PH_EXPORTING_RENDERER_FUNCTIONS
#ifdef _WIN32
#define PH_DLL_EXPORT __declspec(dllexport)
#else
#define PH_DLL_EXPORT
#endif
#else
#define PH_DLL_EXPORT
#endif

// Interface version
// ------------------------------------------------------------------------------------------------

const uint32_t PH_RENDERER_INTERFACE_VERSION = 18;

// Init functions
// ------------------------------------------------------------------------------------------------

/// Returns the version of the renderer interface used by the .dll. Is used to check if a .dll has
/// the expected interface. This function's signature may never be changed and must exist for all
/// future interfaces.
///
/// The implementation of this function should NOT just return PH_RENDERER_INTERFACE_VERSION. It
/// should store an internal version number which should be manually updated when compliance with
/// a new interface has been reached.
extern "C" PH_DLL_EXPORT
uint32_t phRendererInterfaceVersion(void);

/// Returns potential SDL_Window flags required by the renderer. Notably, an OpenGL renderer would
/// return SDL_WINDOW_OPENGL and a Vulkan renderer SDL_WINDOW_VULKAN. Backends that don't require
/// any special flags just return 0. This function is to be called before the renderer is
/// initialized.
extern "C" PH_DLL_EXPORT
uint32_t phRequiredSDL2WindowFlags(void);

/// Initializes this renderer. This function should by design be completely safe to call multiple
/// times in a row. If the render is already initialized it should do nothing. If the renderer has
/// previously been deinitialized it should be initialized to the same state as if it had not been
/// initialized earlier.
/// \param window the SDL_Window to render to
/// \param context the PhantasyEngine context, see "ph/Context.hpp"
/// \param allocator the sfz::Allocator used to allocate all cpu memory used
/// \return false if renderer is NOT initialized, i.e. if something went very wrong. If the renderer
///           has been previously initialized this value will still be true.
extern "C" PH_DLL_EXPORT
phBool32 phInitRenderer(
	phContext* context,
	SDL_Window* window,
	void* allocator);

/// Deinitializes this renderer. This function should by design be completely safe to call multiple
/// times in a row, including before the renderer has been initialized at all. Should also be safe
/// to call if renderer failed to initialize.
extern "C" PH_DLL_EXPORT
void phDeinitRenderer(void);

/// Initializes Imgui in this renderer. Expected to be called once after phInitRenderer().
/// \param fontTexture the font texture atlas
extern "C" PH_DLL_EXPORT
void phInitImgui(const phConstImageView* fontTexture);

// State query functions
// ------------------------------------------------------------------------------------------------

/// Returns the current dimensions of the window Imgui is being rendered to.
/// \param widthOut the width of the window
/// \param heightOut the height of the window
extern "C" PH_DLL_EXPORT
void phImguiWindowDimensions(float* widthOut, float* heightOut);

// Resource management (textures)
// ------------------------------------------------------------------------------------------------

/// Each texture is assigned an index, which can be referred to from materials. Textures are shared
/// between all objects, static and dynamic alike. The renderer is allowed to assume that textures
/// used for static objects never changes, i.e. updating those textures is not guaranteed to
/// change the appearance of the static geometry.

/// Set the texturess in this renderer. Will remove any old textures already registered. Textures
/// will be copied to the renderers internal memory. Texture at index 0 will be assigned index 0,
/// etc.
/// \param textures the textures to add
/// \param numTextures the number of textures to add
extern "C" PH_DLL_EXPORT
void phSetTextures(const phConstImageView* textures, uint32_t numTextures);

/// Adds a texture and returns its assigned index
/// \param texture the texture to add
/// \return the index assigned to the texture
extern "C" PH_DLL_EXPORT
uint16_t phAddTexture(const phConstImageView* texture);

/// Updates (replaces) a texture already registered to this renderer. Will return 0 and not do
/// anything if no texture is registered to the specified index.
/// \param texture the texture to add
/// \param index the index to the registered texture
/// \return true on success, false on failure
extern "C" PH_DLL_EXPORT
phBool32 phUpdateTexture(const phConstImageView* texture, uint16_t index);

/// Returns the number of textures registered in this renderer.
/// \return number of textures in renderer
extern "C" PH_DLL_EXPORT
uint32_t phNumTextures(void);

// Resource management (meshes)
// ------------------------------------------------------------------------------------------------

/// Sets the meshes in this renderer. Will remove any old meshes already registered. Meshes
/// are likely copied to GPU memory, but even CPU renderers are required to copy the meshes to
/// their own memory space. Mesh at index 0 in array will be assigned index 0, etc.
/// \param meshes the array of meshes
/// \param numMeshes the number of meshes in the array
extern "C" PH_DLL_EXPORT
void phSetMeshes(const phConstMeshView* meshes, uint32_t numMeshes);

/// Adds a mesh and returns its assigned index
/// \param mesh the mesh to add
/// \return the index assigned to the mesh
extern "C" PH_DLL_EXPORT
uint32_t phAddMesh(const phConstMeshView* mesh);

/// Updates (replaces) a mesh already registered to this renderer. Will return 0 and not do
/// anything if the index does not have any mesh registered to it.
/// \param mesh the mesh to add
/// \param index the index to the registered mesh
/// \return true on success, false on failure
extern "C" PH_DLL_EXPORT
phBool32 phUpdateMesh(const phConstMeshView* mesh, uint32_t index);

/// Updates a mesh's materials already registered to this renderer. Returns true on success
/// \param meshIdx the index of the mesh whichs materials should be updated
/// \param materials the new materials
/// \param numMaterials the number of materials
/// \return true on success, false on failure
extern "C" PH_DLL_EXPORT
phBool32 phUpdateMeshMaterials(uint32_t meshIdx, const phMaterial* materials, uint32_t numMaterials);

// Resource management (static scene)
// ------------------------------------------------------------------------------------------------

/// A PhantasyEngine renderer may have a single static scene set. A static scene is a
/// self-contained set of textures, meshes, materials and lights with no references to the dynamic
/// resources. The renderer may perform all sorts of optimizations and pre-processing on the
/// static scene, which may allow it to render it faster or in higher quality than the dynamic
/// objects.
///
/// A slight word of warning, all indices (i.e. material indices, texture indices) refer to the
/// arrays in the static scene, not the dynamic arrays. Therefore some care has to be taken to
/// not confuse them.

/// Sets the static scene in this renderer. Will remove any old static scenes previously set.
/// This call may take a long as the renderer may process the data in various ways. As always,
/// the renderer is required to copy any data it intends to use after the call is over.
/// \param scene the static scene to add
extern "C" PH_DLL_EXPORT
void phSetStaticScene(const phStaticSceneView* scene);

/// Removes any previous static scene set.
extern "C" PH_DLL_EXPORT
void phRemoveStaticScene(void);

// Render commands
// ------------------------------------------------------------------------------------------------

/// Called first in a frame before issuing render commands.
/// \param clearColor the vec4 containing the clear color for the screen
/// \param camera the camera to render from. Temporary pointer, make a copy.
/// \param dynamicSphereLights pointer to array of phSphereLights
/// \param numDynamicSphereLights number of elements in dynamicSphereLights array
extern "C" PH_DLL_EXPORT
void phBeginFrame(
	const float* clearColor,
	const phCameraData* camera,
	const phSphereLight* dynamicSphereLights,
	uint32_t numDynamicSphereLights);

/// Renders the static scene.
extern "C" PH_DLL_EXPORT
void phRenderStaticScene(void);

/// Renders numEntities entities. Can be called multiple times before the beginFrame() and
/// finishFrame() calls.
/// \param entities pointer to array of phRenderEntities
/// \param numEntities number of elements in entities array
extern "C" PH_DLL_EXPORT
void phRender(const phRenderEntity* entities, uint32_t numEntities);

/// Renders the imgui UI. Expected to be called once right before phFinishFrame(). Input data is
/// only required to be valid during the duration of the call.
/// \param vertices the imgui vertices to upload
/// \param numVertices the number of imgui vertices
/// \param indices the imgui nidices to upload
/// \param numIndices the number of imgui indices
/// \param commands the imgui commands to perform
/// \param numCommands the number of imgui commands
extern "C" PH_DLL_EXPORT
void phRenderImgui(
	const phImguiVertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices,
	const phImguiCommand* commands,
	uint32_t numCommands);

/// Called last in a frame to finalize rendering to screen.
extern "C" PH_DLL_EXPORT
void phFinishFrame(void);
