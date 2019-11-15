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

#include "sfz/memory/StandardAllocator.hpp"

#include <cinttypes>

#include "sfz/Assert.hpp"
#include "sfz/memory/MemoryUtils.hpp"

#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

namespace sfz {

// Standard allocator implementation
// ------------------------------------------------------------------------------------------------

class StandardAllocator final : public Allocator {
public:
	void* allocate(uint64_t size, uint64_t alignment, const char*) noexcept override final
	{
		sfz_assert(isPowerOfTwo(alignment));

#ifdef _WIN32
		return _aligned_malloc(size, alignment);
#else
		void* ptr = nullptr;
		posix_memalign(&ptr, alignment, size);
		return ptr;
#endif
	}

	void deallocate(void* pointer) noexcept override final
	{
		if (pointer == nullptr) return;
#ifdef _WIN32
		_aligned_free(pointer);
#else
		free(pointer);
#endif
	}

	const char* getName() const noexcept override final
	{
		return "sfzCore StandardAllocator";
	}
};

// StandardAllocator retrieval function
// ------------------------------------------------------------------------------------------------

Allocator* getStandardAllocator() noexcept
{
	static StandardAllocator allocator;
	return &allocator;
}

} // namespace sfz
