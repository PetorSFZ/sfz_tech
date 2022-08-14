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

#include <type_traits>

#include <sfz.h>

#include "sfz/state/ArrayHeader.hpp"
#include "sfz/state/CompMask.hpp"
#include "sfz/state/Entity.hpp"

namespace sfz {

// Constants
// ------------------------------------------------------------------------------------------------

// Magic number in beginning of all Phantasy Engine game states.
constexpr u64 GAME_STATE_MAGIC_NUMBER =
	u64('P') << 0 |
	u64('H') << 8 |
	u64('E') << 16 |
	u64('S') << 24 |
	u64('T') << 32 |
	u64('A') << 40 |
	u64('T') << 48 |
	u64('E') << 56;

// The current data layout version of the game state
constexpr u64 GAME_STATE_VERSION = 6;

// The maximum number of entities a game state can hold
//
// One less than the maximum id of an entity (ENTITY_ID_MAX), we reserve all bits set to 1 (~0,
// the default-value when constructing an Entity) as an error code.
constexpr u32 GAME_STATE_ECS_MAX_NUM_ENTITIES = ENTITY_ID_MAX - 1;

// SingletonRegistryEntry struct
// ------------------------------------------------------------------------------------------------

struct SingletonRegistryEntry final {

	// The offset in bytes to the singleton struct
	u32 offset;

	// The size in bytes of the singleton struct
	u32 sizeInBytes;
};

// ComponentRegistryEntry struct
// ------------------------------------------------------------------------------------------------

struct ComponentRegistryEntry final {

	// The offset in bytes to the ArrayHeader of components for the specific type, ~0 (U32_MAX)
	// if there is no associated data with the given component type.
	u32 offset;

	// Returns whether the component type has associated data or not.
	bool componentTypeHasData() const noexcept { return offset != ~0u; }

	static ComponentRegistryEntry createSized(u32 offset) noexcept { return { offset }; }
	static ComponentRegistryEntry createUnsized() noexcept { return { ~0u }; }
};
static_assert(sizeof(ComponentRegistryEntry) == 4, "ComponentRegistryEntry is padded");

// GameState
// ------------------------------------------------------------------------------------------------

// The header for a GameState. A GameState is a combination of singleton state and an ECS system.
//
// The entire game state is contained in a single chunk of allocated memory, without any pointers
// of any kind. This means that it is possible to memcpy (including writing and reading from file)
// the entire state.
//
// Given:
// S = number of singletons
// N = max number of entities
// K = number of component systems
// The game state has the following representation in memory:
//
// | GameState header |
// | Singleton registry array header|
// | SingletonRegistryEntry 0 |
// | ... |
// | SingletonRegistryEntry S-1 |
// | Singleton struct 0 |
// | ... |
// | Singleton struct S-1 |
// | Component registry array header |
// | ComponentRegistryEntry 0 |
// | ... |
// | ComponentRegistryEntry K-1 |
// | Free entity ids list array header |
// | Free entity id index 0 (value N-1 at first) |
// | ... |
// | Free entity id index N-1 (value 0 at first) |
// | Entity masks array header |
// | Entity mask 0 |
// | .. |
// | Entity mask N-1 |
// | Entity generations list array header |
// | Entity generation 0 |
// | .. |
// | Entity generation N-1 |
// | Component type 0 array header |
// | Component type 0, entity 0 |
// | ... |
// | Component type 0, entity N-1 |
// | .. |
// | Component type K-1 array header |
// | Component type K-1, entity 0 |
// | ... |
// | Component type K-1, entity N-1 |
struct GameStateHeader {

	// Members
	// --------------------------------------------------------------------------------------------

	// Magic number in beginning of the game state. Should spell out "PHESTATE" if viewed in a hex
	// editor. Can be used to check if a binary file seems to be a game state dumped to file.
	// See: https://en.wikipedia.org/wiki/File_format#Magic_number
	u64 magicNumber;

	// The version of the game state, this number should increment each time a change is made to
	// the data layout of the system.
	u64 gameStateVersion;

	// The size of the game state in bytes. This is the number of bytes to copy if you want to copy
	// the entire state using memcpy(). E.g. "memcpy(dst, stateHeader, stateHeader->stateSizeBytes)".
	u64 stateSizeBytes;

	// The number of singleton structs in the game state.
	u32 numSingletons;

	// The number of component types in the ECS system. This includes data-less flags, such as the
	// first (0th) CompMask bit which is reserved for whether an entity is active or not.
	u32 numComponentTypes;

	// The maximum number of entities allowed in the ECS system.
	u32 maxNumEntities;

	// The current number of entities in this ECS system. It is NOT safe to use this as the upper
	// bound when iterating over all entities as the currently existing entities are not guaranteed
	// to be contiguously packed.
	u32 currentNumEntities;

	// Offset in bytes to the ArrayHeader of SingletonRegistryEntry which in turn contains the
	// offsets to the singleton structs, and the sizes of them.
	u32 offsetSingletonRegistry;

	// Offset in bytes to the ArrayHeader of ComponentRegistryEntry which in turn contains the
	// offsets to the ArrayHeaders for the various component types
	u32 offsetComponentRegistry;

	// Offset in bytes to the ArrayHeader of free entity ids (u32)
	u32 offsetFreeEntityIdsList;

	// Offset in bytes to the ArrayHeader of ComponentMask, each entity is its own index into this
	// array of masks.
	u32 offsetComponentMasks;

	// Offset in bytes to the ArrayHeader of entity generations (u8)
	u32 offsetEntityGenerationsList;

	// Unused padding to ensure header is 16-byte aligned.
	u32 ___PADDING_UNUSED___[1];

	// Singleton state API
	// --------------------------------------------------------------------------------------------

	// Returns a pointer to the singleton at the given index. Returns nullptr if the singleton
	// does not exist. The second parameter returns the size of the singleton in bytes.
	u8* singletonUntyped(u32 singletonIndex, u32& singletonSizeBytesOut) noexcept;
	const u8* singletonUntyped(u32 singletonIndex, u32& singletonSizeBytesOut) const noexcept;

	// Returns typed reference to the singleton at the given index. Undefined behavior (likely
	// segfault) if singleton does not exist. The requested type T must be of the correct size.
	template<typename T>
	T& singleton(u32 singletonIndex) noexcept
	{
		//static_assert(std::is_trivially_copyable<T>::value, "Game state singletons must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "Game state singletons must be trivially destructible");
		u32 singletonSize = 0;
		T* singleton = (T*)singletonUntyped(singletonIndex, singletonSize);
		sfz_assert(sizeof(T) == singletonSize);
		return *singleton;
	}
	template<typename T>
	const T& singleton(u32 singletonIndex) const noexcept
	{
		//static_assert(std::is_trivially_copyable<T>::value, "Game state singletons must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "Game state singletons must be trivially destructible");
		u32 singletonSize = 0;
		const T* singleton = (const T*)singletonUntyped(singletonIndex, singletonSize);
		sfz_assert(sizeof(T) == singletonSize);
		return *singleton;
	}

	// ECS API
	// --------------------------------------------------------------------------------------------

	// Creates a new entity with no associated components. Index is guaranteed to be smaller than
	// the ECS system's maximum number of entities. Indices used for removed entities will be used.
	// Returns Entity::invalid() if no more free entities are available.
	// Complexity: O(1)
	Entity createEntity() noexcept;

	// Deletes the given entity and deletes (clears) all associated components. Returns whether
	// successful or not.
	// Complexity: O(K) where K is number of component types
	bool deleteEntity(Entity entity) noexcept;
	bool deleteEntity(u32 entityId) noexcept;

	// Clones a given entity and all its components. Returns Entity::invalid() on failure.
	// Complexity: O(K) where K is number of component types
	Entity cloneEntity(Entity entity) noexcept;

	// Returns pointer to the contiguous array of ComponentMask.
	// Complexity: O(1)
	CompMask* componentMasks() noexcept;
	const CompMask* componentMasks() const noexcept;

	// Returns pointer to the contiguous array of entity generations (u8). If the generation()
	// of an entity does not match the generation at index id() in this list then the entity is
	// invalid (i.e. a "dangling pointer entity"). Generation "0" is reserved as invalid.
	// Complexity: O(1)
	u8* entityGenerations() noexcept;
	const u8* entityGenerations() const noexcept;

	// Returns the current generation for the specified entity id. Convenience function around
	// entityGenerations(), which should be preferred if multiple entities ids generations are to be
	// looked up.
	// Complexity: O(1)
	u8 getGeneration(u32 entityId) const noexcept;

	// Checks whether a given entity is valid or not by comparing its generation with the internal
	// one stored in the ECS system.
	// Complexity: O(1)
	bool checkGeneration(Entity entity) const noexcept;

	// Returns whether the given entity is valid or not by checking if it is in bounds, if it is
	// active and if it has the correct generation. This is a convenience function that should only
	// be used if a few entities need to be checked. Otherwise this should be done manually by
	// grabbing the component masks and generations arrays directly.
	// Complexity: O(1)
	bool checkEntityValid(Entity entity) const noexcept;

	// Returns pointer to the contiguous array of components of a given component type. Returns
	// nullptr if the component type does not have associated data or does not exist. The second
	// parameter returns the size of each component in bytes.
	// Complexity: O(1)
	u8* componentsUntyped(u32 componentType, u32& componentSizeBytesOut) noexcept;
	const u8* componentsUntyped(u32 componentType, u32& componentSizeBytesOut) const noexcept;

	// Returns typed pointer to the contiguous array of components of a given component type.
	// See getComponentArrayUntyped(), the requested type (T) must be of the correct size.
	// Complexity: O(1)
	template<typename T>
	T* components(u32 componentType) noexcept
	{
		static_assert(std::is_trivially_copyable<T>::value, "ECS components must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "ECS components must be trivially destructible");
		u32 componentSize = 0;
		T* components = (T*)componentsUntyped(componentType, componentSize);
		sfz_assert(sizeof(T) == componentSize);
		return components;
	}
	template<typename T>
	const T* components(u32 componentType) const noexcept
	{
		static_assert(std::is_trivially_copyable<T>::value, "ECS components must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "ECS components must be trivially destructible");
		u32 componentSize = 0;
		const T* components = (const T*)componentsUntyped(componentType, componentSize);
		sfz_assert(sizeof(T) == componentSize);
		return components;
	}

	// Adds a component to an entity. Returns whether succesful or not.
	// Complexity: O(1)
	bool addComponentUntyped(
		Entity entity, u32 componentType, const u8* data, u32 dataSize) noexcept;

	// Adds a (typed) component to an entity. Returns whether succesful or not.
	// Complexity: O(1)
	template<typename T>
	bool addComponent(Entity entity, u32 componentType, const T& component) noexcept
	{
		static_assert(std::is_trivially_copyable<T>::value, "ECS components must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "ECS components must be trivially destructible");
		return addComponentUntyped(entity, componentType, (const u8*)&component, sizeof(T));
	}

	// Sets the value (i.e. flag) of an unsized component. Returns whether succesful or not.
	// Complexity: O(1)
	bool setComponentUnsized(Entity entity, u32 componentType, bool value) noexcept;

	// Delets a component from an entity. Returns whether succesful or not.
	// Complexity: O(1)
	bool deleteComponent(Entity entity, u32 componentType) noexcept;

	// Accessing arrays
	// --------------------------------------------------------------------------------------------

	ArrayHeader* singletonRegistryArray() noexcept { return arrayAt(offsetSingletonRegistry); }
	const ArrayHeader* singletonRegistryArray() const noexcept { return arrayAt(offsetSingletonRegistry); }

	ArrayHeader* componentRegistryArray() noexcept { return arrayAt(offsetComponentRegistry); }
	const ArrayHeader* componentRegistryArray() const noexcept { return arrayAt(offsetComponentRegistry); }

	ArrayHeader* freeEntityIdsListArray() noexcept { return arrayAt(offsetFreeEntityIdsList); }
	const ArrayHeader* freeEntityIdsListArray() const noexcept { return arrayAt(offsetFreeEntityIdsList); }

	ArrayHeader* componentMasksArray() noexcept { return arrayAt(offsetComponentMasks); }
	const ArrayHeader* componentMasksArray() const noexcept { return arrayAt(offsetComponentMasks); }

	ArrayHeader* entityGenerationsListArray() noexcept { return arrayAt(offsetEntityGenerationsList); }
	const ArrayHeader* entityGenerationsListArray() const noexcept { return arrayAt(offsetEntityGenerationsList); }

	// Helper methods
	// --------------------------------------------------------------------------------------------

	ArrayHeader* arrayAt(u32 offset) noexcept
	{
		return reinterpret_cast<ArrayHeader*>(reinterpret_cast<u8*>(this) + offset);
	}
	const ArrayHeader* arrayAt(u32 offset) const noexcept
	{
		return reinterpret_cast<const ArrayHeader*>(reinterpret_cast<const u8*>(this) + offset);
	}

	// Constructors & destructors tricks
	// --------------------------------------------------------------------------------------------

	// Copying and moving of the GameStateHeader struct is forbidden. This is a bit of a hack to
	// avoid a certain class of bugs. Essentially, the GameStateHeader assumes it is at the top of
	// a chunk of memory containing the entire game state. If it is not, it will read/write invalid
	// memory for some of its operations. E.g. this could happen if you attempted to do this:
	//
	// GameStateHeader* statePtr = ... // (Pointer to game state memory chunk)
	// GameStateHeader header = *statePtr; // Copies header to header, but not entire state
	// header.someOperation(); // Invalid memory access because there is no state here
	//
	// By disabling the copy constructors the above code would give compile error. Do note that
	// GameStateHeader is still POD (i.e. trivially copyable), even though the C++ compiler does not
	// consider it to be. It is still completely fine to memcpy() and such as long as you know what
	// you are doing.
	GameStateHeader(const GameStateHeader&) = delete;
	GameStateHeader& operator=(const GameStateHeader&) = delete;
	GameStateHeader(GameStateHeader&&) = delete;
	GameStateHeader& operator=(GameStateHeader&&) = delete;
};
static_assert(sizeof(GameStateHeader) == 64, "GameStateHeader is padded");

// Game state functions
// ------------------------------------------------------------------------------------------------

// Calculates the size of a game state in bytes. Can be used to statically allocate the necessary
// memory to hold a game state
constexpr u32 calcSizeOfGameStateBytes(
	u32 numSingletons,
	const u32 singletonSizes[],
	u32 maxNumEntities,
	u32 numComponents,
	const u32 componentSizes[])
{
	u32 totalSizeBytes = 0;

	// GameState Header
	totalSizeBytes += sizeof(GameStateHeader);

	// Singleton registry
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(SingletonRegistryEntry), numSingletons);

	// Singleton structs
	for (u32 i = 0; i < numSingletons; i++) {
		totalSizeBytes += sfzRoundUpAlignedU32(singletonSizes[i], 16);
	}

	// Component registry (+ 1 for active bit)
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(ComponentRegistryEntry), numComponents + 1);

	// Free entity ids list
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(u32), maxNumEntities);

	// Entity masks
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(CompMask), maxNumEntities);

	// Entity generations list
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(u8), maxNumEntities);

	// Component arrays
	for (u32 i = 0; i < numComponents; i++) {
		if (componentSizes[i] > 0) {
			totalSizeBytes += calcArrayHeaderSizeBytes(componentSizes[i], maxNumEntities);
		}
	}

	return totalSizeBytes;
}

// Creates a game state in the specified destination memory.
//
// Returns false and fails if the memory chunk is too small. The required amount of memory can
// be calculated using the "calcSizeOfGameStateBytes()" function.
//
// The resulting state will contain numComponentTypes + 1 types of components. The first type (0)
// is reserved to signify whether and entity is active or not. If you want data-less component
// types, i.e. flags, you should specify 0 as the size in the "componentSizes" array.
bool createGameState(
	void* dstMemory,
	u32 dstMemorySizeBytes,
	u32 numSingletons,
	const u32* singletonSizes,
	u32 maxNumEntities,
	u32 numComponents,
	const u32* componentSizes) noexcept;

} // namespace sfz
