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

#include <cstdint>
#include <mutex>
#include <utility> // std::swap

#include "ZeroG/util/Assert.hpp"

namespace zg {

// Mutex template
// ------------------------------------------------------------------------------------------------

// Forward declare data accessor
template<typename T>
class MutexDataAccessor;

// A simple wrapper around std::mutex which attempts to make it clear exactly what the mutex is
// protecting. A bit similar to Rust's mutex.
template<typename T>
class Mutex final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Mutex() noexcept = default;
	Mutex(const Mutex&) = delete;
	Mutex& operator= (const Mutex&) = delete;
	Mutex(Mutex&&) = delete;
	Mutex& operator= (Mutex&&) = delete;
	~Mutex() noexcept = default;

	Mutex(const T& defaultValue) noexcept : mData(defaultValue) { }
	Mutex(T&& defaultValue) noexcept : mData(std::move(defaultValue)) { }

	// Methods
	// --------------------------------------------------------------------------------------------

	MutexDataAccessor<T> access() noexcept
	{
		return MutexDataAccessor<T>::accessMutexedData(&mMutex, &mData);
	}

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	std::mutex mMutex;
	T mData;
};

// Mutex data accessor template
// ------------------------------------------------------------------------------------------------

// An accessor for data protected by a zg::Mutex.
//
// Essentially calls lock() on creation and unlock() on destruction. Meaning there can only really
// be one accessor existing for a given mutex at any given time.
template<typename T>
class MutexDataAccessor final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	MutexDataAccessor() noexcept = default;
	MutexDataAccessor(const MutexDataAccessor&) = delete;
	MutexDataAccessor& operator= (const MutexDataAccessor&) = delete;
	MutexDataAccessor(MutexDataAccessor&& o) noexcept { this->swap(o); }
	MutexDataAccessor& operator= (MutexDataAccessor& o) noexcept { this->swap(o); return *this; }
	~MutexDataAccessor() noexcept { this->destroy(); }

	static MutexDataAccessor accessMutexedData(std::mutex* mutex, T* data) noexcept
	{
		MutexDataAccessor accessor;
		accessor.mMutex = mutex;
		accessor.mData = data;
		mutex->lock();
		return accessor;
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(MutexDataAccessor& other) noexcept
	{
		std::swap(this->mMutex, other.mMutex);
		std::swap(this->mData, other.mData);
	}

	void destroy() noexcept
	{
		if (mMutex != nullptr) {
			mMutex->unlock();
		}
		mMutex = nullptr;
		mData = nullptr;
	}

	// Data accessors
	// --------------------------------------------------------------------------------------------

	T& data() noexcept { return *mData; }
	const T& data() const noexcept { return *mData; }

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	std::mutex* mMutex = nullptr;
	T* mData = nullptr;
};

} // namespace zg
