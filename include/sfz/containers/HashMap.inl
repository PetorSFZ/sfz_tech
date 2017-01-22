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

// Disable some MSVC warnings for this file
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4127)
#pragma warning(disable : 4189)
#endif

namespace sfz {

// HashMap (implementation): Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, typename Descr>
HashMap<K,V,Descr>::HashMap(uint32_t suggestedCapacity, Allocator* allocator) noexcept
{
	this->create(suggestedCapacity, allocator);
}

template<typename K, typename V, typename Descr>
HashMap<K,V,Descr>::HashMap(const HashMap& other) noexcept
{
	*this = other;
}

template<typename K, typename V, typename Descr>
HashMap<K,V,Descr>& HashMap<K,V,Descr>::operator= (const HashMap& other) noexcept
{
	// Don't copy to itself
	if (this == &other) return *this;

	// Clear and rehash this HashMap
	this->clear();
	this->mAllocator = other.mAllocator;
	this->rehash(other.mCapacity);

	// Add all elements from other HashMap
	for (ConstKeyValuePair pair : other) {
		this->put(pair.key, pair.value);
	}

	return *this;
}

template<typename K, typename V, typename Descr>
HashMap<K,V,Descr>::HashMap(const HashMap& other, Allocator* allocator) noexcept
{
	HashMap tmp(other.capacity(), allocator);

	// Add all elements from other HashMap
	for (ConstKeyValuePair pair : other) {
		tmp.put(pair.key, pair.value);
	}

	*this = std::move(tmp);
}

template<typename K, typename V, typename Descr>
HashMap<K,V,Descr>::HashMap(HashMap&& other) noexcept
{
	this->swap(other);
}

template<typename K, typename V, typename Descr>
HashMap<K,V,Descr>& HashMap<K,V,Descr>::operator= (HashMap&& other) noexcept
{
	this->swap(other);
	return *this;
}

template<typename K, typename V, typename Descr>
HashMap<K,V,Descr>::~HashMap() noexcept
{
	this->destroy();
}

// HashMap (implementation): State methods
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, typename Descr>
void HashMap<K,V,Descr>::create(uint32_t suggestedCapacity, Allocator* allocator) noexcept
{
	this->destroy();
	mAllocator = allocator;
	this->rehash(suggestedCapacity);
}

template<typename K, typename V, typename Descr>
void HashMap<K,V,Descr>::swap(HashMap& other) noexcept
{
	uint32_t thisSize = this->mSize;
	uint32_t thisCapacity = this->mCapacity;
	uint32_t thisPlaceholders = this->mPlaceholders;
	uint8_t* thisDataPtr = this->mDataPtr;
	Allocator* thisAllocator = this->mAllocator;
	
	this->mSize = other.mSize;
	this->mCapacity = other.mCapacity;
	this->mPlaceholders = other.mPlaceholders;
	this->mDataPtr = other.mDataPtr;
	this->mAllocator = other.mAllocator;

	other.mSize = thisSize;
	other.mCapacity = thisCapacity;
	other.mPlaceholders = thisPlaceholders;
	other.mDataPtr = thisDataPtr;
	other.mAllocator = thisAllocator;
}

template<typename K, typename V, typename Descr>
void HashMap<K,V,Descr>::destroy() noexcept
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
	mPlaceholders = 0;
	mDataPtr = nullptr;
	mAllocator = nullptr;
}

template<typename K, typename V, typename Descr>
void HashMap<K,V,Descr>::clear() noexcept
{
	if (mSize == 0) return;

	// Call destructors for all active keys and values if they are not trivially destructible
	if (!std::is_trivially_destructible<K>::value || !std::is_trivially_destructible<V>::value) {
		K* keyPtr = keysPtr();
		V* valuePtr = valuesPtr();
		for (uint64_t i = 0; i < mCapacity; ++i) {
			if (elementInfo((uint32_t)i) == ELEMENT_INFO_OCCUPIED) {
				keyPtr[i].~K();
				valuePtr[i].~V();
			}
		}
	}

	// Clear all element info bits
	std::memset(elementInfoPtr(), 0, sizeOfElementInfoArray());

	// Set size to 0
	mSize = 0;
	mPlaceholders = 0;
}

template<typename K, typename V, typename Descr>
void HashMap<K,V,Descr>::rehash(uint32_t suggestedCapacity) noexcept
{
	// Can't decrease capacity with rehash()
	if (suggestedCapacity < mCapacity) suggestedCapacity = mCapacity;
	
	// Don't rehash if capacity already exists and there are no placeholders
	if (suggestedCapacity == mCapacity && mPlaceholders == 0) return;

	// Convert the suggested capacity to a larger (if possible) prime number
	uint32_t newCapacity = findPrimeCapacity(suggestedCapacity);

	// Set default allocator if no allocator is set
	if (mAllocator == nullptr) mAllocator = getDefaultAllocator();

	// Create a new HashMap, set allocator and allocate memory to it
	HashMap tmp;
	tmp.mCapacity = newCapacity;
	tmp.mAllocator = mAllocator;
	tmp.mDataPtr = (uint8_t*)mAllocator->allocate(tmp.sizeOfAllocatedMemory(), ALIGNMENT, "HashMap");
	std::memset(tmp.mDataPtr, 0, tmp.sizeOfAllocatedMemory());

	// Iterate over all pairs of objects in this HashMap and move them to the new one
	if (this->mDataPtr != nullptr) {
		for (KeyValuePair pair : *this) {
			tmp.put(pair.key, std::move(pair.value));
		}
	}

	// Replace this HashMap with the new one
	this->swap(tmp);
}

template<typename K, typename V, typename Descr>
bool HashMap<K,V,Descr>::ensureProperlyHashed() noexcept
{
	// If HashMap is empty initialize with smallest size
	if (mCapacity == 0) {
		this->rehash(1);
		return true;
	}

	// Check if HashMap needs to be rehashed
	uint32_t maxOccupied = uint32_t(MAX_OCCUPIED_REHASH_FACTOR * mCapacity);
	if ((mSize + mPlaceholders) > maxOccupied) {

		// Determine whether capacity needs to be increase or if is enough to remove placeholders
		uint32_t maxSize = uint32_t(MAX_OCCUPIED_REHASH_FACTOR * mCapacity);
		bool needCapacityIncrease = mSize > maxSize;

		// Rehash
		this->rehash(mCapacity + (needCapacityIncrease ? 1 : 0));
		return true;
	}

	return false;
}

// HashMap (implementation): Getters
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, typename Descr>
V* HashMap<K,V,Descr>::get(const K& key) noexcept
{
	return this->getInternal<K,KeyHash,KeyEqual>(key);
}

template<typename K, typename V, typename Descr>
const V* HashMap<K,V,Descr>::get(const K& key) const noexcept
{
	return this->getInternal<K,KeyHash,KeyEqual>(key);
}

template<typename K, typename V, typename Descr>
V* HashMap<K,V,Descr>::get(const AltK& key) noexcept
{
	return this->getInternal<AltK,AltKeyHash,AltKeyKeyEqual>(key);
}

template<typename K, typename V, typename Descr>
const V* HashMap<K,V,Descr>::get(const AltK& key) const noexcept
{
	return this->getInternal<AltK,AltKeyHash,AltKeyKeyEqual>(key);
}

// HashMap (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, typename Descr>
V& HashMap<K,V,Descr>::put(const K& key, const V& value) noexcept
{
	return this->putInternal<const K&, const V&, KeyHash, KeyEqual>(key, value);
}

template<typename K, typename V, typename Descr>
V& HashMap<K,V,Descr>::put(const K& key, V&& value) noexcept
{
	return this->putInternal<const K&, V, KeyHash, KeyEqual>(key, std::move(value));
}

template<typename K, typename V, typename Descr>
V& HashMap<K,V,Descr>::put(K&& key, const V& value) noexcept
{
	return this->putInternal<K, const V&, KeyHash, KeyEqual>(std::move(key), value);
}

template<typename K, typename V, typename Descr>
V& HashMap<K,V,Descr>::put(K&& key, V&& value) noexcept
{
	return this->putInternal<K, V, KeyHash, KeyEqual>(std::move(key), std::move(value));
}

template<typename K, typename V, typename Descr>
V& HashMap<K,V,Descr>::put(const AltK& key, const V& value) noexcept
{
	return this->putInternal<const AltK&, const V&, AltKeyHash, AltKeyKeyEqual>(key, value);
}

template<typename K, typename V, typename Descr>
V& HashMap<K,V,Descr>::put(const AltK& key, V&& value) noexcept
{
	return this->putInternal<const AltK&, V, AltKeyHash, AltKeyKeyEqual>(key, std::move(value));
}

template<typename K, typename V, typename Descr>
V& HashMap<K,V,Descr>::operator[] (const K& key) noexcept
{
	V* ptr = this->get(key);
	if (ptr != nullptr) return *ptr;
	return this->put(key, V());
}

template<typename K, typename V, typename Descr>
V& HashMap<K,V,Descr>::operator[] (K&& key) noexcept
{
	V* ptr = this->get(key);
	if (ptr != nullptr) return *ptr;
	return this->put(std::move(key), V());
}

template<typename K, typename V, typename Descr>
V& HashMap<K,V,Descr>::operator[] (const AltK& key) noexcept
{
	V* ptr = this->get(key);
	if (ptr != nullptr) return *ptr;
	return this->put(key, V());
}

template<typename K, typename V, typename Descr>
bool HashMap<K,V,Descr>::remove(const K& key) noexcept
{
	return this->removeInternal<K,KeyHash,KeyEqual>(key);
}

template<typename K, typename V, typename Descr>
bool HashMap<K,V,Descr>::remove(const AltK& key) noexcept
{
	return this->removeInternal<AltK,AltKeyHash,AltKeyKeyEqual>(key);
}

// HashMap (implementation): Iterators
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::Iterator& HashMap<K,V,Descr>::Iterator::operator++ () noexcept
{
	// Go through map until we find next occupied slot
	for (uint32_t i = mIndex + 1; i < mHashMap->mCapacity; ++i) {
		uint8_t info = mHashMap->elementInfo(i);
		if (info == ELEMENT_INFO_OCCUPIED) {
			mIndex = i;
			return *this;
		}
	}

	// Did not find any more elements, set to end
	mIndex = uint32_t(~0);
	return *this;
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::Iterator HashMap<K,V,Descr>::Iterator::operator++ (int) noexcept
{
	auto copy = *this;
	++(*this);
	return copy;
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::KeyValuePair HashMap<K,V,Descr>::Iterator::operator* () noexcept
{
	sfz_assert_debug(mIndex != uint32_t(~0));
	sfz_assert_debug(mHashMap->elementInfo(mIndex) == ELEMENT_INFO_OCCUPIED);
	return KeyValuePair(mHashMap->keysPtr()[mIndex], mHashMap->valuesPtr()[mIndex]);
}

template<typename K, typename V, typename Descr>
bool HashMap<K,V,Descr>::Iterator::operator== (const Iterator& other) const noexcept
{
	return (this->mHashMap == other.mHashMap) && (this->mIndex == other.mIndex);
}

template<typename K, typename V, typename Descr>
bool HashMap<K,V,Descr>::Iterator::operator!= (const Iterator& other) const noexcept
{
	return !(*this == other);
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::ConstIterator& HashMap<K,V,Descr>::ConstIterator::operator++ () noexcept
{
	// Go through map until we find next occupied slot
	for (uint32_t i = mIndex + 1; i < mHashMap->mCapacity; ++i) {
		uint8_t info = mHashMap->elementInfo(i);
		if (info == ELEMENT_INFO_OCCUPIED) {
			mIndex = i;
			return *this;
		}
	}

	// Did not find any more elements, set to end
	mIndex = uint32_t(~0);
	return *this;
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::ConstIterator HashMap<K,V,Descr>::ConstIterator::operator++ (int) noexcept
{
	auto copy = *this;
	++(*this);
	return copy;
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::ConstKeyValuePair HashMap<K,V,Descr>::ConstIterator::operator* () noexcept
{
	sfz_assert_debug(mIndex != uint32_t(~0));
	sfz_assert_debug(mHashMap->elementInfo(mIndex) == ELEMENT_INFO_OCCUPIED);
	return ConstKeyValuePair(mHashMap->keysPtr()[mIndex], mHashMap->valuesPtr()[mIndex]);
}

template<typename K, typename V, typename Descr>
bool HashMap<K,V,Descr>::ConstIterator::operator== (const ConstIterator& other) const noexcept
{
	return (this->mHashMap == other.mHashMap) && (this->mIndex == other.mIndex);
}

template<typename K, typename V, typename Descr>
bool HashMap<K,V,Descr>::ConstIterator::operator!= (const ConstIterator& other) const noexcept
{
	return !(*this == other);
}

// HashMap (implementation): Iterator methods
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::Iterator HashMap<K,V,Descr>::begin() noexcept
{
	if (this->size() == 0) return Iterator(*this, uint32_t(~0));
	Iterator it(*this, 0);
	// Unless there happens to be an element in slot 0 we increment the iterator to find it
	if (elementInfo(uint32_t(0)) != ELEMENT_INFO_OCCUPIED) {
		++it;
	}
	return it;
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::ConstIterator HashMap<K,V,Descr>::begin() const noexcept
{
	return cbegin();
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::ConstIterator HashMap<K,V,Descr>::cbegin() const noexcept
{
	if (this->size() == 0) return ConstIterator(*this, uint32_t(~0));
	ConstIterator it(*this, 0);
	// Unless there happens to be an element in slot 0 we increment the iterator to find it
	if (elementInfo(uint32_t(0)) != ELEMENT_INFO_OCCUPIED) {
		++it;
	}
	return it;
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::Iterator HashMap<K,V,Descr>::end() noexcept
{
	return Iterator(*this, uint32_t(~0));
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::ConstIterator HashMap<K,V,Descr>::end() const noexcept
{
	return cend();
}

template<typename K, typename V, typename Descr>
typename HashMap<K,V,Descr>::ConstIterator HashMap<K,V,Descr>::cend() const noexcept
{
	return ConstIterator(*this, uint32_t(~0));
}

// HashMap (implementation): Private methods
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, typename Descr>
uint32_t HashMap<K,V,Descr>::findPrimeCapacity(uint32_t capacity) const noexcept
{
	constexpr uint32_t PRIMES[] = {
		67,
		131,
		257,
		521,
		1031,
		2053,
		4099,
		8209,
		16411,
		32771,
		65537,
		131101,
		262147,
		524309,
		1048583,
		2097169,
		4194319,
		8388617,
		16777259,
		33554467,
		67108879,
		134217757,
		268435459,
		536870923,
		1073741827,
		2147483659
	};

	// Linear search is probably okay for an array this small
	for (uint32_t i = 0; i < sizeof(PRIMES) / sizeof(uint32_t); ++i) {
		if (PRIMES[i] >= capacity) return PRIMES[i];
	}

	// Found no prime, which means that the suggested capacity is too large.
	return MAX_CAPACITY;
}

template<typename K, typename V, typename Descr>
uint64_t HashMap<K,V,Descr>::sizeOfElementInfoArray() const noexcept
{
	// 2 bits per info element, + 1 since mCapacity always is odd.
	uint64_t infoMinRequiredSize = (mCapacity >> 2) + 1;

	// Calculate how many alignment sized chunks is needed to store element info
	uint64_t infoNumAlignmentSizedChunks = (infoMinRequiredSize >> ALIGNMENT_EXP) + 1;
	return infoNumAlignmentSizedChunks << ALIGNMENT_EXP;
}

template<typename K, typename V, typename Descr>
uint64_t HashMap<K,V,Descr>::sizeOfKeyArray() const noexcept
{
	// Calculate how many aligment sized chunks is needed to store keys
	uint64_t keysMinRequiredSize = mCapacity * sizeof(K);
	uint64_t keyNumAlignmentSizedChunks = (keysMinRequiredSize >> ALIGNMENT_EXP) + 1;
	return keyNumAlignmentSizedChunks << ALIGNMENT_EXP;
}

template<typename K, typename V, typename Descr>
uint64_t HashMap<K,V,Descr>::sizeOfValueArray() const noexcept
{
	// Calculate how many alignment sized chunks is needed to store values
	uint64_t valuesMinRequiredSize = mCapacity * sizeof(V);
	uint64_t valuesNumAlignmentSizedChunks = (valuesMinRequiredSize >> ALIGNMENT_EXP) + 1;
	return valuesNumAlignmentSizedChunks << ALIGNMENT_EXP;
}

template<typename K, typename V, typename Descr>
uint64_t HashMap<K,V,Descr>::sizeOfAllocatedMemory() const noexcept
{
	return sizeOfElementInfoArray() + sizeOfKeyArray() + sizeOfValueArray();
}

template<typename K, typename V, typename Descr>
uint8_t* HashMap<K,V,Descr>::elementInfoPtr() const noexcept
{
	return mDataPtr;
}

template<typename K, typename V, typename Descr>
K* HashMap<K,V,Descr>::keysPtr() const noexcept
{
	return reinterpret_cast<K*>(mDataPtr + sizeOfElementInfoArray());
}

template<typename K, typename V, typename Descr>
V* HashMap<K,V,Descr>::valuesPtr() const noexcept
{
	return reinterpret_cast<V*>(mDataPtr + sizeOfElementInfoArray() + sizeOfKeyArray());
}

template<typename K, typename V, typename Descr>
uint8_t HashMap<K,V,Descr>::elementInfo(uint32_t index) const noexcept
{
	uint32_t chunkIndex = index >> 2; // index / 4;
	uint32_t chunkIndexModulo = index & 0x03; // index % 4
	uint32_t chunkIndexModuloTimes2 = chunkIndexModulo << 1;

	uint8_t chunk = elementInfoPtr()[chunkIndex];
	uint8_t info = static_cast<uint8_t>((chunk >> (chunkIndexModuloTimes2)) & 0x3);

	return info;
}

template<typename K, typename V, typename Descr>
void HashMap<K,V,Descr>::setElementInfo(uint32_t index, uint8_t value) noexcept
{
	uint32_t chunkIndex = index >> 2; // index / 4;
	uint32_t chunkIndexModulo = index & 0x03; // index % 4
	uint32_t chunkIndexModuloTimes2 = chunkIndexModulo << 1;

	uint8_t chunk = elementInfoPtr()[chunkIndex];

	// Remove previous info
	chunk = chunk & (~(uint32_t(0x03) << chunkIndexModuloTimes2));

	// Insert new info
	elementInfoPtr()[chunkIndex] =  uint8_t(chunk | (value << chunkIndexModuloTimes2));
}

template<typename K, typename V, typename Descr>
template<typename KT, typename Hash, typename Equal>
uint32_t HashMap<K,V,Descr>::findElementIndex(const KT& key, bool& elementFound,
                                                        uint32_t& firstFreeSlot, bool& isPlaceholder) const noexcept
{
	Hash keyHasher;
	Equal keyComparer;

	elementFound = false;
	firstFreeSlot = uint32_t(~0);
	isPlaceholder = false;
	K* const keys = keysPtr();

	// Early exit if HashMap has no capacity
	if (mCapacity == 0) return uint32_t(~0);

	// Hash the key and find the base index
	const int64_t baseIndex = int64_t(keyHasher(key) % size_t(mCapacity));

	// Check if base index holds the element
	uint8_t info = elementInfo(uint32_t(baseIndex));
	if (info == ELEMENT_INFO_EMPTY) {
		firstFreeSlot = uint32_t(baseIndex);
		return uint32_t(~0);
	}
	else if (info == ELEMENT_INFO_PLACEHOLDER) {
		firstFreeSlot = uint32_t(baseIndex);
		isPlaceholder = true;
	}
	else if (info == ELEMENT_INFO_OCCUPIED) {
		if (keyComparer(key, keys[baseIndex])) {
			elementFound = true;
			return uint32_t(baseIndex);
		}
	}

	// Search for the element using quadratic probing
	const int64_t maxNumProbingAttempts = int64_t(mCapacity);
	for (int64_t i = 1; i < maxNumProbingAttempts; ++i) {
		const int64_t iSquared = i * i;
		
		// Try (base + i²) index
		int64_t index = (baseIndex + iSquared) % int64_t(mCapacity);
		info = elementInfo(uint32_t(index));
		if (info == ELEMENT_INFO_EMPTY) {
			if (firstFreeSlot == uint32_t(~0)) firstFreeSlot = uint32_t(index);
			break;
		} else if (info == ELEMENT_INFO_PLACEHOLDER) {
			if (firstFreeSlot == uint32_t(~0)) {
				firstFreeSlot = uint32_t(index);
				isPlaceholder = true;
			}
		} else if (info == ELEMENT_INFO_OCCUPIED) {
			if (keyComparer(key, keys[index])) {
				elementFound = true;
				return uint32_t(index);
			}
		}

		// Try (base - i²) index
		index = (((baseIndex - iSquared) % int64_t(mCapacity)) + int64_t(mCapacity)) % int64_t(mCapacity);
		info = elementInfo(uint32_t(index));
		if (info == ELEMENT_INFO_EMPTY) {
			if (firstFreeSlot == uint32_t(~0)) firstFreeSlot = uint32_t(index);
			break;
		} else if (info == ELEMENT_INFO_PLACEHOLDER) {
			if (firstFreeSlot == uint32_t(~0)) {
				firstFreeSlot = uint32_t(index);
				isPlaceholder = true;
			}
		} else if (info == ELEMENT_INFO_OCCUPIED) {
			if (keyComparer(key, keys[index])) {
				elementFound = true;
				return uint32_t(index);
			}
		}
	}

	return uint32_t(~0);
}

template<typename K, typename V, typename Descr>
template<typename KT, typename Hash, typename Equal>
V* HashMap<K,V,Descr>::getInternal(const KT& key) const noexcept
{
	// Finds the index of the element
	uint32_t firstFreeSlot = uint32_t(~0);
	bool elementFound = false;
	bool isPlaceholder = false;
	uint32_t index = this->findElementIndex<KT,Hash,Equal>(key, elementFound, firstFreeSlot, isPlaceholder);

	// Returns nullptr if map doesn't contain element
	if (!elementFound) return nullptr;

	// Returns pointer to element
	return &(valuesPtr()[index]);
}

template<typename K, typename V, typename Descr>
template<typename KT, typename VT, typename Hash, typename Equal>
V& HashMap<K,V,Descr>::putInternal(KT&& key, VT&& value) noexcept
{
	// Utilizes perfect forwarding in order to determine if parameters are const references or rvalues.
	// const reference: KT == const K&
	// rvalue: KT == K
	// std::forward<KT>(key) will then return the correct version of key

	ensureProperlyHashed();

	// Finds the index of the element
	uint32_t firstFreeSlot = uint32_t(~0);
	bool elementFound = false;
	bool isPlaceholder = false;
	uint32_t index = this->findElementIndex<KT,Hash,Equal>(key, elementFound, firstFreeSlot, isPlaceholder);

	// If map contains key just replace value and return
	if (elementFound) {
		valuesPtr()[index] = std::forward<VT>(value);
		return valuesPtr()[index];
	}

	// Otherwise insert info, key and value
	setElementInfo(firstFreeSlot, ELEMENT_INFO_OCCUPIED);
	new (keysPtr() + firstFreeSlot) K(std::forward<KT>(key));
	new (valuesPtr() + firstFreeSlot) V(std::forward<VT>(value));

	mSize += 1;
	if (isPlaceholder) mPlaceholders -= 1;
	return valuesPtr()[firstFreeSlot];
}

template<typename K, typename V, typename Descr>
template<typename KT, typename Hash, typename Equal>
bool HashMap<K,V,Descr>::removeInternal(const KT& key) noexcept
{
	// Finds the index of the element
	uint32_t firstFreeSlot = uint32_t(~0);
	bool elementFound = false;
	bool isPlaceholder = false;
	uint32_t index = this->findElementIndex<KT,Hash,Equal>(key, elementFound, firstFreeSlot, isPlaceholder);

	// Returns nullptr if map doesn't contain element
	if (!elementFound) return false;

	// Remove element
	setElementInfo(index, ELEMENT_INFO_PLACEHOLDER);
	keysPtr()[index].~K();
	valuesPtr()[index].~V();

	mSize -= 1;
	mPlaceholders += 1;
	return true;
}

} // namespace sfz

#ifdef _WIN32
#pragma warning(pop)
#endif
