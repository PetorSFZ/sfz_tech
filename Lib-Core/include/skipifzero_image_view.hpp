// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#ifndef SKIPIFZERO_IMAGE_VIEW_HPP
#define SKIPIFZERO_IMAGE_VIEW_HPP
#pragma once

#include "skipifzero.hpp"

namespace sfz {

// Image type enum
// ------------------------------------------------------------------------------------------------

enum class ImageType : u32 {
	UNDEFINED = 0,

	R_U8 = 1,
	RG_U8 = 2,
	RGBA_U8 = 3,
	
	R_F32 = 4,
	RG_F32 = 5,
	RGBA_F32 = 6
};

// ImageView structs
// ------------------------------------------------------------------------------------------------

struct ImageView {
	u8* rawData = nullptr;
	ImageType type = ImageType::UNDEFINED;
	i32 width = 0;
	i32 height = 0;

	template<typename T>
	T* rowPtr(i32 y) noexcept
	{
		return reinterpret_cast<T*>(rawData) + width * y;
	}

	template<typename T>
	T* at(i32 x, i32 y) noexcept { return this->rowPtr<T>(y) + x; }
};

struct ImageViewConst {
	const u8* rawData = nullptr;
	ImageType type = ImageType::UNDEFINED;
	i32 width = 0;
	i32 height = 0;

	template<typename T>
	const T* rowPtr(i32 y) noexcept
	{
		return reinterpret_cast<const T*>(rawData) + width * y;
	}

	template<typename T>
	const T* at(i32 x, i32 y) noexcept { return this->rowPtr<T>(y) + x; }

	// Implicit conversion from ImageView
	ImageViewConst() noexcept = default;
	ImageViewConst(const ImageView& view) noexcept
	{
		this->rawData = view.rawData;
		this->type = view.type;
		this->width = view.width;
		this->height = view.height;
	}
};

} // namespace sfz

#endif
