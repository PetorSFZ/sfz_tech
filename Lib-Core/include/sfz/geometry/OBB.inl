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

inline OBB::OBB(vec3 center, vec3 xAxis, vec3 yAxis, vec3 zAxis, vec3 extents) noexcept
{
	this->center = center;
	this->xAxis() = xAxis;
	this->yAxis() = yAxis;
	this->zAxis() = zAxis;
	this->halfExtents = extents * 0.5f;
	ensureCorrectAxes();
	ensureCorrectExtents();
}

inline OBB::OBB(vec3 center, const vec3 axes[3], vec3 extents) noexcept
:
	OBB(center, axes[0], axes[1], axes[2], extents)
{ }

inline OBB::OBB(vec3 center, vec3 xAxis, vec3 yAxis, vec3 zAxis,
	float xExtent, float yExtent, float zExtent) noexcept
:
	OBB(center, xAxis, yAxis, zAxis, vec3(xExtent, yExtent, zExtent))
{ }

inline OBB::OBB(const AABB& aabb) noexcept
:
	OBB(aabb.position(),
		vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f),
		aabb.extents())
{ }

// Member functions
// ------------------------------------------------------------------------------------------------

inline OBBCorners OBB::corners() const noexcept
{
	OBBCorners tmp;
	this->corners(tmp.corners);
	return tmp;
}

inline void OBB::corners(vec3* arrayOut) const noexcept
{
	vec3 halfXExtVec = xAxis() * halfExtents[0];
	vec3 halfYExtVec = yAxis() * halfExtents[1];
	vec3 halfZExtVec = zAxis() * halfExtents[2];
	arrayOut[0] = center - halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-left
	arrayOut[1] = center - halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-left
	arrayOut[2] = center - halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-left
	arrayOut[3] = center - halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-left
	arrayOut[4] = center + halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-right
	arrayOut[5] = center + halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-right
	arrayOut[6] = center + halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-right
	arrayOut[7] = center + halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-right
}

inline OBB OBB::transformOBB(const mat34& transform) const noexcept
{
	const vec3 newPos = transformPoint(transform, center);

	const vec3 xAxisHalfExt = xAxis() * halfExtents.x;
	const vec3 yAxisHalfExt = yAxis() * halfExtents.y;
	const vec3 zAxisHalfExt = zAxis() * halfExtents.z;
	const vec3 newXAxisHalfExt = transformDir(transform, xAxisHalfExt);
	const vec3 newYAxisHalfExt = transformDir(transform, yAxisHalfExt);
	const vec3 newZAxisHalfExt = transformDir(transform, zAxisHalfExt);

	const vec3 newHalfExt =
		vec3(length(newXAxisHalfExt), length(newYAxisHalfExt), length(newZAxisHalfExt));
	vec3 newAxes[3];
	newAxes[0] = newXAxisHalfExt / newHalfExt.x;
	newAxes[1] = newYAxisHalfExt / newHalfExt.y;
	newAxes[2] = newZAxisHalfExt / newHalfExt.z;

	return OBB(newPos, newAxes, newHalfExt * 2.0f);
}

inline OBB OBB::transformOBB(Quaternion quaternion) const noexcept
{
	sfz_assert(eqf(length(quaternion), 1.0f));
	OBB tmp = *this;
	tmp.rotation.row0 = rotate(quaternion, tmp.rotation.row0);
	tmp.rotation.row1 = rotate(quaternion, tmp.rotation.row1);
	tmp.rotation.row2 = rotate(quaternion, tmp.rotation.row2);
	return tmp;
}

// Getters/setters
// ------------------------------------------------------------------------------------------------

inline void OBB::setExtents(const vec3& newExtents) noexcept
{
	halfExtents = newExtents * 0.5f;
	ensureCorrectExtents();
}

inline void OBB::setXExtent(float newXExtent) noexcept
{
	halfExtents[0] = newXExtent * 0.5f;
	ensureCorrectExtents();
}

inline void OBB::setYExtent(float newYExtent) noexcept
{
	halfExtents[1] = newYExtent * 0.5f;
	ensureCorrectExtents();
}

inline void OBB::setZExtent(float newZExtent) noexcept
{
	halfExtents[2] = newZExtent * 0.5f;
	ensureCorrectExtents();
}

// Helper methods
// ------------------------------------------------------------------------------------------------

inline void OBB::ensureCorrectAxes() const noexcept
{
	// Check if axes are orthogonal
	sfz_assert(eqf(dot(rotation.row0, rotation.row1), 0.0f));
	sfz_assert(eqf(dot(rotation.row0, rotation.row2), 0.0f));
	sfz_assert(eqf(dot(rotation.row1, rotation.row2), 0.0f));

	// Check if axes are normalized
	sfz_assert(eqf(length(rotation.row0), 1.0f));
	sfz_assert(eqf(length(rotation.row1), 1.0f));
	sfz_assert(eqf(length(rotation.row2), 1.0f));
}

inline void OBB::ensureCorrectExtents() const noexcept
{
	// Extents are non-negative
	sfz_assert(0.0f < halfExtents[0]);
	sfz_assert(0.0f < halfExtents[1]);
	sfz_assert(0.0f < halfExtents[2]);
}

} // namespace sfz
