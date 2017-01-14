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

#include <cstdint>
#include <functional> // std::hash

#include "sfz/containers/HashTableKeyDescriptor.hpp"
#include "sfz/strings/DynString.hpp"
#include "sfz/strings/StackString.hpp"

namespace sfz {

using std::size_t;
using std::uint64_t;

static_assert(sizeof(uint64_t) == sizeof(size_t), "size_t is not 64 bit");

// String hash functions
// ------------------------------------------------------------------------------------------------

/// Hashes a null-terminated raw string. The exact hashing function used is currently FNV-1A, this
/// might however change in the future.
size_t hash(const char* str) noexcept;

/// Hashes a DynString, guaranteed to produce the same hash as an equivalent const char*.
size_t hash(const DynString& str) noexcept;

/// Hashes a StackString, guaranteed to produce the same hash as an equivalent const char*.
template<size_t N>
size_t hash(const StackStringTempl<N>& str) noexcept { return sfz::hash(str.str); }

// Raw string hash specializations
// ------------------------------------------------------------------------------------------------

/// Struct with same interface as std::hash, hashes raw strings with sfz::hash()
struct RawStringHash {
	size_t operator() (const char* str) const noexcept;
};

/// Struct with same interface as std::equal_to, compares to raw strings with strcmp().
struct RawStringEqual {
	bool operator()(const char* lhs, const char* rhs) const;
};

/// Specialization of HashTableKeyDescriptor for const char*, makes it possible to use raw strings
/// as keys in hash tables.
template<>
struct HashTableKeyDescriptor<const char*> final {
	using KeyT = const char*;
	using KeyHash = RawStringHash;
	using KeyEqual = RawStringEqual;

	using AltKeyT = NO_ALT_KEY_TYPE;
	using AltKeyHash = NO_ALT_KEY_TYPE;
	using AltKeyKeyEqual = NO_ALT_KEY_TYPE;
};

} // namespace sfz

namespace std {

// DynString & StackString std::hash specializations
// ------------------------------------------------------------------------------------------------

template<>
struct hash<sfz::DynString> {
	size_t operator() (const sfz::DynString& str) const noexcept;
};

template<size_t N>
struct hash<sfz::StackStringTempl<N>> {
	size_t operator() (const sfz::StackStringTempl<N>& str) const noexcept { return sfz::hash<N>(str); }
};

} // namespace std

namespace sfz {

// DynString & StackString HashTableKeyDescriptor specializations
// ------------------------------------------------------------------------------------------------

template<>
struct EqualTo2<const char*, DynString> final {
	bool operator() (const char* lhs, const DynString& rhs) noexcept { return rhs == lhs; }
};

template<>
struct HashTableKeyDescriptor<DynString> final {
	using KeyT = DynString;
	using KeyHash = std::hash<KeyT>;
	using KeyEqual = std::equal_to<KeyT>;

	using AltKeyT = const char*;
	using AltKeyHash = RawStringHash;
	using AltKeyKeyEqual = EqualTo2<AltKeyT, KeyT>;
};

template<size_t N>
struct EqualTo2<const char*, StackStringTempl<N>> final {
	bool operator() (const char* lhs, const StackStringTempl<N>& rhs) noexcept { return rhs == lhs; }
};

template<size_t N>
struct HashTableKeyDescriptor<StackStringTempl<N>> final {
	using KeyT = StackStringTempl<N>;
	using KeyHash = std::hash<KeyT>;
	using KeyEqual = std::equal_to<KeyT>;

	using AltKeyT = const char*;
	using AltKeyHash = RawStringHash;
	using AltKeyKeyEqual = EqualTo2<AltKeyT, KeyT>;
};

} // namespace sfz
