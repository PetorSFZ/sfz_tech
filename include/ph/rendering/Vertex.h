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
using std::uint32_t;
#else
#include <stdint.h>
#endif

// C Vertex struct
// ------------------------------------------------------------------------------------------------

extern "C"
typedef struct {
	float pos[3];
	float normal[3];
	float texcoord[2];
} phVertex;

// C++ Vertex struct
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace ph {

using sfz::vec3;
using sfz::vec2;

struct Vertex final {
	vec3 pos = vec3(0.0f);
	vec3 normal = vec3(0.0f);
	vec2 texcoord = vec2(0.0f);
};

} // namespace ph

static_assert(sizeof(ph::Vertex) == sizeof(float) * 8, "ph::Vertex is padded");
static_assert(sizeof(phVertex) == sizeof(float) * 8, "phVertex is padded");
static_assert(sizeof(ph::Vertex) == sizeof(phVertex), "phVertex and ph::Vertex are different size");

#endif
