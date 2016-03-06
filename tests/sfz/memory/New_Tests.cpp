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

#include "sfz/memory/New.hpp"

using namespace sfz;

TEST_CASE("Default constructed objects", "[sfz::sfz_new]")
{
	int flag = 0;

	struct TestClass {
		int* flagPtr = (int*)42;
		TestClass() = default;
		~TestClass()
		{
			*flagPtr = 1337;
		}
	};

	TestClass* ptr = nullptr;
	ptr = sfz_new<TestClass>();
	REQUIRE(ptr != nullptr);
	REQUIRE(ptr->flagPtr == (int*)42);

	ptr->flagPtr = &flag;
	REQUIRE(flag == 0);
	sfz_delete<TestClass>(ptr);
	REQUIRE(flag == 1337);
	REQUIRE(ptr == nullptr);
}
