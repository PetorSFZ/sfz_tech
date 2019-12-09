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

namespace sfz {

// UniquePtr (implementation): Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename T>
UniquePtr<T>::UniquePtr(T* object, Allocator* allocator) noexcept
:
	mPtr(object),
	mAllocator(allocator)
{ }

template<typename T>
template<typename T2>
UniquePtr<T>::UniquePtr(UniquePtr<T2>&& subclassPtr) noexcept
{
	static_assert(std::is_base_of<T,T2>::value, "T2 is not a subclass of T");
	*this = subclassPtr.template castTake<T>();
}

template<typename T>
UniquePtr<T>::UniquePtr(UniquePtr&& other) noexcept
{
	this->swap(other);
}

template<typename T>
UniquePtr<T>& UniquePtr<T>::operator= (UniquePtr&& other) noexcept
{
	this->swap(other);
	return *this;
}

template<typename T>
UniquePtr<T>::~UniquePtr() noexcept
{
	this->destroy();
}

// UniquePtr (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename T>
void UniquePtr<T>::swap(UniquePtr& other) noexcept
{
	T* thisPtr = this->mPtr;
	Allocator* thisAllocator = this->mAllocator;

	this->mPtr = other.mPtr;
	this->mAllocator = other.mAllocator;

	other.mPtr = thisPtr;
	other.mAllocator = thisAllocator;
}

template<typename T>
void UniquePtr<T>::destroy() noexcept
{
	if (mPtr == nullptr) return;
	mAllocator->deleteObject<T>(mPtr);
	mPtr = nullptr;
	mAllocator = nullptr;
}

template<typename T>
T* UniquePtr<T>::take() noexcept
{
	T* tmp = mPtr;
	mPtr = nullptr;
	mAllocator = nullptr;
	return tmp;
}

template<typename T>
template<typename T2>
UniquePtr<T2> UniquePtr<T>::castTake() noexcept
{
	UniquePtr<T2> tmp(static_cast<T2*>(this->mPtr), this->mAllocator);
	this->mPtr = nullptr;
	this->mAllocator = nullptr;
	return tmp;
}

// UniquePtr (implementation): Operators
// ------------------------------------------------------------------------------------------------

template<typename T>
bool UniquePtr<T>::operator== (const UniquePtr& other) const noexcept
{
	return this->mPtr == other.mPtr;
}

template<typename T>
bool UniquePtr<T>::operator!= (const UniquePtr& other) const noexcept
{
	return !(*this == other);
}

// UniquePtr (implementation): free operators
// ------------------------------------------------------------------------------------------------

template<typename T>
bool operator== (const UniquePtr<T>& lhs, std::nullptr_t) noexcept
{
	return lhs.get() == nullptr;
}

template<typename T>
bool operator== (std::nullptr_t, const UniquePtr<T>& rhs) noexcept
{
	return rhs.get() == nullptr;
}

template<typename T>
bool operator!= (const UniquePtr<T>& lhs, std::nullptr_t) noexcept
{
	return lhs.get() != nullptr;
}

template<typename T>
bool operator!= (std::nullptr_t, const UniquePtr<T>& rhs) noexcept
{
	return rhs.get() == nullptr;
}

// UniquePtr (implementation): makeUnique()
// ------------------------------------------------------------------------------------------------

template<typename T, typename... Args>
UniquePtr<T> makeUnique(Allocator* allocator, Args&&... args) noexcept
{
	return UniquePtr<T>(
		allocator->newObject<T>(sfz_dbg("UniquePtr"), std::forward<Args>(args)...), allocator);
}

template<typename T, typename... Args>
UniquePtr<T> makeUniqueDefault(Args&&... args) noexcept
{
	return makeUnique<T>(getDefaultAllocator(), std::forward<Args>(args)...);
}

} // namespace sfz
