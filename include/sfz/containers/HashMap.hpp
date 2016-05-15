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

#include "sfz/memory/Allocators.hpp"
#include "sfz/util/Hash.hpp"

namespace sfz {

using std::int64_t;
using std::size_t;
using std::uint32_t;
using std::uint8_t;

// HashMap (interface)
// ------------------------------------------------------------------------------------------------

template<typename K, typename V,
         size_t(*HashFun)(const K&) = sfz::hash<K>, typename Allocator = StandardAllocator>
class HashMap {
public:
	// Constants
	// --------------------------------------------------------------------------------------------

	static constexpr uint32_t ALIGNMENT_EXP = 5;
	static constexpr uint32_t ALIGNMENT = 1 << ALIGNMENT_EXP; // 2^5 = 32
	static constexpr uint32_t MIN_CAPACITY = 67;
	static constexpr uint32_t MAX_CAPACITY = 2147483659;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

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

	V* get(const K& key) noexcept;

	//const V* get(const K& key) const noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	// TODO: Document
	void add(const K& key, const V& value) noexcept;

	/// Swaps the contents of two HashMaps
	void swap(HashMap& other) noexcept;

	/// Removes all elements from this HashMap without deallocating memory or changing capacity
	void clear() noexcept;

	/// Destroys all elements stored in this DynArray and deallocates all memory. After this
	/// method is called the size and capacity is 0. If the HashMap is already empty then this
	/// method will do nothing. It is not necessary to call this method manually, it will
	/// automatically be called in the destructor.
	void destroy() noexcept;

private:
	// Private constants
	// --------------------------------------------------------------------------------------------

	static constexpr uint8_t ELEMENT_INFO_EMPTY = 0;
	static constexpr uint8_t ELEMENT_INFO_REMOVED = 1;
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
	/// found or not is returned through the elementFound parameter.
	uint32_t findElementIndex(const K& key, bool& elementFound) const noexcept;

	/// Finds the idex of the first free slot for a given key. If the key is already in the HashMap
	/// the search stops and keyAlreadyExists is set to true.
	uint32_t findFreeSlot(const K& key, bool& keyAlreadyExists) const noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mSize = 0, mCapacity = 0;
	uint8_t* mDataPtr = nullptr;
};

} // namespace sfz

#include "sfz/containers/HashMap.inl"
