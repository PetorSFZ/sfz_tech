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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "utest.h"
#undef near
#undef far

#include "sfz/rendering/HSV.hpp"

UTEST(HSV, rgb_to_hsv)
{
	{
		constexpr sfz::vec3 rgb = sfz::vec3(219.0f, 122.0f, 124.0f) * (1.0f / 255.0f);
		constexpr sfz::vec3 expectedHsv = sfz::vec3(359.0f, 44.0f, 86.0f) * sfz::vec3(1.0f, 0.01f, 0.01f);
		sfz::vec3 hsv = sfz::rgbToHSV(rgb);
		ASSERT_TRUE(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		ASSERT_TRUE(sfz::eqf(expectedHsv.yz, hsv.yz, 0.05f));
	}

	{
		constexpr sfz::vec3 rgb = sfz::vec3(16.0f, 79.0f, 15.0f) * (1.0f / 255.0f);
		constexpr sfz::vec3 expectedHsv = sfz::vec3(119.0f, 80.0f, 31.0f) * sfz::vec3(1.0f, 0.01f, 0.01f);
		sfz::vec3 hsv = sfz::rgbToHSV(rgb);
		ASSERT_TRUE(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		ASSERT_TRUE(sfz::eqf(expectedHsv.yz, hsv.yz, 0.05f));
	}

	{
		constexpr sfz::vec3 rgb = sfz::vec3(226.0f, 149.0f, 210.0f) * (1.0f / 255.0f);
		constexpr sfz::vec3 expectedHsv = sfz::vec3(313.0f, 34.0f, 89.0f) * sfz::vec3(1.0f, 0.01f, 0.01f);

		sfz::vec3 hsv = sfz::rgbToHSV(rgb);
		ASSERT_TRUE(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		ASSERT_TRUE(sfz::eqf(expectedHsv.yz, hsv.yz, 0.05f));
	}

	{
		constexpr sfz::vec3 rgb = sfz::vec3(34.0f, 63.0f, 5.0f) * (1.0f / 255.0f);
		constexpr sfz::vec3 expectedHsv = sfz::vec3(90.0f, 92.0f, 25.0f) * sfz::vec3(1.0f, 0.01f, 0.01f);
		sfz::vec3 hsv = sfz::rgbToHSV(rgb);
		ASSERT_TRUE(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		ASSERT_TRUE(sfz::eqf(expectedHsv.yz, hsv.yz, 0.05f));
	}

	{
		constexpr sfz::vec3 rgb = sfz::vec3(26.0f, 51.0f, 77.0f) * (1.0f / 255.0f);
		constexpr sfz::vec3 expectedHsv = sfz::vec3(211.0f, 66.0f, 30.0f) * sfz::vec3(1.0f, 0.01f, 0.01f);
		sfz::vec3 hsv = sfz::rgbToHSV(rgb);
		ASSERT_TRUE(sfz::eqf(hsv.x, expectedHsv.x, 1.0f));
		ASSERT_TRUE(sfz::eqf(expectedHsv.yz, hsv.yz, 0.05f));
	}
}

UTEST(HSV, hsv_to_rgb)
{
	// If saturation and value is 0, rgb should be 0 regardless of hue value
	{
		constexpr uint32_t NUM_SAMPLES = 10;
		for (uint32_t i = 1; i <= NUM_SAMPLES; i++) {
			const float hue = 360.0f * (float(i) / float(NUM_SAMPLES));
			sfz::vec3 rgb = sfz::hsvToRGB(sfz::vec3(hue, 0.0f, 0.0f));
			ASSERT_TRUE(sfz::eqf(rgb, sfz::vec3(0.0f)));
		}
	}

	// If saturation = 0 and value = 1, then rgb should be 1 regardless value
	{
		constexpr uint32_t NUM_SAMPLES = 10;
		for (uint32_t i = 1; i <= NUM_SAMPLES; i++) {
			const float hue = 360.0f * (float(i) / float(NUM_SAMPLES));
			sfz::vec3 rgb = sfz::hsvToRGB(sfz::vec3(hue, 0.0f, 1.0f));
			ASSERT_TRUE(sfz::eqf(rgb, sfz::vec3(1.0f)));
		}
	}

	{
		constexpr sfz::vec3 hsv = sfz::vec3(359.0f, 44.0f, 86.0f) * sfz::vec3(1.0f, 0.01f, 0.01f);
		constexpr sfz::vec3 expectedRgb = sfz::vec3(219.0f, 122.0f, 124.0f) * (1.0f / 255.0f);
		sfz::vec3 rgb = sfz::hsvToRGB(hsv);
		ASSERT_TRUE(sfz::eqf(expectedRgb, rgb, 0.01f));
	}

	{
		constexpr sfz::vec3 hsv = sfz::vec3(119.0f, 80.0f, 31.0f) * sfz::vec3(1.0f, 0.01f, 0.01f);
		constexpr sfz::vec3 expectedRgb = sfz::vec3(16.0f, 79.0f, 15.0f) * (1.0f / 255.0f);
		sfz::vec3 rgb = sfz::hsvToRGB(hsv);
		ASSERT_TRUE(sfz::eqf(expectedRgb, rgb, 0.01f));
	}

	{
		constexpr sfz::vec3 hsv = sfz::vec3(313.0f, 34.0f, 89.0f) * sfz::vec3(1.0f, 0.01f, 0.01f);
		constexpr sfz::vec3 expectedRgb = sfz::vec3(226.0f, 149.0f, 210.0f) * (1.0f / 255.0f);
		sfz::vec3 rgb = sfz::hsvToRGB(hsv);
		ASSERT_TRUE(sfz::eqf(expectedRgb, rgb, 0.01f));
	}

	{
		constexpr sfz::vec3 hsv = sfz::vec3(90.0f, 92.0f, 25.0f) * sfz::vec3(1.0f, 0.01f, 0.01f);
		constexpr sfz::vec3 expectedRgb = sfz::vec3(34.0f, 63.0f, 5.0f) * (1.0f / 255.0f);
		sfz::vec3 rgb = sfz::hsvToRGB(hsv);
		ASSERT_TRUE(sfz::eqf(expectedRgb, rgb, 0.01f));
	}
}


UTEST(HSV, rgb_to_hsv_and_back)
{
	// Evenly distributed samples over rgb
	constexpr uint32_t NUM_SAMPLES = 16;
	for (uint32_t x = 1; x <= NUM_SAMPLES; x++) {
		const float xVal = float(x) / float(NUM_SAMPLES);

		for (uint32_t y = 1; y <= NUM_SAMPLES; y++) {
			const float yVal = float(y) / float(NUM_SAMPLES);
			
			for (uint32_t z = 1; z <= NUM_SAMPLES; z++) {
				const float zVal = float(z) / float(NUM_SAMPLES);
				
				sfz::vec3 rgbOriginal = sfz::vec3(xVal, yVal, zVal);
				sfz::vec3 hsv = sfz::rgbToHSV(rgbOriginal);
				sfz::vec3 rgb = sfz::hsvToRGB(hsv);

				ASSERT_TRUE(sfz::eqf(rgbOriginal, rgb));
			}
		}
	}
}

UTEST(HSV, hsv_to_rgb_and_back)
{
	// Evenly distributed samples over hsv
	constexpr uint32_t NUM_SAMPLES = 16;
	for (uint32_t x = 1; x <= (NUM_SAMPLES * 2); x++) {
		const float xVal = 359.9f * (float(x) / float(NUM_SAMPLES * 2));

		for (uint32_t y = 1; y <= NUM_SAMPLES; y++) {
			const float yVal = float(y) / float(NUM_SAMPLES);

			for (uint32_t z = 1; z <= NUM_SAMPLES; z++) {
				const float zVal = float(z) / float(NUM_SAMPLES);

				sfz::vec3 hsvOriginal = sfz::vec3(xVal, yVal, zVal);
				sfz::vec3 rgb = sfz::hsvToRGB(hsvOriginal);
				sfz::vec3 hsv = sfz::rgbToHSV(rgb);

				ASSERT_TRUE(sfz::eqf(hsvOriginal, hsv));
			}
		}
	}
}
