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

uint8_t* GameStateHeader::singletonUntyped(uint32_t singletonIndex, uint32_t& singletonSizeBytesOut) noexcept
{
	// Get registry, return nullptr if component type is not in registry
	ArrayHeader* registry = this->singletonRegistryArray();
	sfz_assert(singletonIndex < registry->size);
	if (registry->size <= singletonIndex) return nullptr;

	// Get registry entry, return nullptr if component type has no data
	SingletonRegistryEntry entry = registry->at<SingletonRegistryEntry>(singletonIndex);

	// Return singleton size and pointer
	singletonSizeBytesOut = entry.sizeInBytes;
	return reinterpret_cast<uint8_t*>(this) + entry.offset;
}

const uint8_t* GameStateHeader::singletonUntyped(uint32_t singletonIndex, uint32_t& singletonSizeBytesOut) const noexcept
{
	// Get registry, return nullptr if component type is not in registry
	const ArrayHeader* registry = this->singletonRegistryArray();
	sfz_assert(singletonIndex < registry->size);
	if (registry->size <= singletonIndex) return nullptr;

	// Get registry entry, return nullptr if component type has no data
	SingletonRegistryEntry entry = registry->at<SingletonRegistryEntry>(singletonIndex);

	// Return singleton size and pointer
	singletonSizeBytesOut = entry.sizeInBytes;
	return reinterpret_cast<const uint8_t*>(this) + entry.offset;
}

// GameState: ECS API
// ------------------------------------------------------------------------------------------------

Entity GameStateHeader::createEntity() noexcept
{
	// Get free entity from free entities list
	ArrayHeader* freeEntitiesList = this->freeEntityIdsListArray();
	uint32_t freeEntityId = ~0u;
	bool freeEntityAvailable = freeEntitiesList->popGet(freeEntityId);

	// Return Entity::invalid() if no free entity id is available
	if (!freeEntityAvailable) return Entity::invalid();

	// Increment number of entities
	currentNumEntities += 1;

	// Set component mask
	ArrayHeader* componentMasks = this->componentMasksArray();
	ComponentMask& mask = componentMasks->at<ComponentMask>(freeEntityId);
	mask = ComponentMask::activeMask();

	// Get generation
	uint8_t* generations = this->entityGenerations();
	uint8_t generation = generations[freeEntityId];

	// Return entity
	return Entity::create(freeEntityId, generation);
}

bool GameStateHeader::deleteEntity(Entity entity) noexcept
{
	if (!this->checkGeneration(entity)) return false;
	return this->deleteEntity(entity.id());
}

bool GameStateHeader::deleteEntity(uint32_t entityId) noexcept
{
	if (entityId >= this->maxNumEntities) return false;

	// Get mask
	ArrayHeader* componentMasks = this->componentMasksArray();
	ComponentMask& mask = componentMasks->at<ComponentMask>(entityId);

	// Get generations
	uint8_t* generations = this->entityGenerations();
	uint8_t& generation = generations[entityId];

	// Return false if entity is not active
	if (!mask.active()) return false;

	// Decrement number of entities
	if (currentNumEntities != 0) currentNumEntities -= 1;

	// Remove all associated components
	for (uint32_t i = 0; i < this->numComponentTypes; i++) {

		// Get components array for type i, skip if does not exist
		uint32_t componentSize = 0;
		uint8_t* components = componentsUntyped(i, componentSize);
		if (components == nullptr) continue;

		// Clear component
		memset(components + entityId * componentSize, 0, componentSize);
	}

	// Clear mask
	mask = ComponentMask::empty();

	// Increment generation
	generation += 1;

	// Add entity id back to free entities list
	ArrayHeader* freeEntityIdsList = this->freeEntityIdsListArray();
	freeEntityIdsList->add<uint32_t>(entityId);

	return true;
}

Entity GameStateHeader::cloneEntity(Entity entity) noexcept
{
	// Grab id and generation
	uint32_t entityId = entity.id();
	uint8_t entityGeneration = entity.generation();

	// If id is out of range, return invalid entity
	if (entityId >= this->maxNumEntities) return Entity::invalid();

	// Get mask, exit if entity does not exist
	ComponentMask* masks = this->componentMasks();
	ComponentMask mask = masks[entityId];
	if (!mask.active()) return Entity::invalid();

	// Get generation, exit if entity has wrong generation
	uint8_t* generations = this->entityGenerations();
	uint8_t expectedGeneration = generations[entityId];
	if (entityGeneration != expectedGeneration) return Entity::invalid();

	// Create entity, exit out on failure
	Entity newEntity = this->createEntity();
	if (newEntity == Entity::invalid()) return Entity::invalid();

	// Copy mask
	uint32_t newEntityId = newEntity.id();
	masks[newEntityId] = mask;

	// Copy components
	for (uint32_t i = 1; i < this->numComponentTypes; i++) {
		if (!mask.fulfills(ComponentMask::fromType(i))) continue;

		// Get components array
		uint32_t componentSize = 0;
		uint8_t* components = this->componentsUntyped(i, componentSize);

		// Skip if component type does not have data
		if (components == nullptr) continue;

		// Copy component
		uint8_t* dst = components + newEntityId * componentSize;
		const uint8_t* src = components + entityId * componentSize;
		memcpy(dst, src, componentSize);
	}

	return newEntity;
}

ComponentMask* GameStateHeader::componentMasks() noexcept
{
	return componentMasksArray()->data<ComponentMask>();
}

const ComponentMask* GameStateHeader::componentMasks() const noexcept
{
	return componentMasksArray()->data<ComponentMask>();
}

uint8_t* GameStateHeader::entityGenerations() noexcept
{
	return entityGenerationsListArray()->data<uint8_t>();
}

const uint8_t* GameStateHeader::entityGenerations() const noexcept
{
	return entityGenerationsListArray()->data<uint8_t>();
}

uint8_t GameStateHeader::getGeneration(uint32_t entityId) const noexcept
{
	sfz_assert(entityId < this->maxNumEntities);
	const uint8_t* generations = this->entityGenerations();
	return generations[entityId];
}

bool GameStateHeader::checkGeneration(Entity entity) const noexcept
{
	uint8_t expectedGeneration = this->getGeneration(entity.id());
	return expectedGeneration == entity.generation();
}

bool GameStateHeader::checkEntityValid(Entity entity) const noexcept
{
	// Check if in bounds
	uint32_t entityId = entity.id();
	if (entityId >= this->maxNumEntities) return false;

	// Check if active
	const ComponentMask* masks = componentMasks();
	ComponentMask mask = masks[entityId];
	if (!mask.active()) return false;

	// Check if correct generation
	if (!checkGeneration(entity)) return false;

	return true;
}

uint8_t* GameStateHeader::componentsUntyped(
	uint32_t componentType, uint32_t& componentSizeBytesOut) noexcept
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

const uint8_t* GameStateHeader::componentsUntyped(
	uint32_t componentType, uint32_t& componentSizeBytesOut) const noexcept
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
	Entity entity, uint32_t componentType, const uint8_t* data, uint32_t dataSize) noexcept
{
	uint32_t entityId = entity.id();
	if (entityId >= this->maxNumEntities) return false;
	if (!checkGeneration(entity)) return false;
	if (componentType >= this->numComponentTypes) return false;

	// Return false if mask is not active
	ComponentMask& mask = this->componentMasks()[entityId];
	if (!mask.active()) return false;

	// Get components array, return false if component type does not have data
	uint32_t componentSize = 0;
	uint8_t* components = componentsUntyped(componentType, componentSize);
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
	Entity entity, uint32_t componentType, bool value) noexcept
{
	uint32_t entityId = entity.id();
	if (entityId >= this->maxNumEntities) return false;
	if (!checkGeneration(entity)) return false;
	if (componentType >= this->numComponentTypes) return false;

	// Return false if mask is not active
	ComponentMask& mask = this->componentMasks()[entityId];
	if (!mask.active()) return false;

	// Get components array, return false if component type have data
	uint32_t componentSize = 0;
	uint8_t* components = componentsUntyped(componentType, componentSize);
	if (components != nullptr) return false;

	// Set bit in mask
	mask.setComponentType(componentType, value);

	return true;
}

bool GameStateHeader::deleteComponent(Entity entity, uint32_t componentType) noexcept
{
	uint32_t entityId = entity.id();
	if (entityId >= this->maxNumEntities) return false;
	if (!checkGeneration(entity)) return false;

	// Return false if mask is not active
	ComponentMask& mask = this->componentMasks()[entityId];
	if (!mask.active()) return false;

	// Get components array, forward to setComponentUnsized() if component type does not have data
	uint32_t componentSize = 0;
	uint8_t* components = componentsUntyped(componentType, componentSize);
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
	uint32_t dstMemorySizeBytes,
	uint32_t numSingletons,
	const uint32_t* singletonSizes,
	uint32_t maxNumEntities,
	uint32_t numComponents,
	const uint32_t* componentSizes) noexcept
{
	sfz_assert(numSingletons <= 64);
	sfz_assert(maxNumEntities <= GAME_STATE_ECS_MAX_NUM_ENTITIES);
	sfz_assert(numComponents <= 63); // Not 64 because one is reserved for active bit

	uint32_t totalSizeBytes = 0;

	// GameState Header
	totalSizeBytes += sizeof(GameStateHeader);

	// Singleton registry
	ArrayHeader singletonRegistryHeader;
	singletonRegistryHeader.create<SingletonRegistryEntry>(numSingletons);
	uint32_t singletonRegistrySizeBytes = calcArrayHeaderSizeBytes(sizeof(SingletonRegistryEntry), numSingletons);
	totalSizeBytes += singletonRegistrySizeBytes;

	// Singleton structs
	SingletonRegistryEntry singleRegistryEntries[64] = {};
	for (uint32_t i = 0; i < numSingletons; i++) {
		sfz_assert(singletonSizes[i] != 0);

		// Fill singleton registry
		singleRegistryEntries[i].offset = totalSizeBytes;
		singleRegistryEntries[i].sizeInBytes = singletonSizes[i];

		// Calculate next 16-byte aligned offset and update totalSizeBytes
		totalSizeBytes += uint32_t(roundUpAligned(singletonSizes[i], 16));
	}

	// Components registry (+ 1 for active bit)
	uint32_t offsetComponentRegistryHeader = totalSizeBytes;
	ArrayHeader componentRegistryHeader;
	componentRegistryHeader.create<ComponentRegistryEntry>(numComponents + 1);
	uint32_t componentRegistrySizeBytes = calcArrayHeaderSizeBytes(sizeof(ComponentRegistryEntry), numComponents + 1);
	totalSizeBytes += componentRegistrySizeBytes;

	// Free entity ids list
	ArrayHeader freeEntityIdsHeader;
	freeEntityIdsHeader.create<uint32_t>(maxNumEntities);
	uint32_t freeEntityIdsSizeBytes = calcArrayHeaderSizeBytes(sizeof(uint32_t), maxNumEntities);
	totalSizeBytes += freeEntityIdsSizeBytes;

	// Entity masks
	ArrayHeader masksHeader;
	masksHeader.create<ComponentMask>(maxNumEntities);
	uint32_t masksSizeBytes = calcArrayHeaderSizeBytes(sizeof(ComponentMask), maxNumEntities);
	totalSizeBytes += masksSizeBytes;

	// Entity generations list
	ArrayHeader generationsHeader;
	generationsHeader.create<uint8_t>(maxNumEntities);
	uint32_t generationsSizeBytes = calcArrayHeaderSizeBytes(sizeof(uint8_t), maxNumEntities);
	totalSizeBytes += generationsSizeBytes;

	// Component arrays
	ComponentRegistryEntry componentRegistryEntries[64];
	ArrayHeader componentsArrayHeaders[64];
	for (auto& entry : componentRegistryEntries) entry = ComponentRegistryEntry::createUnsized();
	for (uint32_t i = 0; i < numComponents; i++) {

		// If the component size is 0, don't create ArrayHeader and don't increment total size
		if (componentSizes[i] == 0) continue;

		// Create ArrayHeader
		ArrayHeader& componentsHeader = componentsArrayHeaders[i + 1];
		componentsHeader.createUntyped(maxNumEntities, componentSizes[i]);
		componentsHeader.size = componentsHeader.capacity;

		// Create component registry entry
		componentRegistryEntries[i + 1] = ComponentRegistryEntry::createSized(totalSizeBytes);

		// Increment total size of ecs system
		uint32_t componentsSizeBytes = calcArrayHeaderSizeBytes(componentSizes[i], maxNumEntities);
		totalSizeBytes += componentsSizeBytes;
	}

	// Ensure size calculation is consistent and that allocated memory is big enough
	const uint32_t refSizeBytes = calcSizeOfGameStateBytes(
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
	for (uint32_t i = 0; i < state->numSingletons; i++) {
		singletonRegistry[i] = singleRegistryEntries[i];
	}

	// Set component registry array header
	state->componentRegistryArray()->createCopy(componentRegistryHeader);
	state->componentRegistryArray()->size = componentRegistryHeader.capacity;

	// Fill component registry
	ComponentRegistryEntry* componentsRegistry =
		state->componentRegistryArray()->data<ComponentRegistryEntry>();
	for (uint32_t i = 0; i < state->numComponentTypes; i++) {
		componentsRegistry[i] = componentRegistryEntries[i];
	}

	// Set free entity ids header and fill list with free entity ids
	ArrayHeader* freeEntityIds = state->freeEntityIdsListArray();
	freeEntityIds->createCopy(freeEntityIdsHeader);
	for (int64_t entityId = int64_t(maxNumEntities - 1); entityId >= 0; entityId--) {
		freeEntityIds->add<uint32_t>(uint32_t(entityId));
	}

	// Set component masks header
	state->componentMasksArray()->createCopy(masksHeader);
	state->componentMasksArray()->size = masksHeader.capacity;

	// Set entity generations header and fill with zeroes
	ArrayHeader* generations = state->entityGenerationsListArray();
	generations->createCopy(generationsHeader);

	// Small hack, start entity 0 at generation 1. To reduce risk of default constructed entities
	// (entity 0, generation 0) pointing at something valid.
	generations->at<uint8_t>(0) += 1;

	// Set component types array headers (i = 1 because first is active bit, which has no data)
	for (uint32_t i = 1; i < state->numComponentTypes; i++) {
		if (!componentsRegistry[i].componentTypeHasData()) continue;
		ArrayHeader* header = state->arrayAt(componentsRegistry[i].offset);
		header->createCopy(componentsArrayHeaders[i]);
	}

	return true;
}

} // namespace sfz
