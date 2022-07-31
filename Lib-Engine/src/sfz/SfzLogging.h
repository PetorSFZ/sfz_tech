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

#include <sfz.h>

// Logger
// ------------------------------------------------------------------------------------------------

typedef enum {
	SFZ_LOG_LEVEL_NOISE = 0,
	SFZ_LOG_LEVEL_INFO = 1,
	SFZ_LOG_LEVEL_WARNING = 2,
	SFZ_LOG_LEVEL_ERROR = 3,
	SFZ_LOG_LEVEL_FORCE_I32 = I32_MAX
} SfzLogLevel;

inline const char* sfzLogLevelToString(SfzLogLevel level)
{
	if (level == SFZ_LOG_LEVEL_NOISE) return "NOISE";
	if (level == SFZ_LOG_LEVEL_INFO) return "INFO";
	if (level == SFZ_LOG_LEVEL_WARNING) return "WARNING";
	if (level == SFZ_LOG_LEVEL_ERROR) return "ERROR";
	return "INVALID ENUM";
}

typedef void SfzLogFunc(void* implData, const char* file, int line, SfzLogLevel level, const char* format, ...);

// Logger used for most sfz stuff.
//
// The logger must be thread-safe. I.e. it must be okay to call it simulatenously from multiple
// threads.
sfz_struct(SfzLogger) {
	SfzLogFunc* log;
	void* implData;
};

// Logging state functions
// ------------------------------------------------------------------------------------------------

// Gets this module's static SfzLogger. This logger should typically be retrieved (only) in the
// main module at program boot, then set using "sfzLoggingSetLogger()" in this same module and all
// following modules (DLLs).
SFZ_EXTERN_C SfzLogger* sfzLoggingGetModulesLogger(void);

// Sets/gets the global logger. Typically the program should set the global logger right after
// creating it at program boot.
//
// If using multiple DLLs, the logger needs to be passed to each DLL so they can set it for their
// global variable space.
SFZ_EXTERN_C void sfzLoggingSetLogger(SfzLogger* logger);
SFZ_EXTERN_C SfzLogger* sfzLoggingGetLogger(void);

// Logging macros
// ------------------------------------------------------------------------------------------------

#define SFZ_LOG(logLevel, format, ...) \
	{ \
		SfzLogger* tmpLogger = sfzLoggingGetLogger(); \
		sfz_assert(tmpLogger != NULL); \
		tmpLogger->log(tmpLogger->implData, __FILE__, __LINE__, (logLevel), (format), ##__VA_ARGS__); \
	}

#define SFZ_LOG_NOISE(format, ...) SFZ_LOG(SFZ_LOG_LEVEL_NOISE, (format), ##__VA_ARGS__)
#define SFZ_LOG_INFO(format, ...) SFZ_LOG(SFZ_LOG_LEVEL_INFO, (format), ##__VA_ARGS__)
#define SFZ_LOG_WARNING(format, ...) SFZ_LOG(SFZ_LOG_LEVEL_WARNING, (format), ##__VA_ARGS__)
#define SFZ_LOG_ERROR(format, ...) SFZ_LOG(SFZ_LOG_LEVEL_ERROR, (format), ##__VA_ARGS__)

// Logging helper functions
// ------------------------------------------------------------------------------------------------

// Helper functions used to access internal state in order to display it in console UI.
//
// Note that these will get data from THIS MODULE's logger, NOT the one set with
// sfzLoggingSetLogger(). The reasoning for this is that the user might have replaced the logger
// with a custom one which implementation we know nothing about. As long as the console UI lives
// in the same module as the logger this is not a problem.
//
// Unlike the logger itself, these functions are not thread-safe. In practice this should probably
// not be a major issue.

SFZ_EXTERN_C u32 sfzLoggingCurrentNumMessages(void);
SFZ_EXTERN_C u32 sfzLoggingGetNumMessagesWithAgeLessThan(f32 maxAgeSecs);
SFZ_EXTERN_C i32 sfzLoggingGetMessageLine(u32 msgIdx);
SFZ_EXTERN_C const char* sfzLoggingGetMessageFile(u32 msgIdx);
SFZ_EXTERN_C SfzLogLevel sfzLoggingGetMessageLevel(u32 msgIdx);
SFZ_EXTERN_C i64 sfzLoggingGetMessageTimestamp(u32 msgIdx);
SFZ_EXTERN_C const char* sfzLoggingGetMessageMessage(u32 msgIdx);

SFZ_EXTERN_C void sfzLoggingClearMessages(void);
