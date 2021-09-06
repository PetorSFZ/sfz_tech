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

#pragma once

#include <skipifzero.hpp>

#include <ZeroG.h>

#include "sfz/renderer/RendererState.hpp"

namespace sfz {

inline const char* textureFormatToString(ZgFormat format)
{
	switch (format) {
	case ZG_FORMAT_UNDEFINED: return "UNDEFINED";

	case ZG_FORMAT_R_U8_UNORM: return "R_U8_UNORM";
	case ZG_FORMAT_RG_U8_UNORM: return "RG_U8_UNORM";
	case ZG_FORMAT_RGBA_U8_UNORM: return "RGBA_U8_UNORM";

	case ZG_FORMAT_R_F16: return "R_F16";
	case ZG_FORMAT_RG_F16: return "RG_F16";
	case ZG_FORMAT_RGBA_F16: return "RGBA_F16";

	case ZG_FORMAT_R_F32: return "R_F32";
	case ZG_FORMAT_RG_F32: return "RG_F32";
	case ZG_FORMAT_RGBA_F32: return "RGBA_F32";

	case ZG_FORMAT_DEPTH_F32: return "DEPTH_F32";
	}
	sfz_assert(false);
	return "";
}

inline const char* usageToString(ZgTextureUsage usage)
{
	switch (usage) {
	case ZG_TEXTURE_USAGE_DEFAULT: return "DEFAULT";
	case ZG_TEXTURE_USAGE_RENDER_TARGET: return "RENDER_TARGET";
	case ZG_TEXTURE_USAGE_DEPTH_BUFFER: return "DEPTH_BUFFER";
	}
	sfz_assert(false);
	return "";
}

inline const char* clearValueToString(ZgOptimalClearValue clearValue)
{
	switch (clearValue) {
	case ZG_OPTIMAL_CLEAR_VALUE_UNDEFINED: return "UNDEFINED";
	case ZG_OPTIMAL_CLEAR_VALUE_ZERO: return "ZERO";
	case ZG_OPTIMAL_CLEAR_VALUE_ONE: return "ONE";
	}
	sfz_assert(false);
	return "";
}

inline const char* vertexAttributeTypeToString(ZgVertexAttributeType type)
{
	switch (type) {
	case ZG_VERTEX_ATTRIBUTE_F32: return "ZG_VERTEX_ATTRIBUTE_F32";
	case ZG_VERTEX_ATTRIBUTE_F32_2: return "ZG_VERTEX_ATTRIBUTE_F32_2";
	case ZG_VERTEX_ATTRIBUTE_F32_3: return "ZG_VERTEX_ATTRIBUTE_F32_3";
	case ZG_VERTEX_ATTRIBUTE_F32_4: return "ZG_VERTEX_ATTRIBUTE_F32_4";

	case ZG_VERTEX_ATTRIBUTE_S32: return "ZG_VERTEX_ATTRIBUTE_S32";
	case ZG_VERTEX_ATTRIBUTE_S32_2: return "ZG_VERTEX_ATTRIBUTE_S32_2";
	case ZG_VERTEX_ATTRIBUTE_S32_3: return "ZG_VERTEX_ATTRIBUTE_S32_3";
	case ZG_VERTEX_ATTRIBUTE_S32_4: return "ZG_VERTEX_ATTRIBUTE_S32_4";

	case ZG_VERTEX_ATTRIBUTE_U32: return "ZG_VERTEX_ATTRIBUTE_U32";
	case ZG_VERTEX_ATTRIBUTE_U32_2: return "ZG_VERTEX_ATTRIBUTE_U32_2";
	case ZG_VERTEX_ATTRIBUTE_U32_3: return "ZG_VERTEX_ATTRIBUTE_U32_3";
	case ZG_VERTEX_ATTRIBUTE_U32_4: return "ZG_VERTEX_ATTRIBUTE_U32_4";

	default: break;
	}
	sfz_assert(false);
	return "";
}

inline const char* samplingModeToString(ZgSamplingMode mode)
{
	switch (mode) {
	case ZG_SAMPLING_MODE_NEAREST: return "NEAREST";
	case ZG_SAMPLING_MODE_TRILINEAR: return "TRILINEAR";
	case ZG_SAMPLING_MODE_ANISOTROPIC: return "ANISOTROPIC";
	}
	sfz_assert(false);
	return "UNDEFINED";
}

inline const char* wrappingModeToString(ZgWrappingMode mode)
{
	switch (mode) {
	case ZG_WRAPPING_MODE_CLAMP: return "CLAMP";
	case ZG_WRAPPING_MODE_REPEAT: return "REPEAT";
	}
	sfz_assert(false);
	return "UNDEFINED";
}

inline const char* comparisonFuncToString(ZgComparisonFunc func)
{
	switch (func) {
	case ZG_COMPARISON_FUNC_NONE: return "NONE";
	case ZG_COMPARISON_FUNC_LESS: return "LESS";
	case ZG_COMPARISON_FUNC_LESS_EQUAL: return "LESS_EQUAL";
	case ZG_COMPARISON_FUNC_EQUAL: return "EQUAL";
	case ZG_COMPARISON_FUNC_NOT_EQUAL: return "NOT_EQUAL";
	case ZG_COMPARISON_FUNC_GREATER: return "GREATER";
	case ZG_COMPARISON_FUNC_GREATER_EQUAL: return "GREATER_EQUAL";
	}
	sfz_assert(false);
	return "";
}

inline const char* blendModeToString(PipelineBlendMode mode)
{
	switch (mode) {
	case PipelineBlendMode::NO_BLENDING: return "no_blending";
	case PipelineBlendMode::ALPHA_BLENDING: return "alpha_blending";
	case PipelineBlendMode::ADDITIVE_BLENDING: return "additive_blending";
	}
	sfz_assert(false);
	return "";
}

} // namespace sfz
