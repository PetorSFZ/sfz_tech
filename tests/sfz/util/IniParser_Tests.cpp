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

#include "sfz/util/IniParser.hpp"
#include "sfz/util/IO.hpp"


#include <iostream>


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

	REQUIRE(ini1.getBool("Section1", "bBool1") == true);
	REQUIRE(ini1.getBool("Section1", "bBool2") == false);
	REQUIRE(ini1.getFloat("Section2", "fFloat1") == 3.5f);
	REQUIRE(ini1.getInt("Section2", "iInt1") == -23);

	deleteFile(fpath);
	REQUIRE(ini1.save());

	IniParser ini2(fpath);
	REQUIRE(ini2.load());

	REQUIRE(ini2.getBool("Section1", "bBool1"));
	REQUIRE(!ini2.getBool("Section1", "bBool2"));
	REQUIRE(ini2.getFloat("Section2", "fFloat1") == 3.5f);
	REQUIRE(ini2.getInt("Section2", "iInt1") == -23);

	ini1.setBool("Section1", "bBool1", true);
	ini1.setBool("Section1", "bBool2", false);
	ini1.setFloat("Section2", "fFloat1", 3.5f);
	ini1.setInt("Section2", "iInt1", -23);
	
	deleteFile(fpath);
}
