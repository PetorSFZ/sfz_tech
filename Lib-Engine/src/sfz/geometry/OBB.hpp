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
#include <skipifzero_geometry.hpp>
#include <skipifzero_math.hpp>

namespace sfz {

// OBB helper structs
// ------------------------------------------------------------------------------------------------

struct OBBCorners final {
	f32x3 corners[8];
};

// OBB class
// ------------------------------------------------------------------------------------------------

// Struct representing an Oriented Bounding Box
struct OBB final {

	// Members
	// --------------------------------------------------------------------------------------------

	mat33 rotation;
	f32x3 center;
	f32x3 halfExtents;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	OBB() noexcept = default;
	OBB(const OBB&) noexcept = default;
	OBB& operator= (const OBB&) noexcept = default;
	~OBB() noexcept = default;

	OBB(f32x3 center, f32x3 xAxis, f32x3 yAxis, f32x3 zAxis, f32x3 extents) noexcept;
	OBB(f32x3 center, const f32x3 axes[3], f32x3 extents) noexcept;
	OBB(f32x3 center, f32x3 xAxis, f32x3 yAxis, f32x3 zAxis,
		f32 xExtent, f32 yExtent, f32 zExtent) noexcept;
	explicit OBB(const AABB& aabb) noexcept;

	// Member functions
	// --------------------------------------------------------------------------------------------

	OBBCorners corners() const noexcept;
	void corners(f32x3* arrayOut) const noexcept;
	OBB transformOBB(const mat34& transform) const noexcept;
	OBB transformOBB(Quat quaternion) const noexcept;

	// Getters/setters
	// --------------------------------------------------------------------------------------------

	f32x3 extents() const noexcept { return halfExtents * 2.0f; }
	f32 xExtent() const noexcept { return halfExtents.x * 2.0f; }
	f32 yExtent() const noexcept { return halfExtents.y * 2.0f; }
	f32 zExtent() const noexcept { return halfExtents.z * 2.0f; }

	void setExtents(const f32x3& newExtents) noexcept;
	void setXExtent(f32 newXExtent) noexcept;
	void setYExtent(f32 newYExtent) noexcept;
	void setZExtent(f32 newZExtent) noexcept;

	f32x3 axis(u32 idx) const noexcept { return rotation.row(idx); }
	f32x3 xAxis() const noexcept { return rotation.row(0); }
	f32x3 yAxis() const noexcept { return rotation.row(1); }
	f32x3 zAxis() const noexcept { return rotation.row(2); }

	// Helper methods
	// --------------------------------------------------------------------------------------------

	void ensureCorrectAxes() const noexcept;
	void ensureCorrectExtents() const noexcept;
};
static_assert(sizeof(OBB) == sizeof(f32x3) * 5, "OBB is padded");

} // namespace sfz

#include "sfz/geometry/OBB.inl"
