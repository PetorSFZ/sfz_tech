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

#include <cstdio>
#include <cstring>

#include "sfz/strings/StackString.hpp"

using namespace sfz;

TEST_CASE("printf() constructor", "[sfz::StackString]")
{
	StackString str1;
	str1.printf("%s: %i", "Test", 1);
	
	StackString str2("%s: %i", "Test", 1);
	REQUIRE(str1 == str2);

	StackString128 str3("%s", "1234567890123456789012345678901234567890123456789012345678901234123456789012345678901234567890123456789012345678901234567890123extra");
	REQUIRE(strcmp(str3.str, "1234567890123456789012345678901234567890123456789012345678901234123456789012345678901234567890123456789012345678901234567890123") == 0);

	REQUIRE(str96("hello") == "hello");
}

TEST_CASE("Implicit conversion operators", "[sfz::StackString]")
{
	str96 str("Hello");
	const char* contents = str;
	REQUIRE(str == contents);
}

TEST_CASE("printf() & printfAppend()", "[sfz::StackString]")
{
	StackString str;
	str.printf("%s: %i", "Test", 1);
	REQUIRE(strcmp(str.str, "Test: 1") == 0);

	str.printfAppend(" && %s: %i", "Test", 2);
	REQUIRE(strcmp(str.str, "Test: 1 && Test: 2") == 0);

}

TEST_CASE("insertChars()", "[sfz::StackString]")
{
	StackString32 str;
	const char* aStr = "1234567890123456789012345678901234567890";
	str.insertChars(aStr, 31);
	REQUIRE(str == "1234567890123456789012345678901");
	str.insertChars(aStr, 4);
	REQUIRE(str == "1234");
}

TEST_CASE("StackString comparison operators", "[sfz::StackString]")
{
	StackString str("aba");
	REQUIRE(str == "aba");
	REQUIRE(str != "afae");
	REQUIRE(str < "bbb");
	REQUIRE(str > "aaa");
}
