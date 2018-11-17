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

#include <cassert>

#ifdef _WIN32
#include <intrin.h>
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

// Utility functions
// ------------------------------------------------------------------------------------------------

/// Terminates the program (wrapper for std::terminate())
void terminateProgram() noexcept;

} // namespace sfz

#include "sfz/Assert.inl"
