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

#ifndef SFZ_DEFER_PTR_HPP
#define SFZ_DEFER_PTR_HPP
#pragma once

#include "sfz.h"
#include "sfz_cpp.hpp"

// SfzDeferPtr
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

template<typename T>
using SfzDeferPtrDestroyFunc = void(T*);

// Simple replacement for std::unique_ptr using SfzAllocator
template<typename T>
class SfzDeferPtr final {
public:
	SFZ_DECLARE_DROP_TYPE(SfzDeferPtr);

	SfzDeferPtr(std::nullptr_t) noexcept {};
	SfzDeferPtr(T* object, SfzDeferPtrDestroyFunc<T>* destroy_func) { this->init(object, destroy_func); }

	void init(T* object, SfzDeferPtrDestroyFunc<T>* destroy_func)
	{
		sfz_assert(object != nullptr);
		sfz_assert(destroy_func != nullptr);
		this->destroy();
		m_ptr = object;
		m_destroy_func = destroy_func;
	}

	void destroy() noexcept
	{
		if (m_ptr == nullptr) return;
		m_destroy_func(m_ptr);
		m_ptr = nullptr;
		m_destroy_func = nullptr;
	}

	T* get() const { return m_ptr; }
	
	// Caller takes ownership of the internal pointer
	T* take() noexcept
	{
		T* tmp = m_ptr;
		m_ptr = nullptr;
		m_destroy_func = nullptr;
		return tmp;
	}

	operator T*() { return m_ptr; }
	operator const T*() const { return m_ptr; }

	T& operator* () const { return *m_ptr; }
	T* operator-> () const { return m_ptr; }

	bool operator== (const SfzDeferPtr& other) const { return this->m_ptr == other.m_ptr; }
	bool operator!= (const SfzDeferPtr& other) const { return !(*this == other); }

	bool operator== (std::nullptr_t) const { return this->m_ptr == nullptr; }
	bool operator!= (std::nullptr_t) const { return this->m_ptr != nullptr; }

private:
	T* m_ptr = nullptr;
	SfzDeferPtrDestroyFunc<T>* m_destroy_func = nullptr;
};

#endif // __cplusplus
#endif // SFZ_DEFER_PTR_HPP
