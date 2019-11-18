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

#include "ph/state/ArrayHeader.hpp"

#include <cstring>

#include <skipifzero.hpp>

namespace ph {

// ArrayHeader: Methods
// ------------------------------------------------------------------------------------------------

void ArrayHeader::addUntyped(const uint8_t* data, uint32_t numBytes) noexcept
{
	sfz_assert(numBytes == this->elementSize);
	sfz_assert(this->size < this->capacity);

	// Add element to array and increment size
	uint8_t* dstPtr = atUntyped(this->size);
	memcpy(dstPtr, data, numBytes);
	this->size += 1;
}

void ArrayHeader::pop() noexcept
{
	sfz_assert(0 < this->size);

	// Clear element and decrement size pointer
	uint8_t* dstPtr = atUntyped(this->size - 1);
	memset(dstPtr, 0, this->elementSize);
	this->size -= 1;
}

bool ArrayHeader::popGetUntyped(uint8_t* dst) noexcept
{
	if (0 == this->size) return false;

	uint8_t* src = atUntyped(this->size - 1);
	memcpy(dst, src, this->elementSize);
	memset(src, 0, this->elementSize);
	this->size -= 1;

	return true;
}

// ArrayHeader: Memory helpers
// ------------------------------------------------------------------------------------------------

uint32_t ArrayHeader::numBytesNeededForArrayPart() const noexcept
{
	return capacity * elementSize;
}

uint32_t ArrayHeader::numBytesNeededForArrayPart32Byte() const noexcept
{
	uint32_t bytesBeforePadding = numBytesNeededForArrayPart();
	uint32_t padding = 32 - (bytesBeforePadding & 0x1F); // bytesBeforePadding % 32
	if (padding == 32) padding = 0;
	return bytesBeforePadding + padding;
}

uint32_t ArrayHeader::numBytesNeededForArrayPlusHeader() const noexcept
{
	uint32_t arrayPart = numBytesNeededForArrayPart();
	return arrayPart + sizeof(ArrayHeader);
}

uint32_t ArrayHeader::numBytesNeededForArrayPlusHeader32Byte() const noexcept
{
	uint32_t arrayPart32Byte = numBytesNeededForArrayPart32Byte();
	return arrayPart32Byte + sizeof(ArrayHeader);
}

uint8_t* ArrayHeader::firstByteAfterArray() noexcept
{
	return dataUntyped() + numBytesNeededForArrayPart();
}

const uint8_t* ArrayHeader::firstByteAfterArray() const noexcept
{
	return dataUntyped() + numBytesNeededForArrayPart();
}

uint8_t* ArrayHeader::firstByteAfterArray32Byte() noexcept
{
	return dataUntyped() + numBytesNeededForArrayPart32Byte();
}

const uint8_t* ArrayHeader::firstByteAfterArray32Byte() const noexcept
{
	return dataUntyped() + numBytesNeededForArrayPart32Byte();
}

} // namespace ph
