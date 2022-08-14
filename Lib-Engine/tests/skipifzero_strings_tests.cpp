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

#include <doctest.h>

#include "skipifzero_strings.hpp"

// StringLocal tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("StringLocal: printf_constructor")
{
	SfzStr96 str1 = {};
	sfzStr96Appendf(&str1, "%s: %i", "Test", 1);

	SfzStr96 str2 = sfzStr96InitFmt("%s: %i", "Test", 1);
	CHECK(str1 == str2);

	SfzStr96 str3 = sfzStr96InitFmt("%s", "1234567890123456789012345678901234567890123456789012345678901234123456789012345678901234567890123456789012345678901234567890123extra");
	CHECK(strcmp(str3.str, "12345678901234567890123456789012345678901234567890123456789012341234567890123456789012345678901") == 0);

	CHECK(sfzStr96InitFmt("hello") == "hello");
}

TEST_CASE("StringLocal: implicit_conversion_operators")
{
	SfzStr96 str = sfzStr96Init("Hello");
	const char* contents = str.str;
	CHECK(str == contents);

	SfzStr96 str2 = sfzStr96Init("Hello2");
	CHECK(str2 == "Hello2");

	SfzStr96 str3 = {};
	str3 = sfzStr96Init("Hello3");
	CHECK(str3 == "Hello3");
}

TEST_CASE("StringLocal: appendf")
{
	SfzStr96 str = {};
	sfzStr96Appendf(&str, "%s: %i", "Test", 1);
	CHECK(strcmp(str.str, "Test: 1") == 0);

	sfzStr96Appendf(&str, " && %s: %i", "Test", 2);
	CHECK(strcmp(str.str, "Test: 1 && Test: 2") == 0);
}

TEST_CASE("StringLocal: append_chars")
{
	SfzStr32 str = {};
	const char* aStr = "1234567890123456789012345678901234567890";
	sfzStr32AppendChars(&str, aStr, 31);
	CHECK(str == "1234567890123456789012345678901");
	sfzStr32Clear(&str);
	sfzStr32AppendChars(&str, aStr, 4);
	CHECK(str == "1234");
	sfzStr32AppendChars(&str, aStr, 2);
	CHECK(str == "123412");
}

TEST_CASE("StringLocal: comparison_operators")
{
	SfzStr96 str = sfzStr96Init("aba");
	CHECK(str == "aba");
	CHECK(str != "afae");
}

TEST_CASE("StringLocal: trim")
{
	SfzStr96 str1 = sfzStr96Init("\n\t  \tcool\n \t ");
	sfzStr96Trim(&str1);
	CHECK(str1 == "cool");

	SfzStr96 str2 = sfzStr96Init("foo\n \t ");
	sfzStr96Trim(&str2);
	CHECK(str2 == "foo");

	SfzStr96 str3 = sfzStr96Init("\n\t  \tbar");
	sfzStr96Trim(&str3);
	CHECK(str3 == "bar");

	SfzStr96 str4 = sfzStr96Init("");
	sfzStr96Trim(&str4);
	CHECK(str4 == "");

	SfzStr96 str5 = sfzStr96Init("\n\t  \t");
	sfzStr96Trim(&str5);
	CHECK(str5 == "");
}

TEST_CASE("StringLocal: ends_with")
{
	SfzStr96 str1 = sfzStr96Init("");
	CHECK(sfzStr96EndsWith(&str1, ""));
	CHECK(!sfzStr96EndsWith(&str1, "a"));
	CHECK(!sfzStr96EndsWith(&str1, " "));

	SfzStr96 str2 = sfzStr96Init("cool.png");
	CHECK(sfzStr96EndsWith(&str2, ""));
	CHECK(!sfzStr96EndsWith(&str2, "a"));
	CHECK(sfzStr96EndsWith(&str2, ".png"));
	CHECK(sfzStr96EndsWith(&str2, "cool.png"));
}

TEST_CASE("StringLocal: contains")
{
	SfzStr96 str1 = sfzStr96Init("");
	CHECK(sfzStr96Contains(&str1, ""));
	CHECK(!sfzStr96Contains(&str1, " "));
	CHECK(!sfzStr96Contains(&str1, "\n"));
	CHECK(!sfzStr96Contains(&str1, "\t"));
	CHECK(!sfzStr96Contains(&str1, "a"));
	CHECK(!sfzStr96Contains(&str1, "B"));

	SfzStr96 str2 = sfzStr96Init("cool\t\n");
	CHECK(sfzStr96Contains(&str2, "cool\t\n"));
	CHECK(!sfzStr96Contains(&str2, " cool\t\n"));
	CHECK(sfzStr96Contains(&str2, "cool"));
	CHECK(sfzStr96Contains(&str2, "\t\n"));
	CHECK(sfzStr96Contains(&str2, ""));
}

TEST_CASE("StringLocal: is_part_of")
{
	SfzStr96 str1 = sfzStr96Init("");
	CHECK(sfzStr96IsPartOf(&str1, ""));
	CHECK(sfzStr96IsPartOf(&str1, " "));
	CHECK(sfzStr96IsPartOf(&str1, "\n"));
	CHECK(sfzStr96IsPartOf(&str1, "\t"));
	CHECK(sfzStr96IsPartOf(&str1, "a"));
	CHECK(sfzStr96IsPartOf(&str1, "B"));

	SfzStr96 str2 = sfzStr96Init("cool\t\n");
	CHECK(sfzStr96IsPartOf(&str2, "cool\t\n"));
	CHECK(sfzStr96IsPartOf(&str2, " cool\t\n"));
	CHECK(!sfzStr96IsPartOf(&str2, "cool"));
	CHECK(!sfzStr96IsPartOf(&str2, "\t\n"));
	CHECK(!sfzStr96IsPartOf(&str2, ""));
}

// String hashing tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("Hashing: fnv1a_hash_string")
{
	// Test values taken from public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
	// See http://isthe.com/chongo/tech/comp/fnv/
	CHECK(sfzHashStringFNV1a("") == u64(0xcbf29ce484222325));
	CHECK(sfzHashStringFNV1a("a") == u64(0xaf63dc4c8601ec8c));
	CHECK(sfzHashStringFNV1a("b") == u64(0xaf63df4c8601f1a5));
	CHECK(sfzHashStringFNV1a("c") == u64(0xaf63de4c8601eff2));
	CHECK(sfzHashStringFNV1a("foo") == u64(0xdcb27518fed9d577));
	CHECK(sfzHashStringFNV1a("foobar") == u64(0x85944171f73967e8));
	CHECK(sfzHashStringFNV1a("chongo was here!\n") == u64(0x46810940eff5f915));

	// Assumes sfzHash() is a wrapper around hashStringFNV1a()
	CHECK(sfzHash("") == u64(0xcbf29ce484222325));
	CHECK(sfzHash("a") == u64(0xaf63dc4c8601ec8c));
	CHECK(sfzHash("b") == u64(0xaf63df4c8601f1a5));
	CHECK(sfzHash("c") == u64(0xaf63de4c8601eff2));
	CHECK(sfzHash("foo") == u64(0xdcb27518fed9d577));
	CHECK(sfzHash("foobar") == u64(0x85944171f73967e8));
	CHECK(sfzHash("chongo was here!\n") == u64(0x46810940eff5f915));
}

TEST_CASE("Hashing: fnv1a_hash_bytes")
{
	// Test values taken from public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
	// See http://isthe.com/chongo/tech/comp/fnv/
	CHECK(sfzHashBytesFNV1a((const u8*)"", strlen("")) == u64(0xcbf29ce484222325));
	CHECK(sfzHashBytesFNV1a((const u8*)"a", strlen("a")) == u64(0xaf63dc4c8601ec8c));
	CHECK(sfzHashBytesFNV1a((const u8*)"b", strlen("b")) == u64(0xaf63df4c8601f1a5));
	CHECK(sfzHashBytesFNV1a((const u8*)"c", strlen("c")) == u64(0xaf63de4c8601eff2));
	CHECK(sfzHashBytesFNV1a((const u8*)"foo", strlen("foo")) == u64(0xdcb27518fed9d577));
	CHECK(sfzHashBytesFNV1a((const u8*)"foobar", strlen("foobar")) == u64(0x85944171f73967e8));
	CHECK(sfzHashBytesFNV1a((const u8*)"chongo was here!\n", strlen("chongo was here!\n")) == u64(0x46810940eff5f915));
}
