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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>

#include <skipifzero.hpp>

#include "ZeroG/Context.hpp"

// About
// ------------------------------------------------------------------------------------------------

// By linking these defintions operator new and delete is overloaded for all code in the ZeroG dll.
// This is mainly used to make third-party libraries (such as SPIRV-Cross) use the user defined
// ZeroG allocator instead of the normal global heap.

// Operator new
// ------------------------------------------------------------------------------------------------

#ifdef _WIN32

void* operator new (std::size_t count)
{
	sfz::Allocator* allocator = zg::getAllocator();
	return allocator->allocate(sfz_dbg("operator new"), count);
}

void* operator new[] (std::size_t count)
{
	sfz::Allocator* allocator = zg::getAllocator();
	return allocator->allocate(sfz_dbg("operator new[]"), count);
}

void* operator new (std::size_t count, std::align_val_t val)
{
	sfz_assert(size_t(val) <= 32);
	sfz::Allocator* allocator = zg::getAllocator();
	return allocator->allocate(sfz_dbg("operator new"), count, uint64_t(val));
}

void* operator new[] (std::size_t count, std::align_val_t val)
{
	sfz_assert(size_t(val) <= 32);
	sfz::Allocator* allocator = zg::getAllocator();
	return allocator->allocate(sfz_dbg("operator new[]"), count, uint64_t(val));
}

void* operator new (std::size_t count, const std::nothrow_t&) noexcept
{
	sfz::Allocator* allocator = zg::getAllocator();
	return allocator->allocate(sfz_dbg("operator new"), count);
}

void* operator new[] (std::size_t count, const std::nothrow_t&) noexcept
{
	sfz::Allocator* allocator = zg::getAllocator();
	return allocator->allocate(sfz_dbg("operator new[]"), count);
}

void* operator new (std::size_t count, std::align_val_t val, const std::nothrow_t&) noexcept
{
	sfz_assert(size_t(val) <= 32);
	sfz::Allocator* allocator = zg::getAllocator();
	return allocator->allocate(sfz_dbg("operator new"), count, uint64_t(val));
}

void* operator new[] (std::size_t count, std::align_val_t val, const std::nothrow_t&) noexcept
{
	sfz_assert(size_t(val) <= 32);
	sfz::Allocator* allocator = zg::getAllocator();
	return allocator->allocate(sfz_dbg("operator new[]"), count, uint64_t(val));
}

// Operator delete
// ------------------------------------------------------------------------------------------------

void operator delete (void* ptr) noexcept
{
	AllocatorWrapper& allocator = zg::getContext().allocator;
	if (allocator.isInitialized()) {
		allocator.deallocate(ptr);
	}
	else {
#ifndef NDEBUG
		printf("ZeroG: No allocator set, attempting to deallocate: %llx. Expected if process is terminating.\n",
			uint64_t(ptr));
#endif
	}
}

void operator delete[] (void* ptr) noexcept
{
	AllocatorWrapper& allocator = zg::getContext().allocator;
	if (allocator.isInitialized()) {
		allocator.deallocate(ptr);
	}
	else {
#ifndef NDEBUG
		printf("ZeroG: No allocator set, attempting to deallocate: %llx. Expected if process is terminating.\n",
			uint64_t(ptr));
#endif
	}
}

void operator delete (void* ptr, std::align_val_t val) noexcept
{
	sfz_assert(size_t(val) <= 32);
	AllocatorWrapper& allocator = zg::getContext().allocator;
	if (allocator.isInitialized()) {
		allocator.deallocate(ptr);
	}
	else {
#ifndef NDEBUG
		printf("ZeroG: No allocator set, attempting to deallocate: %llx. Expected if process is terminating.\n",
			uint64_t(ptr));
#endif
	}
}

void operator delete[] (void* ptr, std::align_val_t val) noexcept
{
	sfz_assert(size_t(val) <= 32);
	AllocatorWrapper& allocator = zg::getContext().allocator;
	if (allocator.isInitialized()) {
		allocator.deallocate(ptr);
	}
	else {
#ifndef NDEBUG
		printf("ZeroG: No allocator set, attempting to deallocate: %llx. Expected if process is terminating.\n",
			uint64_t(ptr));
#endif
	}
}

void operator delete (void* ptr, std::size_t sz) noexcept
{
	(void)sz;
	AllocatorWrapper& allocator = zg::getContext().allocator;
	if (allocator.isInitialized()) {
		allocator.deallocate(ptr);
	}
	else {
#ifndef NDEBUG
		printf("ZeroG: No allocator set, attempting to deallocate: %llx. Expected if process is terminating.\n",
			uint64_t(ptr));
#endif
	}
}

void operator delete[] (void* ptr, std::size_t sz) noexcept
{
	(void)sz;
	AllocatorWrapper& allocator = zg::getContext().allocator;
	if (allocator.isInitialized()) {
		allocator.deallocate(ptr);
	}
	else {
#ifndef NDEBUG
		printf("ZeroG: No allocator set, attempting to deallocate: %llx. Expected if process is terminating.\n",
			uint64_t(ptr));
#endif
	}
}

void operator delete (void* ptr, std::size_t sz, std::align_val_t val) noexcept
{
	sfz_assert(size_t(val) <= 32);
	(void)sz;
	AllocatorWrapper& allocator = zg::getContext().allocator;
	if (allocator.isInitialized()) {
		allocator.deallocate(ptr);
	}
	else {
#ifndef NDEBUG
		printf("ZeroG: No allocator set, attempting to deallocate: %llx. Expected if process is terminating.\n",
			uint64_t(ptr));
#endif
	}
}

void operator delete[] (void* ptr, std::size_t sz, std::align_val_t val) noexcept
{
	sfz_assert(size_t(val) <= 32);
	(void)sz;
	AllocatorWrapper& allocator = zg::getContext().allocator;
	if (allocator.isInitialized()) {
		allocator.deallocate(ptr);
	}
	else {
#ifndef NDEBUG
		printf("ZeroG: No allocator set, attempting to deallocate: %llx. Expected if process is terminating.\n",
			uint64_t(ptr));
#endif
	}
}

#endif
