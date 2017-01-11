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

namespace sfz {

using std::size_t;

// StackString template
// ------------------------------------------------------------------------------------------------

/// A simple pod struct holding a fixed size string allocated in local memory (i.e. not on the
/// heap). Useful for small temporary strings or as part of larger objects allocated on the heap.
///
/// As a StackString is quite large if used a lot and improperly it could put significant pressure
/// on the stack and potentially cause stack overflows. Use it responsibly.
///
/// StackString is a template, but it is defined in a compilation unit (.cpp file). This means
/// that only the predefined sizes is available. The default size is 96 chars, equivalent to the
/// size of 12 64bit words. For other available sizes see StackString types further down in
/// this file.
template<size_t N>
struct StackStringTempl final {

	// Public members
	// --------------------------------------------------------------------------------------------

	char str[N];

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	StackStringTempl() noexcept;
	StackStringTempl(const StackStringTempl&) noexcept = default;
	StackStringTempl& operator= (const StackStringTempl&) noexcept = default;
	~StackStringTempl() noexcept = default;

	/// Constructs a StackString with the given string. If the string is larger than the capacity
	/// of this StackString then only what fits will be stored. The resulting StackString is
	/// guaranteed to be null-terminated.
	explicit StackStringTempl(const char* string) noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Calls snprintf() on the internal string, overwriting the content.
	void printf(const char* format, ...) noexcept;

	/// Calls snprintf() on the remaining part of the internal string, effectively appending to it.
	void printfAppend(const char* format, ...) noexcept;

	/// Inserts numChars characters into string. Will append null-terminator. Becomes a call to
	/// strncpy() internally.
	void insertChars(const char* first, size_t numChars) noexcept;

	// Operators
	// --------------------------------------------------------------------------------------------

	bool operator== (const StackStringTempl& other) const noexcept;
	bool operator!= (const StackStringTempl& other) const noexcept;
	bool operator< (const StackStringTempl& other) const noexcept;
	bool operator<= (const StackStringTempl& other) const noexcept;
	bool operator> (const StackStringTempl& other) const noexcept;
	bool operator>= (const StackStringTempl& other) const noexcept;

	bool operator== (const char* other) const noexcept;
	bool operator!= (const char* other) const noexcept;
	bool operator< (const char* other) const noexcept;
	bool operator<= (const char* other) const noexcept;
	bool operator> (const char* other) const noexcept;
	bool operator>= (const char* other) const noexcept;
};

// StackString types
// ------------------------------------------------------------------------------------------------

using StackString32 = StackStringTempl<32>; // Size: 4 64bit words
using StackString64 = StackStringTempl<64>; // Size: 8 64bit words
using StackString96 = StackStringTempl<96>; // Size: 12 64bit words
using StackString128 = StackStringTempl<128>; // Size: 16 64bit words
using StackString192 = StackStringTempl<192>; // Size: 24 64bit words
using StackString256 = StackStringTempl<256>; // Size: 32 64bit words
using StackString320 = StackStringTempl<320>; // Size: 40 64bit words
using StackString512 = StackStringTempl<512>; // Size: 64 64bit words
using StackString1024 = StackStringTempl<1024>; // Size: 128 64bit words

using StackString = StackString96;

} // namespace sfz
