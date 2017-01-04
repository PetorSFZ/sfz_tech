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

#include "sfz/strings/DynString.hpp"

using namespace sfz;

TEST_CASE("constructor (const char* string, uint32_t capacity)", "[sfz::DynString]")
{
	DynString str1("Hello World");
	REQUIRE(std::strcmp(str1.str(), "Hello World") == 0);
	REQUIRE(str1.size() == 11);
	REQUIRE(str1.capacity() == 12);

	DynString str2(nullptr);
	REQUIRE(str2.str() == nullptr);
	REQUIRE(str2.size() == 0);
	REQUIRE(str2.capacity() == 0);;

	DynString str3(nullptr, 16);
	REQUIRE(str3.str() != nullptr);
	REQUIRE(str3.size() == 0);
	REQUIRE(str3.capacity() == 16);

	DynString str4("4th", 8);
	REQUIRE(std::strcmp(str4.str(), "4th") == 0);
	REQUIRE(str4.size() == 3);
	REQUIRE(str4.capacity() == 8);
}

TEST_CASE("DynString: printf() & printfAppend()", "[sfz::DynString]")
{
	DynString str(nullptr, 128);
	str.printf("%s: %i", "Test", 1);
	REQUIRE(strcmp(str.str(), "Test: 1") == 0);
	REQUIRE(str.size() == std::strlen("Test: 1"));

	str.printfAppend(" && %s: %i", "Test", 2);
	REQUIRE(strcmp(str.str(), "Test: 1 && Test: 2") == 0);
	REQUIRE(str.size() == std::strlen("Test: 1 && Test: 2"));

	str.printfAppend(" && %s: %i", "Test", 3);
	REQUIRE(strcmp(str.str(), "Test: 1 && Test: 2 && Test: 3") == 0);
	REQUIRE(str.size() == std::strlen("Test: 1 && Test: 2 && Test: 3"));

	str.printf("%s", "--");
	REQUIRE(strcmp(str.str(), "--") == 0);
	REQUIRE(str.size() == std::strlen("--"));
}

TEST_CASE("DynString comparison operators", "[sfz::DynString]")
{
	DynString str("aba");
	REQUIRE(str == "aba");
	REQUIRE(str != "afae");
	REQUIRE(str < "bbb");
	REQUIRE(str > "aaa");
}
