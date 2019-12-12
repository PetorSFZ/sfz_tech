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

#include <skipifzero.hpp>
#include <skipifzero_allocators.hpp>

#include "sfz/Context.hpp"

using namespace sfz;

UTEST(StandardAllocator, testing_alignment)
{
	sfz::setContext(sfz::getStandardContext());

	void* memory16byte = sfz::getDefaultAllocator()->allocate(sfz_dbg(""), 512, 16);
	ASSERT_TRUE(memory16byte != nullptr);
	ASSERT_TRUE(isAligned(memory16byte, 16));
	sfz::getDefaultAllocator()->deallocate(memory16byte);

	void* memory32byte = sfz::getDefaultAllocator()->allocate(sfz_dbg(""), 512, 32);
	ASSERT_TRUE(memory32byte != nullptr);
	ASSERT_TRUE(isAligned(memory32byte, 32));
	sfz::getDefaultAllocator()->deallocate(memory32byte);

	void* memory64byte = sfz::getDefaultAllocator()->allocate(sfz_dbg(""), 512, 64);
	ASSERT_TRUE(memory64byte != nullptr);
	ASSERT_TRUE(isAligned(memory64byte, 64));
	sfz::getDefaultAllocator()->deallocate(memory64byte);
}

UTEST(StandardAllocator, basic_new_and_delete_tests)
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
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(ptr->flagPtr == &flag);
	ASSERT_TRUE(flag == 1);

	getDefaultAllocator()->deleteObject(ptr);
	ASSERT_TRUE(flag == 2);
}
