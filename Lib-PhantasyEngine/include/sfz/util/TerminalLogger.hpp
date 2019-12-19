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
#include <ctime>

#include <skipifzero_strings.hpp>
#include <skipifzero_ring_buffers.hpp>

#include <sfz/util/LoggingInterface.hpp>

namespace sfz {

using sfz::Allocator;
using sfz::LogLevel;
using sfz::RingBuffer;
using sfz::str32;
using sfz::str64;
using sfz::str2048;

using std::time_t;

// TerminalMessageItem struct
// ------------------------------------------------------------------------------------------------

struct TerminalMessageItem final {
	str64 file;
	int32_t lineNumber;
	time_t timestamp;
	LogLevel level;
	str32 tag;
	str2048 message;
};

// TerminalLogger class
// ------------------------------------------------------------------------------------------------

class TerminalLogger final : public sfz::LoggingInterface {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	TerminalLogger() noexcept = default;
	TerminalLogger(const TerminalLogger&) = delete;
	TerminalLogger& operator= (const TerminalLogger&) = delete;
	TerminalLogger(TerminalLogger&&) = delete;
	TerminalLogger& operator= (TerminalLogger&&) = delete;

	// Methods
	// --------------------------------------------------------------------------------------------

	void init(uint32_t numHistoryItems, Allocator* allocator) noexcept;

	/// Returns current number of messages
	uint32_t numMessages() const noexcept;

	/// Returns message
	const TerminalMessageItem& getMessage(uint32_t index) const noexcept;

	void clearMessages() noexcept { mMessages.clear(); }

	// Overriden methods from LoggingInterface
	// --------------------------------------------------------------------------------------------

	void log(
		const char* file,
		int line,
		LogLevel level,
		const char* tag,
		const char* format,
		...) noexcept override final;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	RingBuffer<TerminalMessageItem> mMessages;
};

// Statically owned logger
// ------------------------------------------------------------------------------------------------

/// Statically owned TerminalLogger. Default constructed. Only to be used when creating the
/// Phantasy Engine context at boot in PhantasyEngineMain.cpp.
TerminalLogger* getStaticTerminalLoggerForBoot() noexcept;

} // namespace sfz
