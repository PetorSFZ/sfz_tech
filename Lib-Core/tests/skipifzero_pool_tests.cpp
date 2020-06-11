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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "utest.h"
#undef near
#undef far

#include "skipifzero.hpp"
#include "skipifzero_allocators.hpp"
#include "skipifzero_pool.hpp"

// PoolHandle tests
// ------------------------------------------------------------------------------------------------

UTEST(PoolHandle, tests)
{
	// Default handle
	{
		sfz::PoolHandle handle;
		ASSERT_TRUE(handle.rawHandle == 0u);
		ASSERT_TRUE(handle.version() == 0u);
		ASSERT_TRUE(handle.idx() == 0u);
		ASSERT_TRUE(!handle.valid());
		ASSERT_TRUE(!handle.internalFreeBit());

		handle.internalSetFreeBit(true);
		ASSERT_TRUE(handle.rawHandle == (1u << 31u));
		ASSERT_TRUE(handle.version() == 0u);
		ASSERT_TRUE(handle.idx() == 0u);
		ASSERT_TRUE(!handle.valid());
		ASSERT_TRUE(handle.internalFreeBit());

		handle.internalSetFreeBit(false);
		ASSERT_TRUE(handle.rawHandle == 0u);
		ASSERT_TRUE(handle.version() == 0u);
		ASSERT_TRUE(handle.idx() == 0u);
		ASSERT_TRUE(!handle.valid());
		ASSERT_TRUE(!handle.internalFreeBit());
	}

	{
		sfz::PoolHandle handle = sfz::PoolHandle::create(0, 1);
		ASSERT_TRUE(handle.rawHandle == (sfz::POOL_IDX_MAX + 1));
		ASSERT_TRUE(handle.version() == 1u);
		ASSERT_TRUE(handle.idx() == 0u);
		ASSERT_TRUE(handle.valid());
		ASSERT_TRUE(!handle.internalFreeBit());

		handle.internalSetFreeBit(true);
		ASSERT_TRUE(handle.version() == 1u);
		ASSERT_TRUE(handle.idx() == 0u);
		ASSERT_TRUE(!handle.valid());
		ASSERT_TRUE(handle.internalFreeBit());

		handle.internalSetFreeBit(false);
		ASSERT_TRUE(handle.rawHandle == (sfz::POOL_IDX_MAX + 1));
		ASSERT_TRUE(handle.version() == 1u);
		ASSERT_TRUE(handle.idx() == 0u);
		ASSERT_TRUE(handle.valid());
		ASSERT_TRUE(!handle.internalFreeBit());
	}

	{
		sfz::PoolHandle handle = sfz::PoolHandle::create(sfz::POOL_IDX_MAX, sfz::POOL_VERSION_MAX);
		ASSERT_TRUE(handle.rawHandle == (sfz::POOL_IDX_MASK | sfz::POOL_VERSION_MASK));
		ASSERT_TRUE(handle.version() == sfz::POOL_VERSION_MAX);
		ASSERT_TRUE(handle.idx() == sfz::POOL_IDX_MAX);
		ASSERT_TRUE(handle.valid());
		ASSERT_TRUE(!handle.internalFreeBit());

		handle.internalSetFreeBit(true);
		ASSERT_TRUE(handle.rawHandle == UINT32_MAX);
		ASSERT_TRUE(handle.version() == sfz::POOL_VERSION_MAX);
		ASSERT_TRUE(handle.idx() == sfz::POOL_IDX_MAX);
		ASSERT_TRUE(!handle.valid());
		ASSERT_TRUE(handle.internalFreeBit());

		handle.internalSetFreeBit(false);
		ASSERT_TRUE(handle.rawHandle == (sfz::POOL_IDX_MASK | sfz::POOL_VERSION_MASK));
		ASSERT_TRUE(handle.version() == sfz::POOL_VERSION_MAX);
		ASSERT_TRUE(handle.idx() == sfz::POOL_IDX_MAX);
		ASSERT_TRUE(handle.valid());
		ASSERT_TRUE(!handle.internalFreeBit());
	}

	{
		sfz::PoolHandle handle = sfz::PoolHandle::create(0, sfz::POOL_VERSION_MAX);
		ASSERT_TRUE(handle.rawHandle == sfz::POOL_VERSION_MASK);
		ASSERT_TRUE(handle.version() == sfz::POOL_VERSION_MAX);
		ASSERT_TRUE(handle.idx() == 0u);
		ASSERT_TRUE(handle.valid());
		ASSERT_TRUE(!handle.internalFreeBit());

		handle.internalSetFreeBit(true);
		ASSERT_TRUE(handle.version() == sfz::POOL_VERSION_MAX);
		ASSERT_TRUE(handle.idx() == 0u);
		ASSERT_TRUE(!handle.valid());
		ASSERT_TRUE(handle.internalFreeBit());

		handle.internalSetFreeBit(false);
		ASSERT_TRUE(handle.rawHandle == sfz::POOL_VERSION_MASK);
		ASSERT_TRUE(handle.version() == sfz::POOL_VERSION_MAX);
		ASSERT_TRUE(handle.idx() == 0u);
		ASSERT_TRUE(handle.valid());
		ASSERT_TRUE(!handle.internalFreeBit());
	}
}

// Pool tests
// ------------------------------------------------------------------------------------------------

UTEST(PoolLocal, default_constructor)
{
	sfz::PoolLocal<float, 7> floatPool;
	
	/*sfz::Array<float> floatArray;
	ASSERT_TRUE(floatArray.size() == 0);
	ASSERT_TRUE(floatArray.capacity() == 0);
	ASSERT_TRUE(floatArray.data() == nullptr);
	ASSERT_TRUE(floatArray.allocator() == nullptr);*/
}
