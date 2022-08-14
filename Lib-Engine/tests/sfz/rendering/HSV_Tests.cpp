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

#include <doctest.h>

#include "sfz/rendering/HSV.hpp"

TEST_CASE("HSV: rgb_to_hsv")
{
	{
		constexpr f32x3 rgb = f32x3_init(219.0f, 122.0f, 124.0f) * (1.0f / 255.0f);
		constexpr f32x3 expectedHsv = f32x3_init(359.0f, 44.0f, 86.0f) * f32x3_init(1.0f, 0.01f, 0.01f);
		f32x3 hsv = sfz::rgbToHSV(rgb);
		CHECK(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		CHECK(sfz::eqf(expectedHsv.y, hsv.y, 0.05f));
		CHECK(sfz::eqf(expectedHsv.z, hsv.z, 0.05f));
	}

	{
		constexpr f32x3 rgb = f32x3_init(16.0f, 79.0f, 15.0f) * (1.0f / 255.0f);
		constexpr f32x3 expectedHsv = f32x3_init(119.0f, 80.0f, 31.0f) * f32x3_init(1.0f, 0.01f, 0.01f);
		f32x3 hsv = sfz::rgbToHSV(rgb);
		CHECK(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		CHECK(sfz::eqf(expectedHsv.y, hsv.y, 0.05f));
		CHECK(sfz::eqf(expectedHsv.z, hsv.z, 0.05f));
	}

	{
		constexpr f32x3 rgb = f32x3_init(226.0f, 149.0f, 210.0f) * (1.0f / 255.0f);
		constexpr f32x3 expectedHsv = f32x3_init(313.0f, 34.0f, 89.0f) * f32x3_init(1.0f, 0.01f, 0.01f);

		f32x3 hsv = sfz::rgbToHSV(rgb);
		CHECK(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		CHECK(sfz::eqf(expectedHsv.y, hsv.y, 0.05f));
		CHECK(sfz::eqf(expectedHsv.z, hsv.z, 0.05f));
	}

	{
		constexpr f32x3 rgb = f32x3_init(34.0f, 63.0f, 5.0f) * (1.0f / 255.0f);
		constexpr f32x3 expectedHsv = f32x3_init(90.0f, 92.0f, 25.0f) * f32x3_init(1.0f, 0.01f, 0.01f);
		f32x3 hsv = sfz::rgbToHSV(rgb);
		CHECK(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		CHECK(sfz::eqf(expectedHsv.y, hsv.y, 0.05f));
		CHECK(sfz::eqf(expectedHsv.z, hsv.z, 0.05f));
	}

	{
		constexpr f32x3 rgb = f32x3_init(26.0f, 51.0f, 77.0f) * (1.0f / 255.0f);
		constexpr f32x3 expectedHsv = f32x3_init(211.0f, 66.0f, 30.0f) * f32x3_init(1.0f, 0.01f, 0.01f);
		f32x3 hsv = sfz::rgbToHSV(rgb);
		CHECK(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		CHECK(sfz::eqf(expectedHsv.y, hsv.y, 0.05f));
		CHECK(sfz::eqf(expectedHsv.z, hsv.z, 0.05f));
	}
}

TEST_CASE("HSV: hsv_to_rgb")
{
	// If saturation and value is 0, rgb should be 0 regardless of hue value
	{
		constexpr u32 NUM_SAMPLES = 10;
		for (u32 i = 1; i <= NUM_SAMPLES; i++) {
			const f32 hue = 360.0f * (f32(i) / f32(NUM_SAMPLES));
			f32x3 rgb = sfz::hsvToRGB(f32x3_init(hue, 0.0f, 0.0f));
			CHECK(sfz::eqf(rgb, f32x3_splat(0.0f)));
		}
	}

	// If saturation = 0 and value = 1, then rgb should be 1 regardless value
	{
		constexpr u32 NUM_SAMPLES = 10;
		for (u32 i = 1; i <= NUM_SAMPLES; i++) {
			const f32 hue = 360.0f * (f32(i) / f32(NUM_SAMPLES));
			f32x3 rgb = sfz::hsvToRGB(f32x3_init(hue, 0.0f, 1.0f));
			CHECK(sfz::eqf(rgb, f32x3_splat(1.0f)));
		}
	}

	{
		constexpr f32x3 hsv = f32x3_init(359.0f, 44.0f, 86.0f) * f32x3_init(1.0f, 0.01f, 0.01f);
		constexpr f32x3 expectedRgb = f32x3_init(219.0f, 122.0f, 124.0f) * (1.0f / 255.0f);
		f32x3 rgb = sfz::hsvToRGB(hsv);
		CHECK(sfz::eqf(expectedRgb, rgb, 0.01f));
	}

	{
		constexpr f32x3 hsv = f32x3_init(119.0f, 80.0f, 31.0f) * f32x3_init(1.0f, 0.01f, 0.01f);
		constexpr f32x3 expectedRgb = f32x3_init(16.0f, 79.0f, 15.0f) * (1.0f / 255.0f);
		f32x3 rgb = sfz::hsvToRGB(hsv);
		CHECK(sfz::eqf(expectedRgb, rgb, 0.01f));
	}

	{
		constexpr f32x3 hsv = f32x3_init(313.0f, 34.0f, 89.0f) * f32x3_init(1.0f, 0.01f, 0.01f);
		constexpr f32x3 expectedRgb = f32x3_init(226.0f, 149.0f, 210.0f) * (1.0f / 255.0f);
		f32x3 rgb = sfz::hsvToRGB(hsv);
		CHECK(sfz::eqf(expectedRgb, rgb, 0.01f));
	}

	{
		constexpr f32x3 hsv = f32x3_init(90.0f, 92.0f, 25.0f) * f32x3_init(1.0f, 0.01f, 0.01f);
		constexpr f32x3 expectedRgb = f32x3_init(34.0f, 63.0f, 5.0f) * (1.0f / 255.0f);
		f32x3 rgb = sfz::hsvToRGB(hsv);
		CHECK(sfz::eqf(expectedRgb, rgb, 0.01f));
	}
}


TEST_CASE("HSV: rgb_to_hsv_and_back")
{
	// Evenly distributed samples over rgb
	constexpr u32 NUM_SAMPLES = 16;
	for (u32 x = 1; x <= NUM_SAMPLES; x++) {
		const f32 xVal = f32(x) / f32(NUM_SAMPLES);

		for (u32 y = 1; y <= NUM_SAMPLES; y++) {
			const f32 yVal = f32(y) / f32(NUM_SAMPLES);
			
			for (u32 z = 1; z <= NUM_SAMPLES; z++) {
				const f32 zVal = f32(z) / f32(NUM_SAMPLES);
				
				f32x3 rgbOriginal = f32x3_init(xVal, yVal, zVal);
				f32x3 hsv = sfz::rgbToHSV(rgbOriginal);
				f32x3 rgb = sfz::hsvToRGB(hsv);

				CHECK(sfz::eqf(rgbOriginal, rgb));
			}
		}
	}
}

TEST_CASE("HSV: hsv_to_rgb_and_back")
{
	// Evenly distributed samples over hsv
	constexpr u32 NUM_SAMPLES = 16;
	for (u32 x = 1; x <= (NUM_SAMPLES * 2); x++) {
		const f32 xVal = 359.9f * (f32(x) / f32(NUM_SAMPLES * 2));

		for (u32 y = 1; y <= NUM_SAMPLES; y++) {
			const f32 yVal = f32(y) / f32(NUM_SAMPLES);

			for (u32 z = 1; z <= NUM_SAMPLES; z++) {
				const f32 zVal = f32(z) / f32(NUM_SAMPLES);

				f32x3 hsvOriginal = f32x3_init(xVal, yVal, zVal);
				f32x3 rgb = sfz::hsvToRGB(hsvOriginal);
				f32x3 hsv = sfz::rgbToHSV(rgb);

				CHECK(sfz::eqf(hsvOriginal, hsv));
			}
		}
	}
}
