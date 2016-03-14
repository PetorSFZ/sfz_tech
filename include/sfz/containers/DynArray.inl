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

// DynArray (implementation): Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
DynArray<T, Allocator>::DynArray() noexcept
:
	mSize{0},
	mCapacity{0},
	mDataPtr{nullptr}
{ }

template<typename T, typename Allocator>
DynArray<T, Allocator>::DynArray(uint32_t size) noexcept
:
	mSize{size},
	mCapacity{size},
	mDataPtr{nullptr}
{
	if (size == 0) return;

	// Allocate memory
	mDataPtr = static_cast<T*>(Allocator::allocate(mSize * sizeof(T), 32));
	// TODO: Handle error case where allocation failed
	
	// Calling constructor for each element if not trivially default constructible
	if (!std::is_trivially_default_constructible<T>::value) {
		for (uint32_t i = 0; i < mSize; ++i) {
			new (mDataPtr + i) T();
		}
	}
}

template<typename T, typename Allocator>
DynArray<T, Allocator>::DynArray(uint32_t size, const T& value) noexcept
:
	mSize{size},
	mCapacity{size},
	mDataPtr{nullptr}
{
	if (size == 0) return;

	// Allocate memory
	mDataPtr = static_cast<T*>(Allocator::allocate(mSize * sizeof(T), 32));
	// TODO: Handle error case where allocation failed
	
	// Calling constructor for each element
	for (uint32_t i = 0; i < mSize; ++i) {
		new (mDataPtr + i) T(value);
	}
}

template<typename T, typename Allocator>
DynArray<T, Allocator>::~DynArray() noexcept
{
	this->destroy();
}

// DynArray (implementation): Public methods
// ------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
void DynArray<T, Allocator>::destroy() noexcept
{
	if (mDataPtr == nullptr) return;

	// Call destructor for each element if not trivially destructible
	if (!std::is_trivially_destructible<T>::value) {
		for (uint32_t i = 0; i < mSize; ++i) {
			mDataPtr[i].~T();
		}
	}

	// Deallocates memory
	Allocator::deallocate(mDataPtr);

	// Sets all members to indicate the now empty state
	mSize = 0;
	mCapacity = 0;
	mDataPtr = nullptr;
}

} // namespace sfz
