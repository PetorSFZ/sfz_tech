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
#else
#include <stdint.h>
#endif

#include "ph/ExternC.h"

// Image type constants
// ------------------------------------------------------------------------------------------------

const uint32_t PH_IMAGE_UNDEFINED = 0;
const uint32_t PH_IMAGE_R_U8 = 1;
const uint32_t PH_IMAGE_RG_U8 = 2;
const uint32_t PH_IMAGE_RGBA_U8 = 3;
const uint32_t PH_IMAGE_R_F32 = 4;
const uint32_t PH_IMAGE_RG_F32 = 5;
const uint32_t PH_IMAGE_RGBA_F32 = 6;

// ImageView structs (C)
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

// ImageView structs (C++)
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace ph {

enum class ImageType : uint32_t {
	UNDEFINED = PH_IMAGE_UNDEFINED,
	R_U8 = PH_IMAGE_R_U8,
	RG_U8 = PH_IMAGE_RG_U8,
	RGBA_U8 = PH_IMAGE_RGBA_U8,
	R_F32 = PH_IMAGE_R_F32,
	RG_F32 = PH_IMAGE_RG_F32,
	RGBA_F32 = PH_IMAGE_RGBA_F32
};

struct ImageView final {
	uint8_t* rawData = nullptr;
	ImageType type = ImageType::UNDEFINED;
	int32_t width = 0;
	int32_t height = 0;
	int32_t bytesPerPixel = 0;

	// Pixel accessors
	inline uint8_t* pixelPtr(int32_t x, int32_t y) noexcept;

	template<typename T>
	T& at(int32_t x, int32_t y) noexcept;

	// Implicit conversions
	ImageView() noexcept = default;
	inline ImageView(const phImageView& view) noexcept;
	inline operator phImageView() const noexcept;
	inline operator phConstImageView() const noexcept;
};
static_assert(sizeof(ph::ImageView) == sizeof(phImageView), "Padded");

struct ConstImageView final {
	const uint8_t* rawData = nullptr;
	ImageType type = ImageType::UNDEFINED;
	int32_t width = 0;
	int32_t height = 0;
	int32_t bytesPerPixel = 0;

	// Pixel accessors
	inline const uint8_t* pixelPtr(int32_t x, int32_t y) const noexcept;

	template<typename T>
	const T& at(int32_t x, int32_t y) const noexcept;

	// Implicit conversions
	ConstImageView() noexcept = default;
	inline ConstImageView(const phImageView& view) noexcept;
	inline ConstImageView(const phConstImageView& view) noexcept;
	inline ConstImageView(const ImageView& view) noexcept;
	inline operator phConstImageView() const noexcept;
};
static_assert(sizeof(ph::ConstImageView) == sizeof(phConstImageView), "Padded");

inline uint8_t* ImageView::pixelPtr(int32_t x, int32_t y) noexcept
{
	return rawData + y * width * bytesPerPixel + x * bytesPerPixel;
}

template<typename T>
T& ImageView::at(int32_t x, int32_t y) noexcept
{
	return *reinterpret_cast<T*>(this->pixelPtr(x, y));
}

inline ImageView::ImageView(const phImageView& view) noexcept
:
	rawData(view.rawData),
	type(static_cast<ImageType>(view.type)),
	width(view.width),
	height(view.height),
	bytesPerPixel(view.bytesPerPixel)
{ }

inline ImageView::operator phImageView() const noexcept
{
	phImageView tmp;
	tmp.rawData = this->rawData;
	tmp.type = uint32_t(this->type);
	tmp.width = this->width;
	tmp.height = this->height;
	tmp.bytesPerPixel = this->bytesPerPixel;
	return tmp;
}

inline ImageView::operator phConstImageView() const noexcept
{
	phConstImageView tmp;
	tmp.rawData = this->rawData;
	tmp.type = uint32_t(this->type);
	tmp.width = this->width;
	tmp.height = this->height;
	tmp.bytesPerPixel = this->bytesPerPixel;
	return tmp;
}

inline const uint8_t* ConstImageView::pixelPtr(int32_t x, int32_t y) const noexcept
{
	return rawData + y * width * bytesPerPixel + x * bytesPerPixel;
}

template<typename T>
const T& ConstImageView::at(int32_t x, int32_t y) const noexcept
{
	return *reinterpret_cast<const T*>(this->pixelPtr(x, y));
}

inline ConstImageView::ConstImageView(const phImageView& view) noexcept
:
	ConstImageView(ImageView(view))
{ }

inline ConstImageView::ConstImageView(const phConstImageView& view) noexcept
:
	rawData(view.rawData),
	type(static_cast<ImageType>(view.type)),
	width(view.width),
	height(view.height),
	bytesPerPixel(view.bytesPerPixel)
{ }

inline ConstImageView::ConstImageView(const ImageView& view) noexcept
:
	rawData(view.rawData),
	type(view.type),
	width(view.width),
	height(view.height),
	bytesPerPixel(view.bytesPerPixel)
{ }

inline ConstImageView::operator phConstImageView() const noexcept
{
	phConstImageView tmp;
	tmp.rawData = this->rawData;
	tmp.type = uint32_t(this->type);
	tmp.width = this->width;
	tmp.height = this->height;
	tmp.bytesPerPixel = this->bytesPerPixel;
	return tmp;
}

} // namespace ph

#endif
