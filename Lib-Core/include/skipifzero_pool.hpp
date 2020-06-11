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

#include "skipifzero.hpp"

namespace sfz {

// PoolHandle
// ------------------------------------------------------------------------------------------------

constexpr uint32_t POOL_IDX_NUM_BITS = 24;
constexpr uint32_t POOL_IDX_MAX = (1 << POOL_IDX_NUM_BITS) - 1; // 2^24 - 1
constexpr uint32_t POOL_IDX_MASK = POOL_IDX_MAX;

constexpr uint32_t POOL_VERSION_NUM_BITS = 32 - POOL_IDX_NUM_BITS - 1; // 1 bit reserved
constexpr uint32_t POOL_VERSION_MAX = (1 << POOL_VERSION_NUM_BITS) - 1;
constexpr uint32_t POOL_VERSION_MASK = POOL_VERSION_MAX << POOL_IDX_NUM_BITS;

constexpr uint32_t POOL_FREE_BIT_MASK = ~(POOL_VERSION_MASK | POOL_IDX_MASK);

struct PoolHandle final {
	uint32_t rawHandle = 0;

	static PoolHandle create(uint32_t idx, uint8_t version) {
		sfz_assert(idx <= POOL_IDX_MAX);
		sfz_assert(0 < version && version <= POOL_VERSION_MAX);
		version &= POOL_VERSION_MAX;
		idx &= POOL_IDX_MASK;
		return { (uint32_t(version) << POOL_IDX_NUM_BITS) | idx };
	}

	bool valid() const { return (rawHandle & POOL_VERSION_MASK) != 0u && !internalFreeBit(); }
	uint32_t idx() const { return rawHandle & POOL_IDX_MASK; }
	uint8_t version() const { return uint8_t((rawHandle & POOL_VERSION_MASK) >> POOL_IDX_NUM_BITS); }
	
	// Free bit, used internally by the Pool
	bool internalFreeBit() const { return (rawHandle & POOL_FREE_BIT_MASK) != 0u; }
	void internalSetFreeBit(bool bit) { rawHandle = (rawHandle & ~POOL_FREE_BIT_MASK) | ((bit ? 1u : 0u) << 31u); }

	bool operator== (PoolHandle o) const { return this->rawHandle == o.rawHandle; }
	bool operator!= (PoolHandle o) const { return this->rawHandle != o.rawHandle; }
};
static_assert(sizeof(PoolHandle) == 4, "PoolHandle is padded");

// PoolHeader
// ------------------------------------------------------------------------------------------------

struct PoolHeader final {
	uint64_t dataOffset;
	uint64_t metaOffset;
	uint32_t size;
	uint32_t elementSize;
	uint32_t capacity;
	uint32_t nextFreeIdx;
};
static_assert(sizeof(PoolHeader) == 32, "PoolHeader is not 32-byte");

// PoolLocal
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
struct alignas(16) PoolLocal final {
	PoolHeader header = {};
	T data[N] = {};
	PoolHandle metaData[N];

	PoolLocal()
	{

	}
	PoolLocal(const PoolLocal&) = default;
	PoolLocal& operator= (const PoolLocal&) = default;
};



} // namespace sfz

#endif
