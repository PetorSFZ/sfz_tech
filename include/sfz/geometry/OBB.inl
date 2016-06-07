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

inline OBB::OBB(const vec3& center, const array<vec3,3>& axes, const vec3& extents) noexcept
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
	mAxes[0] = xAxis;
	mAxes[1] = yAxis;
	mAxes[2] = zAxis;
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

inline array<vec3,8> OBB::corners() const noexcept
{
	std::array<vec3,8> result;
	this->corners(&result[0]);
	return result;
}

inline void OBB::corners(vec3* arrayOut) const noexcept
{
	vec3 halfXExtVec = mAxes[0]*mHalfExtents[0];
	vec3 halfYExtVec = mAxes[1]*mHalfExtents[1];
	vec3 halfZExtVec = mAxes[2]*mHalfExtents[2];
	arrayOut[0] = mCenter - halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-left
	arrayOut[1] = mCenter - halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-left
	arrayOut[2] = mCenter - halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-left
	arrayOut[3] = mCenter - halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-left
	arrayOut[4] = mCenter + halfXExtVec - halfYExtVec - halfZExtVec; // Back-bottom-right
	arrayOut[5] = mCenter + halfXExtVec - halfYExtVec + halfZExtVec; // Front-bottom-right
	arrayOut[6] = mCenter + halfXExtVec + halfYExtVec - halfZExtVec; // Back-top-right
	arrayOut[7] = mCenter + halfXExtVec + halfYExtVec + halfZExtVec; // Front-top-right
}

inline vec3 OBB::closestPoint(const vec3& point) const noexcept
{
	// Algorithm from Real-Time Collision Detection (Section 5.1.4)
	const vec3 distToPoint = point - mCenter;
	vec3 res = mCenter;

	float dist;
	for (size_t i = 0; i < 3; i++) {
		dist = dot(distToPoint, mAxes[i]);
		if (dist > mHalfExtents[i]) dist = mHalfExtents[i];
		if (dist < -mHalfExtents[i]) dist = -mHalfExtents[i];
		res += (dist * mAxes[i]);
	}

	return res;
}

inline OBB OBB::transformOBB(const mat4& transform) const noexcept
{
	const vec3 newPos = transformPoint(transform, mCenter);
	const vec3 newXHExt = transformDir(transform, mAxes[0] * mHalfExtents[0]);
	const vec3 newYHExt = transformDir(transform, mAxes[1] * mHalfExtents[1]);
	const vec3 newZHExt = transformDir(transform, mAxes[2] * mHalfExtents[2]);
	return OBB{newPos, normalize(newXHExt), normalize(newYHExt), normalize(newZHExt),
	           length(newXHExt), length(newYHExt), length(newZHExt)};
}

inline size_t OBB::hash() const noexcept
{
	std::hash<vec3> hasher;
	size_t hash = 0;
	// hash_combine algorithm from boost
	hash ^= hasher(mCenter) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= hasher(mAxes[0]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= hasher(mAxes[1]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= hasher(mAxes[2]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= hasher(mHalfExtents) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash;
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
	sfz_assert_debug(approxEqual(dot(mAxes[0], mAxes[1]), 0.0f));
	sfz_assert_debug(approxEqual(dot(mAxes[0], mAxes[2]), 0.0f));
	sfz_assert_debug(approxEqual(dot(mAxes[1], mAxes[2]), 0.0f));

	// Check if axes are normalized
	sfz_assert_debug(approxEqual(length(mAxes[0]), 1.0f));
	sfz_assert_debug(approxEqual(length(mAxes[1]), 1.0f));
	sfz_assert_debug(approxEqual(length(mAxes[2]), 1.0f));
}

inline void OBB::ensureCorrectExtents() const noexcept
{
	// Extents are non-negative
	sfz_assert_debug(0.0f < mHalfExtents[0]);
	sfz_assert_debug(0.0f < mHalfExtents[1]);
	sfz_assert_debug(0.0f < mHalfExtents[2]);
}

} // namespace sfz

// Specializations of standard library for sfz::OBB
// ------------------------------------------------------------------------------------------------

namespace std {

inline size_t hash<sfz::OBB>::operator() (const sfz::OBB& obb) const noexcept
{
	return obb.hash();
}

} // namespace std
