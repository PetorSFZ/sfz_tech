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

namespace sfz {

// Component Mask
// ------------------------------------------------------------------------------------------------

// A 64-bit mask specifying which components an entity have.
//
// Not all bits need to have associated component data, some can be used as pure data-less flag.
// One such data-less flag is the first bit (0:th), which just indicates if the given entity exists
// or not.
struct ComponentMask final {

	// Members
	// --------------------------------------------------------------------------------------------

	// The raw 64-bit mask.
	uint64_t rawMask;

	// Constructor methods
	// --------------------------------------------------------------------------------------------

	static constexpr ComponentMask fromRawValue(uint64_t bits) noexcept { return { bits }; }
	static constexpr ComponentMask empty() noexcept { return ComponentMask::fromRawValue(0); }
	static constexpr ComponentMask fromType(uint32_t componentType) noexcept
	{
		return ComponentMask::fromRawValue(uint64_t(1) << uint64_t(componentType));
	}
	static constexpr ComponentMask activeMask() noexcept
	{
		return ComponentMask::fromRawValue(1);
	}

	// Operators
	// --------------------------------------------------------------------------------------------

	constexpr bool operator== (const ComponentMask& o) const noexcept { return rawMask == o.rawMask; }
	constexpr bool operator!= (const ComponentMask& o) const noexcept { return rawMask != o.rawMask; }
	constexpr ComponentMask operator& (const ComponentMask& o) const noexcept { return { rawMask & o.rawMask }; }
	constexpr ComponentMask operator| (const ComponentMask& o) const noexcept { return { rawMask | o.rawMask }; }
	constexpr ComponentMask operator~ () const noexcept { return { ~rawMask }; }

	// Methods
	// --------------------------------------------------------------------------------------------

	// Checks whether this mask contains the specified component type or not, somewhat slow.
	// Prefer to build a mask with all bits you want to check, then you fulfills() with it instead.
	constexpr bool hasComponentType(uint32_t componentType) const noexcept
	{
		return this->fulfills(ComponentMask::fromType(componentType));
	}

	// Sets the specified bit of this mask to the specified value.
	constexpr void setComponentType(uint32_t componentType, bool value) noexcept
	{
		if (value) this->rawMask |= ComponentMask::fromType(componentType).rawMask;
		else this->rawMask &= ~ComponentMask::fromType(componentType).rawMask;
	}

	// Checks whether this mask has all the components in the specified parameter mask
	constexpr bool fulfills(const ComponentMask& constraints) const noexcept
	{
		return (this->rawMask & constraints.rawMask) == constraints.rawMask;
	}

	// Checks whether the entity associated with this mask is active or not (i.e. whether the 0:th
	// bit is set or not)
	constexpr bool active() const noexcept { return this->fulfills(ComponentMask::activeMask()); }
};
static_assert(sizeof(ComponentMask) == 8, "ComponentMask is padded");

} // namespace sfz
