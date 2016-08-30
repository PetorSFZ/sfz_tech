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

#include <atomic>
#include <cstddef> // nullptr_t
#include <type_traits> // std::is_array

#include "sfz/memory/Allocators.hpp"
#include "sfz/memory/New.hpp"

namespace sfz {

// UniquePtr (interface)
// ------------------------------------------------------------------------------------------------

/// Simple replacement for std::unique_ptr using sfz allocators
/// Unlike std::unique_ptr there is NO support for arrays, use sfz::DynArray for that.
template<typename T, typename Allocator = StandardAllocator>
class UniquePtr final {
public:
	static_assert(!std::is_array<T>::value, "UniquePtr does not accept array types");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------
	
	// Copying is prohibited
	UniquePtr(const UniquePtr&) = delete;
	UniquePtr& operator= (const UniquePtr&) = delete;

	/// Creates an empty UniquePtr (holding nullptr)
	UniquePtr() noexcept = default;

	/// Creates an empty UniquePtr (holding nullptr)
	UniquePtr(std::nullptr_t) noexcept {};
	
	/// Creates a UniquePtr with the specified object
	/// This UniquePtr takes ownership of the specified object, thus the object in question must
	/// be allocated by the sfz allocator specified so it can be properly destroyed.
	explicit UniquePtr(T* object) noexcept;

	/// Move constructor
	UniquePtr(UniquePtr&& other) noexcept;
	
	/// Move assignment constructor
	UniquePtr& operator= (UniquePtr&& other) noexcept;

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

	/// Swaps the internal pointers of this and the other UniquePtr
	void swap(UniquePtr& other) noexcept;

	// Operators
	// --------------------------------------------------------------------------------------------

	T& operator* () const noexcept { return *mPtr; }
	T* operator-> () const noexcept { return mPtr; }

	bool operator== (const UniquePtr& other) const noexcept;
	bool operator!= (const UniquePtr& other) const noexcept;

	bool operator== (std::nullptr_t) const noexcept;
	bool operator!= (std::nullptr_t) const noexcept;

private:
	T* mPtr = nullptr;
};

/// Constructs a new object of type T with the specified allocator and returns it in a UniquePtr
/// Will exit the program through std::terminate() if constructor throws an exception
/// \return nullptr if memory allocation failed
template<typename T, typename Allocator = StandardAllocator, typename... Args>
UniquePtr<T,Allocator> makeUnique(Args&&... args) noexcept;

// SharedPtr (interface)
// ------------------------------------------------------------------------------------------------

/// Simple replacement for std::shared_ptr using sfz allocators
/// Unlike std::shared_ptr there is NO support for arrays, use sfz::DynArray for that.
template<typename T, typename Allocator = StandardAllocator>
class SharedPtr final {
public:
	static_assert(!std::is_array<T>::value, "SharedPtr does not accept array types");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	/// Creates an empty SharedPtr (holding nullptr and no counter)
	SharedPtr() noexcept = default;

	/// Creates an empty SharedPtr (holding nullptr and no counter)
	SharedPtr(std::nullptr_t) noexcept { }

	/// Creates a SharedPtr with the specified object
	/// This SharedPtr takes ownership of the specified object, thus the object in question must
	/// be allocated by the sfz allocator specified so it can be properly destroyed.
	explicit SharedPtr(T* object) noexcept;
	
	/// Copies a SharedPtr and increments the reference counter
	SharedPtr(const SharedPtr& other) noexcept;

	/// Copies a SharedPtr and increments the reference counter
	SharedPtr& operator= (const SharedPtr& other) noexcept;

	SharedPtr(SharedPtr&& other) noexcept;
	SharedPtr& operator= (SharedPtr&& other) noexcept;

	/// Decrements the reference counter, deletes both the object and counter if counter is 0.
	~SharedPtr() noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Returns the internal pointer
	T* get() const noexcept { return mPtr; }

	/// Returns the number of references to the internal object (or 0 if counter doesn't exist)
	size_t refCount() const noexcept;

	/// Swaps the internal pointers and counters of this and the other SharedPtr
	void swap(SharedPtr& other) noexcept;

	// Operators
	// --------------------------------------------------------------------------------------------

	T& operator* () const noexcept { return *mPtr; }
	T* operator-> () const noexcept { return mPtr; }

	bool operator== (const SharedPtr& other) const noexcept;
	bool operator!= (const SharedPtr& other) const noexcept;

	bool operator== (std::nullptr_t) const noexcept;
	bool operator!= (std::nullptr_t) const noexcept;

private:
	T* mPtr = nullptr;
	std::atomic_size_t* mRefCountPtr = nullptr;
};

/// Constructs a new object of type T with the specified allocator and returns it in a SharedPtr
/// Will exit the program through std::terminate() if constructor throws an exception
/// \return nullptr if memory allocation failed
template<typename T, typename Allocator = StandardAllocator, typename... Args>
SharedPtr<T,Allocator> makeShared(Args&&... args) noexcept;

} // namespace sfz

#include "sfz/memory/SmartPointers.inl"
