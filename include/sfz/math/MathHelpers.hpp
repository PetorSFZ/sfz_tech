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

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "sfz/math/Vector.hpp"
#include "sfz/math/Matrix.hpp"

namespace sfz {

using std::size_t;

template<typename T>
T defaultEpsilon() { return T(0.0001); }

template<typename T>
bool approxEqual(T lhs, T rhs, T epsilon) noexcept;

template<typename T>
bool approxEqual(T lhs, T rhs) noexcept;

template<typename T, size_t N>
bool approxEqual(const Vector<T,N>& lhs, const Vector<T,N>& rhs, T epsilon) noexcept;

template<typename T, size_t N>
bool approxEqual(const Vector<T,N>& lhs, const Vector<T,N>& rhs) noexcept;

template<typename T, size_t M, size_t N>
bool approxEqual(const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs, T epsilon) noexcept;

template<typename T, size_t M, size_t N>
bool approxEqual(const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs) noexcept;

/// Lerp function, v0 when t == 0 and v1 when t == 1
/// See: http://en.wikipedia.org/wiki/Lerp_%28computing%29
template<typename ArgT, typename FloatT>
ArgT lerp(ArgT v0, ArgT v1, FloatT t) noexcept;

template<typename T>
T clamp(T value, T minValue, T maxValue);

} // namespace sfz

#include "sfz/math/MathHelpers.inl"
