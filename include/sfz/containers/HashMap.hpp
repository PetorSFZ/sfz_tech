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
#include "sfz/containers/HashTableKeyDescriptor.hpp"
#include "sfz/memory/Allocator.hpp"

namespace sfz {

using std::size_t;
using std::uint8_t;
using std::uint32_t;
using std::uint64_t;

static_assert(sizeof(size_t) == sizeof(uint64_t), "size_t is not 64 bit");

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
/// An alternate key type can be specified in the HashTableKeyDescriptor. This alt key can be used
/// in most methods instead of the normal key type. This is mostly useful when string classes are
/// used as keys, then const char* can be used as an alt key type. This removes the need to create
/// a temporary key object (which might need to allocate memory). As specified in the
/// HashTableKeyDescriptor documentation, the normal key type needs to be constructable using an
/// alt key.
///
/// HashMap uses sfzCore allocators (read more about them in sfz/memory/Allocator.hpp). Basically
/// they are instance based allocators, so each HashMap needs to have an Allocator pointer. The
/// default constructor does not set any allocator (i.e. nullptr) and does not allocate any memory.
/// An allocator can be set via the create() method or the constructor that takes an allocator.
/// Once an allocator is set it can not be changed unless the HashMap is first destroy():ed, this
/// is done automatically if create() is called again. If no allocator is available (nullptr) when
/// attempting to allocate memory (rehash(), put(), etc), then the default allocator will be
/// retrieved (getDefaultAllocator()) and set.
///
/// \param K the key type
/// \param V the value type
/// \param Descr the HashTableKeyDescriptor (by default sfz::HashTableKeyDescriptor)
template<typename K, typename V, typename Descr = HashTableKeyDescriptor<K>>
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

	// Typedefs
	// --------------------------------------------------------------------------------------------

	using KeyHash = typename Descr::KeyHash;
	using KeyEqual = typename Descr::KeyEqual;

	using AltK = typename Descr::AltKeyT;
	using AltKeyHash = typename Descr::AltKeyHash;
	using AltKeyKeyEqual = typename Descr::AltKeyKeyEqual;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	/// Constructs a new HashMap using create()
	explicit HashMap(uint32_t suggestedCapacity, Allocator* allocator = getDefaultAllocator()) noexcept;

	/// Creates an empty HashMap without setting an allocator or allocating any memory.
	HashMap() noexcept = default;

	/// Copy constructors. Copies content and allocator.
	HashMap(const HashMap& other) noexcept;
	HashMap& operator= (const HashMap& other) noexcept;

	/// Copy constructor that change allocator. Copies content but uses the specific allocator for
	/// the copy instead of the original one.
	HashMap(const HashMap& other, Allocator* allocator) noexcept;

	/// Move constructors. Equivalent to calling target.swap(source).
	HashMap(HashMap&& other) noexcept;
	HashMap& operator= (HashMap&& other) noexcept;
	
	/// Destroys the HashMap using destroy().
	~HashMap() noexcept;

	// State methods
	// --------------------------------------------------------------------------------------------

	/// Calls destroy(), then constucts a new HashMap. Capacity will be larger than or equal to the
	/// suggested capacity.
	void create(uint32_t suggestedCapacity, Allocator* allocator = getDefaultAllocator()) noexcept;

	/// Swaps the contents of two HashMaps, including the allocators.
	void swap(HashMap& other) noexcept;

	/// Destroys all elements stored in this DynArray, deallocates all memory and removes allocator.
	/// After this method is called the size and capacity is 0, allocator is nullptr. If the HashMap
	/// is already empty then this method will only remove the allocator if it exists. It is not
	/// necessary to call this method manually, it will automatically be called in the destructor.
	void destroy() noexcept;

	/// Removes all elements from this HashMap without deallocating memory, changing capacity or
	/// touching the allocator.
	void clear() noexcept;

	/// Rehashes this HashMap. Creates a new HashMap with at least the same capacity as the
	/// current one, or larger if suggested by suggestedCapacity. Then iterates over all elements
	/// in this HashMap and adds them to the new one. Finally this HashMap is replaced by the
	/// new one. Obviously all pointers and references into the old HashMap are invalidated. If no
	/// allocator is set then the default one will be retrieved and set.
	void rehash(uint32_t suggestedCapacity) noexcept;

	/// Checks if HashMap needs to be rehashed, and will do so if necessary. This method is
	/// internally called by put() and operator[]. Will allocate capacity if this HashMap is
	/// empty. Returns whether HashMap was rehashed.
	bool ensureProperlyHashed() noexcept;

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

	/// Returns the allocator of this HashMap. Will return nullptr if no allocator is set.
	Allocator* allocator() const noexcept { return mAllocator; }

	/// Returns pointer to the element associated with the given key, or nullptr if no such element
	/// exists. The pointer is owned by this HashMap and will not be valid if it is rehashed,
	/// which can automatically occur if for example keys are inserted via put() or operator[].
	/// Instead it is recommended to make a copy of the returned value. This method is guaranteed
	/// to never change the state of the HashMap (by causing a rehash), the pointer returned can
	/// however be used to modify the stored value for an element.
	V* get(const K& key) noexcept;
	const V* get(const K& key) const noexcept;
	V* get(const AltK& key) noexcept;
	const V* get(const AltK& key) const noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Adds the specified key value pair to this HashMap. If a value is already associated with
	/// the given key it will be replaced with the new value. Returns a reference to the element
	/// set. As usual, the reference will be invalidated if the HashMap is rehashed, so be careful.
	/// This method will always call ensureProperlyHashed(), which might trigger a rehash.
	///
	/// In particular the following scenario presents a dangerous trap:
	/// V& ref1 = m.put(key1, value1);
	/// V& ref2 = m.put(key2, value2);
	/// At this point only ref2 is guaranteed to be valid, as the second call might have triggered
	/// a rehash. In this particular example consider ignoring the reference returned and instead
	/// retrieve pointers via the get() method (which is guaranteed to not cause a rehash) after
	/// all the keys have been inserted.
	V& put(const K& key, const V& value) noexcept;
	V& put(const K& key, V&& value) noexcept;
	V& put(K&& key, const V& value) noexcept;
	V& put(K&& key, V&& value) noexcept;
	V& put(const AltK& key, const V& value) noexcept;
	V& put(const AltK& key, V&& value) noexcept;

	/// Access operator, will return a reference to the element associated with the given key. If
	/// no such element exists it will be created with the default constructor. This method is
	/// implemented by a call to get(), and then a call to put() if no element existed. In
	/// practice this means that this function is guaranteed to not rehash if the requested
	/// element already exists. This might be dangerous to rely on, so get() should be preferred
	/// if rehashing needs to be avoided. As always, the reference will be invalidated if the
	/// HashMap is rehashed.
	V& operator[] (const K& key) noexcept;
	V& operator[] (K&& key) noexcept;
	V& operator[] (const AltK& key) noexcept;

	/// Attempts to remove the element associated with the given key. Returns false if this
	/// HashMap contains no such element. Guaranteed to not rehash.
	bool remove(const K& key) noexcept;
	bool remove(const AltK& key) noexcept;

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
	uint64_t sizeOfElementInfoArray() const noexcept;

	/// Returns the size of the memory allocation for the key array in bytes
	uint64_t sizeOfKeyArray() const noexcept;

	/// Returns the size of the memory allocation for the value array in bytes
	uint64_t sizeOfValueArray() const noexcept;

	/// Returns the size of the allocated memory in bytes
	uint64_t sizeOfAllocatedMemory() const noexcept;

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
	/// KT: The key type, either K or AltK
	/// KeyHash: Hasher for KeyT
	/// KeyEqual: Comparer for KeyT and K (I.e. KeyEqual for K and AltKeyKeyEqual for AltK)
	template<typename KT, typename Hash, typename Equal>
	uint32_t findElementIndex(const KT& key, bool& elementFound, uint32_t& firstFreeSlot,
	                          bool& isPlaceholder) const noexcept;
	
	/// Internal shared implementation of all get() methods
	template<typename KT, typename Hash, typename Equal>
	V* getInternal(const KT& key) const noexcept;

	/// Internal shared implementation of all put() methods
	template<typename KT, typename VT, typename Hash, typename Equal>
	V& putInternal(KT&& key, VT&& value) noexcept;

	/// Internal shared implementation of all remove() methods
	template<typename KT, typename Hash, typename Equal>
	bool removeInternal(const KT& key) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mSize = 0, mCapacity = 0, mPlaceholders = 0;
	uint8_t* mDataPtr = nullptr;
	Allocator* mAllocator = nullptr;
};

} // namespace sfz

#include "sfz/containers/HashMap.inl"
