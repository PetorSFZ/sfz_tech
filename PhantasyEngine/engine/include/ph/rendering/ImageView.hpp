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

#include <cstdint>

// Image type enum
// ------------------------------------------------------------------------------------------------

enum class ImageType : uint32_t {
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

struct phImageView {
	uint8_t* rawData = nullptr;
	ImageType type = ImageType::UNDEFINED;
	int32_t width = 0;
	int32_t height = 0;

	template<typename T>
	T* rowPtr(int32_t y) noexcept
	{
		return reinterpret_cast<T*>(rawData) + width * y;
	}

	template<typename T>
	T* at(int32_t x, int32_t y) noexcept { return this->rowPtr<T>(y) + x; }
};

struct phConstImageView {
	const uint8_t* rawData = nullptr;
	ImageType type = ImageType::UNDEFINED;
	int32_t width = 0;
	int32_t height = 0;

	template<typename T>
	const T* rowPtr(int32_t y) noexcept
	{
		return reinterpret_cast<const T*>(rawData) + width * y;
	}

	template<typename T>
	const T* at(int32_t x, int32_t y) noexcept { return this->rowPtr<T>(y) + x; }

	// Implicit conversion from phImageView
	phConstImageView() noexcept = default;
	phConstImageView(const phImageView& view) noexcept
	{
		this->rawData = view.rawData;
		this->type = view.type;
		this->width = view.width;
		this->height = view.height;
	}
};
