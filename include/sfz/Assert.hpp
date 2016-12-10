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

#ifdef _WIN32
#include <intrin.h>
#else
#include <cassert>
#endif

// Debug assert
// ------------------------------------------------------------------------------------------------

/// Stops program or opens debugger if condition is false.
/// To be used often to catch bugs during debugging progress. Should normally only be enabled in
/// debug builds. Disabled by defining SFZ_NO_DEBUG, but also by defining SFZ_NO_ASSERTIONS.
#define sfz_assert_debug(condition) sfz_assert_debug_impl(condition)

// Release assert
// ------------------------------------------------------------------------------------------------

/// Stops program or opens debugger if condition is false.
/// To be used for more serious things that you want to catch quickly even in a release build.
/// Should normally always be enabled, but can be disabled by defining SFZ_NO_ASSERTIONS.
#define sfz_assert_release(condition) sfz_assert_release_impl(condition)

namespace sfz {

// Errors
// ------------------------------------------------------------------------------------------------

/// Stops execution of program and displays message. This is meant to be used for errors that
/// can't be recovered from and should crash the program. Uses standard printf() formatting for
/// error message, will append a newline if appropriate so this should not be done manually.
void error(const char* format, ...) noexcept;

// Debug utility functions
// ------------------------------------------------------------------------------------------------

/// Prints an error message in an appropriate way for the given context, uses printf() formatting.
/// Will append a newline if appropriate, so this should not be manually done.
void printErrorMessage(const char* format, ...) noexcept;

/// Terminates the program (wrapper for std::terminate())
void terminateProgram() noexcept;

} // namespace sfz

#include "sfz/Assert.inl"
