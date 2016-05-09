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

#include "sfz/memory/Allocators.hpp"
#include "sfz/util/Hash.hpp"

namespace sfz {

using std::size_t;
using std::uint32_t;
using std::uint64_t;

// HashMap (interface)
// ------------------------------------------------------------------------------------------------

/// HashMap using open adressing.
/// TODO: Custom bitset class (to check whether a position is in use or not)
/// can use popcnt on the bitset class to check how many elements in HashMap, should be reasonably
/// fast
/// +1 on failure ("linear probing")
/// 

template<typename K, typename V,
         size_t(*HashFun)(const K&) = sfz::hash<K>, typename Allocator = StandardAllocator>
class HashMap {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

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

	// Operators
	// --------------------------------------------------------------------------------------------

	V& operator[] (const K& key) noexcept;

	const V& operator[] (const K& key) const noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------



private:
	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mSize = 0, mCapacity = 0;
	uint64_t* mExistsBitSet = nullptr;
	K* mKeys = nullptr;
	V* mValues = nullptr;
};

} // namespace sfz

#include "sfz/containers/HashMap.inl"
