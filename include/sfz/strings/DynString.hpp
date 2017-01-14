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

#include "sfz/containers/DynArray.hpp"
#include "sfz/memory/Allocator.hpp"

namespace sfz {

using std::int32_t;
using std::uint32_t;

// DynString class
// ------------------------------------------------------------------------------------------------

/// A class for managing a dynamic string, replacement for std::string.
///
/// Implemented using a (private) DynArray, many of the functions are simply wrappers around the
/// DynArray interface. Check out DynArray for more specific documentation on these functions.
class DynString final {
public:
	
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	DynString() noexcept = default;
	DynString(const DynString&) noexcept = default;
	DynString& operator= (const DynString&) noexcept = default;
	DynString(DynString&&) noexcept = default;
	DynString& operator= (DynString&&) noexcept = default;

	/// Constructs a DynString with the specified string and capacity. The internal capacity will
	/// be at least large enough to hold the entire string regardless of the value of the capacity
	/// parameter. If the string is shorter than the specified capacity or a nullptr then the 
	/// internal capacity will be set to the specified capacity.
	/// \param string a null-terminated string or nullptr
	/// \param capacity the capacity of the internal DynArray
	explicit DynString(const char* string, uint32_t capacity = 0,
	                   Allocator* allocator = getDefaultAllocator()) noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	const char* str() const noexcept { return mString.data(); }
	char* str() noexcept { return mString.data(); }

	/// Returns length of the internal string minus the null-terminator. If the size of the
	/// internal DynArray is non-zero then this method will return DynArray.size() - 1.
	uint32_t size() const noexcept;
	uint32_t capacity() const noexcept { return mString.capacity(); }

	Allocator* allocator() const noexcept { return mString.allocator(); }

	const DynArray<char>& internalDynArray() const noexcept { return mString; }
	DynArray<char>& internalDynArray() noexcept { return mString; }

	// Public methods (DynArray)
	// --------------------------------------------------------------------------------------------

	void swap(DynString& other) noexcept { mString.swap(other.mString); }
	void setCapacity(uint32_t capacity) noexcept { mString.setCapacity(capacity); }
	void clear() noexcept { mString.clear(); }
	void destroy() noexcept { mString.destroy(); }

	// Public methods (DynString)
	// --------------------------------------------------------------------------------------------

	/// Calls snprintf() on the internal string, overwriting the content.
	/// \return number of chars written
	int32_t printf(const char* format, ...) noexcept;

	/// Calls snprintf() on the remaining part of the internal string, effectively appending to it.
	/// \return number of chars written
	int32_t printfAppend(const char* format, ...) noexcept;

	// Operators
	// --------------------------------------------------------------------------------------------

	bool operator== (const DynString& other) const noexcept;
	bool operator!= (const DynString& other) const noexcept;
	bool operator< (const DynString& other) const noexcept;
	bool operator<= (const DynString& other) const noexcept;
	bool operator> (const DynString& other) const noexcept;
	bool operator>= (const DynString& other) const noexcept;

	bool operator== (const char* other) const noexcept;
	bool operator!= (const char* other) const noexcept;
	bool operator< (const char* other) const noexcept;
	bool operator<= (const char* other) const noexcept;
	bool operator> (const char* other) const noexcept;
	bool operator>= (const char* other) const noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	DynArray<char> mString;
};

} // namespace sfz
