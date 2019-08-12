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

#include <sfz/memory/Allocator.hpp>

#include <ZeroG-cpp.hpp>

namespace ph {

// DynamicGpuAllocator
// ------------------------------------------------------------------------------------------------

struct PageInfo final {
	uint32_t pageSizeBytes = 0;
	uint32_t numAllocations = 0;
	uint32_t numFreeBlocks = 0;
	uint32_t largestFreeBlockBytes = 0;
};

struct DynamicGpuAllocatorState;

class DynamicGpuAllocator final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	DynamicGpuAllocator() noexcept = default;
	DynamicGpuAllocator(const DynamicGpuAllocator&) = delete;
	DynamicGpuAllocator& operator= (const DynamicGpuAllocator&) = delete;
	DynamicGpuAllocator(DynamicGpuAllocator&& o) noexcept { this->swap(o); }
	DynamicGpuAllocator& operator= (DynamicGpuAllocator&& o) noexcept { this->swap(o); return *this; }
	~DynamicGpuAllocator() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(sfz::Allocator* allocator) noexcept;
	void swap(DynamicGpuAllocator& other) noexcept;
	void destroy() noexcept;

	// State query methods
	// --------------------------------------------------------------------------------------------

	uint32_t queryTotalNumAllocationsDevice() const noexcept;
	uint32_t queryTotalNumAllocationsUpload() const noexcept;
	uint32_t queryTotalNumAllocationsTexture() const noexcept;

	uint32_t queryTotalNumDeallocationsDevice() const noexcept;
	uint32_t queryTotalNumDeallocationsUpload() const noexcept;
	uint32_t queryTotalNumDeallocationsTexture() const noexcept;

	uint32_t queryDefaultPageSizeDevice() const noexcept;
	uint32_t queryDefaultPageSizeUpload() const noexcept;
	uint32_t queryDefaultPageSizeTexture() const noexcept;

	uint32_t queryNumPagesDevice() const noexcept;
	uint32_t queryNumPagesUpload() const noexcept;
	uint32_t queryNumPagesTexture() const noexcept;

	PageInfo queryPageInfoDevice(uint32_t pageIdx) const noexcept;
	PageInfo queryPageInfoUpload(uint32_t pageIdx) const noexcept;
	PageInfo queryPageInfoTexture(uint32_t pageIdx) const noexcept;

	// Allocation methods
	// --------------------------------------------------------------------------------------------

	zg::Buffer allocateBuffer(ZgMemoryType memoryType, uint32_t sizeBytes) noexcept;

	zg::Texture2D allocateTexture2D(
		ZgTexture2DFormat format,
		uint32_t width,
		uint32_t height,
		uint32_t numMipmaps,
		uint32_t* allocSizeOut = nullptr) noexcept;
	
	// Deallocation methods
	// --------------------------------------------------------------------------------------------

	void deallocate(zg::Buffer& buffer) noexcept;

	void deallocate(zg::Texture2D& texture) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	DynamicGpuAllocatorState* mState = nullptr;
};

} // namespace ph
