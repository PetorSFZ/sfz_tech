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

#include "sfz/state/ArrayHeader.hpp"

#include <cstring>

#include <skipifzero.hpp>

namespace sfz {

// ArrayHeader: Constructor functions
// ------------------------------------------------------------------------------------------------

void ArrayHeader::createUntyped(u32 capacityIn, u32 elementSizeIn) noexcept
{
	memset(this, 0, sizeof(ArrayHeader));
	this->size = 0;
	this->elementSize = elementSizeIn;
	this->capacity = capacityIn;
}

// ArrayHeader: Methods
// ------------------------------------------------------------------------------------------------

void ArrayHeader::addUntyped(const u8* data, u32 numBytes) noexcept
{
	sfz_assert(numBytes == this->elementSize);
	sfz_assert(this->size < this->capacity);

	// Add element to array and increment size
	u8* dstPtr = atUntyped(this->size);
	memcpy(dstPtr, data, numBytes);
	this->size += 1;
}

void ArrayHeader::pop() noexcept
{
	sfz_assert(0 < this->size);

	// Clear element and decrement size pointer
	u8* dstPtr = atUntyped(this->size - 1);
	memset(dstPtr, 0, this->elementSize);
	this->size -= 1;
}

bool ArrayHeader::popGetUntyped(u8* dst) noexcept
{
	if (0 == this->size) return false;

	u8* src = atUntyped(this->size - 1);
	memcpy(dst, src, this->elementSize);
	memset(src, 0, this->elementSize);
	this->size -= 1;

	return true;
}

} // namespace sfz
