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

#include <skipifzero.hpp>

namespace sfz {

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
struct ArrayHeader final {

	// Public members
	// --------------------------------------------------------------------------------------------

	uint32_t size;
	uint32_t elementSize;
	uint32_t capacity;
	uint8_t ___padding___[4];

	// Contructor functions
	// --------------------------------------------------------------------------------------------

	// POD, default constructible and destructible
	ArrayHeader() noexcept = default;
	~ArrayHeader() noexcept = default;

	// Copying and moving of the ArrayHeader struct is forbidden. This is a bit of a hack to
	// avoid a certain class of bugs. Essentially, the ArrayHeader assumes it is at the top of
	// a chunk of memory containing the entire array. If it is not, it will read/write invalid
	// memory for some of its operations. E.g. this could happen if you attempted to do this:
	//
	// ArrayHeader* arrayPtr = ... // (Pointer to memory chunk containing header and array)
	// ArrayHeader header = *arrayPtr; // Copies header to header, but not the array
	// header.someOperation(); // Invalid memory access because there is no array here
	//
	// By disabling the copy constructors the above code would give compile error. Do note that
	// ArrayHeader is still POD (i.e. trivially copyable), even though the C++ compiler does not
	// consider it to be. It is still completely fine to memcpy() and such as long as you know what
	// you are doing.
	ArrayHeader(const ArrayHeader&) = delete;
	ArrayHeader& operator=(const ArrayHeader&) = delete;
	ArrayHeader(ArrayHeader&&) = delete;
	ArrayHeader& operator=(ArrayHeader&&) = delete;

	void createUntyped(uint32_t capacityIn, uint32_t elementSizeIn) noexcept;

	void createCopy(ArrayHeader& other) noexcept
	{
		this->createUntyped(other.capacity, other.elementSize);
	}

	template<typename T>
	void create(uint32_t capacity) noexcept { return this->createUntyped(capacity, sizeof(T)); }

	// Untyped accessors
	// --------------------------------------------------------------------------------------------

	uint8_t* dataUntyped() noexcept { return ((uint8_t*)this) + sizeof(ArrayHeader); }
	const uint8_t* dataUntyped() const noexcept { return ((const uint8_t*)this) + sizeof(ArrayHeader); }

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
};
static_assert(sizeof(ArrayHeader) == 16, "ArrayHeader is not 16-byte");

constexpr uint32_t calcArrayHeaderSizeBytes(uint32_t componentSize, uint32_t numComponents)
{
	return uint32_t(roundUpAligned(sizeof(ArrayHeader) + componentSize * numComponents, 16));
}

} // namespace sfz
