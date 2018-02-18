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

#include "ph/config/GlobalConfig.hpp"

#include <sfz/math/MathSupport.hpp>
#include <sfz/memory/SmartPointers.hpp>
#include <sfz/util/IniParser.hpp>

#include "ph/utils/Logging.hpp"

namespace ph {

using namespace sfz;

// GlobalConfigImpl
// ------------------------------------------------------------------------------------------------

struct Section final {
	StackString32 sectionKey;
	DynArray<UniquePtr<Setting>> settings;
};

struct GlobalConfigImpl final {
	Allocator* allocator = nullptr;
	IniParser ini;
	DynArray<Section> sections;
	bool loaded = false; // Can only be loaded once... for now
};

// GlobalConfig: Singleton instance
// ------------------------------------------------------------------------------------------------

GlobalConfig& GlobalConfig::instance() noexcept
{
	static GlobalConfig config;
	return config;
}

phConfig GlobalConfig::cInstance() noexcept
{
	phConfig config;

	config.getSetting = [](const char* section, const char* key) -> const phSettingValue* {
		return reinterpret_cast<const phSettingValue*>(
			&instance().getSetting(section, key)->value());
	};

	config.setInt = [](const char* section, const char* key, int32_t value) -> phBool32 {
		Setting* setting = instance().getSetting(section, key);
		if (setting == nullptr) return 0;
		return Bool32(setting->setInt(value));
	};
	config.setFloat = [](const char* section, const char* key, float value) -> phBool32 {
		Setting* setting = instance().getSetting(section, key);
		if (setting == nullptr) return 0;
		return Bool32(setting->setFloat(value));
	};
	config.setBool = [](const char* section, const char* key, phBool32 value) -> phBool32 {
		Setting* setting = instance().getSetting(section, key);
		if (setting == nullptr) return 0;
		return Bool32(setting->setBool(Bool32(value)));
	};

	config.sanitizeInt = [](
		const char* section, const char* key,
		phBool32 writeToFile,
		const phIntBounds* bounds) -> const phSettingValue* {

		Setting* setting = instance().sanitizeInt(section, key, Bool32(writeToFile),
			*reinterpret_cast<const IntBounds*>(bounds));
		return reinterpret_cast<const phSettingValue*>(&setting->value());
	};

	config.sanitizeFloat = [](
		const char* section, const char* key,
		phBool32 writeToFile,
		const phFloatBounds* bounds) -> const phSettingValue* {

		Setting* setting = instance().sanitizeFloat(section, key, Bool32(writeToFile),
			*reinterpret_cast<const FloatBounds*>(bounds));
		return reinterpret_cast<const phSettingValue*>(&setting->value());
	};

	config.sanitizeBool = [](
		const char* section, const char* key,
		phBool32 writeToFile,
		const phBoolBounds* bounds) -> const phSettingValue* {

		Setting* setting = instance().sanitizeBool(section, key, Bool32(writeToFile),
			*reinterpret_cast<const BoolBounds*>(bounds));
		return reinterpret_cast<const phSettingValue*>(&setting->value());
	};

	return config;
}

// GlobalConfig: Methods
// ------------------------------------------------------------------------------------------------

void GlobalConfig::init(const char* basePath, const char* fileName, Allocator* allocator) noexcept
{
	if (mImpl != nullptr) this->destroy();
	mImpl = sfzNew<GlobalConfigImpl>(allocator);
	mImpl->allocator = allocator;

	// Initialize IniParser with path
	StackString256 tmpPath;
	tmpPath.printf("%s%s", basePath, fileName);
	mImpl->ini = IniParser(tmpPath.str);

	// Initialize settings array with allocator
	mImpl->sections.create(64, allocator);
}

void GlobalConfig::destroy() noexcept
{
	if (mImpl == nullptr) return;
	Allocator* allocator = mImpl->allocator;
	sfzDelete<GlobalConfigImpl>(mImpl, allocator);
	mImpl = nullptr;
}

void GlobalConfig::load() noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	sfz_assert_debug(!mImpl->loaded); // TODO: Make it possible to reload settings from file

	// Load ini file
	IniParser& ini = mImpl->ini;
	if (ini.load()) {
		PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Succesfully loaded config ini file");
	} else {
		PH_LOG(LOG_LEVEL_INFO,
			"PhantasyEngine", "Failed to load config ini file, expected if this is first run");
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
			section->sectionKey.printf("%s", item.getSection());
			section->settings.create(64, mImpl->allocator);
		}

		// Create new setting
		section->settings.add(
			makeUnique<Setting>(mImpl->allocator, item.getSection(), item.getKey()));
		Setting& setting = *section->settings.last();

		// Get value of setting
		if (item.getFloat() != nullptr) {
			float floatVal = *item.getFloat();
			int32_t intVal = *item.getInt();
			if (approxEqual(floatVal, float(intVal))) {
				setting.create(SettingValue::createInt(intVal, true, IntBounds(0)));
			}
			else {
				setting.create(SettingValue::createFloat(floatVal, true, FloatBounds(0.0f)));
			}
		}
		else if (item.getBool() != nullptr) {
			bool boolVal = *item.getBool();
			setting.create(SettingValue::createBool(boolVal, true, BoolBounds(false)));
		}
	}

	mImpl->loaded = true;
}

bool GlobalConfig::save() noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	IniParser& ini = mImpl->ini;

	// Update internal ini with the current values of the setting
	for (auto& section : mImpl->sections) {
		for (auto& setting : section.settings) {

			// If setting should not be written to file, just write it if it did not exist in ini
			// file already.
			if (!setting->value().writeToFile)
			{
				switch (setting->type()) {
				case ValueType::INT:
					if (ini.getInt(setting->section().str, setting->key().str) == nullptr) {
						ini.setInt(setting->section().str, setting->key().str,
							setting->intBounds().defaultValue);
					}
					break;
				case ValueType::FLOAT:
					if (ini.getFloat(setting->section().str, setting->key().str) == nullptr) {
						ini.setFloat(setting->section().str, setting->key().str,
							setting->floatBounds().defaultValue);
					}
					break;
				case ValueType::BOOL:
					if (ini.getBool(setting->section().str, setting->key().str) == nullptr) {
						ini.setBool(setting->section().str, setting->key().str,
							setting->boolBounds().defaultValue);
					}
					break;
				}
				continue;
			}

			switch (setting->type()) {
			case ValueType::INT:
				ini.setInt(setting->section().str, setting->key().str, setting->intValue());
				break;
			case ValueType::FLOAT:
				ini.setFloat(setting->section().str, setting->key().str, setting->floatValue());
				break;
			case ValueType::BOOL:
				ini.setBool(setting->section().str, setting->key().str, setting->boolValue());
				break;
			}
		}
	}

	// Write to ini
	return ini.save();
}

Setting* GlobalConfig::createSetting(const char* section, const char* key, bool* created) noexcept
{
	Setting* setting = this->getSetting(section, key);

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
		sectionPtr->sectionKey.printf("%s", section);
		sectionPtr->settings.create(64, mImpl->allocator);
	}

	// Create and return section
	sectionPtr->settings.add(makeUniqueDefault<Setting>(section, key));
	if (created != nullptr) *created = true;
	return sectionPtr->settings.last().get();
}

// GlobalConfig: Getters
// ------------------------------------------------------------------------------------------------

Setting* GlobalConfig::getSetting(const char* section, const char* key) noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	for (auto& sec : mImpl->sections) {
		if (sec.sectionKey != section) continue;
		for (auto& setting : sec.settings) {
			if (setting->key() == key) return setting.get();
		}
	}
	return nullptr;
}

Setting* GlobalConfig::getSetting(const char* key) noexcept
{
	return this->getSetting("", key);
}

void GlobalConfig::getAllSettings(DynArray<Setting*>& settings) noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	for (auto& section : mImpl->sections) {
		for (auto& setting : section.settings) {
			settings.add(setting.get());
		}
	}
}

void GlobalConfig::getSections(DynArray<StackString32>& sections) noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	sections.ensureCapacity(mImpl->sections.size() + sections.size());
	for (auto& section : mImpl->sections) {
		sections.add(section.sectionKey);
	}
}

void GlobalConfig::getSectionSettings(const char* section, DynArray<Setting*>& settings) noexcept
{
	sfz_assert_debug(mImpl != nullptr);

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

Setting* GlobalConfig::sanitizeInt(
	const char* section, const char* key,
	bool writeToFile,
	const IntBounds& bounds) noexcept
{
	bool created = false;
	Setting* setting = this->createSetting(section, key, &created);

	// Store previous value
	int32_t previousValue = 0;
	switch (setting->type()) {
	case ValueType::INT:
		previousValue = setting->intValue();
		break;
	case ValueType::FLOAT:
		previousValue = int32_t(std::round(setting->floatValue()));
		break;
	case ValueType::BOOL:
		previousValue = setting->boolValue() ? 1 : 0;
		break;
	}

	// Create setting according to bounds
	bool boundsGood =
		setting->create(SettingValue::createInt(bounds.defaultValue, writeToFile, bounds));

	// Check if bounds were good
	if (!boundsGood) {
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "Provided bad bounds for setting: %s - %s",
			section, key);
		setting->create(SettingValue::createInt(0));
	}

	// If not created, restore previous value (will be sanitized here)
	if (!created) {
		setting->setInt(previousValue);
	}

	return setting;
}

Setting* GlobalConfig::sanitizeFloat(
	const char* section, const char* key,
	bool writeToFile,
	const FloatBounds& bounds) noexcept
{
	bool created = false;
	Setting* setting = this->createSetting(section, key, &created);

	// Store previous value
	float previousValue = 0.0f;
	switch (setting->type()) {
	case ValueType::INT:
		previousValue = float(setting->intValue());
		break;
	case ValueType::FLOAT:
		previousValue = setting->floatValue();
		break;
	case ValueType::BOOL:
		previousValue = setting->boolValue() ? 1.0f : 0.0f;
		break;
	}

	// Create setting according to bounds
	bool boundsGood =
		setting->create(SettingValue::createFloat(bounds.defaultValue, writeToFile, bounds));

	// Check if bounds were good
	if (!boundsGood) {
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "Provided bad bounds for setting: %s - %s",
			section, key);
		setting->create(SettingValue::createFloat(0.0f));
	}

	// If not created, restore previous value (will be sanitized here)
	if (!created) {
		setting->setFloat(previousValue);
	}

	return setting;
}

Setting* GlobalConfig::sanitizeBool(
	const char* section, const char* key,
	bool writeToFile,
	const BoolBounds& bounds) noexcept
{
	bool created = false;
	Setting* setting = this->createSetting(section, key, &created);

	// Store previous value
	bool previousValue = false;
	switch (setting->type()) {
	case ValueType::INT:
		previousValue = setting->intValue() == 0 ? false : true;
		break;
	case ValueType::FLOAT:
		previousValue = setting->floatValue() == 0.0f ? false : true;
		break;
	case ValueType::BOOL:
		previousValue = setting->boolValue();
		break;
	}

	// Create setting according to bounds
	bool boundsGood =
		setting->create(SettingValue::createBool(bounds.defaultValue, writeToFile, bounds));

	// Check if bounds were good
	if (!boundsGood) {
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "Provided bad bounds for setting: %s - %s",
			section, key);
		setting->create(SettingValue::createBool(false));
	}

	// If not created, restore previous value (will be sanitized here)
	if (!created) {
		setting->setBool(previousValue);
	}

	return setting;
}

Setting* GlobalConfig::sanitizeInt(
	const char* section, const char* key,
	bool writeToFile,
	int32_t defaultValue,
	int32_t minValue,
	int32_t maxValue,
	int32_t step) noexcept
{
	return this->sanitizeInt(section, key, writeToFile,
		IntBounds(defaultValue, minValue, maxValue, step));
}

Setting* GlobalConfig::sanitizeFloat(
	const char* section, const char* key,
	bool writeToFile,
	float defaultValue,
	float minValue,
	float maxValue) noexcept
{
	return this->sanitizeFloat(section, key, writeToFile,
		FloatBounds(defaultValue, minValue, maxValue));
}

Setting* GlobalConfig::sanitizeBool(
	const char* section, const char* key,
	bool writeToFile,
	bool defaultValue) noexcept
{
	return this->sanitizeBool(section, key, writeToFile,
		BoolBounds(defaultValue));
}

// GlobalConfig: Private constructors & destructors
// ------------------------------------------------------------------------------------------------

GlobalConfig::~GlobalConfig() noexcept
{
	this->destroy();
}

} // namespace ph
