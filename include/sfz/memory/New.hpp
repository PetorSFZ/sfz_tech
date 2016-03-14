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

#include <utility> // std::forward

#include "sfz/memory/Allocators.hpp"

namespace sfz {

// New
// ------------------------------------------------------------------------------------------------

/// Constructs a new object of type T with the default allocator
/// The object is guaranteed to be 32-byte aligned
/// Will exit the program through std::terminate() if constructor throws an exception
/// \return nullptr if memory allocation failede
template<typename T, typename... Args>
T* sfz_new(Args&&... args) noexcept
{
	void* memPtr = StandardAllocator::allocate(sizeof(T), 32);
	T* objPtr = nullptr;
	objPtr = new(memPtr) T{std::forward<Args>(args)...};
	// If constructor throws exception std::terminate() will be called since function is noexcept
	return objPtr;
}

// Delete
// ------------------------------------------------------------------------------------------------

/// Deletes an object created with sfz_new()
/// Will exit the program through std::terminate() if destructor throws an exception
/// \param pointer to the object
template<typename T>
void sfz_delete(T* pointer) noexcept
{
	if (pointer == nullptr) return;
	pointer->~T();
	// If destructor throws exception std::terminate() will be called since function is noexcept
	StandardAllocator::deallocate(static_cast<void*>(pointer));
}

} // namespace sfz