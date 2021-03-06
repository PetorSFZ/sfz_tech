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

#ifndef SKIPIFZERO_HASH_MAPS_HPP
#define SKIPIFZERO_HASH_MAPS_HPP
#pragma once

#include "skipifzero.hpp"

namespace sfz {

// sfz::hash
// ------------------------------------------------------------------------------------------------

constexpr uint64_t hash(uint8_t value) { return uint64_t(value); }
constexpr uint64_t hash(uint16_t value) { return uint64_t(value); }
constexpr uint64_t hash(uint32_t value) { return uint64_t(value); }
constexpr uint64_t hash(uint64_t value) { return uint64_t(value); }

constexpr uint64_t hash(int16_t value) { return uint64_t(value); }
constexpr uint64_t hash(int32_t value) { return uint64_t(value); }
constexpr uint64_t hash(int64_t value) { return uint64_t(value); }

constexpr uint64_t hash(const void* value) { return uint64_t(uintptr_t(value)); }

template<typename T, uint32_t N>
constexpr uint64_t hash(Vec<T,N> v)
{
	uint64_t hash = 0;
	for (uint32_t i = 0; i < N; i++) {
		// hash_combine algorithm from boost
		hash ^= sfz::hash(v[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}
	return hash;
}

// HashMap helpers
// ------------------------------------------------------------------------------------------------

// The state of a slot in a HashMap
enum class HashMapSlotState : uint32_t {
	EMPTY = 0, // No key/value pair associated with slot
	PLACEHOLDER = 1, // Key/value pair was associated, but subsequently removed
	OCCUPIED = 2 // Key/value pair associated with slot
};

// The data for a slot in a HashMap. A slot in the OCCUPIED state has an index into the key and
// value arrays of a HashMap indicating where the key/value pair are stored.
class HashMapSlot final {
public:
	HashMapSlot() noexcept = default;
	HashMapSlot(const HashMapSlot&) noexcept = default;
	HashMapSlot& operator= (const HashMapSlot&) noexcept = default;

	HashMapSlot(HashMapSlotState state, uint32_t index) noexcept
	{
		mSlot = ((uint32_t(state) & 0x03u) << 30u) | (index & 0x3FFFFFFFu);
	}

	HashMapSlotState state() const { return HashMapSlotState((mSlot >> 30u) & 0x03u); }
	uint32_t index() const { return mSlot & 0x3FFFFFFFu; }

private:
	uint32_t mSlot = 0u;
};
static_assert(sizeof(HashMapSlot) == sizeof(uint32_t), "");

// Simple container used for hash map iterators.
template<typename K, typename V>
struct HashMapPair final {
	const K& key;
	V& value;
	HashMapPair(const K& key, V& value) noexcept : key(key), value(value) { }
	HashMapPair(const HashMapPair&) noexcept = default;
	HashMapPair& operator= (const HashMapPair&) = delete; // Because references...
};

template<typename MapT, typename K, typename V>
class HashMapItr final {
public:
	HashMapItr(MapT& map, uint32_t idx) noexcept : mMap(&map), mIdx(idx) { }
	HashMapItr(const HashMapItr&) noexcept = default;
	HashMapItr& operator= (const HashMapItr&) noexcept = default;

	HashMapItr& operator++ () { if (mIdx < mMap->size()) { mIdx += 1; } return *this; } // Pre-increment
	HashMapItr operator++ (int) { auto copy = *this; ++(*this); return copy; } // Post-increment
	HashMapPair<K, V> operator* () {
		sfz_assert(mIdx < mMap->size()); return HashMapPair<K, V>(mMap->keys()[mIdx], mMap->values()[mIdx]); }

	bool operator== (const HashMapItr& o) const { return (mMap == o.mMap) && (mIdx == o.mIdx); }
	bool operator!= (const HashMapItr& o) const { return !(*this == o); }

private:
	MapT* mMap;
	uint32_t mIdx;
};

// HashMap
// ------------------------------------------------------------------------------------------------

// A HashMap with closed hashing (open adressing) and linear probing.
//
// Similarly to Mattias Gustavsson's excellent C hash table, the keys and values are compactly
// stored in sequential arrays. This makes iterating over the contents of a HashMap very cache
// efficient, while paying a small cost for an extra indirection when looking up a specific key.
// See: https://github.com/mattiasgustavsson/libs/blob/master/hashtable.h
//
// In order to accomplish the above this implementation uses the concepts of "slots" and "indices".
// A "slot" is a number in the range [0, capacity), and is what the hash a given key is mapped to.
// A "slot" contains an "index" to where the value (and key) associated with the key is stored,
// i.e. an "index" is in the range [0, size).
//
// Removal of elements is O(1), but will leave a placeholder on the previously occupied slot. The
// current number of placeholders can be queried by the placeholders() method. Both size and
// placeholders count as load when checking if the HashMap needs to be rehashed or not.
//
// An alternate key type can be specified by specializing sfz::AltType<K>. This is mostly useful
// when strings are used as keys, then const char* can be used as an alt key type. This removes
// the need to create a temporary key object (which might need to allocate memory).
template<typename K, typename V>
class HashMap final {
public:
	// Constants and typedefs
	// --------------------------------------------------------------------------------------------

	using AltK = typename sfz::AltType<K>::AltT;

	static constexpr uint32_t ALIGNMENT = 32;
	static constexpr uint32_t MIN_CAPACITY = 64;
	static constexpr uint32_t MAX_CAPACITY = (1 << 30) - 1; // 2 bits reserved for info
	static constexpr float MAX_OCCUPIED_REHASH_FACTOR = 0.80f;
	static constexpr float GROW_RATE = 1.75f;

	static_assert(alignof(K) <= ALIGNMENT, "");
	static_assert(alignof(V) <= ALIGNMENT, "");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------
	
	SFZ_DECLARE_DROP_TYPE(HashMap);

	HashMap(uint32_t capacity, Allocator* allocator, DbgInfo allocDbg) noexcept
	{
		this->init(capacity, allocator, allocDbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(uint32_t capacity, Allocator* allocator, DbgInfo allocDbg)
	{
		this->destroy();
		mAllocator = allocator;
		this->rehash(capacity, allocDbg);
	}

	HashMap clone(Allocator* allocator, DbgInfo allocDbg) const
	{
		HashMap tmp(mCapacity, allocator, allocDbg);
		tmp.mSize = this->mSize;
		for (uint32_t i = 0; i < mSize; i++) {
			tmp.mKeys[i] = this->mKeys[i];
			tmp.mValues[i] = this->mValues[i];
		}
		tmp.mPlaceholders = this->mPlaceholders;
		for (uint32_t i = 0; i < mCapacity; i++) {
			tmp.mSlots[i] = this->mSlots[i];
		}
		return tmp;
	}

	// Destroys all elements stored in this HashMap, deallocates all memory and removes allocator.
	void destroy()
	{
		if (mAllocation == nullptr) { mAllocator = nullptr; return; }

		// Remove elements
		this->clear();

		// Deallocate memory
		mAllocator->deallocate(mAllocation);
		mCapacity = 0;
		mPlaceholders = 0;
		mAllocation = nullptr;
		mSlots = nullptr;
		mKeys = nullptr;
		mValues = nullptr;
		mAllocator = nullptr;
	}

	// Removes all elements from this HashMap without deallocating memory.
	void clear()
	{
		if (mSize == 0) return;
		sfz_assert(mSize <= mCapacity);

		// Call destructors for all active keys and values
		for (uint32_t i = 0; i < mSize; i++) {
			mKeys[i].~K();
			mValues[i].~V();
		}

		// Clear all slots
		std::memset(mSlots, 0, roundUpAligned(mCapacity * sizeof(HashMapSlot), ALIGNMENT));

		// Set size to 0
		mSize = 0;
		mPlaceholders = 0;
	}

	// Rehashes this HashMap to the specified capacity. All old pointers and references are invalidated.
	void rehash(uint32_t newCapacity, DbgInfo allocDbg)
	{
		if (newCapacity == 0) return;
		if (newCapacity < MIN_CAPACITY) newCapacity = MIN_CAPACITY;
		if (newCapacity < mCapacity) newCapacity = mCapacity;

		// Don't rehash if capacity already exists and there are no placeholders
		if (newCapacity == mCapacity && mPlaceholders == 0) return;

		sfz_assert_hard(mAllocator != nullptr);

		// Create new hash map and calculate size of its arrays
		HashMap tmp;
		tmp.mCapacity = newCapacity;
		uint64_t sizeOfSlots = roundUpAligned(tmp.mCapacity * sizeof(HashMapSlot), ALIGNMENT);
		uint64_t sizeOfKeys = roundUpAligned(sizeof(K) * tmp.mCapacity, ALIGNMENT);
		uint64_t sizeOfValues = roundUpAligned(sizeof(V) * tmp.mCapacity, ALIGNMENT);
		uint64_t allocSize = sizeOfSlots + sizeOfKeys + sizeOfValues;

		// Allocate and clear memory for new hash map
		tmp.mAllocation = static_cast<uint8_t*>(mAllocator->allocate(allocDbg, allocSize, ALIGNMENT));
		std::memset(tmp.mAllocation, 0, allocSize);
		tmp.mAllocator = mAllocator;
		tmp.mSlots = reinterpret_cast<HashMapSlot*>(tmp.mAllocation);
		tmp.mKeys = reinterpret_cast<K*>(tmp.mAllocation + sizeOfSlots);
		tmp.mValues = reinterpret_cast<V*>(tmp.mAllocation + sizeOfSlots + sizeOfKeys);
		sfz_assert(isAligned(tmp.mKeys, ALIGNMENT));
		sfz_assert(isAligned(tmp.mValues, ALIGNMENT));

		// Iterate over all pairs of objects in this HashMap and move them to the new one
		if (this->mAllocation != nullptr) {
			for (uint32_t i = 0; i < mSize; i++) {
				tmp.put(std::move(mKeys[i]), std::move(mValues[i]));
			}
		}

		// Replace this HashMap with the new one
		this->swap(tmp);
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	const K* keys() const { return mKeys; }
	V* values() { return mValues; }
	const V* values() const { return mValues; }
	uint32_t size() const { return mSize; }
	uint32_t capacity() const { return mCapacity; }
	uint32_t placeholders() const { return mPlaceholders; }
	Allocator* allocator() const { return mAllocator; }

	// Returns pointer to the element associated with the given key, or nullptr if no such element
	// exists. The pointer is valid until the HashMap is rehashed. This method will never cause a
	// rehash by itself.
	V* get(const K& key) { return this->getInternal<K>(key); }
	const V* get(const K& key) const { return this->getInternal<K>(key); }
	V* get(const AltK& key) { return this->getInternal<AltK>(key); }
	const V* get(const AltK& key) const { return this->getInternal<AltK>(key); }

	// Access operator, will return a reference to the element associated with the given key.
	// Will terminate the program if no such key exists.
	V& operator[] (const K& key) { V* ptr = get(key); sfz_assert_hard(ptr != nullptr); return *ptr; }
	const V& operator[] (const K& key) const { const V* ptr = get(key); sfz_assert_hard(ptr != nullptr); return *ptr; }
	V& operator[] (const AltK& key) { V* ptr = get(key); sfz_assert_hard(ptr != nullptr); return *ptr; }
	const V& operator[] (const AltK& key) const { const V* ptr = get(key); sfz_assert_hard(ptr != nullptr); return *ptr; }

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

	// Attempts to remove the element associated with the given key. Returns false if this
	// HashMap contains no such element. Guaranteed to not rehash.
	bool remove(const K& key) { return this->removeInternal<K>(key); }
	bool remove(const AltK& key) { return this->removeInternal<AltK>(key); }

	// Iterators
	// --------------------------------------------------------------------------------------------

	using Iterator = HashMapItr<HashMap, K, V>;
	using ConstIterator = HashMapItr<const HashMap, K, const V>;

	Iterator begin() { return Iterator(*this, 0); }
	ConstIterator begin() const { return cbegin(); }
	ConstIterator cbegin() const { return ConstIterator(*this, 0); }

	Iterator end() { return Iterator(*this, mSize); }
	ConstIterator end() const { return cend(); }
	ConstIterator cend() const { return ConstIterator(*this, mSize); }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	template<typename KT>
	void findSlot(const KT& key, uint32_t& firstFreeSlotIdx, uint32_t& occupiedSlotIdx) const
	{
		firstFreeSlotIdx = ~0u;
		occupiedSlotIdx = ~0u;

		// Search for the element using linear probing
		const uint32_t baseIndex = mCapacity != 0 ? uint32_t(sfz::hash(key) % uint64_t(mCapacity)) : 0;
		for (uint32_t i = 0; i < mCapacity; i++) {
			const uint32_t slotIdx = (baseIndex + i) % mCapacity;
			HashMapSlot slot = mSlots[slotIdx];
			HashMapSlotState state = slot.state();

			if (state != HashMapSlotState::OCCUPIED) {
				if (firstFreeSlotIdx == ~0u) firstFreeSlotIdx = slotIdx;
				if (state == HashMapSlotState::EMPTY) break;
			}
			else {
				if (mKeys[slot.index()] == key) {
					occupiedSlotIdx = slotIdx;
					break;
				}
			}
		}
	}

	// Swaps the position of two key/value pairs in the internal arrays and updates their slots
	void swapElements(uint32_t slotIdx1, uint32_t slotIdx2)
	{
		sfz_assert(slotIdx1 < mCapacity);
		sfz_assert(slotIdx2 < mCapacity);
		HashMapSlot slot1 = mSlots[slotIdx1];
		HashMapSlot slot2 = mSlots[slotIdx2];
		sfz_assert(slot1.state() == HashMapSlotState::OCCUPIED);
		sfz_assert(slot2.state() == HashMapSlotState::OCCUPIED);
		uint32_t idx1 = slot1.index();
		uint32_t idx2 = slot2.index();
		sfz_assert(idx1 < mSize);
		sfz_assert(idx2 < mSize);
		std::swap(mSlots[slotIdx1], mSlots[slotIdx2]);
		std::swap(mKeys[idx1], mKeys[idx2]);
		std::swap(mValues[idx1], mValues[idx2]);
	}

	template<typename KT>
	V* getInternal(const KT& key) const
	{
		// Finds slots
		uint32_t firstFreeSlotIdx = ~0u;
		uint32_t occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return nullptr if map does not contain element
		if (occupiedSlotIdx == ~0u) return nullptr;

		// Returns pointer to element
		sfz_assert(occupiedSlotIdx < mCapacity);
		HashMapSlot slot = mSlots[occupiedSlotIdx];
		sfz_assert(slot.state() == HashMapSlotState::OCCUPIED);
		uint32_t idx = slot.index();
		sfz_assert(idx < mSize);
		return mValues + idx;
	}

	template<typename KT, typename VT>
	V& putInternal(const KT& key, VT&& value)
	{
		// Rehash if necessary
		uint32_t maxNumOccupied = uint32_t(mCapacity * MAX_OCCUPIED_REHASH_FACTOR);
		if ((mSize + mPlaceholders) >= maxNumOccupied) {
			this->rehash(uint32_t((mCapacity + 1) * GROW_RATE), sfz_dbg("HashMap"));
		}

		// Finds slots
		uint32_t firstFreeSlotIdx = ~0u;
		uint32_t occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// If map contains key, replace value and return
		if (occupiedSlotIdx != ~0u) {
			sfz_assert(occupiedSlotIdx < mCapacity);
			HashMapSlot slot = mSlots[occupiedSlotIdx];
			uint32_t idx = slot.index();
			sfz_assert(idx < mSize);
			mValues[idx] = std::forward<VT>(value);
			return mValues[idx];
		}

		// Calculate next index
		uint32_t nextFreeIdx = mSize;
		mSize += 1;

		// Check if previous slot was placeholder and then create new slot
		sfz_assert(firstFreeSlotIdx < mCapacity);
		bool wasPlaceholder = mSlots[firstFreeSlotIdx].state() == HashMapSlotState::PLACEHOLDER;
		if (wasPlaceholder) mPlaceholders -= 1;
		mSlots[firstFreeSlotIdx] = HashMapSlot(HashMapSlotState::OCCUPIED, nextFreeIdx);

		// Insert key and value
		// Perfect forwarding: const reference: VT == const V&, rvalue: VT == V
		// std::forward<VT>(value) will then return the correct version of value
		new (mKeys + nextFreeIdx) K(key);
		new (mValues + nextFreeIdx) V(std::forward<VT>(value));
		return mValues[nextFreeIdx];
	}

	template<typename KT>
	bool removeInternal(const KT& key)
	{
		// Finds slots
		uint32_t firstFreeSlotIdx = ~0u;
		uint32_t occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return false if map does not contain element
		if (occupiedSlotIdx == ~0u) return false;

		// Swap the key/value pair with the last key/value pair in the arrays
		uint32_t lastSlotIdx = ~0u;
		{
			sfz_assert(mSize > 0);
			uint32_t unused = ~0u;
			this->findSlot<K>(mKeys[mSize - 1], unused, lastSlotIdx);
			sfz_assert(lastSlotIdx != ~0u);
		}
		this->swapElements(occupiedSlotIdx, lastSlotIdx);

		// Remove the element
		uint32_t idx = mSlots[occupiedSlotIdx].index();
		sfz_assert(idx < mSize);
		mSlots[occupiedSlotIdx] = HashMapSlot(HashMapSlotState::PLACEHOLDER, ~0u);
		mKeys[idx].~K();
		mValues[idx].~V();

		// Update info
		mSize -= 1;
		mPlaceholders += 1;
		return true;
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mSize = 0, mCapacity = 0, mPlaceholders = 0;
	uint8_t* mAllocation = nullptr;
	HashMapSlot* mSlots = nullptr;
	K* mKeys = nullptr;
	V* mValues = nullptr;
	Allocator* mAllocator = nullptr;
};

// HashMapLocal
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, uint32_t Capacity>
class HashMapLocal {
public:
	using AltK = typename sfz::AltType<K>::AltT;
	static_assert(alignof(K) <= 16, "");
	static_assert(alignof(V) <= 16, "");
	
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	HashMapLocal() { static_assert(sizeof(HashMapLocal) ==
		((sizeof(HashMapSlot) + sizeof(K) + sizeof(V)) * Capacity + 16), ""); }
	HashMapLocal(const HashMapLocal&) = default;
	HashMapLocal& operator= (const HashMapLocal&) = default;
	HashMapLocal(HashMapLocal&& other) noexcept { this->swap(other); }
	HashMapLocal& operator= (HashMapLocal&& other) noexcept { this->swap(other); return *this; }
	~HashMapLocal() = default;

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(HashMapLocal& other)
	{
		for (uint32_t i = 0; i < Capacity; i++) {
			std::swap(this->mSlots[i], other.mSlots[i]);
			std::swap(this->mKeys[i], other.mKeys[i]);
			std::swap(this->mValues[i], other.mValues[i]);
		}
		std::swap(this->mSize, other.mSize);
		std::swap(this->mPlaceholders, other.mPlaceholders);
	}

	void clear()
	{
		if (mSize == 0) return;
		sfz_assert(mSize <= Capacity);

		// Call destructors for all active keys and values
		for (uint32_t i = 0; i < mSize; i++) {
			mKeys[i] = K();
			mValues[i] = V();
		}

		// Clear all slots
		std::memset(mSlots, 0, roundUpAligned(Capacity * sizeof(HashMapSlot), 32));

		// Set size to 0
		mSize = 0;
		mPlaceholders = 0;
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	const K* keys() const { return mKeys; }
	V* values() { return mValues; }
	const V* values() const { return mValues; }
	uint32_t size() const { return mSize; }
	uint32_t capacity() const { return Capacity; }
	uint32_t placeholders() const { return mPlaceholders; }

	V* get(const K& key) { return this->getInternal<K>(key); }
	const V* get(const K& key) const { return this->getInternal<K>(key); }
	V* get(const AltK& key) { return this->getInternal<AltK>(key); }
	const V* get(const AltK& key) const { return this->getInternal<AltK>(key); }

	V& operator[] (const K& key) { V* ptr = get(key); sfz_assert_hard(ptr != nullptr); return *ptr; }
	const V& operator[] (const K& key) const { const V* ptr = get(key); sfz_assert_hard(ptr != nullptr); return *ptr; }
	V& operator[] (const AltK& key) { V* ptr = get(key); sfz_assert_hard(ptr != nullptr); return *ptr; }
	const V& operator[] (const AltK& key) const { const V* ptr = get(key); sfz_assert_hard(ptr != nullptr); return *ptr; }
	
	// Public methods
	// --------------------------------------------------------------------------------------------

	V& put(const K& key, const V& value) { return this->putInternal<const K&, const V&>(key, value); }
	V& put(const K& key, V&& value) { return this->putInternal<const K&, V>(key, std::move(value)); }
	V& put(const AltK& key, const V& value) { return this->putInternal<const AltK&, const V&>(key, value); }
	V& put(const AltK& key, V&& value) { return this->putInternal<const AltK&, V>(key, std::move(value)); }

	bool remove(const K& key) { return this->removeInternal<K>(key); }
	bool remove(const AltK& key) { return this->removeInternal<AltK>(key); }

	// Iterators
	// --------------------------------------------------------------------------------------------

	using Iterator = HashMapItr<HashMapLocal, K, V>;
	using ConstIterator = HashMapItr<const HashMapLocal, K, const V>;

	Iterator begin() { return Iterator(*this, 0); }
	ConstIterator begin() const { return cbegin(); }
	ConstIterator cbegin() const { return ConstIterator(*this, 0); }

	Iterator end() { return Iterator(*this, mSize); }
	ConstIterator end() const { return cend(); }
	ConstIterator cend() const { return ConstIterator(*this, mSize); }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	template<typename KT>
	void findSlot(const KT& key, uint32_t& firstFreeSlotIdx, uint32_t& occupiedSlotIdx) const
	{
		firstFreeSlotIdx = ~0u;
		occupiedSlotIdx = ~0u;

		// Search for the element using linear probing
		const uint32_t baseIndex = Capacity != 0 ? uint32_t(sfz::hash(key) % uint64_t(Capacity)) : 0;
		for (uint32_t i = 0; i < Capacity; i++) {
			const uint32_t slotIdx = (baseIndex + i) % Capacity;
			HashMapSlot slot = mSlots[slotIdx];
			HashMapSlotState state = slot.state();

			if (state != HashMapSlotState::OCCUPIED) {
				if (firstFreeSlotIdx == ~0u) firstFreeSlotIdx = slotIdx;
				if (state == HashMapSlotState::EMPTY) break;
			}
			else {
				if (mKeys[slot.index()] == key) {
					occupiedSlotIdx = slotIdx;
					break;
				}
			}
		}
	}

	// Swaps the position of two key/value pairs in the internal arrays and updates their slots
	void swapElements(uint32_t slotIdx1, uint32_t slotIdx2)
	{
		sfz_assert(slotIdx1 < Capacity);
		sfz_assert(slotIdx2 < Capacity);
		HashMapSlot slot1 = mSlots[slotIdx1];
		HashMapSlot slot2 = mSlots[slotIdx2];
		sfz_assert(slot1.state() == HashMapSlotState::OCCUPIED);
		sfz_assert(slot2.state() == HashMapSlotState::OCCUPIED);
		uint32_t idx1 = slot1.index();
		uint32_t idx2 = slot2.index();
		sfz_assert(idx1 < mSize);
		sfz_assert(idx2 < mSize);
		std::swap(mSlots[slotIdx1], mSlots[slotIdx2]);
		std::swap(mKeys[idx1], mKeys[idx2]);
		std::swap(mValues[idx1], mValues[idx2]);
	}

	template<typename KT>
	V* getInternal(const KT& key)
	{
		// Finds slots
		uint32_t firstFreeSlotIdx = ~0u;
		uint32_t occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return nullptr if map does not contain element
		if (occupiedSlotIdx == ~0u) return nullptr;

		// Returns pointer to element
		sfz_assert(occupiedSlotIdx < Capacity);
		HashMapSlot slot = mSlots[occupiedSlotIdx];
		sfz_assert(slot.state() == HashMapSlotState::OCCUPIED);
		uint32_t idx = slot.index();
		sfz_assert(idx < mSize);
		return mValues + idx;
	}

	template<typename KT>
	const V* getInternal(const KT& key) const
	{
		// Finds slots
		uint32_t firstFreeSlotIdx = ~0u;
		uint32_t occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return nullptr if map does not contain element
		if (occupiedSlotIdx == ~0u) return nullptr;

		// Returns pointer to element
		sfz_assert(occupiedSlotIdx < Capacity);
		HashMapSlot slot = mSlots[occupiedSlotIdx];
		sfz_assert(slot.state() == HashMapSlotState::OCCUPIED);
		uint32_t idx = slot.index();
		sfz_assert(idx < mSize);
		return mValues + idx;
	}

	template<typename KT, typename VT>
	V& putInternal(const KT& key, VT&& value)
	{
		// Finds slots
		uint32_t firstFreeSlotIdx = ~0u;
		uint32_t occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// If map contains key, replace value and return
		if (occupiedSlotIdx != ~0u) {
			sfz_assert(occupiedSlotIdx < Capacity);
			HashMapSlot slot = mSlots[occupiedSlotIdx];
			uint32_t idx = slot.index();
			sfz_assert(idx < mSize);
			mValues[idx] = std::forward<VT>(value);
			return mValues[idx];
		}

		// Calculate next index
		sfz_assert_hard(mSize < Capacity);
		uint32_t nextFreeIdx = mSize;
		mSize += 1;

		// Check if previous slot was placeholder and then create new slot
		sfz_assert_hard(firstFreeSlotIdx < Capacity);
		bool wasPlaceholder = mSlots[firstFreeSlotIdx].state() == HashMapSlotState::PLACEHOLDER;
		if (wasPlaceholder) mPlaceholders -= 1;
		mSlots[firstFreeSlotIdx] = HashMapSlot(HashMapSlotState::OCCUPIED, nextFreeIdx);

		// Insert key and value
		// Perfect forwarding: const reference: VT == const V&, rvalue: VT == V
		// std::forward<VT>(value) will then return the correct version of value
		mKeys[nextFreeIdx] = K(key);
		mValues[nextFreeIdx] = std::forward<VT>(value);
		return mValues[nextFreeIdx];
	}

	template<typename KT>
	bool removeInternal(const KT& key)
	{
		// Finds slots
		uint32_t firstFreeSlotIdx = ~0u;
		uint32_t occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return false if map does not contain element
		if (occupiedSlotIdx == ~0u) return false;

		// Swap the key/value pair with the last key/value pair in the arrays
		uint32_t lastSlotIdx = ~0u;
		{
			sfz_assert(mSize > 0);
			uint32_t unused = ~0u;
			this->findSlot<K>(mKeys[mSize - 1], unused, lastSlotIdx);
			sfz_assert(lastSlotIdx != ~0u);
		}
		this->swapElements(occupiedSlotIdx, lastSlotIdx);

		// Remove the element
		uint32_t idx = mSlots[occupiedSlotIdx].index();
		sfz_assert(idx < mSize);
		mSlots[occupiedSlotIdx] = HashMapSlot(HashMapSlotState::PLACEHOLDER, ~0u);
		mKeys[idx] = K();
		mValues[idx] = V();

		// Update info
		mSize -= 1;
		mPlaceholders += 1;
		return true;
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	HashMapSlot mSlots[Capacity];
	K mKeys[Capacity];
	V mValues[Capacity];
	uint32_t mSize = 0;
	uint32_t mPlaceholders = 0;
	uint32_t mPadding[2] = {};
};

template<typename K, typename V> using Map4 = HashMapLocal<K, V, 4>;
template<typename K, typename V> using Map5 = HashMapLocal<K, V, 5>;
template<typename K, typename V> using Map6 = HashMapLocal<K, V, 6>;
template<typename K, typename V> using Map8 = HashMapLocal<K, V, 8>;
template<typename K, typename V> using Map10 = HashMapLocal<K, V, 10>;
template<typename K, typename V> using Map12 = HashMapLocal<K, V, 12>;
template<typename K, typename V> using Map16 = HashMapLocal<K, V, 16>;
template<typename K, typename V> using Map20 = HashMapLocal<K, V, 20>;
template<typename K, typename V> using Map24 = HashMapLocal<K, V, 24>;
template<typename K, typename V> using Map32 = HashMapLocal<K, V, 32>;
template<typename K, typename V> using Map40 = HashMapLocal<K, V, 40>;
template<typename K, typename V> using Map48 = HashMapLocal<K, V, 48>;
template<typename K, typename V> using Map64 = HashMapLocal<K, V, 64>;
template<typename K, typename V> using Map80 = HashMapLocal<K, V, 80>;
template<typename K, typename V> using Map96 = HashMapLocal<K, V, 96>;
template<typename K, typename V> using Map128 = HashMapLocal<K, V, 128>;
template<typename K, typename V> using Map192 = HashMapLocal<K, V, 192>;
template<typename K, typename V> using Map256 = HashMapLocal<K, V, 256>;
template<typename K, typename V> using Map320 = HashMapLocal<K, V, 320>;
template<typename K, typename V> using Map512 = HashMapLocal<K, V, 512>;

} // namespace sfz

#endif
