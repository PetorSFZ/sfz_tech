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

#include <cuda_runtime.h>

#include "sfz/Assert.hpp"
#include "sfz/Logging.hpp"
#include "sfz/CudaCompatibility.hpp"

// Macros
// ------------------------------------------------------------------------------------------------

/// Checks the error code of a CUDA API call, prints an error message using sfz::printErrorMessage()
/// if not cudaSuccess.
#define CHECK_CUDA_ERROR(error) (sfz::cuda::checkCudaError(__FILE__, __LINE__, error))

namespace sfz {

namespace cuda {

// Error checking
// ------------------------------------------------------------------------------------------------

inline cudaError_t checkCudaError(const char* file, int line, cudaError_t error) noexcept
{
	if (error == cudaSuccess) return error;
	getLogger()->log(file, line, LogLevel::ERROR, "sfzCore", "CUDA error: %s\n",
		cudaGetErrorString(error));
	return error;
}

} // namespace cuda
} // namespace phe
