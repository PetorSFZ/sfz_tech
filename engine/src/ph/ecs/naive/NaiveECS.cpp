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

#include "ph/ecs/naive/NaiveECS.hpp"

#include <cstring>

#include "ph/ecs/EcsEnums.hpp"

namespace ph {

// ECS: API
// ------------------------------------------------------------------------------------------------

uint32_t NaiveEcsHeader::createEntity() noexcept
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

bool NaiveEcsHeader::deleteEntity(uint32_t entity) noexcept
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
		memset(components + entity, 0, componentSize);
	}

	// Clear mask
	mask = ComponentMask::empty();

	// Add entity back to free entities list
	ArrayHeader* freeEntitiesList = this->freeEntitiesListArray();
	freeEntitiesList->add<uint32_t>(entity);

	return true;
}

uint32_t NaiveEcsHeader::cloneEntity(uint32_t entity) noexcept
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

ComponentMask* NaiveEcsHeader::componentMasks() noexcept
{
	return componentMasksArray()->data<ComponentMask>();
}

const ComponentMask* NaiveEcsHeader::componentMasks() const noexcept
{
	return componentMasksArray()->data<ComponentMask>();
}

uint8_t* NaiveEcsHeader::componentsUntyped(
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

const uint8_t* NaiveEcsHeader::componentsUntyped(
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

bool NaiveEcsHeader::addComponentUntyped(
	uint32_t entity, uint32_t componentType, const uint8_t* data, uint32_t dataSize) noexcept
{
	if (entity >= this->maxNumEntities) return false;

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
	memcpy(components + entity, data, dataSize);

	// Ensure bit is set in mask
	mask.setComponentType(componentType, true);

	return true;
}

bool NaiveEcsHeader::deleteComponent(uint32_t entity, uint32_t componentType) noexcept
{
	if (entity >= this->maxNumEntities) return false;

	// Return false if mask is not active
	ComponentMask& mask = this->componentMasks()[entity];
	if (!mask.active()) return false;

	// Get components array, return false if component type does not have data
	uint32_t componentSize = 0;
	uint8_t* components = componentsUntyped(componentType, componentSize);
	if (components == nullptr) return false;

	// Clear component
	memset(components + entity, 0, componentSize);

	// Clear bit in mask
	mask.setComponentType(componentType, false);

	return true;
}

// ECS functions
// ------------------------------------------------------------------------------------------------

EcsContainer createEcs(
	uint32_t maxNumEntities,
	const uint32_t* componentSizes,
	uint32_t numComponentTypes,
	Allocator* allocator) noexcept
{
	uint32_t totalSizeBytes = 0;

	// Ecs Header
	totalSizeBytes += sizeof(NaiveEcsHeader);

	// Registry (+ 1 for active bit)
	ArrayHeader registryHeader = ArrayHeader::create<ComponentRegistryEntry>(numComponentTypes + 1);
	uint32_t registrySizeBytes = registryHeader.numBytesNeededForArrayPlusHeader32Byte();
	totalSizeBytes += registrySizeBytes;

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
	EcsContainer container = EcsContainer::createRaw(totalSizeBytes, allocator);
	NaiveEcsHeader* ecs = container.getNaive();

	// Set ECS header
	ecs->ECS_TYPE = ECS_TYPE_NAIVE;
	ecs->ECS_VERSION = NAIVE_ECS_VERSION;
	ecs->ecsSizeBytes = totalSizeBytes;
	ecs->numComponentTypes = numComponentTypes + 1; // + 1 for active bit
	ecs->maxNumEntities = maxNumEntities;
	ecs->currentNumEntities = 0;
	ecs->offsetComponentRegistry = sizeof(NaiveEcsHeader);
	ecs->offsetFreeEntitiesList = ecs->offsetComponentRegistry + registrySizeBytes;
	ecs->offsetComponentMasks = ecs->offsetFreeEntitiesList + freeEntitiesSizeBytes;

	// Set registry array header
	*ecs->componentRegistryArray() = registryHeader;
	ecs->componentRegistryArray()->size = registryHeader.capacity;

	// Fill registry
	ComponentRegistryEntry* registry =
		ecs->componentRegistryArray()->data<ComponentRegistryEntry>();
	for (uint32_t i = 0; i < ecs->numComponentTypes; i++) {
		registry[i] = componentRegistryEntries[i];
	}

	// Set free entities header and fill list with free entity indices
	ArrayHeader* freeEntities = ecs->freeEntitiesListArray();
	*freeEntities = freeEntitiesHeader;
	for (int64_t entityIdx = int64_t(maxNumEntities - 1); entityIdx >= 0; entityIdx--) {
		freeEntities->add<uint32_t>(uint32_t(entityIdx));
	}

	// Set component masks header
	*ecs->componentMasksArray() = masksHeader;
	ecs->componentMasksArray()->size = masksHeader.capacity;

	// Set component types array headers (i = 1 because first is active bit, which has no data)
	for (uint32_t i = 1; i < ecs->numComponentTypes; i++) {
		if (!registry[i].componentTypeHasData()) continue;
		ArrayHeader* header = ecs->arrayAt(registry[i].offset);
		*header = componentsArrayHeaders[i];
	}

	return container;
}

} // namespace ph
