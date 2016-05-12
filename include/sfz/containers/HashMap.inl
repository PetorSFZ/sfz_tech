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
	mInfoBitsPtr = static_cast<uint8_t*>(Allocator::allocate(sizeofInfoBitsArray(), ALIGNMENT));
	mKeysPtr = static_cast<K*>(Allocator::allocate(mCapacity * sizeof(K), ALIGNMENT));
	mValuesPtr = static_cast<V*>(Allocator::allocate(mCapacity * sizeof(V), ALIGNMENT));
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>::HashMap(const HashMap& other) noexcept
{
	// TODO: Implement
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>& HashMap<K,V,HashFun,Allocator>::operator= (const HashMap& other) noexcept
{
	// TODO: Implement

}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>::HashMap(HashMap&& other) noexcept
{
	// TODO: Implement
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>& HashMap<K,V,HashFun,Allocator>::operator= (HashMap&& other) noexcept
{
	// TODO: Implement
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
HashMap<K,V,HashFun,Allocator>::~HashMap() noexcept
{
	this->destroy();
}

// HashMap (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
void HashMap<K,V,HashFun,Allocator>::clear() noexcept
{
	// Call destructor for all active keys and values if they are not trivially destructible
	if (!std::is_trivially_destructible<K>::value && !std::is_trivially_destructible<K>::value) {
		for (size_t i = 0; i < mCapacity; ++i) {
			if (elementInfo((uint32_t)i) == BIT_INFO_OCCUPIED) {
				mKeysPtr[i].~K();
				mValuesPtr[i].~V();
			}
		}
	}
	else if (!std::is_trivially_destructible<K>::value) {
		for (size_t i = 0; i < mCapacity; ++i) {
			if (elementInfo((uint32_t)i) == BIT_INFO_OCCUPIED) {
				mKeysPtr[i].~K();
			}
		}
	}
	else if (!std::is_trivially_destructible<V>::value) {
		for (size_t i = 0; i < mCapacity; ++i) {
			if (elementInfo((uint32_t)i) == BIT_INFO_OCCUPIED) {
				mValuesPtr[i].~V();
			}
		}
	}

	// Clear all info bits
	std::memset(mInfoBitsPtr, 0, sizeofInfoBitsArray());

	// Set size to 0
	mSize = 0;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
void HashMap<K,V,HashFun,Allocator>::destroy() noexcept
{
	if (mCapacity == 0) return;

	// Remove elements
	this->clear();

	// Deallocate memory
	Allocator::deallocate(mInfoBitsPtr);
	Allocator::deallocate(mKeysPtr);
	Allocator::deallocate(mValuesPtr);
	mCapacity = 0;
	mInfoBitsPtr = nullptr;
	mKeysPtr = nullptr;
	mValuesPtr = nullptr;
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
size_t HashMap<K,V,HashFun,Allocator>::sizeofInfoBitsArray() const noexcept
{
	// 4 bit pairs per byte
	uint32_t baseSize = mCapacity >> 2; 
	// + 1 since mCapacity is odd, since it is a prime
	return baseSize + 1;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
uint8_t HashMap<K,V,HashFun,Allocator>::elementInfo(uint32_t index) const noexcept
{
	uint32_t chunkIndex = index >> 2; // index / 4;
	uint32_t chunkIndexModulo = index & 0x03; // index % 4

	uint8_t chunk = mInfoBitsPtr[chunkIndex];
	uint8_t info = static_cast<uint8_t>((chunk >> (2 * chunkIndexModulo)) & 0x3);

	return info;
}

} // namespace sfz
