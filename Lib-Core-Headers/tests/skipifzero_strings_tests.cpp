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
//    appreciated but is not ASSERT_TRUEd.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "utest.h"
#undef near
#undef far

#include "skipifzero_strings.hpp"

// StringLocal tests
// ------------------------------------------------------------------------------------------------

UTEST(StringLocal, printf_constructor)
{
	sfz::str96 str1;
	str1.printf("%s: %i", "Test", 1);

	sfz::str96 str2("%s: %i", "Test", 1);
	ASSERT_TRUE(str1 == str2);

	sfz::str128 str3("%s", "1234567890123456789012345678901234567890123456789012345678901234123456789012345678901234567890123456789012345678901234567890123extra");
	ASSERT_TRUE(strcmp(str3.str(), "1234567890123456789012345678901234567890123456789012345678901234123456789012345678901234567890123456789012345678901234567890123") == 0);

	ASSERT_TRUE(sfz::str96("hello") == "hello");
}

UTEST(StringLocal, implicit_conversion_operators)
{
	sfz::str96 str("Hello");
	const char* contents = str;
	ASSERT_TRUE(str == contents);
}

UTEST(StringLocal, printf_and_printfappend)
{
	sfz::str96 str;
	str.printf("%s: %i", "Test", 1);
	ASSERT_TRUE(strcmp(str.str(), "Test: 1") == 0);

	str.printfAppend(" && %s: %i", "Test", 2);
	ASSERT_TRUE(strcmp(str.str(), "Test: 1 && Test: 2") == 0);

}

UTEST(StringLocal, Insert_chars)
{
	sfz::str32 str;
	const char* aStr = "1234567890123456789012345678901234567890";
	str.insertChars(aStr, 31);
	ASSERT_TRUE(str == "1234567890123456789012345678901");
	str.insertChars(aStr, 4);
	ASSERT_TRUE(str == "1234");
}

UTEST(StringLocal, comparison_operators)
{
	sfz::str96 str("aba");
	ASSERT_TRUE(str == "aba");
	ASSERT_TRUE(str != "afae");
	ASSERT_TRUE(str < "bbb");
	ASSERT_TRUE(str > "aaa");
}

// String hashing tests
// ------------------------------------------------------------------------------------------------

UTEST(Hashing, fnv1a_hash_string)
{
	// Test values taken from public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
	// See http://isthe.com/chongo/tech/comp/fnv/
	ASSERT_TRUE(sfz::hashStringFNV1a("") == uint64_t(0xcbf29ce484222325));
	ASSERT_TRUE(sfz::hashStringFNV1a("a") == uint64_t(0xaf63dc4c8601ec8c));
	ASSERT_TRUE(sfz::hashStringFNV1a("b") == uint64_t(0xaf63df4c8601f1a5));
	ASSERT_TRUE(sfz::hashStringFNV1a("c") == uint64_t(0xaf63de4c8601eff2));
	ASSERT_TRUE(sfz::hashStringFNV1a("foo") == uint64_t(0xdcb27518fed9d577));
	ASSERT_TRUE(sfz::hashStringFNV1a("foobar") == uint64_t(0x85944171f73967e8));
	ASSERT_TRUE(sfz::hashStringFNV1a("chongo was here!\n") == uint64_t(0x46810940eff5f915));

	// Assumes sfz::hash() is a wrapper around hashStringFNV1a()
	ASSERT_TRUE(sfz::hash("") == uint64_t(0xcbf29ce484222325));
	ASSERT_TRUE(sfz::hash("a") == uint64_t(0xaf63dc4c8601ec8c));
	ASSERT_TRUE(sfz::hash("b") == uint64_t(0xaf63df4c8601f1a5));
	ASSERT_TRUE(sfz::hash("c") == uint64_t(0xaf63de4c8601eff2));
	ASSERT_TRUE(sfz::hash("foo") == uint64_t(0xdcb27518fed9d577));
	ASSERT_TRUE(sfz::hash("foobar") == uint64_t(0x85944171f73967e8));
	ASSERT_TRUE(sfz::hash("chongo was here!\n") == uint64_t(0x46810940eff5f915));
}

UTEST(Hashing, fnv1a_hash_bytes)
{
	// Test values taken from public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
	// See http://isthe.com/chongo/tech/comp/fnv/
	ASSERT_TRUE(sfz::hashBytesFNV1a((const uint8_t*)"", strlen("")) == uint64_t(0xcbf29ce484222325));
	ASSERT_TRUE(sfz::hashBytesFNV1a((const uint8_t*)"a", strlen("a")) == uint64_t(0xaf63dc4c8601ec8c));
	ASSERT_TRUE(sfz::hashBytesFNV1a((const uint8_t*)"b", strlen("b")) == uint64_t(0xaf63df4c8601f1a5));
	ASSERT_TRUE(sfz::hashBytesFNV1a((const uint8_t*)"c", strlen("c")) == uint64_t(0xaf63de4c8601eff2));
	ASSERT_TRUE(sfz::hashBytesFNV1a((const uint8_t*)"foo", strlen("foo")) == uint64_t(0xdcb27518fed9d577));
	ASSERT_TRUE(sfz::hashBytesFNV1a((const uint8_t*)"foobar", strlen("foobar")) == uint64_t(0x85944171f73967e8));
	ASSERT_TRUE(sfz::hashBytesFNV1a((const uint8_t*)"chongo was here!\n", strlen("chongo was here!\n")) == uint64_t(0x46810940eff5f915));
}
