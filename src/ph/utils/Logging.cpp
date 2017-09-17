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

// Logging function
// ------------------------------------------------------------------------------------------------

constexpr const char* LOG_LEVEL_STRINGS[] = {
	"INFO_EXTRA_DETAILED",
	"LOG_LEVEL_INFO_DETAILED",
	"INFO",
	"WARNING",
	"ERROR",
	"END_TOKEN"
};

static const char* toString(phLogLevel level) noexcept
{
	return LOG_LEVEL_STRINGS[int(level)];
}

static void logImpl(phLogLevel level, const char* tag, const char* format, ...) noexcept
{
	// Append LogLevel, tag and newline to format
	char actualFormat[384]; // Large because the message might be passed in the format string
	std::snprintf(actualFormat, sizeof(actualFormat), "[%s] -- [%s]: %s\n", toString(level), tag, format);

	// Print message
	va_list args;
	va_start(args, format);
	switch(level) {
	case LOG_LEVEL_INFO_EXTRA_DETAILED:
	case LOG_LEVEL_INFO_DETAILED:
	case LOG_LEVEL_INFO:
	case LOG_LEVEL_WARNING:
		std::vfprintf(stdout, actualFormat, args);
		std::fflush(stdout);
		break;
	case LOG_LEVEL_ERROR:
#ifdef SFZ_EMSCRIPTEN
	std::vfprintf(stdout, actualFormat, args);
	std::fflush(stdout);
#else
	std::vfprintf(stderr, actualFormat, args);
	std::fflush(stderr);
#endif
		break;
	case LOG_LEVEL_END_TOKEN:
	default:
		break;
	}
	va_end(args);
}

// Get logger function
// ------------------------------------------------------------------------------------------------

phLogger getLogger() noexcept
{
	static phLogger logger = {logImpl};
	return logger;
}

} // namespace ph
