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

#include "sfz/PushWarnings.hpp"
#include "catch2/catch.hpp"
#include "sfz/PopWarnings.hpp"

#include <algorithm>

#include "sfz/containers/DynArray.hpp"
#include "sfz/util/Enumerate.hpp"

using namespace sfz;

struct Counting {

	uint32_t payload = ~0;
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
		std::swap(this->payload, other.payload);
		std::swap(this->copyCounter, other.copyCounter);
		std::swap(this->moveCounter, other.moveCounter);
		this->moveCounter += 1;
		other.moveCounter += 1;
	}
	Counting& operator= (Counting&& other) noexcept
	{
		std::swap(this->payload, other.payload);
		std::swap(this->copyCounter, other.copyCounter);
		std::swap(this->moveCounter, other.moveCounter);
		this->moveCounter += 1;
		other.moveCounter += 1;
		return *this;
	}
};

TEST_CASE("Basic enumerate() tests", "[sfz::enumerate()]")
{
	DynArray<Counting> elements;
	elements.ensureCapacity(32);
	for (uint32_t i = 0; i < 10; i++) {
		elements.add(Counting(i));
	}

	for (uint32_t i = 0; i < 10; i++) {
		const Counting& elem = elements[i];
		REQUIRE(elem.payload == i);
		REQUIRE(elem.copyCounter == 0);
		REQUIRE(elem.moveCounter == 1);
	}

	uint32_t counter = 0;
	for (auto e : enumerate(elements)) {
		REQUIRE(e.idx == counter);
		REQUIRE(e.element.payload == counter);
		REQUIRE(e.element.copyCounter == 0);
		REQUIRE(e.element.moveCounter == 1);
		counter += 1;
	}
}
