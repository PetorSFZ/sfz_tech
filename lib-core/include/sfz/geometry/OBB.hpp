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

struct OBBAxes final {
	vec3 axes[3];

	vec3& operator[] (size_t i) noexcept { return axes[i]; }
	const vec3& operator[] (size_t i) const noexcept { return axes[i]; }
};

struct OBBCorners final {
	vec3 corners[8];
};

// OBB class
// ------------------------------------------------------------------------------------------------

/// Class representing an Oriented Bounding Box
class OBB final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	OBB() noexcept = default;
	OBB(const OBB&) noexcept = default;
	OBB& operator= (const OBB&) noexcept = default;

	inline OBB(const vec3& center, const OBBAxes& axes, const vec3& extents) noexcept;

	inline OBB(const vec3& center, const vec3& xAxis, const vec3& yAxis, const vec3& zAxis,
	           const vec3& extents) noexcept;

	inline OBB(const vec3& center, const vec3& xAxis, const vec3& yAxis, const vec3& zAxis,
	           float xExtent, float yExtent, float zExtent) noexcept;

	explicit inline OBB(const AABB& aabb) noexcept;

	// Public member functions
	// --------------------------------------------------------------------------------------------

	inline OBBCorners corners() const noexcept;
	inline void corners(vec3* arrayOut) const noexcept;
	inline OBB transformOBB(const mat4& transform) const noexcept;

	// Public getters/setters
	// --------------------------------------------------------------------------------------------

	inline vec3 position() const noexcept { return mCenter; }
	inline const OBBAxes& axes() const noexcept { return mAxes; }
	inline vec3 xAxis() const noexcept { return mAxes.axes[0]; }
	inline vec3 yAxis() const noexcept { return mAxes.axes[1]; }
	inline vec3 zAxis() const noexcept { return mAxes.axes[2]; }
	inline vec3 extents() const noexcept { return mHalfExtents * 2.0f; }
	inline float xExtent() const noexcept { return mHalfExtents[0] * 2.0f; }
	inline float yExtent() const noexcept { return mHalfExtents[1] * 2.0f; }
	inline float zExtent() const noexcept { return mHalfExtents[2] * 2.0f; }
	inline vec3 halfExtents() const noexcept { return mHalfExtents; }
	inline float halfXExtent() const noexcept { return mHalfExtents[0]; }
	inline float halfYExtent() const noexcept { return mHalfExtents[1]; }
	inline float halfZExtent() const noexcept { return mHalfExtents[2]; }

	inline void position(const vec3& newCenterPos) noexcept { mCenter = newCenterPos; }
	inline void axes(const OBBAxes& newAxes) noexcept { mAxes = newAxes; }
	inline void xAxis(const vec3& newXAxis) noexcept { mAxes.axes[0] = newXAxis; }
	inline void yAxis(const vec3& newYAxis) noexcept { mAxes.axes[1] = newYAxis; }
	inline void zAxis(const vec3& newZAxis) noexcept { mAxes.axes[2] = newZAxis; }
	inline void extents(const vec3& newExtents) noexcept;
	inline void xExtent(float newXExtent) noexcept;
	inline void yExtent(float newYExtent) noexcept;
	inline void zExtent(float newZExtent) noexcept;
	inline void halfExtents(const vec3& newHalfExtents) noexcept;
	inline void halfXExtent(float newHalfXExtent) noexcept;
	inline void halfYExtent(float newHalfYExtent) noexcept;
	inline void halfZExtent(float newHalfZExtent) noexcept;

private:
	// Private functions
	// --------------------------------------------------------------------------------------------

	inline void ensureCorrectAxes() const noexcept;
	inline void ensureCorrectExtents() const noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	vec3 mCenter;
	OBBAxes mAxes;
	vec3 mHalfExtents;
};

} // namespace sfz

#include "sfz/geometry/OBB.inl"
