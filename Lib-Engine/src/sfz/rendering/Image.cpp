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

#include "sfz/rendering/Image.hpp"

#include <skipifzero_strings.hpp>

#include <sfz/Logging.hpp>

// stb_image implementation
// ------------------------------------------------------------------------------------------------

#include <sfz/PushWarnings.hpp>

static void* sfz_malloc_wrapper(size_t size);
static void* sfz_realloc_sized_wrapper(void* ptr, size_t oldSize, size_t newSize);
static void sfz_free_wrapper(void* ptr);
#define STBI_MALLOC(size) sfz_malloc_wrapper(size)
#define STBI_REALLOC_SIZED(ptr, oldSize, newSize) sfz_realloc_sized_wrapper(ptr, oldSize, newSize)
#define STBI_FREE(ptr) sfz_free_wrapper(ptr)
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>

#include <sfz/PopWarnings.hpp>

// stb_image_write implementation
// ------------------------------------------------------------------------------------------------

#include <sfz/PushWarnings.hpp>

static void* sfz_malloc_wrapper(size_t size);
static void* sfz_realloc_sized_wrapper(void* ptr, size_t oldSize, size_t newSize);
static void sfz_free_wrapper(void* ptr);
#define STBIW_MALLOC(size) sfz_malloc_wrapper(size)
#define STBIW_REALLOC_SIZED(ptr, oldSize, newSize) sfz_realloc_sized_wrapper(ptr, oldSize, newSize)
#define STBIW_FREE(ptr) sfz_free_wrapper(ptr)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include <stb_image_write.h>

#include <sfz/PopWarnings.hpp>

// C allocation wrapper for stb_image
// ------------------------------------------------------------------------------------------------

static SfzAllocator* staticAllocator = nullptr;

static void* sfz_malloc_wrapper(size_t size)
{
	return staticAllocator->alloc(sfz_dbg("stb_image"), size, 32);
}

static void* sfz_realloc_sized_wrapper(void* ptr, size_t oldSize, size_t newSize)
{
	void* newMem = sfz_malloc_wrapper(newSize);
	std::memcpy(newMem, ptr, oldSize);
	sfz_free_wrapper(ptr);
	return newMem;
}

static void sfz_free_wrapper(void* ptr)
{
	staticAllocator->dealloc(ptr);
}

namespace sfz {

using sfz::str320;

// Static helper functions
// ------------------------------------------------------------------------------------------------

static void padRgb(Array<u8>& dst, const u8* src, i32 w, i32 h) noexcept
{
	const i32 srcPitch = w * 3;
	const i32 dstPitch = w * 4;

	dst.ensureCapacity(h * dstPitch);
	dst.hackSetSize(h * dstPitch);

	for (i32 y = 0; y < h; y++) {
		i32 srcOffs = y * srcPitch;
		i32 dstOffs = y * dstPitch;

		for (i32 x = 0; x < w; x++) {
			dst[dstOffs + x * 4] = src[srcOffs + x * 3];
			dst[dstOffs + x * 4 + 1] = src[srcOffs + x * 3 + 1];
			dst[dstOffs + x * 4 + 2] = src[srcOffs + x * 3 + 2];
			dst[dstOffs + x * 4 + 3] = u8(0xFF);
		}
	}
}

static void padRgbFloat(vec4* dstImg, const vec3* srcImg, i32 w, i32 h) noexcept
{
	for (i32 y = 0; y < h; y++) {
		vec4* dstRow = dstImg + w * y;
		const vec3* srcRow = srcImg + w * y;
		for (i32 x = 0; x < w; x++) {
			dstRow[x] = vec4(srcRow[x], 1.0f);
		}
	}
}

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

// Implementations of functions from header
// ------------------------------------------------------------------------------------------------

Image Image::allocate(i32 width, i32 height, ImageType type, SfzAllocator* allocator) noexcept
{
	Image image;
	image.type = type;
	image.width = width;
	image.height = height;
	image.bytesPerPixel = sizeOfElement(type);
	image.rawData.init(width * height * image.bytesPerPixel, allocator, sfz_dbg(""));
	image.rawData.hackSetSize(image.rawData.capacity());
	std::memset(image.rawData.data(), 0, image.rawData.size());
	return image;
}

void setLoadImageAllocator(SfzAllocator* allocator)
{
	staticAllocator = allocator;
}

Image loadImage(const char* basePath, const char* fileName) noexcept
{
	// Some input error handling
	if (basePath == nullptr || fileName == nullptr) {
		SFZ_WARNING("PhantasyEngine", "Invalid path to image");
		return Image();
	}
	if (staticAllocator == nullptr) {
		SFZ_WARNING("PhantasyEngine",
			"Allocator not specified, call setLoadImageAllocator() first");
		return Image();
	}

	// Concatenate path
	str320 path("%s%s", basePath, fileName);

	// Load image
	const bool isHDR = path.endsWith(".hdr");
	int width = 0, height = 0, numChannels = 0;
	int channelSize = 0;
	u8* img = nullptr;
	if (isHDR) {
		f32* floatImg = stbi_loadf(path, &width, &height, &numChannels, 0);
		img = reinterpret_cast<u8*>(floatImg);
		channelSize = sizeof(f32);
	}
	else {
		img = stbi_load(path, &width, &height, &numChannels, 0);
		channelSize = sizeof(u8);
	}

	// Error checking
	if (img == nullptr) {
		SFZ_WARNING("PhantasyEngine", "Unable to load image \"%s\", reason: %s",
			path.str(), stbi_failure_reason());
		return Image();
	}
	if (numChannels != 1 && numChannels != 2 && numChannels != 3 && numChannels != 4) {
		SFZ_WARNING("PhantasyEngine", "Image \"%s\" has unsupported number of channels: %i",
			path.str(), numChannels);
		stbi_image_free(img);
		return Image();
	}

	// Create image from data
	const u32 numBytes = u32(width * height * numChannels * channelSize);
	Image tmp;
	tmp.rawData.init(numBytes, staticAllocator, sfz_dbg(""));
	tmp.width = width;
	tmp.height = height;
	tmp.bytesPerPixel = numChannels * channelSize;
	if (isHDR) {
		sfz_assert_hard(numChannels == 3);
		tmp.rawData.add(u8(0), u32(width * height * sizeof(vec4)));
		padRgbFloat(
			reinterpret_cast<vec4*>(tmp.rawData.data()),
			reinterpret_cast<const vec3*>(img),
			width,
			height);
		tmp.type = ImageType::RGBA_F32;
		tmp.bytesPerPixel = 4 * channelSize;
	}
	else {
		switch (numChannels) {
		case 1:
			tmp.rawData.add(img, numBytes);
			tmp.type = ImageType::R_U8;
			break;
		case 2:
			tmp.rawData.add(img, numBytes);
			tmp.type = ImageType::RG_U8;
			break;
		case 3:
			padRgb(tmp.rawData, img, width, height);
			tmp.type = ImageType::RGBA_U8;
			tmp.bytesPerPixel = 4 * channelSize;
			break;
		case 4:
			tmp.rawData.add(img, numBytes);
			tmp.type = ImageType::RGBA_U8;
			break;
		default:
			sfz_assert_hard(false);
		}
	}

	// Free temp memory used by stb_image and return image
	SFZ_NOISE("PhantasyEngine", "Image \"%s\" loaded succesfully", path.str());
	stbi_image_free(img);
	return tmp;
}

void flipVertically(Image& image, SfzAllocator* allocator) noexcept
{
	sfz_assert(image.rawData.data() != nullptr);
	sfz_assert((image.height % 2) == 0);

	i32 pitch = image.width * image.bytesPerPixel;
	u8* buffer = (u8*)allocator->alloc(sfz_dbg(""), u64(pitch), 32);

	for (i32 i = 0; i < (image.height / 2); i++) {
		u8* begin = image.rawData.data() + i * pitch;
		u8* end = image.rawData.data() + (image.height - i - 1) * pitch;

		std::memcpy(buffer, begin, pitch);
		std::memcpy(begin, end, pitch);
		std::memcpy(end, buffer, pitch);
	}

	allocator->dealloc(buffer);
}

bool saveImagePng(const Image& image, const char* path) noexcept
{
	sfz_assert(image.rawData.data() != nullptr);
	sfz_assert(image.width > 0);
	sfz_assert(image.height > 0);

	int res = stbi_write_png(
		path, image.width, image.height, image.bytesPerPixel, image.rawData.data(), 0);

	return res != 0;
}

} // namespace sfz
