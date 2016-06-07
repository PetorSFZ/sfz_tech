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

inline AABB::AABB(const vec3& min, const vec3& max) noexcept
:
	mMin(min),
	mMax(max)
{
	sfz_assert_debug(min[0] < max[0]);
	sfz_assert_debug(min[1] < max[1]);
	sfz_assert_debug(min[2] < max[2]);
}

inline AABB::AABB(const vec3& centerPos, float xExtent, float yExtent, float zExtent) noexcept
{
	sfz_assert_debug(xExtent > 0);
	sfz_assert_debug(yExtent > 0);
	sfz_assert_debug(zExtent > 0);

	vec3 temp = centerPos;
	temp[0] -= xExtent/2.0f;
	temp[1] -= yExtent/2.0f;
	temp[2] -= zExtent/2.0f;
	mMin = temp;

	temp[0] += xExtent;
	temp[1] += yExtent;
	temp[2] += zExtent;
	mMax = temp;
}

// Public member functions
// ------------------------------------------------------------------------------------------------

inline std::array<vec3,8> AABB::corners() const noexcept
{
	std::array<vec3,8> result;
	this->corners(&result[0]);
	return result;
}

inline void AABB::corners(vec3* arrayOut) const noexcept
{
	const vec3 xExtent = vec3{mMax[0] - mMin[0], 0.0f, 0.0f};
	const vec3 yExtent = vec3{0.0f, mMax[1] - mMin[1], 0.0f};
	const vec3 zExtent = vec3{0.0f, 0.0f, mMax[2] - mMin[2]};

	arrayOut[0] = mMin; // Back-bottom-left
	arrayOut[1] = mMin + zExtent; // Front-bottom-left
	arrayOut[2] = mMin + yExtent; // Back-top-left
	arrayOut[3] = mMin + zExtent + yExtent; // Front-top-left
	arrayOut[4] = mMin + xExtent; // Back-bottom-right
	arrayOut[5] = mMin + zExtent + xExtent; // Front-bottom-right
	arrayOut[6] = mMin + yExtent + xExtent; // Back-top-right
	arrayOut[7] = mMax; // Front-top-right
}

inline vec3 AABB::closestPoint(const vec3& point) const noexcept
{
	vec3 res = point;
	float val;
	for (size_t i = 0; i < 3; i++) {
		val = point[i];
		if (val < mMin[i]) val = mMin[i];
		if (val > mMax[i]) val = mMax[i];
		res[i] = val;
	}
	return res;
}

inline size_t AABB::hash() const noexcept
{
	std::hash<vec3> hasher;
	size_t hash = 0;
	// hash_combine algorithm from boost
	hash ^= hasher(mMin) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= hasher(mMax) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash;
}

// Public getters/setters
// ------------------------------------------------------------------------------------------------

inline void AABB::position(const vec3& newCenterPos) noexcept
{
	const vec3 halfExtents{xExtent()/2.0f, yExtent()/2.0f, zExtent()/2.0f};
	mMin = newCenterPos - halfExtents;
	mMax = newCenterPos + halfExtents;
}

inline void AABB::extents(const vec3& newExtents) noexcept
{
	sfz_assert_debug(newExtents[0] > 0);
	sfz_assert_debug(newExtents[1] > 0);
	sfz_assert_debug(newExtents[2] > 0);
	const vec3 pos = position();
	const vec3 halfExtents = newExtents/2.0f;
	mMin = pos - halfExtents;
	mMin = pos + halfExtents;
}

inline void AABB::xExtent(float newXExtent) noexcept
{
	extents(vec3{newXExtent, yExtent(), zExtent()});
}

inline void AABB::yExtent(float newYExtent) noexcept
{
	extents(vec3{xExtent(), newYExtent, zExtent()});
}

inline void AABB::zExtent(float newZExtent) noexcept
{
	extents(vec3{xExtent(), yExtent(), newZExtent});
}

} // namespace sfz

// Specializations of standard library for sfz::AABB
// ------------------------------------------------------------------------------------------------

namespace std {

inline size_t hash<sfz::AABB>::operator() (const sfz::AABB& aabb) const noexcept
{
	return aabb.hash();
}

} // namespace std
