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

// An sfzCore allocator is a class which fulfills the following conditions:
//
// 1, It must be default constructible and safely copyable.
//
// 2, Memory allocated through an allocator instance may only be deleted by the same instance.
//
// 3, It must implement the following methods:
//
// void* allocate(size_t numBytes, size_t alignment = 0) noexcept
// void deallocate(void*& pointer) noexcept
//
// If allocation fails allocate() should preferably return nullptr, but it is allowed to crash
// or terminate the program.
//
// Once the memory is deallocated deallocate() sets the pointer to nullptr. If deallocation fails
// the pointer is untouched.
//
// The alignment parameters specifies the byte alignment of the allocation, i.e. a value of 8
// would correspond to 8-byte alignment. The value of the alignment parameter must be a power of
// two, otherwise the behavior is undefined.

// Standard allocator
// ------------------------------------------------------------------------------------------------

/// The standard allocator for sfzCore, implementing the sfzCore allocator interface
class StandardAllocator final {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	StandardAllocator() noexcept = default;
	StandardAllocator(const StandardAllocator&) noexcept = default;
	StandardAllocator& operator= (const StandardAllocator&) noexcept = default;
	~StandardAllocator() noexcept = default;

	// Allocation functions
	// --------------------------------------------------------------------------------------------

	/// Allocates memory with the specified byte alignment
	/// \param numBytes the number of bytes to allocate
	/// \param alignment the byte alignment of the memory
	/// \return pointer to allocated memory, nullptr if allocation failed
	void* allocate(size_t numBytes, size_t alignment = 0) noexcept;
	
	/// Deallocates memory previously allocated with this allocator instance
	/// \param pointer to the memory, will be set to nullptr if deallocation succeeded
	void deallocate(void*& pointer) noexcept;
};

// Common memory functions
// ------------------------------------------------------------------------------------------------

/// Checks whether a pointer is aligned to a given byte aligment
/// \param pointer the pointer to test
/// \param alignment the byte aligment
inline bool isAligned(const void* pointer, size_t alignment) noexcept
{
	return ((uintptr_t)pointer % alignment) == 0;
}

} // namespace sfz