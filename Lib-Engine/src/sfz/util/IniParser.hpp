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

#include <limits>

#include <sfz.h>
#include <skipifzero_arrays.hpp>
#include <skipifzero_strings.hpp>

#include "sfz/strings/DynString.hpp"

namespace sfz {

using std::numeric_limits;

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
	const i32* getInt(const char* section, const char* key) const noexcept;

	/// Returns a pointer to the internal value of the specified section and key. This pointer
	/// should not be saved and should be immediately dereferenced. Returns nullpointer if value
	/// does not exist or if it is not of f32 type.
	const f32* getFloat(const char* section, const char* key) const noexcept;

	/// Returns a pointer to the internal value of the specified section and key. This pointer
	/// should not be saved and should be immediately dereferenced. Returns nullpointer if value
	/// does not exist or if it is not of bool type.
	const bool* getBool(const char* section, const char* key) const noexcept;

	// Setters
	// --------------------------------------------------------------------------------------------

	/// Sets the key in specified section to the given value. Creates the section and or key if
	/// they do not already exist. Eventual previous values will be overwritten and the type of
	/// the key might be altered.
	void setInt(const char* section, const char* key, i32 value) noexcept;

	/// Sets the key in specified section to the given value. Creates the section and or key if
	/// they do not already exist. Eventual previous values will be overwritten and the type of
	/// the key might be altered.
	void setFloat(const char* section, const char* key, f32 value) noexcept;

	/// Sets the key in specified section to the given value. Creates the section and or key if
	/// they do not already exist. Eventual previous values will be overwritten and the type of
	/// the key might be altered.
	void setBool(const char* section, const char* key, bool value) noexcept;

	// Sanitizers
	// --------------------------------------------------------------------------------------------

	/// Sanitizing int getter. Ensures that the the item exists and is inside the specified
	/// interval.
	i32 sanitizeInt(const char* section, const char* key,
	                    i32 defaultValue = 0,
	                    i32 minValue = numeric_limits<i32>::min(),
	                    i32 maxValue = numeric_limits<i32>::max()) noexcept;

	/// Sanitizing f32 getter. Ensures that the the item exists and is inside the specified
	/// interval.
	f32 sanitizeFloat(const char* section, const char* key,
	                    f32 defaultValue = 0.0f,
	                    f32 minValue = numeric_limits<f32>::min(),
	                    f32 maxValue = numeric_limits<f32>::max()) noexcept;

	/// Sanitizing bool getter. Ensures that the the item exists.
	bool sanitizeBool(const char* section, const char* key,
	                  bool defaultValue = false) noexcept;

	// Iterators
	// --------------------------------------------------------------------------------------------

	class ItemAccessor final {
	public:
		ItemAccessor(IniParser& iniParser, u32 sectionIndex, u32 keyIndex) noexcept;
		ItemAccessor(const ItemAccessor&) noexcept = default;
		ItemAccessor& operator= (const ItemAccessor&) noexcept = default;

		const char* getSection() const noexcept;
		const char* getKey() const noexcept;
		const i32* getInt() const noexcept;
		const f32* getFloat() const noexcept;
		const bool* getBool() const noexcept;

	private:
		IniParser* mIniParser = nullptr;
		u32 mSectionIndex = u32(~0);
		u32 mKeyIndex = u32(~0);
	};

	class Iterator final {
	public:
		Iterator(IniParser& iniParser, u32 sectionIndex, u32 keyIndex) noexcept;
		Iterator(const Iterator&) noexcept = default;
		Iterator& operator= (const Iterator&) noexcept = default;

		Iterator& operator++ () noexcept; // Pre-increment
		Iterator operator++ (int) noexcept; // Post-increment
		ItemAccessor operator* () noexcept;
		bool operator== (const Iterator& other) const noexcept;
		bool operator!= (const Iterator& other) const noexcept;

	private:
		IniParser* mIniParser = nullptr;
		u32 mSectionIndex = u32(~0);
		u32 mKeyIndex = u32(~0);
	};

	Iterator begin() noexcept;
	Iterator end() noexcept;

private:
	// Private helper classes
	// --------------------------------------------------------------------------------------------

	enum class ItemType : u32 {
		NUMBER, // int and/or f32
		BOOL,
		COMMENT_OWN_ROW,
		COMMENT_APPEND_PREVIOUS_ROW, // Appends a comment to the previous row
	};

	struct Item final {
		ItemType type;
		union {
			struct { i32 i; f32 f; };
			bool b;
		};
		str192 str; // Name or comment depending on ItemType
	};

	struct Section final {
		str64 name;
		Array<Item> items;
		Section() : items(0, getDefaultAllocator(), sfz_dbg("")) {}
		Section(const char* name) : name(name), items(0, getDefaultAllocator(), sfz_dbg("")) { }
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
	Array<Section> mSections;
};

} // namespace sfz
