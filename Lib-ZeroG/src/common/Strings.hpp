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

#include <cstdarg>
#include <cstdio>

#include <skipifzero.hpp>

// String helper functions
// ------------------------------------------------------------------------------------------------

// Usage:
// constexpr uint32_t STRING_SIZE = 128;
// char originalString[STRING_SIZE] = {};
// char* tmpStr = originalString;
// tmpStr[0] = '\0';
// uint32_t bytesLeft = STRING_SIZE;
// printfAppend(tmpStr, bytesLeft, "text");
// printfAppend(tmpStr, bytesLeft, "more text");
inline void printfAppend(char*& str, uint32_t& bytesLeft, const char* format, ...) noexcept
{
	va_list args;
	va_start(args, format);
	int res = std::vsnprintf(str, bytesLeft, format, args);
	va_end(args);

	sfz_assert(res >= 0);
	sfz_assert(res < int(bytesLeft));
	bytesLeft -= res;
	str += res;
}
