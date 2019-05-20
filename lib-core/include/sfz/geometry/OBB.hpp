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

#include "sfz/Assert.hpp"
#include "sfz/math/MathSupport.hpp"
#include "sfz/math/Matrix.hpp"
#include "sfz/math/Vector.hpp"
#include "sfz/geometry/AABB.hpp"

namespace sfz {

// OBB helper structs
// ------------------------------------------------------------------------------------------------

struct OBBCorners final {
	vec3 corners[8];
};

// OBB class
// ------------------------------------------------------------------------------------------------

// Struct representing an Oriented Bounding Box
struct OBB final {

	// Members
	// --------------------------------------------------------------------------------------------

	vec3 center;
	union {
		struct { vec3 xAxis; vec3 yAxis; vec3 zAxis; };
		vec3 axes[3];
	};
	vec3 halfExtents;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	OBB() noexcept = default;
	OBB(const OBB&) noexcept = default;
	OBB& operator= (const OBB&) noexcept = default;
	~OBB() noexcept = default;

	OBB(vec3 center, vec3 xAxis, vec3 yAxis, vec3 zAxis, vec3 extents) noexcept;
	OBB(vec3 center, const vec3 axes[3], vec3 extents) noexcept;
	OBB(vec3 center, vec3 xAxis, vec3 yAxis, vec3 zAxis,
		float xExtent, float yExtent, float zExtent) noexcept;
	explicit OBB(const AABB& aabb) noexcept;

	// Member functions
	// --------------------------------------------------------------------------------------------

	inline OBBCorners corners() const noexcept;
	inline void corners(vec3* arrayOut) const noexcept;
	inline OBB transformOBB(const mat34& transform) const noexcept;

	// Getters/setters
	// --------------------------------------------------------------------------------------------

	inline vec3 extents() const noexcept { return halfExtents * 2.0f; }
	inline float xExtent() const noexcept { return halfExtents.x * 2.0f; }
	inline float yExtent() const noexcept { return halfExtents.y * 2.0f; }
	inline float zExtent() const noexcept { return halfExtents.z * 2.0f; }

	inline void setExtents(const vec3& newExtents) noexcept;
	inline void setXExtent(float newXExtent) noexcept;
	inline void setYExtent(float newYExtent) noexcept;
	inline void setZExtent(float newZExtent) noexcept;

	// Helper methods
	// --------------------------------------------------------------------------------------------

	inline void ensureCorrectAxes() const noexcept;
	inline void ensureCorrectExtents() const noexcept;
};
static_assert(sizeof(OBB) == sizeof(vec3) * 5, "OBB is padded");

} // namespace sfz

#include "sfz/geometry/OBB.inl"
