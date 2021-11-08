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

#ifndef SKIPIFZERO_STRINGS_HPP
#define SKIPIFZERO_STRINGS_HPP
#pragma once

#include <cstdarg>
#include <cstdio>
#include <cctype>

#include "skipifzero.hpp"

#ifdef SFZ_STR_ID_IMPLEMENTATION
#include "skipifzero_hash_maps.hpp"
#endif

// String hashing
// ------------------------------------------------------------------------------------------------

namespace sfz {

// FNV-1a hash function, based on public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
// See http://isthe.com/chongo/tech/comp/fnv/
constexpr u64 hashStringFNV1a(const char* str)
{
	constexpr u64 FNV_64_MAGIC_PRIME = u64(0x100000001B3);

	// Set initial value to FNV-0 hash of "chongo <Landon Curt Noll> /\../\"
	u64 tmp = u64(0xCBF29CE484222325);

	// Hash all bytes in string
	while (char c = *str++) {
		tmp ^= u64(c); // Xor bottom with current byte
		tmp *= FNV_64_MAGIC_PRIME; // Multiply with FNV magic prime
	}
	return tmp;
}

// Alternate version of hashStringFNV1a() which hashes a number of raw bytes (i.e. not a string)
constexpr u64 hashBytesFNV1a(const u8* bytes, u64 numBytes)
{
	constexpr u64 FNV_64_MAGIC_PRIME = u64(0x100000001B3);

	// Set initial value to FNV-0 hash of "chongo <Landon Curt Noll> /\../\"
	u64 tmp = u64(0xCBF29CE484222325);

	// Hash all bytes
	for (u64 i = 0; i < numBytes; i++) {
		tmp ^= u64(bytes[i]); // Xor bottom with current byte
		tmp *= FNV_64_MAGIC_PRIME; // Multiply with FNV magic prime
	}
	return tmp;
}

// Hash strings with FNV-1a by default
constexpr u64 hash(const char* str) { return hashStringFNV1a(str); }

} // namespace sfz

// SfzStrID
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C SfzStrID sfzStrIDCreate(const char* str);
SFZ_EXTERN_C const char* sfzStrIDGetStr(SfzStrID id);

namespace sfz {
constexpr u64 hash(SfzStrID str) { return str.id; }
}

#ifdef SFZ_STR_ID_IMPLEMENTATION

struct SfzStringStorage final {
	SfzAllocator* allocator = nullptr;
	sfz::HashMap<SfzStrID, char*> strs;

	SfzStringStorage(u32 initialCapacity, SfzAllocator* allocator)
	{
		this->allocator = allocator;
		this->strs.init(initialCapacity, allocator, sfz_dbg(""));
	}

	SfzStringStorage(const SfzStringStorage&) = delete;
	SfzStringStorage& operator= (const SfzStringStorage&) = delete;

	~SfzStringStorage()
	{
		for (auto pair : strs) {
			char* str = pair.value;
			allocator->dealloc(str);
		}
		strs.destroy();
		allocator = nullptr;
	}
};

static SfzStringStorage* sfzStrStorage = nullptr;

SFZ_EXTERN_C SfzStrID sfzStrIDCreate(const char* str)
{
	sfz_assert(sfzStrStorage != nullptr);
	SfzStrID id = SFZ_STR_ID_NULL;
	id.id = sfz::hash(str);
	sfz_assert_hard(id != SFZ_STR_ID_NULL);
	
	// Add string to storage and check for collisions
	char** storedStr = sfzStrStorage->strs.get(id);
	if (storedStr == nullptr) {
		u32 strLen = u32(strlen(str));
		char* newStr = reinterpret_cast<char*>(sfzStrStorage->allocator->alloc(sfz_dbg(""), strLen + 1));
		memcpy(newStr, str, strLen);
		newStr[strLen] = '\0';
		storedStr = &sfzStrStorage->strs.put(id, newStr);
	}
	sfz_assert_hard(strcmp(str, *storedStr) == 0);

	return id;
}

SFZ_EXTERN_C const char* sfzStrIDGetStr(SfzStrID id)
{
	sfz_assert(sfzStrStorage != nullptr);
	const char* const* strPtr = sfzStrStorage->strs.get(id);
	if (strPtr == nullptr) return "";
	return *strPtr;
}

#endif

// StringLocal
// ------------------------------------------------------------------------------------------------

namespace sfz {

template<u16 N>
struct StringLocal final {

	// Public members
	// --------------------------------------------------------------------------------------------

	static_assert(N > 0, "");

	char mRawStr[N];

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	StringLocal() noexcept { this->clear(); }
	StringLocal(const StringLocal&) noexcept = default;
	StringLocal& operator= (const StringLocal&) noexcept = default;
	~StringLocal() noexcept = default;

	// Constructs a StringLocal with printf syntax. If the string is larger than the capacity then
	// only what fits will be stored. The resulting string is guaranteed to be null-terminated.
	StringLocal(const char* format, ...) noexcept
	{
		this->clear();
		va_list args;
		va_start(args, format);
		this->vappendf(format, args);
		va_end(args);
	}

	StringLocal& operator= (const char* str) noexcept
	{
		// Note: Do nothing if: str128 tmp = "tmp"; tmp = tmp.str();
		if (str != mRawStr) {
			this->clear();
			this->appendf(str);
		}
		return *this;
	}

	// Public methods
	// --------------------------------------------------------------------------------------------

	u32 size() const { return u32(strnlen(this->mRawStr, N)); }
	u32 capacity() const { return N; }
	const char* str() const { return mRawStr; }

	void clear() { mRawStr[0] = '\0'; }

	void appendf(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		this->vappendf(format, args);
		va_end(args);
	}

	void vappendf(const char* format, va_list args)
	{
		u32 len = this->size();
		u32 capacityLeft = this->capacity() - len;
		int numWritten = vsnprintf(this->mRawStr + len, capacityLeft, format, args);
		sfz_assert(numWritten >= 0);
	}

	void appendChars(const char* chars, u32 numChars)
	{
		u32 len = this->size();
		u32 capacityLeft = this->capacity() - len;
		sfz_assert(numChars < capacityLeft);
		strncpy(this->mRawStr + len, chars, size_t(numChars));
		this->mRawStr[len + numChars] = '\0';
	}

	void removeChars(u32 numChars)
	{
		const u32 len = this->size();
		const u32 numToRemove = sfz::min(numChars, len);
		this->mRawStr[len - numToRemove] = '\0';
	}

	void toLower()
	{
		char* c = mRawStr;
		while (*c != '\0') {
			*c = char(tolower(*c));
			c++;
		}
	}

	void trim()
	{
		const u32 len = this->size();
		if (len == 0) return;

		u32 firstNonWhitespace = 0;
		bool nonWhitespaceFound = false;
		for (u32 i = 0; i < len; i++) {
			char c = mRawStr[i];
			if (!(c == ' ' || c == '\t' || c == '\n')) {
				nonWhitespaceFound = true;
				firstNonWhitespace = i;
				break;
			}
		}

		if (!nonWhitespaceFound) {
			this->clear();
			return;
		}

		u32 lastNonWhitespace = len - 1;
		for (u32 i = len; i > 0; i--) {
			char c = mRawStr[i - 1];
			if (!(c == ' ' || c == '\t' || c == '\n')) {
				lastNonWhitespace = i - 1;
				break;
			}
		}

		const u32 newLen = lastNonWhitespace - firstNonWhitespace + 1;
		if (newLen == len) return;
		memmove(mRawStr, mRawStr + firstNonWhitespace, newLen);
		mRawStr[newLen] = '\0';
	}

	bool endsWith(const char* ending) const
	{
		const u32 len = this->size();
		const u32 endingLen = u32(strnlen(ending, N));
		if (endingLen > len) return false;

		u32 endingIdx = 0;
		for (u32 i = len - endingLen; i < len; i++) {
			if (ending[endingIdx] != mRawStr[i]) return false;
			endingIdx += 1;
		}

		return true;
	}

	bool contains(const char* substring) const
	{
		if (substring == nullptr) return false;
		return strstr(mRawStr, substring) != nullptr;
	}

	bool isPartOf(const char* superstring) const
	{
		if (superstring == nullptr) return false;
		return strstr(superstring, mRawStr) != nullptr;
	}

	// Operators
	// --------------------------------------------------------------------------------------------

	operator const char*() const { return this->mRawStr; }

	bool operator== (const StringLocal& other) const { return *this == other.mRawStr; }
	bool operator!= (const StringLocal& other) const { return *this != other.mRawStr; }
	bool operator< (const StringLocal& other) const { return *this < other.mRawStr; }
	bool operator<= (const StringLocal& other) const { return *this <= other.mRawStr; }
	bool operator> (const StringLocal& other) const { return *this > other.mRawStr; }
	bool operator>= (const StringLocal& other) const { return *this >= other.mRawStr; }

	bool operator== (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->mRawStr, o, N) == 0; }
	bool operator!= (const char* o) const { sfz_assert(o != nullptr); return !(*this == o); }
	bool operator< (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->mRawStr, o, N) < 0; }
	bool operator<= (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->mRawStr, o, N) <= 0; }
	bool operator> (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->mRawStr, o, N) > 0; }
	bool operator>= (const char* o) const { sfz_assert(o != nullptr); return strncmp(this->mRawStr, o, N) >= 0; }
};

using str16 = StringLocal<16>;
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

template<u16 N>
constexpr u64 hash(const StringLocal<N>& str) { return sfz::hash(str.str()); }

// const char* is an alternate type to StringLocal
template<u16 N>
struct AltType<StringLocal<N>> final {
	using AltT = const char*;
};

} // namespace sfz

#endif
