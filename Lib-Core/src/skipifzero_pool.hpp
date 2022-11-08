// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#ifndef SKIPIFZERO_POOL_HPP
#define SKIPIFZERO_POOL_HPP
#pragma once

#include "sfz.h"
#include "sfz_cpp.hpp"

namespace sfz {

// PoolSlot
// ------------------------------------------------------------------------------------------------

constexpr u8 POOL_SLOT_ACTIVE_BIT_MASK = u8(0x80);
constexpr u8 POOL_SLOT_VERSION_MASK = u8(0x7F);

// Represents meta data about a slot in a Pool's value array.
//
// The first 7 bits stores the version of the slot. Each time the slot is "allocated" the version
// is increased. When it reaches 128 it wraps around to 1. Versions are in range [1, 127], 0 is
// reserved as invalid.
//
// The 8th bit is the "active" bit, i.e. whether the slot is currently in use (allocated) or not.
struct PoolSlot final {
	u8 bits;
	u8 version() const { return bits & POOL_SLOT_VERSION_MASK; }
	bool active() const { return (bits & POOL_SLOT_ACTIVE_BIT_MASK) != u8(0); }
};
static_assert(sizeof(PoolSlot) == 1, "PoolSlot is padded");

// Pool
// ------------------------------------------------------------------------------------------------

constexpr u32 POOL_MAX_CAPACITY = 1u << SFZ_HANDLE_INDEX_NUM_BITS;

// An sfz::Pool is a datastructure that is somewhat a mix between an array, an allocator and the
// entity allocation part of an ECS system. Basically, it's an array from which you allocate
// slots from. The array can have holes where you have deallocated objects. Each slot have an
// associated version number so stale handles can't be used when a slot has been deallocated and
// then allocated again.
//
// It is more of a low-level datastructure than either sfz::Array or sfz::HashMap, it is not as
// general purpose as either of those. The following restrictions apply:
//
//   * Will only call destructors when the entire pool is destroyed. When deallocating a slot it
//     will be set to "{}", or a user-defined value. The type must support this.
//   * Does not support resize, capacity must be specified in advance.
//   * Pointers are guaranteed stable because values are never moved/copied, due to above.
//   * There is no "_Local" variant, because then pointers would not be stable.
//
// It's possible to manually (and efficiently) iterate over the contents of a Pool. Example:
//
//     T* values = pool.data();
//     const PoolSlot* slots = pool.slots();
//     const u32 arraySize = pool.arraySize();
//     for (u32 idx = 0; idx < arraySize; idx++) {
//         PoolSlot slot = slots[idx];
//         T& value = values[idx];
//         // "value" will always be initialized here, but depending on your use case it's probably
//         // a bug to read/write to it. Mostly you will want to do the active check shown below.
//         if (!slot.active()) continue;
//         // Now value should be guaranteed safe to use regardless of your use case
//     }
//
// A Pool will never "shrink", i.e. arraySize() will never return a smaller value than before until
// you destroy() the pool completely.
template<typename T>
class Pool final {
public:
	SFZ_DECLARE_DROP_TYPE(Pool);

	explicit Pool(u32 capacity, SfzAllocator* allocator, SfzDbgInfo alloc_dbg) noexcept
	{
		this->init(capacity, allocator, alloc_dbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(u32 capacity, SfzAllocator* allocator, SfzDbgInfo alloc_dbg)
	{
		sfz_assert(capacity != 0); // We don't support resize, so this wouldn't make sense.
		sfz_assert(capacity <= POOL_MAX_CAPACITY);
		sfz_static_assert(alignof(T) <= 32);

		// Destroy previous pool
		this->destroy();
		
		// Calculate offsets, allocate memory and clear it
		const u32 alignment = 32;
		const u32 slots_offset = sfzRoundUpAlignedU32(sizeof(T) * capacity, alignment);
		const u32 free_indices_offset =
			slots_offset + sfzRoundUpAlignedU32(sizeof(PoolSlot) * capacity, alignment);
		const u32 num_bytes_needed =
			free_indices_offset + sfzRoundUpAlignedU32(sizeof(u32) * capacity, alignment);
		u8* memory = reinterpret_cast<u8*>(
			allocator->alloc(alloc_dbg, num_bytes_needed, alignment));
		memset(memory, 0, num_bytes_needed);

		// Set members
		m_allocator = allocator;
		m_capacity = capacity;
		m_data = reinterpret_cast<T*>(memory);
		m_slots = reinterpret_cast<PoolSlot*>(memory + slots_offset);
		m_free_indices = reinterpret_cast<u32*>(memory + free_indices_offset);
	}

	void destroy()
	{
		if (m_data != nullptr) {
			for (u32 i = 0; i < m_array_size; i++) {
				m_data[i].~T();
			}
			m_allocator->dealloc(m_data);
		}
		m_num_allocated = 0;
		m_array_size = 0;
		m_capacity = 0;
		m_data = nullptr;
		m_slots = nullptr;
		m_free_indices = nullptr;
		m_allocator = nullptr;
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	u32 numAllocated() const { return m_num_allocated; }
	u32 numHoles() const { return m_array_size - m_num_allocated; }
	u32 arraySize() const { return m_array_size; }
	u32 capacity() const { return m_capacity; }
	bool isFull() const { return m_num_allocated >= m_capacity; }
	const T* data() const { return m_data; }
	T* data() { return m_data; }
	const PoolSlot* slots() const { return m_slots; }
	SfzAllocator* allocator() const { return m_allocator; }

	PoolSlot getSlot(u32 idx) const { sfz_assert(idx < m_array_size); return m_slots[idx]; }
	u8 getVersion(u32 idx) const { sfz_assert(idx < m_array_size); return m_slots[idx].version(); }
	bool slotIsActive(u32 idx) const { sfz_assert(idx < m_array_size); return m_slots[idx].active(); }
	SfzHandle getHandle(u32 idx) const { sfz_assert(idx < m_array_size); return sfzHandleInit(idx, m_slots[idx].version()); }

	bool handleIsValid(SfzHandle handle) const
	{
		const u32 idx = handle.idx();
		if (idx >= m_array_size) return false;
		PoolSlot slot = m_slots[idx];
		if (!slot.active()) return false;
		if (handle.version() != slot.version()) return false;
		sfz_assert(slot.version() != u8(0));
		return true;
	}

	T* get(SfzHandle handle)
	{
		const u8 version = handle.version();
		const u32 idx = handle.idx();
		if (idx >= m_array_size) return nullptr;
		PoolSlot slot = m_slots[idx];
		if (slot.version() != version) return nullptr;
		if (!slot.active()) return nullptr;
		return &m_data[idx];
	}
	const T* get(SfzHandle handle) const { return const_cast<Pool<T>*>(this)->get(handle); }

	T& operator[] (SfzHandle handle) { T* v = get(handle); sfz_assert(v != nullptr); return *v; }
	const T& operator[] (SfzHandle handle) const { return (*const_cast<Pool<T>*>(this))[handle]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	SfzHandle allocate() { return allocateImpl<T>({}); }
	SfzHandle allocate(const T& value) { return allocateImpl<const T&>(value); }
	SfzHandle allocate(T&& value) { return allocateImpl<T>(sfz_move(value)); }

	void deallocate(SfzHandle handle) { return deallocateImpl<T>(handle, {}); }
	void deallocate(SfzHandle handle, const T& emptyValue) { return deallocateImpl<const T&>(handle, emptyValue); }
	void deallocate(SfzHandle handle, T&& emptyValue) { return deallocateImpl<T>(handle, sfz_move(emptyValue)); }

	void deallocate(u32 idx) { return deallocateImpl<T>(idx, {}); }
	void deallocate(u32 idx, const T& emptyValue) { return deallocateImpl<const T&>(idx, emptyValue); }
	void deallocate(u32 idx, T&& emptyValue) { return deallocateImpl<T>(idx, sfz_move(emptyValue)); }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	// Perfect forwarding: const reference: ForwardT == const T&, rvalue: ForwardT == T
	// std::forward<ForwardT>(value) will then return the correct version of value

	template<typename ForwardT>
	SfzHandle allocateImpl(ForwardT&& value)
	{
		sfz_assert(m_num_allocated < m_capacity);

		// Different path depending on if there are holes or not
		const u32 holes = numHoles();
		u32 idx = ~0u;
		if (holes > 0) {
			idx = m_free_indices[holes - 1];
			m_free_indices[holes - 1] = 0;
		
			// If we are reusing a slot the memory should already be constructed, therefore we
			// should use move/copy assignment in order to make sure we don't skip running a
			// destructor.
			m_data[idx] = sfz_forward(value);
		}
		else {
			idx = m_array_size;
			m_array_size += 1;

			// First time we are using this slot, memory is uninitialized and need to be
			// initialized before usage. Therefore use placement new move/copy constructor.
			new (m_data + idx) T(sfz_forward(value));
		}

		// Update number of allocated
		m_num_allocated += 1;
		sfz_assert(idx < m_array_size);
		sfz_assert(m_array_size <= m_capacity);
		sfz_assert(m_num_allocated <= m_array_size);

		// Update active bit and version in slot
		PoolSlot& slot = m_slots[idx];
		sfz_assert(!slot.active());
		u8 new_version = slot.bits + 1;
		if (new_version > 127) new_version = 1;
		slot.bits = POOL_SLOT_ACTIVE_BIT_MASK | new_version;

		// Create and return handle
		SfzHandle handle = sfzHandleInit(idx, new_version);
		return handle;
	}
	
	template<typename ForwardT>
	void deallocateImpl(SfzHandle handle, ForwardT&& emptyValue)
	{
		const u32 idx = handle.idx();
		sfz_assert(idx < m_array_size);
		sfz_assert(handle.version() == getVersion(idx));
		deallocateImpl<ForwardT>(idx, sfz_forward(emptyValue));
	}

	template<typename ForwardT>
	void deallocateImpl(u32 idx, ForwardT&& emptyValue)
	{
		sfz_assert(m_num_allocated > 0);
		sfz_assert(idx < m_array_size);
		PoolSlot& slot = m_slots[idx];
		sfz_assert(slot.active());
		sfz_assert(slot.version() != 0);

		// Set version and empty value
		slot.bits = slot.version(); // Remove active bit
		m_data[idx] = sfz_forward(emptyValue);
		m_num_allocated -= 1;

		// Store the new hole in free indices
		const u32 holes = numHoles();
		sfz_assert(holes > 0);
		m_free_indices[holes - 1] = idx;
	}
	
	// Private members
	// --------------------------------------------------------------------------------------------

	u32 m_num_allocated = 0;
	u32 m_array_size = 0;
	u32 m_capacity = 0;
	T* m_data = nullptr;
	PoolSlot* m_slots = nullptr;
	u32* m_free_indices = nullptr;
	SfzAllocator* m_allocator = nullptr;
};

} // namespace sfz

#endif
