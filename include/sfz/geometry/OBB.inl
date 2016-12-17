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

inline OBB::OBB(const vec3& center, const OBBAxes& axes, const vec3& extents) noexcept
:
	mCenter(center),
	mHalfExtents(extents/2.0f)
{
	mAxes = axes;
	ensureCorrectAxes();
	ensureCorrectExtents();
}

inline OBB::OBB(const vec3& center, const vec3& xAxis, const vec3& yAxis, const vec3& zAxis,
                const vec3& extents) noexcept
:
	mCenter(center),
	mHalfExtents(extents/2.0f)
{
	mAxes.axes[0] = xAxis;
	mAxes.axes[1] = yAxis;
	mAxes.axes[2] = zAxis;
	ensureCorrectAxes();
	ensureCorrectExtents();
}

inline OBB::OBB(const vec3& center, const vec3& xAxis, const vec3& yAxis, const vec3& zAxis,
               float xExtent, float yExtent, float zExtent) noexcept
:
	OBB(center, xAxis, yAxis, zAxis, vec3{xExtent, yExtent, zExtent})
{
	ensureCorrectAxes();
	ensureCorrectExtents();
}

inline OBB::OBB(const AABB& aabb) noexcept
:
	OBB(aabb.position(), vec3{1, 0, 0}, vec3{0, 1, 0}, vec3{0, 0, 1},
        aabb.xExtent(), aabb.yExtent(), aabb.zExtent())
{
	// Initialization done.
}

// Public member functions
// ------------------------------------------------------------------------------------------------

inline OBBCorners OBB::corners() const noexcept
{
	OBBCorners tmp;
	this->corners(tmp.corners);
	return tmp;
}

inline void OBB::corners(vec3* arrayOut) const noexcept
{
	vec3 halfXExtVec = mAxes.axes[0] * mHalfExtents[0];
	vec3 halfYExtVec = mAxes.axes[1] * mHalfExtents[1];
	vec3 halfZExtVec = mAxes.axes[2] * mHalfExtents[2];
	arrayOut[0] = mCenter - halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-left
	arrayOut[1] = mCenter - halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-left
	arrayOut[2] = mCenter - halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-left
	arrayOut[3] = mCenter - halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-left
	arrayOut[4] = mCenter + halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-right
	arrayOut[5] = mCenter + halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-right
	arrayOut[6] = mCenter + halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-right
	arrayOut[7] = mCenter + halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-right
}

inline OBB OBB::transformOBB(const mat4& transform) const noexcept
{
	const vec3 newPos = transformPoint(transform, mCenter);
	const vec3 newXHExt = transformDir(transform, mAxes.axes[0] * mHalfExtents[0]);
	const vec3 newYHExt = transformDir(transform, mAxes.axes[1] * mHalfExtents[1]);
	const vec3 newZHExt = transformDir(transform, mAxes.axes[2] * mHalfExtents[2]);
	return OBB(newPos, normalize(newXHExt), normalize(newYHExt), normalize(newZHExt),
	           length(newXHExt), length(newYHExt), length(newZHExt));
}

// Public getters/setters
// ------------------------------------------------------------------------------------------------

inline void OBB::extents(const vec3& newExtents) noexcept
{
	mHalfExtents = newExtents / 2.0f;
	ensureCorrectExtents();
}

inline void OBB::xExtent(float newXExtent) noexcept
{
	mHalfExtents[0] = newXExtent / 2.0f;
	ensureCorrectExtents();
}

inline void OBB::yExtent(float newYExtent) noexcept
{
	mHalfExtents[1] = newYExtent / 2.0f;
	ensureCorrectExtents();
}

inline void OBB::zExtent(float newZExtent) noexcept
{
	mHalfExtents[2] = newZExtent / 2.0f;
	ensureCorrectExtents();
}

inline void OBB::halfExtents(const vec3& newHalfExtents) noexcept
{
	mHalfExtents = newHalfExtents;
	ensureCorrectExtents();
}

inline void OBB::halfXExtent(float newHalfXExtent) noexcept
{
	mHalfExtents[0] = newHalfXExtent;
	ensureCorrectExtents();
}

inline void OBB::halfYExtent(float newHalfYExtent) noexcept
{
	mHalfExtents[1] = newHalfYExtent;
	ensureCorrectExtents();
}

inline void OBB::halfZExtent(float newHalfZExtent) noexcept
{
	mHalfExtents[2] = newHalfZExtent;
	ensureCorrectExtents();
}

// Private functions
// ------------------------------------------------------------------------------------------------

inline void OBB::ensureCorrectAxes() const noexcept
{
	// Check if axes are orthogonal
	sfz_assert_debug(approxEqual(dot(mAxes.axes[0], mAxes.axes[1]), 0.0f));
	sfz_assert_debug(approxEqual(dot(mAxes.axes[0], mAxes.axes[2]), 0.0f));
	sfz_assert_debug(approxEqual(dot(mAxes.axes[1], mAxes.axes[2]), 0.0f));

	// Check if axes are normalized
	sfz_assert_debug(approxEqual(length(mAxes.axes[0]), 1.0f));
	sfz_assert_debug(approxEqual(length(mAxes.axes[1]), 1.0f));
	sfz_assert_debug(approxEqual(length(mAxes.axes[2]), 1.0f));
}

inline void OBB::ensureCorrectExtents() const noexcept
{
	// Extents are non-negative
	sfz_assert_debug(0.0f < mHalfExtents[0]);
	sfz_assert_debug(0.0f < mHalfExtents[1]);
	sfz_assert_debug(0.0f < mHalfExtents[2]);
}

} // namespace sfz
