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

#include "sfz/util/LoggingInterface.hpp"
#include "sfz/memory/Allocator.hpp"

namespace sfz {

// sfzCore Context struct
// ------------------------------------------------------------------------------------------------

/// The sfzCore context
///
/// Each application using sfzCore must create a context and set it with setContext() before sfzCore
/// is used. All global (singleton) state of sfzCore is stored in this context.
///
/// If using sfzCore with dynamically linked libraries it is necessary to initialize these libraries
/// by sending them a pointer to the context so they themselves can set their sfzCore context to be
/// the same as for the main application. Otherwise multiple contexts will exist, which will likely
/// cause dangerous problems.
struct Context final {

	/// The default allocator used by sfzCore. Will be used if no allocator is specified for
	/// functions and classes that uses such. This should be set in the beginning of the program,
	/// remaing valid for the rest of it, and never be replaced in the context. If it is replaced
	/// by another allocator you must make very sure that sfzCore has not allocated any memory using
	/// the previous instance, which can be very hard.
	Allocator* defaultAllocator = nullptr;

	/// The current logger used by sfzCore. See `sfz/logging/Logging.hpp` for the logging macros
	/// use this logger.
	LoggingInterface* logger = nullptr;
};

// Context getters/setters
// ------------------------------------------------------------------------------------------------

/// Gets the current sfzCore context. Will return nullptr if it has not been set using setContext().
/// \return pointer to the context
Context* getContext() noexcept;

/// Sets the current sfzCore context. Will not take ownership of the Context struct itself, so the
/// caller has to ensure the pointer remains valid for the remaining duration of the program.
/// If the pointer has already been set this function does nothing and returns false.
/// \param context the sfzCore context to set
/// \return whether context was set or not
bool setContext(Context* context) noexcept;

// Convenience getters
// ------------------------------------------------------------------------------------------------

/// Returns pointer to the default Allocator.
inline Allocator* getDefaultAllocator() noexcept
{
	return getContext()->defaultAllocator;
}

/// Returns pointer to the current logger.
inline LoggingInterface* getLogger() noexcept
{
	return getContext()->logger;
}

// Standard context
// ------------------------------------------------------------------------------------------------

/// Returns pointer to the standard context. This context can be used if you don't want to create
/// and setup a context yourself. To use this context you could use the following call in the
/// beginning of your program:
///
/// sfz::setContext(sfz::getStandardContext());
///
/// \return pointer to the standard context
Context* getStandardContext() noexcept;

} // namespace sfz
