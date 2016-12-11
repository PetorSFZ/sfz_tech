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

#pragma once

// CUDA call macro
// ------------------------------------------------------------------------------------------------

/// A macro to allow for functions to be called from CUDA.
#if defined(__CUDACC__)
#define SFZ_CUDA_CALL inline __host__ __device__
#else
#define SFZ_CUDA_CALL inline
#endif

// CUDA device macro
// ------------------------------------------------------------------------------------------------

/// A macro that is defined if the code is currently being compiled for CUDA devices
#if defined(__CUDACC__)
#if defined(__CUDA_ARCH__)
#define SFZ_CUDA_DEVICE_CODE 1
#endif
#endif
