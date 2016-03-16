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

template<typename T, typename Allocator>
UniquePtr<T, Allocator>::UniquePtr(UniquePtr<T, Allocator>&& other) noexcept
{
	static_assert(!std::is_array<T>::value, "UniquePtr does not accept array types");
	this->swap(other);
}

template<typename T, typename Allocator>
UniquePtr<T, Allocator>& UniquePtr<T, Allocator>::operator= (UniquePtr<T, Allocator>&& other) noexcept
{
	static_assert(!std::is_array<T>::value, "UniquePtr does not accept array types");
	this->swap(other);
	return *this;
}

template<typename T, typename Allocator>
UniquePtr<T, Allocator>::UniquePtr() noexcept
{
	static_assert(!std::is_array<T>::value, "UniquePtr does not accept array types");
}

template<typename T, typename Allocator>
UniquePtr<T, Allocator>::UniquePtr(nullptr_t) noexcept
{
	static_assert(!std::is_array<T>::value, "UniquePtr does not accept array types");
}

template<typename T, typename Allocator>
UniquePtr<T, Allocator>::UniquePtr(T* object) noexcept
:
	mPtr{object}
{
	static_assert(!std::is_array<T>::value, "UniquePtr does not accept array types");
}

template<typename T, typename Allocator>
UniquePtr<T, Allocator>::~UniquePtr() noexcept
{
	static_assert(!std::is_array<T>::value, "UniquePtr does not accept array types");
	this->destroy();
}

// UniquePtr (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
T* UniquePtr<T, Allocator>::take() noexcept
{
	T* tmp = mPtr;
	mPtr = nullptr;
	return tmp;
}

template<typename T, typename Allocator>
void UniquePtr<T, Allocator>::destroy() noexcept
{
	if (mPtr == nullptr) return;
	mPtr->~T();
	// If destructor throws exception std::terminate() will be called since function is noexcept
	Allocator::deallocate(static_cast<void*>(mPtr));
	mPtr = nullptr;
}

template<typename T, typename Allocator>
void UniquePtr<T, Allocator>::swap(UniquePtr& other) noexcept
{
	T* tmp = other.mPtr;
	other.mPtr = this->mPtr;
	this->mPtr = tmp;
}

// UniquePtr (implementation): Operators
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
bool UniquePtr<T, Allocator>::operator== (UniquePtr& other) const noexcept
{
	return this->mPtr == other.mPtr;
}

template<typename T, typename Allocator>
bool UniquePtr<T, Allocator>::operator!= (UniquePtr& other) const noexcept
{
	return !(*this == other);
}

template<typename T, typename Allocator>
bool UniquePtr<T, Allocator>::operator== (nullptr_t) const noexcept
{
	return this->mPtr == nullptr;
}

template<typename T, typename Allocator>
bool UniquePtr<T, Allocator>::operator!= (nullptr_t) const noexcept
{
	return this->mPtr != nullptr;
}

} // namespace sfz