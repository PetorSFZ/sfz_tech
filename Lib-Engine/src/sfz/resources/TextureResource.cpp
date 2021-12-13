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

#include "sfz/resources/TextureResource.hpp"

#include <algorithm>

#include <skipifzero.hpp>

#include "sfz/config/Setting.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/rendering/Image.hpp"

namespace sfz {

// Statics
// ------------------------------------------------------------------------------------------------

static u32 sizeOfElement(ImageType imageType) noexcept
{
	switch (imageType) {
	case ImageType::UNDEFINED: return 0;
	case ImageType::R_U8: return 1 * sizeof(u8);
	case ImageType::RG_U8: return 2 * sizeof(u8);
	case ImageType::RGBA_U8: return 4 * sizeof(u8);

	case ImageType::R_F32: return 1 * sizeof(f32);
	case ImageType::RG_F32: return 2 * sizeof(f32);
	case ImageType::RGBA_F32: return 4 * sizeof(f32);
	}
	sfz_assert(false);
	return 0;
}

static ZgImageViewConstCpu toZeroGImageView(const ImageViewConst& phView) noexcept
{
	ZgImageViewConstCpu zgView = {};
	zgView.format = toZeroGImageFormat(phView.type);
	zgView.data = phView.rawData;
	zgView.width = phView.width;
	zgView.height = phView.height;
	zgView.pitchInBytes = phView.width * sizeOfElement(phView.type);
	return zgView;
}

template<typename T, typename Averager>
void generateMipmapSpecific(
	const ImageViewConst& prevLevel, Image& currLevel, Averager averager) noexcept
{
	const T* srcImg = reinterpret_cast<const T*>(prevLevel.rawData);
	T* dstImg = reinterpret_cast<T*>(currLevel.rawData.data());

	for (i32 y = 0; y < currLevel.height; y++) {
		T* dstRow = dstImg + y * currLevel.width;
		const T* srcRow0 = srcImg + ((y * 2) + 0) * prevLevel.width;
		const T* srcRow1 = srcImg + ((y * 2) + 1) * prevLevel.width;

		for (i32 x = 0; x < currLevel.width; x++) {
			const T* srcPixelRow0 = srcRow0 + (x * 2);
			const T* srcPixelRow1 = srcRow1 + (x * 2);
			dstRow[x] = averager(
				srcPixelRow0[0], srcPixelRow0[1],
				srcPixelRow1[0], srcPixelRow1[1]);
		}
	}
}

// TODO: This is sort of bad because:
// a) We should not downscale in gamma space, but in linear space
// b) We should probably do something smarter than naive averaging
// c) We should not read from previous level, but from the original level when calculating a
//    specific level.
static void generateMipmap(const ImageViewConst& prevLevel, Image& currLevel) noexcept
{
	sfz_assert(prevLevel.type == currLevel.type);
	switch (currLevel.type) {
	case ImageType::R_U8:
		generateMipmapSpecific<u8>(prevLevel, currLevel, [](u8 a, u8 b, u8 c, u8 d) {
			return u8((u32(a) + u32(b) + u32(c) + u32(d)) / 4u);
		});
		break;
	case ImageType::RG_U8:
		generateMipmapSpecific<u8x2>(prevLevel, currLevel, [](u8x2 a, u8x2 b, u8x2 c, u8x2 d) {
			return u8x2((i32x2(a) + i32x2(b) + i32x2(c) + i32x2(d)) / 4u);
		});
		break;
	case ImageType::RGBA_U8:
		generateMipmapSpecific<u8x4>(prevLevel, currLevel, [](u8x4 a, u8x4 b, u8x4 c, u8x4 d) {
			return u8x4((i32x4(a) + i32x4(b) + i32x4(c) + i32x4(d)) / 4u);
		});
		break;

	case ImageType::RGBA_F32:
		generateMipmapSpecific<f32x4>(prevLevel, currLevel, [](f32x4 a, f32x4 b, f32x4 c, f32x4 d) {
			return (a + b + c + d) * (1.0f / 4.0f);
		});
		break;
	
	case ImageType::UNDEFINED:
	case ImageType::R_F32:
	case ImageType::RG_F32:
	default:
		sfz_assert_hard(false);
		break;
	};
}

// TextureResource
// ------------------------------------------------------------------------------------------------

bool TextureResource::needRebuild(i32x2 screenRes) const
{
	if (!this->texture.valid()) return true;

	i32x2 newRes = i32x2(0);
	if (screenRelativeRes) {
		f32 resoScale = this->resScale;
		if (this->resScaleSetting != nullptr) {
			resoScale = resScaleSetting->floatValue() * this->resScaleSettingScale;
			if (this->resScaleSetting2 != nullptr) {
				resoScale *= resScaleSetting2->floatValue();
			}
		}
		f32x2 scaledRes = f32x2(screenRes) * resoScale;
		newRes.x = u32(::roundf(scaledRes.x));
		newRes.y = u32(::roundf(scaledRes.y));
	}
	else if (settingControlledRes) {
		sfz_assert(0 < controlledResSetting->intValue() && controlledResSetting->intValue() <= 16384);
		newRes = i32x2(controlledResSetting->intValue());
	}
	else {
		newRes = this->res;
	}
	
	return newRes != this->res;
}

ZgResult TextureResource::build(i32x2 screenRes)
{
	// Set resolution and resolution scale if screen relative
	i32x2 newRes = i32x2(0);
	if (screenRelativeRes) {
		if (this->resScaleSetting != nullptr) {
			this->resScale = resScaleSetting->floatValue() * this->resScaleSettingScale;
			if (this->resScaleSetting2 != nullptr) {
				this->resScale *= resScaleSetting2->floatValue();
			}
		}
		f32x2 scaledRes = f32x2(screenRes) * this->resScale;
		newRes.x = u32(::roundf(scaledRes.x));
		newRes.y = u32(::roundf(scaledRes.y));
	}
	else if (settingControlledRes) {
		sfz_assert(0 < controlledResSetting->intValue() && controlledResSetting->intValue() <= 16384);
		newRes = i32x2(controlledResSetting->intValue());
	}
	else {
		newRes = this->res;
	}
	if (this->texture.valid() && this->res == newRes) return ZG_SUCCESS;
	this->res = newRes;

	sfz_assert(res.x > 0);
	sfz_assert(res.y > 0);
	sfz_assert(numMipmaps > 0);
	sfz_assert(numMipmaps <= ZG_MAX_NUM_MIPMAPS);

	ZgTextureDesc desc = {};
	desc.format = format;
	desc.committedAllocation = committedAllocation ? ZG_TRUE : ZG_FALSE;
	desc.allowUnorderedAccess = (usage == ZG_TEXTURE_USAGE_DEPTH_BUFFER) ? ZG_FALSE : ZG_TRUE;
	desc.usage = usage;
	desc.optimalClearValue =
		(usage == ZG_TEXTURE_USAGE_DEFAULT) ? ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED : optimalClearValue;
	desc.width = res.x;
	desc.height = res.y;
	desc.numMipmaps = numMipmaps;
	desc.debugName = sfzStrIDGetStr(name);
	return texture.create(desc);
}

void TextureResource::uploadBlocking(
	const ImageViewConst& image,
	SfzAllocator* cpuAllocator,
	ZgUploader* uploader,
	zg::CommandQueue& copyQueue)
{
	sfz_assert(texture.valid());
	sfz_assert(image.width == res.x);
	sfz_assert(image.height == res.y);
	
	// Convert to ZeroG Image View
	ZgImageViewConstCpu view = toZeroGImageView(image);
	sfz_assert(format == view.format);

	// Generate mipmaps (on CPU)
	Image mipmaps[ZG_MAX_NUM_MIPMAPS - 1];
	for (u32 i = 0; i < (numMipmaps - 1); i++) {

		// Get previous mipmap level
		ImageViewConst prevLevel;
		if (i == 0) prevLevel = image;
		else prevLevel = mipmaps[i - 1];

		// Allocate mipmap memory
		mipmaps[i] = Image::allocate(
			prevLevel.width / 2, prevLevel.height / 2, prevLevel.type, cpuAllocator);

		// Generate mipmap
		generateMipmap(prevLevel, mipmaps[i]);
	}

	// Create image views
	ZgImageViewConstCpu imageViews[ZG_MAX_NUM_MIPMAPS] = {};
	imageViews[0] = view;
	for (u32 i = 0; i < (numMipmaps - 1); i++) {
		imageViews[i + 1] = toZeroGImageView(mipmaps[i]);
	}

	// Copy texture to GPU
	zg::CommandList commandList;
	CHECK_ZG copyQueue.beginCommandListRecording(commandList);
	for (u32 i = 0; i < numMipmaps; i++) {
		CHECK_ZG commandList.uploadToTexture(uploader, texture.handle, i, &imageViews[i]);
	}
	CHECK_ZG commandList.enableQueueTransition(texture);
	CHECK_ZG copyQueue.executeCommandList(commandList);
	CHECK_ZG copyQueue.flush();
}

TextureResource TextureResource::createFixedSize(
	const char* name,
	const ImageViewConst& image,
	bool allocateMipmaps,
	ZgTextureUsage usage,
	bool committedAllocation)
{
	sfz_assert(isPowerOfTwo(image.width));
	sfz_assert(isPowerOfTwo(image.height));

	// Calculate number of mipmaps if requested
	u32 numMipmaps = 1;
	if (allocateMipmaps) {
		u32 logWidth = sfz::max(u32(log2(image.width)), 1u);
		u32 logHeight = sfz::max(u32(log2(image.height)), 1u);
		u32 logMin = std::min(logWidth, logHeight);
		numMipmaps = std::min(logMin, (ZG_MAX_NUM_MIPMAPS - 1));
	}
	sfz_assert(numMipmaps != 0);

	return TextureResource::createFixedSize(
		name,
		toZeroGImageFormat(image.type),
		i32x2(image.width, image.height),
		numMipmaps,
		usage,
		committedAllocation);
}

TextureResource TextureResource::createFixedSize(
	const char* name,
	ZgFormat format,
	i32x2 res,
	u32 numMipmaps,
	ZgTextureUsage usage,
	bool committedAllocation)
{
	sfz_assert(res.x > 0);
	sfz_assert(res.y > 0);
	sfz_assert(numMipmaps > 0);
	sfz_assert(numMipmaps <= ZG_MAX_NUM_MIPMAPS);

	TextureResource resource;
	resource.name = sfzStrIDCreate(name);
	resource.format = format;
	resource.res = res;
	resource.numMipmaps = numMipmaps;
	resource.committedAllocation = committedAllocation;
	resource.usage = usage;
	resource.optimalClearValue =
		usage != ZG_TEXTURE_USAGE_DEFAULT ? ZG_OPTIMAL_CLEAR_VALUE_ZERO : ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;

	CHECK_ZG resource.build(i32x2(0));

	return resource;
}

TextureResource TextureResource::createScreenRelative(
	const char* name,
	ZgFormat format,
	i32x2 screenRes,
	f32 scale,
	Setting* scaleSetting,
	ZgTextureUsage usage,
	bool committedAllocation,
	Setting* scaleSetting2,
	f32 resScaleSettingScale)
{
	TextureResource resource;
	resource.name = sfzStrIDCreate(name);
	resource.format = format;
	resource.numMipmaps = 1;
	resource.committedAllocation = committedAllocation;
	resource.usage = usage;
	resource.optimalClearValue = ZG_OPTIMAL_CLEAR_VALUE_ZERO;

	resource.screenRelativeRes = true;
	resource.resScale = scale;
	resource.resScaleSetting = scaleSetting;
	resource.resScaleSetting2 = scaleSetting2;
	resource.resScaleSettingScale = resScaleSettingScale;
	
	CHECK_ZG resource.build(screenRes);
	
	return resource;
}

TextureResource TextureResource::createSettingControlled(
	const char* name,
	ZgFormat format,
	Setting* resSetting,
	u32 numMipmaps,
	ZgTextureUsage usage,
	bool committedAllocation)
{
	sfz_assert(0 < resSetting->intValue() && resSetting->intValue() <= 16384);
	sfz_assert(numMipmaps > 0);
	sfz_assert(numMipmaps <= ZG_MAX_NUM_MIPMAPS);

	TextureResource resource;
	resource.name = sfzStrIDCreate(name);
	resource.format = format;
	resource.res = i32x2(resSetting->intValue());
	resource.numMipmaps = numMipmaps;
	resource.committedAllocation = committedAllocation;
	resource.usage = usage;
	resource.optimalClearValue =
		usage != ZG_TEXTURE_USAGE_DEFAULT ? ZG_OPTIMAL_CLEAR_VALUE_ZERO : ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED;

	resource.settingControlledRes = true;
	resource.controlledResSetting = resSetting;

	CHECK_ZG resource.build(i32x2(0));

	return resource;
}

// Texture functions
// ------------------------------------------------------------------------------------------------

ZgFormat toZeroGImageFormat(ImageType imageType) noexcept
{
	switch (imageType) {
	case ImageType::UNDEFINED: return ZG_FORMAT_UNDEFINED;
	case ImageType::R_U8: return ZG_FORMAT_R_U8_UNORM;
	case ImageType::RG_U8: return ZG_FORMAT_RG_U8_UNORM;
	case ImageType::RGBA_U8: return ZG_FORMAT_RGBA_U8_UNORM;

	case ImageType::R_F32: return ZG_FORMAT_R_F32;
	case ImageType::RG_F32: return ZG_FORMAT_RG_F32;
	case ImageType::RGBA_F32: return ZG_FORMAT_RGBA_F32;
	}
	sfz_assert(false);
	return ZG_FORMAT_UNDEFINED;
}

} // namespace sfz
