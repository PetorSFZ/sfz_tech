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

#include "sfz.h"
#include "sfz_cpp.hpp"

#ifdef __cplusplus

// sfzHash
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func u64 sfzHash(u8 v) { return u64(v); }
sfz_constexpr_func u64 sfzHash(u16 v) { return u64(v); }
sfz_constexpr_func u64 sfzHash(u32 v) { return u64(v); }
sfz_constexpr_func u64 sfzHash(u64 v) { return u64(v); }

sfz_constexpr_func u64 sfzHash(i8 v) { return u64(v); }
sfz_constexpr_func u64 sfzHash(i16 v) { return u64(v); }
sfz_constexpr_func u64 sfzHash(i32 v) { return u64(v); }
sfz_constexpr_func u64 sfzHash(i64 v) { return u64(v); }

sfz_constexpr_func u64 sfzHash(const void* v) { return u64(v); }

sfz_constexpr_func u64 sfzHashCombine(u64 h1, u64 h2)
{
	// hash_combine algorithm from boost
	u64 h = h1 + 0x9e3779b9;
	h ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
	return h;
}

sfz_constexpr_func u64 sfzHash(u8x2 v) { return (u64(v.x) << 8) | u64(v.y); }
sfz_constexpr_func u64 sfzHash(u8x4 v) { return (u64(v.x) << 24) | (u64(v.y) << 16) | (u64(v.z) << 8) | u64(v.w); }

sfz_constexpr_func u64 sfzHash(i32x2 v) { return (u64(v.x) << 32) | u64(v.y); }
sfz_constexpr_func u64 sfzHash(i32x3 v) { return sfzHashCombine(sfzHash(v.xy()), sfzHash(v.z)); }
sfz_constexpr_func u64 sfzHash(i32x4 v) { return sfzHashCombine(sfzHash(v.xyz()), sfzHash(v.w)); }

// HashMap helpers
// ------------------------------------------------------------------------------------------------

// The state of a slot in a HashMap
enum class SfzHashMapSlotState : u32 {
	EMPTY = 0, // No key/value pair associated with slot
	PLACEHOLDER = 1, // Key/value pair was associated, but subsequently removed
	OCCUPIED = 2 // Key/value pair associated with slot
};

// The data for a slot in a HashMap. A slot in the OCCUPIED state has an index into the key and
// value arrays of a HashMap indicating where the key/value pair are stored.
class SfzHashMapSlot final {
public:
	SfzHashMapSlot() noexcept = default;
	SfzHashMapSlot(const SfzHashMapSlot&) noexcept = default;
	SfzHashMapSlot& operator= (const SfzHashMapSlot&) noexcept = default;

	SfzHashMapSlot(SfzHashMapSlotState state, u32 index) noexcept
	{
		mSlot = ((u32(state) & 0x03u) << 30u) | (index & 0x3FFFFFFFu);
	}

	SfzHashMapSlotState state() const { return SfzHashMapSlotState((mSlot >> 30u) & 0x03u); }
	u32 index() const { return mSlot & 0x3FFFFFFFu; }

private:
	u32 mSlot = 0u;
};
static_assert(sizeof(SfzHashMapSlot) == sizeof(u32), "");

// Simple container used for hash map iterators.
template<typename K, typename V>
struct SfzHashMapPair final {
	const K& key;
	V& value;
	SfzHashMapPair(const K& key, V& value) noexcept : key(key), value(value) { }
	SfzHashMapPair(const SfzHashMapPair&) noexcept = default;
	SfzHashMapPair& operator= (const SfzHashMapPair&) = delete; // Because references...
};

template<typename MapT, typename K, typename V>
class SfzHashMapItr final {
public:
	SfzHashMapItr(MapT& map, u32 idx) noexcept : mMap(&map), mIdx(idx) { }
	SfzHashMapItr(const SfzHashMapItr&) noexcept = default;
	SfzHashMapItr& operator= (const SfzHashMapItr&) noexcept = default;

	SfzHashMapItr& operator++ () { if (mIdx < mMap->size()) { mIdx += 1; } return *this; } // Pre-increment
	SfzHashMapItr operator++ (int) { auto copy = *this; ++(*this); return copy; } // Post-increment
	SfzHashMapPair<K, V> operator* () {
		sfz_assert(mIdx < mMap->size()); return SfzHashMapPair<K, V>(mMap->keys()[mIdx], mMap->values()[mIdx]); }

	bool operator== (const SfzHashMapItr& o) const { return (mMap == o.mMap) && (mIdx == o.mIdx); }
	bool operator!= (const SfzHashMapItr& o) const { return !(*this == o); }

private:
	MapT* mMap;
	u32 mIdx;
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
class SfzHashMap final {
public:
	// Constants and typedefs
	// --------------------------------------------------------------------------------------------

	using AltK = typename SfzAltType<K>::AltT;

	static constexpr u32 ALIGNMENT = 32;
	static constexpr u32 MIN_CAPACITY = 64;
	static constexpr u32 MAX_CAPACITY = (1 << 30) - 1; // 2 bits reserved for info
	static constexpr f32 MAX_OCCUPIED_REHASH_FACTOR = 0.80f;
	static constexpr f32 GROW_RATE = 1.75f;

	static_assert(alignof(K) <= ALIGNMENT, "");
	static_assert(alignof(V) <= ALIGNMENT, "");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------
	
	SFZ_DECLARE_DROP_TYPE(SfzHashMap);

	SfzHashMap(u32 capacity, SfzAllocator* allocator, SfzDbgInfo allocDbg) noexcept
	{
		this->init(capacity, allocator, allocDbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(u32 capacity, SfzAllocator* allocator, SfzDbgInfo allocDbg)
	{
		this->destroy();
		mAllocator = allocator;
		this->rehash(capacity, allocDbg);
	}

	SfzHashMap clone(SfzAllocator* allocator, SfzDbgInfo allocDbg) const
	{
		SfzHashMap tmp(mCapacity, allocator, allocDbg);
		tmp.mSize = this->mSize;
		for (u32 i = 0; i < mSize; i++) {
			tmp.mKeys[i] = this->mKeys[i];
			tmp.mValues[i] = this->mValues[i];
		}
		tmp.mPlaceholders = this->mPlaceholders;
		for (u32 i = 0; i < mCapacity; i++) {
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
		mAllocator->dealloc(mAllocation);
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
		for (u32 i = 0; i < mSize; i++) {
			mKeys[i].~K();
			mValues[i].~V();
		}

		// Clear all slots
		memset(mSlots, 0, sfzRoundUpAlignedU64(mCapacity * sizeof(SfzHashMapSlot), ALIGNMENT));

		// Set size to 0
		mSize = 0;
		mPlaceholders = 0;
	}

	// Rehashes this HashMap to the specified capacity. All old pointers and references are invalidated.
	void rehash(u32 newCapacity, SfzDbgInfo allocDbg)
	{
		if (newCapacity == 0) return;
		if (newCapacity < MIN_CAPACITY) newCapacity = MIN_CAPACITY;
		if (newCapacity < mCapacity) newCapacity = mCapacity;

		// Don't rehash if capacity already exists and there are no placeholders
		if (newCapacity == mCapacity && mPlaceholders == 0) return;

		sfz_assert_hard(mAllocator != nullptr);

		// Create new hash map and calculate size of its arrays
		SfzHashMap tmp;
		tmp.mCapacity = newCapacity;
		u64 sizeOfSlots = sfzRoundUpAlignedU64(tmp.mCapacity * sizeof(SfzHashMapSlot), ALIGNMENT);
		u64 sizeOfKeys = sfzRoundUpAlignedU64(sizeof(K) * tmp.mCapacity, ALIGNMENT);
		u64 sizeOfValues = sfzRoundUpAlignedU64(sizeof(V) * tmp.mCapacity, ALIGNMENT);
		u64 allocSize = sizeOfSlots + sizeOfKeys + sizeOfValues;

		// Allocate and clear memory for new hash map
		tmp.mAllocation = static_cast<u8*>(mAllocator->alloc(allocDbg, allocSize, ALIGNMENT));
		memset(tmp.mAllocation, 0, allocSize);
		tmp.mAllocator = mAllocator;
		tmp.mSlots = reinterpret_cast<SfzHashMapSlot*>(tmp.mAllocation);
		tmp.mKeys = reinterpret_cast<K*>(tmp.mAllocation + sizeOfSlots);
		tmp.mValues = reinterpret_cast<V*>(tmp.mAllocation + sizeOfSlots + sizeOfKeys);
		//sfz_assert(isAligned(tmp.mKeys, ALIGNMENT));
		//sfz_assert(isAligned(tmp.mValues, ALIGNMENT));

		// Iterate over all pairs of objects in this HashMap and move them to the new one
		if (this->mAllocation != nullptr) {
			for (u32 i = 0; i < mSize; i++) {
				tmp.put(sfz_move(mKeys[i]), sfz_move(mValues[i]));
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
	u32 size() const { return mSize; }
	u32 capacity() const { return mCapacity; }
	u32 placeholders() const { return mPlaceholders; }
	SfzAllocator* allocator() const { return mAllocator; }

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
	V& put(const K& key, V&& value) { return this->putInternal<const K&, V>(key, sfz_move(value)); }
	V& put(const AltK& key, const V& value) { return this->putInternal<const K&, const V&>(SfzAltType<K>::conv(key), value); }
	V& put(const AltK& key, V&& value) { return this->putInternal<const K&, V>(SfzAltType<K>::conv(key), sfz_move(value)); }

	// Attempts to remove the element associated with the given key. Returns false if this
	// HashMap contains no such element. Guaranteed to not rehash.
	bool remove(const K& key) { return this->removeInternal<K>(key); }
	bool remove(const AltK& key) { return this->removeInternal<AltK>(key); }

	// Iterators
	// --------------------------------------------------------------------------------------------

	using Iterator = SfzHashMapItr<SfzHashMap, K, V>;
	using ConstIterator = SfzHashMapItr<const SfzHashMap, K, const V>;

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
	void findSlot(const KT& key, u32& firstFreeSlotIdx, u32& occupiedSlotIdx) const
	{
		firstFreeSlotIdx = ~0u;
		occupiedSlotIdx = ~0u;

		// Search for the element using linear probing
		const u32 baseIndex = mCapacity != 0 ? u32(sfzHash(key) % u64(mCapacity)) : 0;
		for (u32 i = 0; i < mCapacity; i++) {
			const u32 slotIdx = (baseIndex + i) % mCapacity;
			SfzHashMapSlot slot = mSlots[slotIdx];
			SfzHashMapSlotState state = slot.state();

			if (state != SfzHashMapSlotState::OCCUPIED) {
				if (firstFreeSlotIdx == ~0u) firstFreeSlotIdx = slotIdx;
				if (state == SfzHashMapSlotState::EMPTY) break;
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
	void swapElements(u32 slotIdx1, u32 slotIdx2)
	{
		sfz_assert(slotIdx1 < mCapacity);
		sfz_assert(slotIdx2 < mCapacity);
		SfzHashMapSlot slot1 = mSlots[slotIdx1];
		SfzHashMapSlot slot2 = mSlots[slotIdx2];
		sfz_assert(slot1.state() == SfzHashMapSlotState::OCCUPIED);
		sfz_assert(slot2.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx1 = slot1.index();
		u32 idx2 = slot2.index();
		sfz_assert(idx1 < mSize);
		sfz_assert(idx2 < mSize);
		sfzSwap(mSlots[slotIdx1], mSlots[slotIdx2]);
		sfzSwap(mKeys[idx1], mKeys[idx2]);
		sfzSwap(mValues[idx1], mValues[idx2]);
	}

	template<typename KT>
	V* getInternal(const KT& key) const
	{
		// Finds slots
		u32 firstFreeSlotIdx = ~0u;
		u32 occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return nullptr if map does not contain element
		if (occupiedSlotIdx == ~0u) return nullptr;

		// Returns pointer to element
		sfz_assert(occupiedSlotIdx < mCapacity);
		SfzHashMapSlot slot = mSlots[occupiedSlotIdx];
		sfz_assert(slot.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx = slot.index();
		sfz_assert(idx < mSize);
		return mValues + idx;
	}

	template<typename KT, typename VT>
	V& putInternal(const KT& key, VT&& value)
	{
		// Rehash if necessary
		u32 maxNumOccupied = u32(mCapacity * MAX_OCCUPIED_REHASH_FACTOR);
		if ((mSize + mPlaceholders) >= maxNumOccupied) {
			this->rehash(u32((mCapacity + 1) * GROW_RATE), sfz_dbg("HashMap"));
		}

		// Finds slots
		u32 firstFreeSlotIdx = ~0u;
		u32 occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// If map contains key, replace value and return
		if (occupiedSlotIdx != ~0u) {
			sfz_assert(occupiedSlotIdx < mCapacity);
			SfzHashMapSlot slot = mSlots[occupiedSlotIdx];
			u32 idx = slot.index();
			sfz_assert(idx < mSize);
			mValues[idx] = sfz_forward(value);
			return mValues[idx];
		}

		// Calculate next index
		u32 nextFreeIdx = mSize;
		mSize += 1;

		// Check if previous slot was placeholder and then create new slot
		sfz_assert(firstFreeSlotIdx < mCapacity);
		bool wasPlaceholder = mSlots[firstFreeSlotIdx].state() == SfzHashMapSlotState::PLACEHOLDER;
		if (wasPlaceholder) mPlaceholders -= 1;
		mSlots[firstFreeSlotIdx] = SfzHashMapSlot(SfzHashMapSlotState::OCCUPIED, nextFreeIdx);

		// Insert key and value
		// Perfect forwarding: const reference: VT == const V&, rvalue: VT == V
		// std::forward<VT>(value) will then return the correct version of value
		new (mKeys + nextFreeIdx) K(key);
		new (mValues + nextFreeIdx) V(sfz_forward(value));
		return mValues[nextFreeIdx];
	}

	template<typename KT>
	bool removeInternal(const KT& key)
	{
		// Finds slots
		u32 firstFreeSlotIdx = ~0u;
		u32 occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return false if map does not contain element
		if (occupiedSlotIdx == ~0u) return false;

		// Swap the key/value pair with the last key/value pair in the arrays
		u32 lastSlotIdx = ~0u;
		{
			sfz_assert(mSize > 0);
			u32 unused = ~0u;
			this->findSlot<K>(mKeys[mSize - 1], unused, lastSlotIdx);
			sfz_assert(lastSlotIdx != ~0u);
		}
		this->swapElements(occupiedSlotIdx, lastSlotIdx);

		// Remove the element
		u32 idx = mSlots[occupiedSlotIdx].index();
		sfz_assert(idx < mSize);
		mSlots[occupiedSlotIdx] = SfzHashMapSlot(SfzHashMapSlotState::PLACEHOLDER, ~0u);
		mKeys[idx].~K();
		mValues[idx].~V();

		// Update info
		mSize -= 1;
		mPlaceholders += 1;
		return true;
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	u32 mSize = 0, mCapacity = 0, mPlaceholders = 0;
	u8* mAllocation = nullptr;
	SfzHashMapSlot* mSlots = nullptr;
	K* mKeys = nullptr;
	V* mValues = nullptr;
	SfzAllocator* mAllocator = nullptr;
};

// HashMapLocal
// ------------------------------------------------------------------------------------------------

template<typename K, typename V, u32 Capacity>
class SfzHashMapLocal {
public:
	using AltK = typename SfzAltType<K>::AltT;
	static_assert(alignof(K) <= 16, "");
	static_assert(alignof(V) <= 16, "");
	
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	SfzHashMapLocal() { static_assert(sizeof(SfzHashMapLocal) ==
		((sizeof(SfzHashMapSlot) + sizeof(K) + sizeof(V)) * Capacity + 16), ""); }
	SfzHashMapLocal(const SfzHashMapLocal&) = default;
	SfzHashMapLocal& operator= (const SfzHashMapLocal&) = default;
	SfzHashMapLocal(SfzHashMapLocal&& other) noexcept { this->swap(other); }
	SfzHashMapLocal& operator= (SfzHashMapLocal&& other) noexcept { this->swap(other); return *this; }
	~SfzHashMapLocal() = default;

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(SfzHashMapLocal& other)
	{
		for (u32 i = 0; i < Capacity; i++) {
			sfzSwap(this->mSlots[i], other.mSlots[i]);
			sfzSwap(this->mKeys[i], other.mKeys[i]);
			sfzSwap(this->mValues[i], other.mValues[i]);
		}
		sfzSwap(this->mSize, other.mSize);
		sfzSwap(this->mPlaceholders, other.mPlaceholders);
	}

	void clear()
	{
		if (mSize == 0) return;
		sfz_assert(mSize <= Capacity);

		// Call destructors for all active keys and values
		for (u32 i = 0; i < mSize; i++) {
			mKeys[i] = K();
			mValues[i] = V();
		}

		// Clear all slots
		memset(mSlots, 0, sfzRoundUpAlignedU64(Capacity * sizeof(SfzHashMapSlot), 32));

		// Set size to 0
		mSize = 0;
		mPlaceholders = 0;
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	const K* keys() const { return mKeys; }
	V* values() { return mValues; }
	const V* values() const { return mValues; }
	u32 size() const { return mSize; }
	u32 capacity() const { return Capacity; }
	u32 placeholders() const { return mPlaceholders; }

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
	V& put(const K& key, V&& value) { return this->putInternal<const K&, V>(key, sfz_move(value)); }
	V& put(const AltK& key, const V& value) { return this->putInternal<const K&, const V&>(SfzAltType<K>::conv(key), value); }
	V& put(const AltK& key, V&& value) { return this->putInternal<const K&, V>(SfzAltType<K>::conv(key), sfz_move(value)); }

	bool remove(const K& key) { return this->removeInternal<K>(key); }
	bool remove(const AltK& key) { return this->removeInternal<AltK>(key); }

	// Iterators
	// --------------------------------------------------------------------------------------------

	using Iterator = SfzHashMapItr<SfzHashMapLocal, K, V>;
	using ConstIterator = SfzHashMapItr<const SfzHashMapLocal, K, const V>;

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
	void findSlot(const KT& key, u32& firstFreeSlotIdx, u32& occupiedSlotIdx) const
	{
		firstFreeSlotIdx = ~0u;
		occupiedSlotIdx = ~0u;

		// Search for the element using linear probing
		const u32 baseIndex = Capacity != 0 ? u32(sfzHash(key) % u64(Capacity)) : 0;
		for (u32 i = 0; i < Capacity; i++) {
			const u32 slotIdx = (baseIndex + i) % Capacity;
			SfzHashMapSlot slot = mSlots[slotIdx];
			SfzHashMapSlotState state = slot.state();

			if (state != SfzHashMapSlotState::OCCUPIED) {
				if (firstFreeSlotIdx == ~0u) firstFreeSlotIdx = slotIdx;
				if (state == SfzHashMapSlotState::EMPTY) break;
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
	void swapElements(u32 slotIdx1, u32 slotIdx2)
	{
		sfz_assert(slotIdx1 < Capacity);
		sfz_assert(slotIdx2 < Capacity);
		SfzHashMapSlot slot1 = mSlots[slotIdx1];
		SfzHashMapSlot slot2 = mSlots[slotIdx2];
		sfz_assert(slot1.state() == SfzHashMapSlotState::OCCUPIED);
		sfz_assert(slot2.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx1 = slot1.index();
		u32 idx2 = slot2.index();
		sfz_assert(idx1 < mSize);
		sfz_assert(idx2 < mSize);
		sfzSwap(mSlots[slotIdx1], mSlots[slotIdx2]);
		sfzSwap(mKeys[idx1], mKeys[idx2]);
		sfzSwap(mValues[idx1], mValues[idx2]);
	}

	template<typename KT>
	V* getInternal(const KT& key)
	{
		// Finds slots
		u32 firstFreeSlotIdx = ~0u;
		u32 occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return nullptr if map does not contain element
		if (occupiedSlotIdx == ~0u) return nullptr;

		// Returns pointer to element
		sfz_assert(occupiedSlotIdx < Capacity);
		SfzHashMapSlot slot = mSlots[occupiedSlotIdx];
		sfz_assert(slot.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx = slot.index();
		sfz_assert(idx < mSize);
		return mValues + idx;
	}

	template<typename KT>
	const V* getInternal(const KT& key) const
	{
		// Finds slots
		u32 firstFreeSlotIdx = ~0u;
		u32 occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return nullptr if map does not contain element
		if (occupiedSlotIdx == ~0u) return nullptr;

		// Returns pointer to element
		sfz_assert(occupiedSlotIdx < Capacity);
		SfzHashMapSlot slot = mSlots[occupiedSlotIdx];
		sfz_assert(slot.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx = slot.index();
		sfz_assert(idx < mSize);
		return mValues + idx;
	}

	template<typename KT, typename VT>
	V& putInternal(const KT& key, VT&& value)
	{
		// Finds slots
		u32 firstFreeSlotIdx = ~0u;
		u32 occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// If map contains key, replace value and return
		if (occupiedSlotIdx != ~0u) {
			sfz_assert(occupiedSlotIdx < Capacity);
			SfzHashMapSlot slot = mSlots[occupiedSlotIdx];
			u32 idx = slot.index();
			sfz_assert(idx < mSize);
			mValues[idx] = sfz_forward(value);
			return mValues[idx];
		}

		// Calculate next index
		sfz_assert_hard(mSize < Capacity);
		u32 nextFreeIdx = mSize;
		mSize += 1;

		// Check if previous slot was placeholder and then create new slot
		sfz_assert_hard(firstFreeSlotIdx < Capacity);
		bool wasPlaceholder = mSlots[firstFreeSlotIdx].state() == SfzHashMapSlotState::PLACEHOLDER;
		if (wasPlaceholder) mPlaceholders -= 1;
		mSlots[firstFreeSlotIdx] = SfzHashMapSlot(SfzHashMapSlotState::OCCUPIED, nextFreeIdx);

		// Insert key and value
		// Perfect forwarding: const reference: VT == const V&, rvalue: VT == V
		// std::forward<VT>(value) will then return the correct version of value
		mKeys[nextFreeIdx] = key;
		mValues[nextFreeIdx] = sfz_forward(value);
		return mValues[nextFreeIdx];
	}

	template<typename KT>
	bool removeInternal(const KT& key)
	{
		// Finds slots
		u32 firstFreeSlotIdx = ~0u;
		u32 occupiedSlotIdx = ~0u;
		this->findSlot<KT>(key, firstFreeSlotIdx, occupiedSlotIdx);

		// Return false if map does not contain element
		if (occupiedSlotIdx == ~0u) return false;

		// Swap the key/value pair with the last key/value pair in the arrays
		u32 lastSlotIdx = ~0u;
		{
			sfz_assert(mSize > 0);
			u32 unused = ~0u;
			this->findSlot<K>(mKeys[mSize - 1], unused, lastSlotIdx);
			sfz_assert(lastSlotIdx != ~0u);
		}
		this->swapElements(occupiedSlotIdx, lastSlotIdx);

		// Remove the element
		u32 idx = mSlots[occupiedSlotIdx].index();
		sfz_assert(idx < mSize);
		mSlots[occupiedSlotIdx] = SfzHashMapSlot(SfzHashMapSlotState::PLACEHOLDER, ~0u);
		mKeys[idx] = K();
		mValues[idx] = V();

		// Update info
		mSize -= 1;
		mPlaceholders += 1;
		return true;
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	SfzHashMapSlot mSlots[Capacity];
	K mKeys[Capacity];
	V mValues[Capacity];
	u32 mSize = 0;
	u32 mPlaceholders = 0;
	u32 mPadding[2] = {};
};

template<typename K, typename V> using SfzMap4 = SfzHashMapLocal<K, V, 4>;
template<typename K, typename V> using SfzMap5 = SfzHashMapLocal<K, V, 5>;
template<typename K, typename V> using SfzMap6 = SfzHashMapLocal<K, V, 6>;
template<typename K, typename V> using SfzMap8 = SfzHashMapLocal<K, V, 8>;
template<typename K, typename V> using SfzMap10 = SfzHashMapLocal<K, V, 10>;
template<typename K, typename V> using SfzMap12 = SfzHashMapLocal<K, V, 12>;
template<typename K, typename V> using SfzMap16 = SfzHashMapLocal<K, V, 16>;
template<typename K, typename V> using SfzMap20 = SfzHashMapLocal<K, V, 20>;
template<typename K, typename V> using SfzMap24 = SfzHashMapLocal<K, V, 24>;
template<typename K, typename V> using SfzMap32 = SfzHashMapLocal<K, V, 32>;
template<typename K, typename V> using SfzMap40 = SfzHashMapLocal<K, V, 40>;
template<typename K, typename V> using SfzMap48 = SfzHashMapLocal<K, V, 48>;
template<typename K, typename V> using SfzMap64 = SfzHashMapLocal<K, V, 64>;
template<typename K, typename V> using SfzMap80 = SfzHashMapLocal<K, V, 80>;
template<typename K, typename V> using SfzMap96 = SfzHashMapLocal<K, V, 96>;
template<typename K, typename V> using SfzMap128 = SfzHashMapLocal<K, V, 128>;
template<typename K, typename V> using SfzMap192 = SfzHashMapLocal<K, V, 192>;
template<typename K, typename V> using SfzMap256 = SfzHashMapLocal<K, V, 256>;
template<typename K, typename V> using SfzMap320 = SfzHashMapLocal<K, V, 320>;
template<typename K, typename V> using SfzMap512 = SfzHashMapLocal<K, V, 512>;

#endif // __cplusplus
#endif // SKIPIFZERO_HASH_MAPS
