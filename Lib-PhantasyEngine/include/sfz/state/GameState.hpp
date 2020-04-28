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

#include <skipifzero.hpp>

#include "sfz/state/ArrayHeader.hpp"
#include "sfz/state/ComponentMask.hpp"
#include "sfz/state/Entity.hpp"

namespace sfz {

// Constants
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

// The current data layout version of the game state
constexpr uint64_t GAME_STATE_VERSION = 5;

// The maximum number of entities a game state can hold
//
// One less than the maximum id of an entity (ENTITY_ID_MAX), we reserve all bits set to 1 (~0,
// the default-value when constructing an Entity) as an error code.
constexpr uint32_t GAME_STATE_ECS_MAX_NUM_ENTITIES = ENTITY_ID_MAX - 1;

// SingletonRegistryEntry struct
// ------------------------------------------------------------------------------------------------

struct SingletonRegistryEntry final {

	// The offset in bytes to the singleton struct
	uint32_t offset;

	// The size in bytes of the singleton struct
	uint32_t sizeInBytes;
};

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
	uint64_t magicNumber;

	// The version of the game state, this number should increment each time a change is made to
	// the data layout of the system.
	uint64_t gameStateVersion;

	// The size of the game state in bytes. This is the number of bytes to copy if you want to copy
	// the entire state using memcpy(). E.g. "memcpy(dst, stateHeader, stateHeader->stateSizeBytes)".
	uint64_t stateSizeBytes;

	// The number of singleton structs in the game state.
	uint32_t numSingletons;

	// The number of component types in the ECS system. This includes data-less flags, such as the
	// first (0th) ComponentMask bit which is reserved for whether an entity is active or not.
	uint32_t numComponentTypes;

	// The maximum number of entities allowed in the ECS system.
	uint32_t maxNumEntities;

	// The current number of entities in this ECS system. It is NOT safe to use this as the upper
	// bound when iterating over all entities as the currently existing entities are not guaranteed
	// to be contiguously packed.
	uint32_t currentNumEntities;

	// Offset in bytes to the ArrayHeader of SingletonRegistryEntry which in turn contains the
	// offsets to the singleton structs, and the sizes of them.
	uint32_t offsetSingletonRegistry;

	// Offset in bytes to the ArrayHeader of ComponentRegistryEntry which in turn contains the
	// offsets to the ArrayHeaders for the various component types
	uint32_t offsetComponentRegistry;

	// Offset in bytes to the ArrayHeader of free entity ids (uint32_t)
	uint32_t offsetFreeEntityIdsList;

	// Offset in bytes to the ArrayHeader of ComponentMask, each entity is its own index into this
	// array of masks.
	uint32_t offsetComponentMasks;

	// Offset in bytes to the ArrayHeader of entity generations (uint8_t)
	uint32_t offsetEntityGenerationsList;

	// Unused padding to ensure header is 16-byte aligned.
	uint32_t ___PADDING_UNUSED___[1];

	// Singleton state API
	// --------------------------------------------------------------------------------------------

	// Returns a pointer to the singleton at the given index. Returns nullptr if the singleton
	// does not exist. The second parameter returns the size of the singleton in bytes.
	uint8_t* singletonUntyped(uint32_t singletonIndex, uint32_t& singletonSizeBytesOut) noexcept;
	const uint8_t* singletonUntyped(uint32_t singletonIndex, uint32_t& singletonSizeBytesOut) const noexcept;

	// Returns typed reference to the singleton at the given index. Undefined behavior (likely
	// segfault) if singleton does not exist. The requested type T must be of the correct size.
	template<typename T>
	T& singleton(uint32_t singletonIndex) noexcept
	{
		static_assert(std::is_trivially_copyable<T>::value, "Game state singletons must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "Game state singletons must be trivially destructible");
		uint32_t singletonSize = 0;
		T* singleton = (T*)singletonUntyped(singletonIndex, singletonSize);
		sfz_assert(sizeof(T) == singletonSize);
		return *singleton;
	}
	template<typename T>
	const T& singleton(uint32_t singletonIndex) const noexcept
	{
		static_assert(std::is_trivially_copyable<T>::value, "Game state singletons must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "Game state singletons must be trivially destructible");
		uint32_t singletonSize = 0;
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
	bool deleteEntity(uint32_t entityId) noexcept;

	// Clones a given entity and all its components. Returns Entity::invalid() on failure.
	// Complexity: O(K) where K is number of component types
	Entity cloneEntity(Entity entity) noexcept;

	// Returns pointer to the contiguous array of ComponentMask.
	// Complexity: O(1)
	ComponentMask* componentMasks() noexcept;
	const ComponentMask* componentMasks() const noexcept;

	// Returns pointer to the contiguous array of entity generations (uint8_t). If the generation()
	// of an entity does not match the generation at index id() in this list then the entity is
	// invalid (i.e. a "dangling pointer entity").
	// Complexity: O(1)
	uint8_t* entityGenerations() noexcept;
	const uint8_t* entityGenerations() const noexcept;

	// Returns the current generation for the specified entity id. Convenience function around
	// entityGenerations(), which should be preferred if multiple entities ids generations are to be
	// looked up.
	// Complexity: O(1)
	uint8_t getGeneration(uint32_t entityId) const noexcept;

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
	uint8_t* componentsUntyped(uint32_t componentType, uint32_t& componentSizeBytesOut) noexcept;
	const uint8_t* componentsUntyped(uint32_t componentType, uint32_t& componentSizeBytesOut) const noexcept;

	// Returns typed pointer to the contiguous array of components of a given component type.
	// See getComponentArrayUntyped(), the requested type (T) must be of the correct size.
	// Complexity: O(1)
	template<typename T>
	T* components(uint32_t componentType) noexcept
	{
		static_assert(std::is_trivially_copyable<T>::value, "ECS components must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "ECS components must be trivially destructible");
		uint32_t componentSize = 0;
		T* components = (T*)componentsUntyped(componentType, componentSize);
		sfz_assert(sizeof(T) == componentSize);
		return components;
	}
	template<typename T>
	const T* components(uint32_t componentType) const noexcept
	{
		static_assert(std::is_trivially_copyable<T>::value, "ECS components must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "ECS components must be trivially destructible");
		uint32_t componentSize = 0;
		const T* components = (const T*)componentsUntyped(componentType, componentSize);
		sfz_assert(sizeof(T) == componentSize);
		return components;
	}

	// Adds a component to an entity. Returns whether succesful or not.
	// Complexity: O(1)
	bool addComponentUntyped(
		Entity entity, uint32_t componentType, const uint8_t* data, uint32_t dataSize) noexcept;

	// Adds a (typed) component to an entity. Returns whether succesful or not.
	// Complexity: O(1)
	template<typename T>
	bool addComponent(Entity entity, uint32_t componentType, const T& component) noexcept
	{
		static_assert(std::is_trivially_copyable<T>::value, "ECS components must be trivially copyable");
		static_assert(std::is_trivially_destructible<T>::value, "ECS components must be trivially destructible");
		return addComponentUntyped(entity, componentType, (const uint8_t*)&component, sizeof(T));
	}

	// Sets the value (i.e. flag) of an unsized component. Returns whether succesful or not.
	// Complexity: O(1)
	bool setComponentUnsized(Entity entity, uint32_t componentType, bool value) noexcept;

	// Delets a component from an entity. Returns whether succesful or not.
	// Complexity: O(1)
	bool deleteComponent(Entity entity, uint32_t componentType) noexcept;

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

	ArrayHeader* arrayAt(uint32_t offset) noexcept
	{
		return reinterpret_cast<ArrayHeader*>(reinterpret_cast<uint8_t*>(this) + offset);
	}
	const ArrayHeader* arrayAt(uint32_t offset) const noexcept
	{
		return reinterpret_cast<const ArrayHeader*>(reinterpret_cast<const uint8_t*>(this) + offset);
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
constexpr uint32_t calcSizeOfGameStateBytes(
	uint32_t numSingletons,
	const uint32_t singletonSizes[],
	uint32_t maxNumEntities,
	uint32_t numComponents,
	const uint32_t componentSizes[])
{
	uint32_t totalSizeBytes = 0;

	// GameState Header
	totalSizeBytes += sizeof(GameStateHeader);

	// Singleton registry
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(SingletonRegistryEntry), numSingletons);

	// Singleton structs
	for (uint32_t i = 0; i < numSingletons; i++) {
		totalSizeBytes += uint32_t(roundUpAligned(singletonSizes[i], 16));
	}

	// Component registry (+ 1 for active bit)
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(ComponentRegistryEntry), numComponents + 1);

	// Free entity ids list
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(uint32_t), maxNumEntities);

	// Entity masks
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(ComponentMask), maxNumEntities);

	// Entity generations list
	totalSizeBytes += calcArrayHeaderSizeBytes(sizeof(uint8_t), maxNumEntities);

	// Component arrays
	for (uint32_t i = 0; i < numComponents; i++) {
		totalSizeBytes += calcArrayHeaderSizeBytes(componentSizes[i], maxNumEntities);
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
	uint32_t dstMemorySizeBytes,
	uint32_t numSingletons,
	const uint32_t* singletonSizes,
	uint32_t maxNumEntities,
	uint32_t numComponents,
	const uint32_t* componentSizes) noexcept;

} // namespace sfz
