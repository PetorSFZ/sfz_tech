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

#include "sfz/containers/HashMap.hpp"
#include "sfz/strings/StringHashers.hpp"

using namespace sfz;

TEST_CASE("fnv1aHash()", "[sfz::StringHashers]")
{
	// Assumes sfz::hash() is a wrapper around fnv1aHash()

	// Test values taken from public domain reference code by "chongo <Landon Curt Noll> /\oo/\"
	// See http://isthe.com/chongo/tech/comp/fnv/
	REQUIRE(sfz::hash("") == uint64_t(0xcbf29ce484222325));
	REQUIRE(sfz::hash("a") == uint64_t(0xaf63dc4c8601ec8c));
	REQUIRE(sfz::hash("b") == uint64_t(0xaf63df4c8601f1a5));
	REQUIRE(sfz::hash("c") == uint64_t(0xaf63de4c8601eff2));
	REQUIRE(sfz::hash("foo") == uint64_t(0xdcb27518fed9d577));
	REQUIRE(sfz::hash("foobar") == uint64_t(0x85944171f73967e8));
	REQUIRE(sfz::hash("chongo was here!\n") == uint64_t(0x46810940eff5f915));
}

TEST_CASE("Hash structs")
{
	sfz::RawStringHash cStrHasher;
	std::hash<DynString> dynStrHasher;
	std::hash<StackString> stackStrHasher;
	
	SECTION("Empty strings") {
		REQUIRE(cStrHasher("") == dynStrHasher(DynString()));
		REQUIRE(cStrHasher("") == dynStrHasher(DynString("")));
		DynString dynTmp("Herro");
		dynTmp.clear();
		REQUIRE(cStrHasher("") == dynStrHasher(dynTmp));
		dynTmp.destroy();
		REQUIRE(cStrHasher("") == dynStrHasher(dynTmp));

		REQUIRE(cStrHasher("") == stackStrHasher(StackString()));
		REQUIRE(cStrHasher("") == stackStrHasher(StackString("")));
	}
	SECTION("Longer strings") {
		REQUIRE(cStrHasher("foobar") == dynStrHasher(DynString("foobar")));
		REQUIRE(cStrHasher("foobar") != dynStrHasher(DynString("fooba")));
		REQUIRE(cStrHasher("foobar") != dynStrHasher(DynString("foobar\n")));

		REQUIRE(cStrHasher("foobar") == stackStrHasher(StackString("foobar")));
		REQUIRE(cStrHasher("foobar") != stackStrHasher(StackString("fooba")));
		REQUIRE(cStrHasher("foobar") != stackStrHasher(StackString("foobar\n")));
	}
}
