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

#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "sfz/Assert.hpp"
#include "sfz/Logging.hpp"
#include "sfz/CudaCompatibility.hpp"

namespace sfz {

namespace cuda {

// Check CUDA Macro
// ------------------------------------------------------------------------------------------------

#define CHECK_CUDA (sfz::cuda::CudaErrorChecker(__FILE__, __LINE__)) %

struct CudaErrorChecker
{
	const char* file;
	int line;

	CudaErrorChecker() = delete;
	CudaErrorChecker(const char* file, int line) : file(file), line(line) {}

	cudaError_t operator% (cudaError_t error)
	{
		if (error == cudaSuccess) return error;
		getLogger()->log(file, line, LogLevel::ERROR_LVL, "sfzCore", "CUDA error: %s\n",
			cudaGetErrorString(error));
		return error;
	}
};

} // namespace cuda
} // namespace phe
