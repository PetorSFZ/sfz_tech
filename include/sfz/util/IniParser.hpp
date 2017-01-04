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
#include "sfz/strings/DynString.hpp"
#include "sfz/strings/StackString.hpp"

namespace sfz {

using std::int32_t;
using std::numeric_limits;
using std::uint32_t;

// IniParser class
// ------------------------------------------------------------------------------------------------

/// Class used to parse ini files.
class IniParser final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	IniParser() noexcept = default;
	IniParser(const IniParser&) noexcept = default;
	IniParser& operator= (const IniParser&) noexcept = default;
	IniParser(IniParser&&) noexcept = default;
	IniParser& operator= (IniParser&&) noexcept = default;
	~IniParser() noexcept = default;

	/// Creates a IniParser with the specified path. Will not load or parse anything until load()
	/// is called.
	IniParser(const char* path) noexcept;
	
	// Loading and saving to file functions
	// --------------------------------------------------------------------------------------------

	/// Loads and parses the ini file at the stored path.
	bool load() noexcept;

	/// Saves the content of this IniParser to the ini file at the stored path.
	bool save() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	/// Returns a pointer to the internal value of the specified section and key. This pointer
	/// should not be saved and should be immediately dereferenced. Returns nullpointer if value
	/// does not exist or if it is not of int type.
	const int32_t* getInt(const char* section, const char* key) const noexcept;

	/// Returns a pointer to the internal value of the specified section and key. This pointer
	/// should not be saved and should be immediately dereferenced. Returns nullpointer if value
	/// does not exist or if it is not of float type.
	const float* getFloat(const char* section, const char* key) const noexcept;

	/// Returns a pointer to the internal value of the specified section and key. This pointer
	/// should not be saved and should be immediately dereferenced. Returns nullpointer if value
	/// does not exist or if it is not of bool type.
	const bool* getBool(const char* section, const char* key) const noexcept;

	// Setters
	// --------------------------------------------------------------------------------------------

	/// Sets the key in specified section to the given value. Creates the section and or key if
	/// they do not already exist. Eventual previous values will be overwritten and the type of
	/// the key might be altered.
	void setInt(const char* section, const char* key, int32_t value) noexcept;

	/// Sets the key in specified section to the given value. Creates the section and or key if
	/// they do not already exist. Eventual previous values will be overwritten and the type of
	/// the key might be altered.
	void setFloat(const char* section, const char* key, float value) noexcept;

	/// Sets the key in specified section to the given value. Creates the section and or key if
	/// they do not already exist. Eventual previous values will be overwritten and the type of
	/// the key might be altered.
	void setBool(const char* section, const char* key, bool value) noexcept;

	// Sanitizers
	// --------------------------------------------------------------------------------------------

	/// Sanitizing int getter. Ensures that the the item exists and is inside the specified
	/// interval.
	int32_t sanitizeInt(const char* section, const char* key,
	                    int32_t defaultValue = 0,
	                    int32_t minValue = numeric_limits<int32_t>::min(),
	                    int32_t maxValue = numeric_limits<int32_t>::max()) noexcept;
	
	/// Sanitizing float getter. Ensures that the the item exists and is inside the specified
	/// interval.
	float sanitizeFloat(const char* section, const char* key,
	                    float defaultValue = 0.0f,
	                    float minValue = numeric_limits<float>::min(),
	                    float maxValue = numeric_limits<float>::max()) noexcept;

	/// Sanitizing bool getter. Ensures that the the item exists.
	bool sanitizeBool(const char* section, const char* key,
	                  bool defaultValue = false) noexcept;

	// Iterators
	// --------------------------------------------------------------------------------------------

	class ItemAccessor final {
	public:
		ItemAccessor(IniParser& iniParser, uint32_t sectionIndex, uint32_t keyIndex) noexcept;
		ItemAccessor() noexcept = default;
		ItemAccessor(const ItemAccessor&) noexcept = default;
		ItemAccessor& operator= (const ItemAccessor&) noexcept = default;

		const char* getSection() const noexcept;
		const char* getKey() const noexcept;
		const int32_t* getInt() const noexcept;
		const float* getFloat() const noexcept;
		const bool* getBool() const noexcept;
	
	private:
		IniParser* mIniParser = nullptr;
		uint32_t mSectionIndex = uint32_t(~0);
		uint32_t mKeyIndex = uint32_t(~0);
	};

	class Iterator final {
	public:
		Iterator(IniParser& iniParser, uint32_t sectionIndex, uint32_t keyIndex) noexcept;
		Iterator(const Iterator&) noexcept = default;
		Iterator& operator= (const Iterator&) noexcept = default;

		Iterator& operator++ () noexcept; // Pre-increment
		Iterator operator++ (int) noexcept; // Post-increment
		ItemAccessor operator* () noexcept;
		bool operator== (const Iterator& other) const noexcept;
		bool operator!= (const Iterator& other) const noexcept;

	private:
		IniParser* mIniParser = nullptr;
		uint32_t mSectionIndex = uint32_t(~0);
		uint32_t mKeyIndex = uint32_t(~0);
	};

	Iterator begin() noexcept;
	Iterator end() noexcept;

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
