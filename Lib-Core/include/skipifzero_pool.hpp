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

#include "skipifzero.hpp"

namespace sfz {

// PoolHandle
// ------------------------------------------------------------------------------------------------

constexpr u32 POOL_HANDLE_INDEX_NUM_BITS = 24;
constexpr u32 POOL_MAX_CAPACITY = 1u << POOL_HANDLE_INDEX_NUM_BITS;
constexpr u32 POOL_HANDLE_INDEX_MASK = 0x00FFFFFFu; // 24 bits index
constexpr u32 POOL_HANDLE_VERSION_MASK = 0x7F000000u; // 7 bits version (1 bit reserved for active)

// A handle to an allocated slot in a Pool.
//
// A handle consists of an index (into the Pool's value array) and a version (version of the slot
// indexed in the Pool). If the version is not the same as what is stored in the Pool it means that
// the handle is stale and no longer valid.
//
// A version can be in the range [1, 127]. "0" is reserved as invalid. The 8th bit is reserved to
// store the active bit inside the Pool (unused in handles), see PoolSlot.
struct PoolHandle final {
	u32 bits = 0;

	u32 idx() const { return bits & POOL_HANDLE_INDEX_MASK; }
	u8 version() const { return u8((bits & POOL_HANDLE_VERSION_MASK) >> POOL_HANDLE_INDEX_NUM_BITS); }
	
	constexpr PoolHandle(u32 idx, u8 version)
	{
		sfz_assert((idx & POOL_HANDLE_INDEX_MASK) == idx);
		sfz_assert((version & u8(0x7F)) == version);
		bits = (u32(version) << POOL_HANDLE_INDEX_NUM_BITS) | idx;
	}

	constexpr PoolHandle() noexcept = default;
	constexpr PoolHandle(const PoolHandle&) noexcept = default;
	constexpr PoolHandle& operator= (const PoolHandle&) noexcept = default;
	~PoolHandle() noexcept = default;

	constexpr bool operator== (PoolHandle other) const { return this->bits == other.bits; }
	constexpr bool operator!= (PoolHandle other) const { return this->bits != other.bits; }
};
static_assert(sizeof(PoolHandle) == 4, "PoolHandle is padded");

// A "null" handle typically used as an error type or for uninitialized handles.
constexpr PoolHandle NULL_HANDLE = {};

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

	explicit Pool(u32 capacity, SfzAllocator* allocator, SfzDbgInfo allocDbg) noexcept
	{
		this->init(capacity, allocator, allocDbg);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(u32 capacity, SfzAllocator* allocator, SfzDbgInfo allocDbg)
	{
		sfz_assert(capacity != 0); // We don't support resize, so this wouldn't make sense.
		sfz_assert(capacity <= POOL_MAX_CAPACITY);
		sfz_assert(alignof(T) <= 32);

		// Destroy previous pool
		this->destroy();
		
		// Calculate offsets, allocate memory and clear it
		const u32 alignment = 32;
		const u32 slotsOffset = u32(roundUpAligned(sizeof(T) * capacity, alignment));
		const u32 freeIndicesOffset =
			slotsOffset + u32(roundUpAligned(sizeof(PoolSlot) * capacity, alignment));
		const u32 numBytesNeeded =
			freeIndicesOffset + u32(roundUpAligned(sizeof(u32) * capacity, alignment));
		u8* memory = reinterpret_cast<u8*>(
			allocator->alloc(allocDbg, numBytesNeeded, alignment));
		memset(memory, 0, numBytesNeeded);

		// Set members
		mAllocator = allocator;
		mCapacity = capacity;
		mData = reinterpret_cast<T*>(memory);
		mSlots = reinterpret_cast<PoolSlot*>(memory + slotsOffset);
		mFreeIndices = reinterpret_cast<u32*>(memory + freeIndicesOffset);
	}

	void destroy()
	{
		if (mData != nullptr) {
			// Only call destructors if T is not trivially destructible
			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (u32 i = 0; i < mArraySize; i++) {
					mData[i].~T();
				}
			}
			mAllocator->dealloc(mData);
		}
		mNumAllocated = 0;
		mArraySize = 0;
		mCapacity = 0;
		mData = nullptr;
		mSlots = nullptr;
		mFreeIndices = nullptr;
		mAllocator = nullptr;
	}

	// Getters
	// --------------------------------------------------------------------------------------------

	u32 numAllocated() const { return mNumAllocated; }
	u32 numHoles() const { return mArraySize - mNumAllocated; }
	u32 arraySize() const { return mArraySize; }
	u32 capacity() const { return mCapacity; }
	const T* data() const { return mData; }
	T* data() { return mData; }
	const PoolSlot* slots() const { return mSlots; }
	SfzAllocator* allocator() const { return mAllocator; }

	PoolSlot getSlot(u32 idx) const { sfz_assert(idx < mArraySize); return mSlots[idx]; }
	u8 getVersion(u32 idx) const { sfz_assert(idx < mArraySize); return mSlots[idx].version(); }
	bool slotIsActive(u32 idx) const { sfz_assert(idx < mArraySize); return mSlots[idx].active(); }

	bool handleIsValid(PoolHandle handle) const
	{
		const u32 idx = handle.idx();
		if (idx >= mArraySize) return false;
		PoolSlot slot = mSlots[idx];
		if (!slot.active()) return false;
		if (handle.version() != slot.version()) return false;
		sfz_assert(slot.version() != u8(0));
		return true;
	}

	T* get(PoolHandle handle)
	{
		const u8 version = handle.version();
		const u32 idx = handle.idx();
		if (idx >= mArraySize) return nullptr;
		PoolSlot slot = mSlots[idx];
		if (slot.version() != version) return nullptr;
		if (!slot.active()) return nullptr;
		return &mData[idx];
	}
	const T* get(PoolHandle handle) const { return const_cast<Pool<T>*>(this)->get(handle); }

	T& operator[] (PoolHandle handle) { T* v = get(handle); sfz_assert(v != nullptr); return *v; }
	const T& operator[] (PoolHandle handle) const { return (*const_cast<Pool<T>*>(this))[handle]; }

	// Methods
	// --------------------------------------------------------------------------------------------

	PoolHandle allocate() { return allocateImpl<T>({}); }
	PoolHandle allocate(const T& value) { return allocateImpl<const T&>(value); }
	PoolHandle allocate(T&& value) { return allocateImpl<T>(std::move(value)); }

	void deallocate(PoolHandle handle) { return deallocateImpl<T>(handle, {}); }
	void deallocate(PoolHandle handle, const T& emptyValue) { return deallocateImpl<const T&>(handle, emptyValue); }
	void deallocate(PoolHandle handle, T&& emptyValue) { return deallocateImpl<T>(handle, std::move(emptyValue)); }

	void deallocate(u32 idx) { return deallocateImpl<T>(idx, {}); }
	void deallocate(u32 idx, const T& emptyValue) { return deallocateImpl<const T&>(idx, emptyValue); }
	void deallocate(u32 idx, T&& emptyValue) { return deallocateImpl<T>(idx, std::move(emptyValue)); }

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	// Perfect forwarding: const reference: ForwardT == const T&, rvalue: ForwardT == T
	// std::forward<ForwardT>(value) will then return the correct version of value

	template<typename ForwardT>
	PoolHandle allocateImpl(ForwardT&& value)
	{
		sfz_assert(mNumAllocated < mCapacity);

		// Different path depending on if there are holes or not
		const u32 holes = numHoles();
		u32 idx = ~0u;
		if (holes > 0) {
			idx = mFreeIndices[holes - 1];
			mFreeIndices[holes - 1] = 0;
		
			// If we are reusing a slot the memory should already be constructed, therefore we
			// should use move/copy assignment in order to make sure we don't skip running a
			// destructor.
			mData[idx] = std::forward<ForwardT>(value);
		}
		else {
			idx = mArraySize;
			mArraySize += 1;

			// First time we are using this slot, memory is uninitialized and need to be
			// initialized before usage. Therefore use placement new move/copy constructor.
			new (mData + idx) T(std::forward<ForwardT>(value));
		}

		// Update number of allocated
		mNumAllocated += 1;
		sfz_assert(idx < mArraySize);
		sfz_assert(mArraySize <= mCapacity);
		sfz_assert(mNumAllocated <= mArraySize);

		// Update active bit and version in slot
		PoolSlot& slot = mSlots[idx];
		sfz_assert(!slot.active());
		u8 newVersion = slot.bits + 1;
		if (newVersion > 127) newVersion = 1;
		slot.bits = POOL_SLOT_ACTIVE_BIT_MASK | newVersion;

		// Create and return handle
		PoolHandle handle = PoolHandle(idx, newVersion);
		return handle;
	}
	
	template<typename ForwardT>
	void deallocateImpl(PoolHandle handle, ForwardT&& emptyValue)
	{
		const u32 idx = handle.idx();
		sfz_assert(idx < mArraySize);
		sfz_assert(handle.version() == getVersion(idx));
		deallocateImpl<ForwardT>(idx, std::forward<ForwardT>(emptyValue));
	}

	template<typename ForwardT>
	void deallocateImpl(u32 idx, ForwardT&& emptyValue)
	{
		sfz_assert(mNumAllocated > 0);
		sfz_assert(idx < mArraySize);
		PoolSlot& slot = mSlots[idx];
		sfz_assert(slot.active());
		sfz_assert(slot.version() != 0);

		// Set version and empty value
		slot.bits = slot.version(); // Remove active bit
		mData[idx] = std::forward<ForwardT>(emptyValue);
		mNumAllocated -= 1;

		// Store the new hole in free indices
		const u32 holes = numHoles();
		sfz_assert(holes > 0);
		mFreeIndices[holes - 1] = idx;
	}
	
	// Private members
	// --------------------------------------------------------------------------------------------

	u32 mNumAllocated = 0;
	u32 mArraySize = 0;
	u32 mCapacity = 0;
	T* mData = nullptr;
	PoolSlot* mSlots = nullptr;
	u32* mFreeIndices = nullptr;
	SfzAllocator* mAllocator = nullptr;
};

} // namespace sfz

#endif
