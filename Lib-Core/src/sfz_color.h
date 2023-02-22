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

#ifndef SFZ_COLOR_H
#define SFZ_COLOR_H
#pragma once

#include <math.h> // fmodf

#include <sfz.h>

// HSV (f32)
// ------------------------------------------------------------------------------------------------

// For the below functions:
// RGB in range: [0, 1]
// HSV:
//    Hue (r): [0, 360]
//    Saturation (g): [0, 1]
//    Value (b): [0, 1]

inline f32x3 sfzRgbToHSV(f32x3 rgb)
{
	const f32 r = rgb.x;
	const f32 g = rgb.y;
	const f32 b = rgb.z;
	sfz_assert(0.0f <= r && r <= 1.0f);
	sfz_assert(0.0f <= g && g <= 1.0f);
	sfz_assert(0.0f <= b && b <= 1.0f);
	
	const f32 x_max = f32_max(f32_max(r, g), b);
	const f32 x_min = f32_min(f32_min(r, g), b);
	const f32 chroma = x_max - x_min;

	f32 hue = 0.0f;
	const f32 val = x_max;
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

inline f32x3 sfzHsvToRGB(f32x3 hsv)
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

// Random color generator
// ------------------------------------------------------------------------------------------------

inline f32x3 sfzGetRandomColor(u32 idx, f32 sat, f32 val, f32 start_noise)
{
	// Inspired by: https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
	sfz_constant f32 GOLDEN_RATIO = 1.61803f;
	sfz_constant f32 HUE_DIFF = 360.0f * (1.0f / GOLDEN_RATIO);
	sfz_assert(0.0f <= sat && sat <= 1.0f);
	sfz_assert(0.0f <= val && val <= 1.0f);

	f32 hue = f32(idx) * HUE_DIFF + start_noise;
	hue = fmodf(hue, 360.0f);
	const f32x3 hsv = f32x3_init(hue, sat, val);

	return sfzHsvToRGB(hsv);
}
inline f32x3 sfzGetRandomColorDefaults(u32 idx) { return sfzGetRandomColor(idx, 0.5f, 0.95f, 0.0f); }

// HSV (u8)
// ------------------------------------------------------------------------------------------------

// Unlike the f32 versions, in this one we are always in range [0, 255] to utilize the u8's to
// their fullest. I.e.:
//
// RGB in range: [0, 255]
// HSV:
//    Hue (r): [0, 255]
//    Saturation (g): [0, 255]
//    Value (b): [0, 255]

sfz_constexpr_func u8x4 sfzRgbToHSV_u8(u8x4 rgb)
{
	const u8 r = rgb.x;
	const u8 g = rgb.y;
	const u8 b = rgb.z;
	u8 hue = 0;
	u8 sat = 0;
	u8 val = 0;

	// Calculate value
	const u8 x_max = u8_max(u8_max(r, g), b);
	val = x_max;
	if (val != 0) {

		// Calculate saturation
		const u8 x_min = u8_min(u8_min(r, g), b);
		const u8 chroma = x_max - x_min;
		sat = u8(255 * i32(chroma) / i32(val));
		if (sat != 0) {

			// Calculate hue
			if (x_max == r) {
				hue = 43 * (g - b) / chroma;
			}
			else if (x_max == g) {
				hue = 85 + 43 * (b - r) / chroma;
			}
			else {
				hue = 171 + 43 * (r - g) / chroma;
			}
		}
	}

	return u8x4_init(hue, sat, val, 0);
}

#endif // SFZ_COLOR_H
