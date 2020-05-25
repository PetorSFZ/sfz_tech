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

#ifndef SKIPIFZERO_RING_BUFFERS_HPP
#define SKIPIFZERO_RING_BUFFERS_HPP
#pragma once

#include <atomic>

#include "skipifzero.hpp"

namespace sfz {

// RingBuffer
// ------------------------------------------------------------------------------------------------

// A RingBuffer (circular buffer, double ended queue).
//
// Implemented using "infinite" indexes, i.e. under the assumption that the read/write indices can
// become infinitely large. Since they are uint64_t, this is of course not the case. In practice
// this should never become a problem, as it would take several years of runtime to overflow if you
// move many billions of elements per second through the buffer.
//
// Has some multi-threading guarantees. It is safe to have one thread add elements using add() and
// another removing elements using pop() at the same time (likewise for the addFirst() & popLast()
// pair). It is not safe to have multiple threads add elements at the same time, or have multiple
// threads pop elements at the same time.
template<typename T>
class RingBuffer final {
public:
	static constexpr uint64_t BASE_IDX = (UINT64_MAX >> uint64_t(1)) + uint64_t(1);

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	SFZ_DECLARE_DROP_TYPE(RingBuffer);

	RingBuffer(uint64_t capacity, Allocator* allocator, DbgInfo allocDbg) noexcept
	{
		this->create(capacity, allocator, allocDbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void create(uint64_t capacity, Allocator* allocator, DbgInfo allocDbg)
	{
		this->destroy();
		if (capacity == 0) return;

		// Allocate memory
		mAllocator = allocator;
		mCapacity = capacity;
		mDataPtr = reinterpret_cast<T*>(mAllocator->allocate(
			allocDbg, mCapacity * sizeof(T), sfz::max(32u, uint32_t(alignof(T)))));
	}

	void destroy()
	{
		if (mDataPtr == nullptr) return;
		this->clear();
		mAllocator->deallocate(mDataPtr);
		mAllocator = nullptr;
		mDataPtr = nullptr;
		mCapacity = 0;
	}

	void clear()
	{
		for (uint64_t i = mFirstIndex, len = mLastIndex; i < len; i++) {
			mDataPtr[mapIndex(i)].~T();
		}
		mFirstIndex = BASE_IDX;
		mLastIndex = BASE_IDX;
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	uint64_t size() const { return mLastIndex - mFirstIndex; }
	uint64_t capacity() const { return mCapacity; }
	Allocator* allocator() const { return mAllocator; }

	// Access element in range [0, size), undefined if index is not valid.
	T& operator[] (uint64_t index) { sfz_assert(index < size()); return mDataPtr[mapIndex(mFirstIndex + index)]; }
	const T& operator[] (uint64_t index) const { sfz_assert(index < size()); return mDataPtr[mapIndex(mFirstIndex + index)]; }

	// Accesses the first (first inserted, low index) or last (last inserted, high index) element.
	T& first() { sfz_assert(mCapacity != 0); return mDataPtr[mapIndex(mFirstIndex)]; }
	const T& first() const { sfz_assert(mCapacity != 0); return mDataPtr[mapIndex(mFirstIndex)]; }
	T& last() { sfz_assert(mCapacity != 0); return mDataPtr[mapIndex(mLastIndex - 1)]; }
	const T& last() const { sfz_assert(mCapacity != 0); return mDataPtr[mapIndex(mLastIndex - 1)]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	// Adds an element to the end of the RingBuffer (i.e. last, high index). Returns true if
	// element was successfully inserted, false if RingBuffer is full or has no capacity.
	bool add(const T& value) { return this->addInternal<const T&>(value); }
	bool add(T&& value) { return this->addInternal<T>(std::move(value)); }
	bool add() { return this->addInternal<T>(T()); }

	// Removes the element at the beginning of the RingBuffer (i.e. first, low index). Returns true
	// if element was successfully removed, false if RingBuffer has no elements to remove.
	bool pop(T& out) { return this->popInternal(&out); }
	bool pop() { return this->popInternal(nullptr); }

	// Adds an element to the beginning of the RingBuffer (i.e. first, low index). Returns true if
	// element was successfully inserted, false if RingBuffer is full or has no capacity.
	bool addFirst(const T& value) { return this->addFirstInternal<const T&>(value); }
	bool addFirst(T&& value) { return this->addFirstInternal<T>(std::move(value)); }
	bool addFirst() { return this->addFirstInternal<T>(T()); }

	// Removes the element at the end of the RingBuffer (i.e. last, high index). Returns true if
	// element was successfully removed, false if RingBuffer has no elements to remove.
	bool popLast(T& out) { return this->popLastInternal(&out); }
	bool popLast() { return this->popLastInternal(nullptr); }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	// Maps an "infinite" index into an index into the data array
	uint64_t mapIndex(uint64_t index) const { return index % mCapacity; }

	template<typename PerfectT>
	bool addInternal(PerfectT&& value)
	{
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
		// Perfect forwarding: const reference: PerfectT == const T&, rvalue: PerfectT == T
		// std::forward<PerfectT>(value) will then return the correct version of value
		new (mDataPtr + lastArrayIndex) T(std::forward<PerfectT>(value));
		mLastIndex += 1; // Must increment after element creation, due to multi-threading
		return true;
	}

	template<typename PerfectT>
	bool addFirstInternal(PerfectT&& value)
	{
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
		// Perfect forwarding: const reference: PerfectT == const T&, rvalue: PerfectT == T
		// std::forward<PerfectT>(value) will then return the correct version of value
		firstArrayIndex = mapIndex(mFirstIndex - 1);
		new (mDataPtr + firstArrayIndex) T(std::forward<PerfectT>(value));
		mFirstIndex -= 1; // Must decrement after element creation, due to multi-threading
		return true;
	}

	bool popInternal(T* out)
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

	bool popLastInternal(T* out)
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

	Allocator* mAllocator = nullptr;
	T* mDataPtr = nullptr;
	uint64_t mCapacity = 0;
	std::atomic_uint64_t mFirstIndex{BASE_IDX};
	std::atomic_uint64_t mLastIndex{BASE_IDX};
};

} // namespace sfz

#endif
