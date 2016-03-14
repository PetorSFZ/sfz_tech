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

#include <cstdint>
#include <type_traits>

#include "sfz/memory/Allocators.hpp"

namespace sfz {

using std::uint32_t;

// DynArray (interface)
// ------------------------------------------------------------------------------------------------

/// A class managing a dynamic array, somewhat like std::vector
///
/// A DynArray has both a size and a capacity. The size is the current number of active elements
/// in the internal array. The capacity on the other hand is the number of elements the internal
/// array can hold before it needs to be resized.
///
/// DynArray guarantees that the elements are stored in a (32-byte aligned) contiguous array. It 
/// does, however, not guarantee that a specific element will always occupy the same position in
/// memory. When inserting elements or resizing the internal array objects (or the whole array)
/// may be moved to different memory locations without any copy or move constructors being called.
///
/// Every method in DynArray is declared noexcept. This means that if any constructor or
/// destructor called throws an exception the program will terminate by std::terminate().
template<typename T, typename Allocator = StandardAllocator>
class DynArray final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	/// Creates an empty DynArray without allocating any memory
	DynArray() noexcept;

	/// Creates a DynArray with the initial number of elements and internal array capacity being
	/// equal to the specified size parameter. Each element will be initialized with the default
	/// constructor.
	/// \param size the number of elements to add, if size is 0 no memory will be allocated
	explicit DynArray(uint32_t size) noexcept;

	/// Creates a DynArray with the initial number of elements and internal array capacity being
	/// equal to the specified size parameter. Each element will be initialized to the value
	/// parameter.
	/// \param size the number of elements to add, if size is 0 no memory will be allocated
	/// \param value the value to set each element to
	DynArray(uint32_t size, const T& value) noexcept;

	// TODO: Copy & move constructors

	/// Destroys the internal array using destroy()
	~DynArray() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	/// Returns the size of this DynArray. This is the number of elements in the internal array,
	/// not the capacity of the array.
	uint32_t size() const noexcept { return mSize; }

	/// Returns the capacity of the internal array
	uint32_t capacity() const noexcept { return mCapacity; }

	/// Returns pointer to the internal array. Do note that if the capacity is changed this pointer
	/// may be invalidated.
	T* data() const noexcept { return mDataPtr; }

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Destroys all elements stored in this DynArray and deallocates all memory. After this
	/// method is called the internal array is nullptr, size and capacity is 0. If the DynArray is
	/// already empty then this method will do nothing. It is not necessary to call this method
	/// manually, it will automatically be called in the destructor.
	void destroy() noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------
	
	uint32_t mSize, mCapacity;
	T* mDataPtr;
};

} // namespace sfz

#include "sfz/containers/DynArray.inl"