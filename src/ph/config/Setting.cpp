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

#include <limits>

#include <sfz/math/MathSupport.hpp>

namespace ph {

// Setting: Constructors & destructors
// ------------------------------------------------------------------------------------------------


Setting::Setting(const char* section, const char* key) noexcept 
:
	mSection(section),
	mKey(key)
{
	setInt(0);
}

Setting::Setting(const char* section, const char* key, int32_t value) noexcept
:
	mSection(section),
	mKey(key)
{
	setInt(value);
}

Setting::Setting(const char* section, const char* key, float value) noexcept
:
	mSection(section),
	mKey(key)
{
	setFloat(value);
}

Setting::Setting(const char* section, const char* key, bool value) noexcept
:
	mSection(section),
	mKey(key)
{
	setBool(value);
}

// Setting: Getters
// ------------------------------------------------------------------------------------------------

bool Setting::isBoolValue() const noexcept
{
	return value.type == VALUE_TYPE_INT &&
	       value.i.minValue == 0 &&
	       value.i.maxValue == 1;
}

int32_t Setting::intValue() const noexcept
{
	switch (value.type) {
	case VALUE_TYPE_INT: return value.i.value;
	case VALUE_TYPE_FLOAT: return int32_t(std::round(value.f.value));
	}
	return 0;
}

float Setting::floatValue() const noexcept
{
	switch (value.type) {
	case VALUE_TYPE_INT: return float(value.i.value);
	case VALUE_TYPE_FLOAT: return value.f.value;
	}
	return 0.0f;
}

bool Setting::boolValue() const noexcept
{

	switch (value.type) {
	case VALUE_TYPE_INT: return value.i.value != 0;
	case VALUE_TYPE_FLOAT: return value.f.value != 0.0f; // TODO: Could potentially check bitpattern using "i"
	}
	return false;
}

int32_t Setting::intMinValue() const noexcept
{
	switch (value.type) {
	case VALUE_TYPE_INT: return value.i.minValue;
	case VALUE_TYPE_FLOAT: return int32_t(std::round(value.f.minValue));
	}
	return std::numeric_limits<int32_t>::min();
}

int32_t Setting::intMaxValue() const noexcept
{
	switch (value.type) {
	case VALUE_TYPE_INT: return value.i.maxValue;
	case VALUE_TYPE_FLOAT: return int32_t(std::round(value.f.maxValue));
	}
	return std::numeric_limits<int32_t>::max();
}

float Setting::floatMinValue() const noexcept
{
	switch (value.type) {
	case VALUE_TYPE_INT: return float(value.i.minValue);
	case VALUE_TYPE_FLOAT: return value.f.minValue;
	}
	return std::numeric_limits<float>::min();
}

float Setting::floatMaxValue() const noexcept
{
	switch (value.type) {
	case VALUE_TYPE_INT: return float(value.i.maxValue);
	case VALUE_TYPE_FLOAT: return value.f.maxValue;
	}
	return std::numeric_limits<float>::max();
}

// Setting: Setters
// ------------------------------------------------------------------------------------------------

bool Setting::setInt(int32_t value) noexcept
{
	int32_t minValue = intMinValue();
	int32_t maxValue = intMaxValue();
	int32_t clampedValue = sfz::clamp(value, minValue, maxValue);
	bool clamped = clampedValue != value;
	
	this->value.type = VALUE_TYPE_INT;
	this->value.i.value = clampedValue;
	this->value.i.minValue = minValue;
	this->value.i.maxValue = maxValue;

	return clamped;
}

bool Setting::setFloat(float value) noexcept
{
	float minValue = floatMinValue();
	float maxValue = floatMaxValue();
	float clampedValue = sfz::clamp(value, minValue, maxValue);
	bool clamped = clampedValue != value;

	this->value.type = VALUE_TYPE_FLOAT;
	this->value.f.value = clampedValue;
	this->value.f.minValue = minValue;
	this->value.f.maxValue = maxValue;

	return clamped;
}

void Setting::setInt(int32_t value, int32_t minValue, int32_t maxValue) noexcept
{
	this->value.type = VALUE_TYPE_INT;
	this->value.i.value = value;
	this->value.i.minValue = minValue;
	this->value.i.maxValue = maxValue;
}

void Setting::setFloat(float value, float minValue, float maxValue) noexcept
{
	this->value.type = VALUE_TYPE_FLOAT;
	this->value.f.value = value;
	this->value.f.minValue = minValue;
	this->value.f.maxValue = maxValue;
}

void Setting::setBool(bool value) noexcept
{
	this->value.type = VALUE_TYPE_INT;
	this->value.i.value = value ? 1 : 0;
	this->value.i.minValue = 0;
	this->value.i.maxValue = 1;
}

} // namespace ph
