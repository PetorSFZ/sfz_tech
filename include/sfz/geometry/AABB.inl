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

inline AABB::AABB(const vec3& minIn, const vec3& maxIn) noexcept
:
	min(minIn),
	max(maxIn)
{
}

inline AABB::AABB(const vec3& centerPos, float xExtent, float yExtent, float zExtent) noexcept
{
	vec3 temp = centerPos;
	temp[0] -= xExtent/2.0f;
	temp[1] -= yExtent/2.0f;
	temp[2] -= zExtent/2.0f;
	this->min = temp;

	temp[0] += xExtent;
	temp[1] += yExtent;
	temp[2] += zExtent;
	this->max = temp;
}

// Public member functions
// ------------------------------------------------------------------------------------------------

inline AABBCorners AABB::corners() const noexcept
{
	AABBCorners tmp;
	this->corners(&tmp.corners[0]);
	return tmp;
}

inline void AABB::corners(vec3* arrayOut) const noexcept
{
	const vec3 xExtent = vec3{this->max.x - this->min.x, 0.0f, 0.0f};
	const vec3 yExtent = vec3{0.0f, this->max.y - this->min.y, 0.0f};
	const vec3 zExtent = vec3{0.0f, 0.0f, this->max.z - this->min.z};

	arrayOut[0] = this->min; // Back-bottom-left
	arrayOut[1] = this->min + zExtent; // Front-bottom-left
	arrayOut[2] = this->min + yExtent; // Back-top-left
	arrayOut[3] = this->min + zExtent + yExtent; // Front-top-left
	arrayOut[4] = this->min + xExtent; // Back-bottom-right
	arrayOut[5] = this->min + zExtent + xExtent; // Front-bottom-right
	arrayOut[6] = this->min + yExtent + xExtent; // Back-top-right
	arrayOut[7] = this->max; // Front-top-right
}

// Public getters/setters
// ------------------------------------------------------------------------------------------------

inline void AABB::position(const vec3& newCenterPos) noexcept
{
	const vec3 halfExt = this->halfExtents();
	this->min = newCenterPos - halfExt;
	this->max = newCenterPos + halfExt;
}

inline void AABB::extents(const vec3& newExtents) noexcept
{
	const vec3 pos = position();
	const vec3 halfExt = newExtents * 0.5f;
	this->min = pos - halfExt;
	this->max = pos + halfExt;
}

inline void AABB::xExtent(float newXExtent) noexcept
{
	extents(vec3(newXExtent, yExtent(), zExtent()));
}

inline void AABB::yExtent(float newYExtent) noexcept
{
	extents(vec3(xExtent(), newYExtent, zExtent()));
}

inline void AABB::zExtent(float newZExtent) noexcept
{
	extents(vec3(xExtent(), yExtent(), newZExtent));
}

} // namespace sfz
