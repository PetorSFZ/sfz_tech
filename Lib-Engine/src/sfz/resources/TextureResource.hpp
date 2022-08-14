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

#include <sfz.h>
#include <sfz_image_view.h>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

struct SfzSetting;

// SfzTextureResource
// ------------------------------------------------------------------------------------------------

struct SfzTextureResource final {
	SfzStrID name = SFZ_NULL_STR_ID;

	zg::Texture texture;
	ZgFormat format = ZG_FORMAT_UNDEFINED;
	i32x2 res = i32x2_splat(0);
	u32 numMipmaps = 1;
	bool committedAllocation = false;
	ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT;
	ZgOptimalClearValue optimalClearValue = ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;

	// Whether resolution should be scaled relative screen resolution
	bool screenRelativeRes = false;
	f32 resScale = 1.0f;
	SfzSetting* resScaleSetting = nullptr;
	SfzSetting* resScaleSetting2 = nullptr; // Amount to scale versus value in first setting
	f32 resScaleSettingScale = 1.0f; // Amount to scale versus value in setting

	// Whether resolution is directly controlled by a setting
	bool settingControlledRes = false;
	SfzSetting* controlledResSetting = nullptr;

	bool needRebuild(i32x2 screenRes) const;
	[[nodiscard]] ZgResult build(i32x2 screenRes, SfzStrIDs* ids);

	void uploadBlocking(
		const SfzImageViewConst& image,
		SfzAllocator* cpuAllocator,
		ZgUploader* uploader,
		zg::CommandQueue& copyQueue);

	static SfzTextureResource createFixedSize(
		const char* name,
		SfzStrIDs* ids,
		const SfzImageViewConst& image,
		bool allocateMipmaps = true,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false);

	static SfzTextureResource createFixedSize(
		const char* name,
		SfzStrIDs* ids,
		ZgFormat format,
		i32x2 res,
		u32 numMipmaps = 1,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false);

	static SfzTextureResource createScreenRelative(
		const char* name,
		SfzStrIDs* ids,
		ZgFormat format,
		i32x2 screenRes,
		f32 scale,
		SfzSetting* scaleSetting = nullptr,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false,
		SfzSetting* scaleSetting2 = nullptr,
		f32 resScaleSettingScale = 1.0f);

	static SfzTextureResource createSettingControlled(
		const char* name,
		SfzStrIDs* ids,
		ZgFormat format,
		SfzSetting* resSetting,
		u32 numMipmaps = 1,
		ZgTextureUsage usage = ZG_TEXTURE_USAGE_DEFAULT,
		bool committedAllocation = false);
};

// Texture functions
// ------------------------------------------------------------------------------------------------

ZgFormat toZeroGImageFormat(SfzImageType imageType) noexcept;
