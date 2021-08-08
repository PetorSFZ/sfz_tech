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

#ifndef SKIPIFZERO_NEW_HPP
#define SKIPIFZERO_NEW_HPP
#pragma once

#include <new> // placement new
#include <utility> // std::move, std::forward, std::swap

#include "sfz.h"
#include "skipifzero.hpp"

// "new" and "delete" functions using sfz allocators
// ------------------------------------------------------------------------------------------------

// Constructs a new object of type T, similar to operator new. Guarantees 32-byte alignment.
template<typename T, typename... Args>
T* sfz_new(SfzAllocator* allocator, SfzDbgInfo dbg, Args&&... args) noexcept
{
	// Allocate memory (minimum 32-byte alignment), return nullptr on failure
	void* memPtr = allocator->alloc(dbg, sizeof(T), alignof(T) < 32 ? 32 : alignof(T));
	if (memPtr == nullptr) return nullptr;

	// Creates object (placement new), terminates program if constructor throws exception.
	return new(memPtr) T(std::forward<Args>(args)...);
}

// Deconstructs a C++ object created using sfz_new.
template<typename T>
void sfz_delete(SfzAllocator* allocator, T*& pointer) noexcept
{
	if (pointer == nullptr) return;
	pointer->~T(); // Call destructor, will terminate program if it throws exception.
	allocator->dealloc(pointer);
	pointer = nullptr; // Set callers pointer to nullptr, an attempt to avoid dangling pointers.
}

// UniquePtr
// ------------------------------------------------------------------------------------------------

namespace sfz {

// Simple replacement for std::unique_ptr using SfzAllocator
template<typename T>
class UniquePtr final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	SFZ_DECLARE_DROP_TYPE(UniquePtr);

	// Creates an empty UniquePtr (holding nullptr, no allocator set)
	UniquePtr(std::nullptr_t) noexcept {};

	// Creates a UniquePtr with the specified object and allocator
	// This UniquePtr takes ownership of the specified object, thus the object in question must
	// be allocated by the sfzCore allocator specified so it can be properly destroyed.
	UniquePtr(T* object, SfzAllocator* allocator) noexcept : mPtr(object), mAllocator(allocator) { }

	// Casts another pointer and takes ownership
	template<typename T2>
	UniquePtr(UniquePtr<T2>&& subclassPtr) noexcept { *this = subclassPtr.template castTake<T>(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void destroy() noexcept
	{
		if (mPtr == nullptr) return;
		sfz_delete<T>(mAllocator, mPtr);
		mPtr = nullptr;
		mAllocator = nullptr;
	}

	// Methods
	// --------------------------------------------------------------------------------------------

	T* get() const noexcept { return mPtr; }
	SfzAllocator* allocator() const noexcept { return mAllocator; }

	// Caller takes ownership of the internal pointer
	T* take() noexcept
	{
		T* tmp = mPtr;
		mPtr = nullptr;
		mAllocator = nullptr;
		return tmp;
	}

	// Casts (static_cast) the UniquePtr to another type and destroys the original (this) pointer.
	template<typename T2>
	UniquePtr<T2> castTake() noexcept
	{
		UniquePtr<T2> tmp(static_cast<T2*>(this->mPtr), this->mAllocator);
		this->mPtr = nullptr;
		this->mAllocator = nullptr;
		return tmp;
	}

	// Operators
	// --------------------------------------------------------------------------------------------

	T& operator* () const noexcept { return *mPtr; }
	T* operator-> () const noexcept { return mPtr; }

	bool operator== (const UniquePtr& other) const noexcept { return this->mPtr == other.mPtr; }
	bool operator!= (const UniquePtr& other) const noexcept { return !(*this == other); }

	bool operator== (std::nullptr_t) const noexcept { return this->mPtr == nullptr; }
	bool operator!= (std::nullptr_t) const noexcept { return this->mPtr != nullptr; }

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	T* mPtr = nullptr;
	SfzAllocator* mAllocator = nullptr;
};

// Constructs a new object of type T with the specified allocator and returns it in a UniquePtr
template<typename T, typename... Args>
UniquePtr<T> makeUnique(SfzAllocator* allocator, SfzDbgInfo dbg, Args&&... args) noexcept
{
	return UniquePtr<T>(sfz_new<T>(allocator, dbg, std::forward<Args>(args)...), allocator);
}

} // namespace sfz

#endif
