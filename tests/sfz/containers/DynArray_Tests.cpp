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
#include "catch.hpp"
#include "sfz/PopWarnings.hpp"

#include "sfz/containers/DynArray.hpp"
#include "sfz/memory/SmartPointers.hpp"

using namespace sfz;

TEST_CASE("Default constructor", "[sfz::DynArray]")
{
	DynArray<float> floatArray;
	REQUIRE(floatArray.size() == 0);
	REQUIRE(floatArray.capacity() == 0);
	REQUIRE(floatArray.data() == nullptr);
}

TEST_CASE("Fill constructor", "[sfz::DynArray]")
{
	DynArray<UniquePtr<int>> nullptrs{8};
	for (uint32_t i = 0; i < 8; ++i) {
		REQUIRE(nullptrs.data()[i] == nullptr);
	}
	REQUIRE(nullptrs.size() == 8);
	REQUIRE(nullptrs.capacity() == 8);

	DynArray<int> twos{8, 2};
	for (uint32_t i = 0; i < 8; ++i) {
		REQUIRE(twos.data()[i] == 2);
	}
	REQUIRE(twos.size() == 8);
	REQUIRE(twos.capacity() == 8);

	nullptrs.destroy();
	REQUIRE(nullptrs.data() == nullptr);
	REQUIRE(nullptrs.size() == 0);
	REQUIRE(nullptrs.capacity() == 0);

	twos.destroy();
	REQUIRE(twos.data() == nullptr);
	REQUIRE(twos.size() == 0);
	REQUIRE(twos.capacity() == 0);
}
