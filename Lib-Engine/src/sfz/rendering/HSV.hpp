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

#pragma once

#include <skipifzero.hpp>

namespace sfz {

// HSV
// ------------------------------------------------------------------------------------------------

// For the below functions:
// RGB in range: [0, 1]
// HSV:
//    Hue (r): [0, 360]
//    Saturation (g): [0, 1]
//    Value (b): [0, 1]

inline vec3 rgbToHSV(vec3 rgb)
{
	const float r = rgb.x;
	const float g = rgb.y;
	const float b = rgb.z;
	sfz_assert(0.0f <= r && r <= 1.0f);
	sfz_assert(0.0f <= g && g <= 1.0f);
	sfz_assert(0.0f <= b && b <= 1.0f);
	
	const float xMax = sfz::elemMax(rgb);
	const float xMin = sfz::elemMin(rgb);
	const float chroma = xMax - xMin;

	float hue = 0.0f;
	const float val = xMax;
	if (chroma > 0.0f) {
		if (val == r) {
			hue = 60.0f * ((g - b) / chroma);
		}
		else if (val == g) {
			hue = 60.0f * (2.0f + ((b - r) / chroma));
		}
		else if (val == b) {
			hue = 60.0f * (4.0f + ((r - g) / chroma));
		}
		else {
			sfz_assert(false);
		}
	}

	float sat = 0.0f;
	if (val > 0.0f) {
		sat = chroma / val;
	}

	if (hue < 0.0f) hue += 360.0f;

	return vec3(hue, sat, val);
}

inline vec3 hsvToRGB(vec3 hsv)
{
	const float hue = hsv.x;
	const float sat = hsv.y;
	const float val = hsv.z;
	sfz_assert(0.0f <= hue && hue <= 360.0f);
	sfz_assert(0.0f <= sat && sat <= 1.0f);
	sfz_assert(0.0f <= val && val <= 1.0f);

	const float chroma = val * sat;
	const float X = chroma * (1.0f - sfz::abs(fmodf(hue / 60.0f, 2) - 1.0f));

	vec3 rgb;
	if (hue < 60.0f) {
		rgb = vec3(chroma, X, 0.0f);
	}
	else if (hue < 120.0f) {
		rgb = vec3(X, chroma, 0.0f);
	}
	else if (hue < 180.0f) {
		rgb = vec3(0.0f, chroma, X);
	}
	else if (hue < 240.0f) {
		rgb = vec3(0.0f, X, chroma);
	}
	else if (hue < 300.0f) {
		rgb = vec3(X, 0.0f, chroma);
	}
	else if (hue <= 360.0f) {
		rgb = vec3(chroma, 0.0f, X);
	}

	rgb += vec3(val - chroma);
	return rgb;
}

} // namespace sfz
