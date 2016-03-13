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

#include "sfz/memory/Allocators.hpp"

namespace sfz {

// UniquePtr
// ------------------------------------------------------------------------------------------------

/// Simple replacement for std::unique_ptr using sfz allocators
template<typename T, typename Allocator = StandardAllocator>
class UniquePtr final {
public:
	
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	UniquePtr() noexcept = default;
	UniquePtr(const UniquePtr&) = delete;
	UniquePtr& operator= (const UniquePtr&) = delete;
	
	UniquePtr(T* object) noexcept
	:
		mPtr{object}
	{ }

	UniquePtr(UniquePtr&& other) noexcept
	{
		T* tmp = other.mPtr;
		other.mPtr = this->mPtr;
		this->mPtr = tmp;
	}

	UniquePtr& operator= (UniquePtr&& other) noexcept
	{
		T* tmp = other.mPtr;
		other.mPtr = this->mPtr;
		this->mPtr = tmp;
		return *this;
	}

	~UniquePtr() noexcept
	{
		this->destroy();
	}

	// Public methods
	// --------------------------------------------------------------------------------------------

	T* get() const noexcept { return mPtr; }
	
	/// Destroys the object held by this UniquePtr and deallocates its memory
	/// After this method is called the internal pointer will be assigned nullptr as value. If the
	/// internal pointer already is a nullptr this method will do nothing. It is not necessary to
	/// call this method manually, it will automatically be called in UniquePtr's destructor.
	void destroy() const noexcept
	{
		if (mPtr == nullptr) return;
		try {
			mPtr->~T();
		} catch (...) {
			// This is potentially pretty bad, let's just quit the program
			std::terminate();
		}
		Allocator::deallocate((void*&)mPtr);
	}

private:
	T* mPtr = nullptr;
};

} // namespace sfz