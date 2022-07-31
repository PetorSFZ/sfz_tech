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

// Value type enum
// ------------------------------------------------------------------------------------------------

enum class SfzValueType : u32 {
	INT = 0,
	FLOAT = 1,
	BOOL = 2,
};

// Bounds structs
// ------------------------------------------------------------------------------------------------

struct SfzIntBounds final {
	i32 defaultValue;
	i32 minValue;
	i32 maxValue;
	i32 step;

	SfzIntBounds() noexcept = default;
	explicit SfzIntBounds(
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

struct SfzFloatBounds final {
	f32 defaultValue;
	f32 minValue;
	f32 maxValue;

	SfzFloatBounds() noexcept = default;
	explicit SfzFloatBounds(
		f32 defaultValue,
		f32 minValue = -F32_MAX,
		f32 maxValue = F32_MAX) noexcept
	:
		defaultValue(defaultValue),
		minValue(minValue),
		maxValue(maxValue)
	{ }
};

struct SfzBoolBounds final {
	bool defaultValue;

	SfzBoolBounds() noexcept = default;
	explicit SfzBoolBounds(bool defaultValue) noexcept : defaultValue(defaultValue) { }
};

// C++ Value structs
// ------------------------------------------------------------------------------------------------

struct SfzIntValue final {
	i32 value;
	SfzIntBounds bounds;

	SfzIntValue() noexcept = default;
	explicit SfzIntValue(i32 value, const SfzIntBounds& bounds = SfzIntBounds(0)) noexcept
	:
		value(value),
		bounds(bounds)
	{ }
};

struct SfzFloatValue final {
	f32 value;
	SfzFloatBounds bounds;

	SfzFloatValue() noexcept = default;
	explicit SfzFloatValue(f32 value, const SfzFloatBounds& bounds = SfzFloatBounds(0.0f)) noexcept
	:
		value(value),
		bounds(bounds)
	{ }
};

struct SfzBoolValue final {
	bool value;
	SfzBoolBounds bounds;

	SfzBoolValue() noexcept = default;
	explicit SfzBoolValue(bool value, const SfzBoolBounds& bounds = SfzBoolBounds(false)) noexcept
	:
		value(value),
		bounds(bounds)
	{ }
};

// Setting value struct
// ------------------------------------------------------------------------------------------------

struct SfzSettingValue final {
	SfzValueType type;
	bool writeToFile;
	union {
		SfzIntValue i;
		SfzFloatValue f;
		SfzBoolValue b;
	};

	SfzSettingValue() noexcept = default;
	SfzSettingValue(const SfzSettingValue&) noexcept = default;
	SfzSettingValue& operator= (const SfzSettingValue&) noexcept = default;

	static SfzSettingValue createInt(
		i32 value = 0,
		bool writeToFile = true,
		const SfzIntBounds& bounds = SfzIntBounds(0));

	static SfzSettingValue createFloat(
		f32 value = 0.0f,
		bool writeToFile = true,
		const SfzFloatBounds& bounds = SfzFloatBounds(0.0f));

	static SfzSettingValue createBool(
		bool value = false,
		bool writeToFile = true,
		const SfzBoolBounds& bounds = SfzBoolBounds(false));
};

// Setting class
// ------------------------------------------------------------------------------------------------

struct SfzSetting final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	SfzSetting() = delete;
	SfzSetting(const SfzSetting&) = delete;
	SfzSetting& operator= (const SfzSetting&) = delete;
	SfzSetting(SfzSetting&&) = delete;
	SfzSetting& operator= (SfzSetting&&) = delete;
	~SfzSetting() noexcept = default;

	SfzSetting(const char* section, const char* key) noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	const sfz::str32& section() const noexcept { return mSection; }
	const sfz::str48& key() const noexcept { return mKey; }
	const SfzSettingValue& value() const noexcept { return mValue; }
	SfzValueType type() const noexcept { return mValue.type; }

	i32 intValue() const noexcept;
	f32 floatValue() const noexcept;
	bool boolValue() const noexcept;

	const SfzIntBounds& intBounds() const noexcept;
	const SfzFloatBounds& floatBounds() const noexcept;
	const SfzBoolBounds& boolBounds() const noexcept;

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
	bool create(const SfzSettingValue& value) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	SfzSettingValue mValue;
	sfz::str32 mSection;
	sfz::str48 mKey;
};
