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

// HashMap (implementation): Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>::HashMap(uint32_t suggestedCapacity) noexcept
{
	// Convert the suggested capacity to a larger (if possible) prime number
	mCapacity = findPrimeCapacity(suggestedCapacity);

	// Allocate memory
	mDataPtr = static_cast<uint8_t*>(Allocator::allocate(sizeOfAllocatedMemory(), ALIGNMENT));
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>::HashMap(const HashMap& other) noexcept
{
	*this = other;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>& HashMap<K,V,HashFun,Allocator>::operator= (const HashMap& other) noexcept
{
	// TODO: Implement
	return *nullptr;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>::HashMap(HashMap&& other) noexcept
{
	this->swap(other);
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>& HashMap<K,V,HashFun,Allocator>::operator= (HashMap&& other) noexcept
{
	this->swap(other)
	return *this;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>::~HashMap() noexcept
{
	this->destroy();
}

// HashMap (implementation): Getters
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
V* HashMap<K,V,HashFun,Allocator>::get(const K& key) noexcept
{
	// Hash the key and the probe to find empty index
	uint32_t index = uint32_t(HashFun(key) % size_t(mCapacity));
	uint8_t info = elementInfo(index);

	K* keys = keysPtr();

	for (size_t i = 0; i < size_t(mCapacity); ++i) {
		// Return nullptr if there is no element at index
		if (info <= BIT_INFO_REMOVED) return nullptr;

		// Return pointer to value if it is the correct key
		if (keys[index] == key) return &(valuesPtr()[index]);

		// Try another index (linear probing)
		index += 1;
		info = elementInfo(index);
	}

	// Timed out
	return nullptr;
}

/*template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
const V* HashMap<K,V,HashFun,Allocator>::get(const K& key) const noexcept
{
	// Hash the key and the probe to find empty index
	uint32_t index = uint32_t(HashFun(key) % size_t(mCapacity));
	uint8_t info = elementInfo(index);

	K* keys = keysPtr();

	for (size_t i = 0; i < size_t(mCapacity); ++i) {
		// Return nullptr if there is no element at index
		if (info <= BIT_INFO_REMOVED) return nullptr;

		// Return pointer to value if it is the correct key
		if (keys[index] == key) return &valuesPtr()[index];

		// Try another index (linear probing)
		index += 1;
		info = elementInfo(index);
	}

	// Timed out
	return nullptr;
}*/

// HashMap (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
void HashMap<K,V,HashFun,Allocator>::add(const K& key, const V& value) noexcept
{
	if (mSize >= mCapacity) {
		// TODO: Create more capacity
		return;
	}

	// TODO: Maybe bool return value?

	// Hash the key and the probe to find empty index
	uint32_t index = uint32_t(HashFun(key) % size_t(mCapacity));
	uint8_t info = elementInfo(index);

	// TODO: Change to quadratic probe
	while (info >= BIT_INFO_OCCUPIED) {
		index += 1;
		info = elementInfo(index);
	}

	// Insert info, key and value
	setElementInfo(index, BIT_INFO_OCCUPIED);
	new (keysPtr() + index) K(key);
	new (valuesPtr() + index) V(value);
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
void HashMap<K,V,HashFun,Allocator>::swap(HashMap& other) noexcept
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

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
void HashMap<K,V,HashFun,Allocator>::clear() noexcept
{
	if (mSize == 0) return;

	// Call destructor for all active keys and values if they are not trivially destructible
	if (!std::is_trivially_destructible<K>::value && !std::is_trivially_destructible<K>::value) {
		K* keyPtr = keysPtr();
		V* valuePtr = valuesPtr();
		for (size_t i = 0; i < mCapacity; ++i) {
			if (elementInfo((uint32_t)i) == BIT_INFO_OCCUPIED) {
				keyPtr[i].~K();
				valuePtr[i].~V();
			}
		}
	}
	else if (!std::is_trivially_destructible<K>::value) {
		K* keyPtr = keysPtr();
		for (size_t i = 0; i < mCapacity; ++i) {
			if (elementInfo((uint32_t)i) == BIT_INFO_OCCUPIED) {
				keyPtr[i].~K();
			}
		}
	}
	else if (!std::is_trivially_destructible<V>::value) {
		V* valuePtr = valuesPtr();
		for (size_t i = 0; i < mCapacity; ++i) {
			if (elementInfo((uint32_t)i) == BIT_INFO_OCCUPIED) {
				valuePtr[i].~V();
			}
		}
	}

	// Clear all element info bits
	std::memset(elementInfoPtr(), 0, sizeOfElementInfoArray());

	// Set size to 0
	mSize = 0;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
void HashMap<K,V,HashFun,Allocator>::destroy() noexcept
{
	if (mDataPtr == nullptr) return;

	// Remove elements
	this->clear();

	// Deallocate memory
	Allocator::deallocate(mDataPtr);
	mCapacity = 0;
	mDataPtr = nullptr;
}

// HashMap (implementation): Private methods
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
uint32_t HashMap<K,V,HashFun,Allocator>::findPrimeCapacity(uint32_t capacity) const noexcept
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
	for (size_t i = 0; i < sizeof(PRIMES) / sizeof(uint32_t); ++i) {
		if (PRIMES[i] >= capacity) return PRIMES[i];
	}

	// Found no prime, which means that the suggested capacity is too large.
	return MAX_CAPACITY;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
size_t HashMap<K,V,HashFun,Allocator>::sizeOfElementInfoArray() const noexcept
{
	// 2 bits per info element, + 1 since mCapacity always is odd.
	size_t infoMinRequiredSize = (mCapacity >> 2) + 1;

	// Calculate how many alignment sized chunks is needed to store element info
	size_t infoNumAlignmentSizedChunks = (infoMinRequiredSize >> ALIGNMENT_EXP) + 1;
	return infoNumAlignmentSizedChunks << ALIGNMENT_EXP;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
size_t HashMap<K,V,HashFun,Allocator>::sizeOfKeyArray() const noexcept
{
	// Calculate how many aligment sized chunks is needed to store keys
	size_t keysMinRequiredSize = mCapacity * sizeof(K);
	size_t keyNumAlignmentSizedChunks = (keysMinRequiredSize >> ALIGNMENT_EXP) + 1;
	return keyNumAlignmentSizedChunks << ALIGNMENT_EXP;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
size_t HashMap<K,V,HashFun,Allocator>::sizeOfValueArray() const noexcept
{
	// Calculate how many alignment sized chunks is needed to store values
	size_t valuesMinRequiredSize = mCapacity * sizeof(V);
	size_t valuesNumAlignmentSizedChunks = (valuesMinRequiredSize >> ALIGNMENT_EXP) + 1;
	return valuesNumAlignmentSizedChunks << ALIGNMENT_EXP;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
size_t HashMap<K,V,HashFun,Allocator>::sizeOfAllocatedMemory() const noexcept
{
	return sizeOfElementInfoArray() + sizeOfKeyArray() + sizeOfValueArray();
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
uint8_t* HashMap<K,V,HashFun,Allocator>::elementInfoPtr() const noexcept
{
	return mDataPtr;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
K* HashMap<K,V,HashFun,Allocator>::keysPtr() const noexcept
{
	return reinterpret_cast<K*>(mDataPtr + sizeOfElementInfoArray());
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
V* HashMap<K,V,HashFun,Allocator>::valuesPtr() const noexcept
{
	return reinterpret_cast<V*>(mDataPtr + sizeOfElementInfoArray() + sizeOfKeyArray());
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
uint8_t HashMap<K,V,HashFun,Allocator>::elementInfo(uint32_t index) const noexcept
{
	uint32_t chunkIndex = index >> 2; // index / 4;
	uint32_t chunkIndexModulo = index & 0x03; // index % 4
	uint32_t chunkIndexModuloTimes2 = chunkIndexModulo << 1;

	uint8_t chunk = elementInfoPtr()[chunkIndex];
	uint8_t info = static_cast<uint8_t>((chunk >> (chunkIndexModuloTimes2)) & 0x3);

	return info;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
void HashMap<K,V,HashFun,Allocator>::setElementInfo(uint32_t index, uint8_t value) noexcept
{
	uint32_t chunkIndex = index >> 2; // index / 4;
	uint32_t chunkIndexModulo = index & 0x03; // index % 4
	uint32_t chunkIndexModuloTimes2 = chunkIndexModulo << 1;

	uint8_t chunk = elementInfoPtr()[chunkIndex];
	
	// Remove previous info
	chunk = chunk & (~(uint32_t(0x03) << chunkIndexModuloTimes2));
	
	// Insert new info
	elementInfoPtr()[chunkIndex] =  chunk | (value << chunkIndexModuloTimes2);
}

} // namespace sfz
