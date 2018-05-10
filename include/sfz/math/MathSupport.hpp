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
#include <cstdint>
#include <cstdlib>

#include "sfz/CudaCompatibility.hpp"
#include "sfz/SimdIntrinsics.hpp"
#include "sfz/math/Matrix.hpp"
#include "sfz/math/Vector.hpp"
#include "sfz/math/Quaternion.hpp"

namespace sfz {

using std::int32_t;
using std::uint32_t;

// Floating-point constants
// ------------------------------------------------------------------------------------------------

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f/ PI;

// approxEqual()
// ------------------------------------------------------------------------------------------------

/// Approximate equal function for floating point types.

constexpr float APPROX_EQUAL_EPS = 0.001f;

SFZ_CUDA_CALL bool approxEqual(float lhs, float rhs, float epsilon = APPROX_EQUAL_EPS) noexcept;

SFZ_CUDA_CALL bool approxEqual(vec2 lhs, vec2 rhs, float epsilon = APPROX_EQUAL_EPS) noexcept;
SFZ_CUDA_CALL bool approxEqual(vec3 lhs, vec3 rhs, float epsilon = APPROX_EQUAL_EPS) noexcept;
SFZ_CUDA_CALL bool approxEqual(vec4 lhs, vec4 rhs, float epsilon = APPROX_EQUAL_EPS) noexcept;

template<uint32_t M, uint32_t N>
SFZ_CUDA_CALL bool approxEqual(const Matrix<float,M,N>& lhs, const Matrix<float,M,N>& rhs,
                               float epsilon = APPROX_EQUAL_EPS) noexcept;

SFZ_CUDA_CALL bool approxEqual(Quaternion lhs, Quaternion rhs, float epsilon = APPROX_EQUAL_EPS) noexcept;

// abs()
// ------------------------------------------------------------------------------------------------

/// Returns the absolute value of the argument. The vector versions returns the absolute value of
/// each element in the vector.

SFZ_CUDA_CALL float abs(float val) noexcept;
SFZ_CUDA_CALL int32_t abs(int32_t val) noexcept;

SFZ_CUDA_CALL vec2 abs(vec2 val) noexcept;
SFZ_CUDA_CALL vec3 abs(vec3 val) noexcept;
SFZ_CUDA_CALL vec4 abs(vec4 val) noexcept;

SFZ_CUDA_CALL vec2_s32 abs(vec2_s32 val) noexcept;
SFZ_CUDA_CALL vec3_s32 abs(vec3_s32 val) noexcept;
SFZ_CUDA_CALL vec4_s32 abs(vec4_s32 val) noexcept;

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

SFZ_CUDA_CALL vec2_s32 sgn(vec2_s32 val) noexcept;
SFZ_CUDA_CALL vec3_s32 sgn(vec3_s32 val) noexcept;
SFZ_CUDA_CALL vec4_s32 sgn(vec4_s32 val) noexcept;

// min()
// ------------------------------------------------------------------------------------------------

/// Returns the minimum value of two elements. Element-wise results for vector types.

SFZ_CUDA_CALL float min(float lhs, float rhs) noexcept;
SFZ_CUDA_CALL int32_t min(int32_t lhs, int32_t rhs) noexcept;
SFZ_CUDA_CALL uint32_t min(uint32_t lhs, uint32_t rhs) noexcept;

SFZ_CUDA_CALL vec2 min(vec2 lhs, vec2 rhs) noexcept;
SFZ_CUDA_CALL vec3 min(vec3 lhs, vec3 rhs) noexcept;
SFZ_CUDA_CALL vec4 min(vec4 lhs, vec4 rhs) noexcept;

SFZ_CUDA_CALL vec2 min(vec2 lhs, float rhs) noexcept;
SFZ_CUDA_CALL vec2 min(float lhs, vec2 rhs) noexcept;
SFZ_CUDA_CALL vec3 min(vec3 lhs, float rhs) noexcept;
SFZ_CUDA_CALL vec3 min(float lhs, vec3 rhs) noexcept;
SFZ_CUDA_CALL vec4 min(vec4 lhs, float rhs) noexcept;
SFZ_CUDA_CALL vec4 min(float lhs, vec4 rhs) noexcept;

SFZ_CUDA_CALL vec2_s32 min(vec2_s32 lhs, vec2_s32 rhs) noexcept;
SFZ_CUDA_CALL vec3_s32 min(vec3_s32 lhs, vec3_s32 rhs) noexcept;
SFZ_CUDA_CALL vec4_s32 min(vec4_s32 lhs, vec4_s32 rhs) noexcept;

SFZ_CUDA_CALL vec2_s32 min(vec2_s32 lhs, int32_t rhs) noexcept;
SFZ_CUDA_CALL vec2_s32 min(int32_t lhs, vec2_s32 rhs) noexcept;
SFZ_CUDA_CALL vec3_s32 min(vec3_s32 lhs, int32_t rhs) noexcept;
SFZ_CUDA_CALL vec3_s32 min(int32_t lhs, vec3_s32 rhs) noexcept;
SFZ_CUDA_CALL vec4_s32 min(vec4_s32 lhs, int32_t rhs) noexcept;
SFZ_CUDA_CALL vec4_s32 min(int32_t lhs, vec4_s32 rhs) noexcept;

SFZ_CUDA_CALL vec2_u32 min(vec2_u32 lhs, vec2_u32 rhs) noexcept;
SFZ_CUDA_CALL vec3_u32 min(vec3_u32 lhs, vec3_u32 rhs) noexcept;
SFZ_CUDA_CALL vec4_u32 min(vec4_u32 lhs, vec4_u32 rhs) noexcept;

SFZ_CUDA_CALL vec2_u32 min(vec2_u32 lhs, uint32_t rhs) noexcept;
SFZ_CUDA_CALL vec2_u32 min(uint32_t lhs, vec2_u32 rhs) noexcept;
SFZ_CUDA_CALL vec3_u32 min(vec3_u32 lhs, uint32_t rhs) noexcept;
SFZ_CUDA_CALL vec3_u32 min(uint32_t lhs, vec3_u32 rhs) noexcept;
SFZ_CUDA_CALL vec4_u32 min(vec4_u32 lhs, uint32_t rhs) noexcept;
SFZ_CUDA_CALL vec4_u32 min(uint32_t lhs, vec4_u32 rhs) noexcept;

// max()
// ------------------------------------------------------------------------------------------------

/// Returns the maximum value of two elements. Element-wise results for vector types.

SFZ_CUDA_CALL float max(float lhs, float rhs) noexcept;
SFZ_CUDA_CALL int32_t max(int32_t lhs, int32_t rhs) noexcept;
SFZ_CUDA_CALL uint32_t max(uint32_t lhs, uint32_t rhs) noexcept;

SFZ_CUDA_CALL vec2 max(vec2 lhs, vec2 rhs) noexcept;
SFZ_CUDA_CALL vec3 max(vec3 lhs, vec3 rhs) noexcept;
SFZ_CUDA_CALL vec4 max(vec4 lhs, vec4 rhs) noexcept;

SFZ_CUDA_CALL vec2 max(vec2 lhs, float rhs) noexcept;
SFZ_CUDA_CALL vec2 max(float lhs, vec2 rhs) noexcept;
SFZ_CUDA_CALL vec3 max(vec3 lhs, float rhs) noexcept;
SFZ_CUDA_CALL vec3 max(float lhs, vec3 rhs) noexcept;
SFZ_CUDA_CALL vec4 max(vec4 lhs, float rhs) noexcept;
SFZ_CUDA_CALL vec4 max(float lhs, vec4 rhs) noexcept;

SFZ_CUDA_CALL vec2_s32 max(vec2_s32 lhs, vec2_s32 rhs) noexcept;
SFZ_CUDA_CALL vec3_s32 max(vec3_s32 lhs, vec3_s32 rhs) noexcept;
SFZ_CUDA_CALL vec4_s32 max(vec4_s32 lhs, vec4_s32 rhs) noexcept;

SFZ_CUDA_CALL vec2_s32 max(vec2_s32 lhs, int32_t rhs) noexcept;
SFZ_CUDA_CALL vec2_s32 max(int32_t lhs, vec2_s32 rhs) noexcept;
SFZ_CUDA_CALL vec3_s32 max(vec3_s32 lhs, int32_t rhs) noexcept;
SFZ_CUDA_CALL vec3_s32 max(int32_t lhs, vec3_s32 rhs) noexcept;
SFZ_CUDA_CALL vec4_s32 max(vec4_s32 lhs, int32_t rhs) noexcept;
SFZ_CUDA_CALL vec4_s32 max(int32_t lhs, vec4_s32 rhs) noexcept;

SFZ_CUDA_CALL vec2_u32 max(vec2_u32 lhs, vec2_u32 rhs) noexcept;
SFZ_CUDA_CALL vec3_u32 max(vec3_u32 lhs, vec3_u32 rhs) noexcept;
SFZ_CUDA_CALL vec4_u32 max(vec4_u32 lhs, vec4_u32 rhs) noexcept;

SFZ_CUDA_CALL vec2_u32 max(vec2_u32 lhs, uint32_t rhs) noexcept;
SFZ_CUDA_CALL vec2_u32 max(uint32_t lhs, vec2_u32 rhs) noexcept;
SFZ_CUDA_CALL vec3_u32 max(vec3_u32 lhs, uint32_t rhs) noexcept;
SFZ_CUDA_CALL vec3_u32 max(uint32_t lhs, vec3_u32 rhs) noexcept;
SFZ_CUDA_CALL vec4_u32 max(vec4_u32 lhs, uint32_t rhs) noexcept;
SFZ_CUDA_CALL vec4_u32 max(uint32_t lhs, vec4_u32 rhs) noexcept;

// minElement()
// ------------------------------------------------------------------------------------------------

/// Returns the smallest element in a vector.

SFZ_CUDA_CALL float minElement(vec2 val) noexcept;
SFZ_CUDA_CALL float minElement(vec3 val) noexcept;
SFZ_CUDA_CALL float minElement(vec4 val) noexcept;

SFZ_CUDA_CALL int32_t minElement(vec2_s32 val) noexcept;
SFZ_CUDA_CALL int32_t minElement(vec3_s32 val) noexcept;
SFZ_CUDA_CALL int32_t minElement(vec4_s32 val) noexcept;

SFZ_CUDA_CALL uint32_t minElement(vec2_u32 val) noexcept;
SFZ_CUDA_CALL uint32_t minElement(vec3_u32 val) noexcept;
SFZ_CUDA_CALL uint32_t minElement(vec4_u32 val) noexcept;

// maxElement()
// ------------------------------------------------------------------------------------------------

/// Returns the largest element in a vector.

SFZ_CUDA_CALL float maxElement(vec2 val) noexcept;
SFZ_CUDA_CALL float maxElement(vec3 val) noexcept;
SFZ_CUDA_CALL float maxElement(vec4 val) noexcept;

SFZ_CUDA_CALL int32_t maxElement(vec2_s32 val) noexcept;
SFZ_CUDA_CALL int32_t maxElement(vec3_s32 val) noexcept;
SFZ_CUDA_CALL int32_t maxElement(vec4_s32 val) noexcept;

SFZ_CUDA_CALL uint32_t maxElement(vec2_u32 val) noexcept;
SFZ_CUDA_CALL uint32_t maxElement(vec3_u32 val) noexcept;
SFZ_CUDA_CALL uint32_t maxElement(vec4_u32 val) noexcept;

// clamp() & saturate()
// ------------------------------------------------------------------------------------------------

/// Clamps an argument within the specfied interval. For vector types the limits can be either
/// vectors or scalars. saturate() is a special case of clamp where the parameter is clamped to
/// [0, 1] range.

template<typename ArgT, typename LimitT = ArgT>
SFZ_CUDA_CALL ArgT clamp(const ArgT& value, const LimitT& minValue, const LimitT& maxValue) noexcept;

SFZ_CUDA_CALL float saturate(float value) noexcept;
SFZ_CUDA_CALL vec2 saturate(vec2 value) noexcept;
SFZ_CUDA_CALL vec3 saturate(vec3 value) noexcept;
SFZ_CUDA_CALL vec4 saturate(vec4 value) noexcept;

// lerp()
// ------------------------------------------------------------------------------------------------

/// Linearly interpolates between two arguments. t should be a scalar in the range [0, 1].
/// Quaternions has a specialized version, which assumes that both the parameter Quaternions are
/// unit. In addition, the resulting Quaternion is normalized before returned.

template<typename ArgT, typename FloatT = ArgT>
SFZ_CUDA_CALL ArgT lerp(ArgT v0, ArgT v1, FloatT t) noexcept;

template<>
SFZ_CUDA_CALL Quaternion lerp(Quaternion q0, Quaternion q1, float t) noexcept;

// fma()
// ------------------------------------------------------------------------------------------------

/// Fused multiply add operation. Calculates a * b + c in one operation (if supported by the
/// hardware).

SFZ_CUDA_CALL float fma(float a, float b, float c) noexcept;
SFZ_CUDA_CALL vec2 fma(vec2 a, vec2 b, vec2 c) noexcept;
SFZ_CUDA_CALL vec3 fma(vec3 a, vec3 b, vec3 c) noexcept;
SFZ_CUDA_CALL vec4 fma(vec4 a, vec4 b, vec4 c) noexcept;

} // namespace sfz

#include "sfz/math/MathSupport.inl"
