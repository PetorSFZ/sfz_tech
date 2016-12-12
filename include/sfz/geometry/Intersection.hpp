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

#pragma once

#include "sfz/geometry/AABB.hpp"
#include "sfz/geometry/AABB2D.hpp"
#include "sfz/geometry/Circle.hpp"
#include "sfz/geometry/OBB.hpp"
#include "sfz/geometry/Plane.hpp"
#include "sfz/geometry/Sphere.hpp"

namespace sfz {

// Point inside primitive tests
// ------------------------------------------------------------------------------------------------

inline bool pointInside(const AABB& box, const vec3& point) noexcept;
inline bool pointInside(const OBB& box, const vec3& point) noexcept;
inline bool pointInside(const Sphere& sphere, const vec3& point) noexcept;

inline bool pointInside(const Circle& circle, vec2 point) noexcept;
inline bool pointInside(const AABB2D& rect, vec2 point) noexcept;

// Primitive vs primitive tests (same type)
// ------------------------------------------------------------------------------------------------

inline bool intersects(const AABB& boxA, const AABB& boxB) noexcept;
inline bool intersects(const OBB& boxA, const OBB& boxB) noexcept;
inline bool intersects(const Sphere& sphereA, const Sphere& sphereB) noexcept;

inline bool overlaps(const Circle& lhs, const Circle& rhs) noexcept;
inline bool overlaps(const AABB2D& lhs, const AABB2D& rhs) noexcept;

// AABB2D & Circle tests
// ------------------------------------------------------------------------------------------------

inline bool overlaps(const Circle& circle, const AABB2D& rect) noexcept;
inline bool overlaps(const AABB2D& rect, const Circle& circle) noexcept;

// Plane & AABB tests
// ------------------------------------------------------------------------------------------------

inline bool intersects(const Plane& plane, const AABB& aabb) noexcept;
inline bool intersects(const AABB& aabb, const Plane& plane) noexcept;
/// Checks whether AABB intersects with or is in positive half-space of plane.
inline bool abovePlane(const Plane& plane, const AABB& aabb) noexcept;
/// Checks whether AABB intersects with or is in negative half-space of plane.
inline bool belowPlane(const Plane& plane, const AABB& aabb) noexcept;

// Plane & OBB tests
// ------------------------------------------------------------------------------------------------

inline bool intersects(const Plane& plane, const OBB& obb) noexcept;
inline bool intersects(const OBB& obb, const Plane& plane) noexcept;
/// Checks whether OBB intersects with or is in positive half-space of plane.
inline bool abovePlane(const Plane& plane, const OBB& obb) noexcept;
/// Checks whether OBB intersects with or is in negative half-space of plane.
inline bool belowPlane(const Plane& plane, const OBB& obb) noexcept;

// Plane & Sphere tests
// ------------------------------------------------------------------------------------------------

inline bool intersects(const Plane& plane, const Sphere& sphere) noexcept;
inline bool intersects(const Sphere& sphere, const Plane& plane) noexcept;
/// Checks whether Sphere intersects with or is in positive half-space of plane.
inline bool abovePlane(const Plane& plane, const Sphere& sphere) noexcept;
/// Checks whether Sphere intersects with or is in negative half-space of plane.
inline bool belowPlane(const Plane& plane, const Sphere& sphere) noexcept;

// Closest point tests
// ------------------------------------------------------------------------------------------------

inline vec3 closestPoint(const AABB& aabb, const vec3& point) noexcept;
inline vec3 closestPoint(const OBB& obb, const vec3& point) noexcept;
inline vec3 closestPoint(const Plane& plane, const vec3& point) noexcept;
inline vec3 closestPoint(const Sphere& sphere, const vec3& point) noexcept;

} // namespace sfz

#include "sfz/geometry/Intersection.inl"
