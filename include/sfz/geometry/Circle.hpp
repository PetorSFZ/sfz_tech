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

/// A POD class representing a Circle
struct Circle final {
	
	// Public members
	// --------------------------------------------------------------------------------------------

	vec2 pos;
	float radius;

	// Constructors and destructors
	// --------------------------------------------------------------------------------------------

	Circle() noexcept = default;
	Circle(const Circle&) noexcept = default;
	Circle& operator= (const Circle&) noexcept = default;
	~Circle() noexcept = default;

	inline Circle::Circle(vec2 centerPos, float radius) noexcept
	:
		pos(centerPos),
		radius(radius)
	{ }

	inline Circle::Circle(float centerX, float centerY, float radius) noexcept
	:
		pos(centerX, centerY),
		radius(radius)
	{ }

	// Public getters
	// --------------------------------------------------------------------------------------------

	inline float x() const noexcept { return pos.x; }
	inline float y() const noexcept { return pos.y; }

	// Comparison operators
	// --------------------------------------------------------------------------------------------

	inline bool Circle::operator== (const Circle& other) const noexcept
	{
		return pos == other.pos && radius == other.radius;
	}

	inline bool Circle::operator!= (const Circle& other) const noexcept
	{
		return !((*this) == other);
	}
};

} // namespace sfz
