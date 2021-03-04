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

#include <skipifzero.hpp>

namespace sfz {

// OptVal serializable
// ------------------------------------------------------------------------------------------------

// The OptVal type specifies that the member is optional in the serialized representation.
//
// It has two different states:
// Valid: Whether the value held is valid to get() or not.
// Default: Whether the value held is the "default" value or not. Default value implies that there
//          is no need to serialize it, as the value is the same as what you would get if nothing
//          was specified in the serialized representation.
//
// Example:
//     struct Foo {
//         OptVal<int> val1; // Not valid and not default until something is read from serialized.
//         OptVal<int> val2 = 3; // Valid and default, unless something is read from serialized
//                               // then it is no longer default.
//     };
template<typename T>
class OptVal final {
public:
	OptVal() = default;
	OptVal(const T& val) { set(val); mDefault = true; }
	OptVal(T&& val) { set(std::move(val)); mDefault = true; }

	bool valid() const { return mValid; }
	bool isDefault() const { return mDefault; }

	T& get() { sfz_assert(mValid); return mVal; }
	const T& get() const { sfz_assert(mValid); return mVal; }

	void set(const T& val) { mValid = true; mDefault = false; mVal = val; }
	void set(T&& val) { mValid = true; mDefault = false; mVal = std::move(val); }

	void unset() { this->mValid = false; this->mDefault = false; this->mVal = {}; }

	void setDefault(bool defaultVal) { this->mDefault = defaultVal; }
	
private:
	T mVal = {};
	bool mValid = false;
	bool mDefault = false;
};

} // namespace sfz
