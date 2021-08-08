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

#pragma once

#include <skipifzero.hpp>
#include <skipifzero_allocators.hpp>

#include "ZeroG.h"

// AllocatorWrapper
// ------------------------------------------------------------------------------------------------

// Small wrapper around ZgAllocator (C-API) to convert it to an sfz::Allocator
class AllocatorWrapper final : public sfz::Allocator {
public:

	static AllocatorWrapper createDefaultAllocator() {
		AllocatorWrapper tmp;
		tmp.mInited = true;
		tmp.mHasUserDefinedAllocator = false;
		return tmp;
	}

	static AllocatorWrapper createWrapper(ZgAllocator zgAllocator) {
		AllocatorWrapper tmp;
		tmp.mInited = true;
		tmp.mHasUserDefinedAllocator = true;
		tmp.mZgAllocator = zgAllocator;
		return tmp;
	}

	bool isInitialized() const { return mInited; }

	void* allocate(SfzDbgInfo dbg, uint64_t size, uint64_t alignment) noexcept override final
	{
		sfz_assert(mInited);
		if (mHasUserDefinedAllocator) {
			return mZgAllocator.allocate(
				mZgAllocator.userPtr, uint32_t(size), dbg.staticMsg, dbg.file, dbg.line);
		}
		else {
			return mStandardAllocator.allocate(dbg, size, alignment);
		}
	}

	void deallocate(void* pointer) noexcept override final
	{
		sfz_assert(mInited);
		if (pointer == nullptr) return;
		if (mHasUserDefinedAllocator) {
			mZgAllocator.deallocate(mZgAllocator.userPtr, pointer);
		}
		else {
			mStandardAllocator.deallocate(pointer);
		}
	}

private:
	bool mInited = false;
	bool mHasUserDefinedAllocator = false;
	ZgAllocator mZgAllocator = {};
	sfz::StandardAllocator mStandardAllocator;
};

// Context definition
// ------------------------------------------------------------------------------------------------

struct ZgContext final {
	AllocatorWrapper allocator;
	ZgLogger logger = {};
};

// Global implicit context accessor
// ------------------------------------------------------------------------------------------------

ZgContext& getContext() noexcept;

inline sfz::Allocator* getAllocator() noexcept { return &getContext().allocator; }
inline ZgLogger& getLogger() noexcept { return getContext().logger; }

void setContext(const ZgContext& context) noexcept;
