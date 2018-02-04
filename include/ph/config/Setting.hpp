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

#include <cstdint>

#include <sfz/strings/StackString.hpp>

#include "ph/ConfigInterface.h"

namespace ph {

using sfz::StackString48;
using sfz::StackString64;

// Setting class
// ------------------------------------------------------------------------------------------------

class Setting final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Setting() = delete;
	Setting(const Setting&) = delete;
	Setting& operator= (const Setting&) = delete;
	Setting(Setting&&) = delete;
	Setting& operator= (Setting&&) = delete;
	~Setting() noexcept = default;

	Setting(const char* section, const char* key) noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	const StackString48& section() const noexcept { return mSection; }
	const StackString64& key() const noexcept { return mKey; }
	const SettingValue& value() const noexcept { return mValue; }
	ValueType type() const noexcept { return mValue.type; }

	int32_t intValue() const noexcept;
	float floatValue() const noexcept;
	bool boolValue() const noexcept;

	const IntBounds& intBounds() const noexcept;
	const FloatBounds& floatBounds() const noexcept;
	const BoolBounds& boolBounds() const noexcept;

	// Setters
	// --------------------------------------------------------------------------------------------

	// Sets the value of this Setting. The value might be clamped by the bounds of this Setting.
	// Returns false and does nothing if the Setting is of another type.
	bool setInt(int32_t value) noexcept;
	bool setFloat(float value) noexcept;
	bool setBool(bool value) noexcept;

	// Changes the setting to the specified value (type, bounds, value). Returns true on success,
	// false if the value is invalid in some way.
	bool create(const SettingValue& value) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	SettingValue mValue;
	StackString48 mSection;
	StackString64 mKey;
};

} // namespace ph
