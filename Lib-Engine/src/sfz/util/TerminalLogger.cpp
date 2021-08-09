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

#include "sfz/util/TerminalLogger.hpp"

#include "sfz/util/IO.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#endif

namespace sfz {

// TerminalLogger: Methods
// ------------------------------------------------------------------------------------------------

void TerminalLogger::init(u32 numHistoryItems, SfzAllocator* allocator) noexcept
{
	mMessages.create(numHistoryItems, allocator, sfz_dbg(""));
}

u32 TerminalLogger::numMessages() const noexcept
{
	return u32(mMessages.size());
}

const TerminalMessageItem& TerminalLogger::getMessage(u32 index) const noexcept
{
	return mMessages[index];
}

// TerminalLogger: Overriden methods from LoggingInterface
// ------------------------------------------------------------------------------------------------

void TerminalLogger::log(
	const char* file,
	int line,
	LogLevel level,
	const char* tag,
	const char* format,
	...) noexcept
{
	// Strip path from file
	const char* strippedFile = getFileNameFromPath(file);

	// Remove oldest item
	if (mMessages.size() == mMessages.capacity()) {
		mMessages.pop();
	}

	// Create new item.
	mMessages.add();
	TerminalMessageItem& item = mMessages.last();

	// Fill item with message
	item.file.clear();
	item.file.appendf("%s", strippedFile);
	item.lineNumber = line;
	item.timestamp = std::time(nullptr);
	item.level = level;
	item.tag.clear();
	item.tag.appendf("%s", tag);

	va_list args;
	va_start(args, format);
	vsnprintf(item.message.mRawStr, item.message.capacity(), format, args);
	va_end(args);

	// Also log to terminal
	// TODO: Make this into an option
	bool printToTerminal = true;
	if (printToTerminal) {

		// Set terminal color
#ifdef _WIN32
		static HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (consoleHandle != INVALID_HANDLE_VALUE) {
			if (level == LogLevel::INFO) {
				SetConsoleTextAttribute(consoleHandle, FOREGROUND_GREEN);
			}
			else if (level == LogLevel::WARNING) {
				SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			}
			else if (level == LogLevel::ERROR_LVL) {
				SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
			}
		}
#endif

		// Get time string
		std::tm* tmPtr = std::localtime(&item.timestamp);
		sfz::str32 timeStr;
		size_t res = std::strftime(timeStr.mRawStr, timeStr.capacity(), "%H:%M:%S", tmPtr);
		if (res == 0) {
			timeStr.clear();
			timeStr.appendf("INVALID TIME");
		}

		// Print log level, tag, file and line number.
		printf("[%s] - [%s] - [%s] - [%s:%i]\n",
			timeStr.str(), toString(level), tag, strippedFile, line);

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
};

} // namespace sfz
