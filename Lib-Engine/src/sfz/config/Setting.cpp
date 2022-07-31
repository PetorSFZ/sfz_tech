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

#include "sfz/config/Setting.hpp"

#include <skipifzero.hpp>
#include <skipifzero_math.hpp>

// SettingValue
// ------------------------------------------------------------------------------------------------

SfzSettingValue SfzSettingValue::createInt(
	i32 value,
	bool writeToFile,
	const SfzIntBounds& bounds)
{
	SfzSettingValue setting;
	setting.type = SfzValueType::INT;
	setting.writeToFile = writeToFile;
	setting.i.value = value;
	setting.i.bounds = bounds;
	return setting;
}

SfzSettingValue SfzSettingValue::createFloat(
	f32 value,
	bool writeToFile,
	const SfzFloatBounds& bounds)
{
	SfzSettingValue setting;
	setting.type = SfzValueType::FLOAT;
	setting.writeToFile = writeToFile;
	setting.f.value = value;
	setting.f.bounds = bounds;
	return setting;
}

SfzSettingValue SfzSettingValue::createBool(
	bool value,
	bool writeToFile,
	const SfzBoolBounds& bounds)
{
	SfzSettingValue setting;
	setting.type = SfzValueType::BOOL;
	setting.writeToFile = writeToFile;
	setting.b.value = value;
	setting.b.bounds = bounds;
	return setting;
}

// Setting: Constructors & destructors
// ------------------------------------------------------------------------------------------------

SfzSetting::SfzSetting(const char* section, const char* key) noexcept
:
	mSection(section),
	mKey(key)
{
	mValue = SfzSettingValue::createInt();
}

// Setting: Getters
// ------------------------------------------------------------------------------------------------

i32 SfzSetting::intValue() const noexcept
{
	sfz_assert(this->type() == SfzValueType::INT);
	return mValue.i.value;
}

f32 SfzSetting::floatValue() const noexcept
{
	sfz_assert(this->type() == SfzValueType::FLOAT);
	return mValue.f.value;
}

bool SfzSetting::boolValue() const noexcept
{
	sfz_assert(this->type() == SfzValueType::BOOL);
	return mValue.b.value;
}

const SfzIntBounds& SfzSetting::intBounds() const noexcept
{
	sfz_assert(this->type() == SfzValueType::INT);
	return mValue.i.bounds;
}

const SfzFloatBounds& SfzSetting::floatBounds() const noexcept
{
	sfz_assert(this->type() == SfzValueType::FLOAT);
	return mValue.f.bounds;
}

const SfzBoolBounds& SfzSetting::boolBounds() const noexcept
{
	sfz_assert(this->type() == SfzValueType::BOOL);
	return mValue.b.bounds;
}

// Setting: Setters
// ------------------------------------------------------------------------------------------------

bool SfzSetting::setInt(i32 value) noexcept
{
	if (this->type() != SfzValueType::INT) return false;

	// Clamp value
	value = sfz::clamp(value, mValue.i.bounds.minValue, mValue.i.bounds.maxValue);

	// Ensure value is of a valid step
	i64 diff = i64(value) - i64(mValue.i.bounds.minValue);
	f64 stepsFractions = f64(diff) / f64(mValue.i.bounds.step);
	i64 steps = i64(::round(stepsFractions));
	value = i32(i64(mValue.i.bounds.minValue) + steps * i64(mValue.i.bounds.step));
	mValue.i.value = sfz::clamp(value, mValue.i.bounds.minValue, mValue.i.bounds.maxValue);

	return true;
}

bool SfzSetting::setFloat(f32 value) noexcept
{
	if (this->type() != SfzValueType::FLOAT) return false;
	mValue.f.value = sfz::clamp(value, mValue.f.bounds.minValue, mValue.f.bounds.maxValue);
	return true;
}

bool SfzSetting::setBool(bool value) noexcept
{
	if (this->type() != SfzValueType::BOOL) return false;
	mValue.b.value = value;
	return true;
}

void SfzSetting::setWriteToFile(bool writeToFile) noexcept
{
	mValue.writeToFile = writeToFile;
}

bool SfzSetting::create(const SfzSettingValue& value) noexcept
{
	switch (value.type) {
	case SfzValueType::INT:
		{
			i32 val = value.i.value;
			const SfzIntBounds& bounds = value.i.bounds;
			if (bounds.minValue >= bounds.maxValue) return false;
			if (val < bounds.minValue || bounds.maxValue < val) return false;
			if (bounds.defaultValue < bounds.minValue || bounds.maxValue < bounds.defaultValue)
				return false;

			i64 diff = i64(val) - i64(bounds.minValue);
			if((diff % i64(bounds.step)) != 0) return false;

			diff = i64(bounds.defaultValue) - i64(bounds.minValue);
			if((diff % i64(bounds.step)) != 0) return false;

			mValue = value;
			return true;
		}
		break;
	case SfzValueType::FLOAT:
		{
			f32 val = value.f.value;
			const SfzFloatBounds& bounds = value.f.bounds;
			if (bounds.minValue >= bounds.maxValue) return false;
			if (val < bounds.minValue || bounds.maxValue < val) return false;
			if (bounds.defaultValue < bounds.minValue || bounds.maxValue < bounds.defaultValue)
				return false;

			mValue = value;
			return true;
		}
		break;
	case SfzValueType::BOOL:
		{
			mValue = value;
			return true;
		}
	}

	// Invalid type
	return false;
}
