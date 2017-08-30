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
#include <cmath>

#include <sfz/strings/StackString.hpp>

#include "ph/ConfigInterface.h"

namespace ph {

using sfz::StackString64;
using sfz::StackString192;
using std::int32_t;


// Setting class
// ------------------------------------------------------------------------------------------------

/// A Phantasy Engine Setting
///
/// If the getters are used instead of the raw value a Setting can be accessed as if it were of a
/// different type. For example, a float setting can be accessed using the int getters. The float
/// values will then be rounded before they are returned. A Setting can also be of bool type,
/// this is represented using the VALUE_TYPE_INT and a min value of 0 and max value of 1.
class Setting final {
public:
	// Public members
	// --------------------------------------------------------------------------------------------
	
	/// The value of this Setting
	phSettingValue value;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Setting() = delete;
	Setting(const Setting&) = delete;
	Setting& operator= (const Setting&) = delete;
	Setting(Setting&&) = delete;
	Setting& operator= (Setting&&) = delete;
	~Setting() noexcept = default;

	/// Creates a Setting of type int with value 0
	Setting(const char* section, const char* key) noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	inline const StackString64& section() const noexcept { return mSection; }
	inline const StackString192& key() const noexcept { return mKey; }
	inline phValueType type() const noexcept { return value.type; }

	/// Checks if type is VALUE_TYPE_INT, min value is 0 and max value is 1.
	bool isBoolValue() const noexcept;

	int32_t intValue() const noexcept;
	float floatValue() const noexcept;
	bool boolValue() const noexcept;

	int32_t intMinValue() const noexcept;
	int32_t intMaxValue() const noexcept;
	float floatMinValue() const noexcept;
	float floatMaxValue() const noexcept;
	
	// Setters
	// --------------------------------------------------------------------------------------------

	/// Returns false if the specified value was clamped against the set min and max values. If the
	/// type is changed (i.e. from float to int or int to float) the min and max values are
	/// converted to the new type as well.
	bool setInt(int32_t value) noexcept;
	bool setFloat(float value) noexcept;

	/// Sets the type, value and bounds.
	void setInt(int32_t value, int32_t minValue, int32_t maxValue) noexcept;
	void setFloat(float value, float minValue, float maxValue) noexcept;

	/// Sets the type to bool, i.e. VALUE_TYPE_INT, minValue to 0, maxValue to 1.
	void setBool(bool value) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	StackString64 mSection;
	StackString192 mKey;
};

} // namespace ph
