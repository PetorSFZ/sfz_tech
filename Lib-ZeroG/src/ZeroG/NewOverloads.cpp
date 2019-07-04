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

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "ZeroG/Context.hpp"

// About
// ------------------------------------------------------------------------------------------------

// By linking these defintions operator new and delete is overloaded for all code in the ZeroG dll.
// This is mainly used to make third-party libraries (such as SPIRV-Cross) use the user defined
// ZeroG allocator instead of the normal global heap.

// Operator new
// ------------------------------------------------------------------------------------------------

void* operator new (std::size_t count)
{
	ZgAllocator& allocator = zg::getContext().allocator;
	return allocator.allocate(allocator.userPtr, uint32_t(count), "operator new");
}

void* operator new[] (std::size_t count)
{
	ZgAllocator& allocator = zg::getContext().allocator;
	return allocator.allocate(allocator.userPtr, uint32_t(count), "operator new[]");
}

void* operator new (std::size_t count, std::align_val_t val)
{
	assert(size_t(val) <= 32);
	ZgAllocator& allocator = zg::getContext().allocator;
	return allocator.allocate(allocator.userPtr, uint32_t(count), "operator new");
}

void* operator new[] (std::size_t count, std::align_val_t val)
{
	assert(size_t(val) <= 32);
	ZgAllocator& allocator = zg::getContext().allocator;
	return allocator.allocate(allocator.userPtr, uint32_t(count), "operator new[]");
}

void* operator new (std::size_t count, const std::nothrow_t&) noexcept
{
	ZgAllocator& allocator = zg::getContext().allocator;
	return allocator.allocate(allocator.userPtr, uint32_t(count), "operator new");
}

void* operator new[] (std::size_t count, const std::nothrow_t&) noexcept
{
	ZgAllocator& allocator = zg::getContext().allocator;
	return allocator.allocate(allocator.userPtr, uint32_t(count), "operator new[]");
}

void* operator new (std::size_t count, std::align_val_t val, const std::nothrow_t&) noexcept
{
	assert(size_t(val) <= 32);
	ZgAllocator& allocator = zg::getContext().allocator;
	return allocator.allocate(allocator.userPtr, uint32_t(count), "operator new");
}

void* operator new[] (std::size_t count, std::align_val_t val, const std::nothrow_t&) noexcept
{
	assert(size_t(val) <= 32);
	ZgAllocator& allocator = zg::getContext().allocator;
	return allocator.allocate(allocator.userPtr, uint32_t(count), "operator new[]");
}

// Operator delete
// ------------------------------------------------------------------------------------------------

void operator delete (void* ptr) noexcept
{
	ZgAllocator& allocator = zg::getContext().allocator;
	if (allocator.deallocate != nullptr) {
		return allocator.deallocate(allocator.userPtr, ptr);
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
	ZgAllocator& allocator = zg::getContext().allocator;
	if (allocator.deallocate != nullptr) {
		return allocator.deallocate(allocator.userPtr, ptr);
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
	assert(size_t(val) <= 32);
	ZgAllocator& allocator = zg::getContext().allocator;
	if (allocator.deallocate != nullptr) {
		return allocator.deallocate(allocator.userPtr, ptr);
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
	assert(size_t(val) <= 32);
	ZgAllocator& allocator = zg::getContext().allocator;
	if (allocator.deallocate != nullptr) {
		return allocator.deallocate(allocator.userPtr, ptr);
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
	ZgAllocator& allocator = zg::getContext().allocator;
	if (allocator.deallocate != nullptr) {
		return allocator.deallocate(allocator.userPtr, ptr);
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
	ZgAllocator& allocator = zg::getContext().allocator;
	if (allocator.deallocate != nullptr) {
		return allocator.deallocate(allocator.userPtr, ptr);
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
	assert(size_t(val) <= 32);
	(void)sz;
	ZgAllocator& allocator = zg::getContext().allocator;
	if (allocator.deallocate != nullptr) {
		return allocator.deallocate(allocator.userPtr, ptr);
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
	assert(size_t(val) <= 32);
	(void)sz;
	ZgAllocator& allocator = zg::getContext().allocator;
	if (allocator.deallocate != nullptr) {
		return allocator.deallocate(allocator.userPtr, ptr);
	}
	else {
#ifndef NDEBUG
		printf("ZeroG: No allocator set, attempting to deallocate: %llx. Expected if process is terminating.\n",
			uint64_t(ptr));
#endif
	}
}
