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
#include <skipifzero_arrays.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

struct SfzResourceManager;
struct SfzSetting;
struct SfzStrIDs;

// SfzFramebufferResource
// ------------------------------------------------------------------------------------------------

struct SfzFramebufferResource final {
	SfzStrID name = SFZ_NULL_STR_ID;

	zg::Framebuffer framebuffer;
	SfzArrayLocal<SfzStrID, ZG_MAX_NUM_RENDER_TARGETS> renderTargetNames;
	SfzStrID depthBufferName = SFZ_NULL_STR_ID;
	i32x2 res = i32x2_splat(0);

	// Whether resolution should be scaled relative screen resolution
	bool screenRelativeRes = false;
	f32 resScale = 1.0f;
	SfzSetting* resScaleSetting = nullptr;
	SfzSetting* resScaleSetting2 = nullptr;

	// Whether resolution is directly controlled by a setting
	bool settingControlledRes = false;
	SfzSetting* controlledResSetting = nullptr;

	[[nodiscard]] ZgResult build(i32x2 screenRes, SfzStrIDs* ids, SfzResourceManager* resMan);
};

// SfzFramebufferResourceBuilder
// ------------------------------------------------------------------------------------------------

struct SfzFramebufferResourceBuilder final {
	SfzStr96 name;
	SfzArrayLocal<SfzStrID, ZG_MAX_NUM_RENDER_TARGETS> renderTargetNames;
	SfzStrID depthBufferName = SFZ_NULL_STR_ID;
	i32x2 res = i32x2_splat(0);

	bool screenRelativeRes = false;
	f32 resScale = 1.0f;
	SfzSetting* resScaleSetting = nullptr;
	SfzSetting* resScaleSetting2 = nullptr;

	bool settingControlledRes = false;
	SfzSetting* controlledResSetting = nullptr;

	SfzFramebufferResourceBuilder() = default;
	SfzFramebufferResourceBuilder(const char* name) : name(sfzStr96InitFmt("%s", name)) {}

	SfzFramebufferResourceBuilder& setName(const char* name);
	SfzFramebufferResourceBuilder& setFixedRes(i32x2 res);
	SfzFramebufferResourceBuilder& setScreenRelativeRes(f32 scale);
	SfzFramebufferResourceBuilder& setScreenRelativeRes(SfzSetting* scaleSetting, SfzSetting* scaleSetting2 = nullptr);
	SfzFramebufferResourceBuilder& setSettingControlledRes(SfzSetting* resSetting);
	SfzFramebufferResourceBuilder& addRenderTarget(SfzStrIDs* ids, const char* textureName);
	SfzFramebufferResourceBuilder& addRenderTarget(SfzStrID textureName);
	SfzFramebufferResourceBuilder& setDepthBuffer(SfzStrIDs* ids, const char* textureName);
	SfzFramebufferResourceBuilder& setDepthBuffer(SfzStrID textureName);
	SfzFramebufferResource build(i32x2 screenRes, SfzStrIDs* ids, SfzResourceManager* resMan);
};
