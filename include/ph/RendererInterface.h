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
#include "ph/rendering/RenderEntity.h"
#include "ph/rendering/SphereLight.h"
#include "ConfigInterface.h"
#include "LoggingInterface.h"

// C interface
#ifdef __cplusplus
extern "C" {
#endif

// Macros
// ------------------------------------------------------------------------------------------------

#ifdef SFZ_EMSCRIPTEN
#define DLL_EXPORT
#else
#define DLL_EXPORT __declspec(dllexport)
#endif

// Init functions
// ------------------------------------------------------------------------------------------------

/// Returns the version of the renderer interface used by the .dll. Is used to check if a .dll has
/// the expected interface. This function's signature may never be changed and must exist for all
/// future interfaces.
DLL_EXPORT uint32_t phRendererInterfaceVersion(void);

/// Initializes this renderer. This function should by design be completely safe to call multiple
/// times in a row. If the render is already initialized it should do nothing. If the renderer has
/// previously been deinitialized it should be initialized to the same state as if it had not been
/// initialized earlier.
/// \param allocator the sfz::Allocator used to allocate all cpu memory used
/// \param config the phConfig used for configuring the renderer. Temporary pointer, make a copy.
/// \param logger the phLogger used to print debug information. Temporary pointer, make a copy.
/// \return 0 if renderer is NOT initialized, i.e. if something went very wrong. If the renderer
///           has been previously initialized this value will still be a non-zero value.
DLL_EXPORT uint32_t phInitRenderer(sfzAllocator* allocator, phConfig* config, phLogger* logger);

/// Deinitializes this renderer. This function should by design be completely safe to call multiple
/// times in a row, including before the renderer has been initialized at all. Should also be safe
/// to call if renderer failed to initialize.
DLL_EXPORT void phDeinitRenderer();

// Render commands
// ------------------------------------------------------------------------------------------------

/// Called first in a frame before issuing render commands.
DLL_EXPORT void beginFrame(
	const phCameraData* camera,
	const phSphereLight* dynamicSphereLights,
	uint32_t numDynamicSphereLights);

/// Renders numEntities entities. Can be called multiple times before the beginFrame() and
/// finishFrame() calls.
DLL_EXPORT void render(const phRenderEntity* entities, uint32_t numEntities);

/// Called last in a frame to finalize rendering to screen.
DLL_EXPORT void finishFrame();


// End C interface
#ifdef __cplusplus
}
#endif
