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

#include "sfz/memory/Allocator.hpp"
#include "sfz/memory/New.hpp"

namespace sfz {

// UniquePtr (interface)
// ------------------------------------------------------------------------------------------------

/// Simple replacement for std::unique_ptr using sfzCore allocators
/// Unlike std::unique_ptr there is NO support for arrays, use sfz::DynArray for that.
template<typename T>
class UniquePtr final {
public:
	static_assert(!std::is_array<T>::value, "UniquePtr does not accept array types");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------
	
	// Copying is prohibited
	UniquePtr(const UniquePtr&) = delete;
	UniquePtr& operator= (const UniquePtr&) = delete;

	/// Creates an empty UniquePtr (holding nullptr, no allocator set)
	UniquePtr() noexcept = default;

	/// Creates an empty UniquePtr (holding nullptr, no allocator set)
	UniquePtr(std::nullptr_t) noexcept {};
	
	/// Creates a UniquePtr with the specified object and allocator
	/// This UniquePtr takes ownership of the specified object, thus the object in question must
	/// be allocated by the sfzCore allocator specified so it can be properly destroyed.
	UniquePtr(T* object, Allocator* allocator) noexcept;

	/// Casts a subclass to a base class.
	template<typename T2>
	UniquePtr(UniquePtr<T2>&& subclassPtr) noexcept;

	/// Move constructors. Equivalent to swap() method.
	UniquePtr(UniquePtr&& other) noexcept;
	UniquePtr& operator= (UniquePtr&& other) noexcept;

	/// Destroys the object held using destroy()
	~UniquePtr() noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Swaps the internal pointers (and allocators) of this and the other UniquePtr
	void swap(UniquePtr& other) noexcept;

	/// Destroys the object held by this UniquePtr, deallocates its memory and removes allocator.
	/// After this method is called the internal pointer and allocator will be assigned nullptr as
	/// value. If the internal pointer already is a nullptr this method will do nothing. It is not
	/// necessary to call this method manually, it will automatically be called in UniquePtr's
	/// destructor.
	void destroy() noexcept;

	/// Returns the internal pointer
	T* get() const noexcept { return mPtr; }
	
	/// Returns the allocator of this UniquePtr, returns nullptr if no allocator is set
	Allocator* allocator() const noexcept { return mAllocator; }

	/// Caller takes ownership of the internal pointer
	/// The internal pointer will be set to nullptr without being destroyed, allocator will be
	/// set to nullptr
	T* take() noexcept;

	/// Casts (static_cast) the UniquePtr to another type and destroys the original
	/// The original UniquePtr will be destroyed afterwards (but not the object held).
	template<typename T2>
	UniquePtr<T2> castTake() noexcept;

	// Operators
	// --------------------------------------------------------------------------------------------

	T& operator* () const noexcept { return *mPtr; }
	T* operator-> () const noexcept { return mPtr; }

	bool operator== (const UniquePtr& other) const noexcept;
	bool operator!= (const UniquePtr& other) const noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	T* mPtr = nullptr;
	Allocator* mAllocator = nullptr;
};

template<typename T>
bool operator== (const UniquePtr<T>& lhs, std::nullptr_t rhs) noexcept;

template<typename T>
bool operator== (std::nullptr_t lhs, const UniquePtr<T>& rhs) noexcept;

template<typename T>
bool operator!= (const UniquePtr<T>& lhs, std::nullptr_t rhs) noexcept;

template<typename T>
bool operator!= (std::nullptr_t lhs, const UniquePtr<T>& rhs) noexcept;

/// Constructs a new object of type T with the specified allocator and returns it in a UniquePtr
/// Will exit the program through std::terminate() if constructor throws an exception
/// \return nullptr if memory allocation failed
template<typename T, typename... Args>
UniquePtr<T> makeUnique(Allocator* allocator, Args&&... args) noexcept;

/// Constructs a new object of type T with the default allocator and returns it in a UniquePtr
/// Will exit the program through std::terminate() if constructor throws an exception
/// \return nullptr if memory allocation failed
template<typename T, typename... Args>
UniquePtr<T> makeUniqueDefault(Args&&... args) noexcept;

// SharedPtr (interface)
// ------------------------------------------------------------------------------------------------

namespace detail {

/// Inner state of all SharedPtrs
struct SharedPtrState final {
	Allocator* allocator = nullptr;
	std::atomic_uint32_t refCount = 0;
};

}

/// Simple replacement for std::shared_ptr using sfzCore allocators
/// Unlike std::shared_ptr there is NO support for arrays, use sfz::DynArray for that.
template<typename T>
class SharedPtr final {
public:
	static_assert(!std::is_array<T>::value, "SharedPtr does not accept array types");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	/// Creates an empty SharedPtr (holding nullptr and no state)
	SharedPtr() noexcept = default;

	/// Creates an empty SharedPtr (holding nullptr and no state)
	SharedPtr(std::nullptr_t) noexcept { }

	/// Creates a SharedPtr with the specified object
	/// This SharedPtr takes ownership of the specified object, thus the object in question must
	/// be allocated by the sfzCore allocator specified so it can be properly destroyed.
	SharedPtr(T* object, Allocator* allocator) noexcept;
	
	/// Casts a subclass to a base class.
	template<typename T2>
	SharedPtr(const SharedPtr<T2>& subclassPtr) noexcept;

	/// Casts a subclasss UniquePtr to a base class SharedPtr
	template<typename T2>
	SharedPtr(UniquePtr<T2>&& subclassPtr) noexcept;

	/// Copy constructors. Copies a SharedPtr and increments the reference counter.
	SharedPtr(const SharedPtr& other) noexcept;
	SharedPtr& operator= (const SharedPtr& other) noexcept;

	/// Move constructors. Swaps two pointers (and states) using swap().
	SharedPtr(SharedPtr&& other) noexcept;
	SharedPtr& operator= (SharedPtr&& other) noexcept;

	/// Destroys this SharedPtr (not necessarily the object pointed at) with destroy().
	~SharedPtr() noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Swaps the internal pointers and state of this and the other SharedPtr
	void swap(SharedPtr& other) noexcept;

	/// Decrements the reference counter and deletes both the object and state if counter is 0.
	/// Will do nothing if no object is held. This SharedPointer will have no object or state after
	/// destroy() is called.
	void destroy() noexcept;

	/// Returns the internal pointer
	T* get() const noexcept { return mPtr; }

	/// Returns the allocator used to allocate the internal object
	Allocator* allocator() const noexcept;

	/// Returns the number of references to the internal object (or 0 if counter doesn't exist)
	size_t refCount() const noexcept;

	/// Casts (static_cast) the pointer to another type. The original is preserved, increments
	/// ref count by 1.
	template<typename T2>
	SharedPtr<T2> cast() const noexcept;

	/// Sets the internal state of this SmartPointer. Should never be called. Exists for
	/// implementation reasons.
	void dangerousSetState(T* ptr, detail::SharedPtrState* state) noexcept;

	// Operators
	// --------------------------------------------------------------------------------------------

	T& operator* () const noexcept { return *mPtr; }
	T* operator-> () const noexcept { return mPtr; }

	bool operator== (const SharedPtr& other) const noexcept;
	bool operator!= (const SharedPtr& other) const noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	T* mPtr = nullptr;
	detail::SharedPtrState* mState = nullptr;
};

template<typename T>
bool operator== (const SharedPtr<T>& lhs, std::nullptr_t rhs) noexcept;

template<typename T>
bool operator== (std::nullptr_t lhs, const SharedPtr<T>& rhs) noexcept;

template<typename T>
bool operator!= (const SharedPtr<T>& lhs, std::nullptr_t rhs) noexcept;

template<typename T>
bool operator!= (std::nullptr_t lhs, const SharedPtr<T>& rhs) noexcept;

/// Constructs a new object of type T with the specified allocator and returns it in a SharedPtr
/// Will exit the program through std::terminate() if constructor throws an exception
/// \return nullptr if memory allocation failed
template<typename T, typename... Args>
SharedPtr<T> makeShared(Allocator* allocator, Args&&... args) noexcept;

/// Constructs a new object of type T with the default allocator and returns it in a SharedPtr
/// Will exit the program through std::terminate() if constructor throws an exception
/// \return nullptr if memory allocation failed
template<typename T, typename... Args>
SharedPtr<T> makeSharedDefault(Args&&... args) noexcept;

} // namespace sfz

#include "sfz/memory/SmartPointers.inl"
