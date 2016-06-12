// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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
#include <limits>
#include <map>
#include <string>

#include <sfz/containers/DynString.hpp>

namespace sfz {

using std::int32_t;
using std::numeric_limits;
using std::string;
using std::map;

// IniParser class
// ------------------------------------------------------------------------------------------------

class IniParser final {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	IniParser() = default;
	IniParser(const IniParser&) = default;
	IniParser& operator= (const IniParser&) = default;
	~IniParser() noexcept = default;

	IniParser(const char* path) noexcept;
	
	// Loading and saving to file functions
	// --------------------------------------------------------------------------------------------

	bool load() noexcept;
	bool save() noexcept;

	// Info about a specific item
	// --------------------------------------------------------------------------------------------

	bool itemExists(const string& section, const string& key) const noexcept;
	bool itemIsBool(const string& section, const string& key) const noexcept;
	bool itemIsInt(const string& section, const string& key) const noexcept;
	bool itemIsFloat(const string& section, const string& key) const noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	string getString(const string& section, const string& key,
	                 const string& defaultValue = "") const noexcept;

	bool getBool(const string& section, const string& key,
	             bool defaultValue = false) const noexcept;

	int32_t getInt(const string& section, const string& key,
	               int32_t defaultValue = 0) const noexcept;

	float getFloat(const string& section, const string& key,
	               float defaultValue = 0.0f) const noexcept;

	// Setters
	// --------------------------------------------------------------------------------------------

	void setString(const string& section, const string& key, const string& value) noexcept;
	void setBool(const string& section, const string& key, bool value) noexcept;
	void setInt(const string& section, const string& key, int32_t value) noexcept;
	void setFloat(const string& section, const string& key, float value) noexcept;

	// Sanitizers
	// --------------------------------------------------------------------------------------------

	string sanitizeString(const string& section, const string& key,
	                      const string& defaultValue = "") noexcept;

	bool sanitizeBool(const string& section, const string& key,
	                  bool defaultValue = false) noexcept;

	int32_t sanitizeInt(const string& section, const string& key,
	                    int32_t defaultValue = 0,
	                    int32_t minValue = numeric_limits<int32_t>::min(),
	                    int32_t maxValue = numeric_limits<int32_t>::max()) noexcept;

	float sanitizeFloat(const string& section, const string& key,
	                    float defaultValue = 0.0f,
	                    float minValue = numeric_limits<float>::min(),
	                    float maxValue = numeric_limits<float>::max()) noexcept;

private:
	DynString mPath;
	map<string,map<string,string>> mIniTree;
};

} // namespace sfz
