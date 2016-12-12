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
#include <cstdint>

#include <nmmintrin.h> // SSE 4.2

#include "sfz/CudaCompatibility.hpp"
#include "sfz/math/Matrix.hpp"
#include "sfz/math/Vector.hpp"

namespace sfz {

using std::int32_t;
using std::uint32_t;

// Floating-point constants
// ------------------------------------------------------------------------------------------------

template<typename T = float>
SFZ_CUDA_CALL T PI() noexcept { return T(3.14159265358979323846); }

template<typename T = float>
SFZ_CUDA_CALL T DEG_TO_RAD() noexcept { return PI<T>() / T(180); }

template<typename T = float>
SFZ_CUDA_CALL T RAD_TO_DEG() noexcept { return T(180) / PI<T>(); }

template<typename T = float>
SFZ_CUDA_CALL T defaultEpsilon() { return T(0.0001); }

// Approximate equal functions
// ------------------------------------------------------------------------------------------------

template<typename T, typename EpsT = T>
SFZ_CUDA_CALL bool approxEqual(T lhs, T rhs, EpsT epsilon = defaultEpsilon<EpsT>()) noexcept;

template<typename T, uint32_t N>
SFZ_CUDA_CALL bool approxEqual(const Vector<T,N>& lhs, const Vector<T,N>& rhs,
                               T epsilon = defaultEpsilon<T>()) noexcept;

template<typename T, uint32_t M, uint32_t N>
SFZ_CUDA_CALL bool approxEqual(const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs,
                               T epsilon = defaultEpsilon<T>()) noexcept;

// abs()
// ------------------------------------------------------------------------------------------------

/// Returns the absolute value of the argument. The vector versions returns the absolute value of
/// each element in the vector.

SFZ_CUDA_CALL float abs(float val) noexcept;
SFZ_CUDA_CALL int32_t abs(int32_t val) noexcept;

SFZ_CUDA_CALL vec2 abs(vec2 val) noexcept;
SFZ_CUDA_CALL vec3 abs(vec3 val) noexcept;
SFZ_CUDA_CALL vec4 abs(vec4 val) noexcept;

SFZ_CUDA_CALL vec2i abs(vec2i val) noexcept;
SFZ_CUDA_CALL vec3i abs(vec3i val) noexcept;
SFZ_CUDA_CALL vec4i abs(vec4i val) noexcept;

// sgn()
// ------------------------------------------------------------------------------------------------

/// Returns the sign of the parameter value. For integers it will return -1, 0 or 1. For floating
/// points values it will always return -1.0f or 1.0f (as IEEE-754 has positive and negative 0).
/// Element-wise results for vector types.

SFZ_CUDA_CALL float sgn(float val) noexcept;
SFZ_CUDA_CALL int32_t sgn(int32_t val) noexcept;

SFZ_CUDA_CALL vec2 sgn(vec2 val) noexcept;
SFZ_CUDA_CALL vec3 sgn(vec3 val) noexcept;
SFZ_CUDA_CALL vec4 sgn(vec4 val) noexcept;

SFZ_CUDA_CALL vec2i sgn(vec2i val) noexcept;
SFZ_CUDA_CALL vec3i sgn(vec3i val) noexcept;
SFZ_CUDA_CALL vec4i sgn(vec4i val) noexcept;

// min()
// ------------------------------------------------------------------------------------------------

// max()
// ------------------------------------------------------------------------------------------------

// old
// ------------------------------------------------------------------------------------------------

/// Lerp function, v0 when t == 0 and v1 when t == 1
/// See: http://en.wikipedia.org/wiki/Lerp_%28computing%29
template<typename ArgT, typename FloatT>
SFZ_CUDA_CALL ArgT lerp(ArgT v0, ArgT v1, FloatT t) noexcept;

template<typename T>
SFZ_CUDA_CALL T clamp(T value, T minValue, T maxValue);

} // namespace sfz

#include "sfz/math/MathSupport.inl"
