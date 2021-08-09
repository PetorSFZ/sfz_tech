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

#include <new>

#include "skipifzero.hpp"

namespace sfz {

// Array
// ------------------------------------------------------------------------------------------------

constexpr f32 ARRAY_DYNAMIC_GROW_RATE = 1.75;
constexpr u32 ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY = 64;
constexpr u32 ARRAY_DYNAMIC_MIN_CAPACITY = 2;
constexpr u32 ARRAY_DYNAMIC_MAX_CAPACITY = u32(U32_MAX / ARRAY_DYNAMIC_GROW_RATE) - 1;

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
class Array final {
public:
	SFZ_DECLARE_DROP_TYPE(Array);

	explicit Array(u32 capacity, SfzAllocator* allocator, SfzDbgInfo allocDbg) noexcept
	{
		this->init(capacity, allocator, allocDbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	// Initializes with specified parameters. Guaranteed to only set allocator and not allocate
	// memory if a capacity of 0 is requested.
	void init(u32 capacity, SfzAllocator* allocator, SfzDbgInfo allocDbg)
	{
		this->destroy();
		mAllocator = allocator;
		this->setCapacity(capacity, allocDbg);
	}

	// Removes all elements without deallocating memory.
	void clear() { sfz_assert(mSize <= mCapacity); for (u32 i = 0; i < mSize; i++) mData[i].~T(); mSize = 0; }

	// Destroys all elements, deallocates memory and removes allocator.
	void destroy()
	{
		this->clear();
		if (mData != nullptr) mAllocator->dealloc(mData);
		mCapacity = 0;
		mData = nullptr;
		mAllocator = nullptr;
	}

	// Directly sets the size without touching or initializing any elements. Only safe if T is a
	// trivial type and you know what you are doing, use at your own risk.
	void hackSetSize(u32 size) { mSize = (size <= mCapacity) ? size : mCapacity; }

	// Sets the capacity, allocating memory and moving elements if necessary.
	void setCapacity(u32 capacity, SfzDbgInfo allocDbg = sfz_dbg("Array"))
	{
		if (mSize > capacity) capacity = mSize;
		if (mCapacity == capacity) return;
		if (capacity < ARRAY_DYNAMIC_MIN_CAPACITY) capacity = ARRAY_DYNAMIC_MIN_CAPACITY;
		sfz_assert_hard(mAllocator != nullptr);
		sfz_assert_hard(capacity < ARRAY_DYNAMIC_MAX_CAPACITY);

		// Allocate memory and move/copy over elements from old memory
		T* newAllocation = capacity == 0 ? nullptr : (T*)mAllocator->alloc(
			allocDbg, capacity * sizeof(T), alignof(T) < 32 ? 32 : alignof(T));
		for (u32 i = 0; i < mSize; i++) new(newAllocation + i) T(sfz_move(mData[i]));

		// Destroy old memory and replace state with new memory and values
		u32 sizeBackup = mSize;
		SfzAllocator* allocatorBackup = mAllocator;
		this->destroy();
		mSize = sizeBackup;
		mCapacity = capacity;
		mData = newAllocation;
		mAllocator = allocatorBackup;
	}
	void ensureCapacity(u32 capacity) { if (mCapacity < capacity) setCapacity(capacity); }

	// Getters
	// --------------------------------------------------------------------------------------------

	u32 size() const { return mSize; }
	u32 capacity() const { return mCapacity; }
	const T* data() const { return mData; }
	T* data() { return mData; }
	SfzAllocator* allocator() const { return mAllocator; }

	bool isEmpty() const { return mSize == 0; }

	T& operator[] (u32 idx) { sfz_assert(idx < mSize); return mData[idx]; }
	const T& operator[] (u32 idx) const { sfz_assert(idx < mSize); return mData[idx]; }

	T& first() { sfz_assert(mSize > 0); return mData[0]; }
	const T& first() const { sfz_assert(mSize > 0); return mData[0]; }

	T& last() { sfz_assert(mSize > 0); return mData[mSize - 1]; }
	const T& last() const { sfz_assert(mSize > 0); return mData[mSize - 1]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	// Copy element numCopies times to the back of this array. Increases capacity if needed.
	void add(const T& value, u32 numCopies = 1) { addImpl<const T&>(value, numCopies); }
	void add(T&& value) { addImpl<T>(sfz_move(value), 1); }

	// Copy numElements elements to the back of this array. Increases capacity if needed.
	void add(const T* ptr, u32 numElements)
	{
		growIfNeeded(numElements);
		for (u32 i = 0; i < numElements; i++) new (this->mData + mSize + i) T(ptr[i]);
		mSize += numElements;
	}

	// Adds a zero:ed element and returns reference to it.
	T& add() { addImpl<T>({}, 1); return last(); }

	// Insert elements into the array at the specified position. Increases capacity if needed.
	void insert(u32 pos, const T& value) { insertImpl(pos, &value, 1); }
	void insert(u32 pos, const T* ptr, u32 numElements) { insertImpl(pos, ptr, numElements); }

	// Removes and returns the last element. Undefined if array is empty.
	T pop()
	{
		sfz_assert(mSize > 0);
		mSize -= 1;
		T tmp = sfz_move(mData[mSize]);
		mData[mSize].~T();
		return sfz_move(tmp);
	}

	// Remove numElements elements starting at the specified position.
	void remove(u32 pos, u32 numElements = 1)
	{
		// Destroy elements
		sfz_assert(pos < mSize);
		if (numElements > (mSize - pos)) numElements = (mSize - pos);
		for (u32 i = 0; i < numElements; i++) mData[pos + i].~T();

		// Move the elements after the removed elements
		u32 numElementsToMove = mSize - pos - numElements;
		for (u32 i = 0; i < numElementsToMove; i++) {
			new (mData + pos + i) T(sfz_move(mData[pos + i + numElements]));
			mData[pos + i + numElements].~T();
		}
		mSize -= numElements;
	}

	// Removes element at given position by swapping it with the last element in array.
	// O(1) operation unlike remove(), but obviously does not maintain internal array order.
	void removeQuickSwap(u32 pos) { sfz_assert(pos < mSize); sfz::swap(mData[pos], last()); remove(mSize - 1); }

	// Finds the first instance of the given element, nullptr if not found.
	T* findElement(const T& ref) { return findImpl(mData, [&](const T& e) { return e == ref; }); }
	const T* findElement(const T& ref) const { return findImpl(mData, [&](const T& e) { return e == ref; }); }

	// Finds the first element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* find(F func) { return findImpl(mData, func); }
	template<typename F> const T* find(F func) const { return findImpl(mData, func); }

	// Finds the last element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* findLast(F func) { return findLastImpl(mData, func); }
	template<typename F> const T* findLast(F func) const { return findLastImpl(mData, func); }

	// Sorts the elements in the array, same sort of comparator as std::sort().
	void sort() { sortImpl([](const T& lhs, const T& rhs) { return lhs < rhs; }); }
	template<typename F> void sort(F compareFunc) { sortImpl<F>(compareFunc); }

	// Iterator methods
	// --------------------------------------------------------------------------------------------

	T* begin() { return mData; }
	const T* begin() const { return mData; }
	const T* cbegin() const { return mData; }

	T* end() { return mData + mSize; }
	const T* end() const { return mData + mSize; }
	const T* cend() const { return mData + mSize; }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	void growIfNeeded(u32 elementsToAdd)
	{
		u32 newSize = mSize + elementsToAdd;
		if (newSize <= mCapacity) return;
		u32 newCapacity = (mCapacity == 0) ? ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY :
			sfz::max(u32(mCapacity * ARRAY_DYNAMIC_GROW_RATE), newSize);
		setCapacity(newCapacity);
	}

	template<typename ForwardT>
	void addImpl(ForwardT&& value, u32 numCopies)
	{
		// Perfect forwarding: const reference: ForwardT == const T&, rvalue: ForwardT == T
		// std::forward<ForwardT>(value) will then return the correct version of value
		this->growIfNeeded(numCopies);
		for(u32 i = 0; i < numCopies; i++) new (mData + mSize + i) T(sfz_forward(value));
		mSize += numCopies;
	}

	void insertImpl(u32 pos, const T* ptr, u32 numElements)
	{
		sfz_assert(pos <= mSize);
		growIfNeeded(numElements);

		// Move elements
		T* dstPtr = mData + pos + numElements;
		T* srcPtr = mData + pos;
		u32 numElementsToMove = (mSize - pos);
		for (u32 i = numElementsToMove; i > 0; i--) {
			u32 offs = i - 1;
			new (dstPtr + offs) T(sfz_move(srcPtr[offs]));
			srcPtr[offs].~T();
		}

		// Insert elements
		for (u32 i = 0; i < numElements; ++i) new (this->mData + pos + i) T(ptr[i]);
		mSize += numElements;
	}

	template<typename F>
	T* findImpl(T* data, F func) const
	{
		for (u32 i = 0; i < mSize; ++i) if (func(data[i])) return &data[i];
		return nullptr;
	}

	template<typename F>
	T* findLastImpl(T* data, F func) const
	{
		for (u32 i = mSize; i > 0; i--) if (func(data[i - 1])) return &data[i - 1];
		return nullptr;
	}

	template<typename F>
	void sortImpl(F cppCompareFunc)
	{
		if (mSize == 0) return;

		// Store pointer to compare in temp variable, fix so lambda don't have to capture.
		static thread_local const F* cppCompareFuncPtr = nullptr;
		cppCompareFuncPtr = &cppCompareFunc;

		// Convert C++ compare function to qsort compatible C compare function
		using CCompareT = int(const void*, const void*);
		CCompareT* cCompareFunc = [](const void* rawLhs, const void* rawRhs) -> int {
			const T& lhs = *static_cast<const T*>(rawLhs);
			const T& rhs = *static_cast<const T*>(rawRhs);
			const bool lhsSmaller = (*cppCompareFuncPtr)(lhs, rhs);
			if (lhsSmaller) return -1;
			const bool rhsSmaller = (*cppCompareFuncPtr)(rhs, lhs);
			if (rhsSmaller) return 1;
			return 0;
		};

		// Sort using C's qsort()
		std::qsort(mData, mSize, sizeof(T), cCompareFunc);
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	u32 mSize = 0, mCapacity = 0;
	T* mData = nullptr;
	SfzAllocator* mAllocator = nullptr;
};

// ArrayLocal
// ------------------------------------------------------------------------------------------------

template<typename T, u32 Capacity>
class ArrayLocal final {
public:
	static_assert(alignof(T) <= 16, "");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ArrayLocal() = default;
	ArrayLocal(const ArrayLocal&) = default;
	ArrayLocal& operator= (const ArrayLocal&) = default;
	ArrayLocal(ArrayLocal&& other) noexcept { this->swap(other); }
	ArrayLocal& operator= (ArrayLocal&& other) noexcept { this->swap(other); return *this; }
	~ArrayLocal() = default;

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(ArrayLocal& other)
	{
		for (u32 i = 0; i < Capacity; i++) sfz::swap(this->mData[i], other.mData[i]);
		sfz::swap(this->mSize, other.mSize);
	}

	void clear() { sfz_assert(mSize <= Capacity); for (u32 i = 0; i < mSize; i++) mData[i] = {}; mSize = 0; }
	void setSize(u32 size) { sfz_assert(size <= Capacity); mSize = size; }

	// Getters
	// --------------------------------------------------------------------------------------------

	u32 size() const { return mSize; }
	u32 capacity() const { return Capacity; }
	const T* data() const { return mData; }
	T* data() { return mData; }

	bool isEmpty() const { return mSize == 0; }
	bool isFull() const { return mSize == Capacity; }

	T& operator[] (u32 idx) { sfz_assert(idx < mSize); return mData[idx]; }
	const T& operator[] (u32 idx) const { sfz_assert(idx < mSize); return mData[idx]; }

	T& first() { sfz_assert(mSize > 0); return mData[0]; }
	const T& first() const { sfz_assert(mSize > 0); return mData[0]; }

	T& last() { sfz_assert(mSize > 0); return mData[mSize - 1]; }
	const T& last() const { sfz_assert(mSize > 0); return mData[mSize - 1]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	// Copy element numCopies times to the back of this array.
	void add(const T& value, u32 numCopies = 1) { addImpl<const T&>(value, numCopies); }
	void add(T&& value) { addImpl<T>(sfz_move(value), 1); }

	// Copy numElements elements to the back of this array.
	void add(const T* ptr, u32 numElements)
	{
		sfz_assert((mSize + numElements) <= Capacity);
		for (u32 i = 0; i < numElements; i++) mData[mSize + i] = ptr[i];
		mSize += numElements;
	}

	// Adds a zero:ed element and returns reference to it.
	T& add() { addImpl<T>({}, 1); return last(); }

	// Insert elements into the array at the specified position.
	void insert(u32 pos, const T& value) { insertImpl(pos, &value, 1); }
	void insert(u32 pos, const T* ptr, u32 numElements) { insertImpl(pos, ptr, numElements); }

	// Removes and returns the last element. Undefined if array is empty.
	T pop()
	{
		sfz_assert(mSize > 0);
		mSize -= 1;
		T tmp = sfz_move(mData[mSize]);
		mData[mSize].~T();
		return sfz_move(tmp);
	}

	// Remove numElements elements starting at the specified position.
	void remove(u32 pos, u32 numElements = 1)
	{
		// Destroy elements
		sfz_assert(pos < mSize);
		if (numElements > (mSize - pos)) numElements = (mSize - pos);
		for (u32 i = 0; i < numElements; i++) mData[pos + i] = {};

		// Move the elements after the removed elements
		u32 numElementsToMove = mSize - pos - numElements;
		for (u32 i = 0; i < numElementsToMove; i++) {
			mData[pos + i] = sfz_move(mData[pos + i + numElements]);
		}
		mSize -= numElements;
	}

	// Removes element at given position by swapping it with the last element in array.
	// O(1) operation unlike remove(), but obviously does not maintain internal array order.
	void removeQuickSwap(u32 pos) { sfz_assert(pos < mSize); sfz::swap(mData[pos], last()); remove(mSize - 1); }

	// Finds the first instance of the given element, nullptr if not found.
	T* findElement(const T& ref) { return findImpl(mData, [&](const T& e) { return e == ref; }); }
	const T* findElement(const T& ref) const { return findImpl(mData, [&](const T& e) { return e == ref; }); }

	// Finds the first element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* find(F func) { return findImpl(mData, func); }
	template<typename F> const T* find(F func) const { return findImpl(mData, func); }

	// Finds the last element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* findLast(F func) { return findLastImpl(mData, func); }
	template<typename F> const T* findLast(F func) const { return findLastImpl(mData, func); }

	// Sorts the elements in the array, same sort of comparator as std::sort().
	void sort() { sortImpl([](const T& lhs, const T& rhs) { return lhs < rhs; }); }
	template<typename F> void sort(F compareFunc) { sortImpl<F>(compareFunc); }

	// Iterator methods
	// --------------------------------------------------------------------------------------------

	T* begin() { return mData; }
	const T* begin() const { return mData; }
	const T* cbegin() const { return mData; }

	T* end() { return mData + mSize; }
	const T* end() const { return mData + mSize; }
	const T* cend() const { return mData + mSize; }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	template<typename ForwardT>
	void addImpl(ForwardT&& value, u32 numCopies)
	{
		// Perfect forwarding: const reference: ForwardT == const T&, rvalue: ForwardT == T
		// std::forward<ForwardT>(value) will then return the correct version of value
		sfz_assert((mSize + numCopies) <= Capacity);
		for(u32 i = 0; i < numCopies; i++) mData[mSize + i] = sfz_forward(value);
		mSize += numCopies;
	}

	void insertImpl(u32 pos, const T* ptr, u32 numElements)
	{
		sfz_assert(pos <= mSize);
		sfz_assert((mSize + numElements) <= Capacity);

		// Move elements
		T* dstPtr = mData + pos + numElements;
		T* srcPtr = mData + pos;
		u32 numElementsToMove = (mSize - pos);
		for (u32 i = numElementsToMove; i > 0; i--) dstPtr[i - 1] = sfz_move(srcPtr[i - 1]);

		// Insert elements
		for (u32 i = 0; i < numElements; ++i) mData[pos + i] = ptr[i];
		mSize += numElements;
	}

	template<typename F>
	T* findImpl(T* data, F func)
	{
		for (u32 i = 0; i < mSize; ++i) if (func(data[i])) return &data[i];
		return nullptr;
	}

	template<typename F>
	const T* findImpl(const T* data, F func) const
	{
		for (u32 i = 0; i < mSize; ++i) if (func(data[i])) return &data[i];
		return nullptr;
	}

	template<typename F>
	T* findLastImpl(T* data, F func)
	{
		for (u32 i = mSize; i > 0; i--) if (func(data[i - 1])) return &data[i - 1];
		return nullptr;
	}

	template<typename F>
	const T* findLastImpl(const T* data, F func) const
	{
		for (u32 i = mSize; i > 0; i--) if (func(data[i - 1])) return &data[i - 1];
		return nullptr;
	}

	template<typename F>
	void sortImpl(F cppCompareFunc)
	{
		if (mSize == 0) return;

		// Store pointer to compare in temp variable, fix so lambda don't have to capture.
		static thread_local const F* cppCompareFuncPtr = nullptr;
		cppCompareFuncPtr = &cppCompareFunc;

		// Convert C++ compare function to qsort compatible C compare function
		using CCompareT = int(const void*, const void*);
		CCompareT* cCompareFunc = [](const void* rawLhs, const void* rawRhs) -> int {
			const T& lhs = *static_cast<const T*>(rawLhs);
			const T& rhs = *static_cast<const T*>(rawRhs);
			const bool lhsSmaller = (*cppCompareFuncPtr)(lhs, rhs);
			if (lhsSmaller) return -1;
			const bool rhsSmaller = (*cppCompareFuncPtr)(rhs, lhs);
			if (rhsSmaller) return 1;
			return 0;
		};

		// Sort using C's qsort()
		std::qsort(mData, mSize, sizeof(T), cCompareFunc);
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	T mData[Capacity] = {};
	u32 mSize = 0;
	u32 mPadding[3] = {};
};

template<typename T> using Arr4 = ArrayLocal<T, 4>;
template<typename T> using Arr5 = ArrayLocal<T, 5>;
template<typename T> using Arr6 = ArrayLocal<T, 6>;
template<typename T> using Arr8 = ArrayLocal<T, 8>;
template<typename T> using Arr10 = ArrayLocal<T, 10>;
template<typename T> using Arr12 = ArrayLocal<T, 12>;
template<typename T> using Arr16 = ArrayLocal<T, 16>;
template<typename T> using Arr20 = ArrayLocal<T, 20>;
template<typename T> using Arr24 = ArrayLocal<T, 24>;
template<typename T> using Arr32 = ArrayLocal<T, 32>;
template<typename T> using Arr40 = ArrayLocal<T, 40>;
template<typename T> using Arr48 = ArrayLocal<T, 48>;
template<typename T> using Arr64 = ArrayLocal<T, 64>;
template<typename T> using Arr80 = ArrayLocal<T, 80>;
template<typename T> using Arr96 = ArrayLocal<T, 96>;
template<typename T> using Arr128 = ArrayLocal<T, 128>;
template<typename T> using Arr192 = ArrayLocal<T, 192>;
template<typename T> using Arr256 = ArrayLocal<T, 256>;
template<typename T> using Arr320 = ArrayLocal<T, 320>;
template<typename T> using Arr512 = ArrayLocal<T, 512>;

} // namespace sfz

#endif
