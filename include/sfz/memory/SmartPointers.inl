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
UniquePtr<T, Allocator>::UniquePtr(T* object) noexcept
:
	mPtr{object}
{ }

template<typename T, typename Allocator>
UniquePtr<T, Allocator>::UniquePtr(UniquePtr<T, Allocator>&& other) noexcept
{
	this->swap(other);
}

template<typename T, typename Allocator>
UniquePtr<T, Allocator>& UniquePtr<T, Allocator>::operator= (UniquePtr<T, Allocator>&& other) noexcept
{
	this->swap(other);
	return *this;
}

template<typename T, typename Allocator>
UniquePtr<T, Allocator>::~UniquePtr() noexcept
{
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
	sfz_delete<T, Allocator>(mPtr);
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
bool UniquePtr<T, Allocator>::operator== (const UniquePtr& other) const noexcept
{
	return this->mPtr == other.mPtr;
}

template<typename T, typename Allocator>
bool UniquePtr<T, Allocator>::operator!= (const UniquePtr& other) const noexcept
{
	return !(*this == other);
}

template<typename T, typename Allocator>
bool UniquePtr<T, Allocator>::operator== (std::nullptr_t) const noexcept
{
	return this->mPtr == nullptr;
}

template<typename T, typename Allocator>
bool UniquePtr<T, Allocator>::operator!= (std::nullptr_t) const noexcept
{
	return this->mPtr != nullptr;
}

// UniquePtr (implementation): makeUnique()
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator, typename... Args>
UniquePtr<T, Allocator> makeUnique(Args&&... args) noexcept
{
	return UniquePtr<T, Allocator>(sfz_new<T, Allocator>(std::forward<Args>(args)...));
}

// SharedPtr (implementation): Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
SharedPtr<T, Allocator>::SharedPtr(T* object) noexcept
{
	mRefCountPtr = sfz_new<std::atomic_size_t, Allocator>(size_t(1));
	mPtr = object;
}

template<typename T, typename Allocator>
SharedPtr<T, Allocator>::SharedPtr(const SharedPtr<T, Allocator>& other) noexcept
{
	*this = other;
}

template<typename T, typename Allocator>
SharedPtr<T, Allocator>& SharedPtr<T, Allocator>::operator= (const SharedPtr<T, Allocator>& other) noexcept
{
	if (other == nullptr) return *this;
	(*other.mRefCountPtr)++;
	this->mRefCountPtr = other.mRefCountPtr;
	this->mPtr = other.mPtr;
	return *this;
}

template<typename T, typename Allocator>
SharedPtr<T, Allocator>::SharedPtr(SharedPtr&& other) noexcept
{
	this->swap(other);
}

template<typename T, typename Allocator>
SharedPtr<T, Allocator>& SharedPtr<T, Allocator>::operator= (SharedPtr&& other) noexcept
{
	this->swap(other);
	return *this;
}

template<typename T, typename Allocator>
SharedPtr<T, Allocator>::~SharedPtr() noexcept
{
	if (mPtr == nullptr) return;
	size_t count = mRefCountPtr->fetch_sub(1) - 1;
	if (count == 0) {
		sfz_delete<T, Allocator>(mPtr);
		sfz_delete<std::atomic_size_t, Allocator>(mRefCountPtr);
	}
}

// SharedPtr (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
size_t SharedPtr<T, Allocator>::refCount() const noexcept
{
	if (mRefCountPtr == nullptr) return 0;
	return *mRefCountPtr;
}

template<typename T, typename Allocator>
void SharedPtr<T, Allocator>::swap(SharedPtr& other) noexcept
{
	T* tmpPtr = other.mPtr;
	other.mPtr = this->mPtr;
	this->mPtr = tmpPtr;

	std::atomic_size_t* tmpRefCount = other.mRefCountPtr;
	other.mRefCountPtr = this->mRefCountPtr;
	this->mRefCountPtr = tmpRefCount;
}

// SharedPtr (implementation): Operators
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
bool SharedPtr<T, Allocator>::operator== (const SharedPtr& other) const noexcept
{
	return this->mPtr == other.mPtr;
}

template<typename T, typename Allocator>
bool SharedPtr<T, Allocator>::operator!= (const SharedPtr& other) const noexcept
{
	return !(*this == other);
}

template<typename T, typename Allocator>
bool SharedPtr<T, Allocator>::operator== (std::nullptr_t) const noexcept
{
	return this->mPtr == nullptr;
}

template<typename T, typename Allocator>
bool SharedPtr<T, Allocator>::operator!= (std::nullptr_t) const noexcept
{
	return this->mPtr != nullptr;
}


// SharedPtr (implementation): makeShared()
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator, typename... Args>
SharedPtr<T, Allocator> makeShared(Args&&... args) noexcept
{
	return SharedPtr<T, Allocator>(sfz_new<T, Allocator>(std::forward<Args>(args)...));
}

} // namespace sfz