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
RingBuffer<T>::RingBuffer(uint64_t capacity, Allocator* allocator) noexcept
{
	this->create(capacity, allocator);
}

// RingBuffer (implementation): State method
// ------------------------------------------------------------------------------------------------

template<typename T>
void RingBuffer<T>::create(uint64_t capacity, Allocator* allocator) noexcept
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
		mCapacity * sizeof(T), sfzMax(32, alignof(T)), "RingBuffer");
}

template<typename T>
void RingBuffer<T>::swap(RingBuffer& other) noexcept
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
	for (uint64_t index = mFirstIndex; index < mLastIndex; index++) {
		mDataPtr[mapIndex(index)].~T();
	}

	// Reset indices
	mFirstIndex = RINGBUFFER_BASE_IDX;
	mLastIndex = RINGBUFFER_BASE_IDX;
}

// RingBuffer (implementation): Getters
// ------------------------------------------------------------------------------------------------

template<typename T>
T& RingBuffer<T>::operator[] (uint64_t index) noexcept
{
	return mDataPtr[mapIndex(mFirstIndex + index)];
}

template<typename T>
const T& RingBuffer<T>::operator[] (uint64_t index) const noexcept
{
	return mDataPtr[mapIndex(mFirstIndex + index)];
}

// RingBuffer (implementation): Methods
// ------------------------------------------------------------------------------------------------

template<typename T>
bool RingBuffer<T>::add(const T& value) noexcept
{
	return this->addInternal<const T&>(value);
}

template<typename T>
bool RingBuffer<T>::add(T&& value) noexcept
{
	return this->addInternal<T>(std::move(value));
}

template<typename T>
bool RingBuffer<T>::add() noexcept
{
	return this->addInternal<T>(T());
}

template<typename T>
bool RingBuffer<T>::pop(T& out) noexcept
{
	return this->popInternal(&out);
}

template<typename T>
bool RingBuffer<T>::pop() noexcept
{
	return this->popInternal(nullptr);
}

template<typename T>
bool RingBuffer<T>::addFirst(const T& value) noexcept
{
	return this->addFirstInternal<const T&>(value);
}

template<typename T>
bool RingBuffer<T>::addFirst(T&& value) noexcept
{
	return this->addFirstInternal<T>(std::move(value));
}

template<typename T>
bool RingBuffer<T>::addFirst() noexcept
{
	return this->addFirstInternal<T>(T());
}

template<typename T>
bool RingBuffer<T>::popLast(T& out) noexcept
{
	return this->popLastInternal(&out);
}

template<typename T>
bool RingBuffer<T>::popLast() noexcept
{
	return this->popLastInternal(nullptr);
}

// RingBuffer (implementation): Private methods
// --------------------------------------------------------------------------------------------

template<typename T>
template<typename PerfectT>
bool RingBuffer<T>::addInternal(PerfectT&& value) noexcept
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

template<typename T>
template<typename PerfectT>
bool RingBuffer<T>::addFirstInternal(PerfectT&& value) noexcept
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

template<typename T>
bool RingBuffer<T>::popInternal(T* out) noexcept
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

template<typename T>
bool RingBuffer<T>::popLastInternal(T* out) noexcept
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

} // namespace sfz
