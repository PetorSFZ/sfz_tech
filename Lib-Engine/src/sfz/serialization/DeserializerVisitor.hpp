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

#include "visit_struct/visit_struct.hpp"

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_strings.hpp>

#include <sfz/Logging.hpp>
#include <sfz/serialization/SerializationTypes.hpp>
#include <sfz/util/JsonParser.hpp>

namespace sfz {

// DeserializerVisitor
// ------------------------------------------------------------------------------------------------

struct DeserializerVisitor final {
	Allocator* allocator = nullptr;
	JsonNode parentNode;
	bool success = true;
	bool errorMessagesEnabled = true;
	str320* errorPath = nullptr; // Point to an empty str320 to get better debug output

	void appendErrorPath(const char* name)
	{
		if (errorPath != nullptr) errorPath->appendf(".%s", name);
	}

	void appendErrorPathArray(uint32_t idx)
	{
		if (errorPath != nullptr) errorPath->appendf("[%u]", idx);
	}

	void restoreErrorPath(const char* name)
	{
		if (errorPath != nullptr) errorPath->removeChars(uint32_t(strnlen(name, 100)) + 1);
	}

	void restoreErrorPathArray(uint32_t idx)
	{
		sfz_assert(idx < 10'000'000);
		uint32_t numDigits = 0;
		if (idx < 10) numDigits = 1;
		else if (idx < 100) numDigits = 2;
		else if (idx < 1'000) numDigits = 3;
		else if (idx < 10'000) numDigits = 4;
		else if (idx < 100'000) numDigits = 5;
		else if (idx < 1'000'000) numDigits = 6;
		else if (idx < 10'000'000) numDigits = 7;
		if (errorPath != nullptr) errorPath->removeChars(numDigits + 2);
	}

	void printErrorMessage(const char* message)
	{
		if (errorMessagesEnabled) {
			if (errorPath != nullptr) {
				SFZ_ERROR("Deserializer", "\"%s\": %s", errorPath->str(), message);
			}
		}
	}

	bool ensureNodeIsValid(const JsonNode& node)
	{
		if (!node.isValid()) {
			printErrorMessage("Node is invalid");
			this->success = false;
			return false;
		}
		return true;
	}

	template<typename T>
	void extractValue(JsonNodeValue<T>&& valuePair, T& valOut)
	{
		if (valuePair.exists) {
			valOut = valuePair.value;
		}
		else {
			printErrorMessage("Failed to extract value from node.");
		}
		this->success = this->success && valuePair.exists;
	}

	void deserialize(const JsonNode& node, bool& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		extractValue(node.valueBool(), valOut);
	}

	void deserialize(const JsonNode& node, int32_t& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		extractValue(node.valueInt(), valOut);
	}

	void deserialize(const JsonNode& node, float& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		extractValue(node.valueFloat(), valOut);
	}

	void deserialize(const JsonNode& node, vec2_i32& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		if (node.type() == JsonNodeType::ARRAY && node.arrayLength() == 2) {
			extractValue(node.accessArray(0).valueInt(), valOut.x);
			extractValue(node.accessArray(1).valueInt(), valOut.y);
		}
		else {
			printErrorMessage("Failed, vec2_i32 must be of form [x, y]");
			this->success = false;
		}
	}

	void deserialize(const JsonNode& node, vec3_i32& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		if (node.type() == JsonNodeType::ARRAY && node.arrayLength() == 3) {
			extractValue(node.accessArray(0).valueInt(), valOut.x);
			extractValue(node.accessArray(1).valueInt(), valOut.y);
			extractValue(node.accessArray(2).valueInt(), valOut.z);
		}
		else {
			printErrorMessage("Failed, vec3_i32 must be of form [x, y, z]");
			this->success = false;
		}
	}

	void deserialize(const JsonNode& node, vec4_i32& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		if (node.type() == JsonNodeType::ARRAY && node.arrayLength() == 4) {
			extractValue(node.accessArray(0).valueInt(), valOut.x);
			extractValue(node.accessArray(1).valueInt(), valOut.y);
			extractValue(node.accessArray(2).valueInt(), valOut.z);
			extractValue(node.accessArray(3).valueInt(), valOut.w);
		}
		else {
			printErrorMessage("Failed, vec4_i32 must be of form [x, y, z, w]");
			this->success = false;
		}
	}

	void deserialize(const JsonNode& node, vec2& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		if (node.type() == JsonNodeType::ARRAY && node.arrayLength() == 2) {
			extractValue(node.accessArray(0).valueFloat(), valOut.x);
			extractValue(node.accessArray(1).valueFloat(), valOut.y);
		}
		else {
			printErrorMessage("Failed, vec2 must be of form [x, y]");
			this->success = false;
		}
	}

	void deserialize(const JsonNode& node, vec3& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		if (node.type() == JsonNodeType::ARRAY && node.arrayLength() == 3) {
			extractValue(node.accessArray(0).valueFloat(), valOut.x);
			extractValue(node.accessArray(1).valueFloat(), valOut.y);
			extractValue(node.accessArray(2).valueFloat(), valOut.z);
		}
		else {
			printErrorMessage("Failed, vec3 must be of form [x, y, z]");
			this->success = false;
		}
	}

	void deserialize(const JsonNode& node, vec4& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		if (node.type() == JsonNodeType::ARRAY && node.arrayLength() == 4) {
			extractValue(node.accessArray(0).valueFloat(), valOut.x);
			extractValue(node.accessArray(1).valueFloat(), valOut.y);
			extractValue(node.accessArray(2).valueFloat(), valOut.z);
			extractValue(node.accessArray(3).valueFloat(), valOut.w);
		}
		else {
			printErrorMessage("Failed, vec4 must be of form [x, y, z, w]");
			this->success = false;
		}
	}

	void deserialize(const JsonNode& node, strID& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		str256 tmpStr;
		extractValue(node.valueStr256(), tmpStr);
		if (this->success) valOut = strID(tmpStr);
	}

	void deserialize(const JsonNode& node, str32& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		extractValue(node.valueStr32(), valOut);
	}

	void deserialize(const JsonNode& node, str64& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		extractValue(node.valueStr64(), valOut);
	}

	void deserialize(const JsonNode& node, str96& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		extractValue(node.valueStr96(), valOut);
	}

	void deserialize(const JsonNode& node, str128& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		extractValue(node.valueStr128(), valOut);
	}

	void deserialize(const JsonNode& node, str256& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		extractValue(node.valueStr256(), valOut);
	}

	void deserialize(const JsonNode& node, str320& valOut)
	{
		if (!ensureNodeIsValid(node)) return;
		extractValue(node.valueStr320(), valOut);
	}

	template<typename T>
	void deserialize(const JsonNode& node, OptVal<T>& valOut)
	{
		if (!node.isValid()) return;
		DeserializerVisitor deserializer;
		deserializer.allocator = allocator;
		deserializer.parentNode = this->parentNode.copy();
		deserializer.success = true;
		deserializer.errorMessagesEnabled = false;
		deserializer.errorPath = this->errorPath;
		T val = {};
		deserializer.deserialize(node, val);
		if (deserializer.success) {
			valOut.set(std::move(val));
		}
	}

	template<typename T>
	void deserialize(const JsonNode& node, Array<T>& valOut)
	{
		if (!ensureNodeIsValid(node)) return;

		if (node.type() == JsonNodeType::ARRAY) {
			const uint32_t len = node.arrayLength();
			valOut.init(len, this->allocator, sfz_dbg(""));
			for (uint32_t i = 0; i < len; i++) {
				appendErrorPathArray(i);
				DeserializerVisitor deserializer;
				deserializer.allocator = this->allocator;
				deserializer.parentNode = node.copy();
				deserializer.success = this->success;
				deserializer.errorMessagesEnabled = this->errorMessagesEnabled;
				deserializer.errorPath = this->errorPath;
				JsonNode elementNode = node.accessArray(i);
				T& val = valOut.add();
				deserializer.deserialize(elementNode, val);
				this->success = this->success && deserializer.success;
				restoreErrorPathArray(i);
			}
		}
		else {
			this->success = false;
		}
	}

	template<typename T, uint32_t N>
	void deserialize(const JsonNode& node, ArrayLocal<T,N>& valOut)
	{
		if (!ensureNodeIsValid(node)) return;

		if (node.type() == JsonNodeType::ARRAY) {
			const uint32_t len = child.arrayLength();
			if (len <= valOut.capacity()) {
				valOut.clear();
				for (uint32_t i = 0; i < len; i++) {
					appendErrorPathArray(i);
					DeserializerVisitor deserializer;
					deserializer.allocator = this->allocator;
					deserializer.parentNode = node.copy();
					deserializer.success = this->success;
					deserializer.errorMessagesEnabled = this->errorMessagesEnabled;
					deserializer.errorPath = this->errorPath;
					JsonNode elementNode = node.accessArray(i);
					T& val = valOut.add();
					deserializer.deserialize(elementNode, val);
					this->success = this->success && deserializer.success;
					restoreErrorPathArray(i);
				}
			}
			else {
				printErrorMessage(str128("Json array is too big (%u) for local array (%u)",
					len, valOut.capacity()).str());
				this->success = false;
			}
		}
		else {
			this->success = false;
		}
	}

	template<typename T>
	void deserialize(const JsonNode& node, T& valOut)
	{
		if (!ensureNodeIsValid(node)) return;

		DeserializerVisitor deserializer;
		deserializer.allocator = this->allocator;
		deserializer.parentNode = node.copy();
		deserializer.success = this->success;
		deserializer.errorMessagesEnabled = this->errorMessagesEnabled;
		deserializer.errorPath = this->errorPath;
		visit_struct::for_each(valOut, deserializer);
		this->success = this->success && deserializer.success;
	}

	template<typename T>
	void operator() (const char* name, T& valOut)
	{
		appendErrorPath(name);
		JsonNode child;
		if (this->parentNode.isValid()) {
			child = parentNode.accessMap(name);
		}
		this->deserialize(child, valOut);
		restoreErrorPath(name);
	}
};

// Deserialization function
// ------------------------------------------------------------------------------------------------

template<typename T>
bool deserialize(T& valOut, const char* jsonPath, sfz::Allocator* allocator)
{
	static_assert(visit_struct::traits::is_visitable<T>::value, "Can only deserialize visitable types");

	ParsedJson json = ParsedJson::parseFile(jsonPath, allocator);
	if (!json.isValid()) {
		SFZ_ERROR("Deserializer", "Failed to parse json at: \"%s\"", jsonPath);
		return false;
	}

	str320 tmpErrorPath = "root";
	DeserializerVisitor deserializer;
	deserializer.allocator = allocator;
	deserializer.parentNode = json.root();
	deserializer.errorPath = &tmpErrorPath;
	visit_struct::for_each(valOut, deserializer);
	sfz_assert(tmpErrorPath == "root");

	return deserializer.success;
}

} // namespace sfz
