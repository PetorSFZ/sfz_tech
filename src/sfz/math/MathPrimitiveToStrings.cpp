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

StackString toString(const vec2& vector, uint32_t numDecimals) noexcept
{
	StackString tmp;
	toString(vector, tmp, numDecimals);
	return tmp;
}

StackString toString(const vec3& vector, uint32_t numDecimals) noexcept
{
	StackString tmp;
	toString(vector, tmp, numDecimals);
	return tmp;
}

StackString toString(const vec4& vector, uint32_t numDecimals) noexcept
{
	StackString tmp;
	toString(vector, tmp, numDecimals);
	return tmp;
}

void toString(const vec2& vector, StackString& string, uint32_t numDecimals) noexcept
{
	StackString32 formatStr;
	formatStr.printf("[%%.%uf, %%.%uf]", numDecimals, numDecimals);
	string.printf(formatStr.str, vector.x, vector.y);
}

void toString(const vec3& vector, StackString& string, uint32_t numDecimals) noexcept
{
	StackString32 formatStr;
	formatStr.printf("[%%.%uf, %%.%uf, %%.%uf]", numDecimals, numDecimals, numDecimals);
	string.printf(formatStr.str, vector.x, vector.y, vector.z);
}

void toString(const vec4& vector, StackString& string, uint32_t numDecimals) noexcept
{
	StackString32 formatStr;
	formatStr.printf("[%%.%uf, %%.%uf, %%.%uf, %%.%uf]", numDecimals, numDecimals, numDecimals, numDecimals);
	string.printf(formatStr.str, vector.x, vector.y, vector.z, vector.w);
}

StackString toString(const vec2i& vector) noexcept
{
	StackString tmp;
	toString(vector, tmp);
	return tmp;
}

StackString toString(const vec3i& vector) noexcept
{
	StackString tmp;
	toString(vector, tmp);
	return tmp;
}

StackString toString(const vec4i& vector) noexcept
{
	StackString tmp;
	toString(vector, tmp);
	return tmp;
}

void toString(const vec2i& vector, StackString& string) noexcept
{
	string.printf("[%i, %i]", vector.x, vector.y);
}

void toString(const vec3i& vector, StackString& string) noexcept
{
	string.printf("[%i, %i, %i]", vector.x, vector.y, vector.z);
}

void toString(const vec4i& vector, StackString& string) noexcept
{
	string.printf("[%i, %i, %i, %i]", vector.x, vector.y, vector.z, vector.w);
}

StackString toString(const vec2u& vector) noexcept
{
	StackString tmp;
	toString(vector, tmp);
	return tmp;
}

StackString toString(const vec3u& vector) noexcept
{
	StackString tmp;
	toString(vector, tmp);
	return tmp;
}

StackString toString(const vec4u& vector) noexcept
{
	StackString tmp;
	toString(vector, tmp);
	return tmp;
}

void toString(const vec2u& vector, StackString& string) noexcept
{
	string.printf("[%u, %u]", vector.x, vector.y);
}

void toString(const vec3u& vector, StackString& string) noexcept
{
	string.printf("[%u, %u, %u]", vector.x, vector.y, vector.z);
}

void toString(const vec4u& vector, StackString& string) noexcept
{
	string.printf("[%u, %u, %u, %u]", vector.x, vector.y, vector.z, vector.w);
}

// Matrix toString()
// ------------------------------------------------------------------------------------------------

StackString256 toString(const mat22& matrix, bool rowBreak, uint32_t numDecimals) noexcept
{
	StackString256 tmp;
	toString(matrix, tmp, rowBreak, numDecimals);
	return tmp;
}

StackString256 toString(const mat33& matrix, bool rowBreak, uint32_t numDecimals) noexcept
{
	StackString256 tmp;
	toString(matrix, tmp, rowBreak, numDecimals);
	return tmp;
}

StackString256 toString(const mat44& matrix, bool rowBreak, uint32_t numDecimals) noexcept
{
	StackString256 tmp;
	toString(matrix, tmp, rowBreak, numDecimals);
	return tmp;
}

template<uint32_t H, uint32_t W>
void toStringImpl(const Matrix<float,H,W>& matrix, StackString256& string, bool rowBreak, uint32_t numDecimals) noexcept
{
	string.str[0] = '\0';
	string.printfAppend("[");
	StackString tmp;
	for (uint32_t y = 0; y < H; y++) {
		toString(matrix.rows[y], tmp, numDecimals);
		string.printfAppend("%s", tmp.str);
		if (y < (H-1)) {
			if (rowBreak) {
				string.printfAppend(",\n ");
			} else {
				string.printfAppend(", ");
			}
		}
	}
	string.printfAppend("]");
}

void toString(const mat22& matrix, StackString256& string, bool rowBreak, uint32_t numDecimals) noexcept
{
	toStringImpl(matrix, string, rowBreak, numDecimals);
}

void toString(const mat33& matrix, StackString256& string, bool rowBreak, uint32_t numDecimals) noexcept
{
	toStringImpl(matrix, string, rowBreak, numDecimals);
}

void toString(const mat44& matrix, StackString256& string, bool rowBreak, uint32_t numDecimals) noexcept
{
	toStringImpl(matrix, string, rowBreak, numDecimals);
}

} // namespace sfz
