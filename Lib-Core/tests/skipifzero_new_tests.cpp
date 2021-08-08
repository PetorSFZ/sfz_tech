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

#include "skipifzero_allocators.hpp"
#include "skipifzero_new.hpp"

class Base {
public:
	int val = 0;
	Base() = default;
	Base(const Base&) = delete;
	Base& operator= (const Base&) = delete;
	Base(Base&&) = delete;
	Base& operator= (Base&&) = delete;
};

class Derived : public Base {
public:
	Derived(int valIn) {
		val = valIn;
	}
	Derived(const Derived&) = delete;
	Derived& operator= (const Derived&) = delete;
	Derived(Derived&&) = delete;
	Derived& operator= (Derived&&) = delete;
};

// UniquePtr tests
// ------------------------------------------------------------------------------------------------

UTEST(UniquePtr, basic_tests)
{
	SfzAllocator allocator = sfz::createStandardAllocator();
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

	sfz::UniquePtr<TestClass> ptr = nullptr;
	ASSERT_TRUE(ptr == nullptr);
	ptr = sfz::UniquePtr<TestClass>(sfz_new<TestClass>(&allocator, sfz_dbg(""), &flag), &allocator);
		
	ASSERT_TRUE(ptr.get() != nullptr);
	ASSERT_TRUE(ptr != nullptr);
	ASSERT_TRUE(ptr.get()->flagPtr == &flag);
	ASSERT_TRUE((*ptr).flagPtr == &flag);
	ASSERT_TRUE(ptr->flagPtr == &flag);
	ASSERT_TRUE(flag == 1);

	sfz::UniquePtr<TestClass> second;
	ASSERT_TRUE(second == nullptr);
	second = std::move(ptr);
	ASSERT_TRUE(ptr == nullptr);
	ASSERT_TRUE(second != nullptr);

	ptr.destroy();
	ASSERT_TRUE(flag == 1);
	ASSERT_TRUE(ptr == nullptr);

	second.destroy();
	ASSERT_TRUE(flag == 2);
	ASSERT_TRUE(second == nullptr);
}

UTEST(UniquePtr, make_unique)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	struct Foo {
		int a, b;
		Foo(int a, int b) : a(a), b(b) {}
	};
	auto ptr = sfz::makeUnique<Foo>(&allocator, sfz_dbg(""), 3, 4);
	ASSERT_TRUE(ptr->a == 3);
	ASSERT_TRUE(ptr->b == 4);
}

UTEST(UniquePtr, cast_take)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::UniquePtr<Derived> derived = sfz::makeUnique<Derived>(&allocator, sfz_dbg(""), 3);
	ASSERT_TRUE(derived->val == 3);
	sfz::UniquePtr<Base> base = derived.castTake<Base>();
	ASSERT_TRUE(derived.get() == nullptr);
	ASSERT_TRUE(derived.allocator() == nullptr);
	ASSERT_TRUE(base->val == 3);
	ASSERT_TRUE(base.allocator() == &allocator);
}

UTEST(UniquePtr, cast_constructor)
{
	SfzAllocator allocator = sfz::createStandardAllocator();

	sfz::UniquePtr<Base> ptr = sfz::makeUnique<Derived>(&allocator, sfz_dbg(""), 3);
	ASSERT_TRUE(ptr->val == 3);
}
