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
#include <cstdint>
#include <cstring>
#include <functional>
#include <new> // Placement new

#include "sfz/Assert.hpp"
#include "sfz/memory/Allocators.hpp"

namespace sfz {

using std::int64_t;
using std::size_t;
using std::uint32_t;
using std::uint8_t;

// HashMap (interface)
// ------------------------------------------------------------------------------------------------

/// A HashMap with closed hashing (open adressing).
///
/// Quadratic probing is used in the case of collisions, which can not guarantee that more than
/// half the slots will be searched. For this reason the load factor is 49%, which should
/// guarantee that this never poses a problem.
///
/// The capacity of the the HashMap is always a prime number, so when a certain capacity is
/// suggested a prime bigger than the suggestion will simply be taken from an internal lookup
/// table. In the case of a rehash the capacity generally increases by (approximately) a factor
/// of 2.
///
/// Removal of elements is O(1), but will leave a placeholder on the previously occupied slot. The
/// current number of placeholders can be queried by the placeholders() method. Both size and 
/// placeholders count as load when checking if the HashMap needs to be rehashed or not.
///
/// \param K the key type
/// \param V the value type
/// \param Hash the hash function (by default std::hash)
/// \param KeyEqual the function used to compare keys (by default operator ==)
/// \param Allocator the sfz allocator used to allocate memory
template<typename K, typename V, typename Hash = std::hash<K>,
         typename KeyEqual = std::equal_to<K>, typename Allocator = StandardAllocator>
class HashMap {
public:
	// Constants
	// --------------------------------------------------------------------------------------------

	static constexpr uint32_t ALIGNMENT_EXP = 5;
	static constexpr uint32_t ALIGNMENT = 1 << ALIGNMENT_EXP; // 2^5 = 32
	static constexpr uint32_t MIN_CAPACITY = 67;
	static constexpr uint32_t MAX_CAPACITY = 2147483659;

	/// This factor decides the maximum number of occupied slots (size + placeholders) this
	/// HashMap may contain before it is rehashed by ensureProperlyHashed().
	static constexpr float MAX_OCCUPIED_REHASH_FACTOR = 0.49f;

	/// This factor decides the maximum size allowed to not increase the capacity when rehashing
	/// in ensureProperlyHashed(). For example, size 20% and placeholders 30% would trigger a
	/// rehash, but would not increase capacity. Size 40% and placeholders 10% would trigger a
	/// rehash with capacity increase.
	static constexpr float MAX_SIZE_KEEP_CAPACITY_FACTOR = 0.35f;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	/// Constructs a new HashMap with a capacity larger than or equal to the suggested capacity.
	/// If suggestedCapacity is 0 then no memory will be allocated. Equivalent to creating an empty
	/// HashMap by default constructor and then calling rehash with the suggested capacity.
	explicit HashMap(uint32_t suggestedCapacity) noexcept;

	HashMap() noexcept = default;
	HashMap(const HashMap& other) noexcept;
	HashMap& operator= (const HashMap& other) noexcept;
	HashMap(HashMap&& other) noexcept;
	HashMap& operator= (HashMap&& other) noexcept;
	~HashMap() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	/// Returns the size of this HashMap. This is the number of elements stored, not the current
	/// capacity.
	uint32_t size() const noexcept { return mSize; }

	/// Returns the capacity of this HashMap.
	uint32_t capacity() const noexcept { return mCapacity; }

	/// Returns the number of placeholder positions for removed elements. size + placeholders <=
	/// capacity.
	uint32_t placeholders() const noexcept { return mPlaceholders; }

	/// Returns pointer to the element associated with the given key. The pointer is owned by this
	/// HashMap and will not necessarily be valid if any non-const operations are done to this
	/// HashMap that can change the internal capacity, so make a copy if you intend to keep the
	/// value. Returns nullptr if no element is associated with the given key.
	V* get(const K& key) noexcept;

	/// Returns pointer to the element associated with the given key. The pointer is owned by this
	/// HashMap and will not necessarily be valid if any non-const operations are done to this
	/// HashMap that can change the internal capacity, so make a copy if you intend to keep the
	/// value. Returns nullptr if no element is associated with the given key.
	const V* get(const K& key) const noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Adds the specified key value pair to this HashMap. If a value is already associated with
	/// the given key it will be replaced with the new value. Will call ensureProperlyHashed().
	void put(const K& key, const V& value) noexcept;

	/// Adds the specified key value pair to this HashMap. If a value is already associated with
	/// the given key it will be replaced with the new value. Will call ensureProperlyHashed().
	void put(const K& key, V&& value) noexcept;

	/// Access operator, will return a reference to the element associated with the given key. If
	/// no such element exists it will be created with the default constructor. As always, the
	/// reference will be invalidated if the HashMap is resized. So store a copy if you intend to
	/// keep it. Will call ensureProperlyHashed() if capacity is 0 or if adding a key value pair
	/// to the HashMap.
	V& operator[] (const K& key) noexcept;

	/// Attempts to remove the element associated with the given key. Returns false if this
	/// HashMap contains no such element. 
	bool remove(const K& key) noexcept;

	/// Swaps the contents of two HashMaps
	void swap(HashMap& other) noexcept;

	/// Rehashes this HashMap. Creates a new HashMap with at least the same capacity as the
	/// current one, or larger if suggested by suggestedCapacity. Then iterates over all elements
	/// in this HashMap and adds them to the new one. Finally this HashMap is replaced by the
	/// new one. Obviously all pointers and references into the old HashMap are invalidated.
	void rehash(uint32_t suggestedCapacity) noexcept;

	/// Checks if HashMap needs to be rehashed, and will do so if necessary. This method is
	/// internally called by put() and operator[], if it is manually called before adding a single
	/// key value pair with these methods they are guaranteed to not rehash. Will allocate capacity
	/// if this HashMap is empty. Returns whether HashMap was rehashed.
	bool ensureProperlyHashed() noexcept;

	/// Removes all elements from this HashMap without deallocating memory or changing capacity
	void clear() noexcept;

	/// Destroys all elements stored in this DynArray and deallocates all memory. After this
	/// method is called the size and capacity is 0. If the HashMap is already empty then this
	/// method will do nothing. It is not necessary to call this method manually, it will
	/// automatically be called in the destructor.
	void destroy() noexcept;

	// Iterators
	// --------------------------------------------------------------------------------------------

	/// The return value when dereferencing an iterator. Contains references into the HashMap in
	/// question, so it is only valid as long as no rehashing is performed.
	struct KeyValuePair final {
		const K& key; // Const so user doesn't change key, breaking invariants of the HashMap
		V& value;
		KeyValuePair(const K& key, V& value) noexcept : key(key), value(value) { }
		KeyValuePair(const KeyValuePair&) noexcept = default;
		KeyValuePair& operator= (const KeyValuePair&) = delete; // Because references...
	};

	/// The normal non-const iterator for HashMap.
	class Iterator final {
	public:
		Iterator(HashMap& hashMap, uint32_t index) noexcept : mHashMap(&hashMap), mIndex(index) { }
		Iterator(const Iterator&) noexcept = default;
		Iterator& operator= (const Iterator&) noexcept = default;

		Iterator& operator++ () noexcept; // Pre-increment
		Iterator operator++ (int) noexcept; // Post-increment
		KeyValuePair operator* () noexcept;
		bool operator== (const Iterator& other) const noexcept;
		bool operator!= (const Iterator& other) const noexcept;

	private:
		HashMap* mHashMap;
		uint32_t mIndex;
	};

	/// The return value when dereferencing a const iterator. Contains references into the HashMap
	/// in question, so it is only valid as long as no rehashing is performed.
	struct ConstKeyValuePair final {
		const K& key;
		const V& value;
		ConstKeyValuePair(const K& key, const V& value) noexcept : key(key), value(value) {}
		ConstKeyValuePair(const ConstKeyValuePair&) noexcept = default;
		ConstKeyValuePair& operator= (const ConstKeyValuePair&) = delete; // Because references...
	};

	/// The const iterator for HashMap
	class ConstIterator final {
	public:
		ConstIterator(const HashMap& hashMap, uint32_t index) noexcept : mHashMap(&hashMap), mIndex(index) {}
		ConstIterator(const ConstIterator&) noexcept = default;
		ConstIterator& operator= (const ConstIterator&) noexcept = default;

		ConstIterator& operator++ () noexcept; // Pre-increment
		ConstIterator operator++ (int) noexcept; // Post-increment
		ConstKeyValuePair operator* () noexcept;
		bool operator== (const ConstIterator& other) const noexcept;
		bool operator!= (const ConstIterator& other) const noexcept;

	private:
		const HashMap* mHashMap;
		uint32_t mIndex;
	};

	// Iterator methods
	// --------------------------------------------------------------------------------------------

	Iterator begin() noexcept;
	ConstIterator begin() const noexcept;
	ConstIterator cbegin() const noexcept;

	Iterator end() noexcept;
	ConstIterator end() const noexcept;
	ConstIterator cend() const noexcept;

private:
	// Private constants
	// --------------------------------------------------------------------------------------------

	static constexpr uint8_t ELEMENT_INFO_EMPTY = 0;
	static constexpr uint8_t ELEMENT_INFO_PLACEHOLDER = 1;
	static constexpr uint8_t ELEMENT_INFO_OCCUPIED = 2;

	// Private methods
	// --------------------------------------------------------------------------------------------

	/// Returns a prime number larger than the suggested capacity
	uint32_t findPrimeCapacity(uint32_t capacity) const noexcept;

	/// Return the size of the memory allocation for the element info array in bytes
	size_t sizeOfElementInfoArray() const noexcept;

	/// Returns the size of the memory allocation for the key array in bytes
	size_t sizeOfKeyArray() const noexcept;

	/// Returns the size of the memory allocation for the value array in bytes
	size_t sizeOfValueArray() const noexcept;

	/// Returns the size of the allocated memory in bytes
	size_t sizeOfAllocatedMemory() const noexcept;

	/// Returns pointer to the info bits part of the allocated memory
	uint8_t* elementInfoPtr() const noexcept;

	/// Returns pointer to the key array part of the allocated memory
	K* keysPtr() const noexcept;

	/// Returns pointer to the value array port fo the allocated memory
	V* valuesPtr() const noexcept;

	/// Returns the 2 bit element info about an element position in the HashMap
	/// 0 = empty, 1 = removed, 2 = occupied, (3 is unused)
	uint8_t elementInfo(uint32_t index) const noexcept;

	/// Sets the 2 bit element info with the selected value
	void setElementInfo(uint32_t index, uint8_t value) noexcept;

	/// Finds the index of an element associated with the specified key. Whether an element is
	/// found or not is returned through the elementFound parameter. The first free slot found is
	/// sent back through the firstFreeSlot parameter, if no free slot is found it will be set to
	/// ~0. Whether the found free slot is a placeholder slot or not is sent back through the
	/// isPlaceholder parameter.
	uint32_t findElementIndex(const K& key, bool& elementFound, uint32_t& firstFreeSlot, bool& isPlaceholder) const noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mSize = 0, mCapacity = 0, mPlaceholders = 0;
	uint8_t* mDataPtr = nullptr;
};

} // namespace sfz

#include "sfz/containers/HashMap.inl"
