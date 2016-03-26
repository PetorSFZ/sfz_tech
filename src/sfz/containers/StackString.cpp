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

#include "sfz/containers/StackString.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace sfz {

// StackStringTempl: Explicit instantiation
// ------------------------------------------------------------------------------------------------

template struct StackStringTempl<96>;

template struct StackStringTempl<32>;
template struct StackStringTempl<64>;
template struct StackStringTempl<128>;
template struct StackStringTempl<256>;
template struct StackStringTempl<512>;
template struct StackStringTempl<1024>;

// StackStringTempl: Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<size_t N>
StackStringTempl<N>::StackStringTempl(const char* string) noexcept
{
	std::strncpy(this->string, string, N);
	this->string[N-1] = '\0';
}

// StackStringTempl: Public methods
// ------------------------------------------------------------------------------------------------

template<size_t N>
void StackStringTempl<N>::printf(const char* format, ...) noexcept
{
	va_list args;
	va_start(args, format);
	std::vsnprintf(this->string, N, format, args);
	va_end(args);
}

// StackStringTempl: Operators
// ------------------------------------------------------------------------------------------------

template<size_t N>
bool StackStringTempl<N>::operator== (const StackStringTempl& other) const noexcept
{
	return *this == other.string;
}

template<size_t N>
bool StackStringTempl<N>::operator!= (const StackStringTempl& other) const noexcept
{
	return *this != other.string;
}

template<size_t N>
bool StackStringTempl<N>::operator< (const StackStringTempl& other) const noexcept
{
	return *this < other.string;
}

template<size_t N>
bool StackStringTempl<N>::operator<= (const StackStringTempl& other) const noexcept
{
	return *this <= other.string;
}

template<size_t N>
bool StackStringTempl<N>::operator> (const StackStringTempl& other) const noexcept
{
	return *this > other.string;
}

template<size_t N>
bool StackStringTempl<N>::operator>= (const StackStringTempl& other) const noexcept
{
	return *this >= other.string;
}

template<size_t N>
bool StackStringTempl<N>::operator== (const char* other) const noexcept
{
	return std::strncmp(this->string, other, N) == 0;
}

template<size_t N>
bool StackStringTempl<N>::operator!= (const char* other) const noexcept
{
	return !(*this == other);
}

template<size_t N>
bool StackStringTempl<N>::operator< (const char* other) const noexcept
{
	return std::strncmp(this->string, other, N) < 0;
}

template<size_t N>
bool StackStringTempl<N>::operator<= (const char* other) const noexcept
{
	return std::strncmp(this->string, other, N) <= 0;
}

template<size_t N>
bool StackStringTempl<N>::operator> (const char* other) const noexcept
{
	return std::strncmp(this->string, other, N) > 0;
}

template<size_t N>
bool StackStringTempl<N>::operator>= (const char* other) const noexcept
{
	return std::strncmp(this->string, other, N) >= 0;
}

} // namespace sfz
