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
#include <functional> // std::hash

#include "sfz/memory/Allocator.hpp"

namespace sfz {

using std::uint64_t;
using std::size_t;

static_assert(sizeof(uint64_t) == sizeof(size_t), "size_t must be 64 bit");

// StringID struct
// ------------------------------------------------------------------------------------------------

/// Struct representing the hash of a string. Used to be able to use strings equality comparisons
/// in contexts where actually comparing strings each time would be to expensive. StringIDs should
/// always be created by a StringCollection.
struct StringID final {
	uint64_t id;
};

inline bool operator== (const StringID& lhs, const StringID& rhs) noexcept
{
	return lhs.id == rhs.id;
}

inline bool operator!= (const StringID& lhs, const StringID& rhs) noexcept
{
	return lhs.id != rhs.id;
}

// StringCollection class
// ------------------------------------------------------------------------------------------------

struct StringCollectionImpl; // Pimpl

/// A StringCollection is a collection of registered strings. When a string is registered a unique
/// identifier (StringID) is returned. This identifier is way cheaper to compare for equality than
/// an actual string, which is useful in for example a game engine context.
///
/// In rare cases when two strings have the same has a string collision can occur. If this happens
/// the StringCollection will print the strings and exit the program via sfz::error(). This can be
/// fixed by slightly altering one of the offending strings.
class StringCollection final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------
	
	StringCollection() noexcept = default;
	StringCollection(const StringCollection&) = delete;
	StringCollection& operator= (const StringCollection&) = delete;

	/// Creates a StringCollection by calling createStringCollection()
	StringCollection(uint32_t initialCapacity, Allocator* allocator) noexcept;

	/// Moves this StringCollection (by calling swap())
	StringCollection(StringCollection&& other) noexcept;
	StringCollection& operator= (StringCollection&& other) noexcept;

	/// Destroys this StringCollection by calling destroy()
	~StringCollection() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void createStringCollection(uint32_t initialCapacity, Allocator* allocator) noexcept;
	void swap(StringCollection& other) noexcept;
	void destroy() noexcept;

	/// Returns the current number of strings registered with this StringCollection
	uint32_t numStringsHeld() const noexcept;

	/// Registers a string with this StringCollection and returns its corresponding StringID.
	/// This method is fairly expensive to call, so the StringID should be kept and reused.
	/// If a string collision occurs (i.e., two strings have the same hash) this method will call
	/// sfz::error() and exit the program.
	StringID getStringID(const char* string) noexcept;

	/// Returns the string associated with the given StringID. Returns nullptr if no such string
	/// exists. The pointer is owned by the StringCollection, but it is safe to store it as long
	/// as this StringCollection is not destroy():ed.
	const char* getString(StringID id) const noexcept;

private:
	StringCollectionImpl* mImpl = nullptr;
};

} // namespace sfz

// Specialization of std::hash for StringID
// ------------------------------------------------------------------------------------------------

namespace std {

template<>
struct hash<sfz::StringID> {
	inline size_t operator() (const sfz::StringID& str) const noexcept { return str.id; }
};

} // namespace std
