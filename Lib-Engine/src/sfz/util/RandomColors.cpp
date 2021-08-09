// Copyright (c) 2020 Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include "sfz/util/RandomColors.hpp"

#include "sfz/rendering/HSV.hpp"

namespace sfz {

// Random color generator
// ------------------------------------------------------------------------------------------------

vec3 getRandomColor(u32 idx, float sat, float val, float startNoise)
{
	// Inspired by: https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
	constexpr float GOLDEN_RATIO = 1.61803f;
	constexpr float HUE_DIFF = 360.0f * (1.0f / GOLDEN_RATIO);
	sfz_assert(0.0f <= sat && sat <= 1.0f);
	sfz_assert(0.0f <= val && val <= 1.0f);

	float hue = float(idx) * HUE_DIFF + startNoise;
	hue = fmodf(hue, 360.0f);
	const vec3 hsv = vec3(hue, sat, val);
	
	return hsvToRGB(hsv);
}

} // namespace sfz
