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

#ifdef __cplusplus
#include <cfloat>
#include <cstdint>
#else
#include <float.h>
#include <stdint.h>
#endif

#include "ph/Bool32.h"
#include "ph/ExternC.h"

// C value type enum
// ------------------------------------------------------------------------------------------------

const uint32_t PH_VALUE_TYPE_INT = 0;
const uint32_t PH_VALUE_TYPE_FLOAT = 1;
const uint32_t PH_VALUE_TYPE_BOOL = 2;

// C bounds structs
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	int32_t defaultValue;
	int32_t minValue;
	int32_t maxValue;
	int32_t step;
} phIntBounds;

PH_EXTERN_C
typedef struct {
	float defaultValue;
	float minValue;
	float maxValue;
} phFloatBounds;

PH_EXTERN_C
typedef struct {
	phBool32 defaultValue;
} phBoolBounds;

// C value structs
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	int32_t value;
	phIntBounds bounds;
} phIntValue;

PH_EXTERN_C
typedef struct {
	float value;
	phFloatBounds bounds;
} phFloatValue;

PH_EXTERN_C
typedef struct {
	phBool32 value;
	phBoolBounds bounds;
} phBoolValue;

// C setting value struct
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	uint32_t type;
	phBool32 writeToFile;
	union {
		phIntValue i;
		phFloatValue f;
		phBoolValue b;
	};
} phSettingValue;

// Config struct
// ------------------------------------------------------------------------------------------------

PH_EXTERN_C
typedef struct {
	/// Gets the specified Setting. Returns nullptr if it does not exist.
	const phSettingValue* (*getSetting)(const char* section, const char* key);

	// Set the value of specific type of setting. Value might be modified to fit into the range
	// allowed by the set bounds. Returns false if setting does not exist or if it is of another
	// type.
	phBool32 (*setInt)(const char* section, const char* key, int32_t value);
	phBool32 (*setFloat)(const char* section, const char* key, float value);
	phBool32 (*setBool)(const char* section, const char* key, phBool32 value);

	/// A sanitizer is basically a wrapper around createSetting, with the addition that it also
	/// ensures that the Setting is of the requested type and with values conforming to the
	/// specified bounds. If the setting does not exist or is of an incompatible type it will be
	/// set to the specified default value.

	const phSettingValue* (*sanitizeInt)(
		const char* section, const char* key,
		phBool32 writeToFile,
		const phIntBounds* bounds);

	const phSettingValue* (*sanitizeFloat)(
		const char* section, const char* key,
		phBool32 writeToFile,
		const phFloatBounds* bounds);

	const phSettingValue* (*sanitizeBool)(
		const char* section, const char* key,
		phBool32 writeToFile,
		const phBoolBounds* bounds);

} phConfig;

#ifdef __cplusplus
namespace ph {

// C++ value type enum
// ------------------------------------------------------------------------------------------------

enum class ValueType : uint32_t {
	INT = PH_VALUE_TYPE_INT,
	FLOAT = PH_VALUE_TYPE_FLOAT,
	BOOL = PH_VALUE_TYPE_BOOL,
};

// C++ Bounds structs
// ------------------------------------------------------------------------------------------------

struct IntBounds final {
	int32_t defaultValue;
	int32_t minValue;
	int32_t maxValue;
	int32_t step;

	IntBounds() noexcept = default;
	explicit IntBounds(
		int32_t defaultValue,
		int32_t minValue = INT32_MIN,
		int32_t maxValue = INT32_MAX,
		int32_t step = 1) noexcept
	:
		defaultValue(defaultValue),
		minValue(minValue),
		maxValue(maxValue),
		step(step)
	{ }

	phIntBounds* cPtr() noexcept { return reinterpret_cast<phIntBounds*>(this); }
	const phIntBounds* cPtr() const noexcept { return reinterpret_cast<const phIntBounds*>(this); }
};
static_assert(sizeof(phIntBounds) == sizeof(int32_t) * 4, "phIntBounds is padded");
static_assert(sizeof(phIntBounds) == sizeof(IntBounds), "IntBounds is padded");

struct FloatBounds final {
	float defaultValue;
	float minValue;
	float maxValue;

	FloatBounds() noexcept = default;
	explicit FloatBounds(
		float defaultValue,
		float minValue = FLT_MIN,
		float maxValue = FLT_MAX) noexcept
	:
		defaultValue(defaultValue),
		minValue(minValue),
		maxValue(maxValue)
	{ }

	phFloatBounds* cPtr() noexcept { return reinterpret_cast<phFloatBounds*>(this); }
	const phFloatBounds* cPtr() const noexcept { return reinterpret_cast<const phFloatBounds*>(this); }
};
static_assert(sizeof(phFloatBounds) == sizeof(float) * 3, "phFloatBounds is padded");
static_assert(sizeof(phFloatBounds) == sizeof(FloatBounds), "FloatBounds is padded");

struct BoolBounds final {
	Bool32 defaultValue;

	BoolBounds() noexcept = default;
	explicit BoolBounds(bool defaultValue) noexcept : defaultValue(defaultValue) { }

	phBoolBounds* cPtr() noexcept { return reinterpret_cast<phBoolBounds*>(this); }
	const phBoolBounds* cPtr() const noexcept { return reinterpret_cast<const phBoolBounds*>(this); }
};
static_assert(sizeof(phBoolBounds) == sizeof(phBool32), "phBoolBounds is padded");
static_assert(sizeof(phBoolBounds) == sizeof(BoolBounds), "BoolBounds is padded");

// C++ Value structs
// ------------------------------------------------------------------------------------------------

struct IntValue final {
	int32_t value;
	IntBounds bounds;

	IntValue() noexcept = default;
	explicit IntValue(int32_t value, const IntBounds& bounds = IntBounds(0)) noexcept
	:
		value(value),
		bounds(bounds)
	{ }
};
static_assert(sizeof(phIntValue) == sizeof(int32_t) * 5, "phIntValue is padded");
static_assert(sizeof(phIntValue) == sizeof(IntValue), "IntValue is padded");

struct FloatValue final {
	float value;
	FloatBounds bounds;

	FloatValue() noexcept = default;
	explicit FloatValue(float value, const FloatBounds& bounds = FloatBounds(0.0f)) noexcept
	:
		value(value),
		bounds(bounds)
	{ }
};
static_assert(sizeof(phFloatValue) == sizeof(float) * 4, "phFloatValue is padded");
static_assert(sizeof(phFloatValue) == sizeof(FloatValue), "FloatValue is padded");

struct BoolValue final {
	Bool32 value;
	BoolBounds bounds;

	BoolValue() noexcept = default;
	explicit BoolValue(bool value, const BoolBounds& bounds = BoolBounds(false)) noexcept
	:
		value(value),
		bounds(bounds)
	{ }
};
static_assert(sizeof(phBoolValue) == sizeof(phBool32) * 2, "phBoolValue is padded");
static_assert(sizeof(phBoolValue) == sizeof(BoolValue), "BoolValue is padded");

// C++ setting value struct
// ------------------------------------------------------------------------------------------------

struct SettingValue final {
	ValueType type;
	Bool32 writeToFile;
	union {
		IntValue i;
		FloatValue f;
		BoolValue b;
	};

	SettingValue() noexcept = default;
	SettingValue(const SettingValue&) noexcept = default;
	SettingValue& operator= (const SettingValue&) noexcept = default;

	static SettingValue createInt(
		int32_t value = 0,
		bool writeToFile = true,
		const IntBounds bounds = IntBounds(0));

	static SettingValue createFloat(
		float value = 0.0f,
		bool writeToFile = true,
		const FloatBounds bounds = FloatBounds(0.0f));

	static SettingValue createBool(
		bool value = false,
		bool writeToFile = true,
		const BoolBounds bounds = BoolBounds(false));
};
static_assert(sizeof(phSettingValue) == sizeof(uint32_t) * 7, "phSettingValue is padded");
static_assert(sizeof(phSettingValue) == sizeof(SettingValue), "SettingValue is padded");

inline SettingValue SettingValue::createInt(
	int32_t value,
	bool writeToFile,
	const IntBounds bounds)
{
	SettingValue setting;
	setting.type = ValueType::INT;
	setting.writeToFile = writeToFile;
	setting.i.value = value;
	setting.i.bounds = bounds;
	return setting;
}

inline SettingValue SettingValue::createFloat(
	float value,
	bool writeToFile,
	const FloatBounds bounds)
{
	SettingValue setting;
	setting.type = ValueType::FLOAT;
	setting.writeToFile = writeToFile;
	setting.f.value = value;
	setting.f.bounds = bounds;
	return setting;
}

inline SettingValue SettingValue::createBool(
	bool value,
	bool writeToFile,
	const BoolBounds bounds)
{
	SettingValue setting;
	setting.type = ValueType::BOOL;
	setting.writeToFile = writeToFile;
	setting.b.value = value;
	setting.b.bounds = bounds;
	return setting;
}

} // namespace ph
#endif
