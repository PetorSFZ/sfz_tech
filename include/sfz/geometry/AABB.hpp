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

#include <array>
#include <functional> // std::hash

#include "sfz/Assert.hpp"
#include "sfz/math/Vector.hpp"

namespace sfz {

struct AABB final {
	
	// Members
	// --------------------------------------------------------------------------------------------

	vec3 min, max;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------
	
	AABB() noexcept = default;
	AABB(const AABB&) noexcept = default;
	AABB& operator= (const AABB&) noexcept = default;
	~AABB() noexcept = default;

	inline AABB(const vec3& min, const vec3& max) noexcept;
	inline AABB(const vec3& centerPos, float xExtent, float yExtent, float zExtent) noexcept;

	// Member functions
	// --------------------------------------------------------------------------------------------

	inline std::array<vec3, 8> corners() const noexcept;
	inline void corners(vec3* arrayOut) const noexcept;
	inline vec3 closestPoint(const vec3& point) const noexcept;

	inline size_t hash() const noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	inline vec3 position() const noexcept { return this->min + (extents() * 0.5f); }
	inline vec3 extents() const noexcept { return this->max - this->min; }
	inline float xExtent() const noexcept { return this->max.x - this->min.x; }
	inline float yExtent() const noexcept { return this->max.y - this->min.y; }
	inline float zExtent() const noexcept { return this->max.z - this->min.z; }
	inline vec3 halfExtents() const noexcept { return extents() * 0.5f; }
	inline float halfXExtent() const noexcept { return xExtent() * 0.5f; }
	inline float halfYExtent() const noexcept { return yExtent() * 0.5f; }
	inline float halfZExtent() const noexcept { return zExtent() * 0.5f; }

	// Setters
	// --------------------------------------------------------------------------------------------

	inline void position(const vec3& newCenterPos) noexcept;
	inline void extents(const vec3& newExtents) noexcept;
	inline void xExtent(float newXExtent) noexcept;
	inline void yExtent(float newYExtent) noexcept;
	inline void zExtent(float newZExtent) noexcept;
};

} // namespace sfz

// Specializations of standard library for sfz::AABB
// ------------------------------------------------------------------------------------------------

namespace std {

template<>
struct hash<sfz::AABB> {
	inline size_t operator() (const sfz::AABB& aabb) const noexcept;
};

} // namespace std

#include "sfz/geometry/AABB.inl"
