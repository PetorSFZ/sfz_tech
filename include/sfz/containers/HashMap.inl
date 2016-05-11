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
HashMap<K,V,HashFun,Allocator>::HashMap(uint32_t capacityExponent) noexcept
{
	if (capacityExponent < MIN_EXPONENT) capacityExponent = MIN_EXPONENT;
	else if (capacityExponent > MAX_EXPONENT) capacityExponent = MAX_EXPONENT;
	
	mCapacity = uint32_t(1) << capacityExponent; // 2^capacityExponent
	// Allocate 2 bits per element, i.e. mCapacity / 4 bytes.
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
size_t HashMap<K,V,HashFun,Allocator>::sizeofInfoBitsArray() const noexcept
{
	return (size_t)mCapacity / 4;
}

template<typename K, typename V, size_t(*HashFun)(const K&), typename Allocator>
uint8_t HashMap<K,V,HashFun,Allocator>::elementInfo(uint32_t index) const noexcept
{
	uint32_t chunkIndex = index >> 2; // index / 4;
	uint32_t chunkIndexModulo = index & 0x03; // index % 4

	uint8_t chunk = mInfoBitsPtr[chunkIndex];
	uint8_t info = (chunk >> (2 * chunkIndexModulo)) & 0x3;

	return info;
}

} // namespace sfz
