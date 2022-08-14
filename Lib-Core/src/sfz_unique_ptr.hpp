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

#ifndef SFZ_UNIQUE_PTR_HPP
#define SFZ_UNIQUE_PTR_HPP
#pragma once

#include "sfz.h"
#include "sfz_cpp.hpp"

#ifdef __cplusplus

// SfzUniquePtr
// ------------------------------------------------------------------------------------------------

// Simple replacement for std::unique_ptr using SfzAllocator
template<typename T>
class SfzUniquePtr final {
public:
	SFZ_DECLARE_DROP_TYPE(SfzUniquePtr);

	// Creates an empty UniquePtr (holding nullptr, no allocator set)
	SfzUniquePtr(std::nullptr_t) noexcept {};

	// Creates a UniquePtr with the specified object and allocator
	// This UniquePtr takes ownership of the specified object, thus the object in question must
	// be allocated by the sfzCore allocator specified so it can be properly destroyed.
	SfzUniquePtr(T* object, SfzAllocator* allocator) noexcept : mPtr(object), mAllocator(allocator) { }

	// Casts another pointer and takes ownership
	template<typename T2>
	SfzUniquePtr(SfzUniquePtr<T2>&& subclassPtr) noexcept { *this = subclassPtr.template castTake<T>(); }

	void destroy() noexcept
	{
		if (mPtr == nullptr) return;
		sfz_delete<T>(mAllocator, mPtr);
		mPtr = nullptr;
		mAllocator = nullptr;
	}

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
	SfzUniquePtr<T2> castTake() noexcept
	{
		SfzUniquePtr<T2> tmp(static_cast<T2*>(this->mPtr), this->mAllocator);
		this->mPtr = nullptr;
		this->mAllocator = nullptr;
		return tmp;
	}

	T& operator* () const noexcept { return *mPtr; }
	T* operator-> () const noexcept { return mPtr; }

	bool operator== (const SfzUniquePtr& other) const noexcept { return this->mPtr == other.mPtr; }
	bool operator!= (const SfzUniquePtr& other) const noexcept { return !(*this == other); }

	bool operator== (std::nullptr_t) const noexcept { return this->mPtr == nullptr; }
	bool operator!= (std::nullptr_t) const noexcept { return this->mPtr != nullptr; }

private:
	T* mPtr = nullptr;
	SfzAllocator* mAllocator = nullptr;
};

// Constructs a new object of type T with the specified allocator and returns it in a UniquePtr
template<typename T, typename... Args>
SfzUniquePtr<T> sfzMakeUnique(SfzAllocator* allocator, SfzDbgInfo dbg, Args&&... args) noexcept
{
	return SfzUniquePtr<T>(sfz_new<T>(allocator, dbg, sfz_forward(args)...), allocator);
}

#endif // __cplusplus
#endif // SFZ_UNIQUE_PTR_HPP
