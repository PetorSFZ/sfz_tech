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
#include <sfz/math/Vector.hpp>
using sfz::vec2;
using sfz::vec4;
#else
#include <stdint.h>
#endif

#include "ph/ExternC.h"

// C Imgui rendering structs
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	float pos[2];
	float texcoord[2];
	uint32_t color;
} phImguiVertex;

PH_EXTERN_C
typedef struct {
	uint32_t idxBufferOffset;
	uint32_t numIndices;
	uint32_t padding[2];
	float clipRect[4];
} phImguiCommand;

// C++ Imgui rendering structs
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace ph {

struct ImguiVertex final {
	vec2 pos;
	vec2 texcoord;
	uint32_t color;
};
static_assert(sizeof(phImguiVertex) == sizeof(float) * 5, "phImguiVertex is padded");
static_assert(sizeof(phImguiVertex) == sizeof(ImguiVertex), "ImguiVertex is padded");

struct ImguiCommand final {
	uint32_t idxBufferOffset = 0;
	uint32_t numIndices = 0;
	uint32_t padding[2];
	vec4 clipRect = vec4(0.0f);
};
static_assert(sizeof(phImguiCommand) == sizeof(uint32_t) * 8, "phImguiCommand is padded");
static_assert(sizeof(phImguiCommand) == sizeof(ImguiCommand), "ImguiCommand is padded");

} // namespace ph

#endif
