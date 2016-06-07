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

#include <cmath> // std::abs
#include <functional> // std::hash

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

	inline AABB2D(vec2 centerPos, vec2 dimensions) noexcept;
	inline AABB2D(float centerX, float centerY, float width, float height) noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	inline size_t hash() const noexcept;

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

	inline bool operator== (const AABB2D& other) const noexcept;
	inline bool operator!= (const AABB2D& other) const noexcept;
};

// Standard typedefs
// ------------------------------------------------------------------------------------------------

using Rectangle = AABB2D;

} // namespace sfz

// Specializations of standard library for sfz::AABB2D
// ------------------------------------------------------------------------------------------------

namespace std {

template<>
struct hash<sfz::AABB2D> {
	inline size_t operator() (const sfz::AABB2D& aabb) const noexcept;
};

} // namespace std

#include "sfz/geometry/AABB2D.inl"
