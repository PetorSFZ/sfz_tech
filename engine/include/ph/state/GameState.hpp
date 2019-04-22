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

#include <sfz/Assert.hpp>
#include <sfz/Context.hpp>
#include <sfz/memory/Allocator.hpp>

#include "ph/state/ArrayHeader.hpp"
#include "ph/state/ComponentMask.hpp"
#include "ph/state/GameStateContainer.hpp"

namespace ph {

using sfz::Allocator;

// Naive ECS versions
// ------------------------------------------------------------------------------------------------

// Magic number in beginning of all Phantasy Engine game states.
constexpr uint64_t GAME_STATE_MAGIC_NUMBER = 
	uint64_t('P') << 0 | 
	uint64_t('H') << 8 |
	uint64_t('E') << 16 |
	uint64_t('S') << 24 |
	uint64_t('T') << 32 |
	uint64_t('A') << 40 |
	uint64_t('T') << 48 |
	uint64_t('E') << 56;

constexpr uint32_t NAIVE_ECS_VERSION = 1;

// ComponentRegistryEntry struct
// ------------------------------------------------------------------------------------------------

struct ComponentRegistryEntry final {

	// The offset in bytes to the ArrayHeader of components for the specific type, ~0 (UINT32_MAX)
	// if there is no associated data with the given component type.
	uint32_t offset;

	// Returns whether the component type has associated data or not.
	bool componentTypeHasData() const noexcept { return offset != ~0u; }

	static ComponentRegistryEntry createSized(uint32_t offset) noexcept { return { offset }; }
	static ComponentRegistryEntry createUnsized() noexcept { return { ~0u }; }
};
static_assert(sizeof(ComponentRegistryEntry) == 4, "ComponentRegistryEntry is padded");

// ECS
// ------------------------------------------------------------------------------------------------

// The header for the ECS system
//
// The entire ECS system is contained in a single chunk of allocated memory, without any pointers
// of any kind. This means that it is possible to memcpy (including writing and reading from file)
// the entire system.
//
// Given:
// N = max number of entities
// K = number of component systems
// The ECS system has the following representation in memory:
//
// | ECS header |
// | Component registry array header |
// | ComponentRegistryEntry 0 |
// | ... |
// | ComponentRegistryEntry K-1 |
// | Free entities list array header |
// | Free entity index 0 (N-1 at first) |
// | ... |
// | Free entity index N-1 (0 at first) |
// | Entity masks array header |
// | Entity mask 0 |
// | .. |
// | Entity mask N-1 |
// | Component type 0 array header |
// | Component type 0, entity 0 |
// | ... |
// | Component type 0, entity N-1 |
// | .. |
// | Component type K-1 array header |
// | Component type K-1, entity 0 |
// | ... |
// | Component type K-1, entity N-1 |
struct NaiveEcsHeader final {

	// Members
	// --------------------------------------------------------------------------------------------

	// Magic number in beginning of the game state. Should spell out "PHESTATE" if viewed in a hex
	// editor. Can be used to check if a binary file seems to be a game state dumped to file.
	// See: https://en.wikipedia.org/wiki/File_format#Magic_number
	uint64_t MAGIC_NUMBER;

	// The version of the ECS system, this number should increment each time a change is made to
	// the data layout of the system.
	uint32_t ECS_VERSION;

	// The size of the ECS system in bytes. This is the number of bytes to copy if you want to copy
	// the entire system using memcpy(). E.g. "memcpy(dst, ecsHeader, ecsHeader->ecsSizeBytes)".
	uint32_t ecsSizeBytes;

	// The number of component types in this system. This includes data-less flags, such as the
	// first (0th) ComponentMask bit which is reserved for whether an entity is active or not.
	uint32_t numComponentTypes;

	// The maximum number of entities allowed in this ECS system.
	uint32_t maxNumEntities;

	// The current number of entities in this system. It is NOT safe to use this as the upper bound
	// when iterating over all entities as the currently existing entities are not guaranteed to
	// be contiguously packed.
	uint32_t currentNumEntities;

	// Offset in bytes to the ArrayHeader of ComponentRegistryEntry which in turn contains the
	// offsets to the ArrayHeaders for the various component types
	uint32_t offsetComponentRegistry;

	// Offset in bytes to the ArrayHeader of free entities (uint32_t)
	uint32_t offsetFreeEntitiesList;

	// Offset in bytes to the ArrayHeader of ComponentMask, each entity is its own index into this
	// array of masks.
	uint32_t offsetComponentMasks;

	// Unused padding to ensure header is 32-byte aligned.
	uint32_t ___PADDING_UNUSED___[6];

	// API
	// --------------------------------------------------------------------------------------------

	// Creates a new entity with no associated components. Index is guaranteed to be smaller than
	// the systems maximum number of entities. Indices used for removed entities will be used.
	// Returns ~0 (UINT32_MAX) if no more free entities are available.
	// Complexity: O(1)
	uint32_t createEntity() noexcept;

	// Deletes the given entity and deletes (clears) all associated components. Returns whether
	// successful or not.
	// Complexity: O(K) where K is number of component types
	bool deleteEntity(uint32_t entity) noexcept;

	// Clones a given entity and all its components. Returns ~0 (UINT32_MAX) on failure.
	// Complexity: O(K) where K is number of component types
	uint32_t cloneEntity(uint32_t entity) noexcept;

	// Returns pointer to the contiguous array of ComponentMask.
	// Complexity: O(1)
	ComponentMask* componentMasks() noexcept;
	const ComponentMask* componentMasks() const noexcept;

	// Returns pointer to the contiguous array of components of a given component type. Returns
	// nullptr if the component type does not have associated data or does not exist. The second
	// parameter returns the size of each component in bytes.
	// Complexity: O(1)
	uint8_t* componentsUntyped(uint32_t componentType, uint32_t& componentSizeBytesOut) noexcept;
	const uint8_t* componentsUntyped(uint32_t componentType, uint32_t& componentSizeBytesOut) const noexcept;

	// Returns typed pointer to the contiguous array of components of a given component type.
	// See getComponentArrayUntyped(), the requested type (T) must be of the correct size.
	// Complexity: O(1)
	template<typename T>
	T* components(uint32_t componentType) noexcept
	{
		uint32_t componentSize = 0;
		T* components = (T*)componentsUntyped(componentType, componentSize);
		sfz_assert_debug(sizeof(T) == componentSize);
		return components;
	}
	template<typename T>
	const T* components(uint32_t componentType) const noexcept
	{
		uint32_t componentSize = 0;
		const T* components = (const T*)componentsUntyped(componentType, componentSize);
		sfz_assert_debug(sizeof(T) == componentSize);
		return components;
	}

	// Adds a component to an entity. Returns whether succesful or not.
	// Complexity: O(1)
	bool addComponentUntyped(
		uint32_t entity, uint32_t componentType, const uint8_t* data, uint32_t dataSize) noexcept;

	// Adds a (typed) component to an entity. Returns whether succesful or not.
	// Complexity: O(1)
	template<typename T>
	bool addComponent(uint32_t entity, uint32_t componentType, const T& component) noexcept
	{
		return addComponentUntyped(entity, componentType, (const uint8_t*)&component, sizeof(T));
	}

	// Sets the value (i.e. flag) of an unsized component. Returns whether succesful or not.
	// Complexity: O(1)
	bool setComponentUnsized(uint32_t entity, uint32_t componentType, bool value) noexcept;

	// Delets a component from an entity. Returns whether succesful or not.
	// Complexity: O(1)
	bool deleteComponent(uint32_t entity, uint32_t componentType) noexcept;

	// Accessing arrays
	// --------------------------------------------------------------------------------------------

	ArrayHeader* componentRegistryArray() noexcept { return arrayAt(offsetComponentRegistry); }
	const ArrayHeader* componentRegistryArray() const noexcept { return arrayAt(offsetComponentRegistry); }

	ArrayHeader* freeEntitiesListArray() noexcept { return arrayAt(offsetFreeEntitiesList); }
	const ArrayHeader* freeEntitiesListArray() const noexcept { return arrayAt(offsetFreeEntitiesList); }

	ArrayHeader* componentMasksArray() noexcept { return arrayAt(offsetComponentMasks); }
	const ArrayHeader* componentMasksArray() const noexcept { return arrayAt(offsetComponentMasks); }

	// Helper methods
	// --------------------------------------------------------------------------------------------

	ArrayHeader* arrayAt(uint32_t offset) noexcept
	{
		return reinterpret_cast<ArrayHeader*>(reinterpret_cast<uint8_t*>(this) + offset);
	}
	const ArrayHeader* arrayAt(uint32_t offset) const noexcept
	{
		return reinterpret_cast<const ArrayHeader*>(reinterpret_cast<const uint8_t*>(this) + offset);
	}
};
static_assert(sizeof(NaiveEcsHeader) == 64, "EcsHeader is padded");

// ECS functions
// ------------------------------------------------------------------------------------------------

// Creates a naive ECS system.
//
// The resulting system will contain numComponentTypes + 1 types of components. The first type (0)
// is reserved to signify whether and entity is active or not. If you want data-less component
// types, i.e. flags, you should specify 0 as the size in the "componentSizes" array.
EcsContainer createEcs(
	uint32_t maxNumEntities,
	const uint32_t* componentSizes,
	uint32_t numComponentTypes,
	Allocator* allocator = sfz::getDefaultAllocator()) noexcept;

} // namespace ph
