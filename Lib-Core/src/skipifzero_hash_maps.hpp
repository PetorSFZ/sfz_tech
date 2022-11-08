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
		m_slot = ((u32(state) & 0x03u) << 30u) | (index & 0x3FFFFFFFu);
	}

	SfzHashMapSlotState state() const { return SfzHashMapSlotState((m_slot >> 30u) & 0x03u); }
	u32 index() const { return m_slot & 0x3FFFFFFFu; }

private:
	u32 m_slot = 0u;
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
	SfzHashMapItr(MapT& map, u32 idx) noexcept : m_map(&map), m_idx(idx) { }
	SfzHashMapItr(const SfzHashMapItr&) noexcept = default;
	SfzHashMapItr& operator= (const SfzHashMapItr&) noexcept = default;

	SfzHashMapItr& operator++ () { if (m_idx < m_map->size()) { m_idx += 1; } return *this; } // Pre-increment
	SfzHashMapItr operator++ (int) { auto copy = *this; ++(*this); return copy; } // Post-increment
	SfzHashMapPair<K, V> operator* () {
		sfz_assert(m_idx < m_map->size()); return SfzHashMapPair<K, V>(m_map->keys()[m_idx], m_map->values()[m_idx]); }

	bool operator== (const SfzHashMapItr& o) const { return (m_map == o.m_map) && (m_idx == o.m_idx); }
	bool operator!= (const SfzHashMapItr& o) const { return !(*this == o); }

private:
	MapT* m_map;
	u32 m_idx;
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

	SfzHashMap(u32 capacity, SfzAllocator* allocator, SfzDbgInfo alloc_dbg) noexcept
	{
		this->init(capacity, allocator, alloc_dbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(u32 capacity, SfzAllocator* allocator, SfzDbgInfo alloc_dbg)
	{
		this->destroy();
		m_allocator = allocator;
		this->rehash(capacity, alloc_dbg);
	}

	SfzHashMap clone(SfzAllocator* allocator, SfzDbgInfo alloc_dbg) const
	{
		SfzHashMap tmp(m_capacity, allocator, alloc_dbg);
		tmp.m_size = this->m_size;
		for (u32 i = 0; i < m_size; i++) {
			tmp.m_keys[i] = this->m_keys[i];
			tmp.m_values[i] = this->m_values[i];
		}
		tmp.m_placeholders = this->m_placeholders;
		for (u32 i = 0; i < m_capacity; i++) {
			tmp.m_slots[i] = this->m_slots[i];
		}
		return tmp;
	}

	// Destroys all elements stored in this HashMap, deallocates all memory and removes allocator.
	void destroy()
	{
		if (m_allocation == nullptr) { m_allocator = nullptr; return; }

		// Remove elements
		this->clear();

		// Deallocate memory
		m_allocator->dealloc(m_allocation);
		m_capacity = 0;
		m_placeholders = 0;
		m_allocation = nullptr;
		m_slots = nullptr;
		m_keys = nullptr;
		m_values = nullptr;
		m_allocator = nullptr;
	}

	// Removes all elements from this HashMap without deallocating memory.
	void clear()
	{
		if (m_size == 0) return;
		sfz_assert(m_size <= m_capacity);

		// Call destructors for all active keys and values
		for (u32 i = 0; i < m_size; i++) {
			m_keys[i].~K();
			m_values[i].~V();
		}

		// Clear all slots
		memset(m_slots, 0, sfzRoundUpAlignedU64(m_capacity * sizeof(SfzHashMapSlot), ALIGNMENT));

		// Set size to 0
		m_size = 0;
		m_placeholders = 0;
	}

	// Rehashes this HashMap to the specified capacity. All old pointers and references are invalidated.
	void rehash(u32 new_capacity, SfzDbgInfo alloc_dbg)
	{
		if (new_capacity == 0) return;
		if (new_capacity < MIN_CAPACITY) new_capacity = MIN_CAPACITY;
		if (new_capacity < m_capacity) new_capacity = m_capacity;

		// Don't rehash if capacity already exists and there are no placeholders
		if (new_capacity == m_capacity && m_placeholders == 0) return;

		sfz_assert_hard(m_allocator != nullptr);

		// Create new hash map and calculate size of its arrays
		SfzHashMap tmp;
		tmp.m_capacity = new_capacity;
		u64 size_of_slots = sfzRoundUpAlignedU64(tmp.m_capacity * sizeof(SfzHashMapSlot), ALIGNMENT);
		u64 size_of_keys = sfzRoundUpAlignedU64(sizeof(K) * tmp.m_capacity, ALIGNMENT);
		u64 size_of_values = sfzRoundUpAlignedU64(sizeof(V) * tmp.m_capacity, ALIGNMENT);
		u64 allocSize = size_of_slots + size_of_keys + size_of_values;

		// Allocate and clear memory for new hash map
		tmp.m_allocation = static_cast<u8*>(m_allocator->alloc(alloc_dbg, allocSize, ALIGNMENT));
		memset(tmp.m_allocation, 0, allocSize);
		tmp.m_allocator = m_allocator;
		tmp.m_slots = reinterpret_cast<SfzHashMapSlot*>(tmp.m_allocation);
		tmp.m_keys = reinterpret_cast<K*>(tmp.m_allocation + size_of_slots);
		tmp.m_values = reinterpret_cast<V*>(tmp.m_allocation + size_of_slots + size_of_keys);
		//sfz_assert(isAligned(tmp.m_keys, ALIGNMENT));
		//sfz_assert(isAligned(tmp.m_values, ALIGNMENT));

		// Iterate over all pairs of objects in this HashMap and move them to the new one
		if (this->m_allocation != nullptr) {
			for (u32 i = 0; i < m_size; i++) {
				tmp.put(sfz_move(m_keys[i]), sfz_move(m_values[i]));
			}
		}

		// Replace this HashMap with the new one
		this->swap(tmp);
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	const K* keys() const { return m_keys; }
	V* values() { return m_values; }
	const V* values() const { return m_values; }
	u32 size() const { return m_size; }
	u32 capacity() const { return m_capacity; }
	u32 placeholders() const { return m_placeholders; }
	SfzAllocator* allocator() const { return m_allocator; }

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

	Iterator end() { return Iterator(*this, m_size); }
	ConstIterator end() const { return cend(); }
	ConstIterator cend() const { return ConstIterator(*this, m_size); }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	template<typename KT>
	void findSlot(const KT& key, u32& first_free_slot_idx, u32& occupied_slot_idx) const
	{
		first_free_slot_idx = ~0u;
		occupied_slot_idx = ~0u;

		// Search for the element using linear probing
		const u32 base_index = m_capacity != 0 ? u32(sfzHash(key) % u64(m_capacity)) : 0;
		for (u32 i = 0; i < m_capacity; i++) {
			const u32 slotIdx = (base_index + i) % m_capacity;
			SfzHashMapSlot slot = m_slots[slotIdx];
			SfzHashMapSlotState state = slot.state();

			if (state != SfzHashMapSlotState::OCCUPIED) {
				if (first_free_slot_idx == ~0u) first_free_slot_idx = slotIdx;
				if (state == SfzHashMapSlotState::EMPTY) break;
			}
			else {
				if (m_keys[slot.index()] == key) {
					occupied_slot_idx = slotIdx;
					break;
				}
			}
		}
	}

	// Swaps the position of two key/value pairs in the internal arrays and updates their slots
	void swapElements(u32 slot_idx_1, u32 slot_idx_2)
	{
		sfz_assert(slot_idx_1 < m_capacity);
		sfz_assert(slot_idx_2 < m_capacity);
		SfzHashMapSlot slot1 = m_slots[slot_idx_1];
		SfzHashMapSlot slot2 = m_slots[slot_idx_2];
		sfz_assert(slot1.state() == SfzHashMapSlotState::OCCUPIED);
		sfz_assert(slot2.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx1 = slot1.index();
		u32 idx2 = slot2.index();
		sfz_assert(idx1 < m_size);
		sfz_assert(idx2 < m_size);
		sfzSwap(m_slots[slot_idx_1], m_slots[slot_idx_2]);
		sfzSwap(m_keys[idx1], m_keys[idx2]);
		sfzSwap(m_values[idx1], m_values[idx2]);
	}

	template<typename KT>
	V* getInternal(const KT& key) const
	{
		// Finds slots
		u32 first_free_slot_idx = ~0u;
		u32 occupied_slot_idx = ~0u;
		this->findSlot<KT>(key, first_free_slot_idx, occupied_slot_idx);

		// Return nullptr if map does not contain element
		if (occupied_slot_idx == ~0u) return nullptr;

		// Returns pointer to element
		sfz_assert(occupied_slot_idx < m_capacity);
		SfzHashMapSlot slot = m_slots[occupied_slot_idx];
		sfz_assert(slot.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx = slot.index();
		sfz_assert(idx < m_size);
		return m_values + idx;
	}

	template<typename KT, typename VT>
	V& putInternal(const KT& key, VT&& value)
	{
		// Rehash if necessary
		u32 max_num_occupied = u32(m_capacity * MAX_OCCUPIED_REHASH_FACTOR);
		if ((m_size + m_placeholders) >= max_num_occupied) {
			this->rehash(u32((m_capacity + 1) * GROW_RATE), sfz_dbg("HashMap"));
		}

		// Finds slots
		u32 first_free_slot_idx = ~0u;
		u32 occupied_slot_idx = ~0u;
		this->findSlot<KT>(key, first_free_slot_idx, occupied_slot_idx);

		// If map contains key, replace value and return
		if (occupied_slot_idx != ~0u) {
			sfz_assert(occupied_slot_idx < m_capacity);
			SfzHashMapSlot slot = m_slots[occupied_slot_idx];
			u32 idx = slot.index();
			sfz_assert(idx < m_size);
			m_values[idx] = sfz_forward(value);
			return m_values[idx];
		}

		// Calculate next index
		u32 next_free_idx = m_size;
		m_size += 1;

		// Check if previous slot was placeholder and then create new slot
		sfz_assert(first_free_slot_idx < m_capacity);
		bool was_placeholder = m_slots[first_free_slot_idx].state() == SfzHashMapSlotState::PLACEHOLDER;
		if (was_placeholder) m_placeholders -= 1;
		m_slots[first_free_slot_idx] = SfzHashMapSlot(SfzHashMapSlotState::OCCUPIED, next_free_idx);

		// Insert key and value
		// Perfect forwarding: const reference: VT == const V&, rvalue: VT == V
		// std::forward<VT>(value) will then return the correct version of value
		new (m_keys + next_free_idx) K(key);
		new (m_values + next_free_idx) V(sfz_forward(value));
		return m_values[next_free_idx];
	}

	template<typename KT>
	bool removeInternal(const KT& key)
	{
		// Finds slots
		u32 first_free_slot_idx = ~0u;
		u32 occupied_slot_idx = ~0u;
		this->findSlot<KT>(key, first_free_slot_idx, occupied_slot_idx);

		// Return false if map does not contain element
		if (occupied_slot_idx == ~0u) return false;

		// Swap the key/value pair with the last key/value pair in the arrays
		u32 last_slot_idx = ~0u;
		{
			sfz_assert(m_size > 0);
			u32 unused = ~0u;
			this->findSlot<K>(m_keys[m_size - 1], unused, last_slot_idx);
			sfz_assert(last_slot_idx != ~0u);
		}
		this->swapElements(occupied_slot_idx, last_slot_idx);

		// Remove the element
		u32 idx = m_slots[occupied_slot_idx].index();
		sfz_assert(idx < m_size);
		m_slots[occupied_slot_idx] = SfzHashMapSlot(SfzHashMapSlotState::PLACEHOLDER, ~0u);
		m_keys[idx].~K();
		m_values[idx].~V();

		// Update info
		m_size -= 1;
		m_placeholders += 1;
		return true;
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	u32 m_size = 0, m_capacity = 0, m_placeholders = 0;
	u8* m_allocation = nullptr;
	SfzHashMapSlot* m_slots = nullptr;
	K* m_keys = nullptr;
	V* m_values = nullptr;
	SfzAllocator* m_allocator = nullptr;
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
			sfzSwap(this->m_slots[i], other.m_slots[i]);
			sfzSwap(this->m_keys[i], other.m_keys[i]);
			sfzSwap(this->m_values[i], other.m_values[i]);
		}
		sfzSwap(this->m_size, other.m_size);
		sfzSwap(this->m_placeholders, other.m_placeholders);
	}

	void clear()
	{
		if (m_size == 0) return;
		sfz_assert(m_size <= Capacity);

		// Call destructors for all active keys and values
		for (u32 i = 0; i < m_size; i++) {
			m_keys[i] = K();
			m_values[i] = V();
		}

		// Clear all slots
		memset(m_slots, 0, sfzRoundUpAlignedU64(Capacity * sizeof(SfzHashMapSlot), 32));

		// Set size to 0
		m_size = 0;
		m_placeholders = 0;
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	const K* keys() const { return m_keys; }
	V* values() { return m_values; }
	const V* values() const { return m_values; }
	u32 size() const { return m_size; }
	u32 capacity() const { return Capacity; }
	u32 placeholders() const { return m_placeholders; }

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

	Iterator end() { return Iterator(*this, m_size); }
	ConstIterator end() const { return cend(); }
	ConstIterator cend() const { return ConstIterator(*this, m_size); }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	template<typename KT>
	void findSlot(const KT& key, u32& first_free_slot_idx, u32& occupied_slot_idx) const
	{
		first_free_slot_idx = ~0u;
		occupied_slot_idx = ~0u;

		// Search for the element using linear probing
		const u32 base_index = Capacity != 0 ? u32(sfzHash(key) % u64(Capacity)) : 0;
		for (u32 i = 0; i < Capacity; i++) {
			const u32 slotIdx = (base_index + i) % Capacity;
			SfzHashMapSlot slot = m_slots[slotIdx];
			SfzHashMapSlotState state = slot.state();

			if (state != SfzHashMapSlotState::OCCUPIED) {
				if (first_free_slot_idx == ~0u) first_free_slot_idx = slotIdx;
				if (state == SfzHashMapSlotState::EMPTY) break;
			}
			else {
				if (m_keys[slot.index()] == key) {
					occupied_slot_idx = slotIdx;
					break;
				}
			}
		}
	}

	// Swaps the position of two key/value pairs in the internal arrays and updates their slots
	void swapElements(u32 slot_idx_1, u32 slot_idx_2)
	{
		sfz_assert(slot_idx_1 < Capacity);
		sfz_assert(slot_idx_2 < Capacity);
		SfzHashMapSlot slot1 = m_slots[slot_idx_1];
		SfzHashMapSlot slot2 = m_slots[slot_idx_2];
		sfz_assert(slot1.state() == SfzHashMapSlotState::OCCUPIED);
		sfz_assert(slot2.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx1 = slot1.index();
		u32 idx2 = slot2.index();
		sfz_assert(idx1 < m_size);
		sfz_assert(idx2 < m_size);
		sfzSwap(m_slots[slot_idx_1], m_slots[slot_idx_2]);
		sfzSwap(m_keys[idx1], m_keys[idx2]);
		sfzSwap(m_values[idx1], m_values[idx2]);
	}

	template<typename KT>
	V* getInternal(const KT& key)
	{
		// Finds slots
		u32 first_free_slot_idx = ~0u;
		u32 occupied_slot_idx = ~0u;
		this->findSlot<KT>(key, first_free_slot_idx, occupied_slot_idx);

		// Return nullptr if map does not contain element
		if (occupied_slot_idx == ~0u) return nullptr;

		// Returns pointer to element
		sfz_assert(occupied_slot_idx < Capacity);
		SfzHashMapSlot slot = m_slots[occupied_slot_idx];
		sfz_assert(slot.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx = slot.index();
		sfz_assert(idx < m_size);
		return m_values + idx;
	}

	template<typename KT>
	const V* getInternal(const KT& key) const
	{
		// Finds slots
		u32 first_free_slot_idx = ~0u;
		u32 occupied_slot_idx = ~0u;
		this->findSlot<KT>(key, first_free_slot_idx, occupied_slot_idx);

		// Return nullptr if map does not contain element
		if (occupied_slot_idx == ~0u) return nullptr;

		// Returns pointer to element
		sfz_assert(occupied_slot_idx < Capacity);
		SfzHashMapSlot slot = m_slots[occupied_slot_idx];
		sfz_assert(slot.state() == SfzHashMapSlotState::OCCUPIED);
		u32 idx = slot.index();
		sfz_assert(idx < m_size);
		return m_values + idx;
	}

	template<typename KT, typename VT>
	V& putInternal(const KT& key, VT&& value)
	{
		// Finds slots
		u32 first_free_slot_idx = ~0u;
		u32 occupied_slot_idx = ~0u;
		this->findSlot<KT>(key, first_free_slot_idx, occupied_slot_idx);

		// If map contains key, replace value and return
		if (occupied_slot_idx != ~0u) {
			sfz_assert(occupied_slot_idx < Capacity);
			SfzHashMapSlot slot = m_slots[occupied_slot_idx];
			u32 idx = slot.index();
			sfz_assert(idx < m_size);
			m_values[idx] = sfz_forward(value);
			return m_values[idx];
		}

		// Calculate next index
		sfz_assert_hard(m_size < Capacity);
		u32 next_free_idx = m_size;
		m_size += 1;

		// Check if previous slot was placeholder and then create new slot
		sfz_assert_hard(first_free_slot_idx < Capacity);
		bool was_placeholder = m_slots[first_free_slot_idx].state() == SfzHashMapSlotState::PLACEHOLDER;
		if (was_placeholder) m_placeholders -= 1;
		m_slots[first_free_slot_idx] = SfzHashMapSlot(SfzHashMapSlotState::OCCUPIED, next_free_idx);

		// Insert key and value
		// Perfect forwarding: const reference: VT == const V&, rvalue: VT == V
		// std::forward<VT>(value) will then return the correct version of value
		m_keys[next_free_idx] = key;
		m_values[next_free_idx] = sfz_forward(value);
		return m_values[next_free_idx];
	}

	template<typename KT>
	bool removeInternal(const KT& key)
	{
		// Finds slots
		u32 first_free_slot_idx = ~0u;
		u32 occupied_slot_idx = ~0u;
		this->findSlot<KT>(key, first_free_slot_idx, occupied_slot_idx);

		// Return false if map does not contain element
		if (occupied_slot_idx == ~0u) return false;

		// Swap the key/value pair with the last key/value pair in the arrays
		u32 last_slot_idx = ~0u;
		{
			sfz_assert(m_size > 0);
			u32 unused = ~0u;
			this->findSlot<K>(m_keys[m_size - 1], unused, last_slot_idx);
			sfz_assert(last_slot_idx != ~0u);
		}
		this->swapElements(occupied_slot_idx, last_slot_idx);

		// Remove the element
		u32 idx = m_slots[occupied_slot_idx].index();
		sfz_assert(idx < m_size);
		m_slots[occupied_slot_idx] = SfzHashMapSlot(SfzHashMapSlotState::PLACEHOLDER, ~0u);
		m_keys[idx] = K();
		m_values[idx] = V();

		// Update info
		m_size -= 1;
		m_placeholders += 1;
		return true;
	}

	// Private members
	// --------------------------------------------------------------------------------------------

	SfzHashMapSlot m_slots[Capacity];
	K m_keys[Capacity];
	V m_values[Capacity];
	u32 m_size = 0;
	u32 m_placeholders = 0;
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
