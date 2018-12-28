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

// This entire header is pure C
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Macros
// ------------------------------------------------------------------------------------------------

#if defined(_WIN32)
#if defined(ZG_DLL_EXPORT)
#define ZG_DLL_API __declspec(dllexport)
#else
#define ZG_DLL_API __declspec(dllimport)
#endif
#else
#define ZG_DLL_API
#endif

// Bool
// ------------------------------------------------------------------------------------------------

typedef uint32_t ZgBool;
static const ZgBool ZG_FALSE = 0;
static const ZgBool ZG_TRUE = 1;

// Version information
// ------------------------------------------------------------------------------------------------

// The API version used to compile ZeroG.
static const uint32_t ZG_COMPILED_API_VERSION = 0;

// Returns the API version of ZeroG.
//
// As long as the DLL has the same API version as the version you compiled with it should be
// compatible.
ZG_DLL_API uint32_t zgApiVersion(void);

// Backends enums
// ------------------------------------------------------------------------------------------------

// The various backends supported by ZeroG
enum ZgBackendTypeEnum {
	// The null backend, simply turn every ZeroG call into a no-op.
	ZG_BACKEND_NONE = 0,

	// The D3D12 backend, only available on Windows 10.
	ZG_BACKEND_D3D12,

	//ZG_BACKEND_VULKAN,
	//ZG_BACKEND_METAL,
	//ZG_BACKEND_WEB_GPU,
	//ZG_BACKEND_D3D11
};
typedef uint32_t ZgBackendType;

// Compiled features
// ------------------------------------------------------------------------------------------------

// Feature bits representing different features that can be compiled into ZeroG.
//
// If you depend on a specific feature (such as the D3D12 backend) it is a good idea to query and
// check if it is available 
enum ZgFeatureBitsEnum {
	ZG_FEATURE_BIT_NONE = 0,
	ZG_FEATURE_BIT_BACKEND_D3D12 = 1 << 1,
	//ZG_FEATURE_BIT_BACKEND_VULKAN = 1 << 2
	//ZG_FEATURE_BIT_BACKEND_METAL = 1 << 3
	//ZG_FEATURE_BIT_BACKEND_WEB_GPU = 1 << 4
	//ZG_FEATURE_BIT_BACKEND_D3D11 = 1 << 5
};
typedef uint64_t ZgFeatureBits;

// Returns a bitmask containing the features compiled into this ZeroG dll.
ZG_DLL_API ZgFeatureBits zgCompiledFeatures(void);

// Error codes
// ------------------------------------------------------------------------------------------------

// The error codes
enum ZgErrorCodeEnum {
	ZG_SUCCESS = 0,
	ZG_ERROR_GENERIC,
	ZG_ERROR_UNIMPLEMENTED,
	ZG_ERROR_CPU_OUT_OF_MEMORY,
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
	
	// Function pointer to deallocate function.
	void (*deallocate)(void* userPtr, uint8_t* allocation);
	
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
	ZgContext** contextOut, const ZgContextInitSettings* initSettings);

ZG_DLL_API ZgErrorCode zgDestroyContext(ZgContext* context);


// Everything in this file is pure C
#ifdef __cplusplus
} // extern "C"
#endif
