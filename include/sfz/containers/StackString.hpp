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
/// that only the predefined sizes is available. The default size is 128 chars, equivalent to the
/// size of 16 64bit pointers. For other available sizes see StackString types further down in
/// this file.
template<size_t N>
struct StackStringTempl final {

	// Public members
	// --------------------------------------------------------------------------------------------

	char string[N] = "\0";

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	StackStringTempl() noexcept = default;
	StackStringTempl(const StackStringTempl&) noexcept = default;
	StackStringTempl& operator= (const StackStringTempl&) noexcept = default;
	~StackStringTempl() noexcept = default;

	StackStringTempl(const char* string) noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	
};

// StackString types
// ------------------------------------------------------------------------------------------------

/// Extra small StackString, equivalent to the size of 4 64bit pointers.
using StackStringXS = StackStringTempl<32>;

/// Small StackString, equivalent to the size of 8 64bit pointers.
using StackStringS = StackStringTempl<64>;

/// Default StackString, equivalent to the size of 16 64bit pointers.
using StackString = StackStringTempl<128>;

/// Large StackString, equivalent to the size of 32 64bit pointers.
using StackStringL = StackStringTempl<256>;

/// Extra large StackString, equivalent to the size of 64 64bit pointers
using StackStringXL = StackStringTempl<512>;

/// Extra extra large StackString, equivalent to the size of 128 64bit pointers.
using StackStringXXL = StackStringTempl<1024>;

} // namespace sfz
