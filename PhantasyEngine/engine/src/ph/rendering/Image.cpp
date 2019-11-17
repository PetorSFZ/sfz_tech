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

#include "ph/rendering/Image.hpp"

#include <cstring>

#include <sfz/Logging.hpp>
#include <sfz/strings/StackString.hpp>

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

static sfz::Allocator* static_allocator = nullptr;

static void* sfz_malloc_wrapper(size_t size)
{
	return static_allocator->allocate(sfz_dbg("stb_image"), size, 32);
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
	static_allocator->deallocate(ptr);
}

namespace ph {

using sfz::StackString320;

// Static helper functions
// ------------------------------------------------------------------------------------------------

static void padRgb(DynArray<uint8_t>& dst, const uint8_t* src, int32_t w, int32_t h) noexcept
{
	const int32_t srcPitch = w * 3;
	const int32_t dstPitch = w * 4;

	dst.ensureCapacity(h * dstPitch);
	dst.hackSetSize(h * dstPitch);

	for (int32_t y = 0; y < h; y++) {
		int32_t srcOffs = y * srcPitch;
		int32_t dstOffs = y * dstPitch;

		for (int32_t x = 0; x < w; x++) {
			dst[dstOffs + x * 4] = src[srcOffs + x * 3];
			dst[dstOffs + x * 4 + 1] = src[srcOffs + x * 3 + 1];
			dst[dstOffs + x * 4 + 2] = src[srcOffs + x * 3 + 2];
			dst[dstOffs + x * 4 + 3] = uint8_t(0xFF);
		}
	}
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
	sfz_assert(false);
	return 0;
}

// Implementations of functions from header
// ------------------------------------------------------------------------------------------------

Image Image::allocate(int32_t width, int32_t height, ImageType type, Allocator* allocator) noexcept
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

void setLoadImageAllocator(Allocator* allocator)
{
	static_allocator = allocator;
}

Image loadImage(const char* basePath, const char* fileName) noexcept
{
	// Some input error handling
	if (basePath == nullptr || fileName == nullptr) {
		SFZ_WARNING("PhantasyEngine", "Invalid path to image");
		return Image();
	}
	if (static_allocator == nullptr) {
		SFZ_WARNING("PhantasyEngine",
			"Allocator not specified, call setLoadImageAllocator() first");
		return Image();
	}

	// Concatenate path
	StackString320 path;
	path.printf("%s%s", basePath, fileName);

	// Loading image
	int width, height, numChannels;
	uint8_t* img = stbi_load(path.str, &width, &height, &numChannels, 0);

	// Error checking
	if (img == nullptr) {
		SFZ_WARNING("PhantasyEngine", "Unable to load image \"%s\", reason: %s",
			path.str, stbi_failure_reason());
		return Image();
	}
	if (numChannels != 1 && numChannels != 2 && numChannels != 3 && numChannels != 4) {
		SFZ_WARNING("PhantasyEngine", "Image \"%s\" has unsupported number of channels: %i",
			path.str, numChannels);
		stbi_image_free(img);
		return Image();
	}

	// Create image from data
	Image tmp;
	tmp.rawData.init(uint32_t(width * height * numChannels), static_allocator, sfz_dbg(""));
	tmp.width = width;
	tmp.height = height;
	switch (numChannels) {
	case 1:
		tmp.rawData.add(img, uint32_t(width * height * numChannels));
		tmp.type = ImageType::R_U8;
		tmp.bytesPerPixel = 1;
		break;

	case 2:
		tmp.rawData.add(img, uint32_t(width * height * numChannels));
		tmp.type = ImageType::RG_U8;
		tmp.bytesPerPixel = 2;
		break;

	case 3:
		padRgb(tmp.rawData, img, width, height);
		tmp.type = ImageType::RGBA_U8;
		tmp.bytesPerPixel = 4;
		break;

	case 4:
		tmp.rawData.add(img, uint32_t(width * height * numChannels));
		tmp.type = ImageType::RGBA_U8;
		tmp.bytesPerPixel = 4;
		break;

	default:
		// Should never happen
		sfz_assert_hard(false);
	}

	// Free temp memory used by stb_image and return image
	SFZ_INFO_NOISY("PhantasyEngine", "Image \"%s\" loaded succesfully", path.str);
	stbi_image_free(img);
	return tmp;
}

void flipVertically(Image& image, Allocator* allocator) noexcept
{
	sfz_assert(image.rawData.data() != nullptr);
	sfz_assert((image.height % 2) == 0);

	int32_t pitch = image.width * image.bytesPerPixel;
	uint8_t* buffer = (uint8_t*)allocator->allocate(sfz_dbg(""), uint64_t(pitch), 32);

	for (int32_t i = 0; i < (image.height / 2); i++) {
		uint8_t* begin = image.rawData.data() + i * pitch;
		uint8_t* end = image.rawData.data() + (image.height - i - 1) * pitch;

		std::memcpy(buffer, begin, pitch);
		std::memcpy(begin, end, pitch);
		std::memcpy(end, buffer, pitch);
	}

	allocator->deallocate(buffer);
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

} // namespace ph
