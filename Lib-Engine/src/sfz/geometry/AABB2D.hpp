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

#include <skipifzero.hpp>

namespace sfz {

/// A POD class representing a 2D AABB
struct AABB2D final {

	// Public members
	// --------------------------------------------------------------------------------------------

	f32x2 min, max;

	// Constructors and destructors
	// --------------------------------------------------------------------------------------------

	AABB2D() noexcept = default;
	AABB2D(const AABB2D&) noexcept = default;
	AABB2D& operator= (const AABB2D&) noexcept = default;
	~AABB2D() noexcept = default;

	inline AABB2D(f32x2 centerPos, f32x2 dimensions) noexcept
	:
		min(centerPos - (dimensions * 0.5f)),
		max(centerPos + (dimensions * 0.5f))
	{ }

	inline AABB2D(f32 centerX, f32 centerY, f32 width, f32 height) noexcept
	:
		min(f32x2(centerX - (width * 0.5f), centerY - (height * 0.5f))),
		max(f32x2(centerX + (width * 0.5f), centerY + (height * 0.5f)))
	{ }

	// Public getters
	// --------------------------------------------------------------------------------------------

	inline f32x2 position() const noexcept { return (min + max) * 0.5f; }
	inline f32 x() const noexcept { return (min.x + max.x) * 0.5f; }
	inline f32 y() const noexcept { return (min.y + max.y) * 0.5f; }
	inline f32x2 dimensions() const noexcept { return max - min; }
	inline f32 width() const noexcept { return max.x - min.x; }
	inline f32 height() const noexcept { return max.y - min.y; }

	// Comparison operators
	// --------------------------------------------------------------------------------------------

	inline bool operator== (const AABB2D& other) const noexcept
	{
		return min == other.min && max == other.max;
	}

	inline bool operator!= (const AABB2D& other) const noexcept
	{
		return !((*this) == other);
	}
};

using Rectangle = AABB2D;

} // namespace sfz
