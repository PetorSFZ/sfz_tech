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

#include <cstddef> // nullptr_t
#include <type_traits> // std::is_array

#include "sfz/memory/Allocators.hpp"

namespace sfz {

// UniquePtr (interface)
// ------------------------------------------------------------------------------------------------

/// Simple replacement for std::unique_ptr using sfz allocators
/// Unlike std::unique_ptr there is NO support for arrays, use sfz::DynArray for that.
template<typename T, typename Allocator = StandardAllocator>
class UniquePtr final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------
	
	// Copying is prohibited
	UniquePtr(const UniquePtr&) = delete;
	UniquePtr& operator= (const UniquePtr&) = delete;

	// Move constructors
	UniquePtr(UniquePtr&& other) noexcept;
	UniquePtr& operator= (UniquePtr&& other) noexcept;

	/// Creates an empty UniquePtr (holding nullptr)
	UniquePtr() noexcept;

	/// Creates an empty UniquePtr (holding nullptr)
	UniquePtr(nullptr_t) noexcept;

	/// Creates a UniquePtr with the specified object
	/// This UniquePtr takes ownership of the specified object, thus the object in question must
	/// be allocated by the sfz allocator specified so it can be properly destroyed.
	explicit UniquePtr(T* object) noexcept;
	
	/// Destroys the object held using destroy()
	~UniquePtr() noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Returns the internal pointer
	T* get() const noexcept { return mPtr; }
	
	/// Caller takes ownership of the internal pointer
	/// The internal pointer will be set to nullptr without being destroyed
	T* take() noexcept;

	/// Destroys the object held by this UniquePtr and deallocates its memory
	/// After this method is called the internal pointer will be assigned nullptr as value. If the
	/// internal pointer already is a nullptr this method will do nothing. It is not necessary to
	/// call this method manually, it will automatically be called in UniquePtr's destructor.
	void destroy() noexcept;

	/// Swaps the internal pointers of this and the other UniquePtrs
	void swap(UniquePtr& other) noexcept;

	// Operators
	// --------------------------------------------------------------------------------------------

	T& operator* () const noexcept { return *mPtr; }
	T* operator-> () const noexcept { return mPtr; }

	bool operator== (UniquePtr& other) const noexcept;
	bool operator!= (UniquePtr& other) const noexcept;

	bool operator== (nullptr_t) const noexcept;
	bool operator!= (nullptr_t) const noexcept;

private:
	T* mPtr = nullptr;
};

} // namespace sfz

#include "sfz/memory/SmartPointers.inl"
