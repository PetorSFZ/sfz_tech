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
#include <cstdint>

#include "sfz/Context.hpp"
#include "sfz/memory/Allocator.hpp"

namespace sfz {

// RingBuffer constants
// ------------------------------------------------------------------------------------------------

constexpr uint64_t RINGBUFFER_BASE_IDX = (UINT64_MAX >> uint64_t(1)) + uint64_t(1);

// RingBuffer (interface)
// ------------------------------------------------------------------------------------------------

/// A class representing a RingBuffer (circular buffer, double ended queue).
///
/// Implemented using "infinite" indexes, i.e. under the assumption that the read/write indices can
/// become infinitely large. Since they are uint64_t, this is of course not the case. In practice
/// this should never become a problem, as it would take several years of runtime to overflow if you
/// move many billions of elements per second through the buffer.
///
/// Has some multi-threading guarantees. It is safe to have one thread add elements using add() and
/// another removing elements using pop() at the same time (likewise for the addFirst() & popLast()
/// pair). It is not safe to have multiple threads add elements at the same time, or have multiple
/// threads pop elements at the same time.
template<typename T>
class RingBuffer final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	/// Creates an empty RingBuffer without setting an allocator or allocating any memory.
	RingBuffer() noexcept = default;

	/// Creates a RingBuffer using create().
	explicit RingBuffer(uint64_t capacity, Allocator* allocator = getDefaultAllocator()) noexcept;

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
	void create(uint64_t capacity, Allocator* allocator = getDefaultAllocator()) noexcept;

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
	uint64_t size() const noexcept { return mLastIndex - mFirstIndex; }

	/// Returns the max number of elements that can be held by this RingBuffer
	uint64_t capacity() const noexcept { return mCapacity; }

	/// Returns the allocator of this RingBuffer. Will return nullptr if no allocator is set.
	Allocator* allocator() const noexcept { return mAllocator; }

	/// Element access operator. No bounds checks. Completely undefined if index is not a valid
	/// index in range [0, size), especially if size or capacity == 0.
	T& operator[] (uint64_t index) noexcept;
	const T& operator[] (uint64_t index) const noexcept;

	/// Accesses the first element (i.e. first inserted, low index). Undefined if RingBuffer does
	/// not contain at least one element.
	T& first() noexcept { return mDataPtr[mapIndex(mFirstIndex)]; }
	const T& first() const noexcept { return mDataPtr[mapIndex(mFirstIndex)]; }

	/// Accesses the last element (i.e. last inserted, high index). Undefined if RingBuffer does
	/// not contain at least one element.
	T& last() noexcept { return mDataPtr[mapIndex(mLastIndex - 1)]; }
	const T& last() const noexcept { return mDataPtr[mapIndex(mLastIndex - 1)]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	/// Adds an element to the end of the RingBuffer (i.e. last, high index). Returns true if
	/// element was successfully inserted, false if RingBuffer is full or has no capacity.
	bool add(const T& value) noexcept;
	bool add(T&& value) noexcept;
	bool add() noexcept;

	/// Removes the element at the beginning of the RingBuffer (i.e. first, low index). Returns true
	/// if element was successfully removed, false if RingBuffer has no elements to remove.
	bool pop(T& out) noexcept;
	bool pop() noexcept;

	/// Adds an element to the beginning of the RingBuffer (i.e. first, low index). Returns true if
	/// element was successfully inserted, false if RingBuffer is full or has no capacity.
	bool addFirst(const T& value) noexcept;
	bool addFirst(T&& value) noexcept;
	bool addFirst() noexcept;

	/// Removes the element at the end of the RingBuffer (i.e. last, high index). Returns true if
	/// element was successfully removed, false if RingBuffer has no elements to remove.
	bool popLast(T& out) noexcept;
	bool popLast() noexcept;

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	/// Maps an "infinite" index into an index into the data array
	uint64_t mapIndex(uint64_t index) const noexcept { return index % mCapacity; }

	/// Internal implementation of add(). Utilizes perfect forwarding in order to select whether to
	/// use const& or &&.
	template<typename PerfectT>
	bool addInternal(PerfectT&& value) noexcept;

	/// Internal implementation of addFirst(). Utilizes perfect forwarding in order to select
	/// whether to use const& or &&.
	template<typename PerfectT>
	bool addFirstInternal(PerfectT&& value) noexcept;

	/// Internal implementation of pop()
	bool popInternal(T* out) noexcept;

	/// Internal implementation of popLast()
	bool popLastInternal(T* out) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	Allocator* mAllocator = nullptr;
	T* mDataPtr = nullptr;
	uint64_t mCapacity = 0;
	volatile uint64_t mFirstIndex = RINGBUFFER_BASE_IDX;
	volatile uint64_t mLastIndex = RINGBUFFER_BASE_IDX;
};

} // namespace sfz

#include "sfz/containers/RingBuffer.inl"
