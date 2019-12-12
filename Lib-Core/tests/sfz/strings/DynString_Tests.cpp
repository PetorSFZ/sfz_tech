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
#include "sfz/strings/DynString.hpp"

using namespace sfz;

UTEST(DynString, constructor)
{
	DynString str1("Hello World");
	ASSERT_TRUE(std::strcmp(str1.str(), "Hello World") == 0);
	ASSERT_TRUE(str1.size() == 11);
	ASSERT_TRUE(str1.capacity() == 12);

	DynString str2(nullptr);
	ASSERT_TRUE(str2.str() == nullptr);
	ASSERT_TRUE(str2.size() == 0);
	ASSERT_TRUE(str2.capacity() == 0);;

	DynString str3(nullptr, 16);
	ASSERT_TRUE(str3.str() != nullptr);
	ASSERT_TRUE(str3.size() == 0);
	ASSERT_TRUE(str3.capacity() == 16);

	DynString str4("4th", 8);
	ASSERT_TRUE(std::strcmp(str4.str(), "4th") == 0);
	ASSERT_TRUE(str4.size() == 3);
	ASSERT_TRUE(str4.capacity() == 8);
}

UTEST(DynString, printf_printf_append)
{
	DynString str(nullptr, 128);
	str.printf("%s: %i", "Test", 1);
	ASSERT_TRUE(strcmp(str.str(), "Test: 1") == 0);
	ASSERT_TRUE(str.size() == std::strlen("Test: 1"));

	str.printfAppend(" && %s: %i", "Test", 2);
	ASSERT_TRUE(strcmp(str.str(), "Test: 1 && Test: 2") == 0);
	ASSERT_TRUE(str.size() == std::strlen("Test: 1 && Test: 2"));

	str.printfAppend(" && %s: %i", "Test", 3);
	ASSERT_TRUE(strcmp(str.str(), "Test: 1 && Test: 2 && Test: 3") == 0);
	ASSERT_TRUE(str.size() == std::strlen("Test: 1 && Test: 2 && Test: 3"));

	str.printf("%s", "--");
	ASSERT_TRUE(strcmp(str.str(), "--") == 0);
	ASSERT_TRUE(str.size() == std::strlen("--"));
}

UTEST(DynString, comparison_operators)
{
	DynString str("aba");
	ASSERT_TRUE(str == "aba");
	ASSERT_TRUE(str != "afae");
	ASSERT_TRUE(str < "bbb");
	ASSERT_TRUE(str > "aaa");
}
