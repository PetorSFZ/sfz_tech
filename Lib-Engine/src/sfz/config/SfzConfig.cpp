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

#include "sfz/config/SfzConfig.h"

#include <skipifzero_strings.hpp>

#include <sfz/config/GlobalConfig.hpp>
#include <sfz/config/Setting.hpp>

// Types
// ------------------------------------------------------------------------------------------------

sfz_struct(SfzConfig) {
	SfzAllocator* allocator;
	SfzGlobalConfig globalCfg;
};

// Statics
// ------------------------------------------------------------------------------------------------

static bool extractSectionKey(const char* name, SfzStr96& sectionOut, SfzStr96& keyOut)
{
	const char* dot = strchr(name, '.');
	sfz_assert(dot != nullptr);
	if (dot == nullptr) return false;
	const u32 sectionLen = u32(dot - name);
	sfzStr96AppendChars(&sectionOut, name, sectionLen);
	keyOut = sfzStr96Init(dot + 1);
	return true;
}

// Config
// ------------------------------------------------------------------------------------------------

sfz_extern_c SfzConfig* sfzCfgCreate(const char* basePath, const char* fileName, SfzAllocator* allocator)
{
	SfzConfig* cfg = sfz_new<SfzConfig>(allocator, sfz_dbg(""));
	cfg->allocator = allocator;
	cfg->globalCfg.init(basePath, fileName, allocator);
	cfg->globalCfg.load();
	return cfg;
}

sfz_extern_c void sfzCfgDestroy(SfzConfig* cfg)
{
	if (cfg == nullptr) return;
	SfzAllocator* allocator = cfg->allocator;
	sfz_delete(allocator, cfg);
}

// Getters
// ------------------------------------------------------------------------------------------------

sfz_extern_c SfzGlobalConfig* sfzCfgGetLegacyConfig(SfzConfig* cfg)
{
	return &cfg->globalCfg;
}

sfz_extern_c const SfzGlobalConfig* sfzCfgGetLegacyConfigConst(const SfzConfig* cfg)
{
	return &cfg->globalCfg;
}

sfz_extern_c SfzSetting* sfzCfgGetSetting(SfzConfig* cfg, const char* nameIn)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return nullptr;
	return cfg->globalCfg.getSetting(section.str, key.str);
}

sfz_extern_c const SfzSetting* sfzCfgGetSettingConst(const SfzConfig* cfg, const char* nameIn)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return nullptr;
	return cfg->globalCfg.getSetting(section.str, key.str);
}

sfz_extern_c i32 sfzCfgGetI32(const SfzConfig* cfg, const char* nameIn)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return 0;
	return cfg->globalCfg.getSetting(section.str, key.str)->intValue();
}

sfz_extern_c f32 sfzCfgGetF32(const SfzConfig* cfg, const char* nameIn)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return 0.0f;
	return cfg->globalCfg.getSetting(section.str, key.str)->floatValue();
}

sfz_extern_c bool sfzCfgGetBool(const SfzConfig* cfg, const char* nameIn)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return false;
	return cfg->globalCfg.getSetting(section.str, key.str)->boolValue();
}

// Sanitizers
// ------------------------------------------------------------------------------------------------

sfz_extern_c i32 sfzCfgSanitizeI32(
	SfzConfig* cfg,
	const char* nameIn,
	bool writeToFile,
	i32 defaultVal,
	i32 minVal,
	i32 maxVal,
	i32 step)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return 0;
	return cfg->globalCfg.sanitizeInt(section.str, key.str, writeToFile, defaultVal, minVal, maxVal, step)->intValue();
}

sfz_extern_c f32 sfzCfgSanitizeF32(
	SfzConfig* cfg,
	const char* nameIn,
	bool writeToFile,
	f32 defaultVal,
	f32 minVal,
	f32 maxVal)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return 0.0f;
	return cfg->globalCfg.sanitizeFloat(section.str, key.str, writeToFile, defaultVal, minVal, maxVal)->floatValue();
}

sfz_extern_c bool sfzCfgSanitizeBool(
	SfzConfig* cfg,
	const char* nameIn,
	bool writeToFile,
	bool defaultVal)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return false;
	return cfg->globalCfg.sanitizeBool(section.str, key.str, writeToFile, defaultVal)->boolValue();
}

// Setters
// ------------------------------------------------------------------------------------------------

sfz_extern_c void sfzCfgSetI32(SfzConfig* cfg, const char* nameIn, i32 val)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return;
	SfzSetting* setting = cfg->globalCfg.getSetting(section.str, key.str);
	sfz_assert(setting != nullptr);
	const bool success = setting->setInt(val);
	sfz_assert(success);
}

sfz_extern_c void sfzCfgSetF32(SfzConfig* cfg, const char* nameIn, f32 val)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return;
	SfzSetting* setting = cfg->globalCfg.getSetting(section.str, key.str);
	sfz_assert(setting != nullptr);
	const bool success = setting->setFloat(val);
	sfz_assert(success);
}

sfz_extern_c void sfzCfgSetBool(SfzConfig* cfg, const char* nameIn, bool val)
{
	SfzStr96 section = {};
	SfzStr96 key = {};
	if (!extractSectionKey(nameIn, section, key)) return;
	SfzSetting* setting = cfg->globalCfg.getSetting(section.str, key.str);
	sfz_assert(setting != nullptr);
	const bool success = setting->setBool(val);
	sfz_assert(success);
}

sfz_extern_c void sfzCfgToggleBool(SfzConfig* cfg, const char* nameIn)
{
	const bool val = sfzCfgGetBool(cfg, nameIn);
	sfzCfgSetBool(cfg, nameIn, !val);
}
