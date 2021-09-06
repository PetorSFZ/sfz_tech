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
	ZgFormat format = ZG_FORMAT_UNDEFINED;
	i32x2 res = i32x2(0);
	u32 numMipmaps = 1;
	bool committedAllocation = false;
	ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT;
	ZgOptimalClearValue optimalClearValue = ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;

	// Whether resolution should be scaled relative screen resolution
	bool screenRelativeResolution = false;
	f32 resolutionScale = 1.0f;
	Setting* resolutionScaleSetting = nullptr;
	f32 resScaleSettingScale = 1.0f; // Amount to scale versus value in setting

	// Whether resolution is directly controlled by a setting
	bool settingControlledRes = false;
	Setting* controlledResSetting = nullptr;

	bool needRebuild(i32x2 screenRes) const;
	[[nodiscard]] ZgResult build(i32x2 screenRes);

	void uploadBlocking(
		const ImageViewConst& image,
		SfzAllocator* cpuAllocator,
		zg::CommandQueue& copyQueue);

	static TextureResource createFixedSize(
		const char* name,
		const ImageViewConst& image,
		bool allocateMipmaps = true,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false);

	static TextureResource createFixedSize(
		const char* name,
		ZgFormat format,
		i32x2 res,
		u32 numMipmaps = 1,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false);

	static TextureResource createScreenRelative(
		const char* name,
		ZgFormat format,
		i32x2 screenRes,
		f32 scale,
		Setting* scaleSetting = nullptr,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false,
		f32 resScaleSettingScale = 1.0f);

	static TextureResource createSettingControlled(
		const char* name,
		ZgFormat format,
		Setting* resSetting,
		u32 numMipmaps = 1,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false);
};

// Texture functions
// ------------------------------------------------------------------------------------------------

ZgFormat toZeroGImageFormat(ImageType imageType) noexcept;

} // namespace sfz
