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
#include "catch2/catch.hpp"
#include "sfz/PopWarnings.hpp"

#include "sfz/Context.hpp"
#include "sfz/memory/Allocator.hpp"
#include "sfz/memory/MemoryUtils.hpp"

using namespace sfz;

TEST_CASE("Testing alignment", "[sfz::StandardAllocator]")
{
	sfz::setContext(sfz::getStandardContext());

	void* memory16byte = getDefaultAllocator()->allocate(sfz_dbg(""), 512, 16);
	REQUIRE(memory16byte != nullptr);
	REQUIRE(isAligned(memory16byte, 16));
	getDefaultAllocator()->deallocate(memory16byte);

	void* memory32byte = getDefaultAllocator()->allocate(sfz_dbg(""), 512, 32);
	REQUIRE(memory32byte != nullptr);
	REQUIRE(isAligned(memory32byte, 32));
	getDefaultAllocator()->deallocate(memory32byte);

	void* memory64byte = getDefaultAllocator()->allocate(sfz_dbg(""), 512, 64);
	REQUIRE(memory64byte != nullptr);
	REQUIRE(isAligned(memory64byte, 64));
	getDefaultAllocator()->deallocate(memory64byte);
}

TEST_CASE("Basic new and delete tests", "[sfz::StandardAllocator]")
{
	sfz::setContext(sfz::getStandardContext());

	int flag = 0;

	struct TestClass {
		int* flagPtr;
		TestClass(int* ptr)
		{
			flagPtr = ptr;
			*flagPtr = 1;
		}
		~TestClass()
		{
			*flagPtr = 2;
		}
	};

	TestClass* ptr = nullptr;
	ptr = getDefaultAllocator()->newObject<TestClass>(sfz_dbg("name"), &flag);
	REQUIRE(ptr != nullptr);
	REQUIRE(ptr->flagPtr == &flag);
	REQUIRE(flag == 1);

	getDefaultAllocator()->deleteObject(ptr);
	REQUIRE(flag == 2);
}
