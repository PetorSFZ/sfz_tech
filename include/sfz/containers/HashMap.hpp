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

	static constexpr uint32_t ALIGNMENT = 32;
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

	// Public methods
	// --------------------------------------------------------------------------------------------

	void clear() noexcept;

	void destroy() noexcept;


private:
	// Private constants
	// --------------------------------------------------------------------------------------------

	static constexpr uint8_t BIT_INFO_EMPTY = 0;
	static constexpr uint8_t BIT_INFO_REMOVED = 1;
	static constexpr uint8_t BIT_INFO_OCCUPIED = 2;

	// Private methods
	// --------------------------------------------------------------------------------------------

	/// Returns a prime number larger than the suggested capacity
	uint32_t findPrimeCapacity(uint32_t capacity) const noexcept;

	size_t sizeofInfoBitsArray() const noexcept;
	
	uint8_t elementInfo(uint32_t index) const noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mSize = 0, mCapacity = 0;
	uint8_t* mInfoBitsPtr = nullptr; // 2 bits per element, 0 = empty, 1 = removed, 2 = occupied
	K* mKeysPtr = nullptr;
	V* mValuesPtr = nullptr;
};

} // namespace sfz

#include "sfz/containers/HashMap.inl"
