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

#ifndef SFZ_DEFER_HPP
#define SFZ_DEFER_HPP
#pragma once

#include "sfz.h"

// Defer
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

template<typename F>
struct SfzDeferCallable final {
	F func;
	SfzDeferCallable(F func) : func(func) {}
	~SfzDeferCallable() { func(); }
};

struct SfzDeferMaker final {
	template<typename F>
	SfzDeferCallable<F> operator<< (F func) const { return SfzDeferCallable<F>(func); }
};

#define SFZ_DEFER_NAME_1(x, y) x##y
#define SFZ_DEFER_NAME_2(x, y) SFZ_DEFER_NAME_1(x, y)

#define sfz_defer auto SFZ_DEFER_NAME_2(deferCallable, __COUNTER__) = SfzDeferMaker() <<

#endif // __cplusplus

#endif // SFZ_DEFER_HPP
