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

#include "sfz/memory/Allocator.hpp"
#include "sfz/memory/MemoryUtils.hpp"

using namespace sfz;

TEST_CASE("Testing alignment", "[sfz::StandardAllocator]")
{
	void* memory16byte = getDefaultAllocator()->allocate(512, 16);
	REQUIRE(memory16byte != nullptr);
	REQUIRE(isAligned(memory16byte, 16));
	getDefaultAllocator()->deallocate(memory16byte);

	void* memory32byte = getDefaultAllocator()->allocate(512, 32);
	REQUIRE(memory32byte != nullptr);
	REQUIRE(isAligned(memory32byte, 32));
	getDefaultAllocator()->deallocate(memory32byte);
	
	void* memory64byte = getDefaultAllocator()->allocate(512, 64);
	REQUIRE(memory64byte != nullptr);
	REQUIRE(isAligned(memory64byte, 64));
	getDefaultAllocator()->deallocate(memory64byte);
}