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

#include "sfz/math/Vector.hpp"

namespace sfz {

/// A POD class representing a 2D AABB
struct AABB2D final {
	
	// Public members
	// --------------------------------------------------------------------------------------------

	vec2 min, max;

	// Constructors and destructors
	// --------------------------------------------------------------------------------------------

	AABB2D() noexcept = default;
	AABB2D(const AABB2D&) noexcept = default;
	AABB2D& operator= (const AABB2D&) noexcept = default;
	~AABB2D() noexcept = default;

	inline AABB2D::AABB2D(vec2 centerPos, vec2 dimensions) noexcept
	:
		min(centerPos - (dimensions / 2.0f)),
		max(centerPos + (dimensions / 2.0f))
	{ }

	inline AABB2D::AABB2D(float centerX, float centerY, float width, float height) noexcept
	:
		min(vec2(centerX - (width / 2.0f), centerY - (height / 2.0f))),
		max(vec2(centerX + (width / 2.0f), centerY + (height / 2.0f)))
	{ }

	// Public getters
	// --------------------------------------------------------------------------------------------

	inline vec2 position() const noexcept { return (min + max) / 2.0f; }
	inline float x() const noexcept { return (min.x + max.x) / 2.0f; }
	inline float y() const noexcept { return (min.y + max.y) / 2.0f; }
	inline vec2 dimensions() const noexcept { return max - min; }
	inline float width() const noexcept { return max.x - min.x; }
	inline float height() const noexcept { return max.y - min.y; }

	// Comparison operators
	// --------------------------------------------------------------------------------------------

	inline bool AABB2D::operator== (const AABB2D& other) const noexcept
	{
		return min == other.min && max == other.max;
	}

	inline bool AABB2D::operator!= (const AABB2D& other) const noexcept
	{
		return !((*this) == other);
	}
};

using Rectangle = AABB2D;

} // namespace sfz
