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

// This header file contains the ZeroG C-API. If you are using C++ you might also want to link with
// and use the C++ wrapper static library where appropriate. The C++ wrapper header is
// "ZeroG/ZeroG.hpp".

// Includes
// ------------------------------------------------------------------------------------------------

#pragma once

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

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

typedef uint32_t ZG_BOOL;
static const uint32_t ZG_FALSE = 0;
static const uint32_t ZG_TRUE = 1;

// Version information
// ------------------------------------------------------------------------------------------------

// The API version used to compile ZeroG.
static const uint32_t ZG_COMPILED_API_VERSION = 0;

// Returns the API version of ZeroG.
//
// As long as the DLL has the same API version as the version you compiled with it should be
// compatible.
ZG_DLL_API uint32_t zgApiVersion(void);

// Backends enums and queries
// ------------------------------------------------------------------------------------------------

// The various backends supported by ZeroG
enum ZgBackendTypeEnum {
	// The null backend, simply turn every ZeroG call into a no-op.
	ZG_BACKEND_NULL = 0,

	// The D3D12 backend, only available on Windows 10.
	ZG_BACKEND_D3D12 = 1
};
typedef uint32_t ZgBackendType;

// Queries whether the current DLL is compiled with support for the specified backend.
//
// Note that compiled support does not necessarily mean that the backend is supported on the
// current computer.
ZG_DLL_API ZG_BOOL zgBackendCompiled(ZgBackendType backendType);

// Error codes
// ------------------------------------------------------------------------------------------------

// The error codes
enum ZgErrorCodeEnum {
	ZG_SUCCESS = 0,
	ZG_ERROR_GENERIC = 1,

	ZG_ERROR_INIT_VERSION_MISMATCH,

};
typedef uint32_t ZgErrorCode;

// Memory allocator interface
// ------------------------------------------------------------------------------------------------

// Allocator interface for CPU allocations inside ZeroG.
//
// A few restrictions is placed on custom allocators:
// * They must be thread-safe. I.e. it must be okay to call it simulatenously from multiple threads.
// * All allocations must be at least 32-byte aligned.
//
// If no custom allocator is required, just leave all fields zero in this struct.
typedef struct {
	// Function pointer to allocate function. The allocation created must be 32-byte aligned. The
	// name is a short string (< ~32 chars) explaining what the allocation is used for, useful
	// for debug or visualization purposes.
	uint8_t* (*allocate)(void* userPtr, uint32_t size, const char* name);
	
	// Function pointer to free function.
	void (*free)(void* userPtr, uint8_t* allocation);
	
	// User specified pointer that is provided to each allocate/free call.
	void* userPtr;
} ZgAllocator;

// Context
// ------------------------------------------------------------------------------------------------

// Forward declare ZgContext struct
struct ZgContext;
typedef struct ZgContext ZgContext;

// The settings used to create
typedef struct {
	
	// [Mandatory] The wanted ZeroG backend
	ZgBackendType backend;

	// [Optional] The allocator used to allocate CPU memory
	ZgAllocator allocator;

} ZgContextInitSettings;

ZG_DLL_API ZgErrorCode zgCreateContext(
	ZgContext** contextOut, const ZgContextInitSettings* settings);

ZG_DLL_API ZgErrorCode zgDestroyContext(ZgContext* context);
