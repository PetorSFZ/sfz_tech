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

#include "sfz/containers/DynArray.hpp"
#include "sfz/containers/DynString.hpp"
#include "sfz/containers/StackString.hpp"

namespace sfz {

using std::int32_t;
using std::numeric_limits;
using std::uint32_t;

// IniParser class
// ------------------------------------------------------------------------------------------------

class IniParser final {
public:
	// Constants
	// --------------------------------------------------------------------------------------------
	
	static constexpr int32_t DEFAULT_INT = 0;
	static constexpr float DEFAULT_FLOAT = 0.0f;
	static constexpr bool DEFAULT_BOOL = false;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	IniParser() noexcept = default;
	IniParser(const IniParser&) noexcept = default;
	IniParser& operator= (const IniParser&) noexcept = default;
	~IniParser() noexcept = default;

	IniParser(const char* path) noexcept;
	
	// Loading and saving to file functions
	// --------------------------------------------------------------------------------------------

	bool load() noexcept;
	bool save() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	const int32_t* getInt(const char* section, const char* key) const noexcept;

	const float* getFloat(const char* section, const char* key) const noexcept;

	const bool* getBool(const char* section, const char* key) const noexcept;

	// Setters
	// --------------------------------------------------------------------------------------------

	void setInt(const char* section, const char* key, int32_t value) noexcept;
	void setFloat(const char* section, const char* key, float value) noexcept;
	void setBool(const char* section, const char* key, bool value) noexcept;

	// Sanitizers
	// --------------------------------------------------------------------------------------------

	int32_t sanitizeInt(const char* section, const char* key,
	                    int32_t defaultValue = DEFAULT_INT,
	                    int32_t minValue = numeric_limits<int32_t>::min(),
	                    int32_t maxValue = numeric_limits<int32_t>::max()) noexcept;

	float sanitizeFloat(const char* section, const char* key,
	                    float defaultValue = DEFAULT_FLOAT,
	                    float minValue = numeric_limits<float>::min(),
	                    float maxValue = numeric_limits<float>::max()) noexcept;

	bool sanitizeBool(const char* section, const char* key,
	                  bool defaultValue = DEFAULT_BOOL) noexcept;

private:
	// Private helper classes
	// --------------------------------------------------------------------------------------------

	enum class ItemType : uint32_t {
		NUMBER, // int and/or float
		BOOL,
		COMMENT_OWN_ROW,
		COMMENT_APPEND_PREVIOUS_ROW, // Appends a comment to the previous row
	};

	struct Item final {
		ItemType type;
		union {
			struct { int32_t i; float f; };
			bool b;
		};
		StackString192 str; // Name or comment depending on ItemType
	};

	struct Section final {
		StackString64 name;
		DynArray<Item> items;
		Section() = default;
		Section(const char* name) : name(name) { }
	};

	// Private methods
	// --------------------------------------------------------------------------------------------

	// Finds the specified item, returns nullptr if it doesn't exist
	const Item* findItem(const char* section, const char* key) const noexcept;

	// Finds the specified item, creates it if it doesn't exist
	Item* findItemEnsureExists(const char* section, const char* key) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	DynString mPath;
	DynArray<Section> mSections;
};

} // namespace sfz
