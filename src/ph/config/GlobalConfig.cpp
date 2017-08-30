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

class GlobalConfigImpl final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	IniParser mIni;
	DynArray<UniquePtr<Setting>> mSettings;
	bool mLoaded = false; // Can only be loaded once... for now

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	GlobalConfigImpl() noexcept = default;
	GlobalConfigImpl(const GlobalConfigImpl&) = delete;
	GlobalConfigImpl& operator= (const GlobalConfigImpl&) = delete;
	GlobalConfigImpl(GlobalConfigImpl&&) = delete;
	GlobalConfigImpl& operator= (GlobalConfigImpl&&) = delete;
	~GlobalConfigImpl() noexcept = default;
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
	config.getCreateSetting = [](const char* section, const char* key) -> phSettingValue* {
		return &instance().getCreateSetting(section, key)->value;
	};
	config.getSetting = [](const char* section, const char* key) -> phSettingValue* {
		return &instance().getSetting(section, key)->value;
	};
	return config;
}

// GlobalConfig: Methods
// ------------------------------------------------------------------------------------------------

void GlobalConfig::init(const char* basePath, const char* fileName) noexcept
{
	if (mImpl != nullptr) this->destroy();
	mImpl = sfzNewDefault<GlobalConfigImpl>();

	// Initialize IniParser with path
	StackString256 tmpPath;
	tmpPath.printf("%s%s", basePath, fileName);
	mImpl->mIni = IniParser(tmpPath.str);
}

void GlobalConfig::destroy() noexcept
{
	if (mImpl == nullptr) return;
	sfzDeleteDefault<GlobalConfigImpl>(mImpl);
	mImpl = nullptr;
}

void GlobalConfig::load() noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	sfz_assert_debug(!mImpl->mLoaded); // TODO: Make it possible to reload settings from file

	// Load ini file
	IniParser& ini = mImpl->mIni;
	if (ini.load()) {
		PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Succesfully loaded config ini file");
	} else {
		PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Failed to load config ini file, expected if this is first run");
	}

	// Create setting items of all ini items
	for (auto item : ini) {
		
		// Create new setting
		mImpl->mSettings.add(makeUniqueDefault<Setting>(item.getSection(), item.getKey()));
		Setting& setting = *mImpl->mSettings.last();

		// Get value of setting
		if (item.getFloat() != nullptr) {
			float floatVal = *item.getFloat();
			int32_t intVal = *item.getInt();
			if (approxEqual(floatVal, float(intVal))) {
				setting.setInt(intVal);
			}
			else {
				setting.setFloat(floatVal);
			}
		}
		else if (item.getBool() != nullptr) {
			bool b = *item.getBool();
			setting.setBool(b);
		}
	}

	mImpl->mLoaded = true;
}

bool GlobalConfig::save() noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	IniParser& ini = mImpl->mIni;
	
	// Update internal ini with the current values of the setting
	for (auto& setting : mImpl->mSettings) {
		if (setting->isBoolValue()) {
			ini.setBool(setting->section().str, setting->key().str, setting->boolValue());
		} else if (setting->type() == VALUE_TYPE_INT) {
			ini.setInt(setting->section().str, setting->key().str, setting->intValue());
		} else if (setting->type() == VALUE_TYPE_FLOAT) {
			ini.setFloat(setting->section().str, setting->key().str, setting->floatValue());
		}
	}

	// Write to ini
	return ini.save();
}

Setting* GlobalConfig::getCreateSetting(const char* section, const char* key, bool* created) noexcept
{
	Setting* setting = this->getSetting(section, key);
	
	if (setting != nullptr) {
		if (created != nullptr) *created = false;
		return setting;
	}

	mImpl->mSettings.add(makeUniqueDefault<Setting>(section, key));
	if (created != nullptr) *created = true;
	return mImpl->mSettings.last().get();
}

// GlobalConfig: Getters
// ------------------------------------------------------------------------------------------------

Setting* GlobalConfig::getSetting(const char* section, const char* key) noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	for (auto& setting : mImpl->mSettings) {
		if (setting->section() == section && setting->key() == key) {
			return setting.get();
		}
	}
	return nullptr;
}

Setting* GlobalConfig::getSetting(const char* key) noexcept
{
	return this->getSetting("", key);
}

void GlobalConfig::getSettings(DynArray<Setting*>& settings) noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	settings.ensureCapacity(mImpl->mSettings.size());
	for (auto& setting : mImpl->mSettings) {
		settings.add(setting.get());
	}
}

// GlobalConfig: Sanitizers
// ------------------------------------------------------------------------------------------------

Setting* GlobalConfig::sanitizeInt(const char* section, const char* key,
                                   int32_t defaultValue,
                                   int32_t minValue,
                                   int32_t maxValue) noexcept
{
	sfz_assert_debug(mImpl != nullptr);

	bool created = false;
	Setting* setting = getCreateSetting(section, key, &created);

	// Set default value if created
	if (created) {
		setting->setInt(defaultValue);
		return setting;
	}

	// Make sure setting is of correct type
	if (setting->type() != VALUE_TYPE_INT) {
		setting->setInt(setting->intValue(), minValue, maxValue);
	}

	// Ensure value is in range
	int32_t val = setting->intValue();
	val = std::min(std::max(val, minValue), maxValue);
	setting->setInt(val);

	return setting;
}

Setting* GlobalConfig::sanitizeFloat(const char* section, const char* key,
                                     float defaultValue,
                                     float minValue,
                                     float maxValue) noexcept
{
	bool created = false;
	Setting* setting = getCreateSetting(section, key, &created);

	// Set default value if created
	if (created) {
		setting->setFloat(defaultValue);
		return setting;
	}

	// Make sure setting is of correct type
	if (setting->type() != VALUE_TYPE_FLOAT) {
		setting->setFloat(setting->floatValue(), minValue, maxValue);
	}

	// Ensure value is in range
	float val = setting->floatValue();
	val = std::min(std::max(val, minValue), maxValue);
	setting->setFloat(val);

	return setting;
}

Setting* GlobalConfig::sanitizeBool(const char* section, const char* key,
                                    bool defaultValue) noexcept
{
	bool created = false;
	Setting* setting = getCreateSetting(section, key, &created);

	// Set default value if created
	if (created) {
		setting->setBool(defaultValue);
		return setting;
	}

	// Make sure setting is of correct type
	if (!setting->isBoolValue()) {
		setting->setBool(defaultValue);
	}
	return setting;
}

// GlobalConfig: Private constructors & destructors
// ------------------------------------------------------------------------------------------------

GlobalConfig::~GlobalConfig() noexcept
{
	this->destroy();
}

} // namespace ph
