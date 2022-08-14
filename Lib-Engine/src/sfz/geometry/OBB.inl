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

namespace sfz {

// Constructors & destructors
// ------------------------------------------------------------------------------------------------

inline OBB::OBB(f32x3 center, f32x3 xAxis, f32x3 yAxis, f32x3 zAxis, f32x3 extents) noexcept
{
	this->center = center;
	rotation.rows[0] = xAxis;
	rotation.rows[1] = yAxis;
	rotation.rows[2] = zAxis;
	this->halfExtents = extents * 0.5f;
	ensureCorrectAxes();
	ensureCorrectExtents();
}

inline OBB::OBB(f32x3 center, const f32x3 axes[3], f32x3 extents) noexcept
:
	OBB(center, axes[0], axes[1], axes[2], extents)
{ }

inline OBB::OBB(f32x3 center, f32x3 xAxis, f32x3 yAxis, f32x3 zAxis,
	f32 xExtent, f32 yExtent, f32 zExtent) noexcept
:
	OBB(center, xAxis, yAxis, zAxis, f32x3_init(xExtent, yExtent, zExtent))
{ }

inline OBB::OBB(const AABB& aabb) noexcept
:
	OBB(aabb.pos(),
		f32x3_init(1.0f, 0.0f, 0.0f), f32x3_init(0.0f, 1.0f, 0.0f), f32x3_init(0.0f, 0.0f, 1.0f),
		aabb.dims())
{ }

// Member functions
// ------------------------------------------------------------------------------------------------

inline OBBCorners OBB::corners() const noexcept
{
	OBBCorners tmp;
	this->corners(tmp.corners);
	return tmp;
}

inline void OBB::corners(f32x3* arrayOut) const noexcept
{
	f32x3 halfXExtVec = xAxis() * halfExtents[0];
	f32x3 halfYExtVec = yAxis() * halfExtents[1];
	f32x3 halfZExtVec = zAxis() * halfExtents[2];
	arrayOut[0] = center - halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-left
	arrayOut[1] = center - halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-left
	arrayOut[2] = center - halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-left
	arrayOut[3] = center - halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-left
	arrayOut[4] = center + halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-right
	arrayOut[5] = center + halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-right
	arrayOut[6] = center + halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-right
	arrayOut[7] = center + halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-right
}

inline OBB OBB::transformOBB(const SfzMat44& transform) const noexcept
{
	const f32x3 newPos = sfzMat44TransformPoint(transform, center);

	const f32x3 xAxisHalfExt = xAxis() * halfExtents.x;
	const f32x3 yAxisHalfExt = yAxis() * halfExtents.y;
	const f32x3 zAxisHalfExt = zAxis() * halfExtents.z;
	const f32x3 newXAxisHalfExt = sfzMat44TransformDir(transform, xAxisHalfExt);
	const f32x3 newYAxisHalfExt = sfzMat44TransformDir(transform, yAxisHalfExt);
	const f32x3 newZAxisHalfExt = sfzMat44TransformDir(transform, zAxisHalfExt);

	const f32x3 newHalfExt =
		f32x3_init(f32x3_length(newXAxisHalfExt), f32x3_length(newYAxisHalfExt), f32x3_length(newZAxisHalfExt));
	f32x3 newAxes[3];
	newAxes[0] = newXAxisHalfExt / newHalfExt.x;
	newAxes[1] = newYAxisHalfExt / newHalfExt.y;
	newAxes[2] = newZAxisHalfExt / newHalfExt.z;

	return OBB(newPos, newAxes, newHalfExt * 2.0f);
}

inline OBB OBB::transformOBB(SfzQuat quaternion) const noexcept
{
	sfz_assert(eqf(sfzQuatLength(quaternion), 1.0f));
	OBB tmp = *this;
	tmp.rotation.rows[0] = sfzQuatRotateUnit(quaternion, tmp.rotation.rows[0]);
	tmp.rotation.rows[1] = sfzQuatRotateUnit(quaternion, tmp.rotation.rows[1]);
	tmp.rotation.rows[2] = sfzQuatRotateUnit(quaternion, tmp.rotation.rows[2]);
	return tmp;
}

// Getters/setters
// ------------------------------------------------------------------------------------------------

inline void OBB::setExtents(const f32x3& newExtents) noexcept
{
	halfExtents = newExtents * 0.5f;
	ensureCorrectExtents();
}

inline void OBB::setXExtent(f32 newXExtent) noexcept
{
	halfExtents[0] = newXExtent * 0.5f;
	ensureCorrectExtents();
}

inline void OBB::setYExtent(f32 newYExtent) noexcept
{
	halfExtents[1] = newYExtent * 0.5f;
	ensureCorrectExtents();
}

inline void OBB::setZExtent(f32 newZExtent) noexcept
{
	halfExtents[2] = newZExtent * 0.5f;
	ensureCorrectExtents();
}

// Helper methods
// ------------------------------------------------------------------------------------------------

inline void OBB::ensureCorrectAxes() const noexcept
{
	// Check if axes are orthogonal
	sfz_assert(eqf(f32x3_dot(f32x3(rotation.rows[0]), f32x3(rotation.rows[1])), 0.0f));
	sfz_assert(eqf(f32x3_dot(f32x3(rotation.rows[0]), f32x3(rotation.rows[2])), 0.0f));
	sfz_assert(eqf(f32x3_dot(f32x3(rotation.rows[1]), f32x3(rotation.rows[2])), 0.0f));

	// Check if axes are normalized
	sfz_assert(eqf(f32x3_length(rotation.rows[0]), 1.0f));
	sfz_assert(eqf(f32x3_length(rotation.rows[1]), 1.0f));
	sfz_assert(eqf(f32x3_length(rotation.rows[2]), 1.0f));
}

inline void OBB::ensureCorrectExtents() const noexcept
{
	// Extents are non-negative
	sfz_assert(0.0f < halfExtents[0]);
	sfz_assert(0.0f < halfExtents[1]);
	sfz_assert(0.0f < halfExtents[2]);
}

} // namespace sfz
