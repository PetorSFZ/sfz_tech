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

#include "skipifzero.hpp"

namespace sfz {

// Array
// ------------------------------------------------------------------------------------------------

constexpr float ARRAY_DYNAMIC_GROW_RATE = 1.75;
constexpr uint32_t ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY = 64;
constexpr uint32_t ARRAY_DYNAMIC_MIN_CAPACITY = 2;
constexpr uint32_t ARRAY_DYNAMIC_MAX_CAPACITY = uint32_t(UINT32_MAX / ARRAY_DYNAMIC_GROW_RATE) - 1;

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
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Array() noexcept = default;
	Array(const Array& other) noexcept { *this = other.clone(); }
	Array& operator= (const Array& other) noexcept { *this = other.clone(); return *this; }
	Array(Array&& other) noexcept { this->swap(other); }
	Array& operator= (Array&& other) noexcept { this->swap(other); return *this; }
	~Array() noexcept { this->destroy(); }

	explicit Array(uint32_t capacity, Allocator* allocator, DbgInfo allocDbg) noexcept {
		this->init(capacity, allocator, allocDbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	// Initializes with specified parameters. Guaranteed to only set allocator and not allocate
	// memory if a capacity of 0 is requested.
	void init(uint32_t capacity, Allocator* allocator, DbgInfo allocDbg)
	{
		this->destroy();
		mAllocator = allocator;
		this->setCapacity(capacity, allocDbg);
	}

	Array clone(DbgInfo allocDbg = sfz_dbg("Array")) const
	{
		Array tmp(mCapacity, mAllocator, allocDbg);
		tmp.add(mData, mSize);
		return tmp;
	}

	void swap(Array& other)
	{
		std::swap(this->mSize, other.mSize);
		std::swap(this->mCapacity, other.mCapacity);
		std::swap(this->mData, other.mData);
		std::swap(this->mAllocator, other.mAllocator);
	}

	// Removes all elements without deallocating memory.
	void clear() { for (uint32_t i = 0; i < mSize; i++) mData[i].~T(); mSize = 0; }

	// Destroys all elements, deallocates memory and removes allocator.
	void destroy()
	{
		this->clear();
		if (mData != nullptr) mAllocator->deallocate(mData);
		mCapacity = 0;
		mData = nullptr;
		mAllocator = nullptr;
	}

	// Directly sets the size without touching or initializing any elements. Only safe if T is a
	// trivial type and you know what you are doing, use at your own risk.
	void hackSetSize(uint32_t size) { mSize = (size <= mCapacity) ? size : mCapacity; }

	// Sets the capacity, allocating memory and moving elements if necessary.
	void setCapacity(uint32_t capacity, DbgInfo allocDbg = sfz_dbg("Array"))
	{
		if (mSize > capacity) capacity = mSize;
		if (mCapacity == capacity) return;
		if (capacity < ARRAY_DYNAMIC_MIN_CAPACITY) capacity = ARRAY_DYNAMIC_MIN_CAPACITY;
		sfz_assert_hard(mAllocator != nullptr);
		sfz_assert_hard(capacity < ARRAY_DYNAMIC_MAX_CAPACITY);

		// Allocate memory and move/copy over elements from old memory
		T* newAllocation = capacity == 0 ? nullptr : (T*)mAllocator->allocate(
			allocDbg, capacity * sizeof(T), alignof(T) < 32 ? 32 : alignof(T));
		for (uint32_t i = 0; i < mSize; i++) new(newAllocation + i) T(std::move(mData[i]));

		// Destroy old memory and replace state with new memory and values
		uint32_t sizeBackup = mSize;
		Allocator* allocatorBackup = mAllocator;
		this->destroy();
		mSize = sizeBackup;
		mCapacity = capacity;
		mData = newAllocation;
		mAllocator = allocatorBackup;
	}
	void ensureCapacity(uint32_t capacity) { if (mCapacity < capacity) setCapacity(capacity); }

	// Getters
	// --------------------------------------------------------------------------------------------

	uint32_t size() const { return mSize; }
	uint32_t capacity() const { return mCapacity; }
	const T* data() const { return mData; }
	T* data() { return mData; }
	Allocator* allocator() const { return mAllocator; }

	bool isEmpty() const { return mSize == 0; }

	T& operator[] (uint32_t idx) { sfz_assert(idx < mSize); return mData[idx]; }
	const T& operator[] (uint32_t idx) const { sfz_assert(idx < mSize); return mData[idx]; }

	T& first() { sfz_assert(mSize > 0); return mData[0]; }
	const T& first() const { sfz_assert(mSize > 0); return mData[0]; }

	T& last() { sfz_assert(mSize > 0); return mData[mSize - 1]; }
	const T& last() const { sfz_assert(mSize > 0); return mData[mSize - 1]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	// Copy element numCopies times to the back of this array. Increases capacity if needed.
	void add(const T& value, uint32_t numCopies = 1) { addImpl<const T&>(value, numCopies); }
	void add(T&& value) { addImpl<T>(std::move(value), 1); }

	// Copy numElements elements to the back of this array. Increases capacity if needed.
	void add(const T* ptr, uint32_t numElements)
	{
		growIfNeeded(numElements);
		for (uint32_t i = 0; i < numElements; i++) new (this->mData + mSize + i) T(ptr[i]);
		mSize += numElements;
	}

	// Insert elements into the array at the specified position. Increases capacity if needed.
	void insert(uint32_t pos, const T& value) { insertImpl(pos, &value, 1); }
	void insert(uint32_t pos, const T* ptr, uint32_t numElements) { insertImpl(pos, ptr, numElements); }

	// Removes the last element. If the array is empty nothing happens.
	void pop() { if (mSize == 0) return; mSize -= 1; mData[mSize].~T(); }

	// Remove numElements elements starting at the specified position.
	void remove(uint32_t pos, uint32_t numElements = 1)
	{
		// Destroy elements
		sfz_assert(pos < mSize);
		if (numElements > (mSize - pos)) numElements = (mSize - pos);
		for (uint32_t i = 0; i < numElements; i++) mData[pos + i].~T();

		// Move the elements after the removed elements
		uint32_t numElementsToMove = mSize - pos - numElements;
		for (uint32_t i = 0; i < numElementsToMove; i++) {
			new (mData + pos + i) T(std::move(mData[pos + i + numElements]));
			mData[pos + i + numElements].~T();
		}
		mSize -= numElements;
	}

	// Removes element at given position by swapping it with the last element in array.
	// O(1) operation unlike remove(), but obviously does not maintain internal array order.
	void removeQuickSwap(uint32_t pos) { sfz_assert(pos < mSize); std::swap(mData[pos], last()); remove(mSize - 1); }

	// Finds the first instance of the given element, nullptr if not found.
	T* findElement(const T& ref) { return findImpl(mData, [&](const T& e) { return e == ref; }); }
	const T* findElement(const T& ref) const { return findImpl(mData, [&](const T& e) { return e == ref; }); }

	// Finds the first element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* find(F func) { return findImpl(mData, func); }
	template<typename F> const T* find(F func) const { return findImpl(mData, func); }

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

	void growIfNeeded(uint32_t elementsToAdd)
	{
		uint32_t newSize = mSize + elementsToAdd;
		if (newSize <= mCapacity) return;
		uint32_t newCapacity = (mCapacity == 0) ? ARRAY_DYNAMIC_DEFAULT_INITIAL_CAPACITY :
			uint32_t(mCapacity * ARRAY_DYNAMIC_GROW_RATE);
		setCapacity(newCapacity);
	}

	template<typename ForwardT>
	void addImpl(ForwardT&& value, uint32_t numCopies)
	{
		// Perfect forwarding: const reference: ForwardT == const T&, rvalue: ForwardT == T
		// std::forward<ForwardT>(value) will then return the correct version of value
		this->growIfNeeded(numCopies);
		for(uint32_t i = 0; i < numCopies; i++) new (mData + mSize + i) T(std::forward<ForwardT>(value));
		mSize += numCopies;
	}

	void insertImpl(uint32_t pos, const T* ptr, uint32_t numElements)
	{
		sfz_assert(pos <= mSize);
		growIfNeeded(numElements);

		// Move elements
		T* dstPtr = mData + pos + numElements;
		T* srcPtr = mData + pos;
		uint32_t numElementsToMove = (mSize - pos);
		for (uint32_t i = numElementsToMove; i > 0; i--) {
			uint32_t offs = i - 1;
			new (dstPtr + offs) T(std::move(srcPtr[offs]));
			srcPtr[offs].~T();
		}

		// Insert elements
		for (uint32_t i = 0; i < numElements; ++i) new (this->mData + pos + i) T(ptr[i]);
		mSize += numElements;
	}

	template<typename F>
	T* findImpl(T* data, F func) const
	{
		for (uint32_t i = 0; i < mSize; ++i) if (func(data[i])) return &data[i];
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

	uint32_t mSize = 0, mCapacity = 0;
	T* mData = nullptr;
	Allocator* mAllocator = nullptr;
};

// ArrayLocal
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t Capacity>
class ArrayLocal final {
public:
	static_assert(alignof(T) <= 32, "");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ArrayLocal() { static_assert(sizeof(ArrayLocal) == (sizeof(T) * Capacity + sizeof(uint32_t)), ""); }
	ArrayLocal(const ArrayLocal&) = default;
	ArrayLocal& operator= (const ArrayLocal&) = default;
	ArrayLocal(ArrayLocal&& other) noexcept { this->swap(other); }
	ArrayLocal& operator= (ArrayLocal&& other) noexcept { this->swap(other); return *this; }
	~ArrayLocal() = default;

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(ArrayLocal& other)
	{
		for (uint32_t i = 0; i < Capacity; i++) std::swap(this->mData[i], other.mData[i]);
		std::swap(this->mSize, other.mSize);
	}

	void clear() { for (uint32_t i = 0; i < mSize; i++) mData[i] = {}; mSize = 0; }
	void setSize(uint32_t size) { sfz_assert(size <= Capacity); mSize = size; }

	// Getters
	// --------------------------------------------------------------------------------------------

	uint32_t size() const { return mSize; }
	uint32_t capacity() const { return Capacity; }
	const T* data() const { return mData; }
	T* data() { return mData; }

	bool isEmpty() const { return mSize == 0; }
	bool isFull() const { return mSize == Capacity; }

	T& operator[] (uint32_t idx) { sfz_assert(idx < mSize); return mData[idx]; }
	const T& operator[] (uint32_t idx) const { sfz_assert(idx < mSize); return mData[idx]; }

	T& first() { sfz_assert(mSize > 0); return mData[0]; }
	const T& first() const { sfz_assert(mSize > 0); return mData[0]; }

	T& last() { sfz_assert(mSize > 0); return mData[mSize - 1]; }
	const T& last() const { sfz_assert(mSize > 0); return mData[mSize - 1]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	// Copy element numCopies times to the back of this array.
	void add(const T& value, uint32_t numCopies = 1) { addImpl<const T&>(value, numCopies); }
	void add(T&& value) { addImpl<T>(std::move(value), 1); }

	// Copy numElements elements to the back of this array.
	void add(const T* ptr, uint32_t numElements)
	{
		sfz_assert((mSize + numElements) <= Capacity);
		for (uint32_t i = 0; i < numElements; i++) mData[mSize + i] = ptr[i];
		mSize += numElements;
	}

	// Insert elements into the array at the specified position.
	void insert(uint32_t pos, const T& value) { insertImpl(pos, &value, 1); }
	void insert(uint32_t pos, const T* ptr, uint32_t numElements) { insertImpl(pos, ptr, numElements); }

	// Removes the last element. If the array is empty nothing happens.
	void pop() { if (mSize == 0) return; mSize -= 1; mData[mSize] = {}; }

	// Remove numElements elements starting at the specified position.
	void remove(uint32_t pos, uint32_t numElements = 1)
	{
		// Destroy elements
		sfz_assert(pos < mSize);
		if (numElements > (mSize - pos)) numElements = (mSize - pos);
		for (uint32_t i = 0; i < numElements; i++) mData[pos + i] = {};

		// Move the elements after the removed elements
		uint32_t numElementsToMove = mSize - pos - numElements;
		for (uint32_t i = 0; i < numElementsToMove; i++) {
			mData[pos + i] = std::move(mData[pos + i + numElements]);
		}
		mSize -= numElements;
	}

	// Removes element at given position by swapping it with the last element in array.
	// O(1) operation unlike remove(), but obviously does not maintain internal array order.
	void removeQuickSwap(uint32_t pos) { sfz_assert(pos < mSize); std::swap(mData[pos], last()); remove(mSize - 1); }

	// Finds the first instance of the given element, nullptr if not found.
	T* findElement(const T& ref) { return findImpl(mData, [&](const T& e) { return e == ref; }); }
	const T* findElement(const T& ref) const { return findImpl(mData, [&](const T& e) { return e == ref; }); }

	// Finds the first element that satisfies the given function.
	// Function should have signature: bool func(const T& element)
	template<typename F> T* find(F func) { return findImpl(mData, func); }
	template<typename F> const T* find(F func) const { return findImpl(mData, func); }

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
	void addImpl(ForwardT&& value, uint32_t numCopies)
	{
		// Perfect forwarding: const reference: ForwardT == const T&, rvalue: ForwardT == T
		// std::forward<ForwardT>(value) will then return the correct version of value
		sfz_assert((mSize + numCopies) <= Capacity);
		for(uint32_t i = 0; i < numCopies; i++) mData[mSize + i] = std::forward<ForwardT>(value);
		mSize += numCopies;
	}

	void insertImpl(uint32_t pos, const T* ptr, uint32_t numElements)
	{
		sfz_assert(pos <= mSize);
		sfz_assert((mSize + numElements) <= Capacity);

		// Move elements
		T* dstPtr = mData + pos + numElements;
		T* srcPtr = mData + pos;
		uint32_t numElementsToMove = (mSize - pos);
		for (uint32_t i = numElementsToMove; i > 0; i--) dstPtr[i - 1] = std::move(srcPtr[i - 1]);

		// Insert elements
		for (uint32_t i = 0; i < numElements; ++i) mData[pos + i] = ptr[i];
		mSize += numElements;
	}

	template<typename F>
	T* findImpl(T* data, F func)
	{
		for (uint32_t i = 0; i < mSize; ++i) if (func(data[i])) return &data[i];
		return nullptr;
	}

	template<typename F>
	const T* findImpl(const T* data, F func) const
	{
		for (uint32_t i = 0; i < mSize; ++i) if (func(data[i])) return &data[i];
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
	uint32_t mSize = 0;
};

} // namespace sfz

#endif
