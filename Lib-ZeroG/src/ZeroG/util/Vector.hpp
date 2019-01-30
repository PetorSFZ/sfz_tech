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

#include "ZeroG/util/CpuAllocation.hpp"

namespace zg {

// Vector
// ------------------------------------------------------------------------------------------------

template<typename T>
class Vector final {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Vector() noexcept = default;
	Vector(const Vector&) = delete;
	Vector& operator= (const Vector&) = delete;
	Vector(Vector&&) = delete;
	Vector& operator= (Vector&&) = delete;
	~Vector() { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool create(uint32_t capacity, ZgAllocator allocator, const char* allocationName)
	{
		// Allocate memory
		uint8_t* allocation = allocator.allocate(
			allocator.userPtr, sizeof(T) * capacity, allocationName);
		if (allocation == nullptr) return false;

		// Destroy any previous state
		this->destroy();

		// Store state
		mAllocator = allocator;
		mCapacity = capacity;
		mDataPtr = reinterpret_cast<T*>(allocation);

		return true;
	}
	
	void destroy()
	{
		// Do nothing if empty
		if (mDataPtr == nullptr) return;

		// Destroy all members in vector
		for (uint32_t i = 0; i < mSize; i++) {
			mDataPtr[i].~T();
		}

		// Deallocate memory
		mAllocator.deallocate(mAllocator.userPtr, reinterpret_cast<uint8_t*>(mDataPtr));

		// Reset all members
		mSize = 0;
		mCapacity = 0;
		mDataPtr = nullptr;
		mAllocator = {};
	}

	// Methods
	// --------------------------------------------------------------------------------------------

	bool addMany(uint32_t numElements) noexcept
	{
		if (numElements == 0) return false;
		if ((mSize + numElements) > mCapacity) return false;
		for (uint32_t i = 0; i < numElements; i++) {
			new (mDataPtr + mSize + i) T();
		}
		mSize += numElements;
		return true;
	}

	bool add(const T& value) noexcept
	{
		if (mSize >= mCapacity) return false;
		new (mDataPtr + mSize) T(value);
		mSize += 1;
		return true;
	}

	bool add(T&& value) noexcept
	{
		if (mSize >= mCapacity) return false;
		new (mDataPtr + mSize) T(std::move(value));
		mSize += 1;
		return true;
	}

	bool pop(T& out) noexcept
	{
		if (mSize == 0) return false;
		mSize -= 1;
		out = mDataPtr[mSize];
		mDataPtr[mSize].~T();
		return true;
	}

	bool pop() noexcept
	{
		if (mSize == 0) return false;
		mSize -= 1;
		mDataPtr[mSize].~T();
		return true;
	}

	bool remove(uint32_t position) noexcept
	{
		if (position >= mSize) return false;
		mDataPtr[position].~T();

		uint32_t numElementsToMove = mSize - position - 1;
		for (uint32_t i = 0; i < numElementsToMove; i++) {
			uint32_t dstIndex = position + i;
			uint32_t srcIndex = position + i + 1;
			new (mDataPtr + dstIndex) T(std::move(mDataPtr[srcIndex]));
			mDataPtr[srcIndex].~T();
		}

		mSize -= 1;
	}

	T& operator[] (uint32_t index) noexcept { return mDataPtr[index]; }
	const T& operator[] (uint32_t index) const noexcept { return mDataPtr[index]; }

	// Getters
	// --------------------------------------------------------------------------------------------

	uint32_t size() const noexcept { return mSize; }
	uint32_t capacity() const noexcept { return mCapacity; }
	T* data() noexcept { return mDataPtr; }
	const T* data() const noexcept { return mDataPtr; }
	T& last() noexcept { return mDataPtr[mSize - 1]; }
	const T& last() const noexcept { return mDataPtr[mSize - 1]; }

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mSize = 0;
	uint32_t mCapacity = 0;
	T* mDataPtr = nullptr;
	ZgAllocator mAllocator = {};
};

} // namespace zg
