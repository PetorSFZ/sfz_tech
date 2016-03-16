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

// DynArray (implementation): Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
DynArray<T, Allocator>::DynArray(uint32_t size, uint32_t capacity) noexcept
{
	mSize = size;
	setCapacity(capacity);

	// Calling constructor for each element if not trivially default constructible
	if (!std::is_trivially_default_constructible<T>::value) {
		for (uint32_t i = 0; i < mSize; ++i) {
			new (mDataPtr + i) T();
		}
	}
}

template<typename T, typename Allocator>
DynArray<T, Allocator>::DynArray(uint32_t size, const T& value, uint32_t capacity) noexcept
{
	mSize = size;
	setCapacity(capacity);

	// Calling constructor for each element
	for (uint32_t i = 0; i < mSize; ++i) {
		new (mDataPtr + i) T(value);
	}
}

template<typename T, typename Allocator>
DynArray<T, Allocator>::~DynArray() noexcept
{
	this->destroy();
}

// DynArray (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
void DynArray<T, Allocator>::setCapacity(uint32_t capacity) noexcept
{
	if (mSize > capacity) capacity = mSize;
	if (mCapacity == capacity) return;

	if (mDataPtr == nullptr) {
		if (capacity == 0) return;
		mCapacity = capacity;
		mDataPtr = static_cast<T*>(Allocator::allocate(mCapacity * sizeof(T), 32));
		// TODO: Handle error case where allocation failed
		return;
	}

	if (capacity == 0) {
		this->destroy();
		return;
	}

	mCapacity = capacity;
	mDataPtr = static_cast<T*>(Allocator::reallocate(mDataPtr, mCapacity * sizeof(T), 32));
	// TODO: Handle error case where reallocation failed
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::clear() noexcept
{
	// Call destructor for each element if not trivially destructible
	if (!std::is_trivially_destructible<T>::value) {
		for (uint32_t i = 0; i < mSize; ++i) {
			mDataPtr[i].~T();
		}
	}

	mSize = 0;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::destroy() noexcept
{
	if (mDataPtr == nullptr) return;

	// Remove elements
	this->clear();

	// Deallocate memory
	Allocator::deallocate(mDataPtr);
	mCapacity = 0;
	mDataPtr = nullptr;
}

} // namespace sfz
