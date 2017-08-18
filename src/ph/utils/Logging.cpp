// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include <ph/utils/Logging.hpp>

#include <cstdarg>
#include <cstdio>

namespace ph {

void logImpl(LogLevel level, const char* tag, const char* format, ...) noexcept
{
	// Append LogLevel, tag and newline to format
	char actualFormat[384]; // Large because the message might be passed in the format string
	std::snprintf(actualFormat, sizeof(actualFormat), "[%s] -- [%s]: %s\n", toString(level), tag, format);

	// Print message
	va_list args;
	va_start(args, format);
	switch(level) {
	case LogLevel::INFO_INTRICATE:
	case LogLevel::INFO:
	case LogLevel::WARNING:
		std::vfprintf(stdout, actualFormat, args);
		break;
	case LogLevel::ERROR_LVL:
		std::vfprintf(stderr, actualFormat, args);
		break;
	}
	va_end(args);
}

} // namespace ph
