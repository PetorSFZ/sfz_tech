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

#include "skipifzero_strings.hpp"

// StringLocal tests
// ------------------------------------------------------------------------------------------------

UTEST(StringLocal, printf_constructor)
{
	sfz::str96 str1;
	str1.appendf("%s: %i", "Test", 1);

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

	sfz::str96 str2 = "Hello2";
	ASSERT_TRUE(str2 == "Hello2");

	sfz::str96 str3;
	str3 = "Hello3";
	ASSERT_TRUE(str3 == "Hello3");
}

UTEST(StringLocal, appendf)
{
	sfz::str96 str;
	str.appendf("%s: %i", "Test", 1);
	ASSERT_TRUE(strcmp(str.str(), "Test: 1") == 0);

	str.appendf(" && %s: %i", "Test", 2);
	ASSERT_TRUE(strcmp(str.str(), "Test: 1 && Test: 2") == 0);

}

UTEST(StringLocal, append_chars)
{
	sfz::str32 str;
	const char* aStr = "1234567890123456789012345678901234567890";
	str.appendChars(aStr, 31);
	ASSERT_TRUE(str == "1234567890123456789012345678901");
	str.clear();
	str.appendChars(aStr, 4);
	ASSERT_TRUE(str == "1234");
	str.appendChars(aStr, 2);
	ASSERT_TRUE(str == "123412");
}

UTEST(StringLocal, comparison_operators)
{
	sfz::str96 str("aba");
	ASSERT_TRUE(str == "aba");
	ASSERT_TRUE(str != "afae");
	ASSERT_TRUE(str < "bbb");
	ASSERT_TRUE(str > "aaa");
}

UTEST(StringLocal, trim)
{
	sfz::str96 str1 = "\n\t  \tcool\n \t ";
	str1.trim();
	ASSERT_TRUE(str1 == "cool");

	sfz::str96 str2 = "foo\n \t ";
	str2.trim();
	ASSERT_TRUE(str2 == "foo");

	sfz::str96 str3 = "\n\t  \tbar";
	str3.trim();
	ASSERT_TRUE(str3 == "bar");

	sfz::str96 str4 = "";
	str4.trim();
	ASSERT_TRUE(str4 == "");

	sfz::str96 str5 = "\n\t  \t";
	str5.trim();
	ASSERT_TRUE(str5 == "");
}

UTEST(StringLocal, ends_with)
{
	sfz::str96 str1 = "";
	ASSERT_TRUE(str1.endsWith(""));
	ASSERT_TRUE(!str1.endsWith("a"));
	ASSERT_TRUE(!str1.endsWith(" "));

	sfz::str96 str2 = "cool.png";
	ASSERT_TRUE(str2.endsWith(""));
	ASSERT_TRUE(!str2.endsWith("a"));
	ASSERT_TRUE(str2.endsWith(".png"));
	ASSERT_TRUE(str2.endsWith("cool.png"));
}

UTEST(StringLocal, contains)
{
	sfz::str96 str1 = "";
	ASSERT_TRUE(str1.contains(""));
	ASSERT_TRUE(!str1.contains(" "));
	ASSERT_TRUE(!str1.contains("\n"));
	ASSERT_TRUE(!str1.contains("\t"));
	ASSERT_TRUE(!str1.contains("a"));
	ASSERT_TRUE(!str1.contains("B"));

	sfz::str96 str2 = "cool\t\n";
	ASSERT_TRUE(str2.contains("cool\t\n"));
	ASSERT_TRUE(!str2.contains(" cool\t\n"));
	ASSERT_TRUE(str2.contains("cool"));
	ASSERT_TRUE(str2.contains("\t\n"));
	ASSERT_TRUE(str2.contains(""));
}

UTEST(StringLocal, is_part_of)
{
	sfz::str96 str1 = "";
	ASSERT_TRUE(str1.isPartOf(""));
	ASSERT_TRUE(str1.isPartOf(" "));
	ASSERT_TRUE(str1.isPartOf("\n"));
	ASSERT_TRUE(str1.isPartOf("\t"));
	ASSERT_TRUE(str1.isPartOf("a"));
	ASSERT_TRUE(str1.isPartOf("B"));

	sfz::str96 str2 = "cool\t\n";
	ASSERT_TRUE(str2.isPartOf("cool\t\n"));
	ASSERT_TRUE(str2.isPartOf(" cool\t\n"));
	ASSERT_TRUE(!str2.isPartOf("cool"));
	ASSERT_TRUE(!str2.isPartOf("\t\n"));
	ASSERT_TRUE(!str2.isPartOf(""));
}

// String hashing tests
// ------------------------------------------------------------------------------------------------

UTEST(Hashing, fnv1a_hash_string)
{
	// Test values taken from public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
	// See http://isthe.com/chongo/tech/comp/fnv/
	ASSERT_TRUE(sfzHashStringFNV1a("") == u64(0xcbf29ce484222325));
	ASSERT_TRUE(sfzHashStringFNV1a("a") == u64(0xaf63dc4c8601ec8c));
	ASSERT_TRUE(sfzHashStringFNV1a("b") == u64(0xaf63df4c8601f1a5));
	ASSERT_TRUE(sfzHashStringFNV1a("c") == u64(0xaf63de4c8601eff2));
	ASSERT_TRUE(sfzHashStringFNV1a("foo") == u64(0xdcb27518fed9d577));
	ASSERT_TRUE(sfzHashStringFNV1a("foobar") == u64(0x85944171f73967e8));
	ASSERT_TRUE(sfzHashStringFNV1a("chongo was here!\n") == u64(0x46810940eff5f915));

	// Assumes sfz::hash() is a wrapper around hashStringFNV1a()
	ASSERT_TRUE(sfz::hash("") == u64(0xcbf29ce484222325));
	ASSERT_TRUE(sfz::hash("a") == u64(0xaf63dc4c8601ec8c));
	ASSERT_TRUE(sfz::hash("b") == u64(0xaf63df4c8601f1a5));
	ASSERT_TRUE(sfz::hash("c") == u64(0xaf63de4c8601eff2));
	ASSERT_TRUE(sfz::hash("foo") == u64(0xdcb27518fed9d577));
	ASSERT_TRUE(sfz::hash("foobar") == u64(0x85944171f73967e8));
	ASSERT_TRUE(sfz::hash("chongo was here!\n") == u64(0x46810940eff5f915));
}

UTEST(Hashing, fnv1a_hash_bytes)
{
	// Test values taken from public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
	// See http://isthe.com/chongo/tech/comp/fnv/
	ASSERT_TRUE(sfzHashBytesFNV1a((const u8*)"", strlen("")) == u64(0xcbf29ce484222325));
	ASSERT_TRUE(sfzHashBytesFNV1a((const u8*)"a", strlen("a")) == u64(0xaf63dc4c8601ec8c));
	ASSERT_TRUE(sfzHashBytesFNV1a((const u8*)"b", strlen("b")) == u64(0xaf63df4c8601f1a5));
	ASSERT_TRUE(sfzHashBytesFNV1a((const u8*)"c", strlen("c")) == u64(0xaf63de4c8601eff2));
	ASSERT_TRUE(sfzHashBytesFNV1a((const u8*)"foo", strlen("foo")) == u64(0xdcb27518fed9d577));
	ASSERT_TRUE(sfzHashBytesFNV1a((const u8*)"foobar", strlen("foobar")) == u64(0x85944171f73967e8));
	ASSERT_TRUE(sfzHashBytesFNV1a((const u8*)"chongo was here!\n", strlen("chongo was here!\n")) == u64(0x46810940eff5f915));
}
