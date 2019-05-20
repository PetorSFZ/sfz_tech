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

inline OBB::OBB(const vec3& center, const vec3 axes[3], const vec3& extents) noexcept
{
	this->center = center;
	this->xAxis = axes[0];
	this->yAxis = axes[1];
	this->zAxis = axes[2];
	this->halfExtents = extents * 0.5f;
	ensureCorrectAxes();
	ensureCorrectExtents();
}

inline OBB::OBB(const vec3& center, const vec3& xAxis, const vec3& yAxis, const vec3& zAxis,
                const vec3& extents) noexcept
{
	this->center = center;
	this->axes[0] = xAxis;
	this->axes[1] = yAxis;
	this->axes[2] = zAxis;
	this->halfExtents = extents * 0.5f;
	ensureCorrectAxes();
	ensureCorrectExtents();
}

inline OBB::OBB(const vec3& center, const vec3& xAxis, const vec3& yAxis, const vec3& zAxis,
               float xExtent, float yExtent, float zExtent) noexcept
:
	OBB(center, xAxis, yAxis, zAxis, vec3(xExtent, yExtent, zExtent))
{
	// Done
}

inline OBB::OBB(const AABB& aabb) noexcept
:
	OBB(aabb.position(), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f),
        aabb.xExtent(), aabb.yExtent(), aabb.zExtent())
{
	// Done
}

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
	vec3 halfXExtVec = xAxis * halfExtents[0];
	vec3 halfYExtVec = yAxis * halfExtents[1];
	vec3 halfZExtVec = zAxis * halfExtents[2];
	arrayOut[0] = center - halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-left
	arrayOut[1] = center - halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-left
	arrayOut[2] = center - halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-left
	arrayOut[3] = center - halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-left
	arrayOut[4] = center + halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-right
	arrayOut[5] = center + halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-right
	arrayOut[6] = center + halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-right
	arrayOut[7] = center + halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-right
}

inline OBB OBB::transformOBB(const mat4& transform) const noexcept
{
	const vec3 newPos = transformPoint(transform, center);
	const vec3 newXHExt = transformDir(transform, xAxis * halfExtents[0]);
	const vec3 newYHExt = transformDir(transform, yAxis * halfExtents[1]);
	const vec3 newZHExt = transformDir(transform, zAxis * halfExtents[2]);
	return OBB(newPos, normalize(newXHExt), normalize(newYHExt), normalize(newZHExt),
	           length(newXHExt), length(newYHExt), length(newZHExt));
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
	sfz_assert_debug(approxEqual(dot(xAxis, yAxis), 0.0f));
	sfz_assert_debug(approxEqual(dot(xAxis, zAxis), 0.0f));
	sfz_assert_debug(approxEqual(dot(yAxis, zAxis), 0.0f));

	// Check if axes are normalized
	sfz_assert_debug(approxEqual(length(xAxis), 1.0f));
	sfz_assert_debug(approxEqual(length(yAxis), 1.0f));
	sfz_assert_debug(approxEqual(length(zAxis), 1.0f));
}

inline void OBB::ensureCorrectExtents() const noexcept
{
	// Extents are non-negative
	sfz_assert_debug(0.0f < halfExtents[0]);
	sfz_assert_debug(0.0f < halfExtents[1]);
	sfz_assert_debug(0.0f < halfExtents[2]);
}

} // namespace sfz
