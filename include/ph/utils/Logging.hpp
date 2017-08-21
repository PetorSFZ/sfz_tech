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

#pragma once

#include <cstdint>

namespace ph {

using std::uint32_t;

// LogLevel enum
// ------------------------------------------------------------------------------------------------

enum class LogLevel : uint32_t {
	INFO_INTRICATE = 0, // Really detailed info for deep debugging, normally off to avoid spam
	INFO,
	WARNING,
	ERROR_LVL, // Actually named ERROR, but that is defined by default. So _LVL to avoid conflicts.
	END_TOKEN
};

constexpr const char* LOG_LEVEL_STRINGS[] = {
	"INFO_INTRICATE",
	"INFO",
	"WARNING",
	"ERROR",
	"END_TOKEN"
};

inline const char* toString(LogLevel level) noexcept
{
	return LOG_LEVEL_STRINGS[uint32_t(level)];
}

constexpr uint32_t NUM_LOG_LEVELS = uint32_t(LogLevel::END_TOKEN);

// Logging function
// ------------------------------------------------------------------------------------------------

#define PH_LOG(logLevel, tag, format, ...) logImpl((logLevel), (tag), (format), ##__VA_ARGS__);

// Implementation
// ------------------------------------------------------------------------------------------------

void logImpl(LogLevel level, const char* tag, const char* format, ...) noexcept;

} // namespace ph
