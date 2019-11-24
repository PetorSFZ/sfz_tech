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

#include <cstddef>
#include <cstring>
#include <functional>

#include <skipifzero.hpp>

namespace sfz {

// HashTableKeyDescriptor template
// ------------------------------------------------------------------------------------------------

struct NO_ALT_KEY_TYPE final { NO_ALT_KEY_TYPE() = delete; };

// Template used to describe how a key is hashed and compared with other keys in a hash table. Of
// special note is the possibility to define an alternate key type compatible with the main type.
// This is mainly useful when the key is a string class, in that case "const char*" can be defined
// as an alt key. This can improve performance of the hash table as temporary copies, which might
// require memory allocation, can be avoided.
//
// In order to be a HashTableKeyDescriptor the following typedefs need to be available:
// KeyT: The key type.
// KeyHash: A type with the same interface as std::hash which can hash a key.
//
// In addition the following typedefs used for the alternate key type needs to be available,
// if no alternate key type exist they MUST all be set to NO_ALT_KEY_TYPE.
// AltKeyT: An alternate key type compatible with KeyT.
// AltKeyHash: Key hasher but for AltKeyT, must produce same hash as KeyHash for equivalent keys
// (i.e. AltKeyKeyEqual says they are equal).
//
// In addition there is one additional constraint on an alt key type, it must be possible to
// construct a normal key with the alt key. I.e., the key type needs a Key(AltKey alt)
// constructor.
//
// The default implementation uses std::hash<K>. In other words, as long as std::hash is
// specialized and an equality (==) operator is defined the default HashTableKeyDescriptor should
// just work.
//
// If the default implementation is not enough (say if an alternate key type is wanted), this
// template can either be specialized (in namespace sfz) for a specific key type. Alternatively
// an equivalent struct (which fulfills all the constraints of a HashTableKeyDescriptor) can be
// defined elsewhere and used in its place.
template<typename K>
struct HashTableKeyDescriptor final {
	using KeyT = K;
	using KeyHash = std::hash<KeyT>;

	using AltKeyT = NO_ALT_KEY_TYPE;
	using AltKeyHash = NO_ALT_KEY_TYPE; // If specialized for alt key: std::hash<AltKeyT>
};

// HashMap (interface)
// ------------------------------------------------------------------------------------------------

// A HashMap with closed hashing (open adressing).
//
// Quadratic probing is used in the case of collisions, which can not guarantee that more than
// half the slots will be searched. For this reason the load factor is 49%, which should
// guarantee that this never poses a problem.
//
// The capacity of the the HashMap is always a prime number, so when a certain capacity is
// suggested a prime bigger than the suggestion will simply be taken from an internal lookup
// table. In the case of a rehash the capacity generally increases by (approximately) a factor
// of 2.
//
// Removal of elements is O(1), but will leave a placeholder on the previously occupied slot. The
// current number of placeholders can be queried by the placeholders() method. Both size and
// placeholders count as load when checking if the HashMap needs to be rehashed or not.
//
// An alternate key type can be specified in the HashTableKeyDescriptor. This alt key can be used
// in most methods instead of the normal key type. This is mostly useful when string classes are
// used as keys, then const char* can be used as an alt key type. This removes the need to create
// a temporary key object (which might need to allocate memory). As specified in the
// HashTableKeyDescriptor documentation, the normal key type needs to be constructable using an
// alt key.
//
// HashMap uses sfzCore allocators (read more about them in sfz/memory/Allocator.hpp). Basically
// they are instance based allocators, so each HashMap needs to have an Allocator pointer. The
// default constructor does not set any allocator (i.e. nullptr) and does not allocate any memory.
// An allocator can be set via the create() method or the constructor that takes an allocator.
// Once an allocator is set it can not be changed unless the HashMap is first destroy():ed, this
// is done automatically if create() is called again. If no allocator is available (nullptr) when
// attempting to allocate memory (rehash(), put(), etc), then the default allocator will be
// retrieved (getDefaultAllocator()) and set.
//
// \param K the key type
// \param V the value type
// \param Descr the HashTableKeyDescriptor (by default sfz::HashTableKeyDescriptor)
template<typename K, typename V, typename Descr = HashTableKeyDescriptor<K>>
class HashMap {
public:
	// Constants
	// --------------------------------------------------------------------------------------------

	static constexpr uint32_t ALIGNMENT_EXP = 5;
	static constexpr uint32_t ALIGNMENT = 1 << ALIGNMENT_EXP; // 2^5 = 32
	static constexpr uint32_t MIN_CAPACITY = 67;
	static constexpr uint32_t MAX_CAPACITY = 2147483659;

	// This factor decides the maximum number of occupied slots (size + placeholders) this
	// HashMap may contain before it is rehashed by ensureProperlyHashed().
	static constexpr float MAX_OCCUPIED_REHASH_FACTOR = 0.49f;

	// This factor decides the maximum size allowed to not increase the capacity when rehashing
	// in ensureProperlyHashed(). For example, size 20% and placeholders 30% would trigger a
	// rehash, but would not increase capacity. Size 40% and placeholders 10% would trigger a
	// rehash with capacity increase.
	static constexpr float MAX_SIZE_KEEP_CAPACITY_FACTOR = 0.35f;

	// Typedefs
	// --------------------------------------------------------------------------------------------

	using KeyHash = typename Descr::KeyHash;

	using AltK = typename Descr::AltKeyT;
	using AltKeyHash = typename Descr::AltKeyHash;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	HashMap() noexcept = default;
	HashMap(const HashMap& other) noexcept { *this = other; }
	HashMap& operator= (const HashMap& other) noexcept { *this = other.clone(); return *this; }
	HashMap(HashMap&& other) noexcept { this->swap(other); }
	HashMap& operator= (HashMap&& other) noexcept { this->swap(other); return *this; }
	~HashMap() noexcept { this->destroy(); }

	HashMap(uint32_t suggestedCapacity, Allocator* allocator) noexcept { this->init(suggestedCapacity, allocator); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(uint32_t suggestedCapacity, Allocator* allocator) noexcept
	{
		this->destroy();
		mAllocator = allocator;
		this->rehash(suggestedCapacity);
	}

	HashMap clone(DbgInfo allocDbg = sfz_dbg("HashMapDynamic"), Allocator* allocator = nullptr) const noexcept
	{
		HashMap tmp(mCapacity, allocator != nullptr ? allocator : mAllocator);
		for (auto pair : *this) {
			tmp.put(pair.key, pair.value);
		}
		return tmp;
	}

	// Swaps the contents of two HashMaps, including the allocators.
	void swap(HashMap& other) noexcept
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

	// Destroys all elements stored in this HashMap, deallocates all memory and removes allocator.
	// After this method is called the size and capacity is 0, allocator is nullptr. If the HashMap
	// is already empty then this method will only remove the allocator if it exists. It is not
	// necessary to call this method manually, it will automatically be called in the destructor.
	void destroy() noexcept
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

	// Removes all elements from this HashMap without deallocating memory, changing capacity or
	// touching the allocator.
	void clear() noexcept
	{
		if (mSize == 0) return;

		// Call destructors for all active keys and values if they are not trivially destructible
		if constexpr (!std::is_trivially_destructible<K>::value || !std::is_trivially_destructible<V>::value) {
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

	// Rehashes this HashMap. Creates a new HashMap with at least the same capacity as the
	// current one, or larger if suggested by suggestedCapacity. Then iterates over all elements
	// in this HashMap and adds them to the new one. Finally this HashMap is replaced by the
	// new one. Obviously all pointers and references into the old HashMap are invalidated. If no
	// allocator is set then the default one will be retrieved and set.
	void rehash(uint32_t suggestedCapacity) noexcept
	{
		// Can't decrease capacity with rehash()
		if (suggestedCapacity < mCapacity) suggestedCapacity = mCapacity;

		// Don't rehash if capacity already exists and there are no placeholders
		if (suggestedCapacity == mCapacity && mPlaceholders == 0) return;

		// Convert the suggested capacity to a larger (if possible) prime number
		uint32_t newCapacity = findPrimeCapacity(suggestedCapacity);

		sfz_assert_hard(mAllocator != nullptr);

		// Create a new HashMap, set allocator and allocate memory to it
		HashMap tmp;
		tmp.mCapacity = newCapacity;
		tmp.mAllocator = mAllocator;
		tmp.mDataPtr =
			(uint8_t*)mAllocator->allocate(sfz_dbg("HashMap"), tmp.sizeOfAllocatedMemory(), ALIGNMENT);
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

	// Checks if HashMap needs to be rehashed, and will do so if necessary. This method is
	// internally called by put() and operator[]. Will allocate capacity if this HashMap is
	// empty. Returns whether HashMap was rehashed.
	bool ensureProperlyHashed() noexcept
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

	// Getters
	// --------------------------------------------------------------------------------------------

	// Returns the size of this HashMap. This is the number of elements stored, not the current
	// capacity.
	uint32_t size() const noexcept { return mSize; }

	// Returns the capacity of this HashMap.
	uint32_t capacity() const noexcept { return mCapacity; }

	// Returns the number of placeholder positions for removed elements. size + placeholders <=
	// capacity.
	uint32_t placeholders() const noexcept { return mPlaceholders; }

	// Returns the allocator of this HashMap. Will return nullptr if no allocator is set.
	Allocator* allocator() const noexcept { return mAllocator; }

	// Returns pointer to the element associated with the given key, or nullptr if no such element
	// exists. The pointer is owned by this HashMap and will not be valid if it is rehashed,
	// which can automatically occur if for example keys are inserted via put() or operator[].
	// Instead it is recommended to make a copy of the returned value. This method is guaranteed
	// to never change the state of the HashMap (by causing a rehash), the pointer returned can
	// however be used to modify the stored value for an element.
	V* get(const K& key) noexcept { return this->getInternal<K, KeyHash>(key); }
	const V* get(const K& key) const noexcept { return this->getInternal<K, KeyHash>(key); }
	V* get(const AltK& key) noexcept { return this->getInternal<AltK, AltKeyHash>(key); }
	const V* get(const AltK& key) const noexcept { return this->getInternal<AltK, AltKeyHash>(key); }

	// Public methods
	// --------------------------------------------------------------------------------------------

	// Adds the specified key value pair to this HashMap. If a value is already associated with
	// the given key it will be replaced with the new value. Returns a reference to the element
	// set. As usual, the reference will be invalidated if the HashMap is rehashed, so be careful.
	// This method will always call ensureProperlyHashed(), which might trigger a rehash.
	//
	// In particular the following scenario presents a dangerous trap:
	// V& ref1 = m.put(key1, value1);
	// V& ref2 = m.put(key2, value2);
	// At this point only ref2 is guaranteed to be valid, as the second call might have triggered
	// a rehash. In this particular example consider ignoring the reference returned and instead
	// retrieve pointers via the get() method (which is guaranteed to not cause a rehash) after
	// all the keys have been inserted.
	V& put(const K& key, const V& value) noexcept { return this->putInternal<const K&, const V&, KeyHash>(key, value); }
	V& put(const K& key, V&& value) noexcept { return this->putInternal<const K&, V, KeyHash>(key, std::move(value)); }
	V& put(K&& key, const V& value) noexcept { return this->putInternal<K, const V&, KeyHash>(std::move(key), value); }
	V& put(K&& key, V&& value) noexcept { return this->putInternal<K, V, KeyHash>(std::move(key), std::move(value)); }
	V& put(const AltK& key, const V& value) noexcept { return this->putInternal<const AltK&, const V&, AltKeyHash>(key, value); }
	V& put(const AltK& key, V&& value) noexcept { return this->putInternal<const AltK&, V, AltKeyHash>(key, std::move(value)); }

	// Access operator, will return a reference to the element associated with the given key. If
	// no such element exists it will be created with the default constructor. This method is
	// implemented by a call to get(), and then a call to put() if no element existed. In
	// practice this means that this function is guaranteed to not rehash if the requested
	// element already exists. This might be dangerous to rely on, so get() should be preferred
	// if rehashing needs to be avoided. As always, the reference will be invalidated if the
	// HashMap is rehashed.
	V& operator[] (const K& key) noexcept
	{
		V* ptr = this->get(key);
		if (ptr != nullptr) return *ptr;
		return this->put(key, V());
	}
	V& operator[] (K&& key) noexcept
	{
		V* ptr = this->get(key);
		if (ptr != nullptr) return *ptr;
		return this->put(std::move(key), V());
	}
	V& operator[] (const AltK& key) noexcept
	{
		V* ptr = this->get(key);
		if (ptr != nullptr) return *ptr;
		return this->put(key, V());
	}

	// Attempts to remove the element associated with the given key. Returns false if this
	// HashMap contains no such element. Guaranteed to not rehash.
	bool remove(const K& key) noexcept { return this->removeInternal<K, KeyHash>(key); }
	bool remove(const AltK& key) noexcept { return this->removeInternal<AltK, AltKeyHash>(key); }

	// Iterators
	// --------------------------------------------------------------------------------------------

	// The return value when dereferencing an iterator. Contains references into the HashMap in
	// question, so it is only valid as long as no rehashing is performed.
	struct KeyValuePair final {
		const K& key; // Const so user doesn't change key, breaking invariants of the HashMap
		V& value;
		KeyValuePair(const K& key, V& value) noexcept : key(key), value(value) { }
		KeyValuePair(const KeyValuePair&) noexcept = default;
		KeyValuePair& operator= (const KeyValuePair&) = delete; // Because references...
	};

	// The normal non-const iterator for HashMap.
	class Iterator final {
	public:
		Iterator(HashMap& hashMap, uint32_t index) noexcept : mHashMap(&hashMap), mIndex(index) { }
		Iterator(const Iterator&) noexcept = default;
		Iterator& operator= (const Iterator&) noexcept = default;

		Iterator& operator++ () noexcept // Pre-increment
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

		Iterator operator++ (int) noexcept // Post-increment
		{
			auto copy = *this;
			++(*this);
			return copy;
		}

		KeyValuePair operator* () noexcept
		{
			sfz_assert(mIndex != uint32_t(~0));
			sfz_assert(mHashMap->elementInfo(mIndex) == ELEMENT_INFO_OCCUPIED);
			return KeyValuePair(mHashMap->keysPtr()[mIndex], mHashMap->valuesPtr()[mIndex]);
		}

		bool operator== (const Iterator& other) const noexcept
		{
			return (this->mHashMap == other.mHashMap) && (this->mIndex == other.mIndex);
		}

		bool operator!= (const Iterator& other) const noexcept { return !(*this == other); }

	private:
		HashMap* mHashMap;
		uint32_t mIndex;
	};

	// The return value when dereferencing a const iterator. Contains references into the HashMap
	// in question, so it is only valid as long as no rehashing is performed.
	struct ConstKeyValuePair final {
		const K& key;
		const V& value;
		ConstKeyValuePair(const K& key, const V& value) noexcept : key(key), value(value) {}
		ConstKeyValuePair(const ConstKeyValuePair&) noexcept = default;
		ConstKeyValuePair& operator= (const ConstKeyValuePair&) = delete; // Because references...
	};

	// The const iterator for HashMap
	class ConstIterator final {
	public:
		ConstIterator(const HashMap& hashMap, uint32_t index) noexcept : mHashMap(&hashMap), mIndex(index) {}
		ConstIterator(const ConstIterator&) noexcept = default;
		ConstIterator& operator= (const ConstIterator&) noexcept = default;

		ConstIterator& operator++ () noexcept // Pre-increment
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

		ConstIterator operator++ (int) noexcept // Post-increment
		{
			auto copy = *this;
			++(*this);
			return copy;
		}

		ConstKeyValuePair operator* () noexcept
		{
			sfz_assert(mIndex != uint32_t(~0));
			sfz_assert(mHashMap->elementInfo(mIndex) == ELEMENT_INFO_OCCUPIED);
			return ConstKeyValuePair(mHashMap->keysPtr()[mIndex], mHashMap->valuesPtr()[mIndex]);
		}

		bool operator== (const ConstIterator& other) const noexcept
		{
			return (this->mHashMap == other.mHashMap) && (this->mIndex == other.mIndex);
		}

		bool operator!= (const ConstIterator& other) const noexcept { return !(*this == other); }

	private:
		const HashMap* mHashMap;
		uint32_t mIndex;
	};

	// Iterator methods
	// --------------------------------------------------------------------------------------------

	Iterator begin() noexcept
	{
		if (this->size() == 0) return Iterator(*this, uint32_t(~0));
		Iterator it(*this, 0);
		// Unless there happens to be an element in slot 0 we increment the iterator to find it
		if (elementInfo(uint32_t(0)) != ELEMENT_INFO_OCCUPIED) {
			++it;
		}
		return it;
	}

	ConstIterator begin() const noexcept { return cbegin(); }

	ConstIterator cbegin() const noexcept
	{
		if (this->size() == 0) return ConstIterator(*this, uint32_t(~0));
		ConstIterator it(*this, 0);
		// Unless there happens to be an element in slot 0 we increment the iterator to find it
		if (elementInfo(uint32_t(0)) != ELEMENT_INFO_OCCUPIED) {
			++it;
		}
		return it;
	}

	Iterator end() noexcept { return Iterator(*this, uint32_t(~0)); }
	ConstIterator end() const noexcept { return cend(); }
	ConstIterator cend() const noexcept { return ConstIterator(*this, uint32_t(~0)); }

private:
	// Private constants
	// --------------------------------------------------------------------------------------------

	static constexpr uint8_t ELEMENT_INFO_EMPTY = 0;
	static constexpr uint8_t ELEMENT_INFO_PLACEHOLDER = 1;
	static constexpr uint8_t ELEMENT_INFO_OCCUPIED = 2;

	// Private methods
	// --------------------------------------------------------------------------------------------

	// Returns a prime number larger than the suggested capacity
	uint32_t findPrimeCapacity(uint32_t capacity) const noexcept
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

	// Return the size of the memory allocation for the element info array in bytes
	uint64_t sizeOfElementInfoArray() const noexcept
	{
		// 2 bits per info element, + 1 since mCapacity always is odd.
		uint64_t infoMinRequiredSize = (mCapacity >> 2) + 1;

		// Calculate how many alignment sized chunks is needed to store element info
		uint64_t infoNumAlignmentSizedChunks = (infoMinRequiredSize >> ALIGNMENT_EXP) + 1;
		return infoNumAlignmentSizedChunks << ALIGNMENT_EXP;
	}

	// Returns the size of the memory allocation for the key array in bytes
	uint64_t sizeOfKeyArray() const noexcept
	{
		// Calculate how many aligment sized chunks is needed to store keys
		uint64_t keysMinRequiredSize = mCapacity * sizeof(K);
		uint64_t keyNumAlignmentSizedChunks = (keysMinRequiredSize >> ALIGNMENT_EXP) + 1;
		return keyNumAlignmentSizedChunks << ALIGNMENT_EXP;
	}

	// Returns the size of the memory allocation for the value array in bytes
	uint64_t sizeOfValueArray() const noexcept
	{
		// Calculate how many alignment sized chunks is needed to store values
		uint64_t valuesMinRequiredSize = mCapacity * sizeof(V);
		uint64_t valuesNumAlignmentSizedChunks = (valuesMinRequiredSize >> ALIGNMENT_EXP) + 1;
		return valuesNumAlignmentSizedChunks << ALIGNMENT_EXP;
	}

	// Returns the size of the allocated memory in bytes
	uint64_t sizeOfAllocatedMemory() const noexcept
	{
		return sizeOfElementInfoArray() + sizeOfKeyArray() + sizeOfValueArray();
	}

	// Returns pointer to the info bits part of the allocated memory
	uint8_t* elementInfoPtr() const noexcept { return mDataPtr; }

	// Returns pointer to the key array part of the allocated memory
	K* keysPtr() const noexcept
	{
		return reinterpret_cast<K*>(mDataPtr + sizeOfElementInfoArray());
	}

	// Returns pointer to the value array port fo the allocated memory
	V* valuesPtr() const noexcept
	{
		return reinterpret_cast<V*>(mDataPtr + sizeOfElementInfoArray() + sizeOfKeyArray());
	}

	// Returns the 2 bit element info about an element position in the HashMap
	// 0 = empty, 1 = removed, 2 = occupied, (3 is unused)
	uint8_t elementInfo(uint32_t index) const noexcept
	{
		uint32_t chunkIndex = index >> 2; // index / 4;
		uint32_t chunkIndexModulo = index & 0x03; // index % 4
		uint32_t chunkIndexModuloTimes2 = chunkIndexModulo << 1;

		uint8_t chunk = elementInfoPtr()[chunkIndex];
		uint8_t info = static_cast<uint8_t>((chunk >> (chunkIndexModuloTimes2)) & 0x3);

		return info;
	}

	// Sets the 2 bit element info with the selected value
	void setElementInfo(uint32_t index, uint8_t value) noexcept
	{
		uint32_t chunkIndex = index >> 2; // index / 4;
		uint32_t chunkIndexModulo = index & 0x03; // index % 4
		uint32_t chunkIndexModuloTimes2 = chunkIndexModulo << 1;

		uint8_t chunk = elementInfoPtr()[chunkIndex];

		// Remove previous info
		chunk = chunk & (~(uint32_t(0x03) << chunkIndexModuloTimes2));

		// Insert new info
		elementInfoPtr()[chunkIndex] = uint8_t(chunk | (value << chunkIndexModuloTimes2));
	}

	// Finds the index of an element associated with the specified key. Whether an element is
	// found or not is returned through the elementFound parameter. The first free slot found is
	// sent back through the firstFreeSlot parameter, if no free slot is found it will be set to
	// ~0. Whether the found free slot is a placeholder slot or not is sent back through the
	// isPlaceholder parameter.
	// KT: The key type, either K or AltK
	// KeyHash: Hasher for KeyT
	template<typename KT, typename Hash>
	uint32_t findElementIndex(const KT& key, bool& elementFound, uint32_t& firstFreeSlot,
	                          bool& isPlaceholder) const noexcept
	{
		Hash keyHasher;

		elementFound = false;
		firstFreeSlot = uint32_t(~0);
		isPlaceholder = false;
		K* const keys = keysPtr();

		// Early exit if HashMap has no capacity
		if (mCapacity == 0) return uint32_t(~0);

		// Hash the key and find the base index
		const int64_t baseIndex = int64_t(keyHasher(key) % uint64_t(mCapacity));

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
			if (keys[baseIndex] == key) {
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
			}
			else if (info == ELEMENT_INFO_PLACEHOLDER) {
				if (firstFreeSlot == uint32_t(~0)) {
					firstFreeSlot = uint32_t(index);
					isPlaceholder = true;
				}
			}
			else if (info == ELEMENT_INFO_OCCUPIED) {
				if (keys[index] == key) {
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
			}
			else if (info == ELEMENT_INFO_PLACEHOLDER) {
				if (firstFreeSlot == uint32_t(~0)) {
					firstFreeSlot = uint32_t(index);
					isPlaceholder = true;
				}
			}
			else if (info == ELEMENT_INFO_OCCUPIED) {
				if (keys[index] == key) {
					elementFound = true;
					return uint32_t(index);
				}
			}
		}

		return uint32_t(~0);
	}

	// Internal shared implementation of all get() methods
	template<typename KT, typename Hash>
	V* getInternal(const KT& key) const noexcept
	{
		// Finds the index of the element
		uint32_t firstFreeSlot = uint32_t(~0);
		bool elementFound = false;
		bool isPlaceholder = false;
		uint32_t index = this->findElementIndex<KT, Hash>(key, elementFound, firstFreeSlot, isPlaceholder);

		// Returns nullptr if map doesn't contain element
		if (!elementFound) return nullptr;

		// Returns pointer to element
		return &(valuesPtr()[index]);
	}

	// Internal shared implementation of all put() methods
	template<typename KT, typename VT, typename Hash>
	V& putInternal(KT&& key, VT&& value) noexcept
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
		uint32_t index = this->findElementIndex<KT, Hash>(key, elementFound, firstFreeSlot, isPlaceholder);

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

	// Internal shared implementation of all remove() methods
	template<typename KT, typename Hash>
	bool removeInternal(const KT& key) noexcept
	{
		// Finds the index of the element
		uint32_t firstFreeSlot = uint32_t(~0);
		bool elementFound = false;
		bool isPlaceholder = false;
		uint32_t index = this->findElementIndex<KT, Hash>(key, elementFound, firstFreeSlot, isPlaceholder);

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

	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mSize = 0, mCapacity = 0, mPlaceholders = 0;
	uint8_t* mDataPtr = nullptr;
	Allocator* mAllocator = nullptr;
};

} // namespace sfz
