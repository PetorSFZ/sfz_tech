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

#include "sfz/memory/DebugAllocator.hpp"

#include <cstring>
#include <new>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include "sfz/Assert.hpp"

namespace sfz {

// DebugAllocatorImpl class
// ------------------------------------------------------------------------------------------------

struct DebugAllocatorImpl final {
	char allocatorName[128];
	uint32_t alignmentIntegrityFactor;
	std::unordered_map<void*, DebugAllocationInfo> allocations;
	std::unordered_map<void*, DebugAllocationInfo> history;
};

// DebugAllocator: Constructors & destructors
// ------------------------------------------------------------------------------------------------

DebugAllocator::DebugAllocator(const char* name, uint32_t alignmentIntegrityFactor) noexcept
{
	mImpl = new (std::nothrow) DebugAllocatorImpl();
	std::strncpy(mImpl->allocatorName, name, sizeof(mImpl->allocatorName));
	mImpl->alignmentIntegrityFactor = alignmentIntegrityFactor * 2;
}

DebugAllocator::~DebugAllocator() noexcept
{
	delete mImpl;
}

// DebugAllocator: Overriden Allocator methods
// ------------------------------------------------------------------------------------------------

void* DebugAllocator::allocate(uint64_t size, uint64_t alignment, const char* name) noexcept
{
	// Allocate memory
#ifdef _WIN32
	void* ptr = _aligned_malloc(size + mImpl->alignmentIntegrityFactor * alignment, alignment);
#else
	void* ptr = nullptr;
	posix_memalign(&ptr, alignment, size + mImpl->alignmentIntegrityFactor * alignment);
#endif

	// Write integrity bytes before and after allocation
	const char INTEGRITY_BYTES[] = "BOUNDS";
	uint32_t halfIntegrityFactor = mImpl->alignmentIntegrityFactor / 2;
	uint8_t* bytePtr = (uint8_t*)ptr;
	for (uint32_t i = 0; i < (halfIntegrityFactor * alignment); i++) {
		bytePtr[i] = INTEGRITY_BYTES[(i % 6)];
	}
	for (uint32_t i = 0; i < (halfIntegrityFactor * alignment); i++) {
		bytePtr[i + size + (halfIntegrityFactor * alignment)] = INTEGRITY_BYTES[(i % 6)];
	}

	// Calculate "visible" pointer
	void* visiblePtr = (void*)(bytePtr + (halfIntegrityFactor * alignment));

	// Record info about allocation
	DebugAllocationInfo tmpInfo;
	std::strncpy(tmpInfo.name, name, sizeof(tmpInfo.name));
	tmpInfo.pointer = visiblePtr;
	tmpInfo.size = size;
	tmpInfo.alignment = alignment;
	mImpl->allocations[visiblePtr] = tmpInfo;

	return visiblePtr;
}

void DebugAllocator::deallocate(void* pointer) noexcept
{
	if (pointer == nullptr) return;

	// Check if pointer is deallocatable by this allocator
	auto currItr = mImpl->allocations.find(pointer);
	if (currItr == mImpl->allocations.end()) {

		// Check if pointer has previously been allocated with this allocator
		auto histItr = mImpl->history.find(pointer);
		if (histItr == mImpl->history.end()) {
			sfz::error("Pointer %p not allocated by %s.", pointer, mImpl->allocatorName);
		} else {
			sfz::error("Allocation %s, pointer = %p has already been deallocated by %s.",
			           histItr->second.name, pointer, mImpl->allocatorName);
		}
	}

	// Retrieve allocation info, remove it from current list and add to history
	DebugAllocationInfo info = currItr->second;
	mImpl->allocations.erase(pointer);
	mImpl->history[pointer] = info;

	// Calculate actual pointers
	uint32_t halfIntegrityFactor = mImpl->alignmentIntegrityFactor / 2;
	uint8_t* bytePtr = (uint8_t*)pointer;
	bytePtr = bytePtr - (halfIntegrityFactor * info.alignment);
	void* actualPtr = (void*)bytePtr;

	// Check integrity
	const char INTEGRITY_BYTES[] = "BOUNDS";
	for (uint32_t i = 0; i < (halfIntegrityFactor * info.alignment); i++) {
		if (bytePtr[i] != (INTEGRITY_BYTES[(i % 6)])) {
			sfz::error("Allocator %s: Allocation %s has written out of bounds %u bytes before start of memory.",
			           mImpl->allocatorName, info.name, (halfIntegrityFactor * info.alignment) - i);
		}
	}
	for (uint32_t i = 0; i < (halfIntegrityFactor * info.alignment); i++) {
		if (bytePtr[i + info.size + (halfIntegrityFactor * info.alignment)] != (INTEGRITY_BYTES[(i % 6)])) {
			sfz::error("Allocator %s: Allocation %s has written out of bounds %u bytes after end of memory.",
			           mImpl->allocatorName, info.name, i);
		}
	}

	// Free memory
#ifdef _WIN32
	_aligned_free(actualPtr);
#else
	free(actualPtr);
#endif
}

const char* DebugAllocator::getName() const noexcept
{
	return mImpl->allocatorName;
}

// DebugAllocator: Methods
// ------------------------------------------------------------------------------------------------

uint32_t DebugAllocator::numAllocations() const noexcept
{
	return uint32_t(mImpl->allocations.size());
}

uint32_t DebugAllocator::numDeallocated() const noexcept
{
	return uint32_t(mImpl->history.size());
}

uint32_t DebugAllocator::numTotalAllocations() const noexcept
{
	return numAllocations() + numDeallocated();
}

DebugAllocationInfo* DebugAllocator::allocations(uint32_t* numAllocations) noexcept
{
	*numAllocations = this->numAllocations();
	DebugAllocationInfo* ptr = (DebugAllocationInfo*)this->allocate(
	     sizeof(DebugAllocationInfo) * (*numAllocations), 32, "DebugAllocator::allocations()");
	uint32_t i = 0;
	for (auto& pair : mImpl->allocations) {
		ptr[i] = pair.second;
		i++;
	}
	return ptr;
}

DebugAllocationInfo* DebugAllocator::deallocatedAllocations(uint32_t* numAllocations) noexcept
{
	*numAllocations = this->numDeallocated();
	DebugAllocationInfo* ptr = (DebugAllocationInfo*)this->allocate(
	     sizeof(DebugAllocationInfo) * (*numAllocations), 32, "DebugAllocator::deallocatedAllocations()");
	uint32_t i = 0;
	for (auto& pair : mImpl->history) {
		ptr[i] = pair.second;
		i++;
	}
	return ptr;
}

} // namespace sfz
