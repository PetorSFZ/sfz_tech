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

#include "common/Logging.hpp"

// Invalid argument macro
// ------------------------------------------------------------------------------------------------

// Simple macro that checks that a boolean condition is false. If it is isn't, the condition
// (along with the function it is in) is logged together with the specified error string. Then
// ZG_ERROR_INVALID_ARGUMENT is returned.
#define ZG_ARG_CHECK(cond, errorString) \
	if ((cond)) { \
		ZG_ERROR("%s(): Invalid argument \"%s\": %s", __func__, #cond, (errorString)); \
		return ZG_ERROR_INVALID_ARGUMENT; \
	}
