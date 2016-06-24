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

#include "sfz/math/MathHelpers.hpp"
#include "sfz/util/IniParser.hpp"
#include "sfz/util/IO.hpp"

using namespace sfz;

static const char* stupidFileName = "fafeafeafeafaefa.ini";

static DynString appendBasePath(const char* fileName) noexcept
{
	size_t baseLen = std::strlen(sfz::basePath());
	size_t fileLen = std::strlen(fileName);
	sfz::DynString tmp("", static_cast<uint32_t>(baseLen + fileLen + 10));
	tmp.printf("%s", sfz::basePath());
	tmp.printfAppend("%s", fileName);
	return std::move(tmp);
}

TEST_CASE("Basic IniParser tests", "[sfz::IniParser]")
{
	auto filePath = appendBasePath(stupidFileName);
	const char* fpath = filePath.str();
	
	IniParser ini1(fpath);

	ini1.setBool("Section1", "bBool1", true);
	ini1.setBool("Section1", "bBool2", false);
	ini1.setFloat("Section2", "fFloat1", 3.5f);
	ini1.setInt("Section2", "iInt1", -23);

	REQUIRE(*ini1.getBool("Section1", "bBool1") == true);
	REQUIRE(*ini1.getBool("Section1", "bBool2") == false);
	REQUIRE(*ini1.getFloat("Section2", "fFloat1") == 3.5f);
	REQUIRE(*ini1.getInt("Section2", "iInt1") == -23);

	deleteFile(fpath);
	REQUIRE(ini1.save());

	IniParser ini2(fpath);
	REQUIRE(ini2.load());

	REQUIRE(*ini2.getBool("Section1", "bBool1"));
	REQUIRE(!*ini2.getBool("Section1", "bBool2"));
	REQUIRE(*ini2.getFloat("Section2", "fFloat1") == 3.5f);
	REQUIRE(*ini2.getInt("Section2", "iInt1") == -23);

	ini1.setBool("Section1", "bBool1", true);
	ini1.setBool("Section1", "bBool2", false);
	ini1.setFloat("Section2", "fFloat1", 3.5f);
	ini1.setInt("Section2", "iInt1", -23);
	
	deleteFile(fpath);
}

TEST_CASE("IniParser comparing input and output", "[sfz::IniParser]")
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

	SECTION("First ini") {
		auto path = appendBasePath("test_ini_1.ini");
		const char* cpath = path.str();

		deleteFile(cpath);
		REQUIRE(writeBinaryFile(cpath, reinterpret_cast<const uint8_t*>(INPUT_INI_1), sizeof(INPUT_INI_1)));
		
		IniParser ini(cpath);
		REQUIRE(ini.load());

		REQUIRE(ini.getInt("sect1", "first") != nullptr);
		REQUIRE(*ini.getInt("sect1", "first") == 2);
		REQUIRE(ini.getFloat("sect1", "first") != nullptr);
		REQUIRE(approxEqual(*ini.getFloat("sect1", "first"), 2.0f));
		REQUIRE(ini.getBool("sect1", "first") == nullptr);
		REQUIRE(ini.getBool("sect1", "second") != nullptr);
		REQUIRE(*ini.getBool("sect1", "second") == true);
		REQUIRE(ini.getInt("sect1", "second") == nullptr);
		REQUIRE(ini.getFloat("sect1", "second") == nullptr);

		REQUIRE(ini.getInt("sect2", "third") != nullptr);
		REQUIRE(*ini.getInt("sect2", "third") == 4);
		REQUIRE(ini.getFloat("sect2", "third") != nullptr);
		REQUIRE(approxEqual(*ini.getFloat("sect2", "third"), 4.0f));
		REQUIRE(ini.getBool("sect2", "third") == nullptr);
		REQUIRE(ini.getBool("sect2", "fifth") != nullptr);
		REQUIRE(*ini.getBool("sect2", "fifth") == false);
		REQUIRE(ini.getInt("sect2", "fifth") == nullptr);
		REQUIRE(ini.getFloat("sect2", "fifth") == nullptr);

		REQUIRE(ini.save());

		DynString output = readTextFile(cpath);
		REQUIRE(output == OUTPUT_INI_1);
		deleteFile(cpath);
	}

	SECTION("Second ini") {
		auto path = appendBasePath("test_ini_2.ini");
		const char* cpath = path.str();

		deleteFile(cpath);
		REQUIRE(writeBinaryFile(cpath, reinterpret_cast<const uint8_t*>(INPUT_INI_2), sizeof(INPUT_INI_2)));

		IniParser ini(cpath);
		REQUIRE(ini.load());

		// Adding var2 = false
		REQUIRE(ini.getBool("section1", "var2") == nullptr);
		ini.setBool("section1", "var2", false);
		REQUIRE(ini.getBool("section1", "var2") != nullptr);
		REQUIRE(*ini.getBool("section1", "var2") == false);

		REQUIRE(ini.save());

		DynString output = readTextFile(cpath);
		REQUIRE(output == OUTPUT_INI_2);
		deleteFile(cpath);
	}
}
