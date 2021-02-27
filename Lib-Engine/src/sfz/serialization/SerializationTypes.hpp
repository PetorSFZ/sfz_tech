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

// Opt serializable
// ------------------------------------------------------------------------------------------------

// The opt type specifies that the member is optional in the serialized representation.
template<typename T>
struct Opt final {
	T val;
	Opt() = default;
	Opt(const T& valIn) : val(valIn) {}
	Opt(T&& valIn) : val(std::move(valIn)) {}
	Opt& operator= (const T& valIn) { this->val = valIn; return *this; }
	Opt& operator= (T&& valIn) { this->val = std::move(valIn); return *this; }
	operator T&() { return val; }
	operator const T&() const { return val; }
	T& operator() () { return val; }
	const T& operator() () const { return val; }
};

} // namespace sfz
