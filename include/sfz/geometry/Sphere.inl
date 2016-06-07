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

inline Sphere::Sphere(const vec3& center, float radius) noexcept
:
	mCenter(center),
	mRadius{radius}
{
	sfz_assert_debug(radius > 0.0f);
}

// Public member functions
// ------------------------------------------------------------------------------------------------

inline vec3 Sphere::closestPoint(const vec3& point) const noexcept
{
	const vec3 distToPoint = point - mCenter;
	vec3 res = point;
	if (squaredLength(distToPoint) > mRadius*mRadius)
	{
		res = mCenter + normalize(distToPoint)*mRadius;
	}
	return res;
}

inline size_t Sphere::hash() const noexcept
{
	std::hash<vec3> vecHasher;
	std::hash<float> floatHasher;
	size_t hash = 0;
	// hash_combine algorithm from boost
	hash ^= vecHasher(mCenter) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= floatHasher(mRadius) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash;
}

// Public getters/setters
// ------------------------------------------------------------------------------------------------

inline void Sphere::radius(float newRadius) noexcept
{
	sfz_assert_debug(newRadius > 0.0f);
	mRadius = newRadius;
}

} // namespace sfz

// Specializations of standard library for sfz::Sphere
// ------------------------------------------------------------------------------------------------

namespace std {

inline size_t hash<sfz::Sphere>::operator() (const sfz::Sphere& sphere) const noexcept
{
	return sphere.hash();
}

} // namespace std
