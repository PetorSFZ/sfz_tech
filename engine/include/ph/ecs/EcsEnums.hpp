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

namespace ph {

// ECS type enum
// ------------------------------------------------------------------------------------------------

// These enums are used to represent which type of ECS system is inside a memory chunk in an
// EcsContainer. The first 4 bytes (i.e. uint32_t) in each ECS memory chunk must be this type
// enum in all Phantasy Engine ECS systems.

constexpr uint32_t ECS_TYPE_UNDEFINED = 0;
constexpr uint32_t ECS_TYPE_NAIVE = 1;

inline const char* ecsTypeToString(uint32_t type) noexcept
{
	switch (type) {
	case ECS_TYPE_NAIVE: return "Naive";
	default: break;
	}
	return "Undefined";
}

} // namespace ph
