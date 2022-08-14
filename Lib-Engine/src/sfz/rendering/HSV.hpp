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

#include <sfz.h>
#include <sfz_math.h>

namespace sfz {

// HSV
// ------------------------------------------------------------------------------------------------

// For the below functions:
// RGB in range: [0, 1]
// HSV:
//    Hue (r): [0, 360]
//    Saturation (g): [0, 1]
//    Value (b): [0, 1]

inline f32x3 rgbToHSV(f32x3 rgb)
{
	const f32 r = rgb.x;
	const f32 g = rgb.y;
	const f32 b = rgb.z;
	sfz_assert(0.0f <= r && r <= 1.0f);
	sfz_assert(0.0f <= g && g <= 1.0f);
	sfz_assert(0.0f <= b && b <= 1.0f);
	
	const f32 xMax = sfz::elemMax(rgb);
	const f32 xMin = sfz::elemMin(rgb);
	const f32 chroma = xMax - xMin;

	f32 hue = 0.0f;
	const f32 val = xMax;
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

	f32 sat = 0.0f;
	if (val > 0.0f) {
		sat = chroma / val;
	}

	if (hue < 0.0f) hue += 360.0f;

	return f32x3_init(hue, sat, val);
}

inline f32x3 hsvToRGB(f32x3 hsv)
{
	const f32 hue = hsv.x;
	const f32 sat = hsv.y;
	const f32 val = hsv.z;
	sfz_assert(0.0f <= hue && hue <= 360.0f);
	sfz_assert(0.0f <= sat && sat <= 1.0f);
	sfz_assert(0.0f <= val && val <= 1.0f);

	const f32 chroma = val * sat;
	const f32 X = chroma * (1.0f - f32_abs(fmodf(hue / 60.0f, 2) - 1.0f));

	f32x3 rgb;
	if (hue < 60.0f) {
		rgb = f32x3_init(chroma, X, 0.0f);
	}
	else if (hue < 120.0f) {
		rgb = f32x3_init(X, chroma, 0.0f);
	}
	else if (hue < 180.0f) {
		rgb = f32x3_init(0.0f, chroma, X);
	}
	else if (hue < 240.0f) {
		rgb = f32x3_init(0.0f, X, chroma);
	}
	else if (hue < 300.0f) {
		rgb = f32x3_init(X, 0.0f, chroma);
	}
	else if (hue <= 360.0f) {
		rgb = f32x3_init(chroma, 0.0f, X);
	}

	rgb += f32x3_splat(val - chroma);
	return rgb;
}

} // namespace sfz
