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

#ifndef SFZ_IMAGE_VIEW_H
#define SFZ_IMAGE_VIEW_H
#pragma once

#include "sfz.h"

// Image type enum
// ------------------------------------------------------------------------------------------------

typedef enum SfzImageType {
	SFZ_IMAGE_TYPE_UNDEFINED = 0,

	SFZ_IMAGE_TYPE_R_U8 = 1,
	SFZ_IMAGE_TYPE_RG_U8 = 2,
	SFZ_IMAGE_TYPE_RGBA_U8 = 3,

	SFZ_IMAGE_TYPE_R_F32 = 4,
	SFZ_IMAGE_TYPE_RG_F32 = 5,
	SFZ_IMAGE_TYPE_RGBA_F32 = 6,

	SFZ_IMAGE_TYPE_FORCE_I32 = I32_MAX
} SfzImageType;

// ImageView structs
// ------------------------------------------------------------------------------------------------

sfz_struct(SfzImageViewConst) {
	const u8* rawData;
	SfzImageType type;
	i32x2 res;

#ifdef __cplusplus
	template<typename T>
	const T* rowPtr(i32 y) const { return reinterpret_cast<const T*>(rawData) + res.x * y; }

	template<typename T>
	const T* at(i32 x, i32 y) const { return this->rowPtr<T>(y) + x; }
#endif
};

sfz_struct(SfzImageView) {
	u8* rawData;
	SfzImageType type;
	i32x2 res;

#ifdef __cplusplus
	template<typename T>
	T* rowPtr(i32 y) { return reinterpret_cast<T*>(rawData) + res.x * y; }

	template<typename T>
	T* at(i32 x, i32 y) { return this->rowPtr<T>(y) + x; }

	// Implicit conversion to const view
	operator SfzImageViewConst() const
	{
		SfzImageViewConst cview = {};
		cview.rawData = rawData;
		cview.type = type;
		cview.res = res;
		return cview;
	}
#endif
};

#endif
