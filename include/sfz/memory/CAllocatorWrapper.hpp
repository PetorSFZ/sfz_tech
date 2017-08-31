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

#include "sfz/memory/Allocator.hpp"
#include "sfz/memory/CAllocator.h"

namespace sfz {

// CAllocatorWrapper
// ------------------------------------------------------------------------------------------------

/// The existence of this class will likely feel paradoxial. sfzAllocator is a C-API wrapper of
/// sfz::Allocator, needed for DLLs with C-style APIs. Fine, that makes sense. But why is a
/// sfz::Allocator (C++) wrapper around sfzAllocator necessary or desirable? This is because the
/// DLL itself might be written in C++ even though it uses a C-style API. And since most code that
/// uses sfz::Allocator only accepts the C++ variant (such as sfz::DynArray and sfz::HashMap), the
/// raw sfzAllocator instance will be painful to use. Thus this wrapper. To quote the fundamental
/// theorem of software engineering: "We can solve any problem by introducing an extra level of
/// indirection."
class CAllocatorWrapper final : public Allocator {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	CAllocatorWrapper() = delete;
	CAllocatorWrapper(const CAllocatorWrapper&) = delete;
	CAllocatorWrapper& operator= (const CAllocatorWrapper&) = delete;
	CAllocatorWrapper(CAllocatorWrapper&&) = delete;
	CAllocatorWrapper& operator= (CAllocatorWrapper&&) = delete;

	CAllocatorWrapper(sfzAllocator* cAlloc) noexcept : cAlloc(cAlloc) {}
	~CAllocatorWrapper() noexcept override final;

	// Overriden Allocator methods
	// --------------------------------------------------------------------------------------------

	void* allocate(uint64_t size, uint64_t alignment, const char* name) noexcept override final
	{
		return SFZ_C_ALLOCATE(cAlloc, size, alignment, name);
	}
	void deallocate(void* pointer) noexcept override final
	{
		return SFZ_C_DEALLOCATE(cAlloc, pointer);
	}
	const char* getName() const noexcept override final
	{
		return SFZ_C_GET_NAME(cAlloc);
	}
	sfzAllocator* cAllocator() noexcept override final
	{
		return cAlloc;
	}

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	sfzAllocator* cAlloc;
};

} // namespace sfz
