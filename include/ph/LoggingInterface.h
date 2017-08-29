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

// C interface
#ifdef __cplusplus
extern "C" {
#endif

// LogLevel enum
// ------------------------------------------------------------------------------------------------

typedef enum {
	LOG_LEVEL_INFO_EXTRA_DETAILED = 0,
	LOG_LEVEL_INFO_DETAILED,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_END_TOKEN
} phLogLevel;

// Logger struct
// ------------------------------------------------------------------------------------------------

typedef struct {
	void (*log)(phLogLevel logLevel, const char* tag, const char* format, ...);
} phLogger;

// Logging function
// ------------------------------------------------------------------------------------------------

/// Example usage: PH_LOGGER_LOG(logger, LOG_LEVEL_WARNING, "GameplaySystem", "Too many enemies, num: %u", numEnemies);
#define PH_LOGGER_LOG(logger, logLevel, tag, format, ...) (logger).log((logLevel), (tag), (format),  ##__VA_ARGS__)


// End C interface
#ifdef __cplusplus
}
#endif
