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

#include <skipifzero_strings.hpp>

namespace sfz {

using sfz::str32;
using sfz::str48;

// Value type enum
// ------------------------------------------------------------------------------------------------

enum class ValueType : u32 {
	INT = 0,
	FLOAT = 1,
	BOOL = 2,
};

// Bounds structs
// ------------------------------------------------------------------------------------------------

struct IntBounds final {
	i32 defaultValue;
	i32 minValue;
	i32 maxValue;
	i32 step;

	IntBounds() noexcept = default;
	explicit IntBounds(
		i32 defaultValue,
		i32 minValue = I32_MIN,
		i32 maxValue = I32_MAX,
		i32 step = 1) noexcept
	:
		defaultValue(defaultValue),
		minValue(minValue),
		maxValue(maxValue),
		step(step)
	{ }
};

struct FloatBounds final {
	f32 defaultValue;
	f32 minValue;
	f32 maxValue;

	FloatBounds() noexcept = default;
	explicit FloatBounds(
		f32 defaultValue,
		f32 minValue = -F32_MAX,
		f32 maxValue = F32_MAX) noexcept
	:
		defaultValue(defaultValue),
		minValue(minValue),
		maxValue(maxValue)
	{ }
};

struct BoolBounds final {
	bool defaultValue;

	BoolBounds() noexcept = default;
	explicit BoolBounds(bool defaultValue) noexcept : defaultValue(defaultValue) { }
};

// C++ Value structs
// ------------------------------------------------------------------------------------------------

struct IntValue final {
	i32 value;
	IntBounds bounds;

	IntValue() noexcept = default;
	explicit IntValue(i32 value, const IntBounds& bounds = IntBounds(0)) noexcept
	:
		value(value),
		bounds(bounds)
	{ }
};

struct FloatValue final {
	f32 value;
	FloatBounds bounds;

	FloatValue() noexcept = default;
	explicit FloatValue(f32 value, const FloatBounds& bounds = FloatBounds(0.0f)) noexcept
	:
		value(value),
		bounds(bounds)
	{ }
};

struct BoolValue final {
	bool value;
	BoolBounds bounds;

	BoolValue() noexcept = default;
	explicit BoolValue(bool value, const BoolBounds& bounds = BoolBounds(false)) noexcept
	:
		value(value),
		bounds(bounds)
	{ }
};

// Setting value struct
// ------------------------------------------------------------------------------------------------

struct SettingValue final {
	ValueType type;
	bool writeToFile;
	union {
		IntValue i;
		FloatValue f;
		BoolValue b;
	};

	SettingValue() noexcept = default;
	SettingValue(const SettingValue&) noexcept = default;
	SettingValue& operator= (const SettingValue&) noexcept = default;

	static SettingValue createInt(
		i32 value = 0,
		bool writeToFile = true,
		const IntBounds& bounds = IntBounds(0));

	static SettingValue createFloat(
		f32 value = 0.0f,
		bool writeToFile = true,
		const FloatBounds& bounds = FloatBounds(0.0f));

	static SettingValue createBool(
		bool value = false,
		bool writeToFile = true,
		const BoolBounds& bounds = BoolBounds(false));
};

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

	const str32& section() const noexcept { return mSection; }
	const str48& key() const noexcept { return mKey; }
	const SettingValue& value() const noexcept { return mValue; }
	ValueType type() const noexcept { return mValue.type; }

	i32 intValue() const noexcept;
	f32 floatValue() const noexcept;
	bool boolValue() const noexcept;

	const IntBounds& intBounds() const noexcept;
	const FloatBounds& floatBounds() const noexcept;
	const BoolBounds& boolBounds() const noexcept;

	// Setters
	// --------------------------------------------------------------------------------------------

	// Sets the value of this Setting. The value might be clamped by the bounds of this Setting.
	// Returns false and does nothing if the Setting is of another type.
	bool setInt(i32 value) noexcept;
	bool setFloat(f32 value) noexcept;
	bool setBool(bool value) noexcept;

	// Sets whether to save setting to file or not
	void setWriteToFile(bool writeToFile) noexcept;

	// Changes the setting to the specified value (type, bounds, value). Returns true on success,
	// false if the value is invalid in some way.
	bool create(const SettingValue& value) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	SettingValue mValue;
	str32 mSection;
	str48 mKey;
};

} // namespace sfz
