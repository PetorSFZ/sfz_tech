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

#include <sfz/containers/DynArray.hpp>
#include <sfz/memory/Allocator.hpp>

#include <ph/rendering/ImageView.h>

namespace ph {

using sfz::Allocator;
using sfz::DynArray;

// Image struct
// ------------------------------------------------------------------------------------------------

struct Image final {
	DynArray<uint8_t> rawData;
	ImageType type = ImageType::UNDEFINED;
	int32_t width = -1;
	int32_t height = -1;
	int32_t bytesPerPixel = -1;

	inline ImageView toImageView() noexcept;
	inline ConstImageView toImageView() const noexcept;
	inline operator ImageView() noexcept;
	inline operator ConstImageView() const noexcept;
};

// Image functions
// ------------------------------------------------------------------------------------------------

/// Sets the allocator used for stb_image and the output image from loadImage().
/// This function should ONLY be called if no loadImage() calls is under process, otherwise
/// dangerous race conditions can happen.
void setLoadImageAllocator(Allocator* allocator);

/// Loads an image using stb_image.
///
/// Images must be in 8-bit gray, RGB or RGBA format. RGB images will be padded to RGBA (alpha
/// channel will be set to 0xFF).
Image loadImage(const char* basePath, const char* fileName) noexcept;

// Flips an image vertically, i.e. the top row will be the bottom row, etc.
//
// Allocates a temporary buffer of the same width as the image
void flipVertically(Image& image, Allocator* allocator) noexcept;

// Image struct implementation
// ------------------------------------------------------------------------------------------------

inline ImageView Image::toImageView() noexcept
{
	ImageView tmp;
	tmp.rawData = this->rawData.data();
	tmp.type = this->type;
	tmp.width = this->width;
	tmp.height = this->height;
	tmp.bytesPerPixel = this->bytesPerPixel;
	return tmp;
}

inline ConstImageView Image::toImageView() const noexcept
{
	ConstImageView tmp;
	tmp.rawData = this->rawData.data();
	tmp.type = this->type;
	tmp.width = this->width;
	tmp.height = this->height;
	tmp.bytesPerPixel = this->bytesPerPixel;
	return tmp;
}

inline Image::operator ImageView() noexcept
{
	return this->toImageView();
}

inline Image::operator ConstImageView() const noexcept
{
	return this->toImageView();
}

} // namespacep ph
