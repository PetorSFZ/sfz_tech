// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include <sfz/Context.hpp>
#include <sfz/memory/Allocator.hpp>

namespace ph {

using sfz::Allocator;

// Forward declarations
// ------------------------------------------------------------------------------------------------

struct NaiveEcsHeader;

// EcsContainer class
// ------------------------------------------------------------------------------------------------

// Smart-pointer-ish class owning the memory blob for a single snap-shot of the ECS system
class EcsContainer final {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	EcsContainer() noexcept = default;
	EcsContainer(const EcsContainer&) = delete;
	EcsContainer& operator= (const EcsContainer&) = delete;
	EcsContainer(EcsContainer&& other) noexcept { this->swap(other); }
	EcsContainer& operator= (EcsContainer&& other) noexcept { this->swap(other); return *this; }
	~EcsContainer() noexcept { this->destroy(); }

	static EcsContainer createRaw(uint64_t numBytes, sfz::Allocator* allocator) noexcept;

	// State methods
	// --------------------------------------------------------------------------------------------

	void cloneTo(EcsContainer& ecs) noexcept;
	EcsContainer clone(Allocator* allocator = sfz::getDefaultAllocator()) noexcept;
	void swap(EcsContainer& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	NaiveEcsHeader* getNaive() noexcept;
	const NaiveEcsHeader* getNaive() const noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	Allocator* mAllocator = nullptr;
	uint8_t* mEcsMemoryChunk = nullptr;
	uint64_t mNumBytes = 0;
};

} // namespace ph
