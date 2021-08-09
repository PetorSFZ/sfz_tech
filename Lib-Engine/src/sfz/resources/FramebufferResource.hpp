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

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

namespace sfz {

class Setting;

// FramebufferResource
// ------------------------------------------------------------------------------------------------

struct FramebufferResource final {
	strID name;

	zg::Framebuffer framebuffer;
	ArrayLocal<strID, ZG_MAX_NUM_RENDER_TARGETS> renderTargetNames;
	strID depthBufferName;
	i32x2 res = i32x2(0);

	// Whether resolution should be scaled relative screen resolution
	bool screenRelativeResolution = false;
	f32 resolutionScale = 1.0f;
	Setting* resolutionScaleSetting = nullptr;

	// Whether resolution is directly controlled by a setting
	bool settingControlledRes = false;
	Setting* controlledResSetting = nullptr;

	[[nodiscard]] ZgResult build(i32x2 screenRes);
};

// FramebufferResourceBuilder
// ------------------------------------------------------------------------------------------------

struct FramebufferResourceBuilder final {
	str128 name;
	ArrayLocal<strID, ZG_MAX_NUM_RENDER_TARGETS> renderTargetNames;
	strID depthBufferName;
	i32x2 res = i32x2(0);

	bool screenRelativeResolution = false;
	f32 resolutionScale = 1.0f;
	Setting* resolutionScaleSetting = nullptr;

	bool settingControlledRes = false;
	Setting* controlledResSetting = nullptr;

	FramebufferResourceBuilder() = default;
	FramebufferResourceBuilder(const char* name) : name("%s", name) {}

	FramebufferResourceBuilder& setName(const char* name);
	FramebufferResourceBuilder& setFixedRes(i32x2 res);
	FramebufferResourceBuilder& setScreenRelativeRes(f32 scale);
	FramebufferResourceBuilder& setScreenRelativeRes(Setting* scaleSetting);
	FramebufferResourceBuilder& setSettingControlledRes(Setting* resSetting);
	FramebufferResourceBuilder& addRenderTarget(const char* textureName);
	FramebufferResourceBuilder& addRenderTarget(strID textureName);
	FramebufferResourceBuilder& setDepthBuffer(const char* textureName);
	FramebufferResourceBuilder& setDepthBuffer(strID textureName);
	FramebufferResource build(i32x2 screenRes);
};

} // namespace sfz
