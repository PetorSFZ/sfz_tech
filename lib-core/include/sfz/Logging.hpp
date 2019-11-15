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

#include "sfz/Assert.hpp"
#include "sfz/Context.hpp"
#include "sfz/util/LoggingInterface.hpp"

// Logging macros
// ------------------------------------------------------------------------------------------------

#define SFZ_LOG(logLevel, tag, format, ...) sfz::getLogger()->log( \
	__FILE__, __LINE__, (logLevel), (tag), (format), ##__VA_ARGS__)

#define SFZ_INFO_NOISY(tag, format, ...) SFZ_LOG(sfz::LogLevel::INFO_NOISY, (tag), (format), ##__VA_ARGS__)

#define SFZ_INFO(tag, format, ...) SFZ_LOG(sfz::LogLevel::INFO, (tag), (format), ##__VA_ARGS__)

#define SFZ_WARNING(tag, format, ...) SFZ_LOG(sfz::LogLevel::WARNING, (tag), (format), ##__VA_ARGS__)

#define SFZ_ERROR(tag, format, ...) SFZ_LOG(sfz::LogLevel::ERROR_LVL, (tag), (format), ##__VA_ARGS__)

#define SFZ_ERROR_AND_EXIT(tag, format, ...) \
{ \
	SFZ_ERROR(tag, format, ##__VA_ARGS__); \
	sfz_assert_hard(false); \
}
