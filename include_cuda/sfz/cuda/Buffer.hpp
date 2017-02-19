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

#include <algorithm>
#include <cstdint>
#include <type_traits>

#include <cuda_runtime.h>

#include "sfz/Assert.hpp"
#include "sfz/containers/DynArray.hpp"
#include "sfz/cuda/CudaUtils.hpp"

namespace sfz {

namespace cuda {

using std::uint32_t;
using std::uint64_t;

// Buffer
// ------------------------------------------------------------------------------------------------

/// A Buffer holding CUDA allocated memory on the GPU.
///
/// A Buffer may only hold trivial types as it does not call any constructors or destructors of any
/// kind. In contrast to DynArray, it has no concept of "size". If memory is allocated for an
/// element the element exists.
template<typename T>
class Buffer final {
public:
	static_assert(std::is_trivial<T>::value, "T is not a trivial type");

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	/// Default constructor creates an empty Buffer
	Buffer() noexcept = default;

	/// Copying constructors disabled, use copyTo() if you need to copy a Buffer.
	Buffer(const Buffer&) = delete;
	Buffer& operator= (const Buffer&) = delete;

	/// Creates a Buffer with capacity elements on the GPU
	explicit Buffer(uint32_t capacity) noexcept;

	/// Creates a Buffer and uploads elements from the DynArray to it. Capacity of the Buffer will
	/// be dynArray.size() elements, not same capacity as the DynArray.
	explicit Buffer(const DynArray<T>& dynArray) noexcept;

	/// Creates a Buffer with numElements capacity, numElements will be uploaded from the dataPtr
	Buffer(const T* dataPtr, uint32_t numElements) noexcept;
	
	/// Move constructors. Equivalent to using swap().
	Buffer(Buffer&& other) noexcept;
	Buffer& operator= (Buffer&& other) noexcept;

	/// Destroys this Buffer using destroy().
	~Buffer() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	/// Creates a buffer with capacity elements of capacity. If memory is already allocated it will
	/// first be destroyed using destroy().
	void create(uint32_t capacity) noexcept;

	/// Destroys this Buffer and deallocates all memory. After this method is called the Buffer
	/// will be in an identical state to a default constructed empty one. Not necessary to call
	/// manually, will be called by destructor.
	void destroy() noexcept;

	/// Swaps this Buffer with the specified one.
	void swap(Buffer& other) noexcept;

	/// Uploads numElements elements from srcPtr to Buffer, starting at dstLocation in Buffer.
	void upload(const T* srcPtr, uint32_t dstLocation, uint32_t numElements) noexcept;

	/// Uploads src.size() elelments from src to Buffer
	void upload(const DynArray<T>& src) noexcept;

	/// Downloads numElements elements from Buffer into dstPtr. Starting at srcLocation in Buffer.
	void download(T* dstPtr, uint32_t srcLocation, uint32_t numElements) noexcept;

	/// Downloads Buffer.capacity() elements from Buffer into dstPtr.
	void download(T* dstPtr) noexcept;

	/// Downloads Buffer.capacity() elements from Buffer into dst.
	void download(DynArray<T>& dst) noexcept;

	/// Uploads element to dstLocation in Buffer.
	void uploadElement(const T& element, uint32_t dstLocation) noexcept;

	/// Downloads element from srcLocation in Buffer.
	void downloadElement(T& element, uint32_t srcLocation) noexcept;
	T downloadElement(uint32_t dstLocation) noexcept;

	/// Copies numElements elements from this Buffer to dstBuffer, starting at location srcLocation
	/// in this Buffer and at dstLocation in dstBuffer.
	void copyTo(Buffer& dstBuffer, uint32_t dstLocation, uint32_t srcLocation,
	            uint32_t numElements) noexcept;

	/// Copies numElements from this Buffer to dstBuffer, starting at location 0 in both.
	void copyTo(Buffer& dstBuffer, uint32_t numElements) noexcept;

	/// Copies all elements from this Buffer to dstBuffer, starting at location 0.
	void copyTo(Buffer& dstBuffer) noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	/// Returns pointer to CUDA allocated memory. Should not be dereferenced on CPU.
	inline T* data() noexcept { return mDataPtr; }
	inline const T* data() const noexcept { return mDataPtr; }

	/// Returns capacity of this Buffer.
	inline uint32_t capacity() const noexcept { return mCapacity; }
	
private:
	// Private members
	// --------------------------------------------------------------------------------------------

	T* mDataPtr = nullptr;
	uint32_t mCapacity = 0u;
};

// Buffer implementation: Constructors & destructors
// ------------------------------------------------------------------------------------------------

template<typename T>
Buffer<T>::Buffer(uint32_t capacity) noexcept
{
	this->create(capacity);
}

template<typename T>
Buffer<T>::Buffer(const DynArray<T>& dynArray) noexcept
{
	this->create(dynArray.size());
	this->upload(dynArray.data(), 0u, dynArray.size());
}

template<typename T>
Buffer<T>::Buffer(const T* srcPtr, uint32_t numElements) noexcept
{
	this->create(numElements);
	this->upload(srcPtr, 0u, numElements);
}

template<typename T>
Buffer<T>::Buffer(Buffer&& other) noexcept
{
	this->swap(other);
}

template<typename T>
Buffer<T>& Buffer<T>::operator= (Buffer&& other) noexcept
{
	this->swap(other);
	return *this;
}

template<typename T>
Buffer<T>::~Buffer() noexcept
{
	this->destroy();
}

// Buffer implementation: Methods
// ------------------------------------------------------------------------------------------------

template<typename T>
void Buffer<T>::create(uint32_t capacity) noexcept
{
	if (mCapacity != 0u) this->destroy();
	uint64_t numBytes = capacity * sizeof(T);
	CHECK_CUDA_ERROR(cudaMalloc(&mDataPtr, numBytes));
	mCapacity = capacity;
}

template<typename T>
void Buffer<T>::destroy() noexcept
{
	CHECK_CUDA_ERROR(cudaFree(mDataPtr));
	mDataPtr = nullptr;
	mCapacity = 0u;
}

template<typename T>
void Buffer<T>::swap(Buffer& other) noexcept
{
	std::swap(this->mDataPtr, other.mDataPtr);
	std::swap(this->mCapacity, other.mCapacity);
}

template<typename T>
void Buffer<T>::upload(const T* dataPtr, uint32_t dstLocation, uint32_t numElements) noexcept
{
	sfz_assert_debug((dstLocation + numElements) <= mCapacity);
	uint64_t numBytes = numElements * sizeof(T);
	CHECK_CUDA_ERROR(cudaMemcpy(mDataPtr + dstLocation, dataPtr, numBytes, cudaMemcpyHostToDevice));
}

template<typename T>
void Buffer<T>::upload(const DynArray<T>& src) noexcept
{
	this->upload(src.data(), 0u, src.size());
}

template<typename T>
void Buffer<T>::download(T* dstPtr, uint32_t srcLocation, uint32_t numElements) noexcept
{
	sfz_assert_debug((srcLocation + numElements) <= mCapacity);
	uint64_t numBytes = numElements * sizeof(T);
	CHECK_CUDA_ERROR(cudaMemcpy(dstPtr, mDataPtr + srcLocation, numBytes, cudaMemcpyDeviceToHost));
}

template<typename T>
void Buffer<T>::download(T* dstPtr) noexcept
{
	this->download(dstPtr, 0u, mCapacity);
}

template<typename T>
void Buffer<T>::download(DynArray<T>& dst) noexcept
{
	dst.ensureCapacity(mCapacity);
	dst.clear();
	this->download(dst.data(), 0u, mCapacity);
	dst.setSize(mCapacity);
}

template<typename T>
void Buffer<T>::uploadElement(const T& element, uint32_t dstLocation) noexcept
{
	this->upload(&element, dstLocation, 1u);
}

template<typename T>
void Buffer<T>::downloadElement(T& element, uint32_t srcLocation) noexcept
{
	this->download(&element, srcLocation, 1u);
}

template<typename T>
T Buffer<T>::downloadElement(uint32_t dstLocation) noexcept
{
	T tmp;
	this->downloadElement(tmp, dstLocation);
	return tmp;
}

template<typename T>
void Buffer<T>::copyTo(Buffer& dstBuffer, uint32_t dstLocation, uint32_t srcLocation,
                           uint32_t numElements) noexcept
{
	sfz_assert_debug(dstBuffer.capacity() >= (dstLocation + numElements));
	uint64_t numBytes = numElements * sizeof(T);
	CHECK_CUDA_ERROR(cudaMemcpy(dstBuffer.mDataPtr + dstLocation, this->mDataPtr + srcLocation,
	                 numBytes, cudaMemcpyDeviceToDevice));
}

template<typename T>
void Buffer<T>::copyTo(Buffer& dstBuffer, uint32_t numElements) noexcept
{
	this->copyTo(dstBuffer, 0u, 0u, numElements);
}

template<typename T>
void Buffer<T>::copyTo(Buffer& dstBuffer) noexcept
{
	this->copyTo(dstBuffer, mCapacity);
}

} // namespace cuda
} // namespace sfz
