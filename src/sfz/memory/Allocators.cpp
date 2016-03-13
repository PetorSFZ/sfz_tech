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

#include "sfz/memory/Allocators.hpp"

#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

namespace sfz {

// Standard Allocator: Allocation functions
// ------------------------------------------------------------------------------------------------

void* StandardAllocator::allocate(size_t numBytes, size_t alignment) noexcept
{
#ifdef _WIN32
	return _aligned_malloc(numBytes, alignment);
#else
	void* ptr = nullptr;
	posix_memalign(&ptr, alignment, numBytes);
	return ptr;
#endif
}

void* StandardAllocator::reallocate(void* previous, size_t newSize, size_t alignment) noexcept
{
#ifdef _WIN32
	return _aligned_realloc(previous, newSize, alignment);
#else
#error "Not yet implemented"
#endif
}

void StandardAllocator::deallocate(void*& pointer) noexcept
{
	if (pointer == nullptr) return;
#ifdef _WIN32
	_aligned_free(pointer);
	pointer = nullptr;
#else
	free(ptr);
	pointer = nullptr;
#endif
}

} // namespace sfz