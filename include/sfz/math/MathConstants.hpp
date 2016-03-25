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

#include <cmath>

namespace sfz {

template<typename T = float>
constexpr T PI() noexcept { return T(3.14159265358979323846); }

template<typename T = float>
constexpr T DEG_TO_RAD() noexcept { return PI<T>() / T(180); }

template<typename T = float>
constexpr T RAD_TO_DEG() noexcept { return T(180) / PI<T>(); }

} // namespace sfz
