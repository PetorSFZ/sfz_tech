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

// Types
// ------------------------------------------------------------------------------------------------

struct SfzConfig;
struct SfzGlobalConfig;
struct SfzSetting;

// Config
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C SfzConfig* sfzCfgCreate(const char* basePath, const char* fileName, SfzAllocator* allocator);
SFZ_EXTERN_C void sfzCfgDestroy(SfzConfig* cfg);

// Getters
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C SfzGlobalConfig* sfzCfgGetLegacyConfig(SfzConfig* cfg);
SFZ_EXTERN_C const SfzGlobalConfig* sfzCfgGetLegacyConfigConst(const SfzConfig* cfg);

SFZ_EXTERN_C SfzSetting* sfzCfgGetSetting(SfzConfig* cfg, const char* name);
SFZ_EXTERN_C const SfzSetting* sfzCfgGetSettingConst(const SfzConfig* cfg, const char* name);

SFZ_EXTERN_C i32 sfzCfgGetI32(const SfzConfig* cfg, const char* name);
SFZ_EXTERN_C f32 sfzCfgGetF32(const SfzConfig* cfg, const char* name);
SFZ_EXTERN_C bool sfzCfgGetBool(const SfzConfig* cfg, const char* name);

// Sanitizers
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C i32 sfzCfgSanitizeI32(
	SfzConfig* cfg,
	const char* name,
	bool writeToFile,
	i32 defaultVal,
	i32 minVal,
	i32 maxVal,
	i32 step);

SFZ_EXTERN_C f32 sfzCfgSanitizeF32(
	SfzConfig* cfg,
	const char* name,
	bool writeToFile,
	f32 defaultVal,
	f32 minVal,
	f32 maxVal);

SFZ_EXTERN_C bool sfzCfgSanitizeBool(
	SfzConfig* cfg,
	const char* name,
	bool writeToFile,
	bool defaultVal);

// Setters
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C void sfzCfgSetI32(SfzConfig* cfg, const char* name, i32 val);
SFZ_EXTERN_C void sfzCfgSetF32(SfzConfig* cfg, const char* name, f32 val);
SFZ_EXTERN_C void sfzCfgSetBool(SfzConfig* cfg, const char* name, bool val);
SFZ_EXTERN_C void sfzCfgToggleBool(SfzConfig* cfg, const char* name);
