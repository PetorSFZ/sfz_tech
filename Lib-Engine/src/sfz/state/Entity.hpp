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

#include <skipifzero.hpp>

namespace sfz {

// Entity constants
// ------------------------------------------------------------------------------------------------

constexpr uint32_t ENTITY_ID_NUM_BITS = 24;
constexpr uint32_t ENTITY_ID_MAX = (1 << ENTITY_ID_NUM_BITS) - 1; // 2^24 - 1
constexpr uint32_t ENTITY_ID_PART_MASK = ENTITY_ID_MAX;

constexpr uint32_t ENTITY_GENERATION_NUM_BITS = 32 - ENTITY_ID_NUM_BITS;
constexpr uint32_t ENTITY_GENERATION_MAX = (1 << ENTITY_GENERATION_NUM_BITS) - 1;
constexpr uint32_t ENTITY_GENERATION_PART_MASK = ~ENTITY_ID_PART_MASK;

// Entity
// ------------------------------------------------------------------------------------------------

// An Entity in the ECS system
//
// An entity consists of two components, an ID and a Generation. The ID is central part of the
// entity, it is used to lookup which components are associated with the entity. IDs are reused
// when entities are removed.
//
// The generation is an 8-bit number used to keep track of which generation of a specific ID this
// entity refers to. This is really only used to avoid "dangling pointer entities". Essentially
// the ECS system stores a generation for each possible entity ID. This generation (along with the
// ID) is handed out when a new Entity is created. When an entity is destroyed the ID is returned
// to the pool of free IDs to hand out, but the generation is incremented. By comparing the
// generation of an Entity with the one stored in the ECS system it is possible to check if it is
// the same Entity or if it has been destroyed and the ID reused for another one.
//
// Note that generation 0 is reserved as invalid, meaning an entity with the raw value "0" is
// always considered invalid. For this reason we consider this entity (id 0, gen 0) the "null"
// entity. It is used as an error code or for yet uninitialized entities. Use the constant
// NULL_ENTITY (specified below) for clarity.
struct Entity final {

	// Members
	// --------------------------------------------------------------------------------------------

	uint32_t rawBits = 0;

	// Construtors & destructors
	// --------------------------------------------------------------------------------------------

	Entity() noexcept = default;
	Entity(const Entity&) noexcept = default;
	Entity& operator= (const Entity&) noexcept = default;
	~Entity() noexcept = default;

	static Entity create(uint32_t id, uint8_t generation) noexcept
	{
		sfz_assert(id == (id & ENTITY_ID_PART_MASK));
		Entity tmp;
		tmp.rawBits = (uint32_t(generation) << ENTITY_ID_NUM_BITS) | (id & ENTITY_ID_PART_MASK);
		return tmp;
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	uint32_t id() const noexcept { return rawBits & ENTITY_ID_PART_MASK; }
	uint8_t generation() const noexcept { return uint8_t((rawBits & ENTITY_GENERATION_PART_MASK) >> ENTITY_ID_NUM_BITS); }

	// Operators
	// --------------------------------------------------------------------------------------------

	bool operator== (Entity other) const noexcept { return this->rawBits == other.rawBits; }
	bool operator!= (Entity other) const noexcept { return this->rawBits != other.rawBits; }
};
static_assert(sizeof(Entity) == 4, "Entity is padded");

// A "null" handle typically used as an error type or for uninitialized entities.
constexpr Entity NULL_ENTITY = {};

} // namespace sfz
