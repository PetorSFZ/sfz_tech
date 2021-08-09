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

#include "sfz/math/MathPrimitiveToStrings.hpp"

namespace sfz {

// Vector toString()
// ------------------------------------------------------------------------------------------------

str96 toString(const vec2& vector, u32 numDecimals) noexcept
{
	str96 tmp;
	toString(vector, tmp, numDecimals);
	return tmp;
}

str96 toString(const vec3& vector, u32 numDecimals) noexcept
{
	str96 tmp;
	toString(vector, tmp, numDecimals);
	return tmp;
}

str96 toString(const vec4& vector, u32 numDecimals) noexcept
{
	str96 tmp;
	toString(vector, tmp, numDecimals);
	return tmp;
}

void toString(const vec2& vector, str96& string, u32 numDecimals) noexcept
{
	str32 formatStr;
	formatStr.appendf("[%%.%uf, %%.%uf]", numDecimals, numDecimals);
	string.appendf(formatStr.str(), vector.x, vector.y);
}

void toString(const vec3& vector, str96& string, u32 numDecimals) noexcept
{
	str32 formatStr;
	formatStr.appendf("[%%.%uf, %%.%uf, %%.%uf]", numDecimals, numDecimals, numDecimals);
	string.appendf(formatStr.str(), vector.x, vector.y, vector.z);
}

void toString(const vec4& vector, str96& string, u32 numDecimals) noexcept
{
	str32 formatStr;
	formatStr.appendf("[%%.%uf, %%.%uf, %%.%uf, %%.%uf]", numDecimals, numDecimals, numDecimals, numDecimals);
	string.appendf(formatStr.str(), vector.x, vector.y, vector.z, vector.w);
}

str96 toString(const vec2_i32& vector) noexcept
{
	str96 tmp;
	toString(vector, tmp);
	return tmp;
}

str96 toString(const vec3_i32& vector) noexcept
{
	str96 tmp;
	toString(vector, tmp);
	return tmp;
}

str96 toString(const vec4_i32& vector) noexcept
{
	str96 tmp;
	toString(vector, tmp);
	return tmp;
}

void toString(const vec2_i32& vector, str96& string) noexcept
{
	string.appendf("[%i, %i]", vector.x, vector.y);
}

void toString(const vec3_i32& vector, str96& string) noexcept
{
	string.appendf("[%i, %i, %i]", vector.x, vector.y, vector.z);
}

void toString(const vec4_i32& vector, str96& string) noexcept
{
	string.appendf("[%i, %i, %i, %i]", vector.x, vector.y, vector.z, vector.w);
}

str96 toString(const vec2_u32& vector) noexcept
{
	str96 tmp;
	toString(vector, tmp);
	return tmp;
}

str96 toString(const vec3_u32& vector) noexcept
{
	str96 tmp;
	toString(vector, tmp);
	return tmp;
}

str96 toString(const vec4_u32& vector) noexcept
{
	str96 tmp;
	toString(vector, tmp);
	return tmp;
}

void toString(const vec2_u32& vector, str96& string) noexcept
{
	string.appendf("[%u, %u]", vector.x, vector.y);
}

void toString(const vec3_u32& vector, str96& string) noexcept
{
	string.appendf("[%u, %u, %u]", vector.x, vector.y, vector.z);
}

void toString(const vec4_u32& vector, str96& string) noexcept
{
	string.appendf("[%u, %u, %u, %u]", vector.x, vector.y, vector.z, vector.w);
}

// Matrix toString()
// ------------------------------------------------------------------------------------------------

str256 toString(const mat22& matrix, bool rowBreak, u32 numDecimals) noexcept
{
	str256 tmp;
	toString(matrix, tmp, rowBreak, numDecimals);
	return tmp;
}

str256 toString(const mat33& matrix, bool rowBreak, u32 numDecimals) noexcept
{
	str256 tmp;
	toString(matrix, tmp, rowBreak, numDecimals);
	return tmp;
}

str256 toString(const mat44& matrix, bool rowBreak, u32 numDecimals) noexcept
{
	str256 tmp;
	toString(matrix, tmp, rowBreak, numDecimals);
	return tmp;
}

template<u32 H, u32 W>
void toStringImpl(const Mat<float,H,W>& matrix, str256& string, bool rowBreak, u32 numDecimals) noexcept
{
	string.clear();
	string.appendf("[");
	str96 tmp;
	for (u32 y = 0; y < H; y++) {
		tmp.clear();
		toString(matrix.row(y), tmp, numDecimals);
		string.appendf("%s", tmp.str());
		if (y < (H-1)) {
			if (rowBreak) {
				string.appendf(",\n ");
			} else {
				string.appendf(", ");
			}
		}
	}
	string.appendf("]");
}

void toString(const mat22& matrix, str256& string, bool rowBreak, u32 numDecimals) noexcept
{
	toStringImpl(matrix, string, rowBreak, numDecimals);
}

void toString(const mat33& matrix, str256& string, bool rowBreak, u32 numDecimals) noexcept
{
	toStringImpl(matrix, string, rowBreak, numDecimals);
}

void toString(const mat44& matrix, str256& string, bool rowBreak, u32 numDecimals) noexcept
{
	toStringImpl(matrix, string, rowBreak, numDecimals);
}

} // namespace sfz
