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

// HashMap
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
static_assert(sizeof(HashMapSlot) == sizeof(uint32_t));

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
class HashMap {
public:
	// Constants and typedefs
	// --------------------------------------------------------------------------------------------

	using AltK = typename sfz::AltType<K>::AltT;

	static constexpr uint32_t ALIGNMENT = 32;
	static constexpr uint32_t MIN_CAPACITY = 64;
	static constexpr uint32_t MAX_CAPACITY = (1 << 30) - 1; // 2 bits reserved for info
	static constexpr float MAX_OCCUPIED_REHASH_FACTOR = 0.80f;
	static constexpr float GROW_RATE = 1.75f;

	static_assert(alignof(K) <= ALIGNMENT);
	static_assert(alignof(V) <= ALIGNMENT);

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	HashMap() noexcept = default;
	HashMap(const HashMap& other) noexcept { *this = other; }
	HashMap& operator= (const HashMap& other) noexcept { *this = other.clone(); return *this; }
	HashMap(HashMap&& other) noexcept { this->swap(other); }
	HashMap& operator= (HashMap&& other) noexcept { this->swap(other); return *this; }
	~HashMap() noexcept { this->destroy(); }

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

	HashMap clone(DbgInfo allocDbg = sfz_dbg("HashMap"), Allocator* allocator = nullptr) const
	{
		HashMap tmp(mCapacity, allocator != nullptr ? allocator : mAllocator, allocDbg);
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

	// Swaps the contents of two HashMaps, including the allocators.
	void swap(HashMap& other)
	{
		std::swap(this->mSize, other.mSize);
		std::swap(this->mCapacity, other.mCapacity);
		std::swap(this->mPlaceholders, other.mPlaceholders);
		std::swap(this->mAllocation, other.mAllocation);
		std::swap(this->mSlots, other.mSlots);
		std::swap(this->mKeys, other.mKeys);
		std::swap(this->mValues, other.mValues);
		std::swap(this->mAllocator, other.mAllocator);
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

	template<typename VT>
	struct Pair final {
		const K& key;
		VT& value;
		Pair(const K& key, VT& value) noexcept : key(key), value(value) { }
		Pair(const Pair&) noexcept = default;
		Pair& operator= (const Pair&) = delete; // Because references...
	};

	template<typename MapT, typename VT>
	class Itr final {
	public:
		Itr(MapT& map, uint32_t idx) noexcept : mMap(&map), mIdx(idx) { }
		Itr(const Itr&) noexcept = default;
		Itr& operator= (const Itr&) noexcept = default;

		Itr& operator++ () { if (mIdx < mMap->mSize) { mIdx += 1; } return *this; } // Pre-increment
		Itr operator++ (int) { auto copy = *this; ++(*this); return copy; } // Post-increment
		Pair<VT> operator* () { sfz_assert(mIdx < mMap->mSize); return Pair<VT>(mMap->mKeys[mIdx], mMap->mValues[mIdx]); }

		bool operator== (const Itr& o) const { return (mMap == o.mMap) && (mIdx == o.mIdx); }
		bool operator!= (const Itr& o) const { return !(*this == o); }

	private:
		MapT* mMap;
		uint32_t mIdx;
	};

	using Iterator = Itr<HashMap, V>;
	using ConstIterator = Itr<const HashMap, const V>;

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
			this->rehash((mCapacity + 1) * GROW_RATE, sfz_dbg("HashMap"));
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

} // namespace sfz
