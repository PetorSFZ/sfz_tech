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

#include "sfz/Assert.hpp"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <exception> // std::terminate()

namespace sfz {

// Errors
// ------------------------------------------------------------------------------------------------

void error(const char* format, ...) noexcept
{
	// Append newline to format
	char actualFormat[384]; // Large because the message might be passed in the format string
	std::snprintf(actualFormat, sizeof(actualFormat), "%s\n", format);

	// Print message to stderr
	va_list args;
	va_start(args, format);
	std::vfprintf(stderr, actualFormat, args);
	va_end(args);

	// Attempt to open debugger
	assert(false);

	// Terminate program
	terminateProgram();
}

// Debug utility functions
// ------------------------------------------------------------------------------------------------

void printErrorMessage(const char* format, ...) noexcept
{
	// Append newline to format
	char actualFormat[384]; // Large because the message might be passed in the format string
	std::snprintf(actualFormat, sizeof(actualFormat), "%s\n", format);

	// Print message to stderr
	va_list args;
	va_start(args, format);
	std::vfprintf(stderr, actualFormat, args);
	va_end(args);
}

void terminateProgram() noexcept
{
	std::terminate();
}

} // namespace sfz