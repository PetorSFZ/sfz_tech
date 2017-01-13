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

#include "sfz/memory/Allocator.hpp"

#include <cinttypes>

#include "sfz/Assert.hpp"

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
		return "sfzCore default Allocator";
	}
};

// Default allocator
// ------------------------------------------------------------------------------------------------

static Allocator* standardAllocator() noexcept
{
	static StandardAllocator allocator;
	return static_cast<Allocator*>(&allocator);
}

static Allocator*& currentAllocator() noexcept
{
	static Allocator* allocator = standardAllocator();
	return allocator;
}

static uint64_t& counter() noexcept
{
	static uint64_t c = 0;
	return c;
}

Allocator* getDefaultAllocator() noexcept
{
	counter() += 1;
	return currentAllocator();
}

uint64_t getDefaultAllocatorNumTimesRetrieved() noexcept
{
	return counter();
}

void setDefaultAllocator(Allocator* allocator) noexcept
{
	if (counter() != 0) {
		sfz::error("setDefaultAllocator() failed: getDefaultAllocator() has been called " PRIu64 " times.",
		            counter());
	}
	currentAllocator() = allocator;
}

} // namespace sfz
