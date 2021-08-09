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

#include <skipifzero_math.hpp>

#include "sfz/util/IniParser.hpp"
#include "sfz/util/IO.hpp"

// TODO: Fix test cases for iOS
#ifndef SFZ_IOS

using namespace sfz;

static const char* stupidFileName = "fafeafeafeafaefa.ini";

UTEST(IniParser, basic_tests)
{
	const char* fpath = stupidFileName;

	IniParser ini1(fpath);

	ini1.setBool("Section1", "bBool1", true);
	ini1.setBool("Section1", "bBool2", false);
	ini1.setFloat("Section2", "fFloat1", 3.5f);
	ini1.setInt("Section2", "iInt1", -23);

	ASSERT_TRUE(*ini1.getBool("Section1", "bBool1") == true);
	ASSERT_TRUE(*ini1.getBool("Section1", "bBool2") == false);
	ASSERT_TRUE(*ini1.getFloat("Section2", "fFloat1") == 3.5f);
	ASSERT_TRUE(*ini1.getInt("Section2", "iInt1") == -23);

	sfz::deleteFile(fpath);
	ASSERT_TRUE(ini1.save());

	IniParser ini2(fpath);
	ASSERT_TRUE(ini2.load());

	ASSERT_TRUE(*ini2.getBool("Section1", "bBool1"));
	ASSERT_TRUE(!*ini2.getBool("Section1", "bBool2"));
	ASSERT_TRUE(*ini2.getFloat("Section2", "fFloat1") == 3.5f);
	ASSERT_TRUE(*ini2.getInt("Section2", "iInt1") == -23);

	ini1.setBool("Section1", "bBool1", true);
	ini1.setBool("Section1", "bBool2", false);
	ini1.setFloat("Section2", "fFloat1", 3.5f);
	ini1.setInt("Section2", "iInt1", -23);

	sfz::deleteFile(fpath);
}

UTEST(IniParser, sanitizer_methods)
{
	// sanitizeInt()
	{
		const char* fpath = stupidFileName;
		sfz::deleteFile(fpath);
		IniParser ini(fpath);

		ASSERT_TRUE(ini.getInt("", "val1") == nullptr);
		ASSERT_TRUE(ini.sanitizeInt("", "val1") == 0);
		ASSERT_TRUE(ini.getInt("", "val1") != nullptr);
		ASSERT_TRUE(*ini.getInt("", "val1") == 0);

		ASSERT_TRUE(ini.getInt("", "val2") == nullptr);
		ASSERT_TRUE(ini.sanitizeInt("", "val2", 37) == 37);
		ASSERT_TRUE(ini.getInt("", "val2") != nullptr);
		ASSERT_TRUE(*ini.getInt("", "val2") == 37);

		ASSERT_TRUE(ini.sanitizeInt("", "val2", 0, 0, 36) == 36);
		ASSERT_TRUE(*ini.getInt("", "val2") == 36);
		ASSERT_TRUE(ini.sanitizeInt("", "val2", 0, 38, 39) == 38);
		ASSERT_TRUE(*ini.getInt("", "val2") == 38);
	}
	// sanitizeFloat()
	{
		const char* fpath = stupidFileName;
		sfz::deleteFile(fpath);
		IniParser ini(fpath);

		ASSERT_TRUE(ini.getFloat("", "val1") == nullptr);
		ASSERT_TRUE(eqf(ini.sanitizeFloat("", "val1"), 0.0f));
		ASSERT_TRUE(ini.getFloat("", "val1") != nullptr);
		ASSERT_TRUE(eqf(*ini.getFloat("", "val1"), 0.0f));

		ASSERT_TRUE(ini.getFloat("", "val2") == nullptr);
		ASSERT_TRUE(eqf(ini.sanitizeFloat("", "val2", 37.0f), 37.0f));
		ASSERT_TRUE(ini.getFloat("", "val2") != nullptr);
		ASSERT_TRUE(eqf(*ini.getFloat("", "val2"), 37.0f));

		ASSERT_TRUE(eqf(ini.sanitizeFloat("", "val2", 0.0f, 0.0f, 36.0f), 36.0f));
		ASSERT_TRUE(eqf(*ini.getFloat("", "val2"), 36.0f));
		ASSERT_TRUE(eqf(ini.sanitizeFloat("", "val2", 0.0f, 38.0f, 39.0f), 38.0f));
		ASSERT_TRUE(eqf(*ini.getFloat("", "val2"), 38.0f));
	}
	// sanitizeBool()
	{
		const char* fpath = stupidFileName;
		sfz::deleteFile(fpath);
		IniParser ini(fpath);

		ASSERT_TRUE(ini.getBool("", "val1") == nullptr);
		ASSERT_TRUE(ini.sanitizeBool("", "val1") == false);
		ASSERT_TRUE(ini.getBool("", "val1") != nullptr);
		ASSERT_TRUE(*ini.getBool("", "val1") == false);

		ASSERT_TRUE(ini.getBool("", "val2") == nullptr);
		ASSERT_TRUE(ini.sanitizeBool("", "val2", true) == true);
		ASSERT_TRUE(ini.getBool("", "val2") != nullptr);
		ASSERT_TRUE(*ini.getBool("", "val2") == true);
	}
}

UTEST(IniParser, comparing_input_and_output)
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

	// First ini
	{
		const char* cpath = "test_ini_1.ini";

		deleteFile(cpath);
		ASSERT_TRUE(writeBinaryFile(cpath, reinterpret_cast<const uint8_t*>(INPUT_INI_1), sizeof(INPUT_INI_1)));

		IniParser ini(cpath);
		ASSERT_TRUE(ini.load());

		ASSERT_TRUE(ini.getInt("sect1", "first") != nullptr);
		ASSERT_TRUE(*ini.getInt("sect1", "first") == 2);
		ASSERT_TRUE(ini.getFloat("sect1", "first") != nullptr);
		ASSERT_TRUE(eqf(*ini.getFloat("sect1", "first"), 2.0f));
		ASSERT_TRUE(ini.getBool("sect1", "first") == nullptr);
		ASSERT_TRUE(ini.getBool("sect1", "second") != nullptr);
		ASSERT_TRUE(*ini.getBool("sect1", "second") == true);
		ASSERT_TRUE(ini.getInt("sect1", "second") == nullptr);
		ASSERT_TRUE(ini.getFloat("sect1", "second") == nullptr);

		ASSERT_TRUE(ini.getInt("sect2", "third") != nullptr);
		ASSERT_TRUE(*ini.getInt("sect2", "third") == 4);
		ASSERT_TRUE(ini.getFloat("sect2", "third") != nullptr);
		ASSERT_TRUE(eqf(*ini.getFloat("sect2", "third"), 4.0f));
		ASSERT_TRUE(ini.getBool("sect2", "third") == nullptr);
		ASSERT_TRUE(ini.getBool("sect2", "fifth") != nullptr);
		ASSERT_TRUE(*ini.getBool("sect2", "fifth") == false);
		ASSERT_TRUE(ini.getInt("sect2", "fifth") == nullptr);
		ASSERT_TRUE(ini.getFloat("sect2", "fifth") == nullptr);

		int itemCounter = 0;
		for (IniParser::ItemAccessor i : ini) {
			(void)i;
			itemCounter += 1;
		}
		ASSERT_TRUE(itemCounter == 4);

		IniParser::Iterator it = ini.begin();
		ASSERT_TRUE(it != ini.end());
		auto ac = *it;
		ASSERT_TRUE(strcmp(ac.getSection(), "sect1") == 0);
		ASSERT_TRUE(strcmp(ac.getKey(), "first") == 0);
		ASSERT_TRUE(ac.getInt() != nullptr);
		ASSERT_TRUE(*ac.getInt() == 2);
		ASSERT_TRUE(*ac.getFloat() == 2.0f);
		it++;
		ac = *it;
		ASSERT_TRUE(strcmp(ac.getSection(), "sect1") == 0);
		ASSERT_TRUE(strcmp(ac.getKey(), "second") == 0);
		ASSERT_TRUE(ac.getBool() != nullptr);
		ASSERT_TRUE(*ac.getBool() == true);
		it++;
		ac = *it;
		ASSERT_TRUE(strcmp(ac.getSection(), "sect2") == 0);
		ASSERT_TRUE(strcmp(ac.getKey(), "third") == 0);
		ASSERT_TRUE(ac.getInt() != nullptr);
		ASSERT_TRUE(*ac.getInt() == 4);
		ASSERT_TRUE(*ac.getFloat() == 4.0f);
		it++;
		ac = *it;
		ASSERT_TRUE(strcmp(ac.getSection(), "sect2") == 0);
		ASSERT_TRUE(strcmp(ac.getKey(), "fifth") == 0);
		ASSERT_TRUE(ac.getBool() != nullptr);
		ASSERT_TRUE(*ac.getBool() == false);
		it++;
		ASSERT_TRUE(it == ini.end());


		ASSERT_TRUE(ini.save());

		DynString output = readTextFile(cpath);
		ASSERT_TRUE(output == OUTPUT_INI_1);
		deleteFile(cpath);
	}

	// Second ini
	{
		const char* cpath = "test_ini_2.ini";

		deleteFile(cpath);
		ASSERT_TRUE(writeBinaryFile(cpath, reinterpret_cast<const uint8_t*>(INPUT_INI_2), sizeof(INPUT_INI_2)));

		IniParser ini(cpath);
		ASSERT_TRUE(ini.load());

		// Adding var2 = false
		ASSERT_TRUE(ini.getBool("section1", "var2") == nullptr);
		ini.setBool("section1", "var2", false);
		ASSERT_TRUE(ini.getBool("section1", "var2") != nullptr);
		ASSERT_TRUE(*ini.getBool("section1", "var2") == false);

		ASSERT_TRUE(ini.save());

		DynString output = readTextFile(cpath);
		ASSERT_TRUE(output == OUTPUT_INI_2);
		deleteFile(cpath);
	}
}

#endif
