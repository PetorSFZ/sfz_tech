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
#include <sfz/math/Matrix.hpp>
using std::uint32_t;
#else
#include <stdint.h>
#endif

#include "ph/ExternC.h"

// C RenderEntity struct
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	float transform[12]; // 3x4 right-handed row-major matrix
	uint32_t meshIndex; // The index of the mesh to render
	uint32_t padding[3];
} phRenderEntity;

// C++ RenderEntity struct
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace ph {

using sfz::mat34;

struct RenderEntity final {
	mat34 transform = mat34::identity();
	uint32_t meshIndex = ~0;
	uint32_t padding[3];
};

static_assert(sizeof(phRenderEntity) == sizeof(RenderEntity), "RenderEntity is padded");
static_assert(alignof(phRenderEntity) <= alignof(RenderEntity), "phRenderEntity has higher alignment requirements");

} // namespace ph

#endif
