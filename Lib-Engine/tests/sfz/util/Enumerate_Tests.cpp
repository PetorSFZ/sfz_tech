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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "utest.h"
#undef near
#undef far

#include <algorithm>

#include <skipifzero_arrays.hpp>

#include "sfz/Context.hpp"
#include "sfz/util/Enumerate.hpp"

using namespace sfz;

struct Counting {

	uint32_t payload = ~0u;
	uint32_t copyCounter = 0;
	uint32_t moveCounter = 0;

	Counting() noexcept = default;
	Counting(uint32_t payload) noexcept : payload(payload) {}
	Counting(const Counting& other) noexcept
	{
		this->payload = other.payload;
		this->copyCounter = other.copyCounter;
		this->moveCounter = other.moveCounter;
		this->copyCounter += 1;
	}
	Counting& operator= (const Counting& other) noexcept
	{
		this->payload = other.payload;
		this->copyCounter = other.copyCounter;
		this->moveCounter = other.moveCounter;
		this->copyCounter += 1;
		return *this;
	}
	Counting(Counting&& other) noexcept
	{
		sfz::swap(this->payload, other.payload);
		sfz::swap(this->copyCounter, other.copyCounter);
		sfz::swap(this->moveCounter, other.moveCounter);
		this->moveCounter += 1;
		other.moveCounter += 1;
	}
	Counting& operator= (Counting&& other) noexcept
	{
		sfz::swap(this->payload, other.payload);
		sfz::swap(this->copyCounter, other.copyCounter);
		sfz::swap(this->moveCounter, other.moveCounter);
		this->moveCounter += 1;
		other.moveCounter += 1;
		return *this;
	}
};

UTEST(enumerate, basic_tests)
{
	Array<Counting> elements(0, getDefaultAllocator(), sfz_dbg("elements"));
	elements.ensureCapacity(32);
	for (uint32_t i = 0; i < 10; i++) {
		elements.add(Counting(i));
	}

	for (uint32_t i = 0; i < 10; i++) {
		const Counting& elem = elements[i];
		ASSERT_TRUE(elem.payload == i);
		ASSERT_TRUE(elem.copyCounter == 0);
		ASSERT_TRUE(elem.moveCounter == 1);
	}

	uint32_t counter = 0;
	for (auto e : enumerate(elements)) {
		ASSERT_TRUE(e.idx == counter);
		ASSERT_TRUE(e.element.payload == counter);
		ASSERT_TRUE(e.element.copyCounter == 0);
		ASSERT_TRUE(e.element.moveCounter == 1);
		counter += 1;
	}
}
