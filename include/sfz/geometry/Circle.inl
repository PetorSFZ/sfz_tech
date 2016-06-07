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

// Circle: Constructors and destructors
// ------------------------------------------------------------------------------------------------

inline Circle::Circle(vec2 centerPos, float radius) noexcept
:
	pos{centerPos},
	radius{radius}
{ }

inline Circle::Circle(float centerX, float centerY, float radius) noexcept
:
	pos{centerX, centerY},
	radius{radius}
{ }

// Circle: Public methods
// ------------------------------------------------------------------------------------------------

inline size_t Circle::hash() const noexcept
{
	std::hash<float> hasher;
	size_t hash = 0;
	// hash_combine algorithm from boost
	hash ^= hasher(pos[0]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= hasher(pos[1]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= hasher(radius) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash;
}

// Circle: Comparison operators
// ------------------------------------------------------------------------------------------------

inline bool Circle::operator== (const Circle& other) const noexcept
{
	return pos == other.pos && radius == other.radius;
}

inline bool Circle::operator!= (const Circle& other) const noexcept
{
	return !((*this) == other);
}

} // namespace sfz

// Specializations of standard library for sfz::Circle
// ------------------------------------------------------------------------------------------------

namespace std {

inline size_t hash<sfz::Circle>::operator() (const sfz::Circle& circle) const noexcept
{
	return circle.hash();
}

} // namespace std