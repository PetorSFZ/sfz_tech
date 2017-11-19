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

#ifdef __cplusplus
#include <cstdint>
using std::uint8_t;
using std::uint32_t;
using std::int32_t;
#include <sfz/Assert.hpp>
#include <sfz/containers/DynArray.hpp>
#else
#include <stdint.h>
#endif

#include "ph/ExternC.h"

// Image type constants
// ------------------------------------------------------------------------------------------------

const uint32_t PH_IMAGE_GRAY_U8 = 0;
const uint32_t PH_IMAGE_RGBA_U8 = 1;
const uint32_t PH_IMAGE_GRAY_F32 = 2;
const uint32_t PH_IMAGE_RGBA_F32 = 3;
const uint32_t PH_IMAGE_UNDEFINED = ~0u;

// ImageView structs (C-compatible)
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	uint8_t* rawData;
	uint32_t type;
	int32_t width;
	int32_t height;
	int32_t bytesPerPixel;
} phImageView;

PH_EXTERN_C
typedef struct {
	const uint8_t* rawData;
	uint32_t type;
	int32_t width;
	int32_t height;
	int32_t bytesPerPixel;
} phConstImageView;

// C++ only constructs
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace ph {

using sfz::DynArray;

// Typed enum version of image type constants
// ------------------------------------------------------------------------------------------------

enum class ImageType : uint32_t {
	GRAY_U8 = PH_IMAGE_GRAY_U8,
	RGBA_U8 = PH_IMAGE_RGBA_U8,
	GRAY_F32 = PH_IMAGE_GRAY_F32,
	RGBA_F32 = PH_IMAGE_RGBA_F32,
	UNDEFINED = PH_IMAGE_UNDEFINED
};

// C++ image struct
// ------------------------------------------------------------------------------------------------

struct Image final {
	DynArray<uint8_t> rawData;
	ImageType type = ImageType::UNDEFINED;
	int32_t width = -1;
	int32_t height = -1;
	int32_t bytesPerPixel = -1;

	inline uint8_t* pixelPtr(int32_t x, int32_t y) noexcept;
	inline const uint8_t* pixelPtr(int32_t x, int32_t y) const noexcept;

	template<typename T>
	T& at(int32_t x, int32_t y) noexcept;

	template<typename T>
	const T& at(int32_t x, int32_t y) const noexcept;

	inline phImageView imageView() noexcept;
	inline phConstImageView imageView() const noexcept;
};

inline uint8_t* Image::pixelPtr(int32_t x, int32_t y) noexcept
{
	sfz_assert_debug(rawData.data() != nullptr);
	sfz_assert_debug(0 <= x && x < width);
	sfz_assert_debug(0 <= y && y < height);
	sfz_assert_debug(1 <= bytesPerPixel && bytesPerPixel <= 4);
	return this->rawData.data() + y * width * bytesPerPixel + x * bytesPerPixel;
}

inline const uint8_t* Image::pixelPtr(int32_t x, int32_t y) const noexcept
{
	sfz_assert_debug(rawData.data() != nullptr);
	sfz_assert_debug(0 <= x && x < width);
	sfz_assert_debug(0 <= y && y < height);
	sfz_assert_debug(1 <= bytesPerPixel && bytesPerPixel <= 4);
	return this->rawData.data() + y * width * bytesPerPixel + x * bytesPerPixel;
}

template<typename T>
T& Image::at(int32_t x, int32_t y) noexcept
{
	return *reinterpret_cast<T*>(this->pixelPtr(x, y));
}

template<typename T>
const T& Image::at(int32_t x, int32_t y) const noexcept
{
	return *reinterpret_cast<const T*>(this->pixelPtr(x, y));
}

inline phImageView Image::imageView() noexcept
{
	phImageView tmp;
	tmp.rawData = this->rawData.data();
	tmp.type = uint32_t(this->type);
	tmp.width = this->width;
	tmp.height = this->height;
	tmp.bytesPerPixel = this->bytesPerPixel;
	return tmp;
}

inline phConstImageView Image::imageView() const noexcept
{
	phConstImageView tmp;
	tmp.rawData = this->rawData.data();
	tmp.type = uint32_t(this->type);
	tmp.width = this->width;
	tmp.height = this->height;
	tmp.bytesPerPixel = this->bytesPerPixel;
	return tmp;
}

} // namespace ph

#endif
