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

#include <algorithm> // std::max()
#include <utility> // std::forward

#include "sfz/memory/Allocator.hpp"

namespace sfz {

// New
// ------------------------------------------------------------------------------------------------

/// Constructs a new object of type T with the specified allocator
/// The object is guaranteed to be 32-byte aligned
/// Will exit the program through std::terminate() if constructor throws an exception
/// \param allocator the allocator
/// \return nullptr if memory allocation failed
template<typename T, typename... Args>
T* sfzNew(Allocator* allocator, Args&&... args) noexcept
{
	void* memPtr = allocator->allocate(sizeof(T), std::max<uint32_t>(32, alignof(T)));
	T* objPtr = nullptr;
	objPtr = new(memPtr) T(std::forward<Args>(args)...);
	// If constructor throws exception std::terminate() will be called since function is noexcept
	return objPtr;
}

/// Constructs a new object of type T with the default allocator, see sfzNew().
template<typename T, typename... Args>
T* sfzNewDefault(Args&&... args) noexcept
{
	return sfzNew<T>(getDefaultAllocator(), std::forward<Args>(args)...);
}

// Delete
// ------------------------------------------------------------------------------------------------

/// Deletes an object created with the specified allocator
/// Will exit the program through std::terminate() if destructor throws an exception
/// \param pointer to the object
/// \param allocator the allocator used to allocate the object's memory
template<typename T>
void sfzDelete(T* pointer, Allocator* allocator) noexcept
{
	if (pointer == nullptr) return;
	pointer->~T();
	// If destructor throws exception std::terminate() will be called since function is noexcept
	allocator->deallocate(static_cast<void*>(pointer));
}

/// Deletes an object created with the default allocator, sfzDelete().
template<typename T>
void sfzDeleteDefault(T* pointer) noexcept
{
	return sfzDelete<T>(pointer, getDefaultAllocator());
}

} // namespace sfz
