// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

// About
// ------------------------------------------------------------------------------------------------

// This header file contains the ZeroG C-API. If you are using C++ you should not include this
// directory directly, instead you should link with the CppWrapper static library and include
// "ZeroG/ZeroG.hpp" instead.

// Includes
// ------------------------------------------------------------------------------------------------

#pragma once

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

// TODO: Remove, ZeroG should not depend on SDL2
#include <SDL.h>

// Macros
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus
#define ZG_EXTERN_C extern "C"
#else
#define ZG_EXTERN_C
#endif

#if defined(_WIN32)
#if defined(ZG_DLL_EXPORT)
#define ZG_DLL_API ZG_EXTERN_C __declspec(dllexport)
#else
#define ZG_DLL_API ZG_EXTERN_C __declspec(dllimport)
#endif
#else
#define ZG_DLL_API ZG_EXTERN_C
#endif

// Version information
// ------------------------------------------------------------------------------------------------

// The API version used to compile ZeroG.
static const uint32_t ZG_COMPILED_API_VERSION = 0;

// Returns the API version of ZeroG.
//
// As long as the DLL has the same API version as the version you compiled with it should be
// compatible.
ZG_DLL_API uint32_t zgApiVersion(void);

// Context creation
// ------------------------------------------------------------------------------------------------
