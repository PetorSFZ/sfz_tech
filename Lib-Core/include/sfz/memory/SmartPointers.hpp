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

#include <skipifzero.hpp>

namespace sfz {

// UniquePtr
// ------------------------------------------------------------------------------------------------

// Simple replacement for std::unique_ptr using sfz::Allocator
template<typename T>
class UniquePtr final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	UniquePtr() noexcept = default;
	UniquePtr(const UniquePtr&) = delete;
	UniquePtr& operator= (const UniquePtr&) = delete;
	UniquePtr(UniquePtr&& other) noexcept { this->swap(other); }
	UniquePtr& operator= (UniquePtr&& other) noexcept { this->swap(other); return *this; }
	~UniquePtr() noexcept { this->destroy(); }

	// Creates an empty UniquePtr (holding nullptr, no allocator set)
	UniquePtr(std::nullptr_t) noexcept {};

	// Creates a UniquePtr with the specified object and allocator
	// This UniquePtr takes ownership of the specified object, thus the object in question must
	// be allocated by the sfzCore allocator specified so it can be properly destroyed.
	UniquePtr(T* object, Allocator* allocator) noexcept : mPtr(object), mAllocator(allocator) { }

	// Casts a subclass to a base class.
	template<typename T2>
	UniquePtr(UniquePtr<T2>&& subclassPtr) noexcept
	{
		static_assert(std::is_base_of<T, T2>::value, "T2 is not a subclass of T");
		*this = subclassPtr.template castTake<T>();
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(UniquePtr& other) noexcept
	{
		std::swap(this->mPtr, other.mPtr);
		std::swap(this->mAllocator, other.mAllocator);
	}

	void destroy() noexcept
	{
		if (mPtr == nullptr) return;
		mAllocator->deleteObject<T>(mPtr);
		mPtr = nullptr;
		mAllocator = nullptr;
	}

	// Methods
	// --------------------------------------------------------------------------------------------

	T* get() const noexcept { return mPtr; }
	Allocator* allocator() const noexcept { return mAllocator; }

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

	bool operator== (std::nullptr_t other) const noexcept { return this->mPtr == nullptr; }
	bool operator!= (std::nullptr_t other) const noexcept { return this->mPtr != nullptr; }

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	T* mPtr = nullptr;
	Allocator* mAllocator = nullptr;
};

// Constructs a new object of type T with the specified allocator and returns it in a UniquePtr
template<typename T, typename... Args>
UniquePtr<T> makeUnique(Allocator* allocator, DbgInfo dbg, Args&&... args) noexcept
{
	return UniquePtr<T>(
		allocator->newObject<T>(dbg, std::forward<Args>(args)...), allocator);
}

} // namespace sfz
