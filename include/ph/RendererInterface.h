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

#ifdef __cplusplus
#include <cstdint>
using std::uint32_t;
#else
#include <stdint.h>
#endif

#include <sfz/memory/CAllocator.h>

#include "ph/rendering/CameraData.h"
#include "ph/rendering/Mesh.h"
#include "ph/rendering/RenderEntity.h"
#include "ph/rendering/SphereLight.h"
#include "ph/rendering/Vertex.h"
#include "ConfigInterface.h"
#include "LoggingInterface.h"

// C interface
#ifdef __cplusplus
extern "C" {
#endif

// Forward declare SDL_Window
struct SDL_Window;

// Macros
// ------------------------------------------------------------------------------------------------

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

// Init functions
// ------------------------------------------------------------------------------------------------

/// Returns the version of the renderer interface used by the .dll. Is used to check if a .dll has
/// the expected interface. This function's signature may never be changed and must exist for all
/// future interfaces.
DLL_EXPORT uint32_t phRendererInterfaceVersion(void);

/// Returns potential SDL_Window flags required by the renderer. Notably, an OpenGL renderer would
/// return SDL_WINDOW_OPENGL and a Vulkan renderer SDL_WINDOW_VULKAN. Backends that don't require
/// any special flags just return 0. This function is to be called before the renderer is
/// initialized.
DLL_EXPORT uint32_t phRequiredSDL2WindowFlags(void);

/// Initializes this renderer. This function should by design be completely safe to call multiple
/// times in a row. If the render is already initialized it should do nothing. If the renderer has
/// previously been deinitialized it should be initialized to the same state as if it had not been
/// initialized earlier.
/// \param window the SDL_Window to render to
/// \param allocator the sfz::Allocator used to allocate all cpu memory used
/// \param config the phConfig used for configuring the renderer. Temporary pointer, make a copy.
/// \param logger the phLogger used to print debug information. Temporary pointer, make a copy.
/// \return 0 if renderer is NOT initialized, i.e. if something went very wrong. If the renderer
///           has been previously initialized this value will still be a non-zero value.
DLL_EXPORT uint32_t phInitRenderer(
	SDL_Window* window,
	sfzAllocator* allocator,
	phConfig* config,
	phLogger* logger);

/// Deinitializes this renderer. This function should by design be completely safe to call multiple
/// times in a row, including before the renderer has been initialized at all. Should also be safe
/// to call if renderer failed to initialize.
DLL_EXPORT void phDeinitRenderer();

// Resource management (meshes)
// ------------------------------------------------------------------------------------------------

/// A PhantasyEngine renderer sorts geometry into two types, static and dynamic. Dynamic geometry
/// can be rendered in any way desired each frame, while static geometry may not change in any way
/// until it is replaced.

/// Sets the dynamic meshes in this renderer. Will remove any old meshes already registered. Meshes
/// are likely copied to GPU memory, but even CPU renderers are required to copy the meshes to
/// their own memory space.
/// \param meshes the array of meshes
/// \param numMeshes the number of meshes in the array
DLL_EXPORT void phSetDynamicMeshes(const phMesh* meshes, uint32_t numMeshes);

/// Adds a dynamic mesh and returns its assigned index
/// \param mesh the mesh to add
/// \return the index assigned to the mesh
DLL_EXPORT uint32_t phAddDynamicMesh(const phMesh* mesh);

/// Updates (replaces) a mesh already registered to this renderer. Will return 0 and not do
/// anything if the index does not have any mesh registered to it.
/// \param mesh the mesh to add
/// \param index the index to the registered mesh
DLL_EXPORT uint32_t phUpdateDynamicMesh(const phMesh* mesh, uint32_t index);

// Render commands
// ------------------------------------------------------------------------------------------------

/// Called first in a frame before issuing render commands.
/// \param camera the camera to render from. Temporary pointer, make a copy.
/// \param dynamicSphereLights pointer to array of phSphereLights
/// \param numDynamicSphereLights number of elements in dynamicSphereLights array
DLL_EXPORT void phBeginFrame(
	const phCameraData* camera,
	const phSphereLight* dynamicSphereLights,
	uint32_t numDynamicSphereLights);

/// Renders numEntities entities. Can be called multiple times before the beginFrame() and
/// finishFrame() calls.
/// \param entities pointer to array of phRenderEntities
/// \param numEntities number of elements in entities array
DLL_EXPORT void phRender(const phRenderEntity* entities, uint32_t numEntities);

/// Called last in a frame to finalize rendering to screen.
DLL_EXPORT void phFinishFrame(void);


// End C interface
#ifdef __cplusplus
}
#endif
