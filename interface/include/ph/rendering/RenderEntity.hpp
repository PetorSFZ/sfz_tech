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
#include <sfz/math/Quaternion.hpp>
#include <sfz/math/Vector.hpp>

// RenderEntity struct
// ------------------------------------------------------------------------------------------------

struct phRenderEntity final {
	sfz::Quaternion rotation = sfz::Quaternion::identity();
	sfz::vec3 scale = sfz::vec3(1.0f);
	uint32_t meshIndex = ~0u;
	sfz::vec3 translation = sfz::vec3(0.0f);
	uint32_t ___PADDING__ = ~0u; // TODO: material override for whole mesh

	sfz::mat34 transform() const
	{
		// Apply rotation first
		sfz::mat34 tmp = rotation.toMat34();

		// Matrix multiply in scale (order does not matter)
		sfz::vec4 scaleVec = sfz::vec4(scale, 1.0f);
		tmp.row0 *= scaleVec;
		tmp.row1 *= scaleVec;
		tmp.row2 *= scaleVec;

		// Add translation (last)
		tmp.setColumn(3, translation);

		return tmp;
	}
};
static_assert(sizeof(phRenderEntity) == sizeof(uint32_t) * 12, "phRenderEntity is padded");
