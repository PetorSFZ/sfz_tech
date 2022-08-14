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

#include "sfz/config/GlobalConfig.hpp"

#include <sfz_math.h>
#include <sfz_unique_ptr.hpp>

#include "sfz/SfzLogging.h"
#include "sfz/util/IniParser.hpp"

// GlobalConfigImpl
// ------------------------------------------------------------------------------------------------

namespace {

struct Section final {
	SfzStr32 sectionKey;
	SfzArray<SfzUniquePtr<SfzSetting>> settings;
};

}

struct SfzGlobalConfigImpl final {
	SfzAllocator* allocator = nullptr;
	sfz::IniParser ini;
	SfzArray<Section> sections;
	bool loaded = false; // Can only be loaded once... for now
	bool noSaveMode = false;
};

// GlobalConfig: Methods
// ------------------------------------------------------------------------------------------------

void SfzGlobalConfig::init(const char* basePath, const char* fileName, SfzAllocator* allocator) noexcept
{
	if (mImpl != nullptr) this->destroy();
	mImpl = sfz_new<SfzGlobalConfigImpl>(allocator, sfz_dbg("GlobalConfigImpl"));
	mImpl->allocator = allocator;

	// Initialize IniParser with path
	SfzStr320 tmpPath = sfzStr320InitFmt("%s/%s", basePath, fileName);
	mImpl->ini = sfz::IniParser(tmpPath.str, allocator);

	// Initialize settings array with allocator
	mImpl->sections.init(64, allocator, sfz_dbg(""));
}

void SfzGlobalConfig::destroy() noexcept
{
	if (mImpl == nullptr) return;
	SfzAllocator* allocator = mImpl->allocator;
	sfz_delete(allocator, mImpl);
	mImpl = nullptr;
}

void SfzGlobalConfig::setNoSaveConfigMode()
{
	mImpl->noSaveMode = true;
}

void SfzGlobalConfig::load() noexcept
{
	sfz_assert(mImpl != nullptr);
	sfz_assert(!mImpl->loaded); // TODO: Make it possible to reload settings from file

	// Load ini file
	sfz::IniParser & ini = mImpl->ini;
	if (ini.load()) {
		SFZ_LOG_INFO("Succesfully loaded config ini file");
	} else {
		SFZ_LOG_INFO("Failed to load config ini file, expected if this is first run");
	}

	// Create setting items of all ini items
	for (auto item : ini) {

		// Attempt to find section
		Section* section = nullptr;
		for (Section& s : mImpl->sections) {
			if (s.sectionKey == item.getSection()) {
				section = &s;
				break;
			}
		}

		// If section not found, create it
		if (section == nullptr) {
			mImpl->sections.add(Section());
			section = &mImpl->sections.last();
			sfzStr32Clear(&section->sectionKey);
			sfzStr32Appendf(&section->sectionKey, "%s", item.getSection());
			section->settings.init(64, mImpl->allocator, sfz_dbg(""));
		}

		// Create new setting
		section->settings.add(
			sfzMakeUnique<SfzSetting>(mImpl->allocator, sfz_dbg(""), item.getSection(), item.getKey()));
		SfzSetting& setting = *section->settings.last();

		// Get value of setting
		if (item.getFloat() != nullptr) {
			f32 floatVal = *item.getFloat();
			i32 intVal = *item.getInt();
			if (sfz::eqf(floatVal, f32(intVal))) {
				setting.create(SfzSettingValue::createInt(intVal, true, SfzIntBounds(0)));
			}
			else {
				setting.create(SfzSettingValue::createFloat(floatVal, true, SfzFloatBounds(0.0f)));
			}
		}
		else if (item.getBool() != nullptr) {
			bool boolVal = *item.getBool();
			setting.create(SfzSettingValue::createBool(boolVal, true, SfzBoolBounds(false)));
		}
	}

	mImpl->loaded = true;
}

bool SfzGlobalConfig::save() noexcept
{
	sfz_assert(mImpl != nullptr);
	sfz::IniParser& ini = mImpl->ini;

	if (mImpl->noSaveMode) return false;

	// Update internal ini with the current values of the setting
	for (auto& section : mImpl->sections) {
		for (auto& setting : section.settings) {

			// If setting should not be written to file, just write it if it did not exist in ini
			// file already.
			if (!setting->value().writeToFile)
			{
				switch (setting->type()) {
				case SfzValueType::INT:
					if (ini.getInt(setting->section().str, setting->key().str) == nullptr) {
						ini.setInt(setting->section().str, setting->key().str,
							setting->intBounds().defaultValue);
					}
					break;
				case SfzValueType::FLOAT:
					if (ini.getFloat(setting->section().str, setting->key().str) == nullptr) {
						ini.setFloat(setting->section().str, setting->key().str,
							setting->floatBounds().defaultValue);
					}
					break;
				case SfzValueType::BOOL:
					if (ini.getBool(setting->section().str, setting->key().str) == nullptr) {
						ini.setBool(setting->section().str, setting->key().str,
							setting->boolBounds().defaultValue);
					}
					break;
				}
				continue;
			}

			switch (setting->type()) {
			case SfzValueType::INT:
				ini.setInt(setting->section().str, setting->key().str, setting->intValue());
				break;
			case SfzValueType::FLOAT:
				ini.setFloat(setting->section().str, setting->key().str, setting->floatValue());
				break;
			case SfzValueType::BOOL:
				ini.setBool(setting->section().str, setting->key().str, setting->boolValue());
				break;
			}
		}
	}

	// Write to ini
	return ini.save();
}

SfzSetting* SfzGlobalConfig::createSetting(const char* section, const char* key, bool* created) noexcept
{
	SfzSetting* setting = this->getSetting(section, key);

	// Return setting if it already exists
	if (setting != nullptr) {
		if (created != nullptr) *created = false;
		return setting;
	}

	// Attempt to find section
	Section* sectionPtr = nullptr;
	for (Section& s : mImpl->sections) {
		if (s.sectionKey == section) {
			sectionPtr = &s;
			break;
		}
	}

	// If section not found, create it
	if (sectionPtr == nullptr) {
		mImpl->sections.add(Section());
		sectionPtr = &mImpl->sections.last();
		sfzStr32Clear(&sectionPtr->sectionKey);
		sfzStr32Appendf(&sectionPtr->sectionKey, "%s", section);
		sectionPtr->settings.init(64, mImpl->allocator, sfz_dbg(""));
	}

	// Create and return section
	sectionPtr->settings.add(sfzMakeUnique<SfzSetting>(mImpl->allocator, sfz_dbg(""), section, key));
	if (created != nullptr) *created = true;
	return sectionPtr->settings.last().get();
}

// GlobalConfig: Getters
// ------------------------------------------------------------------------------------------------

SfzSetting* SfzGlobalConfig::getSetting(const char* section, const char* key) noexcept
{
	sfz_assert(mImpl != nullptr);
	for (auto& sec : mImpl->sections) {
		if (sec.sectionKey != section) continue;
		for (auto& setting : sec.settings) {
			if (setting->key() == key) return setting.get();
		}
	}
	return nullptr;
}

const SfzSetting* SfzGlobalConfig::getSetting(const char* section, const char* key) const noexcept
{
	sfz_assert(mImpl != nullptr);
	for (auto& sec : mImpl->sections) {
		if (sec.sectionKey != section) continue;
		for (auto& setting : sec.settings) {
			if (setting->key() == key) return setting.get();
		}
	}
	return nullptr;
}

SfzSetting* SfzGlobalConfig::getSetting(const char* key) noexcept
{
	return this->getSetting("", key);
}

void SfzGlobalConfig::getAllSettings(SfzArray<SfzSetting*>& settings) noexcept
{
	sfz_assert(mImpl != nullptr);
	for (auto& section : mImpl->sections) {
		for (auto& setting : section.settings) {
			settings.add(setting.get());
		}
	}
}

void SfzGlobalConfig::getSections(SfzArray<SfzStr32>& sections) noexcept
{
	sfz_assert(mImpl != nullptr);
	sections.ensureCapacity(mImpl->sections.size() + sections.size());
	for (auto& section : mImpl->sections) {
		sections.add(section.sectionKey);
	}
}

void SfzGlobalConfig::getSectionSettings(const char* section, SfzArray<SfzSetting*>& settings) noexcept
{
	sfz_assert(mImpl != nullptr);

	// Attempt to find section
	Section* sectionPtr = nullptr;
	for (Section& s : mImpl->sections) {
		if (s.sectionKey == section) {
			sectionPtr = &s;
			break;
		}
	}

	// If no section, just return
	if (sectionPtr == nullptr) return;

	// Add settings
	settings.ensureCapacity(sectionPtr->settings.size() + settings.size());
	for (auto& s : sectionPtr->settings) {
		settings.add(s.get());
	}
}

// GlobalConfig: Sanitizers
// ------------------------------------------------------------------------------------------------

SfzSetting* SfzGlobalConfig::sanitizeInt(
	const char* section, const char* key,
	bool writeToFile,
	const SfzIntBounds& bounds) noexcept
{
	bool created = false;
	SfzSetting* setting = this->createSetting(section, key, &created);

	// Store previous value
	i32 previousValue = 0;
	switch (setting->type()) {
	case SfzValueType::INT:
		previousValue = setting->intValue();
		break;
	case SfzValueType::FLOAT:
		previousValue = i32(::roundf(setting->floatValue()));
		break;
	case SfzValueType::BOOL:
		previousValue = setting->boolValue() ? 1 : 0;
		break;
	}

	// Create setting according to bounds
	bool boundsGood =
		setting->create(SfzSettingValue::createInt(bounds.defaultValue, writeToFile, bounds));

	// Check if bounds were good
	if (!boundsGood) {
		SFZ_LOG_ERROR("Provided bad bounds for setting: %s - %s", section, key);
		setting->create(SfzSettingValue::createInt(0));
	}

	// If not created, restore previous value (will be sanitized here)
	if (!created) {
		setting->setInt(previousValue);
	}

	return setting;
}

SfzSetting* SfzGlobalConfig::sanitizeFloat(
	const char* section, const char* key,
	bool writeToFile,
	const SfzFloatBounds& bounds) noexcept
{
	bool created = false;
	SfzSetting* setting = this->createSetting(section, key, &created);

	// Store previous value
	f32 previousValue = 0.0f;
	switch (setting->type()) {
	case SfzValueType::INT:
		previousValue = f32(setting->intValue());
		break;
	case SfzValueType::FLOAT:
		previousValue = setting->floatValue();
		break;
	case SfzValueType::BOOL:
		previousValue = setting->boolValue() ? 1.0f : 0.0f;
		break;
	}

	// Create setting according to bounds
	bool boundsGood =
		setting->create(SfzSettingValue::createFloat(bounds.defaultValue, writeToFile, bounds));

	// Check if bounds were good
	if (!boundsGood) {
		SFZ_LOG_ERROR("Provided bad bounds for setting: %s - %s", section, key);
		setting->create(SfzSettingValue::createFloat(0.0f));
	}

	// If not created, restore previous value (will be sanitized here)
	if (!created) {
		setting->setFloat(previousValue);
	}

	return setting;
}

SfzSetting* SfzGlobalConfig::sanitizeBool(
	const char* section, const char* key,
	bool writeToFile,
	const SfzBoolBounds& bounds) noexcept
{
	bool created = false;
	SfzSetting* setting = this->createSetting(section, key, &created);

	// Store previous value
	bool previousValue = false;
	switch (setting->type()) {
	case SfzValueType::INT:
		previousValue = setting->intValue() == 0 ? false : true;
		break;
	case SfzValueType::FLOAT:
		previousValue = setting->floatValue() == 0.0f ? false : true;
		break;
	case SfzValueType::BOOL:
		previousValue = setting->boolValue();
		break;
	}

	// Create setting according to bounds
	bool boundsGood =
		setting->create(SfzSettingValue::createBool(bounds.defaultValue, writeToFile, bounds));

	// Check if bounds were good
	if (!boundsGood) {
		SFZ_LOG_ERROR("Provided bad bounds for setting: %s - %s", section, key);
		setting->create(SfzSettingValue::createBool(false));
	}

	// If not created, restore previous value (will be sanitized here)
	if (!created) {
		setting->setBool(previousValue);
	}

	return setting;
}

SfzSetting* SfzGlobalConfig::sanitizeInt(
	const char* section, const char* key,
	bool writeToFile,
	i32 defaultValue,
	i32 minValue,
	i32 maxValue,
	i32 step) noexcept
{
	return this->sanitizeInt(section, key, writeToFile,
		SfzIntBounds(defaultValue, minValue, maxValue, step));
}

SfzSetting* SfzGlobalConfig::sanitizeFloat(
	const char* section, const char* key,
	bool writeToFile,
	f32 defaultValue,
	f32 minValue,
	f32 maxValue) noexcept
{
	return this->sanitizeFloat(section, key, writeToFile,
		SfzFloatBounds(defaultValue, minValue, maxValue));
}

SfzSetting* SfzGlobalConfig::sanitizeBool(
	const char* section, const char* key,
	bool writeToFile,
	bool defaultValue) noexcept
{
	return this->sanitizeBool(section, key, writeToFile,
		SfzBoolBounds(defaultValue));
}
