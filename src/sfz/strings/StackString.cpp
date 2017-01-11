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

#include "sfz/strings/StackString.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "sfz/Assert.hpp"

namespace sfz {

// StackStringTempl: Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<size_t N>
StackStringTempl<N>::StackStringTempl() noexcept
{
	static_assert(N > 0, "StackString capacity needs to be greater than 0");
	str[0] = '\0';
}

template<size_t N>
StackStringTempl<N>::StackStringTempl(const char* string) noexcept
{
	std::strncpy(this->str, string, N);
	this->str[N-1] = '\0';
}

// StackStringTempl: Public methods
// ------------------------------------------------------------------------------------------------

template<size_t N>
void StackStringTempl<N>::printf(const char* format, ...) noexcept
{
	va_list args;
	va_start(args, format);
	std::vsnprintf(this->str, N, format, args);
	va_end(args);
}

template<size_t N>
void StackStringTempl<N>::printfAppend(const char* format, ...) noexcept
{
	va_list args;
	va_start(args, format);
	size_t len = std::strlen(this->str);
	std::vsnprintf(this->str + len, N - len, format, args);
	va_end(args);
}

template<size_t N>
void StackStringTempl<N>::insertChars(const char* first, size_t numChars) noexcept
{
	sfz_assert_debug(numChars < N);
	std::strncpy(this->str, first, numChars);
	this->str[numChars] = '\0';
}

// StackStringTempl: Operators
// ------------------------------------------------------------------------------------------------

template<size_t N>
bool StackStringTempl<N>::operator== (const StackStringTempl& other) const noexcept
{
	return *this == other.str;
}

template<size_t N>
bool StackStringTempl<N>::operator!= (const StackStringTempl& other) const noexcept
{
	return *this != other.str;
}

template<size_t N>
bool StackStringTempl<N>::operator< (const StackStringTempl& other) const noexcept
{
	return *this < other.str;
}

template<size_t N>
bool StackStringTempl<N>::operator<= (const StackStringTempl& other) const noexcept
{
	return *this <= other.str;
}

template<size_t N>
bool StackStringTempl<N>::operator> (const StackStringTempl& other) const noexcept
{
	return *this > other.str;
}

template<size_t N>
bool StackStringTempl<N>::operator>= (const StackStringTempl& other) const noexcept
{
	return *this >= other.str;
}

template<size_t N>
bool StackStringTempl<N>::operator== (const char* other) const noexcept
{
	sfz_assert_debug(other != nullptr);
	return std::strncmp(this->str, other, N) == 0;
}

template<size_t N>
bool StackStringTempl<N>::operator!= (const char* other) const noexcept
{
	sfz_assert_debug(other != nullptr);
	return !(*this == other);
}

template<size_t N>
bool StackStringTempl<N>::operator< (const char* other) const noexcept
{
	sfz_assert_debug(other != nullptr);
	return std::strncmp(this->str, other, N) < 0;
}

template<size_t N>
bool StackStringTempl<N>::operator<= (const char* other) const noexcept
{
	sfz_assert_debug(other != nullptr);
	return std::strncmp(this->str, other, N) <= 0;
}

template<size_t N>
bool StackStringTempl<N>::operator> (const char* other) const noexcept
{
	sfz_assert_debug(other != nullptr);
	return std::strncmp(this->str, other, N) > 0;
}

template<size_t N>
bool StackStringTempl<N>::operator>= (const char* other) const noexcept
{
	sfz_assert_debug(other != nullptr);
	return std::strncmp(this->str, other, N) >= 0;
}

// StackStringTempl: Explicit instantiation
// ------------------------------------------------------------------------------------------------

template struct StackStringTempl<32>;
template struct StackStringTempl<64>;
template struct StackStringTempl<96>;
template struct StackStringTempl<128>;
template struct StackStringTempl<192>;
template struct StackStringTempl<256>;
template struct StackStringTempl<320>;
template struct StackStringTempl<512>;
template struct StackStringTempl<1024>;

} // namespace sfz
