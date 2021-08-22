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

#include "sfz/config/Setting.hpp"

namespace sfz {

// GlobalConfig
// ------------------------------------------------------------------------------------------------

struct GlobalConfigImpl; // Pimpl pattern

/// A global configuration class
///
/// The singleton instance should be acquired from the Phantasy Engine global context
///
/// Setting invariants:
/// 1, All settings are owned by the singleton instance, no one else may delete the memory.
/// 2, A setting, once created, can never be destroyed or removed during runtime.
/// 3, A setting will occupy the same place in memory for the duration of the program's runtime.
/// 4, A setting can not change section or key identifiers once created.
///
/// These invariants mean that it is safe (and expected) to store direct pointers to settings and
/// read/write to them when needed. However, settings may change type during runtime. So it is
/// recommended to store a pointer to the setting itself and not its internal int value for
/// example.
///
/// Settings are expected to stay relatively static during the runtime of a program. They are not
/// meant for communication and should not be changed unless the user specifically requests for
/// them to be changed.
class GlobalConfig {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	inline GlobalConfig() noexcept : mImpl(nullptr) {} // Compile fix for Emscripten
	GlobalConfig(const GlobalConfig&) = delete;
	GlobalConfig& operator= (const GlobalConfig&) = delete;
	GlobalConfig(GlobalConfig&&) = delete;
	GlobalConfig& operator= (GlobalConfig&&) = delete;

	~GlobalConfig() noexcept { this->destroy(); }

	// Methods
	// --------------------------------------------------------------------------------------------

	void init(const char* basePath, const char* fileName, SfzAllocator* allocator) noexcept;
	void destroy() noexcept;

	// Set global config to not save to ini file when asked to, mainly used for debug purposes.
	void setNoSaveConfigMode();

	void load() noexcept;
	bool save() noexcept;

	/// Gets the specified Setting. If it does not exist it will be created (type int with value 0).
	/// The optional parameter "created" returns whether the Setting was created or already existed.
	Setting* createSetting(const char* section, const char* key, bool* created = nullptr) noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	/// Gets the specified Setting. Returns nullptr if it does not exist.
	Setting* getSetting(const char* section, const char* key) noexcept;
	Setting* getSetting(const char* key) noexcept;

	/// Returns pointers to all available settings
	void getAllSettings(Array<Setting*>& settings) noexcept;

	/// Returns all sections available
	void getSections(Array<str32>& sections) noexcept;

	/// Returns all settings available in a given section
	void getSectionSettings(const char* section, Array<Setting*>& settings) noexcept;

	// Sanitizers
	// --------------------------------------------------------------------------------------------

	/// A sanitizer is basically a wrapper around createSetting, with the addition that it also
	/// ensures that the Setting is of the requested type and with values conforming to the
	/// specified bounds. If the setting does not exist or is of an incompatible type it will be
	/// set to the specified default value.

	Setting* sanitizeInt(
		const char* section, const char* key,
		bool writeToFile = true,
		const IntBounds& bounds = IntBounds(0)) noexcept;

	Setting* sanitizeFloat(
		const char* section, const char* key,
		bool writeToFile = true,
		const FloatBounds& bounds = FloatBounds(0.0f)) noexcept;

	Setting* sanitizeBool(
		const char* section, const char* key,
		bool writeToFile = true,
		const BoolBounds& bounds = BoolBounds(false)) noexcept;

	Setting* sanitizeInt(
		const char* section, const char* key,
		bool writeToFile = true,
		i32 defaultValue = 0,
		i32 minValue = I32_MIN,
		i32 maxValue = I32_MAX,
		i32 step = 1) noexcept;

	Setting* sanitizeFloat(
		const char* section, const char* key,
		bool writeToFile = true,
		f32 defaultValue = 0.0f,
		f32 minValue = -F32_MAX,
		f32 maxValue = F32_MAX) noexcept;

	Setting* sanitizeBool(
		const char* section, const char* key,
		bool writeToFile = true,
		bool defaultValue = false) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	GlobalConfigImpl* mImpl;
};

} // namespace sfz
