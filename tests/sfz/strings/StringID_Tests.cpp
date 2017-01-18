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

#include <cstring>

#include "sfz/strings/StringID.hpp"

using namespace sfz;

TEST_CASE("Testing StringCollection", "[sfz::StringID]")
{
	StringCollection collection(32, getDefaultAllocator());
	REQUIRE(collection.numStringsHeld() == 0);
	
	StringID id1 = collection.getStringID("Hello");
	REQUIRE(collection.numStringsHeld() == 1);
	StringID id2 = collection.getStringID("World");
	REQUIRE(collection.numStringsHeld() == 2);
	
	REQUIRE(id1 == id1);
	REQUIRE(id2 == id2);
	REQUIRE(id1 != id2);

	REQUIRE(collection.getString(id1) != nullptr);
	REQUIRE(std::strcmp("Hello", collection.getString(id1)) == 0);
	REQUIRE(std::strcmp("World", collection.getString(id2)) == 0);

	StringID badId;
	badId.id = id1.id + id2.id;
	REQUIRE(collection.getString(badId) == nullptr);
	REQUIRE(collection.numStringsHeld() == 2);
}
