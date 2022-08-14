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

#include <sfz_math.h>
#include <skipifzero_allocators.hpp>

#include "sfz/util/IniParser.hpp"
#include "sfz/util/IO.hpp"

// TODO: Fix test cases for iOS
#ifndef SFZ_IOS

using namespace sfz;

static const char* stupidFileName = "fafeafeafeafaefa.ini";

TEST_CASE("IniParser: basic_tests")
{
	SfzAllocator allocator = sfz::createStandardAllocator();
	const char* fpath = stupidFileName;

	IniParser ini1(fpath, &allocator);

	ini1.setBool("Section1", "bBool1", true);
	ini1.setBool("Section1", "bBool2", false);
	ini1.setFloat("Section2", "fFloat1", 3.5f);
	ini1.setInt("Section2", "iInt1", -23);

	CHECK(*ini1.getBool("Section1", "bBool1") == true);
	CHECK(*ini1.getBool("Section1", "bBool2") == false);
	CHECK(*ini1.getFloat("Section2", "fFloat1") == 3.5f);
	CHECK(*ini1.getInt("Section2", "iInt1") == -23);

	sfz::deleteFile(fpath);
	CHECK(ini1.save());

	IniParser ini2(fpath, &allocator);
	CHECK(ini2.load());

	CHECK(*ini2.getBool("Section1", "bBool1"));
	CHECK(!*ini2.getBool("Section1", "bBool2"));
	CHECK(*ini2.getFloat("Section2", "fFloat1") == 3.5f);
	CHECK(*ini2.getInt("Section2", "iInt1") == -23);

	ini1.setBool("Section1", "bBool1", true);
	ini1.setBool("Section1", "bBool2", false);
	ini1.setFloat("Section2", "fFloat1", 3.5f);
	ini1.setInt("Section2", "iInt1", -23);

	sfz::deleteFile(fpath);
}

TEST_CASE("IniParser: sanitizer_methods")
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	// sanitizeInt()
	{
		const char* fpath = stupidFileName;
		sfz::deleteFile(fpath);
		IniParser ini(fpath, &allocator);

		CHECK(ini.getInt("", "val1") == nullptr);
		CHECK(ini.sanitizeInt("", "val1") == 0);
		CHECK(ini.getInt("", "val1") != nullptr);
		CHECK(*ini.getInt("", "val1") == 0);

		CHECK(ini.getInt("", "val2") == nullptr);
		CHECK(ini.sanitizeInt("", "val2", 37) == 37);
		CHECK(ini.getInt("", "val2") != nullptr);
		CHECK(*ini.getInt("", "val2") == 37);

		CHECK(ini.sanitizeInt("", "val2", 0, 0, 36) == 36);
		CHECK(*ini.getInt("", "val2") == 36);
		CHECK(ini.sanitizeInt("", "val2", 0, 38, 39) == 38);
		CHECK(*ini.getInt("", "val2") == 38);
	}
	// sanitizeFloat()
	{
		const char* fpath = stupidFileName;
		sfz::deleteFile(fpath);
		IniParser ini(fpath, &allocator);

		CHECK(ini.getFloat("", "val1") == nullptr);
		CHECK(eqf(ini.sanitizeFloat("", "val1"), 0.0f));
		CHECK(ini.getFloat("", "val1") != nullptr);
		CHECK(eqf(*ini.getFloat("", "val1"), 0.0f));

		CHECK(ini.getFloat("", "val2") == nullptr);
		CHECK(eqf(ini.sanitizeFloat("", "val2", 37.0f), 37.0f));
		CHECK(ini.getFloat("", "val2") != nullptr);
		CHECK(eqf(*ini.getFloat("", "val2"), 37.0f));

		CHECK(eqf(ini.sanitizeFloat("", "val2", 0.0f, 0.0f, 36.0f), 36.0f));
		CHECK(eqf(*ini.getFloat("", "val2"), 36.0f));
		CHECK(eqf(ini.sanitizeFloat("", "val2", 0.0f, 38.0f, 39.0f), 38.0f));
		CHECK(eqf(*ini.getFloat("", "val2"), 38.0f));
	}
	// sanitizeBool()
	{
		const char* fpath = stupidFileName;
		sfz::deleteFile(fpath);
		IniParser ini(fpath, &allocator);

		CHECK(ini.getBool("", "val1") == nullptr);
		CHECK(ini.sanitizeBool("", "val1") == false);
		CHECK(ini.getBool("", "val1") != nullptr);
		CHECK(*ini.getBool("", "val1") == false);

		CHECK(ini.getBool("", "val2") == nullptr);
		CHECK(ini.sanitizeBool("", "val2", true) == true);
		CHECK(ini.getBool("", "val2") != nullptr);
		CHECK(*ini.getBool("", "val2") == true);
	}
}

TEST_CASE("IniParser: comparing_input_and_output")
{
	const char INPUT_INI_1[] =
R"(; This is a comment

[sect1]
          first=       2 ; comment 2
second=     true

       [sect2] ; comment 3
     third      =4.0
fifth    =false
)";

	const char OUTPUT_INI_1[] =
R"(; This is a comment

[sect1]
first=2 ; comment 2
second=true

[sect2] ; comment 3
third=4
fifth=false
)";

	const char INPUT_INI_2[] =
R"(    pi   =  3.0    ;comment
	e	=	3.0  ;'nother comment

; longer comment
; longer comment 2
[section1]
[section2] ; comment sect 2
[section3]
var=true
)";

	const char OUTPUT_INI_2[] =
R"(pi=3 ;comment
e=3 ;'nother comment
; longer comment
; longer comment 2

[section1]
var2=false

[section2] ; comment sect 2

[section3]
var=true
)";

	SfzAllocator allocator = sfz::createStandardAllocator();

	// First ini
	{
		const char* cpath = "test_ini_1.ini";

		deleteFile(cpath);
		CHECK(writeBinaryFile(cpath, reinterpret_cast<const u8*>(INPUT_INI_1), sizeof(INPUT_INI_1)));

		IniParser ini(cpath, &allocator);
		CHECK(ini.load());

		CHECK(ini.getInt("sect1", "first") != nullptr);
		CHECK(*ini.getInt("sect1", "first") == 2);
		CHECK(ini.getFloat("sect1", "first") != nullptr);
		CHECK(eqf(*ini.getFloat("sect1", "first"), 2.0f));
		CHECK(ini.getBool("sect1", "first") == nullptr);
		CHECK(ini.getBool("sect1", "second") != nullptr);
		CHECK(*ini.getBool("sect1", "second") == true);
		CHECK(ini.getInt("sect1", "second") == nullptr);
		CHECK(ini.getFloat("sect1", "second") == nullptr);

		CHECK(ini.getInt("sect2", "third") != nullptr);
		CHECK(*ini.getInt("sect2", "third") == 4);
		CHECK(ini.getFloat("sect2", "third") != nullptr);
		CHECK(eqf(*ini.getFloat("sect2", "third"), 4.0f));
		CHECK(ini.getBool("sect2", "third") == nullptr);
		CHECK(ini.getBool("sect2", "fifth") != nullptr);
		CHECK(*ini.getBool("sect2", "fifth") == false);
		CHECK(ini.getInt("sect2", "fifth") == nullptr);
		CHECK(ini.getFloat("sect2", "fifth") == nullptr);

		int itemCounter = 0;
		for (IniParser::ItemAccessor i : ini) {
			(void)i;
			itemCounter += 1;
		}
		CHECK(itemCounter == 4);

		IniParser::Iterator it = ini.begin();
		CHECK(it != ini.end());
		auto ac = *it;
		CHECK(strcmp(ac.getSection(), "sect1") == 0);
		CHECK(strcmp(ac.getKey(), "first") == 0);
		CHECK(ac.getInt() != nullptr);
		CHECK(*ac.getInt() == 2);
		CHECK(*ac.getFloat() == 2.0f);
		it++;
		ac = *it;
		CHECK(strcmp(ac.getSection(), "sect1") == 0);
		CHECK(strcmp(ac.getKey(), "second") == 0);
		CHECK(ac.getBool() != nullptr);
		CHECK(*ac.getBool() == true);
		it++;
		ac = *it;
		CHECK(strcmp(ac.getSection(), "sect2") == 0);
		CHECK(strcmp(ac.getKey(), "third") == 0);
		CHECK(ac.getInt() != nullptr);
		CHECK(*ac.getInt() == 4);
		CHECK(*ac.getFloat() == 4.0f);
		it++;
		ac = *it;
		CHECK(strcmp(ac.getSection(), "sect2") == 0);
		CHECK(strcmp(ac.getKey(), "fifth") == 0);
		CHECK(ac.getBool() != nullptr);
		CHECK(*ac.getBool() == false);
		it++;
		CHECK(it == ini.end());


		CHECK(ini.save());

		SfzArray<char> output = readTextFile(cpath, &allocator);
		CHECK(strcmp(output.data(), OUTPUT_INI_1) == 0);
		deleteFile(cpath);
	}

	// Second ini
	{
		const char* cpath = "test_ini_2.ini";

		deleteFile(cpath);
		CHECK(writeBinaryFile(cpath, reinterpret_cast<const u8*>(INPUT_INI_2), sizeof(INPUT_INI_2)));

		IniParser ini(cpath, &allocator);
		CHECK(ini.load());

		// Adding var2 = false
		CHECK(ini.getBool("section1", "var2") == nullptr);
		ini.setBool("section1", "var2", false);
		CHECK(ini.getBool("section1", "var2") != nullptr);
		CHECK(*ini.getBool("section1", "var2") == false);

		CHECK(ini.save());

		SfzArray<char> output = readTextFile(cpath, &allocator);
		CHECK(strcmp(output.data(), OUTPUT_INI_2) == 0);
		deleteFile(cpath);
	}
}

#endif
