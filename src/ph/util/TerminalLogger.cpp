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

#include "ph/util/TerminalLogger.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace ph {

// Singleton instance
// ------------------------------------------------------------------------------------------------

TerminalLogger& TerminalLogger::instance() noexcept
{
	static TerminalLogger logger;
	return logger;
}

// TerminalLogger: Methods
// ------------------------------------------------------------------------------------------------

void TerminalLogger::init(uint32_t numHistoryItems, Allocator* allocator) noexcept
{
	mMessages.create(numHistoryItems, allocator);
}

uint32_t TerminalLogger::numMessages() const noexcept
{
	return uint32_t(mMessages.size());
}

const TerminalMessageItem& TerminalLogger::getMessage(uint32_t index) const noexcept
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
#ifdef _WIN32
	const char* strippedFile = std::strrchr(file, '\\') + 1;
#else
	const char* strippedFile = std::strrchr(file, '/') + 1;
#endif
	if (strippedFile == nullptr) strippedFile = file;

	// Remove oldest item
	if (mMessages.size() == mMessages.capacity()) {
		mMessages.pop();
	}

	// Create new item.
	mMessages.add();
	TerminalMessageItem& item = mMessages.last();

	// Fill item with message
	item.file.printf("%s", strippedFile);
	item.lineNumber = line;
	item.timestamp = std::time(nullptr);
	item.level = level;
	item.tag.printf("%s", tag);

	va_list args;
	va_start(args, format);
	vsnprintf(item.message.str, item.message.maxSize(), format, args);
	va_end(args);
};

} // namespace ph
