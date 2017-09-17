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

#include "sfz/memory/CAllocatorWrapper.hpp"

namespace sfz {

// CAllocatorWrapper: Constructors & destructors
// ------------------------------------------------------------------------------------------------

CAllocatorWrapper::~CAllocatorWrapper() noexcept
{
	// Do nothing
}
	
void CAllocatorWrapper::setCAllocator(sfzAllocator* cAlloc) noexcept
{
	if (mCAlloc != nullptr) {
		sfz_assert_release(false);
	}
	mCAlloc = cAlloc;
}
	
// CAllocatorWrapper: Overriden Allocator methods
// ------------------------------------------------------------------------------------------------

void* CAllocatorWrapper::allocate(uint64_t size, uint64_t alignment, const char* name) noexcept
{
	return SFZ_C_ALLOCATE(mCAlloc, size, alignment, name);
}

void CAllocatorWrapper::deallocate(void* pointer) noexcept
{
	return SFZ_C_DEALLOCATE(mCAlloc, pointer);
}

const char* CAllocatorWrapper::getName() const noexcept
{
	return SFZ_C_GET_NAME(mCAlloc);
}

sfzAllocator* CAllocatorWrapper::cAllocator() noexcept
{
	return mCAlloc;
}
	
} // namespace sfz
