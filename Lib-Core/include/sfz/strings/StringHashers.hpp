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

#include <skipifzero_hash_maps.hpp>
#include <skipifzero_strings.hpp>

#include "sfz/strings/DynString.hpp"

namespace sfz {

using std::size_t;
using std::uint32_t;
using std::uint64_t;

// String hash functions
// ------------------------------------------------------------------------------------------------

/// Hashes a null-terminated raw string. The exact hashing function used is currently FNV-1A, this
/// might however change in the future.
uint64_t hash(const char* str) noexcept;

/// Hashes a DynString, guaranteed to produce the same hash as an equivalent const char*.
uint64_t hash(const DynString& str) noexcept;

/// Hashes a StackString, guaranteed to produce the same hash as an equivalent const char*.
template<uint32_t N>
uint64_t hash(const StringLocal<N>& str) noexcept { return sfz::hash(str.str); }

// DynString & StackString HashMapAltKeyDescr specializations
// ------------------------------------------------------------------------------------------------

template<>
struct HashMapAltKey<DynString> final {
	using AltKeyT = const char*;
};

template<size_t N>
struct HashMapAltKey<StringLocal<N>> final {
	using AltKeyT = const char*;
};

} // namespace sfz
