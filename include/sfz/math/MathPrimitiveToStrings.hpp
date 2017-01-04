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

#include "sfz/math/Matrix.hpp"
#include "sfz/math/Vector.hpp"
#include "sfz/strings/StackString.hpp"

namespace sfz {

// Vector toString()
// ------------------------------------------------------------------------------------------------

StackString toString(const vec2& vector, uint32_t numDecimals = 2) noexcept;
StackString toString(const vec3& vector, uint32_t numDecimals = 2) noexcept;
StackString toString(const vec4& vector, uint32_t numDecimals = 2) noexcept;

void toString(const vec2& vector, StackString& string, uint32_t numDecimals = 2) noexcept;
void toString(const vec3& vector, StackString& string, uint32_t numDecimals = 2) noexcept;
void toString(const vec4& vector, StackString& string, uint32_t numDecimals = 2) noexcept;

StackString toString(const vec2i& vector) noexcept;
StackString toString(const vec3i& vector) noexcept;
StackString toString(const vec4i& vector) noexcept;

void toString(const vec2i& vector, StackString& string) noexcept;
void toString(const vec3i& vector, StackString& string) noexcept;
void toString(const vec4i& vector, StackString& string) noexcept;

StackString toString(const vec2u& vector) noexcept;
StackString toString(const vec3u& vector) noexcept;
StackString toString(const vec4u& vector) noexcept;

void toString(const vec2u& vector, StackString& string) noexcept;
void toString(const vec3u& vector, StackString& string) noexcept;
void toString(const vec4u& vector, StackString& string) noexcept;

// Matrix toString()
// ------------------------------------------------------------------------------------------------

StackString256 toString(const mat22& matrix, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;
StackString256 toString(const mat33& matrix, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;
StackString256 toString(const mat44& matrix, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;

void toString(const mat22& matrix, StackString256& string, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;
void toString(const mat33& matrix, StackString256& string, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;
void toString(const mat44& matrix, StackString256& string, bool rowBreak = false, uint32_t numDecimals = 2) noexcept;

} // namespace sfz
