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

#include <skipifzero.hpp>

namespace sfz {

// sfz::hash
// ------------------------------------------------------------------------------------------------

constexpr uint64_t hash(uint8_t value) noexcept { return uint64_t(value); }
constexpr uint64_t hash(uint16_t value) noexcept { return uint64_t(value); }
constexpr uint64_t hash(uint32_t value) noexcept { return uint64_t(value); }
constexpr uint64_t hash(uint64_t value) noexcept { return uint64_t(value); }

constexpr uint64_t hash(int16_t value) noexcept { return uint64_t(value); }
constexpr uint64_t hash(int32_t value) noexcept { return uint64_t(value); }
constexpr uint64_t hash(int64_t value) noexcept { return uint64_t(value); }

constexpr uint64_t hash(float value) noexcept { return uint64_t(*((const uint32_t*)&value)); }
constexpr uint64_t hash(double value) noexcept { return *((const uint64_t*)&value); }

constexpr uint64_t hash(void* value) noexcept { return uint64_t(uintptr_t(value)); }

// HashMapAltKeyDescr
// ------------------------------------------------------------------------------------------------

struct NO_ALT_KEY_TYPE final { NO_ALT_KEY_TYPE() = delete; };

// Alternative key type for a given type, useful for e.g. string types so "const char*" can be
// defined as an alternate key type.
//
// Requirements:
//  * operator== (KeyT, AltKeyT) must be defined
//  * sfz::hash(KeyT) == sfz::hash(HashMapAltKey<KeyT>::AltKeyT)
//  * constructor KeyT(AltKeyT) must be defined
template<typename KeyT>
struct HashMapAltKeyDescr final {
	using AltKeyT = NO_ALT_KEY_TYPE;
};

// HashMapDynamic
// ------------------------------------------------------------------------------------------------

// A HashMap with closed hashing (open adressing) and linear probing.
//
// Removal of elements is O(1), but will leave a placeholder on the previously occupied slot. The
// current number of placeholders can be queried by the placeholders() method. Both size and
// placeholders count as load when checking if the HashMap needs to be rehashed or not.
//
// An alternate key type can be specified in the HashMapAltKeyDescr. This is mostly useful when
// string classes are used as keys, then const char* can be used as an alt key type. This removes
// the need to create a temporary key object (which might need to allocate memory).
template<typename K, typename V, typename AltKeyDescr = HashMapAltKeyDescr<K>>
class HashMapDynamic {
public:
	// Constants and typedefs
	// --------------------------------------------------------------------------------------------

	using AltK = typename AltKeyDescr::AltKeyT;

	static constexpr uint32_t ALIGNMENT_EXP = 5;
	static constexpr uint32_t ALIGNMENT = 1 << ALIGNMENT_EXP; // 2^5 = 32
	static constexpr uint32_t MIN_CAPACITY = 67;
	static constexpr uint32_t MAX_CAPACITY = 2147483659;

	static constexpr uint32_t DEFAULT_INITIAL_CAPACITY = 64;
	static constexpr float MAX_OCCUPIED_REHASH_FACTOR = 0.80f;
	static constexpr float GROW_RATE = 1.75f;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	HashMapDynamic() noexcept = default;
	HashMapDynamic(const HashMapDynamic& other) noexcept { *this = other; }
	HashMapDynamic& operator= (const HashMapDynamic& other) noexcept { *this = other.clone(); return *this; }
	HashMapDynamic(HashMapDynamic&& other) noexcept { this->swap(other); }
	HashMapDynamic& operator= (HashMapDynamic&& other) noexcept { this->swap(other); return *this; }
	~HashMapDynamic() noexcept { this->destroy(); }

	HashMapDynamic(uint32_t suggestedCapacity, Allocator* allocator, DbgInfo allocDbg) noexcept
	{
		this->init(suggestedCapacity, allocator, allocDbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(uint32_t capacity, Allocator* allocator, DbgInfo allocDbg)
	{
		this->destroy();
		mAllocator = allocator;
		this->rehash(capacity, allocDbg);
	}

	HashMapDynamic clone(DbgInfo allocDbg = sfz_dbg("HashMapDynamic"), Allocator* allocator = nullptr) const
	{
		HashMapDynamic tmp(mCapacity, allocator != nullptr ? allocator : mAllocator, allocDbg);
		for (auto pair : *this) {
			tmp.put(pair.key, pair.value);
		}
		return tmp;
	}

	// Swaps the contents of two HashMaps, including the allocators.
	void swap(HashMapDynamic& other)
	{
		std::swap(this->mSize, other.mSize);
		std::swap(this->mCapacity, other.mCapacity);
		std::swap(this->mPlaceholders, other.mPlaceholders);
		std::swap(this->mData, other.mData);
		std::swap(this->mAllocator, other.mAllocator);
	}

	// Destroys all elements stored in this HashMap, deallocates all memory and removes allocator.
	// After this method is called the size and capacity is 0, allocator is nullptr. If the HashMap
	// is already empty then this method will only remove the allocator if it exists. It is not
	// necessary to call this method manually, it will automatically be called in the destructor.
	void destroy()
	{
		if (mData == nullptr) {
			mAllocator = nullptr;
			return;
		}

		// Remove elements
		this->clear();

		// Deallocate memory
		mAllocator->deallocate(mData);
		mCapacity = 0;
		mPlaceholders = 0;
		mData = nullptr;
		mAllocator = nullptr;
	}

	// Removes all elements from this HashMap without deallocating memory, changing capacity or
	// touching the allocator.
	void clear()
	{
		if (mSize == 0) return;

		// Call destructors for all active keys and values
		K* keyPtr = keysPtr();
		V* valuePtr = valuesPtr();
		for (uint32_t i = 0; i < mCapacity; ++i) {
			if (elementInfo(i) == ELEMENT_INFO_OCCUPIED) {
				keyPtr[i].~K();
				valuePtr[i].~V();
			}
		}

		// Clear all element info bits
		std::memset(elementInfoPtr(), 0, sizeOfElementInfoArray());

		// Set size to 0
		mSize = 0;
		mPlaceholders = 0;
	}

	// Rehashes this HashMap to the specified capacity. All old pointers and references are invalidated.
	void rehash(uint32_t newCapacity, DbgInfo allocDbg)
	{
		// Can't decrease capacity with rehash()
		if (newCapacity < mCapacity) newCapacity = mCapacity;

		// Don't rehash if capacity already exists and there are no placeholders
		if (newCapacity == mCapacity && mPlaceholders == 0) return;

		sfz_assert_hard(mAllocator != nullptr);

		// Create a new HashMap, set allocator and allocate memory to it
		HashMapDynamic tmp;
		tmp.mCapacity = newCapacity;
		tmp.mAllocator = mAllocator;
		tmp.mData =
			(uint8_t*)mAllocator->allocate(allocDbg, tmp.sizeOfAllocatedMemory(), ALIGNMENT);
		std::memset(tmp.mData, 0, tmp.sizeOfAllocatedMemory());

		// Iterate over all pairs of objects in this HashMap and move them to the new one
		if (this->mData != nullptr) {
			for (KeyValuePair pair : *this) {
				tmp.put(pair.key, std::move(pair.value));
			}
		}

		// Replace this HashMap with the new one
		this->swap(tmp);
	}

	// Checks if HashMap needs to be rehashed, and will do so if necessary.
	void ensureProperlyHashed(DbgInfo allocDbg = sfz_dbg("HashMapDynamic"))
	{
		uint32_t maxNumOccupied = uint32_t(mCapacity * MAX_OCCUPIED_REHASH_FACTOR);
		if ((mSize + mPlaceholders) >= maxNumOccupied) {
			uint32_t newCapacity = mCapacity * GROW_RATE;
			if (newCapacity < DEFAULT_INITIAL_CAPACITY) newCapacity = DEFAULT_INITIAL_CAPACITY;
			this->rehash(newCapacity, allocDbg);
		}
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	// Returns the size of this HashMap. This is the number of elements stored, not the current
	// capacity.
	uint32_t size() const { return mSize; }

	// Returns the capacity of this HashMap.
	uint32_t capacity() const { return mCapacity; }

	// Returns the number of placeholder positions for removed elements. size + placeholders <=
	// capacity.
	uint32_t placeholders() const { return mPlaceholders; }

	// Returns the allocator of this HashMap. Will return nullptr if no allocator is set.
	Allocator* allocator() const { return mAllocator; }

	// Returns pointer to the element associated with the given key, or nullptr if no such element
	// exists. The pointer is valid until the HashMap is rehashed. This method will never cause a
	// rehash by itself.
	V* get(const K& key) { return this->getInternal<K>(key); }
	const V* get(const K& key) const { return this->getInternal<K>(key); }
	V* get(const AltK& key) { return this->getInternal<AltK>(key); }
	const V* get(const AltK& key) const { return this->getInternal<AltK>(key); }

	// Public methods
	// --------------------------------------------------------------------------------------------

	// Adds the specified key value pair to this HashMap. If a value is already associated with
	// the given key it will be replaced with the new value. Returns a reference to the element
	// set. Might trigger a rehash, which will cause all references to be invalidated.
	//
	// In particular the following scenario presents a dangerous trap:
	// V& ref1 = m.put(key1, value1);
	// V& ref2 = m.put(key2, value2);
	// At this point only ref2 is guaranteed to be valid, as the second call might have triggered
	// a rehash.
	V& put(const K& key, const V& value) { return this->putInternal<const K&, const V&>(key, value); }
	V& put(const K& key, V&& value) { return this->putInternal<const K&, V>(key, std::move(value)); }
	V& put(const AltK& key, const V& value) { return this->putInternal<const AltK&, const V&>(key, value); }
	V& put(const AltK& key, V&& value) { return this->putInternal<const AltK&, V>(key, std::move(value)); }

	// Access operator, will return a reference to the element associated with the given key. If
	// no such element exists it will be created with the default constructor. If element does not
	// exist and is created HashMap may be rehashed, and thus all references might be invalidated.
	V& operator[] (const K& key) { V* ptr = get(key); return ptr != nullptr ? *ptr : put(key, V()); }
	V& operator[] (const AltK& key) { V* ptr = get(key); return ptr != nullptr ? *ptr : put(key, V()); }

	// Attempts to remove the element associated with the given key. Returns false if this
	// HashMap contains no such element. Guaranteed to not rehash.
	bool remove(const K& key) { return this->removeInternal<K>(key); }
	bool remove(const AltK& key) { return this->removeInternal<AltK>(key); }

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
		Iterator(HashMapDynamic& hashMap, uint32_t index) noexcept : mHashMap(&hashMap), mIndex(index) { }
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
		HashMapDynamic* mHashMap;
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
		ConstIterator(const HashMapDynamic& hashMap, uint32_t index) noexcept : mHashMap(&hashMap), mIndex(index) {}
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
		const HashMapDynamic* mHashMap;
		uint32_t mIndex;
	};

	// Iterator methods
	// --------------------------------------------------------------------------------------------

	Iterator begin()
	{
		if (this->size() == 0) return Iterator(*this, uint32_t(~0));
		Iterator it(*this, 0);
		// Unless there happens to be an element in slot 0 we increment the iterator to find it
		if (elementInfo(uint32_t(0)) != ELEMENT_INFO_OCCUPIED) {
			++it;
		}
		return it;
	}

	ConstIterator begin() const { return cbegin(); }

	ConstIterator cbegin() const
	{
		if (this->size() == 0) return ConstIterator(*this, uint32_t(~0));
		ConstIterator it(*this, 0);
		// Unless there happens to be an element in slot 0 we increment the iterator to find it
		if (elementInfo(uint32_t(0)) != ELEMENT_INFO_OCCUPIED) {
			++it;
		}
		return it;
	}

	Iterator end() { return Iterator(*this, uint32_t(~0)); }
	ConstIterator end() const { return cend(); }
	ConstIterator cend() const { return ConstIterator(*this, uint32_t(~0)); }

private:
	// Private constants
	// --------------------------------------------------------------------------------------------

	static constexpr uint8_t ELEMENT_INFO_EMPTY = 0;
	static constexpr uint8_t ELEMENT_INFO_PLACEHOLDER = 1;
	static constexpr uint8_t ELEMENT_INFO_OCCUPIED = 2;

	// Private methods
	// --------------------------------------------------------------------------------------------

	// Return the size of the memory allocation for the element info array in bytes
	uint64_t sizeOfElementInfoArray() const
	{
		// 2 bits per info element, + 1 since mCapacity always is odd.
		uint64_t infoMinRequiredSize = (uint64_t(mCapacity) >> 2) + 1;

		// Calculate how many alignment sized chunks is needed to store element info
		uint64_t infoNumAlignmentSizedChunks = (infoMinRequiredSize >> ALIGNMENT_EXP) + 1;
		return infoNumAlignmentSizedChunks << ALIGNMENT_EXP;
	}

	// Returns the size of the memory allocation for the key array in bytes
	uint64_t sizeOfKeyArray() const
	{
		// Calculate how many aligment sized chunks is needed to store keys
		uint64_t keysMinRequiredSize = mCapacity * sizeof(K);
		uint64_t keyNumAlignmentSizedChunks = (keysMinRequiredSize >> ALIGNMENT_EXP) + 1;
		return keyNumAlignmentSizedChunks << ALIGNMENT_EXP;
	}

	// Returns the size of the memory allocation for the value array in bytes
	uint64_t sizeOfValueArray() const
	{
		// Calculate how many alignment sized chunks is needed to store values
		uint64_t valuesMinRequiredSize = mCapacity * sizeof(V);
		uint64_t valuesNumAlignmentSizedChunks = (valuesMinRequiredSize >> ALIGNMENT_EXP) + 1;
		return valuesNumAlignmentSizedChunks << ALIGNMENT_EXP;
	}

	// Returns the size of the allocated memory in bytes
	uint64_t sizeOfAllocatedMemory() const
	{
		return sizeOfElementInfoArray() + sizeOfKeyArray() + sizeOfValueArray();
	}

	// Returns pointer to the info bits part of the allocated memory
	uint8_t* elementInfoPtr() const { return mData; }

	// Returns pointer to the key array part of the allocated memory
	K* keysPtr() const
	{
		return reinterpret_cast<K*>(mData + sizeOfElementInfoArray());
	}

	// Returns pointer to the value array port fo the allocated memory
	V* valuesPtr() const
	{
		return reinterpret_cast<V*>(mData + sizeOfElementInfoArray() + sizeOfKeyArray());
	}

	// Returns the 2 bit element info about an element position in the HashMap
	// 0 = empty, 1 = removed, 2 = occupied, (3 is unused)
	uint8_t elementInfo(uint32_t index) const
	{
		uint32_t chunkIndex = index >> 2; // index / 4;
		uint32_t chunkIndexModulo = index & 0x03; // index % 4
		uint32_t chunkIndexModuloTimes2 = chunkIndexModulo << 1;

		uint8_t chunk = elementInfoPtr()[chunkIndex];
		uint8_t info = static_cast<uint8_t>((chunk >> (chunkIndexModuloTimes2)) & 0x3);

		return info;
	}

	// Sets the 2 bit element info with the selected value
	void setElementInfo(uint32_t index, uint8_t value)
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
	template<typename KT>
	uint32_t findElementIndex(
		const KT& key,
		bool& elementFound,
		uint32_t& firstFreeSlot,
		bool& isPlaceholder) const
	{
		elementFound = false;
		firstFreeSlot = uint32_t(~0);
		isPlaceholder = false;
		K* const keys = keysPtr();

		// Early exit if HashMap has no capacity
		if (mCapacity == 0) return uint32_t(~0);

		// Search for the element using linear probing
		const uint32_t baseIndex = uint32_t(sfz::hash(key) % uint64_t(mCapacity));
		for (uint32_t i = 0; i < mCapacity; i++) {

			// Try (base + i) index
			uint32_t index = (baseIndex + i) % mCapacity;
			uint8_t info = elementInfo(uint32_t(index));
			if (info == ELEMENT_INFO_EMPTY) {
				if (firstFreeSlot == uint32_t(~0)) firstFreeSlot = index;
				break;
			}
			else if (info == ELEMENT_INFO_PLACEHOLDER) {
				if (firstFreeSlot == uint32_t(~0)) {
					firstFreeSlot = index;
					isPlaceholder = true;
				}
			}
			else if (info == ELEMENT_INFO_OCCUPIED) {
				if (keys[index] == key) {
					elementFound = true;
					return index;
				}
			}
		}

		return uint32_t(~0);
	}

	// Internal shared implementation of all get() methods
	template<typename KT>
	V* getInternal(const KT& key) const
	{
		// Finds the index of the element
		uint32_t firstFreeSlot = uint32_t(~0);
		bool elementFound = false;
		bool isPlaceholder = false;
		uint32_t index = this->findElementIndex<KT>(key, elementFound, firstFreeSlot, isPlaceholder);

		// Returns nullptr if map doesn't contain element
		if (!elementFound) return nullptr;

		// Returns pointer to element
		return &(valuesPtr()[index]);
	}

	// Internal shared implementation of all put() methods
	template<typename KT, typename VT>
	V& putInternal(const KT& key, VT&& value)
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
		uint32_t index = this->findElementIndex<KT>(key, elementFound, firstFreeSlot, isPlaceholder);

		// If map contains key just replace value and return
		if (elementFound) {
			valuesPtr()[index] = std::forward<VT>(value);
			return valuesPtr()[index];
		}

		// Otherwise insert info, key and value
		setElementInfo(firstFreeSlot, ELEMENT_INFO_OCCUPIED);
		new (keysPtr() + firstFreeSlot) K(key);
		new (valuesPtr() + firstFreeSlot) V(std::forward<VT>(value));

		mSize += 1;
		if (isPlaceholder) mPlaceholders -= 1;
		return valuesPtr()[firstFreeSlot];
	}

	// Internal shared implementation of all remove() methods
	template<typename KT>
	bool removeInternal(const KT& key)
	{
		// Finds the index of the element
		uint32_t firstFreeSlot = uint32_t(~0);
		bool elementFound = false;
		bool isPlaceholder = false;
		uint32_t index = this->findElementIndex<KT>(key, elementFound, firstFreeSlot, isPlaceholder);

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
	uint8_t* mData = nullptr;
	Allocator* mAllocator = nullptr;
};

} // namespace sfz
