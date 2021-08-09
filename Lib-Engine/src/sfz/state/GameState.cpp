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

#include "sfz/state/GameState.hpp"

#include <cstring>

namespace sfz {

// GameState: Singleton state API
// ------------------------------------------------------------------------------------------------

u8* GameStateHeader::singletonUntyped(u32 singletonIndex, u32& singletonSizeBytesOut) noexcept
{
	// Get registry, return nullptr if component type is not in registry
	ArrayHeader* registry = this->singletonRegistryArray();
	sfz_assert(singletonIndex < registry->size);
	if (registry->size <= singletonIndex) return nullptr;

	// Get registry entry, return nullptr if component type has no data
	SingletonRegistryEntry entry = registry->at<SingletonRegistryEntry>(singletonIndex);

	// Return singleton size and pointer
	singletonSizeBytesOut = entry.sizeInBytes;
	return reinterpret_cast<u8*>(this) + entry.offset;
}

const u8* GameStateHeader::singletonUntyped(u32 singletonIndex, u32& singletonSizeBytesOut) const noexcept
{
	// Get registry, return nullptr if component type is not in registry
	const ArrayHeader* registry = this->singletonRegistryArray();
	sfz_assert(singletonIndex < registry->size);
	if (registry->size <= singletonIndex) return nullptr;

	// Get registry entry, return nullptr if component type has no data
	SingletonRegistryEntry entry = registry->at<SingletonRegistryEntry>(singletonIndex);

	// Return singleton size and pointer
	singletonSizeBytesOut = entry.sizeInBytes;
	return reinterpret_cast<const u8*>(this) + entry.offset;
}

// GameState: ECS API
// ------------------------------------------------------------------------------------------------

Entity GameStateHeader::createEntity() noexcept
{
	// Get free entity from free entities list
	ArrayHeader* freeEntitiesList = this->freeEntityIdsListArray();
	u32 freeEntityId = 0;
	bool freeEntityAvailable = freeEntitiesList->popGet(freeEntityId);

	// Return Entity::invalid() if no free entity id is available
	if (!freeEntityAvailable) return NULL_ENTITY;

	// Increment number of entities
	currentNumEntities += 1;

	// Set component mask
	ArrayHeader* componentMasks = this->componentMasksArray();
	CompMask& mask = componentMasks->at<CompMask>(freeEntityId);
	mask = CompMask::activeMask();

	// Get generation
	u8* generations = this->entityGenerations();
	u8 generation = generations[freeEntityId];

	// Return entity
	return Entity::create(freeEntityId, generation);
}

bool GameStateHeader::deleteEntity(Entity entity) noexcept
{
	if (!this->checkGeneration(entity)) return false;
	return this->deleteEntity(entity.id());
}

bool GameStateHeader::deleteEntity(u32 entityId) noexcept
{
	if (entityId >= this->maxNumEntities) return false;

	// Get mask
	ArrayHeader* componentMasks = this->componentMasksArray();
	CompMask& mask = componentMasks->at<CompMask>(entityId);

	// Get generations
	u8* generations = this->entityGenerations();
	u8& generation = generations[entityId];

	// Return false if entity is not active
	if (!mask.active()) return false;

	// Decrement number of entities
	if (currentNumEntities != 0) currentNumEntities -= 1;

	// Remove all associated components
	for (u32 i = 0; i < this->numComponentTypes; i++) {

		// Get components array for type i, skip if does not exist
		u32 componentSize = 0;
		u8* components = componentsUntyped(i, componentSize);
		if (components == nullptr) continue;

		// Clear component
		memset(components + entityId * componentSize, 0, componentSize);
	}

	// Clear mask
	mask = CompMask::empty();

	// Increment generation, skip 0 as it is reserved for invalid.
	generation += 1;
	if (generation == 0) generation = 1;

	// Add entity id back to free entities list
	ArrayHeader* freeEntityIdsList = this->freeEntityIdsListArray();
	freeEntityIdsList->add<u32>(entityId);

	return true;
}

Entity GameStateHeader::cloneEntity(Entity entity) noexcept
{
	// Grab id and generation
	u32 entityId = entity.id();
	u8 entityGeneration = entity.generation();

	// If id is out of range, return invalid entity
	if (entityId >= this->maxNumEntities) return NULL_ENTITY;

	// Get mask, exit if entity does not exist
	CompMask* masks = this->componentMasks();
	CompMask mask = masks[entityId];
	if (!mask.active()) return NULL_ENTITY;

	// Get generation, exit if entity has wrong generation
	u8* generations = this->entityGenerations();
	u8 expectedGeneration = generations[entityId];
	if (entityGeneration != expectedGeneration) return NULL_ENTITY;

	// Create entity, exit out on failure
	Entity newEntity = this->createEntity();
	if (newEntity == NULL_ENTITY) return NULL_ENTITY;

	// Copy mask
	u32 newEntityId = newEntity.id();
	masks[newEntityId] = mask;

	// Copy components
	for (u32 i = 1; i < this->numComponentTypes; i++) {
		if (!mask.fulfills(CompMask::fromType(i))) continue;

		// Get components array
		u32 componentSize = 0;
		u8* components = this->componentsUntyped(i, componentSize);

		// Skip if component type does not have data
		if (components == nullptr) continue;

		// Copy component
		u8* dst = components + newEntityId * componentSize;
		const u8* src = components + entityId * componentSize;
		memcpy(dst, src, componentSize);
	}

	return newEntity;
}

CompMask* GameStateHeader::componentMasks() noexcept
{
	return componentMasksArray()->data<CompMask>();
}

const CompMask* GameStateHeader::componentMasks() const noexcept
{
	return componentMasksArray()->data<CompMask>();
}

u8* GameStateHeader::entityGenerations() noexcept
{
	return entityGenerationsListArray()->data<u8>();
}

const u8* GameStateHeader::entityGenerations() const noexcept
{
	return entityGenerationsListArray()->data<u8>();
}

u8 GameStateHeader::getGeneration(u32 entityId) const noexcept
{
	sfz_assert(entityId < this->maxNumEntities);
	const u8* generations = this->entityGenerations();
	return generations[entityId];
}

bool GameStateHeader::checkGeneration(Entity entity) const noexcept
{
	u8 generation = entity.generation();
	if (generation == 0) return false;
	u8 expectedGeneration = this->getGeneration(entity.id());
	return expectedGeneration == generation;
}

bool GameStateHeader::checkEntityValid(Entity entity) const noexcept
{
	// Check if in bounds
	u32 entityId = entity.id();
	if (entityId >= this->maxNumEntities) return false;

	// Check if active
	const CompMask* masks = componentMasks();
	CompMask mask = masks[entityId];
	if (!mask.active()) return false;

	// Check if correct generation
	if (!checkGeneration(entity)) return false;

	return true;
}

u8* GameStateHeader::componentsUntyped(
	u32 componentType, u32& componentSizeBytesOut) noexcept
{
	// Get registry, return nullptr if component type is not in registry
	ArrayHeader* registry = this->componentRegistryArray();
	if (registry->size <= componentType) return nullptr;

	// Get registry entry, return nullptr if component type has no data
	ComponentRegistryEntry entry = registry->at<ComponentRegistryEntry>(componentType);
	if (!entry.componentTypeHasData()) return nullptr;

	// Return component size and data pointer
	ArrayHeader* components = this->arrayAt(entry.offset);
	componentSizeBytesOut = components->elementSize;
	return components->dataUntyped();
}

const u8* GameStateHeader::componentsUntyped(
	u32 componentType, u32& componentSizeBytesOut) const noexcept
{
	// Get registry, return nullptr if component type is not in registry
	const ArrayHeader* registry = this->componentRegistryArray();
	if (registry->size <= componentType) return nullptr;

	// Get registry entry, return nullptr if component type has no data
	ComponentRegistryEntry entry = registry->at<ComponentRegistryEntry>(componentType);
	if (!entry.componentTypeHasData()) return nullptr;

	// Return component size and data pointer
	const ArrayHeader* components = this->arrayAt(entry.offset);
	componentSizeBytesOut = components->elementSize;
	return components->dataUntyped();
}

bool GameStateHeader::addComponentUntyped(
	Entity entity, u32 componentType, const u8* data, u32 dataSize) noexcept
{
	u32 entityId = entity.id();
	if (entityId >= this->maxNumEntities) return false;
	if (!checkGeneration(entity)) return false;
	if (componentType >= this->numComponentTypes) return false;

	// Return false if mask is not active
	CompMask& mask = this->componentMasks()[entityId];
	if (!mask.active()) return false;

	// Get components array, return false if component type does not have data
	u32 componentSize = 0;
	u8* components = componentsUntyped(componentType, componentSize);
	if (components == nullptr) return false;

	// Return false if dataSize does not match componentSize
	if (dataSize != componentSize) return false;

	// Copy component into ECS system
	memcpy(components + entityId * componentSize, data, dataSize);

	// Ensure bit is set in mask
	mask.setComponentType(componentType, true);

	return true;
}

bool GameStateHeader::setComponentUnsized(
	Entity entity, u32 componentType, bool value) noexcept
{
	u32 entityId = entity.id();
	if (entityId >= this->maxNumEntities) return false;
	if (!checkGeneration(entity)) return false;
	if (componentType >= this->numComponentTypes) return false;

	// Return false if mask is not active
	CompMask& mask = this->componentMasks()[entityId];
	if (!mask.active()) return false;

	// Get components array, return false if component type have data
	u32 componentSize = 0;
	u8* components = componentsUntyped(componentType, componentSize);
	if (components != nullptr) return false;

	// Set bit in mask
	mask.setComponentType(componentType, value);

	return true;
}

bool GameStateHeader::deleteComponent(Entity entity, u32 componentType) noexcept
{
	u32 entityId = entity.id();
	if (entityId >= this->maxNumEntities) return false;
	if (!checkGeneration(entity)) return false;

	// Return false if mask is not active
	CompMask& mask = this->componentMasks()[entityId];
	if (!mask.active()) return false;

	// Get components array, forward to setComponentUnsized() if component type does not have data
	u32 componentSize = 0;
	u8* components = componentsUntyped(componentType, componentSize);
	if (components == nullptr) return this->setComponentUnsized(entity, componentType, false);

	// Clear component
	memset(components + entityId * componentSize, 0, componentSize);

	// Clear bit in mask
	mask.setComponentType(componentType, false);

	return true;
}

// Game state functions
// ------------------------------------------------------------------------------------------------

bool createGameState(
	void* dstMemory,
	u32 dstMemorySizeBytes,
	u32 numSingletons,
	const u32* singletonSizes,
	u32 maxNumEntities,
	u32 numComponents,
	const u32* componentSizes) noexcept
{
	sfz_assert(numSingletons <= 64);
	sfz_assert(maxNumEntities <= GAME_STATE_ECS_MAX_NUM_ENTITIES);
	sfz_assert(numComponents <= 63); // Not 64 because one is reserved for active bit

	u32 totalSizeBytes = 0;

	// GameState Header
	totalSizeBytes += sizeof(GameStateHeader);

	// Singleton registry
	ArrayHeader singletonRegistryHeader;
	singletonRegistryHeader.create<SingletonRegistryEntry>(numSingletons);
	u32 singletonRegistrySizeBytes = calcArrayHeaderSizeBytes(sizeof(SingletonRegistryEntry), numSingletons);
	totalSizeBytes += singletonRegistrySizeBytes;

	// Singleton structs
	SingletonRegistryEntry singleRegistryEntries[64] = {};
	for (u32 i = 0; i < numSingletons; i++) {
		sfz_assert(singletonSizes[i] != 0);

		// Fill singleton registry
		singleRegistryEntries[i].offset = totalSizeBytes;
		singleRegistryEntries[i].sizeInBytes = singletonSizes[i];

		// Calculate next 16-byte aligned offset and update totalSizeBytes
		totalSizeBytes += u32(roundUpAligned(singletonSizes[i], 16));
	}

	// Components registry (+ 1 for active bit)
	u32 offsetComponentRegistryHeader = totalSizeBytes;
	ArrayHeader componentRegistryHeader;
	componentRegistryHeader.create<ComponentRegistryEntry>(numComponents + 1);
	u32 componentRegistrySizeBytes = calcArrayHeaderSizeBytes(sizeof(ComponentRegistryEntry), numComponents + 1);
	totalSizeBytes += componentRegistrySizeBytes;

	// Free entity ids list
	ArrayHeader freeEntityIdsHeader;
	freeEntityIdsHeader.create<u32>(maxNumEntities);
	u32 freeEntityIdsSizeBytes = calcArrayHeaderSizeBytes(sizeof(u32), maxNumEntities);
	totalSizeBytes += freeEntityIdsSizeBytes;

	// Entity masks
	ArrayHeader masksHeader;
	masksHeader.create<CompMask>(maxNumEntities);
	u32 masksSizeBytes = calcArrayHeaderSizeBytes(sizeof(CompMask), maxNumEntities);
	totalSizeBytes += masksSizeBytes;

	// Entity generations list
	ArrayHeader generationsHeader;
	generationsHeader.create<u8>(maxNumEntities);
	u32 generationsSizeBytes = calcArrayHeaderSizeBytes(sizeof(u8), maxNumEntities);
	totalSizeBytes += generationsSizeBytes;

	// Component arrays
	ComponentRegistryEntry componentRegistryEntries[64];
	ArrayHeader componentsArrayHeaders[64];
	for (auto& entry : componentRegistryEntries) entry = ComponentRegistryEntry::createUnsized();
	for (u32 i = 0; i < numComponents; i++) {

		// If the component size is 0, don't create ArrayHeader and don't increment total size
		if (componentSizes[i] == 0) continue;

		// Create ArrayHeader
		ArrayHeader& componentsHeader = componentsArrayHeaders[i + 1];
		componentsHeader.createUntyped(maxNumEntities, componentSizes[i]);
		componentsHeader.size = componentsHeader.capacity;

		// Create component registry entry
		componentRegistryEntries[i + 1] = ComponentRegistryEntry::createSized(totalSizeBytes);

		// Increment total size of ecs system
		u32 componentsSizeBytes = calcArrayHeaderSizeBytes(componentSizes[i], maxNumEntities);
		totalSizeBytes += componentsSizeBytes;
	}

	// Ensure size calculation is consistent and that allocated memory is big enough
	const u32 refSizeBytes = calcSizeOfGameStateBytes(
		numSingletons, singletonSizes, maxNumEntities, numComponents, componentSizes);
	sfz_assert_hard(refSizeBytes == totalSizeBytes);
	if (dstMemorySizeBytes < totalSizeBytes) return false;

	// Clear destination memory, cast it to GameStateHeader and start filling it in
	memset(dstMemory, 0, totalSizeBytes);
	GameStateHeader* state = static_cast<GameStateHeader*>(dstMemory);

	// Set game state header
	state->magicNumber = GAME_STATE_MAGIC_NUMBER;
	state->gameStateVersion = GAME_STATE_VERSION;
	state->stateSizeBytes = totalSizeBytes;
	state->numSingletons = numSingletons;
	state->numComponentTypes = numComponents + 1; // + 1 for active bit
	state->maxNumEntities = maxNumEntities;
	state->currentNumEntities = 0;
	state->offsetSingletonRegistry = sizeof(GameStateHeader);
	state->offsetComponentRegistry = offsetComponentRegistryHeader;
	state->offsetFreeEntityIdsList = state->offsetComponentRegistry + componentRegistrySizeBytes;
	state->offsetComponentMasks = state->offsetFreeEntityIdsList + freeEntityIdsSizeBytes;
	state->offsetEntityGenerationsList = state->offsetComponentMasks + masksSizeBytes;

	// Set singleton registry array header
	state->singletonRegistryArray()->createCopy(singletonRegistryHeader);
	state->singletonRegistryArray()->size = singletonRegistryHeader.capacity;

	// Fill singleton registry
	SingletonRegistryEntry* singletonRegistry =
		state->singletonRegistryArray()->data<SingletonRegistryEntry>();
	for (u32 i = 0; i < state->numSingletons; i++) {
		singletonRegistry[i] = singleRegistryEntries[i];
	}

	// Set component registry array header
	state->componentRegistryArray()->createCopy(componentRegistryHeader);
	state->componentRegistryArray()->size = componentRegistryHeader.capacity;

	// Fill component registry
	ComponentRegistryEntry* componentsRegistry =
		state->componentRegistryArray()->data<ComponentRegistryEntry>();
	for (u32 i = 0; i < state->numComponentTypes; i++) {
		componentsRegistry[i] = componentRegistryEntries[i];
	}

	// Set free entity ids header and fill list with free entity ids
	ArrayHeader* freeEntityIds = state->freeEntityIdsListArray();
	freeEntityIds->createCopy(freeEntityIdsHeader);
	for (int64_t entityId = int64_t(maxNumEntities - 1); entityId >= 0; entityId--) {
		freeEntityIds->add<u32>(u32(entityId));
	}

	// Set component masks header
	state->componentMasksArray()->createCopy(masksHeader);
	state->componentMasksArray()->size = masksHeader.capacity;

	// Set entity generations header and fill with ones
	ArrayHeader* generations = state->entityGenerationsListArray();
	generations->createCopy(generationsHeader);
	for (u32 i = 0; i < maxNumEntities; i++) {
		generations->at<u8>(i) = 1;
	}

	// Set component types array headers (i = 1 because first is active bit, which has no data)
	for (u32 i = 1; i < state->numComponentTypes; i++) {
		if (!componentsRegistry[i].componentTypeHasData()) continue;
		ArrayHeader* header = state->arrayAt(componentsRegistry[i].offset);
		header->createCopy(componentsArrayHeaders[i]);
	}

	return true;
}

} // namespace sfz
