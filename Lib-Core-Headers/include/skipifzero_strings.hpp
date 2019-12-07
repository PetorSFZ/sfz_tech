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

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "skipifzero.hpp"

namespace sfz {

// StringLocal
// ------------------------------------------------------------------------------------------------

template<uint16_t N>
struct StringLocal final {

	// Public members
	// --------------------------------------------------------------------------------------------

	static_assert(N > 0);

	char str[N];

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	StringLocal() noexcept { str[0] = '\0'; }
	StringLocal(const StringLocal&) noexcept = default;
	StringLocal& operator= (const StringLocal&) noexcept = default;
	~StringLocal() noexcept = default;

	// Constructs a StringLocal with printf syntax. If the string is larger than the capacity then
	// only what fits will be stored. The resulting string is guaranteed to be null-terminated.
	explicit StringLocal(const char* format, ...) noexcept
	{
		va_list args;
		va_start(args, format);
		vsnprintf(this->str, N, format, args);
		va_end(args);
		str[N - 1] = '\0';
	}

	// Public methods
	// --------------------------------------------------------------------------------------------

	uint32_t size() const { return uint32_t(strlen(this->str)); }
	uint32_t capacity() const { return N; }

	// Calls snprintf() on the internal string, overwriting the content.
	void printf(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		vsnprintf(this->str, N, format, args);
		va_end(args);
	}

	// Calls snprintf() on the remaining part of the internal string, effectively appending to it.
	void printfAppend(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		size_t len = strlen(this->str);
		vsnprintf(this->str + len, N - len, format, args);
		va_end(args);
	}

	// Inserts numChars characters into string. Will append null-terminator. Becomes a call to
	// strncpy() internally.
	void insertChars(const char* first, uint32_t numChars)
	{
		sfz_assert(numChars < N);
		strncpy(this->str, first, size_t(numChars));
		this->str[numChars] = '\0';
	}

	// Operators
	// --------------------------------------------------------------------------------------------

	operator char*() { return this->str; }
	operator const char*() const { return this->str; }

	bool operator== (const StringLocal& other) const { return *this == other.str; }
	bool operator!= (const StringLocal& other) const { return *this != other.str; }
	bool operator< (const StringLocal& other) const { return *this < other.str; }
	bool operator<= (const StringLocal& other) const { return *this <= other.str; }
	bool operator> (const StringLocal& other) const { return *this > other.str; }
	bool operator>= (const StringLocal& other) const { return *this >= other.str; }

	bool operator== (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->str, o, N) == 0; }
	bool operator!= (const char* o) const { sfz_assert(o != nullptr); return !(*this == o); }
	bool operator< (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->str, o, N) < 0; }
	bool operator<= (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->str, o, N) <= 0; }
	bool operator> (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->str, o, N) > 0; }
	bool operator>= (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->str, o, N) >= 0; }
};

using str32 = StringLocal<32>;
using str48 = StringLocal<48>;
using str64 = StringLocal<64>;
using str80 = StringLocal<80>;
using str96 = StringLocal<96>;
using str128 = StringLocal<128>;
using str192 = StringLocal<192>;
using str256 = StringLocal<256>;
using str320 = StringLocal<320>;
using str512 = StringLocal<512>;
using str1024 = StringLocal<1024>;
using str2048 = StringLocal<2048>;
using str4096 = StringLocal<4096>;

// const char* is an alternate type to StringLocal
template<uint16_t N>
struct AltType<StringLocal<N>> final {
	using AltT = const char*;
};

// String hashing
// ------------------------------------------------------------------------------------------------

// FNV-1a hash function, based on public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
// See http://isthe.com/chongo/tech/comp/fnv/
constexpr uint64_t hashStringFNV1a(const char* str)
{
	constexpr uint64_t FNV_64_MAGIC_PRIME = uint64_t(0x100000001B3);

	// Set initial value to FNV-0 hash of "chongo <Landon Curt Noll> /\../\"
	uint64_t tmp = uint64_t(0xCBF29CE484222325);

	// Hash all bytes in string
	while (char c = *str++) {
		tmp ^= uint64_t(c); // Xor bottom with current byte
		tmp *= FNV_64_MAGIC_PRIME; // Multiply with FNV magic prime
	}
	return tmp;
}

// Alternate version of hashStringFNV1a() which hashes a number of raw bytes (i.e. not a string)
constexpr uint64_t hashBytesFNV1a(const uint8_t* bytes, uint64_t numBytes)
{
	constexpr uint64_t FNV_64_MAGIC_PRIME = uint64_t(0x100000001B3);

	// Set initial value to FNV-0 hash of "chongo <Landon Curt Noll> /\../\"
	uint64_t tmp = uint64_t(0xCBF29CE484222325);

	// Hash all bytes
	for (uint64_t i = 0; i < numBytes; i++) {
		tmp ^= uint64_t(bytes[i]); // Xor bottom with current byte
		tmp *= FNV_64_MAGIC_PRIME; // Multiply with FNV magic prime
	}
	return tmp;
}

// Hash strings with FNV-1a by default
constexpr uint64_t hash(const char* str) { return hashStringFNV1a(str); }

template<uint16_t N>
constexpr uint64_t hash(const StringLocal<N>& str) { return hashStringFNV1a(str); }

} // namespace sfz
