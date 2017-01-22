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

template<typename T>
DynArray<T>::DynArray(uint32_t capacity, Allocator* allocator) noexcept
{
	this->create(capacity, allocator);
}

template<typename T>
DynArray<T>::DynArray(const DynArray& other) noexcept
{
	*this = other;
}

template<typename T>
DynArray<T>& DynArray<T>::operator= (const DynArray& other) noexcept
{
	// Don't copy to itself
	if (this == &other) return *this;

	// Don't copy if source is empty
	if (other.data() == nullptr) {
		this->destroy();
		this->mAllocator = other.mAllocator; // Other might still have allocator even if empty
		return *this;
	}

	// Deallocate memory and set allocator if different
	if (this->mAllocator != other.mAllocator) {
		this->destroy();
		this->mAllocator = other.mAllocator;
	}

	// Clear old elements and set capacity
	this->clear();
	this->ensureCapacity(other.mCapacity);
	this->mSize = other.mSize;

	// Copy elements
	// TODO: memcpy for trivially copyable types?
	for (uint32_t i = 0; i < mSize; ++i) {
		new (this->mDataPtr + i) T(*(other.mDataPtr + i));
	}

	return *this;
}

template<typename T>
DynArray<T>::DynArray(const DynArray& other, Allocator* allocator) noexcept
{
	DynArray<T> tmp(other.mCapacity, allocator);

	// Copy elements
	for (uint32_t i = 0; i < other.mSize; i++) {
		new (tmp.mDataPtr + i) T(other[i]);
	}
	tmp.mSize = other.mSize;

	*this = std::move(tmp);
}

template<typename T>
DynArray<T>::DynArray(DynArray&& other) noexcept
{
	this->swap(other);
}

template<typename T>
DynArray<T>& DynArray<T>::operator= (DynArray&& other) noexcept
{
	this->swap(other);
	return *this;
}

template<typename T>
DynArray<T>::~DynArray() noexcept
{
	this->destroy();
}

// DynArray (implementation): State methods
// ------------------------------------------------------------------------------------------------

template<typename T>
void DynArray<T>::create(uint32_t capacity, Allocator* allocator) noexcept
{
	this->destroy();
	mAllocator = allocator;
	this->setCapacity(capacity);
}

template<typename T>
void DynArray<T>::swap(DynArray& other) noexcept
{
	uint32_t thisSize = this->mSize;
	uint32_t thisCapacity = this->mCapacity;
	T* thisDataPtr = this->mDataPtr;
	Allocator* thisAllocator = this->mAllocator;

	this->mSize = other.mSize;
	this->mCapacity = other.mCapacity;
	this->mDataPtr = other.mDataPtr;
	this->mAllocator = other.mAllocator;

	other.mSize = thisSize;
	other.mCapacity = thisCapacity;
	other.mDataPtr = thisDataPtr;
	other.mAllocator = thisAllocator;
}

template<typename T>
void DynArray<T>::destroy() noexcept
{
	if (mDataPtr == nullptr) {
		mAllocator = nullptr;
		return;
	}

	// Remove elements
	this->clear();

	// Deallocate memory
	mAllocator->deallocate(mDataPtr);
	mCapacity = 0;
	mDataPtr = nullptr;
	mAllocator = nullptr;
}

template<typename T>
void DynArray<T>::clear() noexcept
{
	// Call destructor for each element if not trivially destructible
	if (!std::is_trivially_destructible<T>::value) {
		for (uint64_t i = 0; i < mSize; ++i) {
			mDataPtr[i].~T();
		}
	}
	mSize = 0;
}

template<typename T>
void DynArray<T>::setSize(uint32_t size) noexcept
{
	static_assert(std::is_trivial<T>::value, "Can only set size if type is trivial");
	if (size > mCapacity) size = mCapacity;
	mSize = size;
}

template<typename T>
void DynArray<T>::setCapacity(uint32_t capacity) noexcept
{
	// Can't have less capacity than what is needed to store current elements
	if (mSize > capacity) capacity = mSize;

	// Check if capacity is already correct
	if (mCapacity == capacity) return;

	// Check if this is initial memory allocation
	if (mDataPtr == nullptr) {
		if (capacity == 0) return;

		// If no allocator is available, retrieve the default one
		if (mAllocator == nullptr) mAllocator = getDefaultAllocator();

		// Allocates memory and returns
		mCapacity = capacity;
		mDataPtr = (T*)mAllocator->allocate(mCapacity * sizeof(T),
		                                    std::max<uint32_t>(MINIMUM_ALIGNMENT, alignof(T)),
		                                    "DynArray");
		sfz_assert_debug(mDataPtr != nullptr);
		return;
	}

	// Backup allocator so we can restore it after destroy()
	Allocator* allocatorBackup = mAllocator;

	// Destroy DynArray (but keep allocator) if requested capacity is 0
	if (capacity == 0) {
		this->destroy();
		mAllocator = allocatorBackup;
		return;
	}

	// Allocate new memory and move/copy over elements from old memory
	T* newDataPtr = (T*)mAllocator->allocate(capacity * sizeof(T),
	                                         std::max<uint32_t>(MINIMUM_ALIGNMENT, alignof(T)),
	                                         "DynArray");
	for (uint32_t i = 0; i < mSize; i++) {
		new (newDataPtr + i) T(std::move(mDataPtr[i]));
	}

	// Deallocate old memory
	uint32_t sizeBackup = mSize;
	this->destroy();
	
	// Replace state with new buffer
	mSize = sizeBackup;
	mCapacity = capacity;
	mDataPtr = newDataPtr;
	mAllocator = allocatorBackup;
}

template<typename T>
void DynArray<T>::ensureCapacity(uint32_t capacity) noexcept
{
	if (mCapacity < capacity) {
		this->setCapacity(capacity);
	}
}

// DynArray (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename T>
void DynArray<T>::add(const T& value) noexcept
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

template<typename T>
void DynArray<T>::add(T&& value) noexcept
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

template<typename T>
void DynArray<T>::addMany(uint32_t numElements) noexcept
{
	if (numElements == 0) return;
	this->ensureCapacity(mSize + numElements); // TODO: A bit lazy, should use increase factor
	for (uint32_t i = 0; i < numElements; i++) {
		new (mDataPtr + mSize + i) T();
	}
	mSize += numElements;
}

template<typename T>
void DynArray<T>::addMany(uint32_t numElements, const T& value) noexcept
{
	if (numElements == 0) return;
	this->ensureCapacity(mSize + numElements); // TODO: A bit lazy, should use increase factor
	for (uint32_t i = 0; i < numElements; i++) {
		new (mDataPtr + mSize + i) T(value);
	}
	mSize += numElements;
}

template<typename T>
void DynArray<T>::add(const T* arrayPtr, uint32_t numElements) noexcept
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

template<typename T>
void DynArray<T>::add(const DynArray& elements) noexcept
{
	this->add(elements.data(), elements.size());
}

template<typename T>
void DynArray<T>::insert(uint32_t position, const T& value) noexcept
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
	T* dstPtr = mDataPtr + position + 1;
	T* srcPtr = mDataPtr + position;
	uint32_t numElementsToMove = (mSize - position);
	for (uint32_t i = numElementsToMove; i > 0; i--) {
		uint32_t offs = i - 1;
		new (dstPtr + offs) T(std::move(srcPtr[offs]));
		srcPtr[offs].~T();
	}

	// Insert element
	new (mDataPtr + position) T(value);
	mSize += 1;
}

template<typename T>
void DynArray<T>::insert(uint32_t position, T&& value) noexcept
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
	T* dstPtr = mDataPtr + position + 1;
	T* srcPtr = mDataPtr + position;
	uint32_t numElementsToMove = (mSize - position);
	for (uint32_t i = numElementsToMove; i > 0; i--) {
		uint32_t offs = i - 1;
		new (dstPtr + offs) T(std::move(srcPtr[offs]));
		srcPtr[offs].~T();
	}

	// Insert element
	new (mDataPtr + position) T(std::move(value));
	mSize += 1;
}

template<typename T>
void DynArray<T>::insert(uint32_t position, const T* arrayPtr, uint32_t numElements) noexcept
{
	sfz_assert_debug(position <= mSize);
	if (mCapacity < mSize + numElements) {
		uint64_t newCapacity = uint64_t(CAPACITY_INCREASE_FACTOR * (uint64_t(mSize) + uint64_t(numElements)));
		if (newCapacity > MAX_CAPACITY) newCapacity = MAX_CAPACITY;
		sfz_assert_debug(mCapacity < newCapacity);
		setCapacity(uint32_t(newCapacity));
	}

	// Move elements
	T* dstPtr = mDataPtr + position + numElements;
	T* srcPtr = mDataPtr + position;
	uint32_t numElementsToMove = (mSize - position);
	for (uint32_t i = numElementsToMove; i > 0; i--) {
		uint32_t offs = i - 1;
		new (dstPtr + offs) T(std::move(srcPtr[offs]));
		srcPtr[offs].~T();
	}

	// Copy elements
	// TODO: memcpy for trivially copyable types?
	for (uint32_t i = 0; i < numElements; ++i) {
		new (this->mDataPtr + position + i) T(*(arrayPtr + i));
	}
	mSize += numElements;
}

template<typename T>
void DynArray<T>::remove(uint32_t position, uint32_t numElements) noexcept
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

template<typename T>
void DynArray<T>::removeLast() noexcept
{
	if (mSize > 0) {
		mSize -= 1;
		mDataPtr[mSize].~T();
	}
}

template<typename T>
template<typename F>
T* DynArray<T>::find(F func) noexcept
{
	for (uint32_t i = 0; i < mSize; ++i) {
		if (func(mDataPtr[i])) return &mDataPtr[i];
	}
	return nullptr;
}

template<typename T>
template<typename F>
const T* DynArray<T>::find(F func) const noexcept
{
	for (uint32_t i = 0; i < mSize; ++i) {
		if (func(mDataPtr[i])) return &mDataPtr[i];
	}
	return nullptr;
}

template<typename T>
template<typename F>
int64_t DynArray<T>::findIndex(F func) const noexcept
{
	for (uint32_t i = 0; i < mSize; ++i) {
		if (func(mDataPtr[i])) return int64_t(i);
	}
	return -1;
}

// DynArray (implementation): Extra declaration of static constexpr constants (not required by MSVC).
// See: http://stackoverflow.com/a/14396189
// ------------------------------------------------------------------------------------------------

template<typename T> constexpr uint32_t DynArray<T>::MINIMUM_ALIGNMENT;
template<typename T> constexpr uint32_t DynArray<T>::DEFAULT_INITIAL_CAPACITY;
template<typename T> constexpr uint64_t DynArray<T>::MAX_CAPACITY;
template<typename T> constexpr float DynArray<T>::CAPACITY_INCREASE_FACTOR;

} // namespace sfz
