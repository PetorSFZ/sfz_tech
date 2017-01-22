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

#include "sfz/memory/Allocator.hpp"

namespace sfz {

// DebugAllocationInfo
// ------------------------------------------------------------------------------------------------

/// Struct containing information about an allocation made by a DebugAllocator instance.
/// The information in the struct is what the user expects, not what the allocator actually did.
/// For example, the allocator will allocate more memory than what the user specifies in order to
/// check for out-of-bounds writes.
struct DebugAllocationInfo {
	char name[32];
	void* pointer;
	uint64_t size; // Size of allocation in bytes
	uint64_t alignment; // Alignment of allocation in bytes
};

// DebugAllocator class
// ------------------------------------------------------------------------------------------------

struct DebugAllocatorImpl; // Pimpl pattern

/// Debug allocator which can be used to debug code using sfzCore Allocators.
///
/// Features:
/// * Keeps track of all allocations made
/// * Checks if an allocation is made with this allocator at deallocation
/// * Checks if an allocation is already deallocated at deallocation
/// * Checks if memory has been written out of bounds before and after allocation
/// * Can be used to check if all memory has been properly deallocated by calling numAllocations()
///   and compare result with expected value (likely 0).
///
/// All internal memory allocations is made with the standard C++ operator new and delete. A
/// DebugAllocator should only be used when debugging, not in release code.
class DebugAllocator final : public Allocator {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	DebugAllocator() = delete;
	DebugAllocator(const DebugAllocator&) = delete;
	DebugAllocator& operator= (const DebugAllocator&) = delete;
	DebugAllocator(DebugAllocator&&) = delete;
	DebugAllocator& operator= (DebugAllocator&&) = delete;

	/// Creates a DebugAllocator
	/// name is the name of the allocator instance
	/// alignmentIntegrityFactor specifies how many alignments of bytes should be padded onto
	/// start and end of each allocation. Specific values is written to this padding during
	/// allocation, and at deallocation it is checked that it has not been overwritten. In other
	/// words, a larger values means that more memory is checked for corruption.
	DebugAllocator(const char* name, uint32_t alignmentIntegrityFactor = 4) noexcept;
	~DebugAllocator() noexcept override final;

	// Overriden Allocator methods
	// --------------------------------------------------------------------------------------------

	void* allocate(uint64_t size, uint64_t alignment, const char* name) noexcept override final;
	void deallocate(void* pointer) noexcept override final;
	const char* getName() const noexcept override final;

	// Methods
	// --------------------------------------------------------------------------------------------
	
	/// Returns the current number of active allocations
	uint32_t numAllocations() const noexcept;

	/// Returns the number of allocations that has been deallocated
	uint32_t numDeallocated() const noexcept;

	/// Returns the total number of allocations made (both active and deallocated)
	uint32_t numTotalAllocations() const noexcept;

	/// Returns a list with all currently active allocations in this DebugAllocator. The list
	/// itself is allocated by this DebugAllocator and needs to be deallocated when done.
	DebugAllocationInfo* allocations(uint32_t* numAllocations) noexcept;

	/// Returns a list with all allocations that has been deallocated. The list itself is
	/// allocated by this DebugAllocator and needs to be deallocated when done.
	DebugAllocationInfo* deallocatedAllocations(uint32_t* numAllocations) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	DebugAllocatorImpl* mImpl = nullptr;
};

} // namespace sfz
