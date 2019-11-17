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

#include "ZeroG/util/CpuAllocation.hpp"

#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

namespace zg {

// Default allocator
// ------------------------------------------------------------------------------------------------

static void* defaultAllocate(void* userPtr, uint32_t size, const char* name)
{
	(void)userPtr;
	(void)name;
#ifdef _WIN32
	return reinterpret_cast<void*>(_aligned_malloc(size, 32));
#else
	void* ptr = nullptr;
	posix_memalign(&ptr, 32, size);
	return reinterpret_cast<void*>(ptr);
#endif
}

static void defaultDeallocate(void* userPtr, void* allocation)
{
	(void)userPtr;
	if (allocation == nullptr) return;
#ifdef _WIN32
	_aligned_free(allocation);
#else
	free(allocation);
#endif
}

ZgAllocator getDefaultAllocator() noexcept
{
	ZgAllocator allocator = {};
	allocator.allocate = defaultAllocate;
	allocator.deallocate = defaultDeallocate;
	return allocator;
}

} // namespace zg
