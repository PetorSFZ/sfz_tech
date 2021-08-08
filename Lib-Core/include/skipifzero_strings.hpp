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

namespace sfz {

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

// strID
// ------------------------------------------------------------------------------------------------

// A string id represents the hash of a string. Used to cheaply compare strings (e.g. in a hash map).
struct strID final {
	uint64_t id = 0; // 0 is reserved for invalid hashes

	constexpr strID() = default;
	constexpr explicit strID(uint64_t hashId) : id(hashId) {}
	strID(const char* str);

	bool isValid() const { return id != 0; }
	const char* str() const;
	bool operator== (strID o) const { return this->id == o.id; }
	bool operator!= (strID o) const { return this->id != o.id; }
	operator uint64_t() const { return id; }
};
static_assert(sizeof(strID) == sizeof(uint64_t), "strID is padded");

constexpr strID STR_ID_INVALID = strID();

constexpr uint64_t hash(strID str) { return str.id; }

#ifdef SFZ_STR_ID_IMPLEMENTATION

struct StringStorage final {
	SfzAllocator* allocator = nullptr;
	HashMap<strID, char*> strs;

	StringStorage(uint32_t initialCapacity, SfzAllocator* allocator)
	{
		this->allocator = allocator;
		this->strs.init(initialCapacity, allocator, sfz_dbg(""));
	}

	StringStorage(const StringStorage&) = delete;
	StringStorage& operator= (const StringStorage&) = delete;

	~StringStorage()
	{
		for (auto pair : strs) {
			char* str = pair.value;
			allocator->dealloc(str);
		}
		strs.destroy();
		allocator = nullptr;
	}
};

static StringStorage* strStorage = nullptr;

strID::strID(const char* str)
{
	sfz_assert(strStorage != nullptr);
	this->id = strID(sfz::hash(str));
	sfz_assert_hard(this->isValid());

	// Add string to storage and check for collisions
	char** storedStr = strStorage->strs.get(strID(id));
	if (storedStr == nullptr) {
		uint32_t strLen = uint32_t(strlen(str));
		char* newStr = reinterpret_cast<char*>(strStorage->allocator->alloc(sfz_dbg(""), strLen + 1));
		memcpy(newStr, str, strLen);
		newStr[strLen] = '\0';
		storedStr = &strStorage->strs.put(strID(id), newStr);
	}
	sfz_assert_hard(strcmp(str, *storedStr) == 0);
}

const char* strID::str() const
{
	sfz_assert(strStorage != nullptr);
	const char* const* strPtr = strStorage->strs.get(strID(id));
	if (strPtr == nullptr) return "";
	return *strPtr;
}

#endif

// StringLocal
// ------------------------------------------------------------------------------------------------

template<uint16_t N>
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

	uint32_t size() const { return uint32_t(strnlen(this->mRawStr, N)); }
	uint32_t capacity() const { return N; }
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
		uint32_t len = this->size();
		uint32_t capacityLeft = this->capacity() - len;
		int numWritten = vsnprintf(this->mRawStr + len, capacityLeft, format, args);
		sfz_assert(numWritten >= 0);
	}

	void appendChars(const char* chars, uint32_t numChars)
	{
		uint32_t len = this->size();
		uint32_t capacityLeft = this->capacity() - len;
		sfz_assert(numChars < capacityLeft);
		strncpy(this->mRawStr + len, chars, size_t(numChars));
		this->mRawStr[len + numChars] = '\0';
	}

	void removeChars(uint32_t numChars)
	{
		const uint32_t len = this->size();
		const uint32_t numToRemove = sfz::min(numChars, len);
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
		const uint32_t len = this->size();
		if (len == 0) return;

		uint32_t firstNonWhitespace = 0;
		bool nonWhitespaceFound = false;
		for (uint32_t i = 0; i < len; i++) {
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

		uint32_t lastNonWhitespace = len - 1;
		for (uint32_t i = len; i > 0; i--) {
			char c = mRawStr[i - 1];
			if (!(c == ' ' || c == '\t' || c == '\n')) {
				lastNonWhitespace = i - 1;
				break;
			}
		}

		const uint32_t newLen = lastNonWhitespace - firstNonWhitespace + 1;
		if (newLen == len) return;
		memmove(mRawStr, mRawStr + firstNonWhitespace, newLen);
		mRawStr[newLen] = '\0';
	}

	bool endsWith(const char* ending) const
	{
		const uint32_t len = this->size();
		const uint32_t endingLen = uint32_t(strnlen(ending, N));
		if (endingLen > len) return false;

		uint32_t endingIdx = 0;
		for (uint32_t i = len - endingLen; i < len; i++) {
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

template<uint16_t N>
constexpr uint64_t hash(const StringLocal<N>& str) { return sfz::hash(str.str()); }

// const char* is an alternate type to StringLocal
template<uint16_t N>
struct AltType<StringLocal<N>> final {
	using AltT = const char*;
};

} // namespace sfz

#endif
