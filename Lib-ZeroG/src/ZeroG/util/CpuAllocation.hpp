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

#pragma once

#include <new>
#include <utility> // std::forward

#include "ZeroG.h"

namespace zg {

// New/Delete helpers
// ------------------------------------------------------------------------------------------------

template<typename T, typename... Args>
T* zgNew(ZgAllocator& allocator, const char* name, Args&&... args) noexcept
{
	uint8_t* memPtr = allocator.allocate(allocator.userPtr, sizeof(T), name);
	T* objPtr = nullptr;
	objPtr = new(memPtr) T(std::forward<Args>(args)...);
	// If constructor throws exception std::terminate() will be called since function is noexcept
	return objPtr;
}

template<typename T>
void zgDelete(ZgAllocator& allocator, T* pointer) noexcept
{
	if (pointer == nullptr) return;
	pointer->~T();
	// If destructor throws exception std::terminate() will be called since function is noexcept
	allocator.deallocate(allocator.userPtr, reinterpret_cast<uint8_t*>(pointer));
}

// Default allocator
// ------------------------------------------------------------------------------------------------

ZgAllocator getDefaultAllocator() noexcept;

} // namespace zg
