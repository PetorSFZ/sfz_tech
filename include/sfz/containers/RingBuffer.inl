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

namespace sfz {

// RingBuffer (implementation): Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename T>
RingBuffer<T>::RingBuffer(uint32_t capacity, Allocator* allocator) noexcept
{
	this->create(capacity, allocator);
}

// RingBuffer (implementation): State method
// ------------------------------------------------------------------------------------------------

template<typename T>
void RingBuffer<T>::create(uint32_t capacity, Allocator* allocator) noexcept
{
	// Make sure instance is in a clean state
	this->destroy();

	// Set allocator
	mAllocator = allocator;

	// If capacity is 0, do nothing.
	if (capacity == 0) return;
	mCapacity = capacity;

	// Allocate memory
	mDataPtr = (T*)mAllocator->allocate(
		mCapacity * sizeof(T), std::max<uint32_t>(32, alignof(T)), "RingBuffer");
}

template<typename T>
void RingBuffer<T>::swap(RingBuffer& other) noexcept
{
	std::swap(this->mAllocator, other.mAllocator);
	std::swap(this->mDataPtr, other.mDataPtr);
	std::swap(this->mSize, other.mSize);
	std::swap(this->mCapacity, other.mCapacity);
	std::swap(this->mFirstIndex, other.mFirstIndex);
}

template<typename T>
void RingBuffer<T>::destroy() noexcept
{
	// If no memory allocated, remove any potential allocator and return
	if (mDataPtr == nullptr) {
		mAllocator = nullptr;
		return;
	}

	// Remove elements
	this->clear();

	// Deallocate memory and reset member variables
	mAllocator->deallocate(mDataPtr);
	mAllocator = nullptr;
	mDataPtr = nullptr;
	mCapacity = 0;
}

template<typename T>
void RingBuffer<T>::clear() noexcept
{
	// Call destructors
	for (uint32_t i = 0; i < mSize; i++)
	{
		uint32_t index = (mFirstIndex + i) % mCapacity;
		mDataPtr[index].~T();
	}

	// Reset size and first index
	mSize = 0;
	mFirstIndex = 0;
}

// RingBuffer (implementation): Getters
// ------------------------------------------------------------------------------------------------

template<typename T>
T& RingBuffer<T>::operator[] (uint32_t index) noexcept
{
	return mDataPtr[(mFirstIndex + index) % mCapacity];
}

template<typename T>
const T& RingBuffer<T>::operator[] (uint32_t index) const noexcept
{
	return mDataPtr[(mFirstIndex + index) % mCapacity];
}

// RingBuffer (implementation): Methods
// ------------------------------------------------------------------------------------------------

template<typename T>
bool RingBuffer<T>::add(const T& value, bool overwrite) noexcept
{
	return this->addInternal<const T&>(value, overwrite);
}

template<typename T>
bool RingBuffer<T>::add(T&& value, bool overwrite) noexcept
{
	return this->addInternal<T>(std::move(value), overwrite);
}

template<typename T>
bool RingBuffer<T>::addFirst(const T& value, bool overwrite) noexcept
{
	return this->addFirstInternal<const T&>(value, overwrite);
}

template<typename T>
bool RingBuffer<T>::addFirst(T&& value, bool overwrite) noexcept
{
	return this->addFirstInternal<T>(std::move(value), overwrite);
}

// RingBuffer (implementation): Private methods
// --------------------------------------------------------------------------------------------

template<typename T>
template<typename PerfectT>
bool RingBuffer<T>::addInternal(PerfectT&& value, bool overwrite) noexcept
{
	// Utilizes perfect forwarding to determine if parameters are const references or rvalues.
	// const reference: PerfectT == const T&
	// rvalue: PerfectT == T
	// std::forward<PerfectT>(value) will then return the correct version of the value

	// Do nothing if no memory is allocated.
	if (mCapacity == 0) return false;

	// Check if RingBuffer is full
	if (mSize == mCapacity) {

		// Replace first element if overwrite
		if (overwrite) {
			mDataPtr[mFirstIndex] = std::forward<PerfectT>(value);
			mFirstIndex = (mFirstIndex + 1) % mCapacity;
			return true;
		}

		// Do not insert element if not overwrite
		else {
			return false;
		}
	}

	// Add element to buffer
	uint32_t writeIndex = (mFirstIndex + mSize) % mCapacity;
	new (mDataPtr + writeIndex) T(std::forward<PerfectT>(value));
	mSize += 1;
	return true;
}

template<typename T>
template<typename PerfectT>
bool RingBuffer<T>::addFirstInternal(PerfectT&& value, bool overwrite) noexcept
{
	// Utilizes perfect forwarding to determine if parameters are const references or rvalues.
	// const reference: PerfectT == const T&
	// rvalue: PerfectT == T
	// std::forward<PerfectT>(value) will then return the correct version of the value

	// Do nothing if no memory is allocated.
	if (mCapacity == 0) return false;

	// Check if RingBuffer is full
	if (mSize == mCapacity) {

		// Replace last element if overwrite
		if (overwrite) {
			uint32_t lastIndex = (mFirstIndex + mSize - 1) % mCapacity;
			mDataPtr[lastIndex] = std::forward<PerfectT>(value);
			mFirstIndex = lastIndex;
			return true;
		}

		// Do not insert element if not overwrite
		else {
			return false;
		}
	}

	// Add element to buffer
	mFirstIndex = mFirstIndex == 0 ? (mCapacity - 1) : (mFirstIndex - 1);
	if (mSize == 0) mFirstIndex = 0; // Special case were size == 0
	new (mDataPtr + mFirstIndex) T(std::forward<PerfectT>(value));
	mSize += 1;
	return true;
}

} // namespace sfz
