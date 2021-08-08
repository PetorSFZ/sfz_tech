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

#include "sfz/util/JsonParser.hpp"

#include <algorithm>
#include <cstring>
#include <type_traits>

#include <skipifzero_new.hpp>

#include <sfz/PushWarnings.hpp>
#define SAJSON_NO_STD_STRING
#include "sajson.h"
#include <sfz/PopWarnings.hpp>

#include <sfz/Logging.hpp>
#include <sfz/util/IO.hpp>

namespace sfz {

// Statics
// ------------------------------------------------------------------------------------------------

static_assert(JSON_NODE_IMPL_SIZE >= sizeof(sajson::value), "JsonNode is too small");
//static_assert(std::is_trivially_copyable<sajson::value>::value, "sajson::value is not memcpy:able");

static const sajson::value& castToSajsonValue(const uint8_t* memory) noexcept
{
	return *reinterpret_cast<const sajson::value*>(memory);
}

static uint64_t copyStripCppComments(char* dst, const char* src, uint64_t srcLen) noexcept
{
	sfz_assert(srcLen > 0);

	// The total number of bytes in the new string, excluding null-terminator
	uint64_t totalNumBytes = 0;

	const char* currSrc = src;
	const char* nextComment = std::strstr(src, "//");
	while (nextComment != nullptr) {

		// Copy part until comment
		uint64_t numBytesToCopy = nextComment - currSrc;
		std::memcpy(dst + totalNumBytes, currSrc, numBytesToCopy);
		totalNumBytes += numBytesToCopy;

		// Update current src pointer to point to part right after comment
		currSrc = nextComment + 2;

		// Find next line break (so we know how much of string to skip)
		const char* nextLineBreak = std::strchr(currSrc, '\n');

		// If no line break found, skip rest of string
		if (nextLineBreak == nullptr) {
			// Set currSrc to length of src so we don't copy rest in post step
			currSrc = src + srcLen;
			break;
		}

		// Skip part between comment and line break
		currSrc = nextLineBreak + 1;

		// Find next comment
		nextComment = std::strstr(currSrc, "//");
	}

	// Copy the remainder of the string
	sfz_assert(currSrc <= (src + srcLen));
	uint64_t remainderNumBytes = (src + srcLen) - currSrc;
	if (remainderNumBytes != 0) {
		std::memcpy(dst + totalNumBytes, currSrc, remainderNumBytes);
	}
	totalNumBytes += remainderNumBytes;

	// Set null terminator
	dst[totalNumBytes] = '\0';

	return totalNumBytes;
}

// JsonNode: Constructors & destructors
// ------------------------------------------------------------------------------------------------

JsonNode JsonNode::createFromImplDefined(const void* implDefined) noexcept
{
	const sajson::value& value = *reinterpret_cast<const sajson::value*>(implDefined);
	JsonNode node;
	new (node.mImpl) sajson::value(value);
	node.mActive = true;
	return node;
}

// JsonNode: State methods
// ------------------------------------------------------------------------------------------------

JsonNode JsonNode::copy() const noexcept
{
	JsonNode tmp = {};
	tmp.mActive = true;
	memcpy(tmp.mImpl, this->mImpl, sizeof(mImpl));
	return tmp;
}

void JsonNode::swap(JsonNode& other) noexcept
{
	std::swap(this->mImpl, other.mImpl);
	std::swap(this->mActive, other.mActive);
}

void JsonNode::destroy() noexcept
{
	if (!mActive) return;
	reinterpret_cast<sajson::value*>(mImpl)->~value();
	memset(mImpl, 0, sizeof(mImpl));
	mActive = false;
}

// JsonNode: Methods (all nodes)
// ------------------------------------------------------------------------------------------------

JsonNodeType JsonNode::type() const noexcept
{
	// Return NONE type if not active
	if (!mActive) return JsonNodeType::NONE;

	const sajson::value& value = castToSajsonValue(mImpl);
	sajson::type type = value.get_type();
	switch (type) {
	case sajson::TYPE_INTEGER: return JsonNodeType::INTEGER;
	case sajson::TYPE_DOUBLE: return JsonNodeType::FLOATING_POINT;
	case sajson::TYPE_NULL: return JsonNodeType::NONE;
	case sajson::TYPE_FALSE: return JsonNodeType::BOOL;
	case sajson::TYPE_TRUE: return JsonNodeType::BOOL;
	case sajson::TYPE_STRING: return JsonNodeType::STRING;
	case sajson::TYPE_ARRAY: return JsonNodeType::ARRAY;
	case sajson::TYPE_OBJECT: return JsonNodeType::MAP;
	}
	sfz_assert(false);
	return JsonNodeType::MAP;
}

// JsonNode: Methods (non-leaf nodes)
// ------------------------------------------------------------------------------------------------

uint32_t JsonNode::mapNumObjects() const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return NONE if wrong type
	if (this->type() != JsonNodeType::MAP) return 0;

	// Return number of objects
	return (uint32_t)value.get_length();
}

JsonNode JsonNode::accessMap(const char* nodeName) const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return NONE if wrong type
	if (this->type() != JsonNodeType::MAP) return JsonNode();

	// Get value in map
	sajson::string nodeNameStr(nodeName, std::strlen(nodeName));
	sajson::value elementValue = value.get_value_of_key(nodeNameStr);

	// Return NONE node if invalid access
	if (elementValue.get_type() == sajson::TYPE_NULL) return JsonNode();

	// Return node
	return JsonNode::createFromImplDefined(&elementValue);
}

uint32_t JsonNode::arrayLength() const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return 0 if node is of wrong type
	if (this->type() != JsonNodeType::ARRAY) return 0;

	return (uint32_t)value.get_length();
}

JsonNode JsonNode::accessArray(uint32_t index) const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return NONE if wrong type
	if (this->type() != JsonNodeType::ARRAY) return JsonNode();

	// Return NONE if out of range
	if (index >= (uint32_t)value.get_length()) return JsonNode();

	// Return array element
	sajson::value elementValue = value.get_array_element(index);
	return JsonNode::createFromImplDefined(&elementValue);
}

// JsonNode: Methods (leaf nodes)
// ------------------------------------------------------------------------------------------------

bool JsonNode::value(bool& valueOut) const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return false if node is of wrong type
	if (this->type() != JsonNodeType::BOOL) return false;

	// Return value
	valueOut = value.get_type() == sajson::TYPE_TRUE;
	return true;
}

bool JsonNode::value(int32_t& valueOut) const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return false if node is of wrong type
	if (this->type() != JsonNodeType::INTEGER) return false;

	// Return value
	valueOut = value.get_integer_value();
	return true;
}

bool JsonNode::value(float& valueOut) const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return false if node is of wrong type
	JsonNodeType t = this->type();
	if (t != JsonNodeType::FLOATING_POINT && t != JsonNodeType::INTEGER) return false;

	// Return value
	if (t == JsonNodeType::FLOATING_POINT) {
		valueOut = (float)value.get_double_value();
	}
	else {
		valueOut = (float)value.get_integer_value();
	}
	return true;
}

bool JsonNode::value(double& valueOut) const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return false if node is of wrong type
	JsonNodeType t = this->type();
	if (t != JsonNodeType::FLOATING_POINT && t != JsonNodeType::INTEGER) return false;

	// Return value
	if (t == JsonNodeType::FLOATING_POINT) {
		valueOut = value.get_double_value();
	}
	else {
		valueOut = (double)value.get_integer_value();
	}
	return true;
}

bool JsonNode::value(char* strOut, uint32_t strCapacity) const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return false if node is of wrong type
	if (this->type() != JsonNodeType::STRING) return false;

	// Check that capacity is enough
	uint32_t strLen = (uint32_t)value.get_string_length();
	if (strLen >= strCapacity) return false; // >= so we guarantee space for null-terminator

	// Copy out string
	std::memcpy(strOut, value.as_cstring(), strLen);
	strOut[strLen] = '\0'; // Ensure string is null-terminated
	return true;
}

uint32_t JsonNode::stringLength() const noexcept
{
	sfz_assert(this->mActive);
	const sajson::value& value = castToSajsonValue(mImpl);

	// Return false if node is of wrong type
	if (this->type() != JsonNodeType::STRING) return 0;

	// Return string length
	uint32_t strLen = (uint32_t)value.get_string_length();
	return strLen;
}

JsonNodeValue<bool> JsonNode::valueBool() const noexcept
{
	JsonNodeValue<bool> tmp;
	tmp.value = false; // Default-value
	tmp.exists = this->value(tmp.value);
	return tmp;
}

JsonNodeValue<int32_t> JsonNode::valueInt() const noexcept
{
	JsonNodeValue<int32_t> tmp;
	tmp.value = 0; // Default-value
	tmp.exists = this->value(tmp.value);
	return tmp;
}

JsonNodeValue<float> JsonNode::valueFloat() const noexcept
{
	JsonNodeValue<float> tmp;
	tmp.value = 0.0f; // Default-value
	tmp.exists = this->value(tmp.value);
	return tmp;
}

JsonNodeValue<double> JsonNode::valueDouble() const noexcept
{
	JsonNodeValue<double> tmp;
	tmp.value = 0.0; // Default-value
	tmp.exists = this->value(tmp.value);
	return tmp;
}

JsonNodeValue<str32> JsonNode::valueStr32() const noexcept
{
	JsonNodeValue<str32> tmp;
	tmp.exists = this->value(tmp.value.mRawStr, tmp.value.capacity());
	return tmp;
}

JsonNodeValue<str64> JsonNode::valueStr64() const noexcept
{
	JsonNodeValue<str64> tmp;
	tmp.exists = this->value(tmp.value.mRawStr, tmp.value.capacity());
	return tmp;
}

JsonNodeValue<str96> JsonNode::valueStr96() const noexcept
{
	JsonNodeValue<str96> tmp;
	tmp.exists = this->value(tmp.value.mRawStr, tmp.value.capacity());
	return tmp;
}

JsonNodeValue<str128> JsonNode::valueStr128() const noexcept
{
	JsonNodeValue<str128> tmp;
	tmp.exists = this->value(tmp.value.mRawStr, tmp.value.capacity());
	return tmp;
}

JsonNodeValue<str256> JsonNode::valueStr256() const noexcept
{
	JsonNodeValue<str256> tmp;
	tmp.exists = this->value(tmp.value.mRawStr, tmp.value.capacity());
	return tmp;
}

JsonNodeValue<str320> JsonNode::valueStr320() const noexcept
{
	JsonNodeValue<str320> tmp;
	tmp.exists = this->value(tmp.value.mRawStr, tmp.value.capacity());
	return tmp;
}

// ParsedJson: Implementation
// ------------------------------------------------------------------------------------------------

struct ParsedJsonImpl {
	sfz::Allocator* allocator = nullptr;

	char* jsonStringCopy = nullptr;
	sajson::mutable_string_view jsongStringView;

	size_t* astMemory = nullptr;
	sajson::bounded_allocation astAllocation = sajson::bounded_allocation(nullptr, 0);

	sajson::document* doc = nullptr;
};

// ParsedJson: Constructors & destructors
// ------------------------------------------------------------------------------------------------

ParsedJson ParsedJson::parseString(
	const char* jsonString, sfz::Allocator* allocator, bool allowCppComments) noexcept
{
	// Ensure json string is not nullptr
	if (jsonString == nullptr) {
		SFZ_ERROR("JSON", "JSON string may not be nullptr");
		return ParsedJson();
	}

	// Allocate implementation
	ParsedJson parsedJson;
	parsedJson.mImpl = sfz_new<ParsedJsonImpl>(allocator, sfz_dbg(""));
	ParsedJsonImpl& impl = *parsedJson.mImpl;
	impl.allocator = allocator;

	// Calculate length of string and allocate memory for it
	uint64_t jsonStringLen = std::strlen(jsonString);
	if (jsonStringLen == 0) {
		sfz_delete(allocator, parsedJson.mImpl);
		SFZ_ERROR("JSON", "JSON string must be longer than 0");
		return ParsedJson();
	}
	impl.jsonStringCopy = reinterpret_cast<char*>(
		allocator->allocate(sfz_dbg(""), jsonStringLen + 1, 32));

	// Copy string and strip Cpp comments if specified, also modify length of string
	if (allowCppComments) {
		jsonStringLen = copyStripCppComments(impl.jsonStringCopy, jsonString, jsonStringLen);
		//printf("Comment stripped json:\n\"%s\"", impl.jsonStringCopy);
	}

	// Otherwise just copy the string
	else {
		std::memcpy(impl.jsonStringCopy, jsonString, jsonStringLen);
		impl.jsonStringCopy[jsonStringLen] = '\0';
	}

	// Create view of the string for sajson
	impl.jsongStringView = sajson::mutable_string_view(jsonStringLen, impl.jsonStringCopy);

	// Allocate memory for ast
	// Assume a constant factor times the json string itself is enough memory for the AST
	const uint64_t SIZE_FACTOR = 4;
	uint64_t allocationSize = SIZE_FACTOR * jsonStringLen;
	impl.astMemory =
		reinterpret_cast<size_t*>(allocator->allocate(sfz_dbg(""), allocationSize, 32));
	impl.astAllocation = sajson::bounded_allocation(impl.astMemory, allocationSize / sizeof(size_t));

	// Parse json string
	impl.doc = sfz_new<sajson::document>(allocator, sfz_dbg("sajson::document"), 
		sajson::parse(impl.astAllocation, impl.jsongStringView));
	if (!impl.doc->is_valid()) {
		SFZ_ERROR("JSON", "Json parse failed at %i:%i: %s",
			(int)impl.doc->get_error_line(), (int)impl.doc->get_error_column(), impl.doc->get_error_message_as_cstring());
		parsedJson.destroy();
		return ParsedJson();
	}

	return parsedJson;
}

ParsedJson ParsedJson::parseFile(
	const char* jsonPath, sfz::Allocator* allocator, bool allowCppComments) noexcept
{
	sfz::DynString jsonString = sfz::readTextFile(jsonPath, allocator);
	if (jsonString.size() == 0) {
		SFZ_ERROR("JSON", "Failed to load JSON file at: %s", jsonPath);
		return ParsedJson();
	}
	return ParsedJson::parseString(jsonString.str(), allocator, allowCppComments);
}

// ParsedJson: State methods
// ------------------------------------------------------------------------------------------------

void ParsedJson::swap(ParsedJson& other) noexcept
{
	std::swap(this->mImpl, other.mImpl);
}

void ParsedJson::destroy() noexcept
{
	if (mImpl == nullptr) return;
	sfz::Allocator* allocator = mImpl->allocator;

	// Deallocate sajson document
	if (mImpl->doc != nullptr) {
		sfz_delete(allocator, mImpl->doc);
	}

	// Deallocate json string copy
	if (mImpl->jsonStringCopy != nullptr) {
		allocator->deallocate(mImpl->jsonStringCopy);
	}

	// Deallocate sajson ast
	if (mImpl->astMemory != nullptr) {
		allocator->deallocate(mImpl->astMemory);
	}

	// Deallocate implementation
	sfz_delete(allocator, mImpl);
	mImpl = nullptr;
}

// ParsedJson: State methods
// ------------------------------------------------------------------------------------------------

JsonNode ParsedJson::root() const noexcept
{
	sfz_assert(mImpl->doc->is_valid());
	sajson::value value = mImpl->doc->get_root();
	return JsonNode::createFromImplDefined(&value);
}

} // namespace sfz
