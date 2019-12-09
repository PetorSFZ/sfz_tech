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

#include "ph/config/Setting.hpp"

#include <skipifzero.hpp>

#include <sfz/math/MathSupport.hpp>

namespace sfz {

// SettingValue
// ------------------------------------------------------------------------------------------------

SettingValue SettingValue::createInt(
	int32_t value,
	bool writeToFile,
	const IntBounds& bounds)
{
	SettingValue setting;
	setting.type = ValueType::INT;
	setting.writeToFile = writeToFile;
	setting.i.value = value;
	setting.i.bounds = bounds;
	return setting;
}

SettingValue SettingValue::createFloat(
	float value,
	bool writeToFile,
	const FloatBounds& bounds)
{
	SettingValue setting;
	setting.type = ValueType::FLOAT;
	setting.writeToFile = writeToFile;
	setting.f.value = value;
	setting.f.bounds = bounds;
	return setting;
}

SettingValue SettingValue::createBool(
	bool value,
	bool writeToFile,
	const BoolBounds& bounds)
{
	SettingValue setting;
	setting.type = ValueType::BOOL;
	setting.writeToFile = writeToFile;
	setting.b.value = value;
	setting.b.bounds = bounds;
	return setting;
}

// Setting: Constructors & destructors
// ------------------------------------------------------------------------------------------------

Setting::Setting(const char* section, const char* key) noexcept
:
	mSection(section),
	mKey(key)
{
	mValue = SettingValue::createInt();
}

// Setting: Getters
// ------------------------------------------------------------------------------------------------

int32_t Setting::intValue() const noexcept
{
	sfz_assert(this->type() == ValueType::INT);
	return mValue.i.value;
}

float Setting::floatValue() const noexcept
{
	sfz_assert(this->type() == ValueType::FLOAT);
	return mValue.f.value;
}

bool Setting::boolValue() const noexcept
{
	sfz_assert(this->type() == ValueType::BOOL);
	return mValue.b.value;
}

const IntBounds& Setting::intBounds() const noexcept
{
	sfz_assert(this->type() == ValueType::INT);
	return mValue.i.bounds;
}

const FloatBounds& Setting::floatBounds() const noexcept
{
	sfz_assert(this->type() == ValueType::FLOAT);
	return mValue.f.bounds;
}

const BoolBounds& Setting::boolBounds() const noexcept
{
	sfz_assert(this->type() == ValueType::BOOL);
	return mValue.b.bounds;
}

// Setting: Setters
// ------------------------------------------------------------------------------------------------

bool Setting::setInt(int32_t value) noexcept
{
	if (this->type() != ValueType::INT) return false;

	// Clamp value
	value = sfz::clamp(value, mValue.i.bounds.minValue, mValue.i.bounds.maxValue);

	// Ensure value is of a valid step
	int64_t diff = int64_t(value) - int64_t(mValue.i.bounds.minValue);
	double stepsFractions = double(diff) / double(mValue.i.bounds.step);
	int64_t steps = int64_t(std::round(stepsFractions));
	value = int32_t(int64_t(mValue.i.bounds.minValue) + steps * int64_t(mValue.i.bounds.step));
	mValue.i.value = sfz::clamp(value, mValue.i.bounds.minValue, mValue.i.bounds.maxValue);

	return true;
}

bool Setting::setFloat(float value) noexcept
{
	if (this->type() != ValueType::FLOAT) return false;
	mValue.f.value = sfz::clamp(value, mValue.f.bounds.minValue, mValue.f.bounds.maxValue);
	return true;
}

bool Setting::setBool(bool value) noexcept
{
	if (this->type() != ValueType::BOOL) return false;
	mValue.b.value = value;
	return true;
}

void Setting::setWriteToFile(bool writeToFile) noexcept
{
	mValue.writeToFile = writeToFile;
}

bool Setting::create(const SettingValue& value) noexcept
{
	switch (value.type) {
	case ValueType::INT:
		{
			int32_t val = value.i.value;
			const IntBounds& bounds = value.i.bounds;
			if (bounds.minValue >= bounds.maxValue) return false;
			if (val < bounds.minValue || bounds.maxValue < val) return false;
			if (bounds.defaultValue < bounds.minValue || bounds.maxValue < bounds.defaultValue)
				return false;

			int64_t diff = int64_t(val) - int64_t(bounds.minValue);
			if((diff % int64_t(bounds.step)) != 0) return false;

			diff = int64_t(bounds.defaultValue) - int64_t(bounds.minValue);
			if((diff % int64_t(bounds.step)) != 0) return false;

			mValue = value;
			return true;
		}
		break;
	case ValueType::FLOAT:
		{
			float val = value.f.value;
			const FloatBounds& bounds = value.f.bounds;
			if (bounds.minValue >= bounds.maxValue) return false;
			if (val < bounds.minValue || bounds.maxValue < val) return false;
			if (bounds.defaultValue < bounds.minValue || bounds.maxValue < bounds.defaultValue)
				return false;

			mValue = value;
			return true;
		}
		break;
	case ValueType::BOOL:
		{
			mValue = value;
			return true;
		}
	}

	// Invalid type
	return false;
}

} // namespace sfz
