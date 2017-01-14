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

#include "sfz/strings/DynString.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "sfz/Assert.hpp"

namespace sfz {

// DynString: Constructors & destructors
// ------------------------------------------------------------------------------------------------

DynString::DynString(const char* string, uint32_t capacity, Allocator* allocator) noexcept
{
	// Special case when string is nullptr
	if (string == nullptr) {
		mString.create(capacity, allocator);
		return;
	}

	// Check if string length is larger than requested capacity 
	size_t length = std::strlen(string) + 1; // +1 for null-terminator
	if (capacity < length) capacity = uint32_t(length);

	// Set allocator and allocate memory
	mString.create(capacity, allocator);
	mString.setSize(uint32_t(length));

	// Copy string to internal DynArray
	std::strcpy(mString.data(), string);
}

// DynString: Getters
// ------------------------------------------------------------------------------------------------

uint32_t DynString::size() const noexcept
{
	uint32_t tmp = mString.size();
	if (tmp == 0) return 0;
	return tmp - 1; // Remove null-terminator
}

// DynString: Public methods
// ------------------------------------------------------------------------------------------------

int32_t DynString::printf(const char* format, ...) noexcept
{
	va_list args;
	va_start(args, format);
	int32_t res = std::vsnprintf(mString.data(), mString.capacity(), format, args);
	va_end(args);
	sfz_assert_debug(res >= 0);
	mString.setSize(static_cast<uint32_t>(res) + 1); // +1 for null-terminator
	return res;
}

int32_t DynString::printfAppend(const char* format, ...) noexcept
{
	va_list args;
	va_start(args, format);
	uint32_t len = this->size();
	int32_t res = std::vsnprintf(mString.data() + len, mString.capacity() - len, format, args);
	va_end(args);
	sfz_assert_debug(res >= 0);
	mString.setSize(len + static_cast<uint32_t>(res) + 1); // +1 for null-terminator
	return res;
}

// DynString: Operators
// ------------------------------------------------------------------------------------------------

bool DynString::operator== (const DynString& other) const noexcept
{
	return *this == other.mString.data();
}

bool DynString::operator!= (const DynString& other) const noexcept
{
	return *this != other.mString.data();
}

bool DynString::operator< (const DynString& other) const noexcept
{
	return *this < other.mString.data();
}

bool DynString::operator<= (const DynString& other) const noexcept
{
	return *this <= other.mString.data();
}

bool DynString::operator> (const DynString& other) const noexcept
{
	return *this > other.mString.data();
}

bool DynString::operator>= (const DynString& other) const noexcept
{
	return *this >= other.mString.data();
}

bool DynString::operator== (const char* other) const noexcept
{
	sfz_assert_debug(mString.data() != nullptr);
	sfz_assert_debug(other != nullptr);
	return std::strncmp(mString.data(), other, mString.size()) == 0;
}

bool DynString::operator!= (const char* other) const noexcept
{
	return !(*this == other);
}

bool DynString::operator< (const char* other) const noexcept
{
	sfz_assert_debug(mString.data() != nullptr);
	sfz_assert_debug(other != nullptr);
	return std::strncmp(mString.data(), other, mString.size()) < 0;
}

bool DynString::operator<= (const char* other) const noexcept
{
	return !(*this > other);
}

bool DynString::operator> (const char* other) const noexcept
{
	sfz_assert_debug(mString.data() != nullptr);
	sfz_assert_debug(other != nullptr);
	return std::strncmp(mString.data(), other, mString.size()) > 0;
}

bool DynString::operator>= (const char* other) const noexcept
{
	return !(*this < other);
}

} // namespace sfz
