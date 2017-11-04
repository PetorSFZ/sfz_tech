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
#include <cstdint>
using std::int32_t;
#else
#include <stdint.h>
#endif

// C interface
#ifdef __cplusplus
extern "C" {
#endif

// ValueType enum
// ------------------------------------------------------------------------------------------------

typedef enum {
	VALUE_TYPE_INT,
	VALUE_TYPE_FLOAT
} phValueType;

// Value structs
// ------------------------------------------------------------------------------------------------

typedef struct {
	float value;
	float minValue, maxValue;
} phFloatValue;

typedef struct {
	int32_t value;
	int32_t minValue, maxValue;
} phIntValue;

typedef struct {
	phValueType type;
	union {
		phIntValue i;
		phFloatValue f;
	};
} phSettingValue;

// Config struct
// ------------------------------------------------------------------------------------------------

typedef struct {
	/// Gets the specified Setting. If it does not exist it will be created (type int with value 0).
	phSettingValue* (*getCreateSetting)(const char* section, const char* key);

	/// Gets the specified Setting. Returns nullptr if it does not exist.
	phSettingValue* (*getSetting)(const char* section, const char* key);
} phConfig;


// End C interface
#ifdef __cplusplus
}
#endif
