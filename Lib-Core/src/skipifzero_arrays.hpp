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

#ifndef SKIPIFZERO_ARRAYS_HPP
#define SKIPIFZERO_ARRAYS_HPP
#pragma once

#include "sfz.h"
#include "sfz_cpp.hpp"

#ifdef __cplusplus

// Array
// ------------------------------------------------------------------------------------------------

constexpr f32 SFZ_ARRAY_DYNAMIC_GROW_RATE = 1.75;
constexpr u32 SFZ_ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY = 64;
constexpr u32 SFZ_ARRAY_DYNAMIC_MIN_CAPACITY = 2;
constexpr u32 SFZ_ARRAY_DYNAMIC_MAX_CAPACITY = U32_MAX - 1;

// A class managing a dynamic array, somewhat like std::vector.
//
// An Array has both a size and a capacity. The size is the current number elements in the array,
// the capacity is the amount of elements the array can hold before it needs to be resized.
//
// An Array needs to be supplied an allocator before it can start allocating memory, this is done
// through the init() method (or it's constructor wrapper). Calling init() with capacity 0 is
// guaranteed to just set the allocator and not allocate any memory.
//
// Array does not guarantee that a specific element will always occupy the same position in memory.
// E.g., elements may be moved around when the array is modified. It is not safe to modify the
// Array when iterating over it, as the iterators will not update on resize.
template<typename T>
class SfzArray final {
public:
	SFZ_DECLARE_DROP_TYPE(SfzArray);

	explicit SfzArray(u32 capacity, SfzAllocator* allocator, SfzDbgInfo alloc_dbg) noexcept
	{
		this->init(capacity, allocator, alloc_dbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	// Initializes with specified parameters. Guaranteed to only set allocator and not allocate
	// memory if a capacity of 0 is requested.
	void init(u32 capacity, SfzAllocator* allocator, SfzDbgInfo alloc_dbg)
	{
		this->destroy();
		m_allocator = allocator;
		this->setCapacity(capacity, alloc_dbg);
	}

	// Removes all elements without deallocating memory.
	void clear() { sfz_assert(m_size <= m_capacity); for (u32 i = 0; i < m_size; i++) m_data[i].~T(); m_size = 0; }

	// Destroys all elements, deallocates memory and removes allocator.
	void destroy()
	{
		this->clear();
		if (m_data != nullptr) m_allocator->dealloc(m_data);
		m_capacity = 0;
		m_data = nullptr;
		m_allocator = nullptr;
	}

	// Directly sets the size without touching or initializing any elements. Only safe if T is a
	// trivial type and you know what you are doing, use at your own risk.
	void hackSetSize(u32 size) { m_size = (size <= m_capacity) ? size : m_capacity; }

	// Sets the capacity, allocating memory and moving elements if necessary.
	void setCapacity(u32 capacity, SfzDbgInfo alloc_dbg = sfz_dbg("Array"))
	{
		if (m_size > capacity) capacity = m_size;
		if (m_capacity == capacity) return;
		if (capacity < SFZ_ARRAY_DYNAMIC_MIN_CAPACITY) capacity = SFZ_ARRAY_DYNAMIC_MIN_CAPACITY;
		sfz_assert_hard(m_allocator != nullptr);
		sfz_assert_hard(capacity < SFZ_ARRAY_DYNAMIC_MAX_CAPACITY);

		// Allocate memory and move/copy over elements from old memory
		T* new_allocation = capacity == 0 ? nullptr : (T*)m_allocator->alloc(
			alloc_dbg, capacity * sizeof(T), alignof(T) < 32 ? 32 : alignof(T));
		for (u32 i = 0; i < m_size; i++) new(new_allocation + i) T(sfz_move(m_data[i]));

		// Destroy old memory and replace state with new memory and values
		u32 size_backup = m_size;
		SfzAllocator* allocator_backup = m_allocator;
		this->destroy();
		m_size = size_backup;
		m_capacity = capacity;
		m_data = new_allocation;
		m_allocator = allocator_backup;
	}
	void ensureCapacity(u32 capacity) { if (m_capacity < capacity) setCapacity(capacity); }

	// Getters
	// --------------------------------------------------------------------------------------------

	u32 size() const { return m_size; }
	u32 capacity() const { return m_capacity; }
	const T* data() const { return m_data; }
	T* data() { return m_data; }
	SfzAllocator* allocator() const { return m_allocator; }

	bool isEmpty() const { return m_size == 0; }

	T& operator[] (u32 idx) { sfz_assert(idx < m_size); return m_data[idx]; }
	const T& operator[] (u32 idx) const { sfz_assert(idx < m_size); return m_data[idx]; }

	T& first() { sfz_assert(m_size > 0); return m_data[0]; }
	const T& first() const { sfz_assert(m_size > 0); return m_data[0]; }

	T& last() { sfz_assert(m_size > 0); return m_data[m_size - 1]; }
	const T& last() const { sfz_assert(m_size > 0); return m_data[m_size - 1]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	// Copy element numCopies times to the back of this array. Increases capacity if needed.
	void add(const T& value, u32 num_copies = 1) { addImpl<const T&>(value, num_copies); }
	void add(T&& value) { addImpl<T>(sfz_move(value), 1); }

	// Copy numElements elements to the back of this array. Increases capacity if needed.
	void add(const T* ptr, u32 num_elements)
	{
		growIfNeeded(num_elements);
		for (u32 i = 0; i < num_elements; i++) new (this->m_data + m_size + i) T(ptr[i]);
		m_size += num_elements;
	}

	// Adds a zero:ed element and returns reference to it.
	T& add() { addImpl<T>({}, 1); return last(); }

	// Insert elements into the array at the specified position. Increases capacity if needed.
	void insert(u32 pos, const T& value) { insertImpl(pos, &value, 1); }
	void insert(u32 pos, const T* ptr, u32 num_elements) { insertImpl(pos, ptr, num_elements); }

	// Removes and returns the last element. Undefined if array is empty.
	T pop()
	{
		sfz_assert(m_size > 0);
		m_size -= 1;
		T tmp = sfz_move(m_data[m_size]);
		m_data[m_size].~T();
		return sfz_move(tmp);
	}

	// Remove numElements elements starting at the specified position.
	void remove(u32 pos, u32 num_elements = 1)
	{
		// Destroy elements
		sfz_assert(pos < m_size);
		if (num_elements > (m_size - pos)) num_elements = (m_size - pos);
		for (u32 i = 0; i < num_elements; i++) m_data[pos + i].~T();

		// Move the elements after the removed elements
		u32 num_elements_to_move = m_size - pos - num_elements;
		for (u32 i = 0; i < num_elements_to_move; i++) {
			new (m_data + pos + i) T(sfz_move(m_data[pos + i + num_elements]));
			m_data[pos + i + num_elements].~T();
		}
		m_size -= num_elements;
	}

	// Removes element at given position by swapping it with the last element in array.
	// O(1) operation unlike remove(), but obviously does not maintain internal array order.
	void removeQuickSwap(u32 pos) { sfz_assert(pos < m_size); sfzSwap(m_data[pos], last()); remove(m_size - 1); }

	// Finds the first instance of the given element, nullptr if not found.
	T* findElement(const T& ref) { return findImpl(m_data, [&](const T& e) { return e == ref; }); }
	const T* findElement(const T& ref) const { return findImpl(m_data, [&](const T& e) { return e == ref; }); }

	// Finds the first element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* find(F func) { return findImpl(m_data, func); }
	template<typename F> const T* find(F func) const { return findImpl(m_data, func); }

	// Finds the last element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* findLast(F func) { return findLastImpl(m_data, func); }
	template<typename F> const T* findLast(F func) const { return findLastImpl(m_data, func); }

	// Sorts the elements in the array, same sort of comparator as std::sort().
	void sort() { sortImpl([](const T& lhs, const T& rhs) { return lhs < rhs; }); }
	template<typename F> void sort(F compare_func) { sortImpl<F>(compare_func); }

	// Iterator methods
	// --------------------------------------------------------------------------------------------

	T* begin() { return m_data; }
	const T* begin() const { return m_data; }
	const T* cbegin() const { return m_data; }

	T* end() { return m_data + m_size; }
	const T* end() const { return m_data + m_size; }
	const T* cend() const { return m_data + m_size; }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	void growIfNeeded(u32 elements_to_add)
	{
		u32 new_size = m_size + elements_to_add;
		if (new_size <= m_capacity) return;
		u32 new_capacity = (m_capacity == 0) ? SFZ_ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY :
			u32_max(u32(m_capacity * SFZ_ARRAY_DYNAMIC_GROW_RATE), new_size);
		setCapacity(new_capacity);
	}

	template<typename ForwardT>
	void addImpl(ForwardT&& value, u32 num_copies)
	{
		// Perfect forwarding: const reference: ForwardT == const T&, rvalue: ForwardT == T
		// std::forward<ForwardT>(value) will then return the correct version of value
		this->growIfNeeded(num_copies);
		for(u32 i = 0; i < num_copies; i++) new (m_data + m_size + i) T(sfz_forward(value));
		m_size += num_copies;
	}

	void insertImpl(u32 pos, const T* ptr, u32 num_elements)
	{
		sfz_assert(pos <= m_size);
		growIfNeeded(num_elements);

		// Move elements
		T* dst_ptr = m_data + pos + num_elements;
		T* src_ptr = m_data + pos;
		u32 num_elements_to_move = (m_size - pos);
		for (u32 i = num_elements_to_move; i > 0; i--) {
			u32 offs = i - 1;
			new (dst_ptr + offs) T(sfz_move(src_ptr[offs]));
			src_ptr[offs].~T();
		}

		// Insert elements
		for (u32 i = 0; i < num_elements; ++i) new (this->m_data + pos + i) T(ptr[i]);
		m_size += num_elements;
	}

	template<typename F>
	T* findImpl(T* data, F func) const
	{
		for (u32 i = 0; i < m_size; ++i) if (func(data[i])) return &data[i];
		return nullptr;
	}

	template<typename F>
	T* findLastImpl(T* data, F func) const
	{
		for (u32 i = m_size; i > 0; i--) if (func(data[i - 1])) return &data[i - 1];
		return nullptr;
	}

	template<typename F>
	void sortImpl(F cpp_compare_func)
	{
		if (m_size == 0) return;

		// Store pointer to compare in temp variable, fix so lambda don't have to capture.
		static thread_local const F* cpp_compare_func_ptr = nullptr;
		cpp_compare_func_ptr = &cpp_compare_func;

		// Convert C++ compare function to qsort compatible C compare function
		using CCompareT = int(const void*, const void*);
		CCompareT* c_compare_func = [](const void* raw_lhs, const void* raw_rhs) -> int {
			const T& lhs = *static_cast<const T*>(raw_lhs);
			const T& rhs = *static_cast<const T*>(raw_rhs);
			const bool lhs_smaller = (*cpp_compare_func_ptr)(lhs, rhs);
			if (lhs_smaller) return -1;
			const bool rhs_smaller = (*cpp_compare_func_ptr)(rhs, lhs);
			if (rhs_smaller) return 1;
			return 0;
		};

		// Sort using C's qsort()
		qsort(m_data, m_size, sizeof(T), c_compare_func);
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	u32 m_size = 0, m_capacity = 0;
	T* m_data = nullptr;
	SfzAllocator* m_allocator = nullptr;
};

// SfzArrayLocal
// ------------------------------------------------------------------------------------------------

template<typename T, u32 Capacity>
class SfzArrayLocal final {
public:
	static_assert(alignof(T) <= 16, "");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	SfzArrayLocal() = default;
	SfzArrayLocal(const SfzArrayLocal&) = default;
	SfzArrayLocal& operator= (const SfzArrayLocal&) = default;
	SfzArrayLocal(SfzArrayLocal&& other) noexcept { this->swap(other); }
	SfzArrayLocal& operator= (SfzArrayLocal&& other) noexcept { this->swap(other); return *this; }
	~SfzArrayLocal() = default;

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(SfzArrayLocal& other)
	{
		for (u32 i = 0; i < Capacity; i++) sfzSwap(this->m_data[i], other.m_data[i]);
		sfzSwap(this->m_size, other.m_size);
	}

	void clear() { sfz_assert(m_size <= Capacity); for (u32 i = 0; i < m_size; i++) m_data[i] = {}; m_size = 0; }
	void setSize(u32 size) { sfz_assert(size <= Capacity); m_size = size; }

	// Getters
	// --------------------------------------------------------------------------------------------

	u32 size() const { return m_size; }
	u32 capacity() const { return Capacity; }
	const T* data() const { return m_data; }
	T* data() { return m_data; }

	bool isEmpty() const { return m_size == 0; }
	bool isFull() const { return m_size == Capacity; }

	T& operator[] (u32 idx) { sfz_assert(idx < m_size); return m_data[idx]; }
	const T& operator[] (u32 idx) const { sfz_assert(idx < m_size); return m_data[idx]; }

	T& first() { sfz_assert(m_size > 0); return m_data[0]; }
	const T& first() const { sfz_assert(m_size > 0); return m_data[0]; }

	T& last() { sfz_assert(m_size > 0); return m_data[m_size - 1]; }
	const T& last() const { sfz_assert(m_size > 0); return m_data[m_size - 1]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	// Copy element num_copies times to the back of this array.
	void add(const T& value, u32 num_copies = 1) { addImpl<const T&>(value, num_copies); }
	void add(T&& value) { addImpl<T>(sfz_move(value), 1); }

	// Copy num_elements elements to the back of this array.
	void add(const T* ptr, u32 num_elements)
	{
		sfz_assert((m_size + num_elements) <= Capacity);
		for (u32 i = 0; i < num_elements; i++) m_data[m_size + i] = ptr[i];
		m_size += num_elements;
	}

	// Adds a zero:ed element and returns reference to it.
	T& add() { addImpl<T>({}, 1); return last(); }

	// Insert elements into the array at the specified position.
	void insert(u32 pos, const T& value) { insertImpl(pos, &value, 1); }
	void insert(u32 pos, const T* ptr, u32 num_elements) { insertImpl(pos, ptr, num_elements); }

	// Removes and returns the last element. Undefined if array is empty.
	T pop()
	{
		sfz_assert(m_size > 0);
		m_size -= 1;
		T tmp = sfz_move(m_data[m_size]);
		m_data[m_size].~T();
		return sfz_move(tmp);
	}

	// Remove numElements elements starting at the specified position.
	void remove(u32 pos, u32 num_elements = 1)
	{
		// Destroy elements
		sfz_assert(pos < m_size);
		if (num_elements > (m_size - pos)) num_elements = (m_size - pos);
		for (u32 i = 0; i < num_elements; i++) m_data[pos + i] = {};

		// Move the elements after the removed elements
		u32 num_elements_to_move = m_size - pos - num_elements;
		for (u32 i = 0; i < num_elements_to_move; i++) {
			m_data[pos + i] = sfz_move(m_data[pos + i + num_elements]);
		}
		m_size -= num_elements;
	}

	// Removes element at given position by swapping it with the last element in array.
	// O(1) operation unlike remove(), but obviously does not maintain internal array order.
	void removeQuickSwap(u32 pos) { sfz_assert(pos < m_size); sfzSwap(m_data[pos], last()); remove(m_size - 1); }

	// Finds the first instance of the given element, nullptr if not found.
	T* findElement(const T& ref) { return findImpl(m_data, [&](const T& e) { return e == ref; }); }
	const T* findElement(const T& ref) const { return findImpl(m_data, [&](const T& e) { return e == ref; }); }

	// Finds the first element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* find(F func) { return findImpl(m_data, func); }
	template<typename F> const T* find(F func) const { return findImpl(m_data, func); }

	// Finds the last element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* findLast(F func) { return findLastImpl(m_data, func); }
	template<typename F> const T* findLast(F func) const { return findLastImpl(m_data, func); }

	// Sorts the elements in the array, same sort of comparator as std::sort().
	void sort() { sortImpl([](const T& lhs, const T& rhs) { return lhs < rhs; }); }
	template<typename F> void sort(F compare_func) { sortImpl<F>(compare_func); }

	// Iterator methods
	// --------------------------------------------------------------------------------------------

	T* begin() { return m_data; }
	const T* begin() const { return m_data; }
	const T* cbegin() const { return m_data; }

	T* end() { return m_data + m_size; }
	const T* end() const { return m_data + m_size; }
	const T* cend() const { return m_data + m_size; }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	template<typename ForwardT>
	void addImpl(ForwardT&& value, u32 num_copies)
	{
		// Perfect forwarding: const reference: ForwardT == const T&, rvalue: ForwardT == T
		// std::forward<ForwardT>(value) will then return the correct version of value
		sfz_assert((m_size + num_copies) <= Capacity);
		for(u32 i = 0; i < num_copies; i++) m_data[m_size + i] = sfz_forward(value);
		m_size += num_copies;
	}

	void insertImpl(u32 pos, const T* ptr, u32 num_elements)
	{
		sfz_assert(pos <= m_size);
		sfz_assert((m_size + num_elements) <= Capacity);

		// Move elements
		T* dstPtr = m_data + pos + num_elements;
		T* srcPtr = m_data + pos;
		u32 num_elements_to_move = (m_size - pos);
		for (u32 i = num_elements_to_move; i > 0; i--) dstPtr[i - 1] = sfz_move(srcPtr[i - 1]);

		// Insert elements
		for (u32 i = 0; i < num_elements; ++i) m_data[pos + i] = ptr[i];
		m_size += num_elements;
	}

	template<typename F>
	T* findImpl(T* data, F func)
	{
		for (u32 i = 0; i < m_size; ++i) if (func(data[i])) return &data[i];
		return nullptr;
	}

	template<typename F>
	const T* findImpl(const T* data, F func) const
	{
		for (u32 i = 0; i < m_size; ++i) if (func(data[i])) return &data[i];
		return nullptr;
	}

	template<typename F>
	T* findLastImpl(T* data, F func)
	{
		for (u32 i = m_size; i > 0; i--) if (func(data[i - 1])) return &data[i - 1];
		return nullptr;
	}

	template<typename F>
	const T* findLastImpl(const T* data, F func) const
	{
		for (u32 i = m_size; i > 0; i--) if (func(data[i - 1])) return &data[i - 1];
		return nullptr;
	}

	template<typename F>
	void sortImpl(F cpp_compare_func)
	{
		if (m_size == 0) return;

		// Store pointer to compare in temp variable, fix so lambda don't have to capture.
		static thread_local const F* cpp_compare_func_ptr = nullptr;
		cpp_compare_func_ptr = &cpp_compare_func;

		// Convert C++ compare function to qsort compatible C compare function
		using CCompareT = int(const void*, const void*);
		CCompareT* c_compare_func = [](const void* raw_lhs, const void* raw_rhs) -> int {
			const T& lhs = *static_cast<const T*>(raw_lhs);
			const T& rhs = *static_cast<const T*>(raw_rhs);
			const bool lhs_smaller = (*cpp_compare_func_ptr)(lhs, rhs);
			if (lhs_smaller) return -1;
			const bool rhs_smaller = (*cpp_compare_func_ptr)(rhs, lhs);
			if (rhs_smaller) return 1;
			return 0;
		};

		// Sort using C's qsort()
		qsort(m_data, m_size, sizeof(T), c_compare_func);
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	T m_data[Capacity] = {};
	u32 m_size = 0;
	u32 m_padding[3] = {};
};

template<typename T> using SfzArr4 = SfzArrayLocal<T, 4>;
template<typename T> using SfzArr5 = SfzArrayLocal<T, 5>;
template<typename T> using SfzArr6 = SfzArrayLocal<T, 6>;
template<typename T> using SfzArr8 = SfzArrayLocal<T, 8>;
template<typename T> using SfzArr10 = SfzArrayLocal<T, 10>;
template<typename T> using SfzArr12 = SfzArrayLocal<T, 12>;
template<typename T> using SfzArr16 = SfzArrayLocal<T, 16>;
template<typename T> using SfzArr20 = SfzArrayLocal<T, 20>;
template<typename T> using SfzArr24 = SfzArrayLocal<T, 24>;
template<typename T> using SfzArr32 = SfzArrayLocal<T, 32>;
template<typename T> using SfzArr40 = SfzArrayLocal<T, 40>;
template<typename T> using SfzArr48 = SfzArrayLocal<T, 48>;
template<typename T> using SfzArr64 = SfzArrayLocal<T, 64>;
template<typename T> using SfzArr80 = SfzArrayLocal<T, 80>;
template<typename T> using SfzArr96 = SfzArrayLocal<T, 96>;
template<typename T> using SfzArr128 = SfzArrayLocal<T, 128>;
template<typename T> using SfzArr192 = SfzArrayLocal<T, 192>;
template<typename T> using SfzArr256 = SfzArrayLocal<T, 256>;
template<typename T> using SfzArr320 = SfzArrayLocal<T, 320>;
template<typename T> using SfzArr512 = SfzArrayLocal<T, 512>;

#endif // __cplusplus
#endif // SKIPIFZERO_ARRAYS_HPP
