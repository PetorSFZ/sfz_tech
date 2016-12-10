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
		for (uint64_t i = 0; i < mSize; ++i) {
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
	for (uint64_t i = 0; i < mSize; ++i) {
		new (mDataPtr + i) T(value);
	}
}

template<typename T, typename Allocator>
DynArray<T, Allocator>::DynArray(const DynArray& other) noexcept
{
	*this = other;
}

template<typename T, typename Allocator>
DynArray<T, Allocator>& DynArray<T, Allocator>::operator= (const DynArray& other) noexcept
{
	// Don't copy to itself
	if (this == &other) return *this;

	// Don't copy if source is empty
	if (other.data() == nullptr) {
		this->destroy();
		return *this;
	}

	// Clear old elements and set capacity
	this->clear();
	if (this->mCapacity < other.mCapacity) {
		this->setCapacity(other.mCapacity);
	}

	this->mSize = other.mSize;

	// Copy elements
	// TODO: memcpy for trivially copyable types?
	for (uint32_t i = 0; i < mSize; ++i) {
		new (this->mDataPtr + i) T(*(other.mDataPtr + i));
	}

	return *this;
}

template<typename T, typename Allocator>
DynArray<T, Allocator>::DynArray(DynArray&& other) noexcept
{
	this->swap(other);
}

template<typename T, typename Allocator>
DynArray<T, Allocator>& DynArray<T, Allocator>::operator= (DynArray&& other) noexcept
{
	this->swap(other);
	return *this;
}

template<typename T, typename Allocator>
DynArray<T, Allocator>::~DynArray() noexcept
{
	this->destroy();
}

// DynArray (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
void DynArray<T, Allocator>::add(const T& value) noexcept
{
	if (mSize >= mCapacity) {
		uint64_t newCapacity = uint64_t(CAPACITY_INCREASE_FACTOR * mCapacity);
		if (newCapacity == 0) newCapacity = DEFAULT_INITIAL_CAPACITY;
		if (newCapacity > MAX_CAPACITY) newCapacity = MAX_CAPACITY;
		sfz_assert_debug(mCapacity < newCapacity);
		setCapacity(uint32_t(newCapacity));
	}
	new (mDataPtr + mSize) T(value);
	mSize += 1;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::add(T&& value) noexcept
{
	if (mSize >= mCapacity) {
		uint64_t newCapacity = uint64_t(CAPACITY_INCREASE_FACTOR * mCapacity);
		if (newCapacity == 0) newCapacity = DEFAULT_INITIAL_CAPACITY;
		if (newCapacity > MAX_CAPACITY) newCapacity = MAX_CAPACITY;
		sfz_assert_debug(mCapacity < newCapacity);
		setCapacity(uint32_t(newCapacity));
	}
	new (mDataPtr + mSize) T(std::move(value));
	mSize += 1;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::add(const T* arrayPtr, uint32_t numElements) noexcept
{
	// Assert that we do not attempt to add elements from this array to this array
	sfz_assert_debug(!(mDataPtr <= arrayPtr && arrayPtr < mDataPtr + mCapacity));

	if (mCapacity < mSize + numElements) {
		uint64_t newCapacity = uint64_t(CAPACITY_INCREASE_FACTOR * (uint64_t(mSize) + uint64_t(numElements)));
		if (newCapacity > MAX_CAPACITY) {
			newCapacity = MAX_CAPACITY;
		}
		sfz_assert_debug(mCapacity < newCapacity);
		setCapacity(uint32_t(newCapacity));
	}

	// Copy elements
	// TODO: memcpy for trivially copyable types?
	for (uint32_t i = 0; i < numElements; ++i) {
		new (this->mDataPtr + mSize + i) T(*(arrayPtr + i));
	}
	mSize += numElements;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::add(const DynArray& elements) noexcept
{
	this->add(elements.data(), elements.size());
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::insert(uint32_t position, const T& value) noexcept
{
	sfz_assert_debug(position <= mSize);
	if (mSize >= mCapacity) {
		uint64_t newCapacity = uint64_t(CAPACITY_INCREASE_FACTOR * mCapacity);
		if (newCapacity == 0) newCapacity = DEFAULT_INITIAL_CAPACITY;
		if (newCapacity > MAX_CAPACITY) newCapacity = MAX_CAPACITY;
		sfz_assert_debug(mCapacity < newCapacity);
		setCapacity(uint32_t(newCapacity));
	}

	// Move elements
	std::memmove(mDataPtr + position + 1, mDataPtr + position, (mSize - position) * sizeof(T));
	
	// Insert element
	new (mDataPtr + position) T(value);
	mSize += 1;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::insert(uint32_t position, T&& value) noexcept
{
	sfz_assert_debug(position <= mSize);
	if (mSize >= mCapacity) {
		uint64_t newCapacity = uint64_t(CAPACITY_INCREASE_FACTOR * mCapacity);
		if (newCapacity == 0) newCapacity = DEFAULT_INITIAL_CAPACITY;
		if (newCapacity > MAX_CAPACITY) newCapacity = MAX_CAPACITY;
		sfz_assert_debug(mCapacity < newCapacity);
		setCapacity(uint32_t(newCapacity));
	}

	// Move elements
	std::memmove(mDataPtr + position + 1, mDataPtr + position, (mSize - position) * sizeof(T));

	// Insert element
	new (mDataPtr + position) T(std::move(value));
	mSize += 1;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::insert(uint32_t position, const T* arrayPtr, uint32_t numElements) noexcept
{
	sfz_assert_debug(position <= mSize);
	if (mCapacity < mSize + numElements) {
		uint64_t newCapacity = uint64_t(CAPACITY_INCREASE_FACTOR * (uint64_t(mSize) + uint64_t(numElements)));
		if (newCapacity > MAX_CAPACITY) newCapacity = MAX_CAPACITY;
		sfz_assert_debug(mCapacity < newCapacity);
		setCapacity(uint32_t(newCapacity));
	}

	// Move elements
	std::memmove(mDataPtr + position + numElements, mDataPtr + position, (mSize - position) * sizeof(T));

	// Copy elements
	// TODO: memcpy for trivially copyable types?
	for (uint32_t i = 0; i < numElements; ++i) {
		new (this->mDataPtr + position + i) T(*(arrayPtr + i));
	}
	mSize += numElements;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::remove(uint32_t position, uint32_t numElements) noexcept
{
	// Destroy the elements to remove
	uint32_t numElementsToRemove = std::min(numElements, mSize - position);
	for (uint32_t i = 0; i < numElementsToRemove; i++) {
		mDataPtr[position + i].~T();
	}

	// Move the elements after the removed elements
	uint32_t numElementsToMove = mSize - position - numElementsToRemove;
	for (uint32_t i = 0; i < numElementsToMove; i++) {
		uint32_t toIndex = position + i;
		uint32_t fromIndex = position + i + numElementsToRemove;

		new (mDataPtr + toIndex) T(std::move(mDataPtr[fromIndex]));
		mDataPtr[fromIndex].~T();
	}

	mSize -= numElementsToRemove;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::removeLast() noexcept
{
	if (mSize > 0) {
		mSize -= 1;
		mDataPtr[mSize].~T();
	}
}

template<typename T, typename Allocator>
template<typename F>
T* DynArray<T, Allocator>::find(F func) noexcept
{
	for (uint32_t i = 0; i < mSize; ++i) {
		if (func(mDataPtr[i])) return &mDataPtr[i];
	}
	return nullptr;
}

template<typename T, typename Allocator>
template<typename F>
const T* DynArray<T, Allocator>::find(F func) const noexcept
{
	for (uint32_t i = 0; i < mSize; ++i) {
		if (func(mDataPtr[i])) return &mDataPtr[i];
	}
	return nullptr;
}

template<typename T, typename Allocator>
template<typename F>
int64_t DynArray<T, Allocator>::findIndex(F func) const noexcept
{
	for (uint32_t i = 0; i < mSize; ++i) {
		if (func(mDataPtr[i])) return int64_t(i);
	}
	return -1;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::swap(DynArray& other) noexcept
{
	uint32_t thisSize = this->mSize;
	uint32_t thisCapacity = this->mCapacity;
	T* thisDataPtr = this->mDataPtr;

	this->mSize = other.mSize;
	this->mCapacity = other.mCapacity;
	this->mDataPtr = other.mDataPtr;

	other.mSize = thisSize;
	other.mCapacity = thisCapacity;
	other.mDataPtr = thisDataPtr;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::setCapacity(uint32_t capacity) noexcept
{
	if (mSize > capacity) capacity = mSize;
	if (mCapacity == capacity) return;

	if (mDataPtr == nullptr) {
		if (capacity == 0) return;
		mCapacity = capacity;
		mDataPtr = static_cast<T*>(Allocator::allocate(mCapacity * sizeof(T), ALIGNMENT));
		sfz_assert_debug(mDataPtr != nullptr);
		return;
	}

	if (capacity == 0) {
		this->destroy();
		return;
	}

	// Allocate new memory and move/copy over elements from old memory
	T* newDataPtr = static_cast<T*>(Allocator::allocate(capacity * sizeof(T), ALIGNMENT));
	for (uint32_t i = 0; i < mSize; i++) {
		new (newDataPtr + i) T(std::move(mDataPtr[i]));
	}

	// Deallocate old memory
	uint32_t sizeCopy = mSize;
	this->destroy();

	// Replace state with new buffer
	mSize = sizeCopy;
	mCapacity = capacity;
	mDataPtr = newDataPtr;
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::ensureCapacity(uint32_t capacity) noexcept
{
	if (mCapacity < capacity) {
		this->setCapacity(capacity);
	}
}

template<typename T, typename Allocator>
void DynArray<T, Allocator>::clear() noexcept
{
	// Call destructor for each element if not trivially destructible
	if (!std::is_trivially_destructible<T>::value) {
		for (uint64_t i = 0; i < mSize; ++i) {
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

template<typename T, typename Allocator>
void DynArray<T, Allocator>::setSize(uint32_t size) noexcept
{
	static_assert(std::is_trivial<T>::value, "Can only set size if type is trivial");
	if (size > mCapacity) size = mCapacity;
	mSize = size;
}

// DynArray (implementation): Iterators
// --------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
T* DynArray<T, Allocator>::begin() noexcept
{
	return mDataPtr;
}

template<typename T, typename Allocator>
const T* DynArray<T, Allocator>::begin() const noexcept
{
	return mDataPtr;
}

template<typename T, typename Allocator>
const T* DynArray<T, Allocator>::cbegin() const noexcept
{
	return mDataPtr;
}

template<typename T, typename Allocator>
T* DynArray<T, Allocator>::end() noexcept
{
	return mDataPtr + mSize;
}

template<typename T, typename Allocator>
const T* DynArray<T, Allocator>::end() const noexcept
{
	return mDataPtr + mSize;
}

template<typename T, typename Allocator>
const T* DynArray<T, Allocator>::cend() const noexcept
{
	return mDataPtr + mSize;
}

// DynArray (implementation): Extra declaration of static constexpr constants (not required by MSVC).
// See: http://stackoverflow.com/a/14396189
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator> constexpr uint32_t DynArray<T,Allocator>::ALIGNMENT;
template<typename T, typename Allocator> constexpr uint32_t DynArray<T,Allocator>::DEFAULT_INITIAL_CAPACITY;
template<typename T, typename Allocator> constexpr uint64_t DynArray<T,Allocator>::MAX_CAPACITY;
template<typename T, typename Allocator> constexpr float DynArray<T,Allocator>::CAPACITY_INCREASE_FACTOR;

} // namespace sfz
