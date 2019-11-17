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
#include <new> // placement new
#include <utility> // std::move, std::forward, std::swap

namespace sfz {

// Debug information
// ------------------------------------------------------------------------------------------------

// Tiny struct that contains debug information, i.e. file, line number and a message.
// Note that the message MUST be a compile-time constant, it may NOT be dynamically allocated.
struct DbgInfo final {
	const char* staticMsg = ""; // MUST be a compile-time constant, pointer must always be valid.
	const char* file = "";
	uint32_t line = 0;
	DbgInfo() noexcept = default;
	DbgInfo(const char* staticMsg, const char* file, uint32_t line) noexcept :
		staticMsg(staticMsg), file(file), line(line) {}
};

// Tiny macro that creates a DbgInfo struct with current file and line number. Message must be a
// compile time constant, i.e.string must be valid for the remaining duration of the program.
#define sfz_dbg(staticMsg) sfz::DbgInfo(staticMsg, __FILE__, __LINE__)

// Allocator Interface
// ------------------------------------------------------------------------------------------------

// The Allocator interface.
//
// * Allocators are instance based and can therefore be decided at runtime.
// * Typically classes should not own or create allocators, only keep simple pointers (Allocator*).
// * Typically allocator pointers should be moved/copied when a class is moved/copied.
// * Typically equality operators (==, !=) should ignore allocator pointers.
// * It is the responsibility of the creator of the allocator instance to ensure that all users
//   that have been provided a pointer have freed all their memory and are done using the allocator
//   before the allocator itself is removed. Often this means that an allocator need to be kept
//   alive for the remaining lifetime of the program.
// * All virtual methods are marked noexcept, meaning an allocator may never throw exceptions.
class Allocator {
public:

	virtual ~Allocator() noexcept {}

	// Allocates memory with the specified byte alignment, returns nullptr on failure.
	virtual void* allocate(
		DbgInfo dbg, uint64_t size, uint64_t alignment = 32) noexcept = 0;

	// Deallocates memory previously allocated with this instance.
	//
	// Deallocating nullptr is required to be a no-op. Deallocating pointers not allocated by this
	// instance is undefined behavior, and may result in catastrophic failure.
	virtual void deallocate(void* pointer) noexcept = 0;

	// Constructs a new object of type T, similar to operator new. Guarantees 32-byte alignment.
	template<typename T, typename... Args>
	T* newObject(DbgInfo dbg, Args&&... args) noexcept
	{
		// Allocate memory (minimum 32-byte alignment), return nullptr on failure
		void* memPtr = this->allocate(dbg, sizeof(T), alignof(T) < 32 ? 32 : alignof(T));
		if (memPtr == nullptr) return nullptr;

		// Creates object (placement new), terminates program if constructor throws exception.
		return new(memPtr) T(std::forward<Args>(args)...);
	}

	// Deletes an object created with this allocator, similar to operator delete.
	template<typename T>
	void deleteObject(T*& pointer) noexcept
	{
		if (pointer == nullptr) return;
		pointer->~T(); // Call destructor, will terminate program if it throws exception.
		this->deallocate(pointer);
		pointer = nullptr; // Set callers pointer to nullptr, an attempt to avoid dangling pointers.
	}
};

} // namespace sfz
