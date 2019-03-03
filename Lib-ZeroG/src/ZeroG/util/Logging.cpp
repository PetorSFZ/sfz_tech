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

#include "ZeroG/util/Logging.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "ZeroG/util/Assert.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

static const char* toString(ZgLogLevel level) noexcept
{
	switch (level) {
	case ZG_LOG_LEVEL_INFO: return "INFO";
	case ZG_LOG_LEVEL_WARNING: return "WARNING";
	case ZG_LOG_LEVEL_ERROR: return "ERROR";
	}
	ZG_ASSERT(false);
	return "";
}

static const char* stripFilePath(const char* file) noexcept
{
	ZG_ASSERT(file != nullptr);
	const char* strippedFile1 = std::strrchr(file, '\\');
	const char* strippedFile2 = std::strrchr(file, '/');
	if (strippedFile1 == nullptr && strippedFile2 == nullptr) {
		return file;
	}
	else if (strippedFile2 == nullptr) {
		return strippedFile1 + 1;
	}
	else {
		return strippedFile2 + 1;
	}
}

static void defaultLog(
	void* userPtr, const char* file, int line, ZgLogLevel level, const char* message) noexcept
{
	(void)userPtr;

	// Strip path from file
	const char* strippedFile = stripFilePath(file);

	// Print message
	printf("[ZeroG] -- [%s] -- [%s:%i]:\n%s\n\n", toString(level), strippedFile, line, message);

	// Flush stdout
	fflush(stdout);
}

// Logger wrappers for logging macros
// ------------------------------------------------------------------------------------------------

void logWrapper(
	ZgContext& ctx, const char* file, int line, ZgLogLevel level, const char* fmt, ...) noexcept
{
	// Create message
	char messageBuffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(messageBuffer, sizeof(messageBuffer), fmt, args);
	va_end(args);

	// Log
	ctx.logger.log(ctx.logger.userPtr, file, line, level, messageBuffer);
}

void logWrapper(
	ZgLogger& logger, const char* file, int line, ZgLogLevel level, const char* fmt, ...) noexcept
{
	// Create message
	char messageBuffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(messageBuffer, sizeof(messageBuffer), fmt, args);
	va_end(args);
	
	// Log
	logger.log(logger.userPtr, file, line, level, messageBuffer);
}

// Default logger
// ------------------------------------------------------------------------------------------------

ZgLogger getDefaultLogger() noexcept
{
	ZgLogger logger = {};
	logger.log = defaultLog;
	return logger;
}

} // namespace zg
