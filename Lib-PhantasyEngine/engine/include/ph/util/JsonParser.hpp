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

#include <cstdint>

#include <sfz/memory/Allocator.hpp>
#include <sfz/strings/StackString.hpp>

namespace ph {

using sfz::str256;

// ParsedJsonNodeType enum
// ------------------------------------------------------------------------------------------------

// The different types of nodes
enum class ParsedJsonNodeType : uint32_t {
	// Undefined node, not valid to do any operations on
	NONE = 0,

	// Non-leaf nodes (does not contain values, but other nodes)
	MAP,
	ARRAY,

	// Leaf nodes (value can be accessed directly from node)
	BOOL,
	INTEGER,
	FLOATING_POINT,
	STRING,
};

// ParsedJsonNode class
// ------------------------------------------------------------------------------------------------

// Size of the implementation of ParsedJsonNode in bytes
constexpr uint32_t PARSED_JSON_NODE_IMPL_SIZE = 32;

// Minimal helper struct that contains a value and whether the value existed or not
//
// Used as alternative getter for retrieving values from a ParsedJsonNode.
template<typename T>
struct ParsedJsonNodeValue {
	T value;
	bool exists = false;
};

// Represents a node in a ParsedJson instance
//
// Used to traverse and access contents of a ParsedJson. NONE nodes are considered invalid and are
// used as error codes for invalid accesses. Default constructed ParsedJsonNodes are also NONE.
class ParsedJsonNode final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ParsedJsonNode() noexcept = default;
	ParsedJsonNode(const ParsedJsonNode&) = delete;
	ParsedJsonNode& operator= (const ParsedJsonNode&) = delete;
	ParsedJsonNode(ParsedJsonNode&& other) noexcept { this->swap(other); }
	ParsedJsonNode& operator= (ParsedJsonNode&& other) noexcept { this->swap(other); return *this; }
	~ParsedJsonNode() noexcept { this->destroy(); }

	static ParsedJsonNode createFromImplDefined(const void* implDefined) noexcept;

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(ParsedJsonNode& other) noexcept;
	void destroy() noexcept;

	// Methods (all nodes)
	// --------------------------------------------------------------------------------------------

	// Returns type of node. NONE if the node is invalid for some reason.
	ParsedJsonNodeType type() const noexcept;

	// Returns whether the node is valid or not. NONE nodes are considered invalid.
	bool isValid() const noexcept { return mActive; }

	// Methods (non-leaf nodes)
	// --------------------------------------------------------------------------------------------

	// Returns the number of objects in a map, returns 0 if not a map node
	uint32_t mapNumObjects() const noexcept;

	// Accesses a node in a map, returns NONE on invalid access or if not map
	ParsedJsonNode accessMap(const char* nodeName) const noexcept;

	// Length of the array, returns 0 if not an array node
	uint32_t arrayLength() const noexcept;

	// Accesses a node in the array, returns NONE on invalid access or if not array
	ParsedJsonNode accessArray(uint32_t index) const noexcept;

	// Methods (leaf nodes)
	// --------------------------------------------------------------------------------------------

	bool value(bool& valueOut) const noexcept;
	bool value(int32_t& valueOut) const noexcept;
	bool value(float& valueOut) const noexcept;
	bool value(double& valueOut) const noexcept;
	bool value(char* strOut, uint32_t strCapacity) const noexcept;
	uint32_t stringLength() const noexcept; // Returns 0 if not a string

	ParsedJsonNodeValue<bool> valueBool() const noexcept;
	ParsedJsonNodeValue<int32_t> valueInt() const noexcept;
	ParsedJsonNodeValue<float> valueFloat() const noexcept;
	ParsedJsonNodeValue<double> valueDouble() const noexcept;
	ParsedJsonNodeValue<str256> valueStr256() const noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	uint8_t mImpl[PARSED_JSON_NODE_IMPL_SIZE] = {};
	bool mActive = false;
};

// ParsedJson class
// ------------------------------------------------------------------------------------------------

struct ParsedJsonImpl; // Pimpl pattern

// A class that represents a parsed JSON file
//
// Parse a JSON file using either ParsedJson::parseString() or ParsedJson::parseFile(). The parsed
// contents can then be accessed by recursively accessing the nodes, starting with the root node.
//
// The "allowCppComments" flag sets whether a PhantasyEngine specific extension should be enabled
// or not. This extension enables the use of // comments in the json files. This is normally not
// allowed, but makes it way more human-friendly to use them. This same extension seem to be in use
// by e.g. Visual Studio Code, so it can't be entirely uncommon. A note of warning, this is not
// super robust and will break json files which have "//" inside of a string.
class ParsedJson final {
public:
	// Constructors & destructors
	// ---------------------------------------------------------------------------------------------

	ParsedJson() noexcept = default;
	ParsedJson(const ParsedJson&) = delete;
	ParsedJson& operator= (const ParsedJson&) = delete;
	ParsedJson(ParsedJson&& other) noexcept { this->swap(other); }
	ParsedJson& operator= (ParsedJson&& other) noexcept { this->swap(other); return *this; }
	~ParsedJson() noexcept { this->destroy(); }

	static ParsedJson parseString(
		const char* jsonString, sfz::Allocator* allocator, bool allowCppComments = true) noexcept;
	static ParsedJson parseFile(
		const char* jsonPath, sfz::Allocator* allocator, bool allowCppComments = true) noexcept;

	// State methods
	// ---------------------------------------------------------------------------------------------

	void swap(ParsedJson& other) noexcept;
	void destroy() noexcept;

	// Methods
	// ---------------------------------------------------------------------------------------------

	bool isValid() const noexcept { return mImpl != nullptr; }
	ParsedJsonNode root() const noexcept;

private:
	// Private members
	// ---------------------------------------------------------------------------------------------

	ParsedJsonImpl* mImpl = nullptr;
};

} // namespace ph
