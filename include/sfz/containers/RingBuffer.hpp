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

#include <algorithm>

#include "sfz/Context.hpp"
#include "sfz/memory/Allocator.hpp"

namespace sfz {

// RingBuffer (interface)
// ------------------------------------------------------------------------------------------------

template<typename T>
class RingBuffer final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	/// Creates an empty RingBuffer without setting an allocator or allocating any memory.
	RingBuffer() noexcept = default;

	/// Creates a RingBuffer using create().
	explicit RingBuffer(uint32_t capacity, Allocator* allocator = getDefaultAllocator()) noexcept;

	/// Copying not allowed.
	RingBuffer(const RingBuffer&) = delete;
	RingBuffer& operator= (const RingBuffer&) = delete;

	/// Move constructors. Equivalent to calling target.swap(source).
	RingBuffer(RingBuffer&& other) noexcept { this->swap(other); }
	RingBuffer& operator= (RingBuffer&& other) noexcept { this->swap(other); return *this; }

	/// Destroys the RingBuffer using destroy().
	~RingBuffer() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	/// Calls destroy(), then sets the specified allocator and allocates memory from it.
	void create(uint32_t capacity, Allocator* allocator = getDefaultAllocator()) noexcept;

	/// Swaps the contents of two RingBuffers, including the allocator pointers.
	void swap(RingBuffer& other) noexcept;

	/// Destroys all elements stored in this RingBuffer, deallocates all memory and removes
	/// allocator. This method is always safe to call and will attempt to do the minimum amount of
	/// work. It is not necessary to call this method manually, it will automatically be called in
	/// the destructor.
	void destroy() noexcept;

	/// Removes all elements from this RingBuffer without deallocating memory, changing capacity or
	/// touching the allocator.
	void clear() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	/// Returns the number of elements currently in the RingBuffer
	uint32_t size() const noexcept { return mSize; }

	/// Returns the max number of elements that can be held by this RingBuffer
	uint32_t capacity() const noexcept { return mCapacity; }

	/// Returns the allocator of this RingBuffer. Will return nullptr if no allocator is set.
	Allocator* allocator() const noexcept { return mAllocator; }

	/// Element access operator. No bounds checks. Completely undefined if index is not a valid
	/// index in range [0, size), especially if size or capacity == 0.
	T& operator[] (uint32_t index) noexcept;
	const T& operator[] (uint32_t index) const noexcept;

	/// Accesses the first element (i.e. first inserted, low index). Undefined if RingBuffer does
	/// not contain at least one element.
	T& first() noexcept { return mDataPtr[mFirstIndex]; }
	const T& first() const noexcept { return mDataPtr[mFirstIndex]; }

	/// Accesses the last element (i.e. last inserted, high index). Undefined if RingBuffer does
	/// not contain at least one element.
	T& last() noexcept { return mDataPtr[(mFirstIndex + mSize - 1) % mCapacity]; }
	const T& last() const noexcept { return mDataPtr[(mFirstIndex + mSize - 1) % mCapacity]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	/// Adds an element to the end of the RingBuffer (i.e. high index). Returns whether the element
	/// was succesfully inserted into the buffer or not.
	///
	/// The overwrite parameter determines the behavior when the RingBuffer is full (i.e.
	/// capacity == size). If overwrite is false the element will not be inserted, if it is true
	/// the element will overwrite the first element (i.e. the element at the read end, index 0).
	bool add(const T& value, bool overwrite = false) noexcept;
	bool add(T&& value, bool overwrite = false) noexcept;

	/// Adds an element to the beginning of the RingBuffer (i.e. index 0). Returns whether the
	/// element was succesfully inserted into the buffer or not.
	///
	/// The overwrite parameter determines the behavior when the RingBuffer is full (i.e.
	/// capacity == size). If overwrite is false the element will not be inserted, if it is true
	/// the element will overwrite the last element (i.e. the element at the write end, index
	/// capacity-1).
	bool addFirst(const T& value, bool overwrite = false) noexcept;
	bool addFirst(T&& value, bool overwrite = false) noexcept;

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	/// Internal implementation of add(). Utilizes perfect forwarding in order to select whether to
	/// use const& or &&.
	template<typename PerfectT>
	bool addInternal(PerfectT&& value, bool overwrite) noexcept;

	/// Internal implementation of addFirst(). Utilizes perfect forwarding in order to select
	/// whether to use const& or &&.
	template<typename PerfectT>
	bool addFirstInternal(PerfectT&& value, bool overwrite) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	Allocator* mAllocator = nullptr;
	T* mDataPtr = nullptr;
	uint32_t mSize = 0;
	uint32_t mCapacity = 0;
	uint32_t mFirstIndex = 0;
};

} // namespace sfz

#include "sfz/containers/RingBuffer.inl"
