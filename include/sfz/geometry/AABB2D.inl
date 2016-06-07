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

// AABB2D: Constructors and destructors
// ------------------------------------------------------------------------------------------------

inline AABB2D::AABB2D(vec2 centerPos, vec2 dimensions) noexcept
:
	min{centerPos - (dimensions / 2.0f)},
	max{centerPos + (dimensions / 2.0f)}
{ }

inline AABB2D::AABB2D(float centerX, float centerY, float width, float height) noexcept
:
	min{vec2{centerX - (width / 2.0f), centerY - (height / 2.0f)}},
	max{vec2{centerX + (width / 2.0f), centerY + (height / 2.0f)}}
{ }

// AABB2D: Public methods
// ------------------------------------------------------------------------------------------------

inline size_t AABB2D::hash() const noexcept
{
	std::hash<vec2> hasher;
	size_t hash = 0;
	// hash_combine algorithm from boost
	hash ^= hasher(min) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= hasher(max) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash;
}

// AABB2D: Comparison operators
// ------------------------------------------------------------------------------------------------

inline bool AABB2D::operator== (const AABB2D& other) const noexcept
{
	return min == other.min && max == other.max;
}

inline bool AABB2D::operator!= (const AABB2D& other) const noexcept
{
	return !((*this) == other);
}

} // namespace sfz

// Specializations of standard library for sfz::AABB2D
// ------------------------------------------------------------------------------------------------

namespace std {

inline size_t hash<sfz::AABB2D>::operator() (const sfz::AABB2D& aabb) const noexcept
{
	return aabb.hash();
}

} // namespace std
