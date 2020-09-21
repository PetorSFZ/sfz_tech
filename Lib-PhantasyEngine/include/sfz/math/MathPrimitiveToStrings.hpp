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

#include <skipifzero.hpp>
#include <skipifzero_math.hpp>
#include <skipifzero_strings.hpp>

namespace sfz {

// Vector toString()
// ------------------------------------------------------------------------------------------------

str96 toString(const vec2& vector, uint32_t numDecimals = 2) noexcept;
str96 toString(const vec3& vector, uint32_t numDecimals = 2) noexcept;
str96 toString(const vec4& vector, uint32_t numDecimals = 2) noexcept;

void toString(const vec2& vector, str96& string, uint32_t numDecimals = 2) noexcept;
void toString(const vec3& vector, str96& string, uint32_t numDecimals = 2) noexcept;
void toString(const vec4& vector, str96& string, uint32_t numDecimals = 2) noexcept;

str96 toString(const vec2_i32& vector) noexcept;
str96 toString(const vec3_i32& vector) noexcept;
str96 toString(const vec4_i32& vector) noexcept;

void toString(const vec2_i32& vector, str96& string) noexcept;
void toString(const vec3_i32& vector, str96& string) noexcept;
void toString(const vec4_i32& vector, str96& string) noexcept;

str96 toString(const vec2_u32& vector) noexcept;
str96 toString(const vec3_u32& vector) noexcept;
str96 toString(const vec4_u32& vector) noexcept;

void toString(const vec2_u32& vector, str96& string) noexcept;
void toString(const vec3_u32& vector, str96& string) noexcept;
void toString(const vec4_u32& vector, str96& string) noexcept;

// Matrix toString()
// ------------------------------------------------------------------------------------------------

str256 toString(const mat22& matrix, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;
str256 toString(const mat33& matrix, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;
str256 toString(const mat44& matrix, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;

void toString(const mat22& matrix, str256& string, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;
void toString(const mat33& matrix, str256& string, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;
void toString(const mat44& matrix, str256& string, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;

} // namespace sfz
