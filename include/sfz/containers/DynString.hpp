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

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "sfz/containers/DynArray.hpp"
#include "sfz/memory/Allocators.hpp"

namespace sfz {

using std::size_t;
using std::uint32_t;

// DynString (interface)
// ------------------------------------------------------------------------------------------------

template<typename Allocator>
class DynStringTempl final {
public:
	
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	DynStringTempl() noexcept = default;
	DynStringTempl(const DynStringTempl&) noexcept = default;
	DynStringTempl& operator= (const DynStringTempl&) noexcept = default;
	DynStringTempl(DynStringTempl&&) noexcept = default;
	DynStringTempl& operator= (DynStringTempl&&) noexcept = default;

	/// Constructs a DynString with the specified string and capacity. The internal capacity will
	/// be at least large enough to hold the entire string regardless of the value of the capacity
	/// parameter. If the string is shorter than the specified capacity or a nullptr then the 
	/// internal capacity will be set to the specified capacity.
	/// \param string a null-terminated string or nullptr
	/// \param capacity the capacity of the internal DynArray
	DynStringTempl(const char* string, uint32_t capacity = 0) noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	const char* str() const noexcept { return mString.data(); }
	char* str() noexcept { return mString.data(); }

	uint32_t size() const noexcept { return mString.size(); }
	uint32_t capacity() const noexcept { return mString.capacity(); }


private:
	// Private members
	// --------------------------------------------------------------------------------------------

	DynArray<char,Allocator> mString;
};

// Default typedef
// ------------------------------------------------------------------------------------------------

using DynString = DynStringTempl<StandardAllocator>;

} // namespace sfz

#include "sfz/containers/DynString.inl"
