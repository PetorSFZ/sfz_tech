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

#include "ph/state/GameState.hpp"

#include <cstring>

namespace ph {

// GameState: Singleton state API
// ------------------------------------------------------------------------------------------------

uint8_t* GameStateHeader::singletonUntyped(uint32_t singletonIndex, uint32_t& singletonSizeBytesOut) noexcept
{
	// Get registry, return nullptr if component type is not in registry
	ArrayHeader* registry = this->singletonRegistryArray();
	sfz_assert_debug(singletonIndex < registry->size);
	if (registry->size <= singletonIndex) return nullptr;

	// Get registry entry, return nullptr if component type has no data
	SingletonRegistryEntry entry = registry->at<SingletonRegistryEntry>(singletonIndex);

	// Return singleton size and pointer
	singletonSizeBytesOut = entry.sizeInBytes;
	return reinterpret_cast<uint8_t*>(this + entry.offset);
}

const uint8_t* GameStateHeader::singletonUntyped(uint32_t singletonIndex, uint32_t& singletonSizeBytesOut) const noexcept
{
	// Get registry, return nullptr if component type is not in registry
	const ArrayHeader* registry = this->singletonRegistryArray();
	sfz_assert_debug(singletonIndex < registry->size);
	if (registry->size <= singletonIndex) return nullptr;

	// Get registry entry, return nullptr if component type has no data
	SingletonRegistryEntry entry = registry->at<SingletonRegistryEntry>(singletonIndex);

	// Return singleton size and pointer
	singletonSizeBytesOut = entry.sizeInBytes;
	return reinterpret_cast<const uint8_t*>(this + entry.offset);
}

// GameState: ECS API
// ------------------------------------------------------------------------------------------------

uint32_t GameStateHeader::createEntity() noexcept
{
	// Get free entity from free entities list
	ArrayHeader* freeEntitiesList = this->freeEntitiesListArray();
	uint32_t freeEntity = ~0u;
	bool freeEntityAvailable = freeEntitiesList->popGet(freeEntity);

	// Return ~0 if no free entity is available
	if (!freeEntityAvailable) return ~0u;

	// Increment number of entities
	currentNumEntities += 1;

	// Set component mask
	ArrayHeader* componentMasks = this->componentMasksArray();
	ComponentMask& mask = componentMasks->at<ComponentMask>(freeEntity);
	mask = ComponentMask::activeMask();

	return freeEntity;
}

bool GameStateHeader::deleteEntity(uint32_t entity) noexcept
{
	if (entity >= this->maxNumEntities) return false;

	// Get mask
	ArrayHeader* componentMasks = this->componentMasksArray();
	ComponentMask& mask = componentMasks->at<ComponentMask>(entity);

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
		memset(components + entity * componentSize, 0, componentSize);
	}

	// Clear mask
	mask = ComponentMask::empty();

	// Add entity back to free entities list
	ArrayHeader* freeEntitiesList = this->freeEntitiesListArray();
	freeEntitiesList->add<uint32_t>(entity);

	return true;
}

uint32_t GameStateHeader::cloneEntity(uint32_t entity) noexcept
{
	if (entity >= this->maxNumEntities) return ~0u;

	// Get mask, exit if entity does not exist
	ComponentMask* masks = this->componentMasks();
	ComponentMask mask = masks[entity];
	if (!mask.active()) return ~0u;

	// Create entity, exit out on failure
	uint32_t newEntity = this->createEntity();
	if (newEntity == ~0u) return ~0u;

	// Copy mask
	masks[newEntity] = mask;

	// Copy components
	for (uint32_t i = 1; i < this->numComponentTypes; i++) {
		if (!mask.fulfills(ComponentMask::fromType(i))) continue;

		// Get components array
		uint32_t componentSize = 0;
		uint8_t* components = this->componentsUntyped(i, componentSize);

		// Skip if component type does not have data
		if (components == nullptr) continue;

		// Copy component
		uint8_t* dst = components + newEntity * componentSize;
		const uint8_t* src = components + entity * componentSize;
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
	uint32_t entity, uint32_t componentType, const uint8_t* data, uint32_t dataSize) noexcept
{
	if (entity >= this->maxNumEntities) return false;
	if (componentType >= this->numComponentTypes) return false;

	// Return false if mask is not active
	ComponentMask& mask = this->componentMasks()[entity];
	if (!mask.active()) return false;

	// Get components array, return false if component type does not have data
	uint32_t componentSize = 0;
	uint8_t* components = componentsUntyped(componentType, componentSize);
	if (components == nullptr) return false;

	// Return false if dataSize does not match componentSize
	if (dataSize != componentSize) return false;

	// Copy component into ECS system
	memcpy(components + entity * componentSize, data, dataSize);

	// Ensure bit is set in mask
	mask.setComponentType(componentType, true);

	return true;
}

bool GameStateHeader::setComponentUnsized(
	uint32_t entity, uint32_t componentType, bool value) noexcept
{
	if (entity >= this->maxNumEntities) return false;
	if (componentType >= this->numComponentTypes) return false;

	// Return false if mask is not active
	ComponentMask& mask = this->componentMasks()[entity];
	if (!mask.active()) return false;

	// Get components array, return false if component type have data
	uint32_t componentSize = 0;
	uint8_t* components = componentsUntyped(componentType, componentSize);
	if (components != nullptr) return false;

	// Set bit in mask
	mask.setComponentType(componentType, value);

	return true;
}

bool GameStateHeader::deleteComponent(uint32_t entity, uint32_t componentType) noexcept
{
	if (entity >= this->maxNumEntities) return false;

	// Return false if mask is not active
	ComponentMask& mask = this->componentMasks()[entity];
	if (!mask.active()) return false;

	// Get components array, forward to setComponentUnsized() if component type does not have data
	uint32_t componentSize = 0;
	uint8_t* components = componentsUntyped(componentType, componentSize);
	if (components == nullptr) return this->setComponentUnsized(entity, componentType, false);

	// Clear component
	memset(components + entity * componentSize, 0, componentSize);

	// Clear bit in mask
	mask.setComponentType(componentType, false);

	return true;
}

// Game state functions
// ------------------------------------------------------------------------------------------------

GameStateContainer createGameState(
	uint32_t numSingletonStructs,
	const uint32_t* singletonStructSizes,
	uint32_t maxNumEntities,
	const uint32_t* componentSizes,
	uint32_t numComponentTypes,
	Allocator* allocator) noexcept
{
	sfz_assert_debug(numSingletonStructs <= 64);

	uint32_t totalSizeBytes = 0;

	// GameState Header
	totalSizeBytes += sizeof(GameStateHeader);

	// Singleton registry
	ArrayHeader singletonRegistryHeader = ArrayHeader::create<SingletonRegistryEntry>(numSingletonStructs);
	uint32_t singletonRegistrySizeBytes = singletonRegistryHeader.numBytesNeededForArrayPlusHeader32Byte();
	totalSizeBytes += singletonRegistrySizeBytes;

	// Singleton structs
	SingletonRegistryEntry singleRegistryEntries[64] = {};
	for (uint32_t i = 0; i < numSingletonStructs; i++) {
		sfz_assert_debug(singletonStructSizes[i] != 0);

		// Fill singleton registry
		singleRegistryEntries[i].offset = totalSizeBytes;
		singleRegistryEntries[i].sizeInBytes = singletonStructSizes[i];

		// Calculate next 32-byte aligned offset and update totalSizeBytes
		uint32_t bytesBeforePadding = singletonStructSizes[i];
		uint32_t padding = 32 - (bytesBeforePadding & 0x1F); // bytesBeforePadding % 32
		if (padding == 32) padding = 0;
		uint32_t bytesIncludingPadding = bytesBeforePadding + padding;
		totalSizeBytes += bytesIncludingPadding;
	}
	
	// Components registry (+ 1 for active bit)
	uint32_t offsetComponentRegistryHeader = totalSizeBytes;
	ArrayHeader componentRegistryHeader = ArrayHeader::create<ComponentRegistryEntry>(numComponentTypes + 1);
	uint32_t componentRegistrySizeBytes = componentRegistryHeader.numBytesNeededForArrayPlusHeader32Byte();
	totalSizeBytes += componentRegistrySizeBytes;

	// Free entities list
	ArrayHeader freeEntitiesHeader = ArrayHeader::create<uint32_t>(maxNumEntities);
	uint32_t freeEntitiesSizeBytes = freeEntitiesHeader.numBytesNeededForArrayPlusHeader32Byte();
	totalSizeBytes += freeEntitiesSizeBytes;

	// Entity masks
	ArrayHeader masksHeader = ArrayHeader::create<ComponentMask>(maxNumEntities);
	uint32_t masksSizeBytes = masksHeader.numBytesNeededForArrayPlusHeader32Byte();
	totalSizeBytes += masksSizeBytes;

	// Component arrays
	ComponentRegistryEntry componentRegistryEntries[64];
	ArrayHeader componentsArrayHeaders[64];
	for (auto& entry : componentRegistryEntries) entry = ComponentRegistryEntry::createUnsized();
	for (uint32_t i = 0; i < numComponentTypes; i++) {

		// If the component size is 0, don't create ArrayHeader and don't increment total size
		if (componentSizes[i] == 0) continue;

		// Create ArrayHeader
		ArrayHeader& componentsHeader = componentsArrayHeaders[i + 1];
		componentsHeader = ArrayHeader::createUntyped(maxNumEntities, componentSizes[i]);
		componentsHeader.size = componentsHeader.capacity;

		// Create component registry entry
		componentRegistryEntries[i + 1] = ComponentRegistryEntry::createSized(totalSizeBytes);

		// Increment total size of ecs system
		uint32_t componentsSizeBytes = componentsHeader.numBytesNeededForArrayPlusHeader32Byte();
		totalSizeBytes += componentsSizeBytes;
	}

	// Allocate memory
	GameStateContainer container = GameStateContainer::createRaw(totalSizeBytes, allocator);
	GameStateHeader* state = container.getHeader();

	// Set game state header
	state->magicNumber = GAME_STATE_MAGIC_NUMBER;
	state->gameStateVersion = GAME_STATE_VERSION;
	state->stateSizeBytes = totalSizeBytes;
	state->numSingletons = numSingletonStructs;
	state->numComponentTypes = numComponentTypes + 1; // + 1 for active bit
	state->maxNumEntities = maxNumEntities;
	state->currentNumEntities = 0;
	state->offsetSingletonRegistry = sizeof(GameStateHeader);
	state->offsetComponentRegistry = offsetComponentRegistryHeader;
	state->offsetFreeEntitiesList = state->offsetComponentRegistry + componentRegistrySizeBytes;
	state->offsetComponentMasks = state->offsetFreeEntitiesList + freeEntitiesSizeBytes;

	// Set singleton registry array header
	*state->singletonRegistryArray() = singletonRegistryHeader;
	state->singletonRegistryArray()->size = singletonRegistryHeader.capacity;

	// Fill singleton registry
	SingletonRegistryEntry* singletonRegistry =
		state->singletonRegistryArray()->data<SingletonRegistryEntry>();
	for (uint32_t i = 0; i < state->numSingletons; i++) {
		singletonRegistry[i] = singleRegistryEntries[i];
	}

	// Set component registry array header
	*state->componentRegistryArray() = componentRegistryHeader;
	state->componentRegistryArray()->size = componentRegistryHeader.capacity;

	// Fill component registry
	ComponentRegistryEntry* componentsRegistry =
		state->componentRegistryArray()->data<ComponentRegistryEntry>();
	for (uint32_t i = 0; i < state->numComponentTypes; i++) {
		componentsRegistry[i] = componentRegistryEntries[i];
	}

	// Set free entities header and fill list with free entity indices
	ArrayHeader* freeEntities = state->freeEntitiesListArray();
	*freeEntities = freeEntitiesHeader;
	for (int64_t entityIdx = int64_t(maxNumEntities - 1); entityIdx >= 0; entityIdx--) {
		freeEntities->add<uint32_t>(uint32_t(entityIdx));
	}

	// Set component masks header
	*state->componentMasksArray() = masksHeader;
	state->componentMasksArray()->size = masksHeader.capacity;

	// Set component types array headers (i = 1 because first is active bit, which has no data)
	for (uint32_t i = 1; i < state->numComponentTypes; i++) {
		if (!componentsRegistry[i].componentTypeHasData()) continue;
		ArrayHeader* header = state->arrayAt(componentsRegistry[i].offset);
		*header = componentsArrayHeaders[i];
	}

	return container;
}

} // namespace ph
