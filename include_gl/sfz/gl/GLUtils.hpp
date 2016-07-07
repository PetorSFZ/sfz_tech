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

namespace sfz {

namespace gl {

/// Prints system information to stdout.
void printSystemGLInfo() noexcept;

// OpenGL debug message functions
// ------------------------------------------------------------------------------------------------

/// Severity level for OpenGL debug messages
/// Ordered by "severity", NOTIFICATION < LOW < MEDIUM < HIGH < NONE.
/// Specifying NOTIFICATION means all messages will be accepted, NONE means no messages will be.
enum class Severity {
	NOTIFICATION,
	LOW,
	MEDIUM,
	HIGH,
	NONE
};

/// Setups so OpenGL debug messages will be printed using sfz::printErrorMessage()
/// Requires an OpenGL debug context to function properly. Should be called after OpenGL
/// extension loading is done.
/// \param acceptLevel the lowest level of message severity accepted and printed
/// \param breakLevel the lowest level of message severity which will break to debugger
void setupDebugMessages(Severity acceptLevel = Severity::LOW,
                        Severity breakLevel = Severity::HIGH) noexcept;

/// Sets the lowest level of message severity accepted and printed
void setDebugMessageAcceptLevel(Severity level) noexcept;

/// Sets the lowest level of message severity which will break to debugger
void setDebugMessageBreakLevel(Severity level) noexcept;

} // namespace gl
} // namespace sfz
