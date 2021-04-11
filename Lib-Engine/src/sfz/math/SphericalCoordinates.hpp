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

#include <skipifzero.hpp>

namespace sfz {

// Spherical coordinates
// ------------------------------------------------------------------------------------------------

struct SphericalCoord final {

	// Distance from center ("length of vector"), range [0, inf]
	float r;

	// Rotation around z-axis in degrees, range [0, 360)
	float phi;

	// "Vertical" rotation angle in degrees, range [0, 180]
	float theta;
};

inline const SphericalCoord toSpherical(vec3 v)
{
	SphericalCoord coord = {};
	coord.r = length(v);
	if (coord.r == 0.0f) return coord;
	coord.phi = atan2(v.y, v.x) * sfz::RAD_TO_DEG; // Range [-180, 180]
	if (coord.phi < 0.0f) coord.phi += 360.0f; // Move into range [0, 360]
	if (coord.phi >= 360.0f) coord.phi -= 360.0f; // Move into range [0, 360)
	coord.theta = acos(v.z / coord.r) * sfz::RAD_TO_DEG; // Range [0, 180]
	coord.theta = sfz::clamp(coord.theta, 0.0f, 180.0f);
	return coord;
}

inline const vec3 fromSpherical(SphericalCoord c)
{
	const float thetaRads = c.theta * sfz::DEG_TO_RAD;
	const float phiRads = c.phi * sfz::DEG_TO_RAD;
	vec3 v;
	v.x = c.r * sin(thetaRads) * cos(phiRads);
	v.y = c.r * sin(thetaRads) * sin(phiRads);
	v.z = c.r * cos(thetaRads);
	return v;
}

} // namespace sfz
