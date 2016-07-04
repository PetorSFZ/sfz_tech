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

#include "sfz/memory/New.hpp"
#include "sfz/memory/SmartPointers.hpp"

using namespace sfz;

// UniquePtr tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("Basic UniquePtr tests", "[sfz::UniquePtr]")
{
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

	UniquePtr<TestClass> ptr = nullptr;
	REQUIRE(ptr == nullptr);
	ptr = UniquePtr<TestClass>{sfz_new<TestClass>(&flag)};
	REQUIRE(ptr.get() != nullptr);
	REQUIRE(ptr != nullptr);
	REQUIRE(ptr.get()->flagPtr == &flag);
	REQUIRE((*ptr).flagPtr == &flag);
	REQUIRE(ptr->flagPtr == &flag);
	REQUIRE(flag == 1);

	UniquePtr<TestClass> second;
	REQUIRE(second == nullptr);
	second = std::move(ptr);
	REQUIRE(ptr == nullptr);
	REQUIRE(second != nullptr);

	ptr.destroy();
	REQUIRE(flag == 1);
	REQUIRE(ptr == nullptr);
	
	second.destroy();
	REQUIRE(flag == 2);
	REQUIRE(second == nullptr);
}

TEST_CASE("makeUnique()", "[sfz::SmartPointers]")
{
	struct Foo {
		int a, b;
		Foo(int a, int b) : a(a), b(b) {} 
	};
	auto ptr = makeUnique<Foo>(3, 4);
	REQUIRE(ptr->a == 3);
	REQUIRE(ptr->b == 4);
}

// SharedPtr tests
// ------------------------------------------------------------------------------------------------

TEST_CASE("Basic SharedPtr tests", "[sfz::SharedPtr]")
{
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

		TestClass(const TestClass&) = delete;
		TestClass& operator= (const TestClass&) = delete;
		TestClass(TestClass&&) = delete;
		TestClass& operator= (TestClass&&) = delete;
	};

	REQUIRE(flag == 0);
	TestClass* item = sfz_new<TestClass>(&flag);
	REQUIRE(flag == 1);

	{
		SharedPtr<TestClass> ptr = nullptr;
		REQUIRE(ptr == nullptr);
		REQUIRE(ptr.refCount() == 0);
		ptr = SharedPtr<TestClass>(item);
		REQUIRE(ptr != nullptr);
		REQUIRE(ptr.refCount() == 1);
	}
	REQUIRE(flag == 2);

	flag = 0;
	{
		REQUIRE(flag == 0);
		SharedPtr<TestClass> ptr(sfz_new<TestClass>(&flag));
		REQUIRE(ptr != nullptr);
		REQUIRE(ptr.refCount() == 1);
		REQUIRE(flag == 1);
		{
			SharedPtr<TestClass> second = ptr;
			REQUIRE(flag == 1);
			REQUIRE(ptr.get() == second.get());
			REQUIRE(ptr == second);
			REQUIRE(ptr.refCount() == 2);
			REQUIRE(ptr.refCount() == second.refCount());
		}
		REQUIRE(flag == 1);
		REQUIRE(ptr.refCount() == 1);
	}
	REQUIRE(flag == 2);
}

TEST_CASE("makeShared()", "[sfz::SmartPointers]")
{
	struct Foo {
		int a, b;
		Foo(int a, int b) : a(a), b(b) {}
	};
	auto ptr = makeShared<Foo>(3, 4);
	REQUIRE(ptr->a == 3);
	REQUIRE(ptr->b == 4);
}
