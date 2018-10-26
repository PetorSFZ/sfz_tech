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

#include "sfz/Assert.hpp"
#include "sfz/math/Vector.hpp"
#include "sfz/math/MathSupport.hpp"

namespace sfz {

/// Class representing a Plane
///
/// Mathematical definition (plane normal = n, position on plane = p, position to test = x):
/// f(x) = dot(n, x - p) = dot(n, x) - d
/// d = dot(n, p)
class Plane final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	inline Plane() noexcept = default;

	/// dot(normal, x) - d = 0
	inline Plane(const vec3& normal, float d) noexcept;
	/// dot(normal, x - position) = 0
	inline Plane(const vec3& normal, const vec3& position) noexcept;

	// Public member functions
	// --------------------------------------------------------------------------------------------

	/// Returns the signed distance to the plane. Positive if above, negative if below.
	inline float signedDistance(const vec3& point) const noexcept;

	// Public getters/setters
	// --------------------------------------------------------------------------------------------

	inline const vec3& normal() const noexcept { return mNormal; }
	inline float d() const noexcept { return mD; }

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	vec3 mNormal;
	float mD;
};

} // namespace sfz

#include "sfz/geometry/Plane.inl"
