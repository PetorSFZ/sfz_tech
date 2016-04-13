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
#include <cstring> // std::memcpy()
#include <type_traits>

#include "sfz/memory/Allocators.hpp"

namespace sfz {

using std::uint32_t;
using std::uint64_t;

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
/// DynArray iterators are simply pointers to the internal array. Modifying a DynArray while
/// iterating over it will likely have unintended consequences if you are not very careful.
///
/// Every method in DynArray is declared noexcept. This means that if any constructor or
/// destructor called throws an exception the program will terminate by std::terminate().
template<typename T, typename Allocator = StandardAllocator>
class DynArray final {
public:
	// Constants
	// --------------------------------------------------------------------------------------------

	static constexpr uint32_t ALIGNMENT = 32;
	static constexpr uint32_t DEFAULT_INITIAL_CAPACITY = 64;
	static constexpr uint64_t MAX_CAPACITY = 4294967295;
	static constexpr float CAPACITY_INCREASE_FACTOR = 1.75f;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	/// Creates an empty DynArray without allocating any memory
	DynArray() noexcept = default;

	/// Creates a DynArray with size initial number of elements and internal array of size 
	/// capacity. Each element will be initialized with the default constructor. If both size and
	/// capacity is 0 no memory will be allocated. If capacity is less than size the internal
	/// array will be of size size instead.
	/// \param size the number of elements to add
	/// \param capacity the capacity of the internal array
	explicit DynArray(uint32_t size, uint32_t capacity = 0) noexcept;

	/// Creates a DynArray with size initial number of elements and internal array of size 
	/// capacity. Each element will be initialized to the value parameter. If both size and
	/// capacity is 0 no memory will be allocated. If capacity is less than size the internal
	/// array will be of size size instead.
	/// \param size the number of elements to add
	/// \param capacity the capacity of the internal array
	DynArray(uint32_t size, const T& value, uint32_t capacity = 0) noexcept;

	/// Copy constructors. If the target DynArray has larger capacity than the source DynArray
	/// then the capacity remains intact and no memory reallocation is performed.
	DynArray(const DynArray& other) noexcept;
	DynArray& operator= (const DynArray& other) noexcept;

	/// Move constructors. Equivalent to calling target.swap(source).
	DynArray(DynArray&& other) noexcept;
	DynArray& operator= (DynArray&& other) noexcept;

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
	const T* data() const noexcept { return mDataPtr; }

	/// Returns pointer to the internal array. Do note that if the capacity is changed this pointer
	/// may be invalidated.
	T* data() noexcept { return mDataPtr; }

	/// Element access operator. No range checks.
	T& operator[] (uint32_t index) noexcept { return mDataPtr[index]; }

	/// Element access operator. No range checks.
	const T& operator[] (uint32_t index) const noexcept { return mDataPtr[index]; }

	/// Accesses the first element. Undefined if DynArray does not contain at least one element.
	T& first() noexcept { return mDataPtr[0]; }

	/// Accesses the first element. Undefined if DynArray does not contain at least one element.
	const T& first() const noexcept { return mDataPtr[0]; }

	/// Accesses the last element. Undefined if DynArray does not contain at least one element.
	T& last() noexcept { return mDataPtr[mSize - 1]; }
	
	/// Accesses the last element. Undefined if DynArray does not contain at least one element.
	const T& last() const noexcept { return mDataPtr[mSize - 1]; }

	// Public methods
	// --------------------------------------------------------------------------------------------
	
	/// Copy an element to the back of the internal array. Will increase capacity of internal 
	/// array if needed.
	void add(const T& value) noexcept;

	/// Move an element to the back of the internal array. Will increase capacity of the internal
	/// array if needed.
	void add(T&& value) noexcept;

	/// Copy a number of elements to the back of the DynArray from a contiguous array. Undefined
	/// behaviour if trying to add elements from this DynArray.
	void add(const T* arrayPtr, uint32_t numElements) noexcept;

	/// Copy all the elements from another DynArray to the back of this DynArray. Undefined
	/// behaviour if attempting to add elements from the same DynArray.
	void add(const DynArray& elements) noexcept;

	/// Insert an element to the specified position in the the internal array. Will move elements
	/// one position ahead to make room. Will increase capacity of internal array if needed.
	void insert(uint32_t position, const T& value) noexcept;

	/// Insert an element to the specified position in the the internal array. Will move elements
	/// one position ahead to make room. Will increase capacity of internal array if needed.
	void insert(uint32_t position, T&& value) noexcept;
	
	/// Insert a number of elements to the internal array starting at the specified position. Will
	/// move elements ahead to make room. Will increase capacity of the internal array if needed.
	/// Undefined behaviour if trying to add elements from this DynArray.
	void insert(uint32_t position, const T* arrayPtr, uint32_t numElements) noexcept;

	/// Remove a number of elements starting at the specified position. Elements after the
	/// specified range will be moved ahead in the array. If the numElements is larger than the
	/// number of elements in the array only the available ones will be removed.
	void remove(uint32_t position, uint32_t numElements = 1) noexcept;

	/// Swaps the contents of two DynArrays
	void swap(DynArray& other) noexcept;

	/// Sets the capacity of this DynArray. If the requested capacity is less than the size (number
	/// of elements) in this DynArray then the capacity will be set to the size instead.
	/// \param capacity the new capacity
	void setCapacity(uint32_t capacity) noexcept;

	/// Ensures this DynArray has at least the specified amount of capacity. If the current
	/// capacity is less than the requested one then setCapacity() will be called.
	void ensureCapacity(uint32_t capacity) noexcept;

	/// Removes all elements from this DynArray without deallocating memory or changing capacity
	void clear() noexcept;

	/// Destroys all elements stored in this DynArray and deallocates all memory. After this
	/// method is called the internal array is nullptr, size and capacity is 0. If the DynArray is
	/// already empty then this method will do nothing. It is not necessary to call this method
	/// manually, it will automatically be called in the destructor.
	void destroy() noexcept;

	/// Directly sets the internal size member. Only available if T is a trivial type. If the size
	/// parameter is larger than the capacity the internal size it will be set to capacity instead.
	void setSize(uint32_t size) noexcept;

	// Iterators
	// --------------------------------------------------------------------------------------------

	T* begin() noexcept;
	const T* begin() const noexcept;
	const T* cbegin() const noexcept;

	T* end() noexcept;
	const T* end() const noexcept;
	const T* cend() const noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------
	
	uint32_t mSize = 0, mCapacity = 0;
	T* mDataPtr = nullptr;
};

} // namespace sfz

#include "sfz/containers/DynArray.inl"
