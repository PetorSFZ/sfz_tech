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
	bool optional = false;
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

	void printErrorMessage(const char* name, const char* message)
	{
		if (errorPath != nullptr) {
			SFZ_ERROR("Deserializer", "\"%s\": %s", errorPath->str(), message);
		}
		else {
			SFZ_ERROR("Deserializer", "Node \"%s\": %s", name, message);
		}
	}

	JsonNode accessMapChecked(const char* name) 
	{
		JsonNode child = parentNode.accessMap(name);
		if (parentNode.isValid() && !child.isValid() && !this->optional) {
			printErrorMessage(name, "Node is invalid");
			this->success = false;
		}
		return child;
	}

	template<typename T>
	void extractValue(const char* name, JsonNodeValue<T>&& valuePair, T& valOut)
	{
		if (valuePair.exists) {
			valOut = valuePair.value;
		}
		else if (!this->optional) {
			printErrorMessage(name, "Failed to extract value from node.");
		}
		this->success = this->optional || (this->success && valuePair.exists);
	}

	void operator() (const char* name, bool& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) extractValue(name, child.valueBool(), valOut);
		else this->success = this->optional || false;
		restoreErrorPath(name);
	}

	void operator() (const char* name, int32_t& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) extractValue(name, child.valueInt(), valOut);
		else this->success = this->optional || false;
		restoreErrorPath(name);
	}

	void operator() (const char* name, float& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) extractValue(name, child.valueFloat(), valOut);
		else this->success = this->optional || false;
		restoreErrorPath(name);
	}

	void operator() (const char* name, vec2_i32& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) {
			if (child.type() == JsonNodeType::ARRAY && child.arrayLength() == 2) {
				extractValue("x", child.accessArray(0).valueInt(), valOut.x);
				extractValue("y", child.accessArray(1).valueInt(), valOut.y);
			}
			else {
				printErrorMessage(name, "Failed, vec2_i32 must be of form [x, y]");
				this->success = this->optional || false;
			}
		}
		else {
			this->success = this->optional || false;
		}
		restoreErrorPath(name);
	}

	void operator() (const char* name, vec3_i32& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) {
			if (child.type() == JsonNodeType::ARRAY && child.arrayLength() == 3) {
				extractValue("x", child.accessArray(0).valueInt(), valOut.x);
				extractValue("y", child.accessArray(1).valueInt(), valOut.y);
				extractValue("z", child.accessArray(2).valueInt(), valOut.z);
			}
			else {
				printErrorMessage(name, "Failed, vec3_i32 must be of form [x, y, z]");
				this->success = this->optional || false;
			}
		}
		else {
			this->success = this->optional || false;
		}
		restoreErrorPath(name);
	}

	void operator() (const char* name, vec4_i32& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) {
			if (child.type() == JsonNodeType::ARRAY && child.arrayLength() == 4) {
				extractValue("x", child.accessArray(0).valueInt(), valOut.x);
				extractValue("y", child.accessArray(1).valueInt(), valOut.y);
				extractValue("z", child.accessArray(2).valueInt(), valOut.z);
				extractValue("w", child.accessArray(3).valueInt(), valOut.w);
			}
			else {
				printErrorMessage(name, "Failed, vec4_i32 must be of form [x, y, z, w]");
				this->success = this->optional || false;
			}
		}
		else {
			this->success = this->optional || false;
		}
		restoreErrorPath(name);
	}

	void operator() (const char* name, vec2& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) {
			if (child.type() == JsonNodeType::ARRAY && child.arrayLength() == 2) {
				extractValue("x", child.accessArray(0).valueFloat(), valOut.x);
				extractValue("y", child.accessArray(1).valueFloat(), valOut.y);
			}
			else {
				printErrorMessage(name, "Failed, vec2 must be of form [x, y]");
				this->success = this->optional || false;
			}
		}
		else {
			this->success = this->optional || false;
		}
		restoreErrorPath(name);
	}

	void operator() (const char* name, vec3& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) {
			if (child.type() == JsonNodeType::ARRAY && child.arrayLength() == 3) {
				extractValue("x", child.accessArray(0).valueFloat(), valOut.x);
				extractValue("y", child.accessArray(1).valueFloat(), valOut.y);
				extractValue("z", child.accessArray(2).valueFloat(), valOut.z);
			}
			else {
				printErrorMessage(name, "Failed, vec3 must be of form [x, y, z]");
				this->success = this->optional || false;
			}
		}
		else {
			this->success = this->optional || false;
		}
		restoreErrorPath(name);
	}

	void operator() (const char* name, vec4& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) {
			if (child.type() == JsonNodeType::ARRAY && child.arrayLength() == 4) {
				extractValue("x", child.accessArray(0).valueFloat(), valOut.x);
				extractValue("y", child.accessArray(1).valueFloat(), valOut.y);
				extractValue("z", child.accessArray(2).valueFloat(), valOut.z);
				extractValue("w", child.accessArray(3).valueFloat(), valOut.w);
			}
			else {
				printErrorMessage(name, "Failed, vec4 must be of form [x, y, z, w]");
				this->success = this->optional || false;
			}
		}
		else {
			this->success = this->optional || false;
		}
		restoreErrorPath(name);
	}

	void operator() (const char* name, strID& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) {
			str256 tmpStr;
			extractValue(name, child.valueStr256(), tmpStr);
			if (this->success) valOut = strID(tmpStr);
		}
		else {
			this->success = this->optional || false;
		}
		restoreErrorPath(name);
	}

	void operator() (const char* name, str32& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) extractValue(name, child.valueStr32(), valOut);
		else this->success = this->optional || false;
		restoreErrorPath(name);
	}

	void operator() (const char* name, str64& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) extractValue(name, child.valueStr64(), valOut);
		else this->success = this->optional || false;
		restoreErrorPath(name);
	}

	void operator() (const char* name, str96& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) extractValue(name, child.valueStr96(), valOut);
		else this->success = this->optional || false;
		restoreErrorPath(name);
	}

	void operator() (const char* name, str128& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) extractValue(name, child.valueStr128(), valOut);
		else this->success = this->optional || false;
		restoreErrorPath(name);
	}

	void operator() (const char* name, str256& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) extractValue(name, child.valueStr256(), valOut);
		else this->success = this->optional || false;
		restoreErrorPath(name);
	}

	void operator() (const char* name, str320& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) extractValue(name, child.valueStr320(), valOut);
		else this->success = this->optional || false;
		restoreErrorPath(name);
	}

	template<typename T>
	void operator() (const char* name, Opt<T>& valOut)
	{
		DeserializerVisitor deserializer;
		deserializer.allocator = allocator;
		deserializer.parentNode = this->parentNode.copy();
		deserializer.success = true;
		deserializer.optional = true;
		deserializer.errorPath = this->errorPath;
		deserializer(name, valOut.val);
		this->success = this->optional || (this->success && deserializer.success);
	}

	template<typename T>
	void operator() (const char* name, Array<T>& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) {
			if (child.type() == JsonNodeType::ARRAY) {
				const uint32_t len = child.arrayLength();
				valOut.init(len, this->allocator, sfz_dbg(""));
				for (uint32_t i = 0; i < len; i++) {
					appendErrorPathArray(i);
					T& val = valOut.add();
					DeserializerVisitor deserializer;
					deserializer.allocator = this->allocator;
					deserializer.parentNode = child.accessArray(i);
					deserializer.success = this->success;
					deserializer.optional = this->optional;
					deserializer.errorPath = this->errorPath;
					visit_struct::for_each(val, deserializer);
					this->success = this->optional || (this->success && deserializer.success);
					restoreErrorPathArray(i);
				}
			}
			else {
				this->success = this->optional || false;
			}
		}
		else {
			this->success = this->optional || false;
		}
		restoreErrorPath(name);
	}

	template<typename T, uint32_t N>
	void operator() (const char* name, ArrayLocal<T,N>& valOut)
	{
		appendErrorPath(name);
		JsonNode child = accessMapChecked(name);
		if (child.isValid()) {
			if (child.type() == JsonNodeType::ARRAY) {
				const uint32_t len = child.arrayLength();
				if (len <= valOut.capacity()) {
					valOut.clear();
					for (uint32_t i = 0; i < len; i++) {
						appendErrorPathArray(i);
						T& val = valOut.add();
						DeserializerVisitor deserializer;
						deserializer.allocator = this->allocator;
						deserializer.parentNode = child.accessArray(i);
						deserializer.success = this->success;
						deserializer.optional = this->optional;
						deserializer.errorPath = this->errorPath;
						visit_struct::for_each(val, deserializer);
						this->success = this->optional || (this->success && deserializer.success);
						restoreErrorPathArray(i);
					}
				}
				else {
					printErrorMessage(name, str128("Json array is too big (%u) for local array (%u)",
						len, valOut.capacity()).str());
					this->success = this->optional || false;
				}
			}
			else {
				this->success = this->optional || false;
			}
		}
		else {
			this->success = this->optional || false;
		}
		restoreErrorPath(name);
	}

	template<typename T>
	void operator() (const char* name, T& valOut)
	{
		appendErrorPath(name);
		DeserializerVisitor deserializer;
		deserializer.allocator = this->allocator;
		deserializer.parentNode = accessMapChecked(name);
		if (deserializer.parentNode.isValid()) {
			deserializer.success = this->success;
			deserializer.optional = this->optional;
			deserializer.errorPath = this->errorPath;
			visit_struct::for_each(valOut, deserializer);
			this->success = this->optional || (this->success && deserializer.success);
		}
		else {
			this->success == this->optional || false;
		}
		restoreErrorPath(name);
	}
};

} // namespace sfz
