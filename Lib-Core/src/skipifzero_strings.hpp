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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "sfz.h"
#include "sfz_cpp.hpp"

#ifdef SFZ_STR_ID_IMPLEMENTATION
#include "skipifzero_hash_maps.hpp"
#endif

// String hashing
// ------------------------------------------------------------------------------------------------

// FNV-1a hash function, based on public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
// See http://isthe.com/chongo/tech/comp/fnv/
sfz_constexpr_func u64 sfzHashStringFNV1a(const char* str)
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
sfz_constexpr_func u64 sfzHashBytesFNV1a(const u8* bytes, u64 numBytes)
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

sfz_constexpr_func u64 sfzHash(const char* str) { return sfzHashStringFNV1a(str); }

// SfzStrID
// ------------------------------------------------------------------------------------------------

struct SfzStrIDs;

sfz_constexpr_func SfzStrID sfzStrIDCreate(const char* str) { return { sfzHashStringFNV1a(str) }; }
sfz_extern_c SfzStrID sfzStrIDCreateRegister(SfzStrIDs* ids, const char* str);
sfz_extern_c const char* sfzStrIDGetStr(const SfzStrIDs* ids, SfzStrID id);

sfz_constexpr_func u64 sfzHash(SfzStrID str) { return str.id; }

#ifdef SFZ_STR_ID_IMPLEMENTATION

struct SfzStrIDs final {
	SfzAllocator* allocator = nullptr;
	SfzHashMap<SfzStrID, char*> strs;

	SfzStrIDs(u32 initialCapacity, SfzAllocator* allocator)
	{
		this->allocator = allocator;
		this->strs.init(initialCapacity, allocator, sfz_dbg(""));
	}

	SfzStrIDs(const SfzStrIDs&) = delete;
	SfzStrIDs& operator= (const SfzStrIDs&) = delete;

	~SfzStrIDs()
	{
		for (auto pair : strs) {
			char* str = pair.value;
			allocator->dealloc(str);
		}
		strs.destroy();
		allocator = nullptr;
	}
};

sfz_extern_c SfzStrID sfzStrIDCreateRegister(SfzStrIDs* ids, const char* str)
{
	sfz_assert(ids != nullptr);
	SfzStrID id = SFZ_NULL_STR_ID;
	id.id = sfzHash(str);
	sfz_assert_hard(id != SFZ_NULL_STR_ID);
	
	// Add string to storage and check for collisions
	char** storedStr = ids->strs.get(id);
	if (storedStr == nullptr) {
		u32 strLen = u32(strlen(str));
		char* newStr = reinterpret_cast<char*>(ids->allocator->alloc(sfz_dbg(""), strLen + 1));
		memcpy(newStr, str, strLen);
		newStr[strLen] = '\0';
		storedStr = &ids->strs.put(id, newStr);
	}
	sfz_assert_hard(strcmp(str, *storedStr) == 0);

	return id;
}

sfz_extern_c const char* sfzStrIDGetStr(const SfzStrIDs* ids, SfzStrID id)
{
	sfz_assert(ids != nullptr);
	const char* const* strPtr = ids->strs.get(id);
	if (strPtr == nullptr) return "<unknown>";
	return *strPtr;
}

#endif

// String functions
// --------------------------------------------------------------------------------------------

inline u32 sfzStrSize(SfzStrViewConst v) { return u32(strnlen(v.str, v.capacity)); }
inline u32 sfzStr32Size(const SfzStr32* s) { return sfzStrSize(sfzStr32ToViewConst(s)); }
inline u32 sfzStr96Size(const SfzStr96* s) { return sfzStrSize(sfzStr96ToViewConst(s)); }
inline u32 sfzStr320Size(const SfzStr320* s) { return sfzStrSize(sfzStr320ToViewConst(s)); }
inline u32 sfzStr2560Size(const SfzStr2560* s) { return sfzStrSize(sfzStr2560ToViewConst(s)); }

sfz_constexpr_func void sfzStrClear(SfzStrView v) { v.str[0] = '\0'; }
sfz_constexpr_func void sfzStr32Clear(SfzStr32* s) { sfzStrClear(sfzStr32ToView(s)); }
sfz_constexpr_func void sfzStr96Clear(SfzStr96* s) { sfzStrClear(sfzStr96ToView(s)); }
sfz_constexpr_func void sfzStr320Clear(SfzStr320* s) { sfzStrClear(sfzStr320ToView(s)); }
sfz_constexpr_func void sfzStr2560Clear(SfzStr2560* s) { sfzStrClear(sfzStr2560ToView(s)); }

inline void sfzStrVAppendf(SfzStrView v, const char* fmt, va_list args)
{
	u32 len = sfzStrSize(sfzStrViewToConst(v));
	u32 capacityLeft = v.capacity - len;
	int numWritten = vsnprintf(v.str + len, capacityLeft, fmt, args);
	sfz_assert(numWritten >= 0);
}
inline void sfzStr32VAppendf(SfzStr32* s, const char* fmt, va_list args) { sfzStrVAppendf(sfzStr32ToView(s), fmt, args); }
inline void sfzStr96VAppendf(SfzStr96* s, const char* fmt, va_list args) { sfzStrVAppendf(sfzStr96ToView(s), fmt, args); }
inline void sfzStr320VAppendf(SfzStr320* s, const char* fmt, va_list args) { sfzStrVAppendf(sfzStr320ToView(s), fmt, args); }
inline void sfzStr2560VAppendf(SfzStr2560* s, const char* fmt, va_list args) { sfzStrVAppendf(sfzStr2560ToView(s), fmt, args); }

inline void sfzStrAppendf(SfzStrView v, const char* fmt, ...) { va_list va; va_start(va, fmt); sfzStrVAppendf(v, fmt, va); va_end(va); }
inline void sfzStr32Appendf(SfzStr32* s, const char* fmt, ...) { va_list va; va_start(va, fmt); sfzStrVAppendf(sfzStr32ToView(s), fmt, va); va_end(va); }
inline void sfzStr96Appendf(SfzStr96* s, const char* fmt, ...) { va_list va; va_start(va, fmt); sfzStrVAppendf(sfzStr96ToView(s), fmt, va); va_end(va); }
inline void sfzStr320Appendf(SfzStr320* s, const char* fmt, ...) { va_list va; va_start(va, fmt); sfzStrVAppendf(sfzStr320ToView(s), fmt, va); va_end(va); }
inline void sfzStr2560Appendf(SfzStr2560* s, const char* fmt, ...) { va_list va; va_start(va, fmt); sfzStrVAppendf(sfzStr2560ToView(s), fmt, va); va_end(va); }

sfz_constexpr_func void sfzStrInit(SfzStrView v, const char* str)
{
	sfzStrClear(v);
	u32 i = 0;
	for (const u32 len = v.capacity - 1; i < len; i++) {
		const char c = str[i];
		if (c == '\0') break;
		v.str[i] = c;
	}
	v.str[i] = '\0';
}
sfz_constexpr_func SfzStr32 sfzStr32Init(const char* str) { SfzStr32 s = {}; sfzStrInit(sfzStr32ToView(&s), str); return s; }
sfz_constexpr_func SfzStr96 sfzStr96Init(const char* str) { SfzStr96 s = {}; sfzStrInit(sfzStr96ToView(&s), str); return s; }
sfz_constexpr_func SfzStr320 sfzStr320Init(const char* str) { SfzStr320 s = {}; sfzStrInit(sfzStr320ToView(&s), str); return s; }
sfz_constexpr_func SfzStr2560 sfzStr2560Init(const char* str) { SfzStr2560 s = {}; sfzStrInit(sfzStr2560ToView(&s), str); return s; }

inline void sfzStrInitFmt(SfzStrView v, const char* fmt, ...)
{
	sfzStrClear(v);
	va_list args;
	va_start(args, fmt);
	sfzStrVAppendf(v, fmt, args);
	va_end(args);
}
inline SfzStr32 sfzStr32InitFmt(const char* fmt, ...) { SfzStr32 s = {}; va_list va; va_start(va, fmt); sfzStr32VAppendf(&s, fmt, va); va_end(va); return s; }
inline SfzStr96 sfzStr96InitFmt(const char* fmt, ...) { SfzStr96 s = {}; va_list va; va_start(va, fmt); sfzStr96VAppendf(&s, fmt, va); va_end(va); return s; }
inline SfzStr320 sfzStr320InitFmt(const char* fmt, ...) { SfzStr320 s = {}; va_list va; va_start(va, fmt); sfzStr320VAppendf(&s, fmt, va); va_end(va); return s; }
inline SfzStr2560 sfzStr2560InitFmt(const char* fmt, ...) { SfzStr2560 s = {}; va_list va; va_start(va, fmt); sfzStr2560VAppendf(&s, fmt, va); va_end(va); return s; }

inline void sfzStrAppendChars(SfzStrView v, const char* chars, u32 numChars)
{
	u32 len = sfzStrSize(sfzStrViewToConst(v));
	u32 capacityLeft = v.capacity - len;
	sfz_assert(numChars < capacityLeft);
	strncpy(v.str + len, chars, size_t(numChars));
	v.str[len + numChars] = '\0';
}
inline void sfzStr32AppendChars(SfzStr32* s, const char* chars, u32 numChars) { return sfzStrAppendChars(sfzStr32ToView(s), chars, numChars); }
inline void sfzStr96AppendChars(SfzStr96* s, const char* chars, u32 numChars) { return sfzStrAppendChars(sfzStr96ToView(s), chars, numChars); }
inline void sfzStr320AppendChars(SfzStr320* s, const char* chars, u32 numChars) { return sfzStrAppendChars(sfzStr320ToView(s), chars, numChars); }
inline void sfzStr2560AppendChars(SfzStr2560* s, const char* chars, u32 numChars) { return sfzStrAppendChars(sfzStr2560ToView(s), chars, numChars); }

inline void sfzStrRemoveChars(SfzStrView v, u32 numChars)
{
	const u32 len = sfzStrSize(sfzStrViewToConst(v));
	const u32 numToRemove = u32_min(numChars, len);
	v.str[len - numToRemove] = '\0';
}
inline void sfzStr32RemoveChars(SfzStr32* s, u32 numChars) { return sfzStrRemoveChars(sfzStr32ToView(s), numChars); }
inline void sfzStr96RemoveChars(SfzStr96* s, u32 numChars) { return sfzStrRemoveChars(sfzStr96ToView(s), numChars); }
inline void sfzStr320RemoveChars(SfzStr320* s, u32 numChars) { return sfzStrRemoveChars(sfzStr320ToView(s), numChars); }
inline void sfzStr2560RemoveChars(SfzStr2560* s, u32 numChars) { return sfzStrRemoveChars(sfzStr2560ToView(s), numChars); }

inline void sfzStrToLower(SfzStrView v)
{
	char* c = v.str;
	while (*c != '\0') {
		*c = char(tolower(*c));
		c++;
	}
}
inline void sfzStr32ToLower(SfzStr32* s) { return sfzStrToLower(sfzStr32ToView(s)); }
inline void sfzStr96ToLower(SfzStr96* s) { return sfzStrToLower(sfzStr96ToView(s)); }
inline void sfzStr320ToLower(SfzStr320* s) { return sfzStrToLower(sfzStr320ToView(s)); }
inline void sfzStr2560ToLower(SfzStr2560* s) { return sfzStrToLower(sfzStr2560ToView(s)); }

inline void sfzStrTrim(SfzStrView v)
{
	const u32 len = sfzStrSize(sfzStrViewToConst(v));
	if (len == 0) return;

	u32 firstNonWhitespace = 0;
	bool nonWhitespaceFound = false;
	for (u32 i = 0; i < len; i++) {
		char c = v.str[i];
		if (!(c == ' ' || c == '\t' || c == '\n')) {
			nonWhitespaceFound = true;
			firstNonWhitespace = i;
			break;
		}
	}

	if (!nonWhitespaceFound) {
		sfzStrClear(v);
		return;
	}

	u32 lastNonWhitespace = len - 1;
	for (u32 i = len; i > 0; i--) {
		char c = v.str[i - 1];
		if (!(c == ' ' || c == '\t' || c == '\n')) {
			lastNonWhitespace = i - 1;
			break;
		}
	}

	const u32 newLen = lastNonWhitespace - firstNonWhitespace + 1;
	if (newLen == len) return;
	memmove(v.str, v.str + firstNonWhitespace, newLen);
	v.str[newLen] = '\0';
}
inline void sfzStr32Trim(SfzStr32* s) { return sfzStrTrim(sfzStr32ToView(s)); }
inline void sfzStr96Trim(SfzStr96* s) { return sfzStrTrim(sfzStr96ToView(s)); }
inline void sfzStr320Trim(SfzStr320* s) { return sfzStrTrim(sfzStr320ToView(s)); }
inline void sfzStr2560Trim(SfzStr2560* s) { return sfzStrTrim(sfzStr2560ToView(s)); }

inline bool sfzStrEndsWith(SfzStrViewConst v, const char* ending)
{
	const u32 len = sfzStrSize(v);
	const u32 endingLen = u32(strnlen(ending, v.capacity));
	if (endingLen > len) return false;

	u32 endingIdx = 0;
	for (u32 i = len - endingLen; i < len; i++) {
		if (ending[endingIdx] != v.str[i]) return false;
		endingIdx += 1;
	}

	return true;
}
inline u32 sfzStr32EndsWith(const SfzStr32* s, const char* ending) { return sfzStrEndsWith(sfzStr32ToViewConst(s), ending); }
inline u32 sfzStr96EndsWith(const SfzStr96* s, const char* ending) { return sfzStrEndsWith(sfzStr96ToViewConst(s), ending); }
inline u32 sfzStr320EndsWith(const SfzStr320* s, const char* ending) { return sfzStrEndsWith(sfzStr320ToViewConst(s), ending); }
inline u32 sfzStr2560EndsWith(const SfzStr2560* s, const char* ending) { return sfzStrEndsWith(sfzStr2560ToViewConst(s), ending); }

inline bool sfzStrContains(SfzStrViewConst v, const char* substr)
{
	if (substr == nullptr) return false;
	return strstr(v.str, substr) != nullptr;
}
inline u32 sfzStr32Contains(const SfzStr32* s, const char* substr) { return sfzStrContains(sfzStr32ToViewConst(s), substr); }
inline u32 sfzStr96Contains(const SfzStr96* s, const char* substr) { return sfzStrContains(sfzStr96ToViewConst(s), substr); }
inline u32 sfzStr320Contains(const SfzStr320* s, const char* substr) { return sfzStrContains(sfzStr320ToViewConst(s), substr); }
inline u32 sfzStr2560Contains(const SfzStr2560* s, const char* substr) { return sfzStrContains(sfzStr2560ToViewConst(s), substr); }

inline bool sfzStrIsPartOf(SfzStrViewConst v, const char* superstr)
{
	if (superstr == nullptr) return false;
	return strstr(superstr, v.str) != nullptr;
}
inline u32 sfzStr32IsPartOf(const SfzStr32* s, const char* superstr) { return sfzStrIsPartOf(sfzStr32ToViewConst(s), superstr); }
inline u32 sfzStr96IsPartOf(const SfzStr96* s, const char* superstr) { return sfzStrIsPartOf(sfzStr96ToViewConst(s), superstr); }
inline u32 sfzStr320IsPartOf(const SfzStr320* s, const char* superstr) { return sfzStrIsPartOf(sfzStr320ToViewConst(s), superstr); }
inline u32 sfzStr2560IsPartOf(const SfzStr2560* s, const char* superstr) { return sfzStrIsPartOf(sfzStr2560ToViewConst(s), superstr); }

// String operators
// --------------------------------------------------------------------------------------------

#ifdef __cplusplus

inline bool operator== (SfzStrViewConst v, const char* o) { sfz_assert(o != nullptr); return strncmp(v.str, o, v.capacity) == 0; }
inline bool operator!= (SfzStrViewConst v, const char* o) { sfz_assert(o != nullptr); return !(v == o); }

inline bool operator== (const SfzStr32& s, const char* o) { return sfzStr32ToViewConst(&s) == o; }
inline bool operator!= (const SfzStr32& s, const char* o) { return sfzStr32ToViewConst(&s) != o; }

inline bool operator== (const SfzStr96& s, const char* o) { return sfzStr96ToViewConst(&s) == o; }
inline bool operator!= (const SfzStr96& s, const char* o) { return sfzStr96ToViewConst(&s) != o; }

inline bool operator== (const SfzStr320& s, const char* o) { return sfzStr320ToViewConst(&s) == o; }
inline bool operator!= (const SfzStr320& s, const char* o) { return sfzStr320ToViewConst(&s) != o; }

inline bool operator== (const SfzStr2560& s, const char* o) { return sfzStr2560ToViewConst(&s) == o; }
inline bool operator!= (const SfzStr2560& s, const char* o) { return sfzStr2560ToViewConst(&s) != o; }

inline bool operator== (SfzStrViewConst lhs, SfzStrViewConst rhs) { return lhs == rhs.str; }
inline bool operator!= (SfzStrViewConst lhs, SfzStrViewConst rhs) { return lhs != rhs.str; }

inline bool operator== (const SfzStr32& lhs, const SfzStr32& rhs) { return sfzStr32ToViewConst(&lhs) == sfzStr32ToViewConst(&rhs); }
inline bool operator!= (const SfzStr32& lhs, const SfzStr32& rhs) { return sfzStr32ToViewConst(&lhs) != sfzStr32ToViewConst(&rhs); }

inline bool operator== (const SfzStr96& lhs, const SfzStr96& rhs) { return sfzStr96ToViewConst(&lhs) == sfzStr96ToViewConst(&rhs); }
inline bool operator!= (const SfzStr96& lhs, const SfzStr96& rhs) { return sfzStr96ToViewConst(&lhs) != sfzStr96ToViewConst(&rhs); }

inline bool operator== (const SfzStr320& lhs, const SfzStr320& rhs) { return sfzStr320ToViewConst(&lhs) == sfzStr320ToViewConst(&rhs); }
inline bool operator!= (const SfzStr320& lhs, const SfzStr320& rhs) { return sfzStr320ToViewConst(&lhs) != sfzStr320ToViewConst(&rhs); }

inline bool operator== (const SfzStr2560& lhs, const SfzStr2560& rhs) { return sfzStr2560ToViewConst(&lhs) == sfzStr2560ToViewConst(&rhs); }
inline bool operator!= (const SfzStr2560& lhs, const SfzStr2560& rhs) { return sfzStr2560ToViewConst(&lhs) != sfzStr2560ToViewConst(&rhs); }

#endif // __cplusplus

// sfzHash and SfzAltType implementations for SfzHashMap
// --------------------------------------------------------------------------------------------

#ifdef __cplusplus

constexpr u64 sfzHash(const SfzStr32& str) { return sfzHash(str.str); }
constexpr u64 sfzHash(const SfzStr96& str) { return sfzHash(str.str); }
constexpr u64 sfzHash(const SfzStr320& str) { return sfzHash(str.str); }
constexpr u64 sfzHash(const SfzStr2560& str) { return sfzHash(str.str); }

template<> struct SfzAltType<SfzStr32> final {
	using AltT = const char*;
	static inline SfzStr32 conv(const char* str) { return sfzStr32Init(str); }
};
template<> struct SfzAltType<SfzStr96> final {
	using AltT = const char*;
	static inline SfzStr96 conv(const char* str) { return sfzStr96Init(str); }
};
template<> struct SfzAltType<SfzStr320> final {
	using AltT = const char*;
	static inline SfzStr320 conv(const char* str) { return sfzStr320Init(str); }
};
template<> struct SfzAltType<SfzStr2560> final {
	using AltT = const char*;
	static inline SfzStr2560 conv(const char* str) { return sfzStr2560Init(str); }
};

#endif // __cplusplus
#endif // SKIPIFZERO_STRINGS_HPP
