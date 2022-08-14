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

#include <sfz.h>
#include <skipifzero_arrays.hpp>

namespace sfz {

// Image struct
// ------------------------------------------------------------------------------------------------

struct Image final {
	SfzArray<u8> rawData;
	SfzImageType type = SFZ_IMAGE_TYPE_UNDEFINED;
	i32x2 res = i32x2_splat(-1);
	i32 bytesPerPixel = -1;

	static Image allocate(
		i32 width, i32 height, SfzImageType type, SfzAllocator* allocator) noexcept;

	inline SfzImageView toImageView()
	{
		SfzImageView tmp = {};
		tmp.rawData = this->rawData.data();
		tmp.type = this->type;
		tmp.res = this->res;
		return tmp;
	}
	inline SfzImageViewConst toImageView() const
	{
		SfzImageViewConst tmp = {};
		tmp.rawData = this->rawData.data();
		tmp.type = this->type;
		tmp.res = this->res;
		return tmp;
	}
	operator SfzImageView() { return this->toImageView(); }
	operator SfzImageViewConst() const { return this->toImageView(); }
};

// Image functions
// ------------------------------------------------------------------------------------------------

// Sets the allocator used for stb_image and the output image from loadImage().
// This function should ONLY be called if no loadImage() calls is under process, otherwise
// dangerous race conditions can happen.
void setLoadImageAllocator(SfzAllocator* allocator);

// Loads an image using stb_image.
//
// Images must be in 8-bit gray, RGB or RGBA format. RGB images will be padded to RGBA (alpha
// channel will be set to 0xFF).
Image loadImage(const char* basePath, const char* fileName) noexcept;

// Flips an image vertically, i.e. the top row will be the bottom row, etc.
//
// Allocates a temporary buffer of the same width as the image
void flipVertically(Image& image, SfzAllocator* allocator) noexcept;

// Saves to file using stb_image_write
bool saveImagePng(const Image& image, const char* path) noexcept;

} // namespacep ph
