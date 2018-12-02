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

namespace ph {

// ArrayHeader struct
// ------------------------------------------------------------------------------------------------

// The header for an in-place array.
//
// I.e., a chunk of memory could look like the following:
// | ArrayHeader |
// | Element 0   |
// | Element 1   |
// | ...         |
// | Element N   |
// [ First byte after array ]
//
// The ArrayHeader has methods for accessing the elements in the array following it in memory. It
// also has methods for getting a pointer to the first byte after the array, which could be useful
// if having multiple ArrayHeaders tightly packed in a chunk of memory.
struct ArrayHeader final {

	// Public members
	// --------------------------------------------------------------------------------------------

	uint32_t size;
	uint32_t elementSize;
	uint32_t capacity;
	uint8_t ___padding___[20];

	// Contructor functions
	// --------------------------------------------------------------------------------------------

	// POD, so everything is default
	ArrayHeader() noexcept = default;
	ArrayHeader(const ArrayHeader&) noexcept = default;
	ArrayHeader& operator= (const ArrayHeader&) noexcept = default;
	~ArrayHeader() noexcept = default;

	static ArrayHeader createUntyped(uint32_t capacity, uint32_t elementSize) noexcept
	{
		ArrayHeader header = {};
		header.size = 0;
		header.elementSize = elementSize;
		header.capacity = capacity;
		return header;
	}

	template<typename T>
	static ArrayHeader create(uint32_t capacity) noexcept
	{
		return ArrayHeader::createUntyped(capacity, sizeof(T));
	}

	// Untyped accessors
	// --------------------------------------------------------------------------------------------

	uint8_t* dataUntyped() noexcept { return (uint8_t*)this + sizeof(ArrayHeader); }
	const uint8_t* dataUntyped() const noexcept { return (const uint8_t*)this + sizeof(ArrayHeader); }

	uint8_t* atUntyped(uint32_t index) noexcept { return dataUntyped() + index * elementSize; }
	const uint8_t* atUntyped(uint32_t index) const noexcept { return dataUntyped() + index * elementSize; }

	// Typed accessors
	// --------------------------------------------------------------------------------------------

	template<typename T>
	T* data() noexcept { return (T*)this->dataUntyped(); }

	template<typename T>
	const T* data() const noexcept { return (const T*)this->dataUntyped(); }

	template<typename T>
	T& at(uint32_t index) noexcept { return this->data<T>()[index]; }

	template<typename T>
	const T& at(uint32_t index) const noexcept { return this->data<T>()[index]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	void addUntyped(const uint8_t* data, uint32_t numBytes) noexcept;
	
	template<typename T>
	void add(const T& data) noexcept { addUntyped((const uint8_t*)&data, sizeof(T)); }

	void pop() noexcept;

	bool popGetUntyped(uint8_t* dst) noexcept;

	template<typename T>
	bool popGet(T& out) noexcept { return popGetUntyped(reinterpret_cast<uint8_t*>(&out)); }

	// Memory helpers
	// --------------------------------------------------------------------------------------------

	uint32_t numBytesNeededForArrayPart() const noexcept;
	uint32_t numBytesNeededForArrayPart32Byte() const noexcept;
	uint32_t numBytesNeededForArrayPlusHeader() const noexcept;
	uint32_t numBytesNeededForArrayPlusHeader32Byte() const noexcept;

	uint8_t* firstByteAfterArray() noexcept;
	const uint8_t* firstByteAfterArray() const noexcept;

	uint8_t* firstByteAfterArray32Byte() noexcept;
	const uint8_t* firstByteAfterArray32Byte() const noexcept;
};
static_assert(sizeof(ArrayHeader) == 32, "ArrayHeader is not 32-byte");

} // namespace ph
