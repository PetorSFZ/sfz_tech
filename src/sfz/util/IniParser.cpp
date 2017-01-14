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

#include "sfz/util/IniParser.hpp"

#include <algorithm>
#include <cctype> // std::tolower()

#include "sfz/Assert.hpp"
#include "sfz/math/MathSupport.hpp"
#include "sfz/util/IO.hpp"

namespace sfz {

// Static functions
// ------------------------------------------------------------------------------------------------

static bool isWhitespace(char c) noexcept
{
	return c == ' ' || c == '\t';
}

static void printLoadError(const DynString& path, uint32_t line, const char* message) noexcept
{
	printErrorMessage("Failed to load \"%s\" at line %u: %s\n", path.str(), line, message);
}

// IniParser: Constructors & destructors
// ------------------------------------------------------------------------------------------------

IniParser::IniParser(const char* path) noexcept
:
	mPath(path)
{ }
	
// IniParser: Loading and saving to file functions
// ------------------------------------------------------------------------------------------------

bool IniParser::load() noexcept
{
	// Check if a path is available
	if (mPath.str() == nullptr) {
		printErrorMessage("Can't load ini file without path.");
		return false;
	}

	// Read file
	DynString fileContents = readTextFile(mPath.str());
	if (fileContents.size() == 0) {
		printLoadError(mPath, 0, "Ini file is empty.");
		return false;
	}	

	// Retrieve line information
	struct LineInfo final {
		uint32_t lineNumber = uint32_t(~0);
		uint32_t startIndex = uint32_t(~0);
		uint32_t length = uint32_t(~0);
	};
	DynArray<LineInfo> lines(256);
	{
		LineInfo tmp;
		tmp.lineNumber = 1;
		for (uint32_t i = 0; i < fileContents.size(); i++) {
			char c = fileContents.str()[i];

			// Special case when line has not yet started
			if (tmp.startIndex == uint32_t(~0)) {
				
				// Trim whitespace in beginning of line
				if (isWhitespace(c)) continue;

				// Skip empty rows
				if (c == '\n') {
					tmp.lineNumber += 1;
					continue;
				}

				// Start line
				tmp.startIndex = i;
				tmp.length = 1;
			}

			// Continue if whitespace
			if (isWhitespace(c)) continue;

			// Add line 
			if (c == '\n') {
				lines.add(tmp);
				tmp.lineNumber += 1;
				tmp.startIndex = uint32_t(~0);
				tmp.length = uint32_t(~0);
				continue;
			}

			// Increase length
			tmp.length = i - tmp.startIndex + 1;
		}
	}

	// Create temporary parse tree and add the first initial empty section
	DynArray<Section> newSections(64);
	newSections.add(Section(""));

	// Parse contents of ini file
	for (LineInfo line : lines) {
		const char* startPtr = fileContents.str() + line.startIndex;
		char firstChar = startPtr[0];

		// Comment
		if (firstChar == ';') {
			if ((line.length - 1) > 191) {
				printLoadError(mPath, line.lineNumber, "Too long comment, please split into multiple rows.");
				return false;
			}
			Item comment;
			comment.type = ItemType::COMMENT_OWN_ROW;
			comment.str.insertChars(startPtr + 1, (size_t)std::min(uint32_t(191), line.length - 1));
			newSections.last().items.add(comment);
			continue;
		}

		// Section
		else if (firstChar == '[') {
			
			// Find length of section name
			uint32_t index = 1;
			bool foundEndToken = false;
			while (index < line.length) {
				if (startPtr[index] == ']') {
					foundEndToken = true;
					break;
				}
				index += 1;
			}
			uint32_t nameLength = index - 1;

			if (!foundEndToken) {
				printLoadError(mPath, line.lineNumber, "Missing ']'.");
				return false;
			}
			if (nameLength > 191) {
				printLoadError(mPath, line.lineNumber, "Too long section name.");
				return false;
			}

			// Insert section
			Section tmpSection;
			tmpSection.name.insertChars(startPtr + 1, nameLength);
			newSections.add(tmpSection);

			// Find start of optional comment
			index += 1; // Next token after ']'
			bool foundCommentStart = false;
			while (index < line.length) {
				char c = startPtr[index];
				if (c == ';') {
					foundCommentStart = true;
					index += 1;
					break;
				}
				if (!isWhitespace(c)) {
					printLoadError(mPath, line.lineNumber, "Invalid tokens after ']'.");
					return false;
				}
				index += 1;
			}

			// Add optional comment if it exists
			if (foundCommentStart) {
				uint32_t commentLength = line.length - index;

				if (commentLength > 191) {
					printLoadError(mPath, line.lineNumber, "Too long comment, please split into multiple rows.");
					return false;
				}

				Item item;
				item.type = ItemType::COMMENT_APPEND_PREVIOUS_ROW;
				item.str.insertChars(startPtr + index, commentLength);
				newSections.last().items.add(item);
			}
		}

		// Item
		else {
			
			// Find name-value separator
			uint32_t index = 0;
			uint32_t lastNameCharIndex = 0;
			bool separatorFound = false;
			while (index < line.length) {
				char c = startPtr[index];
				if (c == '=') {
					index += 1;
					separatorFound = true;
					break;
				}
				if (!isWhitespace(c)) {
					if (index > 0 && ((lastNameCharIndex + 1) != index)) {
						printLoadError(mPath, line.lineNumber, "White space in item name");
						return false;
					}
					lastNameCharIndex = index;
				}
				index += 1;
			}

			if (!separatorFound) {
				printLoadError(mPath, line.lineNumber, "Missing '='.");
				return false;
			}
	
			uint32_t nameLength = lastNameCharIndex + 1;
			if (nameLength > 191) {
				printLoadError(mPath, line.lineNumber, "Too long item name.");
				return false;
			}

			// Insert name into item
			Item item;
			item.str.insertChars(startPtr, nameLength);

			// Find first char of value
			uint32_t valueIndex = uint32_t(~0);
			while (index < line.length) {
				if (!isWhitespace(startPtr[index])) {
					valueIndex = index;
					break;
				}
				index += 1;
			}

			if (valueIndex == uint32_t(~0)) {
				printLoadError(mPath, line.lineNumber, "No value.");
				return false;
			}

			char firstValueChar = (char)std::tolower(startPtr[index]);

			// Check if value is bool
			if (firstValueChar == 't' || firstValueChar == 'f') {
				if (strncmp(startPtr + index, "true", 4) == 0) {
					item.b = true;
				} else if (strncmp(startPtr + index, "false", 5) == 0) {
					item.b = false;
				} else {
					printLoadError(mPath, line.lineNumber, "Invalid value.");
					return false;
				}
				item.type = ItemType::BOOL;
			}
			// Otherwise assume value is a number
			else {
				item.type = ItemType::NUMBER;
				item.i = std::atoi(startPtr + index);
				item.f = (float)std::atof(startPtr + index);
				int floatToint = (int)item.f;
				if (item.i != floatToint) {
					printLoadError(mPath, line.lineNumber, "Invalid value.");
					return false;
				}
			}
			
			// Add item to current section
			newSections.last().items.add(item);

			// Find start of optional comment
			bool foundCommentStart = false;
			while (index < line.length) {
				if (startPtr[index] == ';') {
					foundCommentStart = true;
					index += 1;
					break;
				}
				index += 1;
			}

			// Add optional comment if it exists
			if (foundCommentStart) {
				uint32_t commentLength = line.length - index;

				if (commentLength > 191) {
					printLoadError(mPath, line.lineNumber, "Too long comment, please split into multiple rows.");
					return false;
				}

				Item commentItem;
				commentItem.type = ItemType::COMMENT_APPEND_PREVIOUS_ROW;
				commentItem.str.insertChars(startPtr + index, commentLength);
				newSections.last().items.add(commentItem);
			}
		}
	}

	// Swap the new parse tree with the old one and return
	mSections = std::move(newSections);
	return true;
}

bool IniParser::save() noexcept
{
	// Delete current file
	deleteFile(mPath.str());
	
	// Calculate upper bound for the memory requirements of the string representation
	uint32_t memoryReqs = 0;
	for (auto& sect : mSections) {
		memoryReqs += 200;
		memoryReqs += sect.items.size() * 200;
	}

	// Create string representation from parse tree
	DynString str("", memoryReqs);
	for (uint32_t sectIndex = 0; sectIndex < mSections.size(); sectIndex++) {
		Section& section = mSections[sectIndex];

		// Print section header
		if (section.name != "") {
			str.printfAppend("[%s]", section.name.str);
			if (section.items.size() >= 1 && section.items[0].type == ItemType::COMMENT_APPEND_PREVIOUS_ROW) {
				str.printfAppend(" ;%s", section.items[0].str.str);
			}
			str.printfAppend("\n");
		}

		for (uint32_t i = 0; i < section.items.size(); i++) {
			Item& item = section.items[i];
	
			// Print items content to string
			switch (item.type) {
			case ItemType::NUMBER:
				if (sfz::approxEqual(std::round(item.f), item.f)) {
					str.printfAppend("%s=%i", item.str.str, item.i);
				} else {
					str.printfAppend("%s=%f", item.str.str, item.f);
				}
				break;
			case ItemType::BOOL:
				str.printfAppend("%s=%s", item.str.str, item.b ? "true" : "false");
				break;
			case ItemType::COMMENT_OWN_ROW:
				str.printfAppend(";%s", item.str.str);
				break;
			case ItemType::COMMENT_APPEND_PREVIOUS_ROW:
				continue;
			}

			// Append comment if next item is comment to append
			if ((i + 1) < section.items.size()) {
				Item& nextItem = section.items[i + 1];
				if (nextItem.type == ItemType::COMMENT_APPEND_PREVIOUS_ROW) {
					str.printfAppend(" ;%s", nextItem.str.str);
				}
			}
			str.printfAppend("\n");
		}

		if ((sectIndex + 1) < mSections.size()) {
			str.printfAppend("\n");
		}
	}

	// Write string to file
	bool success = sfz::writeBinaryFile(mPath.str(), reinterpret_cast<const uint8_t*>(str.str()), str.size());
	if (!success) {
		sfz::printErrorMessage("Failed to write ini file \"%s\"", mPath.str());
		return false;
	}

	return true;
}

// IniParser: Getters
// ------------------------------------------------------------------------------------------------

const int32_t* IniParser::getInt(const char* section, const char* key) const noexcept
{
	// Attempt to find item, return nullptr if it doesn't exist
	const Item* itemPtr = this->findItem(section, key);
	if (itemPtr == nullptr) return nullptr;
	if (itemPtr->type != ItemType::NUMBER) return nullptr;

	// Return pointer to value
	return &itemPtr->i;
}

const float* IniParser::getFloat(const char* section, const char* key) const noexcept
{
	// Attempt to find item, return nullptr if it doesn't exist
	const Item* itemPtr = this->findItem(section, key);
	if (itemPtr == nullptr) return nullptr;
	if (itemPtr->type != ItemType::NUMBER) return nullptr;

	// Return pointer to value
	return &itemPtr->f;
}

const bool* IniParser::getBool(const char* section, const char* key) const noexcept
{
	// Attempt to find item, return nullptr if it doesn't exist
	const Item* itemPtr = this->findItem(section, key);
	if (itemPtr == nullptr) return nullptr;
	if (itemPtr->type != ItemType::BOOL) return nullptr;

	// Return pointer to value
	return &itemPtr->b;
}

// IniParser: Setters
// ------------------------------------------------------------------------------------------------

void IniParser::setInt(const char* section, const char* key, int32_t value) noexcept
{
	Item* itemPtr = this->findItemEnsureExists(section, key);
	itemPtr->type = ItemType::NUMBER;
	itemPtr->i = value;
	itemPtr->f = static_cast<float>(value);
}

void IniParser::setFloat(const char* section, const char* key, float value) noexcept
{
	Item* itemPtr = this->findItemEnsureExists(section, key);
	itemPtr->type = ItemType::NUMBER;
	itemPtr->f = value;
	itemPtr->i = static_cast<int32_t>(std::round(value));
}

void IniParser::setBool(const char* section, const char* key, bool value) noexcept
{
	Item* itemPtr = this->findItemEnsureExists(section, key);
	itemPtr->type = ItemType::BOOL;
	itemPtr->b = value;
}

// IniParser: Sanitizers
// ------------------------------------------------------------------------------------------------

int32_t IniParser::sanitizeInt(const char* section, const char* key,
                               int32_t defaultValue, int32_t minValue, int32_t maxValue) noexcept
{
	sfz_assert_debug(minValue <= maxValue);
	const Item* itemPtr = this->findItem(section, key);
	if (itemPtr == nullptr || itemPtr->type != ItemType::NUMBER) {
		this->setInt(section, key, defaultValue);
		itemPtr = this->findItem(section, key);
	}

	int32_t value = itemPtr->i;
	if (value > maxValue) {
		value = maxValue;
		this->setInt(section, key, value);
	} else if (value < minValue) {
		value = minValue;
		this->setInt(section, key, value);
	}

	return value;
}

float IniParser::sanitizeFloat(const char* section, const char* key,
                               float defaultValue, float minValue, float maxValue) noexcept
{
	sfz_assert_debug(minValue <= maxValue);
	const Item* itemPtr = this->findItem(section, key);
	if (itemPtr == nullptr || itemPtr->type != ItemType::NUMBER) {
		this->setFloat(section, key, defaultValue);
		itemPtr = this->findItem(section, key);
	}

	float value = itemPtr->f;
	if (value > maxValue) {
		value = maxValue;
		this->setFloat(section, key, value);
	} else if (value < minValue) {
		value = minValue;
		this->setFloat(section, key, value);
	}

	return value;
}

bool IniParser::sanitizeBool(const char* section, const char* key, bool defaultValue) noexcept
{
	const Item* itemPtr = this->findItem(section, key);
	if (itemPtr == nullptr || itemPtr->type != ItemType::BOOL) {
		this->setBool(section, key, defaultValue);
		itemPtr = this->findItem(section, key);
	}
	return itemPtr->b;
}

// IniParser: Iterators (ItemAccessor)
// ------------------------------------------------------------------------------------------------

IniParser::ItemAccessor::ItemAccessor(IniParser& iniParser, uint32_t sectionIndex, uint32_t keyIndex) noexcept
:
	mIniParser(&iniParser),
	mSectionIndex(sectionIndex),
	mKeyIndex(keyIndex)
{
	sfz_assert_debug(mIniParser != nullptr);
	sfz_assert_debug(mSectionIndex < mIniParser->mSections.size());
	sfz_assert_debug(mKeyIndex < mIniParser->mSections[mSectionIndex].items.size());
}

const char* IniParser::ItemAccessor::getSection() const noexcept
{
	return mIniParser->mSections[mSectionIndex].name.str;
}

const char* IniParser::ItemAccessor::getKey() const noexcept
{
	return mIniParser->mSections[mSectionIndex].items[mKeyIndex].str.str;
}

const int32_t* IniParser::ItemAccessor::getInt() const noexcept
{
	const Item& item = mIniParser->mSections[mSectionIndex].items[mKeyIndex];
	if (item.type == ItemType::NUMBER) {
		return &item.i;
	}
	return nullptr;
}

const float* IniParser::ItemAccessor::getFloat() const noexcept
{
	const Item& item = mIniParser->mSections[mSectionIndex].items[mKeyIndex];
	if (item.type == ItemType::NUMBER) {
		return &item.f;
	}
	return nullptr;
}

const bool* IniParser::ItemAccessor::getBool() const noexcept
{
	const Item& item = mIniParser->mSections[mSectionIndex].items[mKeyIndex];
	if (item.type == ItemType::BOOL) {
		return &item.b;
	}
	return nullptr;
}

// IniParser: Iterators (Iterator)
// ------------------------------------------------------------------------------------------------

IniParser::Iterator::Iterator(IniParser& iniParser, uint32_t sectionIndex, uint32_t keyIndex) noexcept
:
	mIniParser(&iniParser),
	mSectionIndex(sectionIndex),
	mKeyIndex(keyIndex)
{ }

IniParser::Iterator& IniParser::Iterator::operator++ () noexcept
{
	// Go through IniParser until we find next item
	uint32_t& s = mSectionIndex;
	uint32_t& k = mKeyIndex;
	k++;
	while (s < mIniParser->mSections.size()) {
		const Section& sect = mIniParser->mSections[s];
		while (k < sect.items.size()) {
			const Item& item = sect.items[k];
			if (item.type == ItemType::NUMBER || item.type == ItemType::BOOL) {
				mSectionIndex = s;
				mKeyIndex = k;
				return *this;
			}
			k++;
		}
		k = 0;
		s++;
	}

	// Did not find any more items, set to end
	s = uint32_t(~0);
	k = uint32_t(~0);
	return *this;
}

IniParser::Iterator IniParser::Iterator::operator++ (int) noexcept
{
	auto copy = *this;
	++(*this);
	return copy;
}

IniParser::ItemAccessor IniParser::Iterator::operator* () noexcept
{
	sfz_assert_debug(mIniParser != nullptr);
	sfz_assert_debug(mSectionIndex < mIniParser->mSections.size());
	sfz_assert_debug(mKeyIndex < mIniParser->mSections[mSectionIndex].items.size());
	return ItemAccessor(*mIniParser, mSectionIndex, mKeyIndex);
}

bool IniParser::Iterator::operator== (const Iterator& other) const noexcept
{
	return this->mIniParser == other.mIniParser &&
	       this->mSectionIndex == other.mSectionIndex &&
	       this->mKeyIndex == other.mKeyIndex;
}

bool IniParser::Iterator::operator!= (const Iterator& other) const noexcept
{
	return !(*this == other);
}

// IniParser: Iterators (Iterator methods)
// ------------------------------------------------------------------------------------------------

IniParser::Iterator IniParser::begin() noexcept
{
	if (mSections.size() == 0) return this->end();
	
	// Check if first position is an item
	Iterator it(*this, 0, 0);
	if (mSections[0].items.size() > 0) {
		ItemType t = mSections[0].items[0].type;
		if (t == ItemType::NUMBER || t == ItemType::BOOL) return it;
	}

	// Iterate to first item
	it++;
	return it;
}

IniParser::Iterator IniParser::end() noexcept
{
	return Iterator(*this, uint32_t(~0), uint32_t(~0));
}

// IniParser: Private methods
// ------------------------------------------------------------------------------------------------

const IniParser::Item* IniParser::findItem(const char* section, const char* key) const noexcept
{
	for (const Section& sect : mSections) {
		if (sect.name != section) continue;
		for (const Item& item : sect.items) {
			if (item.str != key) continue;
			return &item;
		}
		return nullptr;
	}
	return nullptr;
}

IniParser::Item* IniParser::findItemEnsureExists(const char* section, const char* key) noexcept
{
	// Find section
	Section* sectPtr = nullptr;
	for (Section& sect : mSections) {
		if (sect.name == section) {
			sectPtr = &sect;
			break;
		}
	}

	// Create section if it does not exist
	if (sectPtr == nullptr) {
		mSections.add(Section(section));
		sectPtr = &mSections.last();
	}

	// Find item
	Item* itemPtr = nullptr;
	for (Item& item : sectPtr->items) {
		if (item.str == key) {
			itemPtr = &item;
			break;
		}
	}

	// Create item if it does not exist
	if (itemPtr == nullptr) {
		Item tmp;
		tmp.str.printf(key);
		sectPtr->items.add(tmp);
		itemPtr = &sectPtr->items.last();
	}

	return itemPtr;
}

} // namespace sfz
