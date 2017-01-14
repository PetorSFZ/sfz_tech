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

namespace sfz {

using std::uint64_t;

// sfzCore Allocator Interface
// ------------------------------------------------------------------------------------------------

/// The base interface for an sfzCore allocator
///
/// Allocators are used for everything in sfzCore that allocates memory, such as containers. There
/// are two main differences compared to STL allocators:
/// 1, An Allocator does not construct objects, only allocate/deallocate memory. I.e. more similar
/// to malloc() and free() than operator new and delete.
/// 2, sfzCore Allocators are instance based and does not use templates. This means that the
/// allocator itself can be decided at runtime and does not need to be part of the type of a
/// container.
///
/// sfzCore allocators are mainly inspired by "Towards a Better Allocator Model" and EASTL
/// (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1850.pdf and
/// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html#std_allocator respectively).
/// There are, however, a number of differences compared to both these approaches.
///
/// For containers (and other classes) that uses allocators, the following rules should be
/// followed:
/// * Allocators are not part of the type.
/// * Allocators are not owned by the class instance, only a simple pointer (Allocator*) should be
///   kept.
/// * Classes should not create or destroy allocators, they should be supplied an Allocator pointer
///   upon creation. A class might use the Allocator supplied by getDefaultAllocator() if no 
///   Allocator is provided upon creation, or optionally simply require that an Allocator is
///   explicitly provided.
/// * When a class instance is copied, the allocator (pointer) is also copied (in contrast to
///   TaBAM above where allocators are not copied).
/// * When a class instance is moved, the allocator (pointer) is also moved.
/// * A container utilizing allocators is recommended to have a copy constructor that also takes
///   an explicit additional allocator parameter (WITHOUT default value getDefaultAllocator()).
///   I.e. for vector: vector(const vector& other, Allocator* allocator). This copy constructor
///   should copy the contents but use the specified Allocator for the copy instead of the one
///   used in the original instance.
/// * Equality operators (==, !=) should ignore the Allocator pointer, i.e. two string instances
///   with different allocators can still be equal.
/// * Classes that uses Allocator should provide a getter to the pointer, a setter should however
///   only be provided if it can be guaranteed safe.
/// * A feature of TaBAM is that "children" inherit allocators. Imagine vector<vector<int>>,
///   the inner vector class would use the same allocator as the outer vector. This is not the
///   case with sfzCore Allocators. If the same allocator should be used for all inner vectors
///   it must be supplied to each and every one of them upon creation.
/// * It is up to the creator of the Allocator instance to ensure that there no longer exists
///   any pointers to the instance before it is destroyed. In practice this can probably be
///   pretty hard except for small contained problems, so once an Allocator is instantiated it
///   will likely have to be kept alive for the rest of the program's lifetime.
/// 
/// All virtual methods are marked noexcept, meaning an allocator may never throw exceptions. It
/// may, however, during really exceptional circumstances terminate the program.
class Allocator {
public:
	/// Allocates memory with the specified byte alignment
	/// \param size the number of bytes to allocate
	/// \param alignment the byte alignment of the allocation
	/// \param name the name of the allocation (optional for both caller and implementation)
	/// \return pointer to allocated memory, nullptr if allocation failed
	virtual void* allocate(uint64_t size, uint64_t alignment = 32,
	                       const char* name = "???") noexcept = 0;

	/// Deallocates memory previously allocated with this allocator instance. Deallocating memory
	/// allocated by other Allocator (instance or even implementation) is only possible if the
	/// allocator and deallocator are compatible(), otherwise it will likely result in hard to
	/// debug catastrophic failure.
	/// Attempting to deallocate nullptr is safe and will result in no change
	/// \param pointer to the memory
	virtual void deallocate(void* pointer) noexcept = 0;

	/// Returns the name of this Allocator. Having a name is optional and how it is used is
	/// completely up to the implementation. There are however some general implementation
	/// suggestions:
	/// * Have unique names per instance (even of same type)
	/// * Specify name in constructor
	/// * Never change the name after it has been set
	/// * Keep the name short (<32 chars)
	virtual const char* getName() const noexcept { return "sfzCore Allocator"; }

	virtual ~Allocator() noexcept {}
};

// Default allocator
// ------------------------------------------------------------------------------------------------

/// Returns pointer to the default Allocator.
Allocator* getDefaultAllocator() noexcept;

/// Returns the number of times getDefaultAllocator() has been called since the start of the
/// program.
uint64_t getDefaultAllocatorNumTimesRetrieved() noexcept;

/// Sets the default Allocator to a user-provided one. Must be the first thing the program does.
/// Will terminate the program if getDefaultAllocatorNumTimesRetrieved() does not return 0.
/// After this function is called getDefaultAllocator() will return the user-provided one instead
/// of the original one, so the user is responsible for keeping the new Allocator alive for the
/// rest of the program's lifetime.
void setDefaultAllocator(Allocator* allocator) noexcept;

} // namespace sfz
