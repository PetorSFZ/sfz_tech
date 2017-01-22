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
	*this = subclassPtr.castTake<T>();
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
	sfzDelete<T>(mPtr, mAllocator);
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
	return UniquePtr<T>(sfzNew<T>(allocator, std::forward<Args>(args)...), allocator);
}

template<typename T, typename... Args>
UniquePtr<T> makeUniqueDefault(Args&&... args) noexcept
{
	return makeUnique<T>(getDefaultAllocator(), std::forward<Args>(args)...);
}

// SharedPtr (implementation): Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename T>
SharedPtr<T>::SharedPtr(T* object, Allocator* allocator) noexcept
{
	mPtr = object;
	mState = sfzNew<detail::SharedPtrState>(allocator);
	mState->allocator = allocator;
	mState->refCount = 1;
}

template<typename T>
template<typename T2>
SharedPtr<T>::SharedPtr(const SharedPtr<T2>& subclassPtr) noexcept
{
	static_assert(std::is_base_of<T,T2>::value, "T2 is not a subclass of T");
	*this = subclassPtr.cast<T>();
}

template<typename T>
template<typename T2>
SharedPtr<T>::SharedPtr(UniquePtr<T2>&& subclassPtr) noexcept
{
	static_assert(std::is_base_of<T,T2>::value || std::is_same<T,T2>::value, "T2 is not a subclass of T");
	if (subclassPtr == nullptr) return;

	mState = sfzNew<detail::SharedPtrState>(subclassPtr.allocator());
	mState->allocator = subclassPtr.allocator();
	mState->refCount = 1;
	mPtr = static_cast<T*>(subclassPtr.take());
}

template<typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other) noexcept
{
	*this = other;
}

template<typename T>
SharedPtr<T>& SharedPtr<T>::operator= (const SharedPtr& other) noexcept
{
	// Don't copy to same SharedPointer
	if (this == &other) return *this;
	
	// Destroy whatevers currently in this pointer
	this->destroy();
	
	// If other is nullptr we are done
	if (other == nullptr) return *this;

	// Increment ref counter
	other.mState->refCount += 1;
	
	// Copy pointer and state
	this->mPtr = other.mPtr;
	this->mState = other.mState;

	return *this;
}

template<typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) noexcept
{
	this->swap(other);
}

template<typename T>
SharedPtr<T>& SharedPtr<T>::operator= (SharedPtr&& other) noexcept
{
	this->swap(other);
	return *this;
}

template<typename T>
SharedPtr<T>::~SharedPtr() noexcept
{
	this->destroy();
}

// SharedPtr (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename T>
void SharedPtr<T>::swap(SharedPtr& other) noexcept
{
	T* thisPtr = this->mPtr;
	this->mPtr = other.mPtr;
	other.mPtr = thisPtr;

	detail::SharedPtrState* thisState = this->mState;
	this->mState = other.mState;
	other.mState = thisState;
}

template<typename T>
void SharedPtr<T>::destroy() noexcept
{
	if (mPtr == nullptr) return;
	uint32_t count = mState->refCount.fetch_sub(1) - 1;
	if (count == 0) {
		sfzDelete(mPtr, mState->allocator);
		sfzDelete(mState, mState->allocator);
	}
	mPtr = nullptr;
	mState = nullptr;
}

template<typename T>
Allocator* SharedPtr<T>::allocator() const noexcept
{
	if (mState == nullptr) return nullptr;
	return mState->allocator;
}

template<typename T>
size_t SharedPtr<T>::refCount() const noexcept
{
	if (mState == nullptr) return 0;
	return mState->refCount;
}

template<typename T>
template<typename T2>
SharedPtr<T2> SharedPtr<T>::cast() const noexcept
{
	if (*this == nullptr) return nullptr;

	// Increment ref counter
	mState->refCount += 1;;

	// Cast pointer
	SharedPtr<T2> tmp;
	tmp.dangerousSetState(static_cast<T2*>(this->mPtr), this->mState);
	return tmp;
}

template<typename T>
void SharedPtr<T>::dangerousSetState(T* ptr, detail::SharedPtrState* state) noexcept
{
	this->mPtr = ptr;
	this->mState = state;
}

// SharedPtr (implementation): Operators
// ------------------------------------------------------------------------------------------------

template<typename T>
bool SharedPtr<T>::operator== (const SharedPtr& other) const noexcept
{
	return this->mPtr == other.mPtr;
}

template<typename T>
bool SharedPtr<T>::operator!= (const SharedPtr& other) const noexcept
{
	return !(*this == other);
}

// SharedPtr (implementation): Free operators
// ------------------------------------------------------------------------------------------------

template<typename T>
bool operator== (const SharedPtr<T>& lhs, std::nullptr_t) noexcept
{
	return lhs.get() == nullptr;
}

template<typename T>
bool operator== (std::nullptr_t, const SharedPtr<T>& rhs) noexcept
{
	return rhs.get() == nullptr;
}

template<typename T>
bool operator!= (const SharedPtr<T>& lhs, std::nullptr_t) noexcept
{
	return lhs.get() != nullptr;
}

template<typename T>
bool operator!= (std::nullptr_t, const SharedPtr<T>& rhs) noexcept
{
	return rhs.get() != nullptr;
}

// SharedPtr (implementation): makeShared()
// ------------------------------------------------------------------------------------------------

template<typename T, typename... Args>
SharedPtr<T> makeShared(Allocator* allocator, Args&&... args) noexcept
{
	return SharedPtr<T>(sfzNew<T>(allocator, std::forward<Args>(args)...), allocator);
}

template<typename T, typename... Args>
SharedPtr<T> makeSharedDefault(Args&&... args) noexcept
{
	return makeShared<T>(getDefaultAllocator(), std::forward<Args>(args)...);
}

} // namespace sfz