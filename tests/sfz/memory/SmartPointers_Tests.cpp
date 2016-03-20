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
#include "sfz/memory/SmartPointers.hpp"

using namespace sfz;

TEST_CASE("Basic UniquePtr tests", "[sfz::UniquePtr]")
{
	int flag = 0;

	struct TestClass {
		int* flagPtr;
		TestClass(int* ptr)
		{
			flagPtr = ptr;
			*flagPtr = 1;
		}
		~TestClass()
		{
			*flagPtr = 2;
		}
	};

	UniquePtr<TestClass> ptr = nullptr;
	REQUIRE(ptr == nullptr);
	ptr = UniquePtr<TestClass>{sfz_new<TestClass>(&flag)};
	REQUIRE(ptr.get() != nullptr);
	REQUIRE(ptr != nullptr);
	REQUIRE(ptr.get()->flagPtr == &flag);
	REQUIRE((*ptr).flagPtr == &flag);
	REQUIRE(ptr->flagPtr == &flag);
	REQUIRE(flag == 1);

	UniquePtr<TestClass> second;
	REQUIRE(second == nullptr);
	second = std::move(ptr);
	REQUIRE(ptr == nullptr);
	REQUIRE(second != nullptr);

	ptr.destroy();
	REQUIRE(flag == 1);
	REQUIRE(ptr == nullptr);
	
	second.destroy();
	REQUIRE(flag == 2);
	REQUIRE(second == nullptr);
}