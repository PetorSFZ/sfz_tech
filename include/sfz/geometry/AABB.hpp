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

/// Class representing an Axis-Aligned Bounding Box.
class AABB final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	AABB() noexcept = default;
	AABB(const AABB&) noexcept = default;
	AABB& operator= (const AABB&) noexcept = default;

	inline AABB(const vec3& min, const vec3& max) noexcept;
	inline AABB(const vec3& centerPos, float xExtent, float yExtent, float zExtent) noexcept;

	// Public member functions
	// --------------------------------------------------------------------------------------------

	inline std::array<vec3,8> corners() const noexcept;
	inline void corners(vec3* arrayOut) const noexcept;
	inline vec3 closestPoint(const vec3& point) const noexcept;

	inline size_t hash() const noexcept;

	// Public getters
	// --------------------------------------------------------------------------------------------

	inline vec3 min() const noexcept { return mMin; }
	inline vec3 max() const noexcept { return mMax; }
	inline vec3 position() const noexcept { return mMin + (extents()/2.0f); }
	inline vec3 extents() const noexcept { return mMax - mMin; }
	inline float xExtent() const noexcept { return mMax[0] - mMin[0]; }
	inline float yExtent() const noexcept { return mMax[1] - mMin[1]; }
	inline float zExtent() const noexcept { return mMax[2] - mMin[2]; }
	inline vec3 halfExtents() const noexcept { return extents() / 2.0f; }
	inline float halfXExtent() const noexcept { return xExtent() / 2.0f; }
	inline float halfYExtent() const noexcept { return yExtent() / 2.0f; }
	inline float halfZExtent() const noexcept { return zExtent() / 2.0f; }

	// Public setters
	// --------------------------------------------------------------------------------------------

	inline void min(const vec3& newMin) noexcept { mMin = newMin; }
	inline void max(const vec3& newMax) noexcept { mMax = newMax; }
	inline void position(const vec3& newCenterPos) noexcept;
	inline void extents(const vec3& newExtents) noexcept;
	inline void xExtent(float newXExtent) noexcept;
	inline void yExtent(float newYExtent) noexcept;
	inline void zExtent(float newZExtent) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	vec3 mMin, mMax;
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
