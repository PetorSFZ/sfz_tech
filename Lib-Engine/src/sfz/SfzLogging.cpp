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

#include "sfz/SfzLogging.h"

#include <atomic>
#include <time.h> // time_t

#include <skipifzero_strings.hpp>

#include "sfz/util/IO.hpp"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#endif

// Constants
// ------------------------------------------------------------------------------------------------

sfz_constant u32 SFZ_LOGGER_MAX_NUM_MESSAGES = 256;
sfz_constant bool SFZ_LOGGER_LOG_TO_TERMINAL = true;

// Logger implementation
// ------------------------------------------------------------------------------------------------

struct SfzLogMessageItem final {
	i32 line;
	SfzLogLevel level;
	time_t timestamp;
	sfz::str64 file;
	sfz::str2048 message;
};

struct SfzLoggerImpl {
	SfzLogMessageItem messages[SFZ_LOGGER_MAX_NUM_MESSAGES];
	std::atomic_uint64_t nextMsgIdx;
};

static void sfzLogFunc(void* implData, const char* file, int line, SfzLogLevel level, const char* format, ...)
{
	SfzLoggerImpl& impl = *static_cast<SfzLoggerImpl*>(implData);

	// Strip path from file
	const char* strippedFile = sfz::getFileNameFromPath(file);

	// Get message data
	const u64 msgIdxWrapping = std::atomic_fetch_add(&impl.nextMsgIdx, u64(1));
	const u64 msgIdx = msgIdxWrapping % u64(SFZ_LOGGER_MAX_NUM_MESSAGES);
	SfzLogMessageItem& item = impl.messages[msgIdx];

	// Fill message data
	item.line = line;
	item.level = level;
	item.timestamp = ::time(nullptr);
	item.file.clear();
	item.file.appendf("%s", strippedFile);
	item.message.clear();
	
	va_list args;
	va_start(args, format);
	vsnprintf(item.message.mRawStr, item.message.capacity(), format, args);
	va_end(args);

	if (SFZ_LOGGER_LOG_TO_TERMINAL) {
		// Set terminal color
#ifdef _WIN32
		static HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (consoleHandle != INVALID_HANDLE_VALUE) {
			/*if (level == SFZ_LOG_LEVEL_NOISE) {
				SetConsoleTextAttribute(consoleHandle, FOREGROUND_BLUE);
			}
			else*/ if (level == SFZ_LOG_LEVEL_INFO) {
				SetConsoleTextAttribute(consoleHandle, FOREGROUND_GREEN);
			}
			else if (level == SFZ_LOG_LEVEL_WARNING) {
				SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			}
			else if (level == SFZ_LOG_LEVEL_ERROR) {
				SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
			}
		}
#endif

		// Get time string
		tm* tmPtr = ::localtime(&item.timestamp);
		sfz::str32 timeStr;
		size_t res = ::strftime(timeStr.mRawStr, timeStr.capacity(), "%H:%M:%S", tmPtr);
		if (res == 0) {
			timeStr.clear();
			timeStr.appendf("INVALID TIME");
		}

		// Print log level, file and line number.
		printf("[%s] - [%s] - [%s:%i]\n",
			timeStr.str(), sfzLogLevelToString(level), strippedFile, line);

		// Print message
		printf("%s", item.message.str());
		// Print newline
		printf("\n\n");

		// Flush stdout
		fflush(stdout);

		// Restore terminal color
#ifdef _WIN32
		if (consoleHandle != INVALID_HANDLE_VALUE) {
			SetConsoleTextAttribute(consoleHandle, 7);
		}
#endif
	}
}

// Logging state functions
// ------------------------------------------------------------------------------------------------

// Static memory holding a logger
static SfzLoggerImpl sfzLoggerImpl = {};
static SfzLogger sfzLoggerData = {};

// Points to the current global logger, for the main module this is the variables above, but for
// other DLLs this should be set using sfzLoggingSetLogger() to the main module's.
static SfzLogger* sfzGlobalLogger = nullptr;

SFZ_EXTERN_C SfzLogger* sfzLoggingGetModulesLogger(void)
{
	sfzLoggerData.log = sfzLogFunc;
	sfzLoggerData.implData = &sfzLoggerImpl;
	return &sfzLoggerData;
}

SFZ_EXTERN_C void sfzLoggingSetLogger(SfzLogger* logger)
{
	sfzGlobalLogger = logger;
}

SFZ_EXTERN_C SfzLogger* sfzLoggingGetLogger(void)
{
	return sfzGlobalLogger;
}

// Logging helper functions
// ------------------------------------------------------------------------------------------------

static SfzLogMessageItem* sfzLoggingGetItem(u32 msgIdxIn)
{
	sfz_assert(msgIdxIn < SFZ_LOGGER_MAX_NUM_MESSAGES);
	const u64 nextIdxWrapping = sfzLoggerImpl.nextMsgIdx;
	const u64 numMessages = sfzLoggingCurrentNumMessages();
	const u64 firstIdxWrapping = nextIdxWrapping - numMessages;
	const u64 msgIdxWrapping = firstIdxWrapping + msgIdxIn;
	const u32 msgIdx = u32(msgIdxWrapping % u64(SFZ_LOGGER_MAX_NUM_MESSAGES));
	return &sfzLoggerImpl.messages[msgIdx];
}

SFZ_EXTERN_C u32 sfzLoggingCurrentNumMessages(void)
{
	const u64 numMessages = sfzLoggerImpl.nextMsgIdx;
	if (numMessages > u64(SFZ_LOGGER_MAX_NUM_MESSAGES)) return SFZ_LOGGER_MAX_NUM_MESSAGES;
	return u32(numMessages);
}

SFZ_EXTERN_C u32 sfzLoggingGetNumMessagesWithAgeLessThan(f32 maxAgeSecs)
{
	const time_t now = ::time(nullptr);
	const u32 numMessages = sfzLoggingCurrentNumMessages();
	u32 numActiveMessages = 0;
	for (u32 i = 0; i < numMessages; i++) {
		// Reverse order, newest first
		const SfzLogMessageItem* item = sfzLoggingGetItem(numMessages - i - 1);
		const f32 age = f32(now - item->timestamp);
		if (age > maxAgeSecs) {
			break;
		}
		numActiveMessages = i + 1;
	}
	return numActiveMessages;
}

SFZ_EXTERN_C i32 sfzLoggingGetMessageLine(u32 msgIdx)
{
	const SfzLogMessageItem* item = sfzLoggingGetItem(msgIdx);
	return item->line;
}

SFZ_EXTERN_C const char* sfzLoggingGetMessageFile(u32 msgIdx)
{
	const SfzLogMessageItem* item = sfzLoggingGetItem(msgIdx);
	return item->file.str();
}

SFZ_EXTERN_C SfzLogLevel sfzLoggingGetMessageLevel(u32 msgIdx)
{
	const SfzLogMessageItem* item = sfzLoggingGetItem(msgIdx);
	return item->level;
}

SFZ_EXTERN_C i64 sfzLoggingGetMessageTimestamp(u32 msgIdx)
{
	const SfzLogMessageItem* item = sfzLoggingGetItem(msgIdx);
	return i64(item->timestamp);
}

SFZ_EXTERN_C const char* sfzLoggingGetMessageMessage(u32 msgIdx)
{
	const SfzLogMessageItem* item = sfzLoggingGetItem(msgIdx);
	return item->message.str();
}

SFZ_EXTERN_C void sfzLoggingClearMessages(void)
{
	sfzLoggerImpl.nextMsgIdx = 0;
}
