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

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

// C typedef
// ------------------------------------------------------------------------------------------------

typedef uint32_t phBool32;

// C++ Bool32 struct
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus
namespace ph {

struct Bool32 final {

	// Implementation value
	uint32_t value;

	// Default constructors
	Bool32() noexcept = default;
	Bool32(const Bool32&) noexcept = default;
	Bool32& operator= (const Bool32&) noexcept = default;
	~Bool32() noexcept = default;

	// (Implicit callable) constructors from boolean values
	Bool32(phBool32 phBoolValue) noexcept : value((phBoolValue != 0) ? 1 : 0) { }
	Bool32(bool boolValue) noexcept : value(boolValue ? 1 : 0) { }

	// Implicit cast to boolean values
	operator bool() const noexcept { return value != 0; }
	operator phBool32() const noexcept { return (value != 0) ? 1 : 0; }
};
static_assert(sizeof(phBool32) == sizeof(Bool32), "ph::Bool32 is padded");

} // namespace ph
#endif
