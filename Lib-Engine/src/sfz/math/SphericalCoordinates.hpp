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

#include <sfz.h>

namespace sfz {

// Spherical coordinates
// ------------------------------------------------------------------------------------------------

struct SphericalCoord final {

	// Distance from center ("length of vector"), range [0, inf]
	f32 r;

	// Rotation around z-axis in degrees, range [0, 360)
	f32 phi;

	// "Vertical" rotation angle in degrees, range [0, 180]
	f32 theta;
};

inline const SphericalCoord toSpherical(f32x3 v)
{
	SphericalCoord coord = {};
	coord.r = f32x3_length(v);
	if (coord.r == 0.0f) return coord;
	coord.phi = sfz_atan2(v.y, v.x) * SFZ_RAD_TO_DEG; // Range [-180, 180]
	if (coord.phi < 0.0f) coord.phi += 360.0f; // Move into range [0, 360]
	if (coord.phi >= 360.0f) coord.phi -= 360.0f; // Move into range [0, 360)
	coord.theta = sfz_acos(v.z / coord.r) * SFZ_RAD_TO_DEG; // Range [0, 180]
	coord.theta = f32_clamp(coord.theta, 0.0f, 180.0f);
	return coord;
}

inline const f32x3 fromSpherical(SphericalCoord c)
{
	const f32 thetaRads = c.theta * SFZ_DEG_TO_RAD;
	const f32 phiRads = c.phi * SFZ_DEG_TO_RAD;
	f32x3 v;
	v.x = c.r * sfz_sin(thetaRads) * sfz_cos(phiRads);
	v.y = c.r * sfz_sin(thetaRads) * sfz_sin(phiRads);
	v.z = c.r * sfz_cos(thetaRads);
	return v;
}

} // namespace sfz
