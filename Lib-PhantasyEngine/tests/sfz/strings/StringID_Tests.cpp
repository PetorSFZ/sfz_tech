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

#include <cstring>

#include "sfz/Context.hpp"
#include "sfz/strings/StringID.hpp"

using namespace sfz;

UTEST(StringID, testing_string_collection)
{
	StringCollection collection(32, getDefaultAllocator());
	ASSERT_TRUE(collection.numStringsHeld() == 0);

	strID id1 = collection.getStringID("Hello");
	ASSERT_TRUE(collection.numStringsHeld() == 1);
	strID id2 = collection.getStringID("World");
	ASSERT_TRUE(collection.numStringsHeld() == 2);

	ASSERT_TRUE(id1 == id1);
	ASSERT_TRUE(id2 == id2);
	ASSERT_TRUE(id1 != id2);

	ASSERT_TRUE(collection.getString(id1) != nullptr);
	ASSERT_TRUE(std::strcmp("Hello", collection.getString(id1)) == 0);
	ASSERT_TRUE(std::strcmp("World", collection.getString(id2)) == 0);

	strID badId;
	badId.id = id1.id + id2.id;
	ASSERT_TRUE(collection.getString(badId) == nullptr);
	ASSERT_TRUE(collection.numStringsHeld() == 2);
}

UTEST(StringID, ensuring_we_always_get_same_has_for_same_string)
{
	StringCollection collection(32, getDefaultAllocator());
	ASSERT_TRUE(collection.numStringsHeld() == 0);

	strID helloWorldId = collection.getStringID("Hello World!");
	constexpr uint64_t HELLO_WORLD_HASH = 10092224619179044402ull;
	ASSERT_TRUE(helloWorldId.id == HELLO_WORLD_HASH);

	// Ensure we get same string id both times
	strID helloWorldId2 = collection.getStringID("Hello World!");
	ASSERT_TRUE(helloWorldId == helloWorldId2);
	ASSERT_TRUE(collection.numStringsHeld() == 1);
}
