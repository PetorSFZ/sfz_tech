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

#ifndef SFZ_CPP_H
#define SFZ_CPP_H
#pragma once

#include "sfz.h"

#ifdef __cplusplus

// std::move() and std::forward() replacements
// ------------------------------------------------------------------------------------------------

// std::move() and std::forward() requires including a relatively expensive header (<utility>), and
// are fairly complex in their standard implementations. This is a significantly cheaper
// implementation that works for many use-cases.
//
// See: https://www.foonathan.net/2020/09/move-forward/

// Implementation of std::remove_reference_t
// See: https://en.cppreference.com/w/cpp/types/remove_reference
template<typename T> struct sfz_remove_ref { typedef T type; };
template<typename T> struct sfz_remove_ref<T&> { typedef T type; };
template<typename T> struct sfz_remove_ref<T&&> { typedef T type; };
template<typename T> using sfz_remove_ref_t = typename sfz_remove_ref<T>::type;

#define sfz_move(obj) static_cast<sfz_remove_ref_t<decltype(obj)>&&>(obj)
#define sfz_forward(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

// Placement new
// ------------------------------------------------------------------------------------------------

// In C++ you not allowed to use placement new unless you include <new>. Unfortunately, <new> is a
// big and expensive header. Here is a hacky per compiler/platform forward declaration of placement
// new.

#if defined(_MSC_VER)

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void* operator new(u64, void* ptr) noexcept { return ptr; }
inline void operator delete(void*, void*) noexcept { }
#endif

#else
#error "Not implemented for this compiler
#endif

// "new" and "delete" functions using sfz allocators
// ------------------------------------------------------------------------------------------------

// Constructs a new object of type T, similar to operator new. Guarantees 32-byte alignment.
template<typename T, typename... Args>
T* sfz_new(SfzAllocator* allocator, SfzDbgInfo dbg, Args&&... args) noexcept
{
	// Allocate memory (minimum 32-byte alignment), return nullptr on failure
	void* mem_ptr = allocator->alloc(dbg, sizeof(T), alignof(T) < 32 ? 32 : alignof(T));
	if (mem_ptr == nullptr) return nullptr;

	// Creates object (placement new), terminates program if constructor throws exception.
	return new(mem_ptr) T(sfz_forward(args)...);
}

// Deconstructs a C++ object created using sfz_new.
template<typename T>
void sfz_delete(SfzAllocator* allocator, T*& pointer) noexcept
{
	if (pointer == nullptr) return;
	pointer->~T(); // Call destructor, will terminate program if it throws exception.
	allocator->dealloc(pointer);
	pointer = nullptr; // Set callers pointer to nullptr, an attempt to avoid dangling pointers.
}

// std::swap replacement
// ------------------------------------------------------------------------------------------------

// std::swap() is similarly to std::move() and std::forward() hidden in relatively expensive
// headers (<utility>), so we have our own simple implementation instead.

template<typename T>
void sfzSwap(T& lhs, T& rhs)
{
	T tmp = sfz_move(lhs);
	lhs = sfz_move(rhs);
	rhs = sfz_move(tmp);
}

// memcpy() and memswp()
// ------------------------------------------------------------------------------------------------

// Swaps size bytes of memory between two buffers. Undefined behaviour if the buffers overlap, with
// the exception that its safe to call if both pointers are the same (i.e. point to the same buffer).
sfz_constexpr_func void sfz_memswp(void* __restrict a, void* __restrict b, u64 size)
{
	u8* __restrict a_bytes = (u8* __restrict)a;
	u8* __restrict b_bytes = (u8* __restrict)b;

	const u64 MEMSWP_TMP_BUFFER_SIZE = 64;
	u8 tmpBuffer[MEMSWP_TMP_BUFFER_SIZE] = {};

	// Swap buffers in temp buffer sized chunks
	u64 bytesLeft = size;
	while (bytesLeft >= MEMSWP_TMP_BUFFER_SIZE) {
		memcpy(tmpBuffer, a_bytes, MEMSWP_TMP_BUFFER_SIZE);
		memcpy(a_bytes, b_bytes, MEMSWP_TMP_BUFFER_SIZE);
		memcpy(b_bytes, tmpBuffer, MEMSWP_TMP_BUFFER_SIZE);
		a_bytes += MEMSWP_TMP_BUFFER_SIZE;
		b_bytes += MEMSWP_TMP_BUFFER_SIZE;
		bytesLeft -= MEMSWP_TMP_BUFFER_SIZE;
	}

	// Swap remaining bytes
	if (bytesLeft > 0) {
		memcpy(tmpBuffer, a_bytes, bytesLeft);
		memcpy(a_bytes, b_bytes, bytesLeft);
		memcpy(b_bytes, tmpBuffer, bytesLeft);
	}
}

// DropType
// ------------------------------------------------------------------------------------------------

// A DropType is a type that is default constructible and move:able, but not copy:able.
//
// It must implement "void destroy()", which must destroy all members and reset the state of the
// type to the same state as if it was default constructed. It should be safe to call destroy()
// multiple times in a row.
//
// The move semantics of DropType's are implemented using sfz_memswp(), which means that all
// members of a DropType MUST be either trivially copyable (i.e. primitives such as integers,
// floats and pointers) or DropTypes themselves. This is currently not enforced, be careful.
//
// Usage:
//
// class SomeType final {
// public:
//     SFZ_DECLARE_DROP_TYPE(SomeType);
//     void destroy() { /* ... */ }
//     // ...
// };
#define SFZ_DECLARE_DROP_TYPE(T) \
	T() = default; \
	T(const T&) = delete; \
	T& operator= (const T&) = delete; \
	T(T&& other) noexcept { this->swap(other); } \
	T& operator= (T&& other) noexcept { this->swap(other); return *this; } \
	void swap(T& other) noexcept { sfz_memswp(this, &other, sizeof(T)); } \
	~T() noexcept { this->destroy(); }

// Alternate type definition
// ------------------------------------------------------------------------------------------------

struct SFZ_NO_ALT_TYPE final { SFZ_NO_ALT_TYPE() = delete; };

// Defines an alternate type for a given type. Mainly used to define alternate key types for hash
// maps. E.g., for a string type "const char*" can be defined as an alternate key type.
//
// Requirements of an alternate type:
//  * operator== (T, AltT) must be defined
//  * sfzHash(T) and sfzHash(AltT) must be defined
//  * sfzHash(T) == sfzHash(AltT)
//  * SfzAltType<T>::conv(T) -> AltT must be defined
template<typename T>
struct SfzAltType final {
	using AltT = SFZ_NO_ALT_TYPE;
};

#endif // __cplusplus
#endif // SFZ_CPP_H
