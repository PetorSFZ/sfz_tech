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

#include "ph/renderer/GpuTextures.hpp"

#include <algorithm>

#include <sfz/Assert.hpp>
#include <sfz/math/Vector.hpp>

#include "ph/renderer/ZeroGUtils.hpp"
#include "ph/rendering/Image.hpp"

namespace ph {

using sfz::vec2_u8;
using sfz::vec2_u32;
using sfz::vec4_u8;
using sfz::vec4_u32;

// Statics
// ------------------------------------------------------------------------------------------------

static constexpr bool isPowerOfTwo(uint32_t value) noexcept
{
	return value != 0 && (value & (value - 1)) == 0;
}

static uint32_t sizeOfElement(ImageType imageType) noexcept
{
	switch (imageType) {
	case ImageType::UNDEFINED: return 0;
	case ImageType::R_U8: return 1 * sizeof(uint8_t);
	case ImageType::RG_U8: return 2 * sizeof(uint8_t);
	case ImageType::RGBA_U8: return 4 * sizeof(uint8_t);

	case ImageType::R_F32: return 1 * sizeof(float);
	case ImageType::RG_F32: return 2 * sizeof(float);
	case ImageType::RGBA_F32: return 4 * sizeof(float);
	}
	sfz_assert_debug(false);
	return 0;
}

static ZgImageViewConstCpu toZeroGImageView(const phConstImageView& phView) noexcept
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
	const phConstImageView& prevLevel, Image& currLevel, Averager averager) noexcept
{
	const T* srcImg = reinterpret_cast<const T*>(prevLevel.rawData);
	T* dstImg = reinterpret_cast<T*>(currLevel.rawData.data());
	for (uint32_t y = 0; y < currLevel.height; y++) {
		for (uint32_t x = 0; x < currLevel.width; x++) {

			T* dstPixelPtr = dstImg + y * currLevel.width + x;
			const T* srcPixelPtrRow0 = srcImg + ((y * 2) + 0) * prevLevel.width + (x * 2);
			const T* srcPixelPtrRow1 = srcImg + ((y * 2) + 1) * prevLevel.width + (x * 2);

			*dstPixelPtr = averager(
				srcPixelPtrRow0[0], srcPixelPtrRow0[1],
				srcPixelPtrRow1[0], srcPixelPtrRow1[1]);
		}
	}
}

// TODO: This is sort of bad because:
// a) We should not downscale in gamma space, but in linear space
// b) We should probably do something smarter than naive averaging
// c) We should not read from previous level, but from the original level when calculating a
//    specific level.
static void generateMipmap(const phConstImageView& prevLevel, Image& currLevel) noexcept
{
	sfz_assert_debug(prevLevel.type == currLevel.type);
	switch (currLevel.type) {
	case ImageType::R_U8:
		generateMipmapSpecific<uint8_t>(prevLevel, currLevel, [](uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
			return uint8_t((uint32_t(a) + uint32_t(b) + uint32_t(c) + uint32_t(d)) / 4u);
		});
		break;
	case ImageType::RG_U8:
		generateMipmapSpecific<vec2_u8>(prevLevel, currLevel, [](vec2_u8 a, vec2_u8 b, vec2_u8 c, vec2_u8 d) {
			return vec2_u8((vec2_u32(a) + vec2_u32(b) + vec2_u32(c) + vec2_u32(d)) / 4u);
		});
		break;
	case ImageType::RGBA_U8:
		generateMipmapSpecific<vec4_u8>(prevLevel, currLevel, [](vec4_u8 a, vec4_u8 b, vec4_u8 c, vec4_u8 d) {
			return vec4_u8((vec4_u32(a) + vec4_u32(b) + vec4_u32(c) + vec4_u32(d)) / 4u);
		});
		break;
	
	case ImageType::UNDEFINED:
	case ImageType::R_F32:
	case ImageType::RG_F32:
	case ImageType::RGBA_F32:
	default:
		sfz_assert_release(false);
		break;
	};
}

// Texture functions
// ------------------------------------------------------------------------------------------------

ZgTexture2DFormat toZeroGImageFormat(ImageType imageType) noexcept
{
	switch (imageType) {
	case ImageType::UNDEFINED: return ZG_TEXTURE_2D_FORMAT_UNDEFINED;
	case ImageType::R_U8: return ZG_TEXTURE_2D_FORMAT_R_U8;
	case ImageType::RG_U8: return ZG_TEXTURE_2D_FORMAT_RG_U8;
	case ImageType::RGBA_U8: return ZG_TEXTURE_2D_FORMAT_RGBA_U8;

	case ImageType::R_F32:
	case ImageType::RG_F32:
	case ImageType::RGBA_F32:
		// TODO: Implement
		break;
	}
	sfz_assert_debug(false);
	return ZG_TEXTURE_2D_FORMAT_UNDEFINED;
}

zg::Texture2D textureAllocateAndUploadBlocking(
	const phConstImageView& image,
	DynamicGpuAllocator& gpuAllocator,
	sfz::Allocator* cpuAllocator,
	zg::CommandQueue& copyQueue,
	bool generateMipmaps,
	uint32_t& numMipmapsOut) noexcept
{
	sfz_assert_debug(isPowerOfTwo(image.width));
	sfz_assert_debug(isPowerOfTwo(image.height));

	// Convert to ZeroG Image View
	ZgImageViewConstCpu view = toZeroGImageView(image);

	// Calculate number of mipmaps if requested
	uint32_t numMipmaps = 1;
	if (generateMipmaps) {
		uint32_t logWidth = uint32_t(log2(image.width));
		uint32_t logHeight = uint32_t(log2(image.width));
		uint32_t logMin = std::min(logWidth, logHeight);
		numMipmaps = std::min(logMin, (ZG_TEXTURE_2D_MAX_NUM_MIPMAPS - 1));
	}

	// Allocate Texture
	uint32_t textureSizeBytes = 0;
	zg::Texture2D texture = gpuAllocator.allocateTexture2D(
		view.format, view.width, view.height, numMipmaps, &textureSizeBytes);
	sfz_assert_debug(texture.valid());
	sfz_assert_debug(textureSizeBytes != 0);
	if (!texture.valid()) return zg::Texture2D();

	// Generate mipmaps (on CPU)
	sfz_assert_debug(numMipmaps != 0);
	Image mipmaps[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS - 1];
	for (uint32_t i = 0; i < (numMipmaps - 1); i++) {
		
		// Get previous mipmap level
		phConstImageView prevLevel;
		if (i == 0) prevLevel = image;
		else prevLevel = mipmaps[i - 1];

		// Allocate mipmap memory
		mipmaps[i] = Image::allocate(
			prevLevel.width / 2, prevLevel.height / 2, prevLevel.type, cpuAllocator);

		// Generate mipmap
		generateMipmap(prevLevel, mipmaps[i]);
	}

	// Create image views
	ZgImageViewConstCpu imageViews[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS] = {};
	imageViews[0] = view;
	for (uint32_t i = 0; i < (numMipmaps - 1); i++) {
		imageViews[i + 1] = toZeroGImageView(mipmaps[i]);
	}

	// Allocate temporary upload buffers
	zg::Buffer tmpUploadBuffers[ZG_TEXTURE_2D_MAX_NUM_MIPMAPS];
	for (uint32_t i = 0; i < numMipmaps; i++) {
		// TODO: Figure out exactly how much memory is needed
		uint32_t bufferSize = (imageViews[i].pitchInBytes * imageViews[i].height) + 1024;
		tmpUploadBuffers[i] = gpuAllocator.allocateBuffer(ZG_MEMORY_TYPE_UPLOAD, bufferSize);
		sfz_assert_debug(tmpUploadBuffers[i].valid());
	}

	// Copy texture to GPU
	zg::CommandList commandList;
	CHECK_ZG copyQueue.beginCommandListRecording(commandList);
	for (uint32_t i = 0; i < numMipmaps; i++) {
		CHECK_ZG commandList.memcpyToTexture(texture, i, imageViews[i], tmpUploadBuffers[i]);
	}
	CHECK_ZG commandList.enableQueueTransition(texture);
	CHECK_ZG copyQueue.executeCommandList(commandList);
	CHECK_ZG copyQueue.flush();

	// Deallocate temporary upload buffers
	for (uint32_t i = 0; i < numMipmaps; i++) {
		gpuAllocator.deallocate(tmpUploadBuffers[i]);
	}

	numMipmapsOut = numMipmaps;
	return std::move(texture);
}

} // namespace ph
