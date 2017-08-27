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

// C interface
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Macros
// ------------------------------------------------------------------------------------------------

#ifdef SFZ_EMSCRIPTEN
#define DLL_EXPORT
#else
#define DLL_EXPORT __declspec(dllexport)
#endif

// Interface
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
/// \return 0 if renderer is NOT initialized, i.e. if something went very wrong. If the renderer
///           has been previously initialized this value will still be a non-zero value.
DLL_EXPORT uint32_t phInitRenderer(void* allocator);

/// Deinitializes this renderer. This function should by design be completely safe to call multiple
/// times in a row, including before the renderer has been initialized at all. Should also be safe
/// to call if renderer failed to initialize.
DLL_EXPORT void phDeinitRenderer();


// End C interface
#ifdef __cplusplus
}
#endif
