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
#include <atomic>
#include <cstdint>

#include "ZeroG/util/CpuAllocation.hpp"

namespace zg {

// RingBuffer constants
// ------------------------------------------------------------------------------------------------

constexpr uint64_t RINGBUFFER_BASE_IDX = (UINT64_MAX >> uint64_t(1)) + uint64_t(1);

// RingBuffer (interface)
// ------------------------------------------------------------------------------------------------

/// A class representing a RingBuffer (circular buffer, double ended queue).
///
/// Modified version from sfzCore, see: https://github.com/PetorSFZ/sfzCore
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
	RingBuffer(uint32_t capacity, ZgAllocator allocator, const char* allocationName) noexcept
	{
		this->create(capacity, allocator, allocationName);
	}

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
	void create(uint32_t capacity, ZgAllocator allocator, const char* allocationName) noexcept
	{
		// Make sure instance is in a clean state
		this->destroy();

		// Set allocator
		mAllocator = allocator;

		// If capacity is 0, do nothing.
		if (capacity == 0) return;
		mCapacity = capacity;

		// Allocate memory
		mDataPtr = reinterpret_cast<T*>(
			mAllocator.allocate(mAllocator.userPtr, mCapacity * sizeof(T), allocationName));
	}

	/// Swaps the contents of two RingBuffers, including the allocator pointers.
	void swap(RingBuffer& other) noexcept
	{
		std::swap(this->mAllocator, other.mAllocator);
		std::swap(this->mDataPtr, other.mDataPtr);
		
		//std::swap(this->mFirstIndex, other.mFirstIndex);
		//std::swap(this->mLastIndex, other.mLastIndex);
		uint64_t thisFirstIndexCopy = this->mFirstIndex;
		uint64_t thisLastIndexCopy = this->mLastIndex;
		this->mFirstIndex.exchange(other.mFirstIndex);
		this->mLastIndex.exchange(other.mLastIndex);
		other.mFirstIndex.exchange(thisFirstIndexCopy);
		other.mLastIndex.exchange(thisLastIndexCopy);
		
		std::swap(this->mCapacity, other.mCapacity);
	}

	/// Destroys all elements stored in this RingBuffer, deallocates all memory and removes
	/// allocator. This method is always safe to call and will attempt to do the minimum amount of
	/// work. It is not necessary to call this method manually, it will automatically be called in
	/// the destructor.
	void destroy() noexcept
	{
		// If no memory allocated, remove any potential allocator and return
		if (mDataPtr == nullptr) {
			mAllocator = {};
			return;
		}

		// Remove elements
		this->clear();

		// Deallocate memory and reset member variables
		mAllocator.deallocate(mAllocator.userPtr, reinterpret_cast<uint8_t*>(mDataPtr));
		mAllocator = {};
		mDataPtr = nullptr;
		mCapacity = 0;
	}

	/// Removes all elements from this RingBuffer without deallocating memory, changing capacity or
	/// touching the allocator.
	void clear() noexcept
	{
		// Call destructors
		for (uint64_t index = mFirstIndex; index < mLastIndex; index++) {
			mDataPtr[mapIndex(index)].~T();
		}

		// Reset indices
		mFirstIndex = RINGBUFFER_BASE_IDX;
		mLastIndex = RINGBUFFER_BASE_IDX;
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	/// Returns the number of elements currently in the RingBuffer
	uint64_t size() const noexcept { return mLastIndex - mFirstIndex; }

	/// Returns the max number of elements that can be held by this RingBuffer
	uint32_t capacity() const noexcept { return mCapacity; }

	/// Element access operator. No bounds checks. Completely undefined if index is not a valid
	/// index in range [0, size), especially if size or capacity == 0.
	T& operator[] (uint64_t index) noexcept
	{
		return mDataPtr[mapIndex(mFirstIndex + index)];
	}
	const T& operator[] (uint64_t index) const noexcept
	{
		return mDataPtr[mapIndex(mFirstIndex + index)];
	}

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
	bool add(const T& value) noexcept { return this->addInternal<const T&>(value); }
	bool add(T&& value) noexcept { return this->addInternal<T>(std::move(value)); }
	bool add() noexcept { return this->addInternal<T>(T()); }

	/// Removes the element at the beginning of the RingBuffer (i.e. first, low index). Returns true
	/// if element was successfully removed, false if RingBuffer has no elements to remove.
	bool pop(T& out) noexcept { return this->popInternal(&out); }
	bool pop() noexcept { return this->popInternal(nullptr); }

	/// Adds an element to the beginning of the RingBuffer (i.e. first, low index). Returns true if
	/// element was successfully inserted, false if RingBuffer is full or has no capacity.
	bool addFirst(const T& value) noexcept { return this->addFirstInternal<const T&>(value); }
	bool addFirst(T&& value) noexcept { return this->addFirstInternal<T>(std::move(value)); }
	bool addFirst() noexcept { return this->addFirstInternal<T>(T()); }

	/// Removes the element at the end of the RingBuffer (i.e. last, high index). Returns true if
	/// element was successfully removed, false if RingBuffer has no elements to remove.
	bool popLast(T& out) noexcept { return this->popLastInternal(&out); }
	bool popLast() noexcept { return this->popLastInternal(nullptr); }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	/// Maps an "infinite" index into an index into the data array
	uint64_t mapIndex(uint64_t index) const noexcept { return index % uint64_t(mCapacity); }

	/// Internal implementation of add(). Utilizes perfect forwarding in order to select whether to
	/// use const& or &&.
	template<typename PerfectT>
	bool addInternal(PerfectT&& value) noexcept
	{
		// Utilizes perfect forwarding to determine if parameters are const references or rvalues.
		// const reference: PerfectT == const T&
		// rvalue: PerfectT == T
		// std::forward<PerfectT>(value) will then return the correct version of the value

		// Do nothing if no memory is allocated.
		if (mCapacity == 0) return false;

		// Map indices
		uint64_t firstArrayIndex = mapIndex(mFirstIndex);
		uint64_t lastArrayIndex = mapIndex(mLastIndex);

		// Don't insert if buffer is full
		if (firstArrayIndex == lastArrayIndex) {
			// Don't exit if buffer is empty
			if (mFirstIndex  != mLastIndex) return false;
		}

		// Add element to buffer
		new (mDataPtr + lastArrayIndex) T(std::forward<PerfectT>(value));
		mLastIndex += 1; // Must increment after element creation, due to multi-threading
		return true;
	}

	/// Internal implementation of addFirst(). Utilizes perfect forwarding in order to select
	/// whether to use const& or &&.
	template<typename PerfectT>
	bool addFirstInternal(PerfectT&& value) noexcept
	{
		// Utilizes perfect forwarding to determine if parameters are const references or rvalues.
		// const reference: PerfectT == const T&
		// rvalue: PerfectT == T
		// std::forward<PerfectT>(value) will then return the correct version of the value

		// Do nothing if no memory is allocated.
		if (mCapacity == 0) return false;

		// Map indices
		uint64_t firstArrayIndex = mapIndex(mFirstIndex);
		uint64_t lastArrayIndex = mapIndex(mLastIndex);

		// Don't insert if buffer is full
		if (firstArrayIndex == lastArrayIndex) {
			// Don't exit if buffer is empty
			if (mFirstIndex != mLastIndex) return false;
		}

		// Add element to buffer
		firstArrayIndex = mapIndex(mFirstIndex - 1);
		new (mDataPtr + firstArrayIndex) T(std::forward<PerfectT>(value));
		mFirstIndex -= 1; // Must decrement after element creation, due to multi-threading
		return true;
	}

	/// Internal implementation of pop()
	bool popInternal(T* out) noexcept
	{
		// Return no element if buffer is empty
		if (mFirstIndex == mLastIndex) return false;

		// Move out element and call destructor.
		uint64_t firstArrayIndex = mapIndex(mFirstIndex);
		if (out != nullptr) *out = std::move(mDataPtr[firstArrayIndex]);
		mDataPtr[firstArrayIndex].~T();

		// Increment index (after destructor called, because multi-threading)
		mFirstIndex += 1;

		return true;
	}

	/// Internal implementation of popLast()
	bool popLastInternal(T* out) noexcept
	{
		// Return no element if buffer is empty
		if (mFirstIndex == mLastIndex) return false;

		// Map index to array
		uint64_t lastArrayIndex = mapIndex(mLastIndex - 1);

		// Move out element and call destructor.
		if (out != nullptr) *out = std::move(mDataPtr[lastArrayIndex]);
		mDataPtr[lastArrayIndex].~T();

		// Decrement index (after destructor called, because multi-threading)
		mLastIndex -= 1;

		return true;
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	ZgAllocator mAllocator = {};
	T* mDataPtr = nullptr;
	uint32_t mCapacity = 0;
	std::atomic_uint64_t mFirstIndex = RINGBUFFER_BASE_IDX;
	std::atomic_uint64_t mLastIndex = RINGBUFFER_BASE_IDX;
};

} // namespace zg
