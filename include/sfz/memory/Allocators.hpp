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

#include <cstddef>
#include <cstdint>

namespace sfz {

using std::size_t;
using std::uint32_t;
using std::uintptr_t;

// sfzCore Allocator Interface
// ------------------------------------------------------------------------------------------------

// An sfzCore allocator is a non-instantiable class with the following interface:
//
// static void* allocate(size_t size, size_t alignment = 32) noexcept;
// static void* reallocate(void* previous, size_t newSize, size_t alignment = 32) noexcept;
// static void deallocate(void*& pointer) noexcept;
//
// Loosely allocate() maps to malloc(), reallocate() to realloc() and deallocate() to free().
// See the standard allocator for more info on the requirements of each of these functions.

// Standard allocator
// ------------------------------------------------------------------------------------------------

/// The standard allocator for sfzCore, implementing the sfzCore allocator interface
class StandardAllocator final {
public:

	StandardAllocator() = delete;
	StandardAllocator(const StandardAllocator&) = delete;
	StandardAllocator& operator= (const StandardAllocator&) = delete;

	/// Allocates memory with the specified byte alignment
	/// \param size the number of bytes to allocate
	/// \param alignment the byte alignment of the allocation
	/// \return pointer to allocated memory, nullptr if allocation failed
	static void* allocate(size_t size, size_t alignment = 32) noexcept;
	
	/// Reallocates memory to a new size
	/// Will attempt to expand or contract the previous allocation if possible. Otherwise it will
	/// allocate a new block and copy the memory from the previous block to it and then deallocate
	/// the old block.
	/// \param previous the previous allocation
	/// \param newSize the new size of the allocation
	/// \param alignment the byte alignment of the allocation, MUST be the same as of the old block
	/// \return pointer to the new allocation
	static void* reallocate(void* previous, size_t newSize, size_t alignment = 32) noexcept;

	/// Deallocates memory previously allocated with this allocator instance
	/// \param pointer to the memory, will be set to nullptr if deallocation succeeded
	static void deallocate(void*& pointer) noexcept;
};

} // namespace sfz