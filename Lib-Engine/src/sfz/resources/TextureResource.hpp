// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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
#include <skipifzero_image_view.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

namespace sfz {

class Setting;

// TextureResource
// ------------------------------------------------------------------------------------------------

struct TextureResource final {
	strID name;

	zg::Texture texture;
	ZgTextureFormat format = ZG_TEXTURE_FORMAT_UNDEFINED;
	vec2_u32 res = vec2_u32(0u);
	uint32_t numMipmaps = 1;
	bool committedAllocation = false;
	ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT;
	ZgOptimalClearValue optimalClearValue = ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;

	bool screenRelativeResolution = false;
	float resolutionScale = 1.0f;
	Setting* resolutionScaleSetting = nullptr;

	[[nodiscard]] ZgResult build(vec2_u32 screenRes);

	void uploadBlocking(
		const ImageViewConst& image,
		sfz::Allocator* cpuAllocator,
		zg::CommandQueue& copyQueue);

	static TextureResource createFixedSize(
		const char* name,
		const ImageViewConst& image,
		bool allocateMipmaps = true,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false);

	static TextureResource createFixedSize(
		const char* name,
		ZgTextureFormat format,
		vec2_u32 res,
		uint32_t numMipmaps = 1,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false);

	static TextureResource createScreenRelative(
		const char* name,
		ZgTextureFormat format,
		vec2_u32 screenRes,
		float scale,
		Setting* scaleSetting = nullptr,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false);
};

// Texture functions
// ------------------------------------------------------------------------------------------------

ZgTextureFormat toZeroGImageFormat(ImageType imageType) noexcept;

} // namespace sfz
