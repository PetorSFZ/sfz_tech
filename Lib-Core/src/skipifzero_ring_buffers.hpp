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

#include "sfz.h"
#include "sfz_cpp.hpp"

namespace sfz {

// RingBuffer
// ------------------------------------------------------------------------------------------------

// A RingBuffer (circular buffer, double ended queue).
//
// Implemented using "infinite" indexes, i.e. under the assumption that the read/write indices can
// become infinitely large. Since they are u64, this is of course not the case. In practice
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
	static constexpr u64 BASE_IDX = (UINT64_MAX >> u64(1)) + u64(1);

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	SFZ_DECLARE_DROP_TYPE(RingBuffer);

	RingBuffer(u64 capacity, SfzAllocator* allocator, SfzDbgInfo alloc_dbg) noexcept
	{
		this->create(capacity, allocator, alloc_dbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void create(u64 capacity, SfzAllocator* allocator, SfzDbgInfo alloc_dbg)
	{
		this->destroy();
		if (capacity == 0) return;

		// Allocate memory
		m_allocator = allocator;
		m_capacity = capacity;
		m_data_ptr = reinterpret_cast<T*>(m_allocator->alloc(
			alloc_dbg, m_capacity * sizeof(T), u32_max(32u, u32(alignof(T)))));
	}

	void destroy()
	{
		if (m_data_ptr == nullptr) return;
		this->clear();
		m_allocator->dealloc(m_data_ptr);
		m_allocator = nullptr;
		m_data_ptr = nullptr;
		m_capacity = 0;
	}

	void clear()
	{
		for (u64 i = m_first_index, len = m_last_index; i < len; i++) {
			m_data_ptr[mapIndex(i)].~T();
		}
		m_first_index = BASE_IDX;
		m_last_index = BASE_IDX;
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	u64 size() const { return m_last_index - m_first_index; }
	u64 capacity() const { return m_capacity; }
	SfzAllocator* allocator() const { return m_allocator; }

	// Access element in range [0, size), undefined if index is not valid.
	T& operator[] (u64 index) { sfz_assert(index < size()); return m_data_ptr[mapIndex(m_first_index + index)]; }
	const T& operator[] (u64 index) const { sfz_assert(index < size()); return m_data_ptr[mapIndex(m_first_index + index)]; }

	// Accesses the first (first inserted, low index) or last (last inserted, high index) element.
	T& first() { sfz_assert(m_capacity != 0); return m_data_ptr[mapIndex(m_first_index)]; }
	const T& first() const { sfz_assert(m_capacity != 0); return m_data_ptr[mapIndex(m_first_index)]; }
	T& last() { sfz_assert(m_capacity != 0); return m_data_ptr[mapIndex(m_last_index - 1)]; }
	const T& last() const { sfz_assert(m_capacity != 0); return m_data_ptr[mapIndex(m_last_index - 1)]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	// Adds an element to the end of the RingBuffer (i.e. last, high index). Returns true if
	// element was successfully inserted, false if RingBuffer is full or has no capacity.
	bool add(const T& value) { return this->addInternal<const T&>(value); }
	bool add(T&& value) { return this->addInternal<T>(sfz_move(value)); }
	bool add() { return this->addInternal<T>(T()); }

	// Removes the element at the beginning of the RingBuffer (i.e. first, low index). Returns true
	// if element was successfully removed, false if RingBuffer has no elements to remove.
	bool pop(T& out) { return this->popInternal(&out); }
	bool pop() { return this->popInternal(nullptr); }

	// Adds an element to the beginning of the RingBuffer (i.e. first, low index). Returns true if
	// element was successfully inserted, false if RingBuffer is full or has no capacity.
	bool addFirst(const T& value) { return this->addFirstInternal<const T&>(value); }
	bool addFirst(T&& value) { return this->addFirstInternal<T>(sfz_move(value)); }
	bool addFirst() { return this->addFirstInternal<T>(T()); }

	// Removes the element at the end of the RingBuffer (i.e. last, high index). Returns true if
	// element was successfully removed, false if RingBuffer has no elements to remove.
	bool popLast(T& out) { return this->popLastInternal(&out); }
	bool popLast() { return this->popLastInternal(nullptr); }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	// Maps an "infinite" index into an index into the data array
	u64 mapIndex(u64 index) const { return index % m_capacity; }

	template<typename PerfectT>
	bool addInternal(PerfectT&& value)
	{
		if (m_capacity == 0) return false;

		// Map indices
		u64 first_array_index = mapIndex(m_first_index);
		u64 last_array_index = mapIndex(m_last_index);

		// Don't insert if buffer is full
		if (first_array_index == last_array_index) {
			// Don't exit if buffer is empty
			if (m_first_index != m_last_index) return false;
		}

		// Add element to buffer
		// Perfect forwarding: const reference: PerfectT == const T&, rvalue: PerfectT == T
		// std::forward<PerfectT>(value) will then return the correct version of value
		new (m_data_ptr + last_array_index) T(sfz_forward(value));
		m_last_index += 1; // Must increment after element creation, due to multi-threading
		return true;
	}

	template<typename PerfectT>
	bool addFirstInternal(PerfectT&& value)
	{
		if (m_capacity == 0) return false;

		// Map indices
		u64 first_array_index = mapIndex(m_first_index);
		u64 last_array_index = mapIndex(m_last_index);

		// Don't insert if buffer is full
		if (first_array_index == last_array_index) {
			// Don't exit if buffer is empty
			if (m_first_index != m_last_index) return false;
		}

		// Add element to buffer
		// Perfect forwarding: const reference: PerfectT == const T&, rvalue: PerfectT == T
		// std::forward<PerfectT>(value) will then return the correct version of value
		first_array_index = mapIndex(m_first_index - 1);
		new (m_data_ptr + first_array_index) T(sfz_forward(value));
		m_first_index -= 1; // Must decrement after element creation, due to multi-threading
		return true;
	}

	bool popInternal(T* out)
	{
		// Return no element if buffer is empty
		if (m_first_index == m_last_index) return false;

		// Move out element and call destructor.
		u64 first_array_index = mapIndex(m_first_index);
		if (out != nullptr) *out = sfz_move(m_data_ptr[first_array_index]);
		m_data_ptr[first_array_index].~T();

		// Increment index (after destructor called, because multi-threading)
		m_first_index += 1;

		return true;
	}

	bool popLastInternal(T* out)
	{
		// Return no element if buffer is empty
		if (m_first_index == m_last_index) return false;

		// Map index to array
		u64 last_array_index = mapIndex(m_last_index - 1);

		// Move out element and call destructor.
		if (out != nullptr) *out = sfz_move(m_data_ptr[last_array_index]);
		m_data_ptr[last_array_index].~T();

		// Decrement index (after destructor called, because multi-threading)
		m_last_index -= 1;

		return true;
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	SfzAllocator* m_allocator = nullptr;
	T* m_data_ptr = nullptr;
	u64 m_capacity = 0;
	std::atomic_uint64_t m_first_index{BASE_IDX};
	std::atomic_uint64_t m_last_index{BASE_IDX};
};

} // namespace sfz

#endif
